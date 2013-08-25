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

#ifndef __DLNA_MEDIA_INTERNAL_HPP__
#define __DLNA_MEDIA_INTERNAL_HPP__

#include <dlna_media.hpp>
#include <log.hpp>
#include <vector>

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable: 4244)
#endif

extern "C" {
#	include <libavcodec/avcodec.h>
#	include <libavformat/avformat.h>
}

#ifdef _MSC_VER
#	pragma warning(pop)

#	pragma comment(lib, "avformat.lib")
#	pragma comment(lib, "avcodec.lib")
#	pragma comment(lib, "avutil.lib")
#endif

namespace net
{
	namespace dlna
	{
		extern Log::Module DLNA;
		struct log : public Log::basic_log<log>
		{
			static const Log::Module& module() { return DLNA; }
		};
		namespace mime
		{
			static const char* IMAGE_JPEG          = "image/jpeg";
			static const char* IMAGE_PNG           = "image/png";
			static const char* IMAGE_GIF           = "image/gif";
			static const char* IMAGE_BMP           = "image/bmp";

			static const char* AUDIO_3GP           = "audio/3gpp";
			static const char* AUDIO_ADTS          = "audio/vnd.dlna.adts";
			static const char* AUDIO_ATRAC         = "audio/x-sony-oma";
			static const char* AUDIO_DOLBY_DIGITAL = "audio/vnd.dolby.dd-raw";
			static const char* AUDIO_LPCM          = "audio/L16";
			static const char* AUDIO_MPEG          = "audio/mpeg";
			static const char* AUDIO_MPEG_4        = "audio/mp4";
			static const char* AUDIO_WMA           = "audio/x-ms-wma";

			static const char* VIDEO_3GP           = "video/3gpp";
			static const char* VIDEO_ASF           = "video/x-ms-asf";
			static const char* VIDEO_MPEG          = "video/mpeg";
			static const char* VIDEO_MPEG_4        = "video/mp4";
			static const char* VIDEO_MPEG_TS       = "video/vnd.dlna.mpeg-tts";
			static const char* VIDEO_WMV           = "video/x-ms-wmv";
		}

		namespace labels
		{
			static const char* IMAGE_PICTURE    = "picture";
			static const char* IMAGE__ICON      = "icon";

			static const char* AUDIO_MONO       = "mono";
			static const char* AUDIO_2CH        = "2-ch";
			static const char* AUDIO_2CH_MULTI  = "2-ch multi";
			static const char* AUDIO_MULTI      = "multi";

			static const char* VIDEO_CIF15      = "CIF15";
			static const char* VIDEO_CIF30      = "CIF30";
			static const char* VIDEO_QCIF15     = "QCIF15";
			static const char* VIDEO_SD         = "SD";
			static const char* VIDEO_HD         = "HD";
		}

		namespace audio
		{
			enum class profile
			{
				INVALID = 0,

				/* Advanced Audio Codec variants */
				AAC,
				AAC_320,
				AAC_MULT5,
				AAC_BSAC,
				AAC_BSAC_MULT5,
				AAC_HE_L2,
				AAC_HE_L2_320,
				AAC_HE_L3,
				AAC_HE_MULT5,
				AAC_HE_V2_L2,
				AAC_HE_V2_L2_320,
				AAC_HE_V2_L3,
				AAC_HE_V2_MULT5,
				AAC_LTP,
				AAC_LTP_MULT5,
				AAC_LTP_MULT7,

				AC3,
				AC3_EXTENDED,

				AMR,
				AMR_WB,

				ATRAC,

				G726,

				LPCM,

				/* MPEG audio variants */
				MP2,
				MP3,
				MP3_EXTENDED,
				MP3_UPNP,
				MP3_EXTENDED_UPNP,

				/* Windows Media Audio variants */
				WMA_BASELINE,
				WMA_FULL,
				WMA_PRO
			};
		}

		namespace container
		{
			enum container_type
			{
				UNKNOWN,
				IMAGE, /* for PNG and JPEG */
				ASF, /* usually for WMA/WMV */
				AMR,
				AAC,
				AC3,
				MP3,
				WAV,
				MOV,
				_3GP,
				MP4,
				FF_MPEG,    /* FFMPEG "mpeg" */
				FF_MPEG_TS, /* FFMPEG "mpegts" */
				MPEG_ELEMENTARY_STREAM,
				MPEG_PROGRAM_STREAM,
				MPEG_TRANSPORT_STREAM,
				MPEG_TRANSPORT_STREAM_DLNA,
				MPEG_TRANSPORT_STREAM_DLNA_NO_TS,
			};

			inline std::ostream& operator <<(std::ostream& o, container_type container)
			{
#define ENUM_STR(x) case x: return o << #x
				switch (container)
				{
					ENUM_STR(UNKNOWN);
					ENUM_STR(IMAGE);
					ENUM_STR(ASF);
					ENUM_STR(AMR);
					ENUM_STR(AAC);
					ENUM_STR(AC3);
					ENUM_STR(MP3);
					ENUM_STR(WAV);
					ENUM_STR(MOV);
					ENUM_STR(_3GP);
					ENUM_STR(MP4);
					ENUM_STR(FF_MPEG);
					ENUM_STR(FF_MPEG_TS);
					ENUM_STR(MPEG_ELEMENTARY_STREAM);
					ENUM_STR(MPEG_PROGRAM_STREAM);
					ENUM_STR(MPEG_TRANSPORT_STREAM);
					ENUM_STR(MPEG_TRANSPORT_STREAM_DLNA);
					ENUM_STR(MPEG_TRANSPORT_STREAM_DLNA_NO_TS);
				default: return o << "UNKNOWN-CONTAINER(" << (int) container << ")";
				}
#undef ENUM_STR
			}
		}

		struct ff_codec
		{
			AVStream *m_stream;
			AVCodecContext *m_codec;

			ff_codec()
				: m_stream(nullptr)
				, m_codec(nullptr)
			{}

			void fill(AVStream* stream)
			{
				m_stream = stream;
				m_codec = stream ? stream->codec : nullptr;
			}

			explicit operator bool() { return m_stream != nullptr; }
		};

		struct stream_codec
		{
			ff_codec m_video;
			ff_codec m_audio;
		};

		static inline bool stream_is_image(AVFormatContext *ctx, container::container_type container, const stream_codec& codecs)
		{
			return
				ctx->nb_streams == 1 &&
				codecs.m_video.m_codec != nullptr &&
				container == dlna::container::IMAGE;
		}

		static inline bool stream_has_cover(AVFormatContext *, container::container_type, const stream_codec& codecs)
		{
			if (codecs.m_video.m_codec == nullptr || codecs.m_video.m_stream == nullptr)
				return false;
			return
				(codecs.m_video.m_stream->disposition & AV_DISPOSITION_ATTACHED_PIC) == AV_DISPOSITION_ATTACHED_PIC;
		}

		static inline bool stream_is_audio(AVFormatContext * ctx, container::container_type container, const stream_codec& codecs)
		{
			return
				codecs.m_audio.m_codec != nullptr && 
				(codecs.m_video.m_codec == nullptr || stream_has_cover(ctx, container, codecs));
		}

		static inline bool stream_is_video(AVFormatContext *, container::container_type, const stream_codec& codecs)
		{
			return
				codecs.m_video.m_codec != nullptr &&
				codecs.m_video.m_stream != nullptr &&
				codecs.m_audio.m_codec != nullptr &&
				codecs.m_audio.m_stream != nullptr;
		}

		struct profile_db
		{
			typedef const Profile * (*call_type)(AVFormatContext *ctx, container::container_type container, const stream_codec& codecs);

			static void register_profile(call_type call, const char* exts)
			{
				get().m_probes.emplace_back(call, exts);
			}
			static const Profile * guess_profile(AVFormatContext *ctx, container::container_type container, const stream_codec& codecs)
			{
				auto& probes = get().m_probes;
				for (auto && probe : probes)
				{
					auto ret = probe.probe(ctx, container, codecs);
					if (ret)
						return ret;
				}
				return nullptr;
			}
		private:
			struct profile_probe
			{

				call_type m_call;
				const char* m_extensions;

				profile_probe(call_type call, const char* exts)
					: m_call(call)
					, m_extensions(exts)
				{
				}

				const Profile * probe(AVFormatContext *ctx, container::container_type container, const stream_codec& codecs) const
				{
					if (!m_call)
						return nullptr;
					return m_call(ctx, container, codecs);
				}
			};

			profile_db() {};
			static profile_db& get()
			{
				static profile_db obj;
				return obj;
			}

			std::vector<profile_probe> m_probes;
		};

		struct boundary {
			Profile* m_profile;
			int m_max_width;
			int m_max_height;

			inline bool encases(const AVCodecContext* codec) const
			{
				return
					codec->width <= m_max_width &&
					codec->height <= m_max_height;
			}
		};

		template <typename T1, typename T2>
		struct codec_ids_or
		{
			static bool contains(AVCodecID id)
			{
				return T1::contains(id) || T2::contains(id);
			}
		};

		template <AVCodecID tmplt, AVCodecID... args>
		struct codec_id_list : codec_ids_or<codec_id_list<tmplt>, codec_id_list<args...>>
		{
		};

		template <AVCodecID tmplt>
		struct codec_id_list<tmplt>
		{
			static inline bool contains(AVCodecID id)
			{
				return tmplt == id;
			}
		};

		namespace image
		{
			template <typename Final, AVCodecID... list>
			struct image_module
			{
				template <size_t len>
				static inline const Profile* probe(const boundary(&boundaries)[len], const AVCodecContext* codec)
				{
					for (const auto& boundary : boundaries)
					{
						if (boundary.encases(codec))
							return boundary.m_profile;
					}

					return nullptr;
				}

				template <size_t len>
				static inline const Profile * profiles(const boundary(&boundaries)[len], AVFormatContext *ctx, container::container_type container, const stream_codec& codecs)
				{
					if (!stream_is_image(ctx, container, codecs))
						return nullptr;

					if (!codec_id_list<list...>::contains(codecs.m_video.m_codec->codec_id))
						return nullptr;

					return probe(boundaries, codecs.m_video.m_codec);
				}

				static void register_profiles(const char* exts)
				{
					profile_db::register_profile(Final::probe, exts);
				}
			};
		}

		void register_image_profiles();
		void register_audio_profiles();
	}
}

#include "codec_ostream.hpp"

#endif //__DLNA_MEDIA_INTERNAL_HPP__
