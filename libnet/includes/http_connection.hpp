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

#ifndef __HTTP_CONNECTION_HPP__
#define __HTTP_CONNECTION_HPP__

#include <memory>
#include <set>
#include <boost/asio.hpp>
#include <http_parser.hpp>
#include <http.hpp>

namespace net
{
	namespace http
	{
		struct connection;
		typedef std::shared_ptr<connection> connection_ptr;

		class connection_manager : private boost::noncopyable
		{
		public:
			/// Add the specified connection to the manager and start it.
			void start(connection_ptr c);

			/// Stop the specified connection.
			void stop(connection_ptr c);

			/// Stop all connections.
			void stop_all();

		private:
			/// The managed connections.
			std::set<connection_ptr> m_connections;
		};

		struct connection : private boost::noncopyable, public std::enable_shared_from_this<connection>
		{
			explicit connection(boost::asio::ip::tcp::socket && socket, connection_manager& manager);

			void start() { read_some_more(); }
			void stop() { m_socket.close(); }
		private:
			void read_some_more();
			void send_reply();

			std::array<char, 8192> m_buffer;
			boost::asio::ip::tcp::socket m_socket;
			http::header_parser<http::http_request> m_parser;
			connection_manager& m_manager;
			int m_pos;
		};

	}
}

#endif // __HTTP_CONNECTION_HPP__
