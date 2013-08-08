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

#include "pch.h"
#include <http_server.hpp>

namespace net
{
	namespace http
	{
		server::server(boost::asio::io_service& service, const std::string& usn, net::ushort port)
			: m_port(port)
			, m_handler(usn)
			, m_io_service(service)
			, m_acceptor(service)
			, m_socket(service)
		{
			boost::asio::ip::tcp::resolver resolver(m_io_service);
			std::ostringstream port_s; port_s << port;
			boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(
				boost::asio::ip::tcp::resolver::query{ "0.0.0.0", port_s.str() }
			);
			m_acceptor.open(endpoint.protocol());
			m_acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
			m_acceptor.bind(endpoint);
			m_acceptor.listen();
		}

		void server::stop()
		{
			// The server is stopped by cancelling all outstanding asynchronous
			// operations. Once all operations have finished the io_service::run()
			// call will exit.
			m_acceptor.close();
			m_manager.stop_all();
		}

		void server::do_accept()
		{
			m_acceptor.async_accept(m_socket, [this](boost::system::error_code ec) {
				// Check whether the server was stopped by a signal before this
				// completion handler had a chance to run.
				if (!m_acceptor.is_open())
				{
					return;
				}

				if (!ec)
				{
					m_manager.start(std::make_shared<http::connection>(std::move(m_socket), m_manager, m_handler));
				}

				do_accept();
			});
		}
	}
}
