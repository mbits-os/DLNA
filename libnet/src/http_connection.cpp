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
#include <http_connection.hpp>
#include <types.hpp>

namespace net
{
	namespace http
	{
		void connection_manager::start(connection_ptr c)
		{
			m_connections.insert(c);
			c->start();
		}

		void connection_manager::stop(connection_ptr c)
		{
			m_connections.erase(c);
			c->stop();
		}

		void connection_manager::stop_all()
		{
			for (auto c : m_connections)
				c->stop();
			m_connections.clear();
		}

		connection::connection(boost::asio::ip::tcp::socket && socket, connection_manager& manager)
			: m_socket(std::move(socket))
			, m_manager(manager)
			, m_pos(0)
		{
		}

		void connection::read_some_more()
		{
			auto self(shared_from_this());
			m_socket.async_read_some(boost::asio::buffer(m_buffer),
				[this, self](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec)
				{
					std::cout << "Remote: " << net::to_string(m_socket.remote_endpoint().address()) << ":" << m_socket.remote_endpoint().port() << "\r\n";
					bool _continue = true;

					auto data = m_buffer.data();
					for (std::size_t i = 0; i < bytes_transferred; ++i)
					{
						::putchar((unsigned char) *data);
						if (*data == '\r' || (m_pos && (*data == '\n')))
							m_pos++;
						else
							m_pos = 0;
						if (m_pos == 4)
						{
							boost::system::error_code ignored_ec;
							m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
							m_manager.stop(self);
							_continue = false;
						}
						data++;
					}

					if (_continue)
						read_some_more();
				}
				else if (ec != boost::asio::error::operation_aborted)
				{
					m_manager.stop(shared_from_this());
				}
			});
		}
	}
}
