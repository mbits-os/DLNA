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

#ifdef _WIN32
#include <sdkddkver.h>
#endif

#ifndef __FS_ITEMS_HPP__
#define __FS_ITEMS_HPP__

#include <media_server.hpp>
#include <boost/filesystem.hpp>
#include <log.hpp>
#include <mutex>
#include <future>

namespace fs = boost::filesystem;
namespace av = net::ssdp::import::av;


namespace lan
{
	extern Log::Module APP;
	struct log : public Log::basic_log<log>
	{
		static const Log::Module& module() { return APP; }
	};

	namespace item
	{
#define ITEM_PROP(type, name) \
	private: \
	type m_##name; \
	public: \
	type get_##name() const { return m_ ## name; }\
	bool set_##name(type val) { m_ ## name = val; return true; }
#define ITEM_PROP_V(type, name) \
	private: \
	type m_##name; \
	public: \
	type get_##name() const override { return m_ ## name; }\
	bool set_##name(type val) { m_ ## name = val; return true; }
#define ITEM_SPROP(name) \
	private: \
	std::string m_##name; \
	public: \
	std::string get_##name() const { return m_ ## name; }\
	bool set_##name(const std::string& val) { m_ ## name = val; return true; }

		struct path_item : av::items::common_props_item
		{
			path_item(av::MediaServer* device, const fs::path& path, net::ulong duration)
				: av::items::common_props_item(device)
				, m_path(path)
				, m_duration(duration)
			{
				set_title(m_path.filename().string());
				m_last_write = fs::last_write_time(m_path);
			}
			time_t     get_last_write_time() const override { return m_last_write; }
			net::ulong get_duration() const        override { return m_duration; }
			net::ulong get_size() const            override { return fs::is_directory(m_path) ? 0 : (net::ulong)fs::file_size(m_path); }
			fs::path   get_path() const                     { return m_path; }
			void       set_cover(const std::string& base64);
			void       set_cover(const fs::path& cover)     { m_cover = media::from_file(cover, false); }
			media_ptr  get_cover()                          { return m_cover; }

		protected:
			fs::path   m_path;
			media_ptr  m_cover;
			time_t     m_last_write;

			net::ulong m_duration;
		};

		struct common_file : path_item
		{
			common_file(av::MediaServer* device, const fs::path& path, net::ulong duration)
				: path_item(device, path, duration)
			{
			}
			container_type list(net::ulong /*start_from*/, net::ulong /*max_count*/)       override { return container_type(); }
			net::ulong     predict_count(net::ulong served) const                          override { return served; }
			net::ulong     update_id() const                                               override { return 0; }
			item_ptr       get_item(const std::string& /*id*/)                             override { return nullptr; }
			bool           is_image() const                                                override { return false; }
			bool           is_folder() const                                               override { return false; }
			media_ptr      get_media(bool main_resource)                                   override;
			void           output(std::ostream& o, const std::vector<std::string>& filter,
			                      const net::ssdp::import::av::client_interface_ptr& client,
			                      const net::config::config_ptr& config) const             override;
			virtual void   attrs(std::ostream& /*o*/, const std::vector<std::string>& /*filter*/,
			                      const net::config::config_ptr& /*config*/) const                  {};

		};

		struct photo_file : common_file
		{
			photo_file(av::MediaServer* device, const fs::path& path, net::ulong)
				: common_file(device, path, 0)
			{
			}
			const char* get_upnp_class() const override { return "object.item.imageItem.photo"; }
			bool is_image() const override { return true; }

			ITEM_PROP_V(net::ulong, width);
			ITEM_PROP_V(net::ulong, height);
		};

		struct video_file : common_file
		{
			video_file(av::MediaServer* device, const fs::path& path, net::ulong duration)
				: common_file(device, path, duration)
			{
			}
			const char* get_upnp_class() const override { return "object.item.videoItem"; }

			ITEM_PROP_V(net::ulong, width);
			ITEM_PROP_V(net::ulong, height);
			ITEM_PROP_V(int, ref_frame_count);
		};

		struct audio_file : common_file
		{
			audio_file(av::MediaServer* device, const fs::path& path, net::ulong duration)
				: common_file(device, path, duration)
			{
			}
			const char* get_upnp_class() const override { return "object.item.audioItem.musicTrack"; }
			void attrs(std::ostream& o, const std::vector<std::string>& filter, const net::config::config_ptr& config) const override;

			ITEM_SPROP(artist);
			ITEM_SPROP(album);
			ITEM_SPROP(genre);
			ITEM_PROP(int, track_position);
			ITEM_PROP_V(net::ulong, bitrate);
			ITEM_PROP_V(net::ulong, sample_freq);
			ITEM_PROP_V(net::ulong, channels);
		};

		struct container_file : path_item, std::enable_shared_from_this<container_file>
		{
			typedef std::promise<container_type> async_promise_type;
			typedef std::future<container_type> future_type;

			struct container_task
			{
				async_promise_type m_promise;
				net::ulong         m_start_from;
				net::ulong         m_max_count;
				container_task(async_promise_type promise, net::ulong start_from, net::ulong max_count)
					: m_promise(std::move(promise))
					, m_start_from(start_from)
					, m_max_count(max_count)
				{
				}
			};

			container_file(av::MediaServer* device, const fs::path& path)
				: path_item(device, path, 0)
				, m_update_id(1)
				, m_current_max(0)
				, m_running(false)
			{
			}

			container_type list(net::ulong start_from, net::ulong max_count)               override;
			net::ulong     predict_count(net::ulong served) const                          override;
			net::ulong     update_id() const                                               override { return (net::ulong)m_update_id; }
			item_ptr       get_item(const std::string& id)                                 override;
			bool           is_image() const                                                override { return false; }
			bool           is_folder() const                                               override { return true; }
			void           output(std::ostream& o, const std::vector<std::string>& filter,
			                      const net::ssdp::import::av::client_interface_ptr& client,
			                      const net::config::config_ptr& config) const             override;
			const char*    get_upnp_class() const                                          override { return "object.container.storageFolder"; }

			void           rescan_if_needed();
			virtual bool   rescan_needed()         { return false; }
			virtual void   rescan()                {}
			virtual void   folder_changed();
			virtual void   add_child(item_ptr);
			virtual void   remove_child(item_ptr);

		private:
			net::ulong m_current_max;

		protected:
			std::vector<av::items::media_item_ptr> m_children;
			time_t m_update_id;
			std::list<container_task> m_tasks;
			bool m_running;
			mutable std::mutex m_guard;

			bool mark_start()
			{
				std::lock_guard<std::mutex> lock(m_guard);
				if (m_running)
					return false;
				m_running = true;
				return true;
			}
			void mark_stop()
			{
				std::lock_guard<std::mutex> lock(m_guard);
				m_running = false;
				fulfill_promises(true);
			}
			bool        is_running() const { return m_running; }
			future_type async_list(net::ulong start_from, net::ulong max_count);
			void        fill_request(async_promise_type promise, net::ulong start_from, net::ulong max_count);
			void        fulfill_promises(bool forced);
		};

		struct directory_item : container_file
		{
			directory_item(av::MediaServer* device, const fs::path& path)
				: container_file(device, path)
				, m_last_scan(0)
			{
			}

			void check_updates() override;
			bool rescan_needed() override;
			void rescan()        override;

			struct contents
			{
				fs::path m_path;
				contents(const fs::path& path) : m_path(path) {}
				fs::directory_iterator begin() const { return fs::directory_iterator(m_path); }
				fs::directory_iterator end() const { return fs::directory_iterator(); }
			};

			static fs::path get_path(const av::items::media_item_ptr& ptr)
			{
				if (!ptr)
					return fs::path();

				return static_cast<path_item*>(ptr.get())->get_path();
			}
		private:
			time_t m_last_scan;
		};

		av::items::media_item_ptr from_path(const fs::path& path);
	}
}

#endif //__FS_ITEMS_HPP__