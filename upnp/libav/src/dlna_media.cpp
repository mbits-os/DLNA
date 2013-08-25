/*
 * Copyright (C) 2013 midnightBITS
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pch.h"
#include "dlna_media_internal.hpp"
#include <boost/filesystem/fstream.hpp>

namespace fs = boost::filesystem;

namespace net { namespace dlna {

	Log::Module DLNA {"DLNA"};

	init::init()
	{
		av_log_set_level(AV_LOG_QUIET);
		av_register_all();

		register_image_profiles();
		register_audio_profiles();
	}

	init::~init()
	{
	}

	struct av_format_contex
	{
		av_format_contex() : m_ctx(nullptr), m_ok(false), m_allocated(false) {}
		av_format_contex(const boost::filesystem::path& path)
			: m_ctx(nullptr), m_ok(false), m_allocated(false)
		{
			open(path);
		}
		~av_format_contex() { close(); }

		explicit operator bool() const { return m_ok; }

		bool open(const boost::filesystem::path& path)
		{
			close();
			m_ok = (avformat_open_input(&m_ctx, path.string().c_str(), nullptr, nullptr) == 0);
			return m_ok;
		}

		bool open_from_memory(const void* data, ::size_t length)
		{
			close();
			m_allocated = true;
			m_ctx = avformat_alloc_context();
			if (!m_ctx)
				return false;
			m_io = std::make_shared<io>(data, length);
			m_buffer = std::shared_ptr<unsigned char>(reinterpret_cast<unsigned char*>(av_malloc(8192)), &av_free);
			m_ioContext = std::shared_ptr<AVIOContext>(
				avio_alloc_context(m_buffer.get(), 8192, 0, reinterpret_cast<void*>(m_io.get()), io::read, nullptr, nullptr), &av_free
				);

			m_ctx->pb = m_ioContext.get();
			m_ok = (avformat_open_input(&m_ctx, "MEMORY", nullptr, nullptr) == 0);
			return m_ok;
		}

		bool find_stream_info()
		{
			if (m_ok)
				m_ok = (av_find_stream_info(m_ctx) == 0);
			return m_ok;
		}

		bool get_codecs(stream_codec& out)
		{
			if (!m_ok)
				return false;

			AVStream* audio_stream = nullptr;
			AVStream* video_stream = nullptr;
			AVStream** curr = m_ctx->streams;

			for (unsigned int i = 0; i < m_ctx->nb_streams; ++i, ++curr)
			{
				if (!audio_stream && (*curr)->codec->codec_type == AVMEDIA_TYPE_AUDIO)
				{
					audio_stream = *curr;
					if (video_stream)
						break;
				}
				else if (!video_stream && (*curr)->codec->codec_type == AVMEDIA_TYPE_VIDEO)
				{
					video_stream = *curr;
					if (audio_stream)
						break;
				}
			}
			if (!audio_stream && !video_stream)
				return false;

			out.m_audio.fill(audio_stream);
			out.m_video.fill(video_stream);
			return true;
		}

		container::container_type get_container()
		{
			if (!m_ok)
				return container::UNKNOWN;

			static const struct
			{
				const char *name;
				container::container_type type;
			}
			format_mapping [] =
			{
				{ "image2",                  container::IMAGE     },
				{ "gif",                     container::IMAGE     },
				{ "asf",                     container::ASF       },
				{ "amr",                     container::AMR       },
				{ "aac",                     container::AAC       },
				{ "ac3",                     container::AC3       },
				{ "mp3",                     container::MP3       },
				{ "wav",                     container::WAV       },
				{ "mov,mp4,m4a,3gp,3g2,mj2", container::MOV       },
				{ "mpeg",                    container::FF_MPEG   },
				{ "mpegts",                  container::FF_MPEG_TS}
			};

			for (auto && mapping : format_mapping)
			{
				if (!std::strcmp(mapping.name, m_ctx->iformat->name))
				{
					switch (mapping.type)
					{
					case container::FF_MPEG:
					case container::FF_MPEG_TS:
						return get_mpeg_container(m_ctx->filename);
					case container::MOV:
						return get_mov_container(m_ctx->filename);
					default:
						return mapping.type;
					}
				}
			}
			return container::UNKNOWN;
		}

		bool get_metadata(ItemMetadata& meta)
		{
			if (!m_ok || !m_ctx->metadata)
				return false;

			AVDictionaryEntry *t = nullptr;
			while ((t = av_dict_get(m_ctx->metadata, "", t, AV_DICT_IGNORE_SUFFIX)) != nullptr)
			{
				if (!strcmp(t->key, "title")) meta.m_title = t->value;
				else if (!strcmp(t->key, "artist")) meta.m_artist = t->value;
				else if (!strcmp(t->key, "album_artist")) meta.m_album_artist = t->value;
				else if (!strcmp(t->key, "composer")) meta.m_composer = t->value;
				else if (!strcmp(t->key, "date")) meta.m_date = t->value;
				else if (!strcmp(t->key, "comment")) meta.m_comment = t->value;
				else if (!strcmp(t->key, "album")) meta.m_album = t->value;
				else if (!strcmp(t->key, "genre")) meta.m_genre = t->value;
				else if (!strcmp(t->key, "track")) meta.m_track = atoi(t->value);
			}

			return true;
		}

		bool get_properties(ItemProperties& props)
		{
			if (!m_ok)
				return false;

			stream_codec codecs;
			if (!get_codecs(codecs))
				return false;

			props.clear();
			props.m_duration    = (net::ulong)(m_ctx->duration / AV_TIME_BASE);
			props.m_bitrate     = (net::ulong) (m_ctx->bit_rate / 8);
			if (codecs.m_audio.m_codec)
			{
				props.m_sample_freq = codecs.m_audio.m_codec->sample_rate;
				props.m_bps = codecs.m_audio.m_codec->bits_per_raw_sample;
				props.m_channels = codecs.m_audio.m_codec->channels;
			}
			if (codecs.m_video.m_codec)
			{
				props.m_width = codecs.m_video.m_codec->width;
				props.m_height = codecs.m_video.m_codec->height;
			}
			return true;
		}

		const Profile* guess_profile(const boost::filesystem::path& dbg_path)
		{
			stream_codec codecs;
			if (!get_codecs(codecs))
				return nullptr;

			auto container = get_container();
			if (container == dlna::container::UNKNOWN)
			{
				log::error() << "can't find container description: " << dbg_path.string().c_str();
				return false;
			}

			auto profile = profile_db::guess_profile(m_ctx, container, codecs);
			if (!profile)
			{
				log::error log;
				log << "can't find profile: " << dbg_path.string().c_str();
				log << "\n  CONTAINER: " << container << "; streams=" << m_ctx->nb_streams;
				if (codecs.m_video.m_codec)
				{
					log << "\n  VIDEO: " << codecs.m_video.m_codec->codec_id << "; " << codecs.m_video.m_codec->width << "x" << codecs.m_video.m_codec->height;
				}
				if (codecs.m_audio.m_codec)
				{
					log << "\n  AUDIO: " << codecs.m_video.m_codec->codec_id << "; channels=" << codecs.m_audio.m_codec->channels
						<< "; sample_rate=" << codecs.m_audio.m_codec->sample_rate << "; bit_rate=" << codecs.m_audio.m_codec->bit_rate;
				}
				return nullptr;
			}

			return profile;
		}

		bool get_cover(std::vector<char>& dst)
		{
			stream_codec codecs;
			if (!get_codecs(codecs))
				return false;

			auto container = get_container();
			if (container == dlna::container::UNKNOWN)
				return false;

			if (!stream_has_cover(m_ctx, container, codecs))
				return false;

			auto att = codecs.m_video.m_stream->attached_pic;

			dst.resize(att.size);
			if (dst.size() != (size_t)att.size)
				return false;

			memcpy((unsigned char*) dst.data(), att.data, dst.size());
			return true;
		}

		void close()
		{
			if (m_ctx)
			{
				av_close_input_file(m_ctx);
				if (m_allocated)
					avformat_free_context(m_ctx);
			}
			m_buffer.reset();
			m_ioContext.reset();
			m_ctx = nullptr;
			m_ok = false;
			m_allocated = false;
		}

		AVFormatContext* get() const { return m_ctx; }

	private:
		struct io
		{
			const uint8_t* data;
			const uint8_t* end;

			::size_t _read(uint8_t* ptr, ::size_t len)
			{
				::size_t rest = end - data;
				if (rest > len)
					rest = len;
				memcpy(ptr, data, rest);
				data += rest;
				return rest;
			}

			io(const void* data, ::size_t length)
			{
				this->data = (const uint8_t*) data;
				this->end = this->data + length;
			}

			static int read(void* opaque, uint8_t* buf, int buf_size)
			{
				io* _this = reinterpret_cast<io*>(opaque);
				return (int) _this->_read(buf, buf_size);
			}
		};

		AVFormatContext* m_ctx;
		bool m_ok;
		bool m_allocated;
		std::shared_ptr<unsigned char> m_buffer;
		std::shared_ptr<AVIOContext> m_ioContext;
		std::shared_ptr<io> m_io;

		container::container_type get_mpeg_container(const fs::path& path)
		{
			static const unsigned char MPEG_PACK_HEADER = 0xBA;
			static const unsigned char MPEG_TS_SYNC_CODE = 0x47;
			static const size_t        MPEG_TS_PACKET_LENGTH = 188;
			static const size_t        MPEG_TS_PACKET_LENGTH_DLNA = MPEG_TS_PACKET_LENGTH + 4;

			unsigned char buffer[2 * MPEG_TS_PACKET_LENGTH_DLNA + 1];

			{
				fs::ifstream f(path, std::ios::in | std::ios::binary);
				f.read((char*)buffer, sizeof(buffer));
				if (f.gcount() != sizeof(buffer))
					return container::UNKNOWN;
			}

			for (size_t i = 0; i < MPEG_TS_PACKET_LENGTH_DLNA; i++)
			{
				if (buffer[i] == MPEG_TS_SYNC_CODE)
				{
					if (buffer[i + MPEG_TS_PACKET_LENGTH == MPEG_TS_SYNC_CODE])
						return container::MPEG_TRANSPORT_STREAM;
					if (buffer[i + MPEG_TS_PACKET_LENGTH_DLNA == MPEG_TS_SYNC_CODE])
					{
						//timestamp == 0x00000000?
						for (size_t j = 1; j < 4; ++j)
							if (buffer[i + j]) return container::MPEG_TRANSPORT_STREAM_DLNA;

						return container::MPEG_TRANSPORT_STREAM_DLNA_NO_TS;
					}
				}
			}

			if (buffer[0] == 0x00 && buffer[1] == 0x00 && buffer[2] == 0x01)
			{
				return (buffer[3] == MPEG_PACK_HEADER)
					? container::MPEG_PROGRAM_STREAM
					: container::MPEG_ELEMENTARY_STREAM;
			}

			return container::UNKNOWN;
		}

		container::container_type get_mov_container(const fs::path& path)
		{
			auto ext = path.extension();
			if (ext == "3gp" ||
				ext == "3gpp" ||
				ext == "3g2")
				return container::_3GP;
			return path.empty() ? container::UNKNOWN : container::MP4;
		}
	};

	const Profile* Profile::guess_from_file(const boost::filesystem::path& path)
	{
		if (fs::is_directory(path))
			return nullptr;

		av_format_contex ctx { path };
		if (!ctx)
			return nullptr;

		if (!ctx.find_stream_info())
			return nullptr;

		return ctx.guess_profile(path);
	}

	const Profile* Profile::guess_from_memory(const void* data, ::size_t length)
	{
		av_format_contex ctx;
		if (!ctx.open_from_memory(data, length))
			return nullptr;

		if (!ctx.find_stream_info())
			return nullptr;

		return ctx.guess_profile("MEMORY");
	}

	bool Item::open(const boost::filesystem::path& path)
	{
		m_profile.clear();
		m_meta.clear();

		boost::system::error_code ec;
		m_props.m_last_write_time = fs::last_write_time(path, ec);
		if (ec) m_props.m_last_write_time = 0;

		if (fs::is_directory(path))
		{
			m_meta.m_title = path.filename().string();
			if (m_meta.m_title == ".")
				return false;

			m_props.clear();
			m_class = Class::Container;

			return true;
		}

		m_class = Class::Unknown;
		av_format_contex ctx { path };
		if (!ctx)
			return false;

		if (!ctx.find_stream_info())
			return false;

		auto profile = ctx.guess_profile(path);
		if (!profile)
			return false;

		if (!ctx.get_cover(m_cover))
			m_cover.clear();

		m_profile = *profile;
		m_class = m_profile.m_class;
		ctx.get_metadata(m_meta);
		ctx.get_properties(m_props);
		m_props.m_size = fs::file_size(path, ec);
		if (ec) m_props.m_size = 0;

		return true;
	}
}}
