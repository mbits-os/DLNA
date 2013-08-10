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
#ifndef __UDP_HPP__
#define __UDP_HPP__

#include <memory>
#include <set>
#include <boost/utility.hpp>
#include <boost/asio.hpp>
#include <types.hpp>

namespace net
{
	namespace udp
	{
		struct datagram;
		typedef std::shared_ptr<datagram> datagram_ptr;

		class datagram_socket
		{
		protected:
			bool                           m_async_close;
			boost::asio::io_service&       m_io_service;
			boost::asio::ip::udp::endpoint m_endpoint;
			boost::asio::ip::udp::socket   m_socket;
			boost::asio::ip::address       m_address;
			net::ushort                    m_port;
			std::set<datagram_ptr>         m_datagrams;
		public:
			datagram_socket(boost::asio::io_service& io_service, const boost::asio::ip::address& address, net::ushort port);
			void post(datagram_ptr d);
			void post(std::string payload);
			void done(datagram_ptr d);
			void async_close();

			boost::asio::io_service& service() { return m_io_service; }
			boost::asio::ip::udp::socket& socket() { return m_socket; }
			boost::asio::ip::udp::endpoint& endpoint() { return m_endpoint;  }
		};

		struct datagram : private boost::noncopyable, public std::enable_shared_from_this<datagram>
		{
			datagram(datagram_socket& socket, std::string payload)
				: m_socket(socket)
				, m_payload(std::move(payload))
			{}

			virtual void post() { post(m_socket.socket(), m_socket.endpoint()); }
		protected:
			void post(boost::asio::ip::udp::socket& socket, boost::asio::ip::udp::endpoint& endpoint);

			datagram_socket& m_socket;
			std::string m_payload;
		};

		struct endpoint_datagram : datagram
		{
			endpoint_datagram(datagram_socket& socket, const boost::asio::ip::udp::endpoint& endpoint, std::string payload)
				: datagram(socket, std::move(payload))
				, m_endpoint(endpoint)
			{}

			void post() override { datagram::post(m_socket.socket(), m_endpoint); }
		private:
			boost::asio::ip::udp::endpoint m_endpoint;
		};

		struct multicast_receiver
		{
			typedef boost::asio::io_service        io_service_t;
			typedef boost::asio::ip::udp::socket   socket_t;
			typedef boost::asio::ip::address_v4    address_t;
			typedef boost::asio::ip::udp::endpoint endpoint_t;
			typedef std::array<char, 2048>         buffer_t;

			typedef std::function<bool()> receive_handler_t;

			multicast_receiver(io_service_t& io_service, const endpoint_t& multicast);
			virtual ~multicast_receiver() {}

			void start(const receive_handler_t& handler);
			void stop();

			const char* data() const { return m_buffer.data(); }
			size_t received() const { return m_received; }
			const endpoint_t& remote() const { return m_remote; }

		private:

			io_service_t&     m_io_service;
			address_t         m_local;
			endpoint_t        m_multicast;
			endpoint_t        m_remote;
			socket_t          m_socket;
			buffer_t          m_buffer;
			size_t            m_received;
			receive_handler_t m_handler;

			void receive();
		};

		struct multicast_socket : std::enable_shared_from_this<multicast_socket>
		{
			multicast_socket(boost::asio::io_service& io_service, const boost::asio::ip::udp::endpoint& endpoint);
			virtual ~multicast_socket();
			void send(const std::string& msg);

		private:
			boost::asio::io_service&       m_io_service;
			boost::asio::ip::address_v4    m_local;
			boost::asio::ip::udp::endpoint m_remote;
			boost::asio::ip::udp::socket   m_socket;
		};

		struct unicast_socket : std::enable_shared_from_this<unicast_socket>
		{
			unicast_socket(boost::asio::io_service& io_service, const boost::asio::ip::udp::endpoint& endpoint);
			virtual ~unicast_socket();
			void send(const std::string& msg);

		private:
			boost::asio::io_service&       m_io_service;
			boost::asio::ip::udp::endpoint m_remote;
			boost::asio::ip::udp::socket   m_socket;
		};
	}
}

#endif // __UDP_HPP__