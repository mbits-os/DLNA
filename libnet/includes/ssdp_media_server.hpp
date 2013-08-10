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
				struct service_helper : service
				{
					media_server* m_device;
					service_helper(media_server* device)
						: m_device(device)
					{}

					void soap_answer(const char* method, http::response& response, const std::string& body);
				};

#define SOAP_CALL(type, name) \
	void type ## name(const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response) \
	{ \
		soap_answer(#name, response, soap_ ## type ## name(req, doc)); \
	} \
	std::string soap_ ## type ## name(const http::http_request& req, const dom::XmlDocumentPtr& doc)

#define SOAP_CONTROL_CALL(name) SOAP_CALL(control_, name)
#define SOAP_EVENT_CALL(name) SOAP_CALL(event_, name)

				struct content_directory : service_helper
				{
					content_directory(media_server* device)
						: service_helper(device)
					{
						typedef content_directory service_t;
						SSDP_ADD_CONTROL(GetSortCapabilities);
						SSDP_ADD_CONTROL(GetSystemUpdateID);
						SSDP_ADD_CONTROL(Browse);
					}
					const char* get_type() const override { return "urn:schemas-upnp-org:service:ContentDirectory:1"; }
					const char* get_uri() const override { return "content_directory"; }

					SOAP_CONTROL_CALL(GetSortCapabilities);
					SOAP_CONTROL_CALL(GetSystemUpdateID);
					SOAP_CONTROL_CALL(Browse);
				};
				typedef std::shared_ptr<content_directory> content_directory_ptr;

				struct connection_manager : service_helper
				{
					connection_manager(media_server* device)
						: service_helper(device)
					{
						typedef connection_manager service_t;
						SSDP_ADD_CONTROL(GetProtocolInfo);
					}
					const char* get_type() const override { return "urn:schemas-upnp-org:service:ConnectionManager:1"; }
					const char* get_uri() const override { return "connection_manager"; }

					SOAP_CONTROL_CALL(GetProtocolInfo);
				};
				typedef std::shared_ptr<connection_manager> connection_manager_ptr;
			}

			struct media_server : device
			{
				service::content_directory_ptr m_directory;
				service::connection_manager_ptr m_manager;
				media_server(const device_info& info)
					: device(info)
					, m_directory(std::make_shared<service::content_directory>(this))
					, m_manager(std::make_shared<service::connection_manager>(this))
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