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
#include <udp.hpp>

namespace net
{
	namespace udp
	{
		void datagram::post(boost::asio::ip::udp::socket& socket, boost::asio::ip::udp::endpoint& endpoint)
		{
			auto shared = shared_from_this();
			//std::cout << m_payload;
			socket.async_send_to(boost::asio::buffer(m_payload), endpoint, [this, shared](boost::system::error_code ec, size_t)
			{
				m_socket.done(shared_from_this());
			});
		}

		datagram_socket::datagram_socket(boost::asio::io_service& io_service, const boost::asio::ip::address& address, net::ushort port)
			: m_io_service(io_service)
			, m_endpoint(address, port)
			, m_socket(m_io_service, m_endpoint.protocol())
			, m_address(address)
			, m_port(port)
		{
		}

		void datagram_socket::post(datagram_ptr d)
		{
			m_datagrams.insert(d);
			d->post(m_socket, m_endpoint);
		}

		void datagram_socket::post(std::string payload)
		{
			post(std::make_shared<datagram>(*this, std::move(payload)));
		}

		void datagram_socket::done(datagram_ptr d)
		{
			m_datagrams.erase(d);
		}
	}
}
