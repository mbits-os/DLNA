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
#include "fs_items.hpp"
#include <MediaInfo.h>
#include <regex>
#include <future>
#include <threads.hpp>

#pragma comment(lib, "mi.lib")

namespace mi = MediaInfo;

namespace lan
{
	Log::Module APP { "APPL" };

	struct MI
	{
		static bool extract(const fs::path& file, mi::IContainer* env)
		{
			return get()->extract(fs::absolute(file).native(), env);
		}
	private:
		static std::shared_ptr<mi::API> get()
		{
			static auto s_mi = std::make_shared<mi::API>();
			return s_mi;
		}
	};

	namespace item
	{
		namespace Media
		{
			enum class Class
			{
				Other,
				Image,
				Audio,
				Video
			};

#define TPROPERTY(type, name) \
		private: \
		type m_##name; \
		public: \
		type get_##name() const override { /*std::cout << "Reading " #name ": " << m_ ## name << "\n";*/ return m_ ## name; }\
		bool set_##name(type val) override { /*std::cout << "Setting " #name ": " << val << "\n";*/ m_ ## name = val; return true; }
#define SPROPERTY(name) \
		private: \
		std::string m_##name; \
		public: \
		const std::string& get_##name() const override { /*std::cout << "Reading " #name ": " << m_ ## name << "\n";*/ return m_ ## name; }\
		const char* get_##name##_c() const override { return m_ ## name.c_str(); }\
		bool set_##name(const char* val) override { /*std::cout << "Setting " #name ": " << val << "\n";*/ m_ ## name = val; return true; }

			struct MediaTrack : mi::ITrack
			{
				mi::TrackType m_type;
				int m_id;

				MediaTrack(mi::TrackType type, int id)
					: m_type(type)
					, m_id(id)
					, m_duration(0)
					, m_bitrate(0)
					, m_sample_freq(0)
					, m_channels(0)
					, m_width(0)
					, m_height(0)
					, m_track_position(0)
					, m_ref_frame_count(0)
				{
				}
				mi::TrackType get_type() const override { return m_type; }
				int get_id() const override { return m_id; }

				TPROPERTY(unsigned long, duration);
				TPROPERTY(unsigned long, bitrate);
				TPROPERTY(unsigned long, sample_freq);
				TPROPERTY(unsigned long, channels);
				TPROPERTY(unsigned long, width);
				TPROPERTY(unsigned long, height);
				SPROPERTY(mime);
				SPROPERTY(title);
				SPROPERTY(artist);
				SPROPERTY(album);
				SPROPERTY(genre);
				SPROPERTY(cover);
				TPROPERTY(int, track_position);
				TPROPERTY(int, ref_frame_count);
				SPROPERTY(format);
			};
			typedef std::shared_ptr<MediaTrack> track_ptr;

			struct MetadataContainer : mi::IContainer, MediaTrack
			{
				Class m_class;
				std::vector<track_ptr> m_tracks;
				MetadataContainer()
					: m_class(Class::Other)
					, MediaTrack(mi::TrackType::General, 0)
				{
				}

				Class fileClass() const { return m_class; }

				mi::ITrack* create_track(mi::TrackType type, int id) override
				{
					switch (type)
					{
					case mi::TrackType::Image:
					case mi::TrackType::Audio:
					case mi::TrackType::Video:
						break;
					case mi::TrackType::General:
						return this;
					default:
						return nullptr;
					}

					auto ptr = std::make_shared<MediaTrack>(type, id);
					if (!ptr)
						return nullptr;

					m_tracks.push_back(ptr);

					if (type == mi::TrackType::Image && m_class == Class::Other)
						m_class = Class::Image;

					if (type == mi::TrackType::Audio && m_class != Class::Video)
						m_class = Class::Audio;

					if (type == mi::TrackType::Video)
						m_class = Class::Video;

					return ptr.get();
				}
				size_t get_length() const override { return 1 + m_tracks.size(); }
				ITrack* get_item(size_t track) const override { return track == 0 ? const_cast<MetadataContainer*>(this) : m_tracks[track - 1].get(); }
			};
		}

		template <typename T> struct track_type;
		template <> struct track_type<video_file> { enum { value = MediaInfo::TrackType::Video }; };
		template <> struct track_type<audio_file> { enum { value = MediaInfo::TrackType::Audio }; };
		template <> struct track_type<photo_file> { enum { value = MediaInfo::TrackType::Image }; };

#define GET(name) \
	auto name = env.get_ ## name(); \
	if (!name) name = primary->get_ ## name(); \
	if (!name && secondary) name = secondary->get_ ## name()
#define GETS(name) \
	auto name = env.get_ ## name(); \
	if (name.empty()) name = primary->get_ ## name(); \
	if (name.empty() && secondary) name = secondary->get_ ## name()

#define MOVE(name) \
	GET(name); \
	item->set_ ## name(name);

#define MOVES(name) \
	GETS(name); \
	item->set_ ## name(name);

		template <typename T>
		void item_specific(T* item, Media::MetadataContainer& env, const Media::track_ptr& primary, const Media::track_ptr& secondary)
		{
		}

		void item_specific(video_file* item, Media::MetadataContainer& env, const Media::track_ptr& primary, const Media::track_ptr& secondary)
		{
			MOVE(width);
			MOVE(height);
			MOVE(ref_frame_count);

			if (!env.get_cover().empty()) item->set_cover(env.get_cover());
			else if (!primary->get_cover().empty()) item->set_cover(primary->get_cover());
		}

		void item_specific(audio_file* item, Media::MetadataContainer& env, const Media::track_ptr& primary, const Media::track_ptr& secondary)
		{
			MOVES(album);
			MOVES(artist);
			MOVES(genre);
			MOVE(track_position);

			MOVE(bitrate);
			MOVE(sample_freq);
			MOVE(channels);

			auto title = env.get_title();
			if (title.empty()) title = primary->get_title();
			if (!title.empty())
				item->set_title(title);

			if (!env.get_cover().empty()) item->set_cover(env.get_cover());
			else if (!primary->get_cover().empty()) item->set_cover(primary->get_cover());
		}

		void item_specific(photo_file* item, Media::MetadataContainer& env, const Media::track_ptr& primary, const Media::track_ptr& secondary)
		{
			MOVE(width);
			MOVE(height);
		}

		template <typename T>
		std::shared_ptr<T> create(av::MediaServer* device, const fs::path& path, Media::MetadataContainer& env)
		{
			Media::track_ptr primary, secondary;
			for (auto&& track : env.m_tracks)
			{
				if ((int)track->get_type() == track_type<T>::value)
				{
					primary = track;
					break;
				}
			}

			if (track_type<T>::value == (int) MediaInfo::TrackType::Video)
			{
				for (auto && track : env.m_tracks)
				{
					if (track->get_type() == MediaInfo::TrackType::Audio)
					{
						secondary = track;
						break;
					}
				}
			}

			if (!primary)
				return nullptr;

			GET(duration);
			GETS(mime);

			auto ret = std::make_shared<T>(device, path, duration);

			if (mime.empty())
				mime = "video/mpeg";
			ret->set_mime(mime);

			item_specific(ret.get(), env, primary, secondary);

			if (!ret->get_cover())
			{
				fs::path cover;
				if (env.fileClass() == Media::Class::Video ||
					env.fileClass() == Media::Class::Audio)
				{
					cover = path.string() + ".cover.png";
					if (!fs::exists(cover))
						cover = path.string() + ".cover.jpg";
				}

				if (fs::exists(cover))
					ret->set_cover(cover);
			}

			return ret;
		}

		av::items::media_item_ptr from_path(av::MediaServer* device, const fs::path& path)
		{
			if (fs::is_directory(path))
			{
				if (path.filename() == ".")
					return nullptr;
				auto ret = std::make_shared<directory_item>(device, path);

				fs::path cover = path / "Folder.jpg";
				if (fs::exists(cover))
					ret->set_cover(cover);

				return ret;
			}

			Media::MetadataContainer env;
			if (!MI::extract(path, &env))
			{
				log::error() << "Could not extract metadata from " << path;
			}

			switch (env.fileClass())
			{
			case Media::Class::Video: return create<video_file>(device, path, env);
			case Media::Class::Audio: return create<audio_file>(device, path, env);
			case Media::Class::Image: return create<photo_file>(device, path, env);
			}
			return nullptr;
		}

		struct embedded_cover;
		typedef std::shared_ptr<embedded_cover> embedded_cover_ptr;

		class referenced_content : public net::http::content
		{
			embedded_cover_ptr m_ref;
			std::size_t m_pointer;
		public:
			referenced_content(const embedded_cover_ptr& ref): m_ref(ref), m_pointer(0) {}
			bool can_skip() override { return true; }
			bool size_known() override { return true; }
			std::size_t get_size() override;
			std::size_t skip(std::size_t size) override;
			std::size_t read(void* buffer, std::size_t size) override;
		};

		struct embedded_cover : av::items::media, std::enable_shared_from_this<embedded_cover>
		{
			std::string m_mime;
			std::vector<char> m_text;

			bool prep_response(net::http::response& resp) override
			{
				auto& header = resp.header();
				header.append("content-type", m_mime);
				resp.content(std::make_shared<referenced_content>(shared_from_this()));

				return true;
			}
		};

		std::size_t referenced_content::get_size()
		{
			return m_ref->m_text.size();
		}

		std::size_t referenced_content::skip(std::size_t size)
		{
			auto rest = m_ref->m_text.size() - m_pointer;
			if (size > rest)
				size = rest;

			m_pointer += size;
			return size;
		}

		std::size_t referenced_content::read(void* buffer, std::size_t size)
		{
			auto rest = m_ref->m_text.size() - m_pointer;
			if (size > rest)
				size = rest;
			memcpy(buffer, m_ref->m_text.data() + m_pointer, size);
			m_pointer += size;
			return size;
		}

		int decode_char(char c)
		{
			static char alphabet [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
			for (int i = 0; i < sizeof(alphabet); ++i)
				if (alphabet[i] == c)
					return i;
			return -1;
		}

		void base64_decode(const std::string& base64, std::vector<char>& dst)
		{
			size_t len = base64.length();
			const char* p = base64.c_str();
			const char* e = p + len;

			size_t out_len = ((len + 3) >> 2) * 3;

			while (p != e && e[-1] == '=') { --e; --len; --out_len; }

			dst.resize(out_len);

			size_t pos = 0;
			unsigned int bits = 0;
			unsigned int accu = 0;
			for (; p != e; ++p)
			{
				int val = decode_char(*p);
				if (val < 0)
					continue;

				accu = (accu << 6) | (val & 0x3F);
				bits += 6;
				while (bits >= 8)
				{
					bits -= 8;
					dst[pos++] = ((accu >> bits) & 0xFF);
				}
			}
		}

		void path_item::set_cover(const std::string& base64)
		{
			auto ret = std::make_shared<embedded_cover>();

			base64_decode(base64, ret->m_text);
			ret->m_mime = "image/png"; // TODO: detect jpg

			m_cover = ret;
		}

		common_file::media_ptr common_file::get_media(bool main_resource)
		{
			if (main_resource)
				return media::from_file(get_path(), get_mime(), true);

			return m_cover;
		}

		void common_file::output(std::ostream& o, const std::vector<std::string>& filter, const net::config::config_ptr& config) const
		{
			output_open(o, filter, 0);
			attrs(o, filter, config);
			output_close(o, filter, config);
		}

		bool contains(const std::vector<std::string>& filter, const char* key)
		{
			if (filter.empty()) return true;
			return std::find(filter.begin(), filter.end(), key) != filter.end();
		}

		void audio_file::attrs(std::ostream& o, const std::vector<std::string>& filter, const net::config::config_ptr& config) const
		{
#define SDISPLAY(name, item) \
	auto name = get_##name(); \
	if (!name.empty() && contains(filter, item)) \
		o << "    <" item ">" << net::xmlencode(name) << "</" item ">\n";

			SDISPLAY(album, "upnp:album");
			SDISPLAY(artist, "upnp:artist");
			if (!artist.empty() && contains(filter, "dc:creator"))
				o << "    <dc:creator>" << net::xmlencode(artist) << "</dc:creator>\n";
			SDISPLAY(genre, "upnp:genre");
		}

#pragma region container_file

		container_file::container_type container_file::list(net::ulong start_from, net::ulong max_count)
		{
			rescan_if_needed();
			auto future = async_list(start_from, max_count);
			auto data = std::move(future.get());
			//log::info() << "Got slice " << get_path().filename() << " (" << start_from << ", " << max_count << ")";
			return data;
		}

		net::ulong container_file::predict_count(net::ulong served) const
		{
			std::lock_guard<std::mutex> lock(m_guard);
			if (is_running())
				return served + 1;
			return m_children.size();
		}

		void container_file::rescan_if_needed()
		{
			if (!rescan_needed())
				return;

			if (!mark_start())
				return; // someone was quicker...

			auto shared = shared_from_this();
			auto thread = std::thread([shared, this]{
				static std::atomic<int> count;
				threads::set_name("I/O Thread #" + std::to_string(count++));

				rescan();
				fulfill_promises(true); // release all remaining futures

				mark_stop();
			});

			thread.detach();
		}

		container_file::future_type container_file::async_list(net::ulong start_from, net::ulong max_count)
		{
			async_promise_type promise;
			auto ret = promise.get_future();

			{
				std::lock_guard<std::mutex> lock(m_guard);

				if (is_running())
				{
					m_tasks.emplace_back(std::move(promise), start_from, max_count);
				}
				else
				{
					fill_request(std::move(promise), start_from, max_count);
				}
			}

			return ret;
		}

		void container_file::fill_request(async_promise_type promise, net::ulong start_from, net::ulong max_count)
		{
			try
			{
				log::info() << "Returning slice " << get_path().filename() << " (" << start_from << ", " << max_count << ")";
				container_type out;
				if (start_from > m_children.size())
					start_from = m_children.size();

				auto end_at = m_children.size() - start_from;
				if (end_at > max_count)
					end_at = max_count;
				end_at += start_from;

				for (auto i = start_from; i < end_at; ++i)
					out.push_back(m_children.at(i));

				promise.set_value(out);
			}
			catch (...)
			{
				promise.set_exception(std::current_exception());
			}
		}

		void container_file::fulfill_promises(bool forced)
		{
			auto cur = m_tasks.begin();
			while (cur != m_tasks.end())
			{
				auto rest = m_children.size() - cur->m_start_from;
				if (forced || rest >= cur->m_max_count)
				{
					fill_request(std::move(cur->m_promise), cur->m_start_from, cur->m_max_count);
					cur = m_tasks.erase(cur);
					continue;
				}
				++cur;
			}
		}

		av::items::media_item_ptr container_file::get_item(const std::string& id)
		{
			av::items::media_item_ptr candidate;
			std::string rest_of_id;

			std::tie(candidate, rest_of_id) = av::items::find_item(m_children, id);

			if (!candidate || rest_of_id.empty())
				return candidate;

			if (!candidate->is_folder())
				return nullptr;

			return candidate->get_item(rest_of_id);
		}

		void container_file::output(std::ostream& o, const std::vector<std::string>& filter, const net::config::config_ptr& config) const
		{
			output_open(o, filter, m_children.size());
			output_close(o, filter, config);
		}

		void container_file::folder_changed()
		{
			::time(&m_update_id);
			m_device->object_changed();
		}
		void container_file::add_child(av::items::media_item_ptr child)
		{
			remove_child(child);

			std::lock_guard<std::mutex> lock(m_guard);

			m_children.push_back(child);
			auto id = ++m_current_max;
			child->set_id(id);
			child->set_objectId_attr(get_objectId_attr() + av::items::SEP + std::to_string(id));

			if (is_running())
				fulfill_promises(false);
		}

		void container_file::remove_child(av::items::media_item_ptr child)
		{
			std::lock_guard<std::mutex> lock(m_guard);
			folder_changed();

			auto pos = std::find(m_children.begin(), m_children.end(), child);
			if (pos != m_children.end())
				m_children.erase(pos);
		}
#pragma endregion

		void directory_item::check_updates()
		{
			if (rescan_needed())
				folder_changed();
		}
		bool directory_item::rescan_needed()
		{
			bool ret = m_last_scan != fs::last_write_time(m_path);
			if (ret)
				log::info() << "Scan needed in " << m_path;
			return ret;
		}
		void directory_item::rescan()
		{
			log::info() << "Scanning " << m_path;

			m_last_scan = fs::last_write_time(m_path);

			std::vector<std::pair<fs::path, int>> entries;
			for (auto && file : contents(m_path))
				entries.emplace_back(file.path(), 1);

			std::vector<std::pair<fs::path, av::items::media_item_ptr>> current;
			for (auto && ptr : m_children)
				current.emplace_back(get_path(ptr), ptr);
			auto update = m_update_id + 1;

			std::sort(entries.begin(), entries.end(), [](const std::pair<fs::path, int>& lhs, const std::pair<fs::path, int>& rhs) { return lhs.first < rhs.first; });
			std::sort(current.begin(), current.end(), [](const std::pair<fs::path, av::items::media_item_ptr>& lhs, const std::pair<fs::path, av::items::media_item_ptr>& rhs) { return lhs.first < rhs.first; });

			auto entry = entries.begin();
			auto curr = current.begin();

			while ((entry != entries.end()) && (curr != current.end()))
			{
				if (entry->first == curr->first)
				{
					entry->second = 0; // do not add
					curr->second = nullptr; // do not remove
					++entry;
					++curr;
				}
				else if (entry->first < curr->first)
				{
					++entry;
				}
				else
				{
					++curr;
				}
			}

			for (auto && curr : current)
				if (curr.second)
					remove_child(curr.second);

			for (auto && entry : entries)
				if (entry.second)
				{
					auto item = from_path(m_device, entry.first);
					if (item)
						add_child(item);
				}

			log::info() << "Finished scanning " << m_path;
		}
	}
}
