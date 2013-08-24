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
#ifndef __SSDP_MEDIA_SERVER_INTERNAL_HPP__
#define __SSDP_MEDIA_SERVER_INTERNAL_HPP__

#include <media_server.hpp>
#include <log.hpp>

namespace net { namespace ssdp { namespace import { namespace av {
	extern Log::Module Multimedia;
	struct log : public Log::basic_log<log>
	{
		static const Log::Module& module() { return Multimedia; }
	};

	namespace items
	{
		struct root_item : common_props_item
		{
			root_item(MediaServer* device)
				: common_props_item(device)
				, m_update_id(1)
				, m_current_max(0)
			{
			}

			container_type list(ulong start_from, ulong max_count)           override;
			ulong          predict_count(ulong /*served*/) const             override { return m_children.size(); }
			media_item_ptr get_item(const std::string& id)                   override;
			bool           is_image() const                                  override { return false; }
			bool           is_folder() const                                 override { return true; }
			void           output(std::ostream& o,
			                   const std::vector<std::string>& filter,
			                   const config::config_ptr& config) const       override;
			const char*    get_upnp_class() const                            override { return "object.container.storageFolder"; }
			ulong          update_id() const                                 override { return m_device->system_update_id(); }
			virtual void   add_child(media_item_ptr);
			virtual void   remove_child(media_item_ptr);

			

			std::string get_parent_attr() const override {
				static std::string parent_id { "-1" };
				return parent_id;
			}

		private:
			ulong          m_current_max;
			time_t         m_update_id;
			container_type m_children;
		};

		std::pair<ulong, std::string> pop_id(const std::string& id);
	}

}}}} // net::ssdp::import::av

#endif //__SSDP_MEDIA_SERVER_INTERNAL_HPP__
