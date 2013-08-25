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

#include <regex>
#include <future>
#include <threads.hpp>
#include <dlna_media.hpp>

namespace lan
{
	Log::Module APP { "APPL" };

	namespace item
	{

		struct ffmpeg_file : common_file
		{
			net::dlna::Item m_item;
			ffmpeg_file(av::MediaServer* device, const fs::path& path, const net::dlna::Item& item)
				: common_file(device, path)
				, m_item(item)
			{
			}
			const char* get_upnp_class() const override
			{
				switch (m_item.m_class)
				{
				case net::dlna::Class::Image: return "object.item.imageItem.photo";
				case net::dlna::Class::Audio: return "object.item.audioItem.musicTrack";
				case net::dlna::Class::Video: return "object.item.videoItem";
				}
				return "object.item";
			}
			bool is_image() const override { return true; }

			void           set_title(const std::string& title)      override { m_item.m_meta.m_title = title; }
			std::string    get_title() const                        override { return m_item.m_meta.m_title.empty() ? m_path.filename().string() : m_item.m_meta.m_title; }
			const net::dlna::ItemMetadata* get_metadata() const     override { return &m_item.m_meta; }
			const net::dlna::ItemProperties* get_properties() const override { return &m_item.m_props; }
			const net::dlna::Profile* get_profile() const           override { return &m_item.m_profile; }
		};

		std::shared_ptr<ffmpeg_file> create(av::MediaServer* device, const fs::path& path, net::dlna::Item& item)
		{
			return std::make_shared<ffmpeg_file>(device, path, item);
		}

		av::items::media_item_ptr from_path(av::MediaServer* device, const fs::path& path)
		{
			if (path.filename() == ".")
				return nullptr;

			net::dlna::Item item;
			if (!item.open(path))
				return nullptr;

			switch (item.m_class)
			{
			case net::dlna::Class::Video:
			case net::dlna::Class::Audio:
			case net::dlna::Class::Image:
				return create(device, path, item);
			case net::dlna::Class::Container:
				{
					auto ret = std::make_shared<directory_item>(device, path);

					fs::path cover = path / "Folder.jpg";
					if (fs::exists(cover))
						ret->set_cover(cover);

					return ret;
				}
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
			{
				auto profile = get_profile();
				if (profile)
					return media::from_file(get_path(), profile->m_mime, true);
				return media::from_file(get_path(), true);
			}

			return m_cover;
		}

		void common_file::output(std::ostream& o, const std::vector<std::string>& filter, const net::ssdp::import::av::client_interface_ptr& client, const net::config::config_ptr& config) const
		{
			output_open(o, filter, 0);
			attrs(o, filter, config);
			output_close(o, filter, client, config);
		}

		bool contains(const std::vector<std::string>& filter, const char* key)
		{
			if (filter.empty()) return true;
			return std::find(filter.begin(), filter.end(), key) != filter.end();
		}

#pragma region container_file

		container_file::container_type container_file::list(net::ulong start_from, net::ulong max_count)
		{
			rescan_if_needed();
			auto future = async_list(start_from, max_count);
			auto data = std::move(future.get());
			//log::info() << "Got slice " << get_filename().filename() << " (" << start_from << ", " << max_count << ")";
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
				log::info() << "Returning slice " << get_path().filename() << " (" << start_from << ", " << start_from + max_count << ")";
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

		void container_file::output(std::ostream& o, const std::vector<std::string>& filter, const net::ssdp::import::av::client_interface_ptr& client, const net::config::config_ptr& config) const
		{
			output_open(o, filter, m_children.size());
			output_close(o, filter, client, config);
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
			child->set_objectId_attr(get_raw_objectId_attr() + av::items::SEP + std::to_string(id));
			child->set_parent_attr(get_objectId_attr());

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

		bool less(const fs::path& lhs, const fs::path& rhs)
		{
			bool lhs_dir = fs::is_directory(lhs);
			bool rhs_dir = fs::is_directory(rhs);

			if (lhs_dir != rhs_dir)
				return lhs_dir; // directories before files

			return lhs < rhs;
		}

		struct statistics
		{
			long long start;
			static statistics* ptr;

			statistics()
			{
				ptr = this;
				LARGE_INTEGER S;
				QueryPerformanceCounter(&S);
				start = S.QuadPart;
			}
			~statistics()
			{
				ptr = nullptr;
				LARGE_INTEGER stop, F;
				QueryPerformanceCounter(&stop);
				QueryPerformanceFrequency(&F);

				now(stop.QuadPart - start, F.QuadPart, " total");
			}

			static void now(long long ticks, long long second, const char* msg)
			{
				long long seconds = ticks / second;
				const char* freq = "s";
				log::warning log;
				if (seconds > 59)
				{
					ticks -= (seconds / 60) * 60 * second;
					log << (seconds / 60) << 'm';
				}
				else
				{
					if (ticks * 10 / second < 9)
					{
						freq = "ms";
						ticks *= 1000;
						if (ticks * 10 / second < 9)
						{
							freq = "us";
							ticks *= 1000;
						}
					}
				}
				log << (ticks / second) << '.' << std::setw(3) << std::setfill('0') << ((ticks * 1000 / second) % 1000) << freq << msg;
			}
		};

		statistics* statistics::ptr = nullptr;

		struct sub_stat
		{
			long long start;
			const fs::path& path;

			sub_stat(const fs::path& path)
				: path(path)
			{
				LARGE_INTEGER S;
				QueryPerformanceCounter(&S);
				start = S.QuadPart;
			}
			~sub_stat()
			{
				if (!statistics::ptr) return;
				LARGE_INTEGER stop, F;
				QueryPerformanceCounter(&stop);
				QueryPerformanceFrequency(&F);
				long long ticks = stop.QuadPart - start;
				if (ticks / 2 > F.QuadPart) // it took more than 2s?
					statistics::now(ticks, F.QuadPart, (" for " + path.filename().string()).c_str());
			}
		private:
			sub_stat& operator=(const sub_stat&);
		};

		void directory_item::rescan()
		{
			statistics stats;

			log::info() << "Scanning " << m_path;

			m_last_scan = fs::last_write_time(m_path);

			std::vector<std::pair<fs::path, int>> entries;
			for (auto && file : contents(m_path))
				entries.emplace_back(file.path(), 1);

			std::vector<std::pair<fs::path, av::items::media_item_ptr>> current;
			for (auto && ptr : m_children)
				current.emplace_back(get_path(ptr), ptr);

			std::sort(entries.begin(), entries.end(), [](const std::pair<fs::path, int>& lhs, const std::pair<fs::path, int>& rhs) { return less(lhs.first, rhs.first); });
			std::sort(current.begin(), current.end(), [](const std::pair<fs::path, av::items::media_item_ptr>& lhs, const std::pair<fs::path, av::items::media_item_ptr>& rhs) { return less(lhs.first, rhs.first); });

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
					sub_stat sub(entry.first);
					auto item = from_path(m_device, entry.first);
					if (item)
						add_child(item);
				}

				log::info() << "Finished scanning " << m_path << "; found " << m_children.size() << " item(s)";
		}
	}
}
