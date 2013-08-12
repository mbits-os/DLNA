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

		module_version get_os_module_version();
		inline module_version get_upnp_module_version() { return { "UPnP", 1, 1 }; }

		inline std::tuple<module_version, module_version, module_version> get_ssdp_server_version(const module_version& server)
		{
			return std::make_tuple(get_os_module_version(), get_upnp_module_version(), server);
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

		struct request_data
		{
			virtual ~request_data() {}
			virtual size_t content_length() const = 0;
			virtual size_t read(void* dest, size_t size) = 0;
		};

		typedef std::shared_ptr<request_data> request_data_ptr;

		enum class http_method
		{
			other,
			get,
			head,
			post,
			m_search,
			notify
		};
		struct http_request : http_request_line, mime::headers
		{
			boost::asio::ip::address m_remote_address;
			net::ushort m_remote_port;
			request_data_ptr m_request_data;

			http_request() : m_remote_port(0) {}
			http_request(const std::string& method, const std::string& resource, protocol proto = http_1_1)
				: http_request_line(method, resource, proto)
				, m_remote_port(0)
			{}

			template <typename endpoint_type>
			void remote_endpoint(const endpoint_type& endpoint)
			{
				m_remote_address = endpoint.address();
				m_remote_port = endpoint.port();
			}

			void request_data(request_data_ptr ptr) { m_request_data = ptr; }
			request_data_ptr request_data() const { return m_request_data; }

			boost::filesystem::path resource() const
			{
				//TODO: decode and normalize
				return m_resource;
			}

			http_method method() const
			{
				if (m_method == "GET")      return http_method::get;
				if (m_method == "HEAD")     return http_method::head;
				if (m_method == "POST")     return http_method::post;
				if (m_method == "M-SEARCH") return http_method::m_search;
				if (m_method == "NOTIFY")   return http_method::notify;

				return http_method::other;
			}

			std::string simple(const std::string& name) const
			{
				auto it = find(name);
				if (it == end())
					return std::string();
				return it->value();
			}
			std::string quoted(const std::string& name) const
			{
				auto tmp = simple(name);
				if (!tmp.empty() && *tmp.begin() == '"' && *tmp.rbegin() == '"')
					tmp = tmp.substr(1, tmp.length() - 2);
				return tmp;
			}
			std::string user_agent() const { return simple("user-agent"); }
			std::string SOAPAction() const { return quoted("soapaction"); }
			std::string ssdp_MAN() const { return quoted("man"); }
			std::string ssdp_ST() const { return quoted("st"); }
			std::string ssdp_USN() const { return quoted("usn"); }
		};

		inline std::ostream& operator << (std::ostream& o, const http_request& resp)
		{
			return o << (const http_request_line&) resp << (const mime::headers&)resp << "\r\n";
		}

		template <typename socket_type>
		struct request_data_impl : request_data
		{
			socket_type& m_socket;
			size_t m_content_length;
			struct Seen
			{
				const char* m_data;
				size_t m_size;
				Seen(const char* data, size_t size)
					: m_data(data)
					, m_size(size)
				{}
				operator bool () const { return m_data != nullptr; }
				size_t read(void* dest, size_t size)
				{
					auto rest = m_size;
					if (rest > size)
						rest = size;

					memcpy(dest, m_data, rest);
					m_size -= rest;
					m_data += rest;
					if (!m_size)
						m_data = nullptr;

					return rest;
				}
			} m_first_chunk;

			request_data_impl(socket_type& socket, size_t content_length, const char* data, size_t size)
				: m_socket(socket)
				, m_content_length(content_length)
				, m_first_chunk(data, size)
			{
			}
			size_t content_length() const override { return m_content_length; }
			size_t read(void* dest, size_t size) override
			{
				size_t read = 0;
				if (m_first_chunk)
				{
					read = m_first_chunk.read(dest, size);
					(char*&) dest += read;
					size -= read;
				}
				if (size)
				{
					boost::system::error_code ignore_ec;
					read += m_socket.read_some(boost::asio::buffer(dest, size), ignore_ec);
				}
				return read;
			}
		};

		template <typename socket_type>
		request_data_ptr make_request_data(socket_type& socket, size_t content_length, const char* data, size_t size)
		{
			return std::make_shared<request_data_impl<socket_type>>(socket, content_length, size ? data : nullptr, size);
		}

		inline const char* http_message(int status)
		{
			switch (status)
			{
			case 100: return "Continue";
			case 101: return "Switching Protocols";
			case 200: return "OK";
			case 201: return "Created";
			case 202: return "Accepted";
			case 203: return "Non-Authoritative Information";
			case 204: return "No Content";
			case 205: return "Reset Content";
			case 206: return "Partial Content";
			case 300: return "Multiple Choices";
			case 301: return "Moved Permanently";
			case 302: return "Found";
			case 303: return "See Other";
			case 304: return "Not Modified";
			case 305: return "Use Proxy";
			case 306: return "(Unused)";
			case 307: return "Temporary Redirect";
			case 400: return "Bad Request";
			case 401: return "Unauthorized";
			case 402: return "Payment Required";
			case 403: return "Forbidden";
			case 404: return "Not Found";
			case 405: return "Method Not Allowed";
			case 406: return "Not Acceptable";
			case 407: return "Proxy Authentication Required";
			case 408: return "Request Timeout";
			case 409: return "Conflict";
			case 410: return "Gone";
			case 411: return "Length Required";
			case 412: return "Precondition Failed";
			case 413: return "Request Entity Too Large";
			case 414: return "Request-URI Too Long";
			case 415: return "Unsupported Media Type";
			case 416: return "Requested Range Not Satisfiable";
			case 417: return "Expectation Failed";
			case 500: return "Internal Server Error";
			case 501: return "Not Implemented";
			case 502: return "Bad Gateway";
			case 503: return "Service Unavailable";
			case 504: return "Gateway Timeout";
			case 505: return "HTTP Version Not Supported";
			};
			return nullptr;
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
			o << first_line.m_protocol << " " << first_line.m_status;
			const char* message = http_message(first_line.m_status);
			if (message)
				o << " " << message;
			return o << "\r\n";
		}

		struct http_response : http_response_line, mime::headers
		{
			http_response(int status = 200, protocol proto = http_1_1)
				: http_response_line(status, proto)
			{}

			void clear(const module_version& server)
			{
				m_protocol = http_1_1;
				m_status = 200;
				mime::headers::clear();
				append("server")->out() << server << " (" << get_os_module_version() << ")";
				append("date")->out() << to_string(time::now());
			}
		};

		inline std::ostream& operator << (std::ostream& o, const http_response& resp)
		{
			return o << (const http_response_line&) resp << (const mime::headers&)resp << "\r\n";
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
