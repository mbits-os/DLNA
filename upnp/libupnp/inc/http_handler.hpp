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

#ifndef __HTTP_HANDLER_HPP__
#define __HTTP_HANDLER_HPP__

#include <http/request_handler.hpp>
#include <string>
#include <vector>
#include <boost/filesystem.hpp>
#include <device.hpp>
#include <config.hpp>

namespace net
{
	namespace http
	{
		typedef std::vector<std::pair<std::string, std::string>> template_vars;
		class http_handler: public request_handler, boost::noncopyable
		{
			struct client
			{
				boost::asio::ip::address m_address;
				ssdp::client_info_ptr m_client;
				client(const boost::asio::ip::address& address, const ssdp::client_info_ptr& client)
					: m_address(address)
					, m_client(client)
				{}
			};

			template_vars      m_vars;
			ssdp::device_ptr   m_device;
			config::config_ptr m_config;
			std::vector<client> m_clients_seen;

			void make_templated(const char* tmplt, const char* content_type, response& resp);
			void make_device_xml(response& resp);
			void make_service_xml(response& resp, const ssdp::service_ptr& service);
			void make_file(const boost::filesystem::path& path, response& resp);
			ssdp::client_info_ptr client_from_request(const http_request& req);
		public:
			http_handler(const ssdp::device_ptr& device, const config::config_ptr& config);
			void handle(const http_request& req, response& resp) override;
			void make_404(response& resp) override;
			void make_500(response& resp);
		};
	}
}

#endif // __HTTP_HANDLER_HPP__
