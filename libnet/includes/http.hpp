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

#ifndef __HTTP_HPP__
#define __HTTP_HPP__

#include <types.hpp>
#include <mime.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <tuple>

namespace net
{
	namespace http
	{
		enum protocol
		{
			undeclared,
			http_1_0,
			http_1_1
		};

		inline std::ostream& operator << (std::ostream&o, protocol proto)
		{
			switch (proto)
			{
			case http_1_0: return o << "HTTP/1.0";
			case http_1_1: return o << "HTTP/1.1";
			};
			return o;
		}

		struct http_request_line
		{
			std::string m_method;
			std::string m_resource;
			protocol m_protocol;

			http_request_line()
				: m_protocol(http_1_1)
			{}

			http_request_line(const std::string& method, const std::string& resource, protocol proto = http_1_1)
				: m_method(method)
				, m_resource(resource)
				, m_protocol(proto)
			{}
		};

		inline std::ostream& operator << (std::ostream& o, const http_request_line& first_line)
		{
			return o << first_line.m_method << " " << first_line.m_resource << " " << first_line.m_protocol << "\r\n";
		}

		struct http_request : http_request_line, mime::headers
		{
			http_request() {}
			http_request(const std::string& method, const std::string& resource, protocol proto = http_1_1)
				: http_request_line(method, resource, proto)
			{}
		};

		inline std::ostream& operator << (std::ostream& o, const http_request& resp)
		{
			return o << (const http_request_line&) resp << (const mime::headers&)resp << "\r\n";
		}

		struct http_response_line
		{
			protocol m_protocol;
			int m_status;

			http_response_line(int status = 200, protocol proto = http_1_1)
				: m_status(status)
				, m_protocol(proto)
			{}
		};

		inline std::ostream& operator << (std::ostream& o, const http_response_line& first_line)
		{
			o << first_line.m_protocol << " " << first_line.m_status << " ";
			switch (first_line.m_status)
			{
			case 100: o << ""; break;
			}
			return o << "\r\n";
		}

		struct http_response : http_response_line, mime::headers
		{
			http_response(int status = 200, protocol proto = http_1_1)
				: http_response_line(status, proto)
			{}
		};

		inline std::ostream& operator << (std::ostream& o, const http_response& resp)
		{
			o << (const http_response_line&) resp << (const mime::headers&)resp << "\r\n";
		}

		struct module_version
		{
			std::string m_name;
			net::uint m_major;
			net::uint m_minor;
		};

		inline std::ostream& operator << (std::ostream & o, const module_version& mod)
		{
			return o << mod.m_name << "/" << mod.m_major << "." << mod.m_minor;
		}

		// to be implemented by a 
		module_version get_os_module_version();
		module_version get_server_module_version();
		inline module_version get_upnp_module_version() { return { "UPnP", 1, 1 }; }

		inline std::tuple<module_version, module_version, module_version> get_server_version()
		{
			return std::make_tuple(get_os_module_version(), get_upnp_module_version(), get_server_module_version());
		}
	}
}

namespace std
{
	inline ostream& operator << (ostream& o, const std::tuple<net::http::module_version, net::http::module_version, net::http::module_version>& version)
	{
		return o << std::get<0>(version) << " " << std::get<1>(version) << " " << std::get<2>(version);
	}
}
#endif // __HTTP_HPP__
