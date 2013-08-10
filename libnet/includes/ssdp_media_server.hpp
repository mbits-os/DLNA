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
#ifndef __SSDP_MEDIA_SERVER_HPP__
#define __SSDP_MEDIA_SERVER_HPP__

#include <ssdp_device.hpp>

namespace net
{
	namespace ssdp
	{
		namespace av
		{
			struct media_server;
			namespace service
			{
				struct content_directory : service
				{
					media_server* m_device;
					content_directory(media_server* device)
						: m_device(device)
					{
						typedef content_directory service_t;
						SSDP_ADD_CONTROL(GetSystemUpdateID);
						SSDP_ADD_CONTROL(Browse);
					}
					const char* get_type() const override { return "urn:schemas-upnp-org:service:ContentDirectory:1"; }
					const char* get_uri() const override { return "content_directory"; }

					void control_GetSystemUpdateID(const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response);
					void control_Browse(const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response);
				};
				typedef std::shared_ptr<content_directory> content_directory_ptr;

				struct content_manager : service
				{
					media_server* m_device;
					content_manager(media_server* device)
						: m_device(device)
					{
						typedef content_directory service_t;
					}
					const char* get_type() const override { return "urn:schemas-upnp-org:service:ContentManager:1"; }
					const char* get_uri() const override { return "content_manager"; }
				};
				typedef std::shared_ptr<content_manager> content_manager_ptr;
			}

			struct media_server : device
			{
				service::content_directory_ptr m_directory;
				service::content_manager_ptr m_manager;
				media_server(const device_info& info)
					: device(info)
					, m_directory(std::make_shared<service::content_directory>(this))
					, m_manager(std::make_shared<service::content_manager>(this))
				{
					add(m_directory);
					add(m_manager);
				}

				const char* get_type() const override { return "urn:schemas-upnp-org:device:MediaServer:1"; }
			};
		}
	}
}

#endif //__SSDP_MEDIA_SERVER_HPP__
