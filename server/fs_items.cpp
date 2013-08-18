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
			struct MediaTrack : mi::ITrack
			{
				mi::TrackType m_type;
				int m_id;
				MediaTrack(mi::TrackType type, int id)
				{
				}
				mi::TrackType get_type() const override { return m_type; }
				int get_id() const override { return m_id; }
			};
			typedef std::shared_ptr<MediaTrack> track_ptr;

			struct MediaEnvelope : mi::IContainer, mi::ITrack
			{
				Class m_class;
				std::vector<track_ptr> m_tracks;
				MediaEnvelope()
					: m_class(Class::Other)
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

				mi::TrackType get_type() const override { return mi::TrackType::General; }
				int get_id() const override { return 0; }
			};
		}

		av::items::media_item_ptr from_path(av::MediaServer* device, const fs::path& path)
		{
			if (fs::is_directory(path))
			{
				if (path.filename() == ".")
					return nullptr;
				return std::make_shared<directory_item>(device, path);
			}

			Media::MediaEnvelope env;
			if (!MI::extract(path, &env))
			{
				log::error() << "Could not extract metadata from " << path;
			}

			switch (env.fileClass())
			{
			case Media::Class::Video: return std::make_shared<video_file>(device, path);
			case Media::Class::Audio: return std::make_shared<audio_file>(device, path);
			case Media::Class::Image: return std::make_shared<photo_file>(device, path);
			}
			return nullptr;
		}

		void common_file::output(std::ostream& o, const std::vector<std::string>& filter) const
		{
			output_open(o, filter, 0);
			output_close(o, filter);
		}

#pragma region container_file

		container_file::container_type container_file::list(net::ulong start_from, net::ulong max_count, const av::items::sort_criteria& sort)
		{
			rescan_if_needed();
			auto future = async_list(start_from, max_count, sort);
			auto data = std::move(future.get());
			log::info() << "Got slice " << get_path().filename() << " (" << start_from << ", " << max_count << ")";
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
			if (!rescan_is_needed())
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

		container_file::future_type container_file::async_list(net::ulong start_from, net::ulong max_count, const av::items::sort_criteria& sort)
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

					if (!sort.empty())
					{
						// TODO: sort the result
					}
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

		void container_file::output(std::ostream& o, const std::vector<std::string>& filter) const
		{
			output_open(o, filter, m_children.size());
			output_close(o, filter);
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
			if (rescan_is_needed())
				folder_changed();
		}
		bool directory_item::rescan_is_needed()
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
