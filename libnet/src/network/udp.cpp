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
#include <interface.hpp>

namespace net
{
	namespace udp
	{
		multicast_receiver::multicast_receiver(boost::asio::io_service& io_service, const endpoint_t& multicast)
			: m_io_service(io_service)
			, m_local(net::iface::get_default_interface())
			, m_multicast(multicast)
			, m_socket(m_io_service, multicast.protocol())
		{
		}

		void multicast_receiver::start(const receive_handler_t& handler)
		{
			m_socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
			m_socket.bind(endpoint_t(m_local, m_multicast.port()));
			m_socket.set_option(boost::asio::ip::multicast::join_group(m_multicast.address().to_v4(), m_local));

			m_handler = handler;
			receive();
		}

		void multicast_receiver::stop()
		{
			m_socket.set_option(boost::asio::ip::multicast::leave_group(m_multicast.address().to_v4(), m_local));
			m_socket.close();
		}

		void multicast_receiver::receive()
		{
			m_socket.async_receive_from(boost::asio::buffer(m_buffer), m_remote, [this](const boost::system::error_code& ec, std::size_t received)
			{
				if (!ec)
				{
					m_received = received;
					if (m_handler())
						receive();
				}
			});
		}

		template <typename socket_type>
		struct datagram_packet
		{
			std::shared_ptr<socket_type> m_socket;
			std::string m_msg;

			datagram_packet(const std::shared_ptr<socket_type>& socket, const std::string& msg)
				: m_socket(socket)
				, m_msg(msg)
			{}
		};

		template <typename socket_type>
		std::shared_ptr<datagram_packet<socket_type>> make_packet(socket_type* socket, const std::string& msg)
		{
			auto self = socket->shared_from_this();
			return std::make_shared<datagram_packet<socket_type>>(self, msg);
		}


		multicast_socket::multicast_socket(boost::asio::io_service& io_service, const boost::asio::ip::udp::endpoint& endpoint)
			: m_io_service(io_service)
			, m_local(net::iface::get_default_interface())
			, m_remote(endpoint)
			, m_socket(io_service, endpoint.protocol())
		{
			m_socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
			m_socket.bind(boost::asio::ip::udp::endpoint(m_local, m_remote.port()));
			m_socket.set_option(boost::asio::ip::multicast::join_group(m_remote.address().to_v4(), m_local));
		}

		multicast_socket::~multicast_socket()
		{
			m_socket.set_option(boost::asio::ip::multicast::leave_group(m_remote.address().to_v4(), m_local));
		}

		void multicast_socket::send(const std::string& msg)
		{
			auto packet = make_packet(this, msg);
			m_socket.async_send_to(boost::asio::buffer(packet->m_msg), m_remote, [this, packet](const boost::system::error_code& ec, std::size_t received)
			{
			});
		}

		unicast_socket::unicast_socket(boost::asio::io_service& io_service, const boost::asio::ip::udp::endpoint& endpoint)
			: m_io_service(io_service)
			, m_remote(endpoint)
			, m_socket(io_service, endpoint.protocol())
		{
		}

		unicast_socket::~unicast_socket()
		{
		}

		void unicast_socket::send(const std::string& msg)
		{
			auto packet = make_packet(this, msg);
			m_socket.async_send_to(boost::asio::buffer(packet->m_msg), m_remote, [packet](const boost::system::error_code& ec, std::size_t received)
			{
			});
		}
	}
}
