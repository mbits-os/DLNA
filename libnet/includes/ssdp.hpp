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
#ifndef __SSDP_HPP__
#define __SSDP_HPP__

#include <iostream>
#include <http.hpp>
#include <http_server.hpp>
#include <udp.hpp>
#include <interface.hpp>

namespace net
{
	namespace ssdp
	{
		enum notification_type
		{
			ALIVE,
			BYEBYE
		};

		static const net::ushort PORT = 1900;
		static const long INTERVAL = 1800;

		inline std::ostream& operator << (std::ostream& o, notification_type type)
		{
			switch (type)
			{
			case ALIVE: return o << "ssdp:alive";
			case BYEBYE: return o << "ssdp:byebye";
			}
			return o << "ssdp:unknown:" << (int) type;
		}

		const boost::asio::ip::address & ipv4_multicast();

		struct notifier : udp::datagram_socket
		{
			notifier(boost::asio::io_service& io_service, long seconds, const std::string& usn, net::ushort port);

			void join_group();
			void leave_group();

			void notify(notification_type nts)
			{
				printf("Sending %s...\n", nts == ALIVE ? "ALIVE" : "BYEBYE"); fflush(stdout);

				notify("upnp:rootdevice", nts);
				notify(m_usn, nts);
				notify("urn:schemas-upnp-org:device:MediaServer:1", nts);
				notify("urn:schemas-upnp-org:service:ContentDirectory:1", nts);
			}

			bool is_valid() const { return !m_local.is_unspecified(); }

			void start()
			{
				m_timer.async_wait([this](boost::system::error_code ec)
				{
					if (!ec)
						stillAlive();
				});
			}
			void stop()
			{
				m_timer.cancel();
				if (is_valid())
					notify(BYEBYE);
				async_close();
			}

			template <typename Buffer, typename ListenHandler>
			void listen_from(Buffer& buffer, boost::asio::ip::udp::endpoint& endpoint, ListenHandler handler)
			{
				m_socket.async_receive_from(buffer, endpoint, handler);
			}
		private:
			boost::asio::deadline_timer m_timer;
			long                        m_interval;
			boost::asio::ip::address    m_local;
			std::string                 m_usn;
			net::ushort                 m_port;

			std::string build_msg(const std::string& nt, notification_type nts) const;

			void notify(const std::string& nt, notification_type nts)
			{
				auto message = build_msg(nt, nts);
				post(std::move(message));
			}

			void stillAlive()
			{
				notify(ALIVE);
				m_timer.expires_at(m_timer.expires_at() + boost::posix_time::seconds(m_interval));
				m_timer.async_wait([this](boost::system::error_code ec)
				{
					if (!ec)
						stillAlive();
				});
			}
		};

		struct listener
		{
			listener(notifier& socket);
			void start();
			void stop();
		private:
			notifier& m_notifier;
			boost::asio::ip::udp::endpoint m_remote_endpoint;
			std::array<char, 1024> m_buffer;

			void do_accept();
		};

		struct server
		{
			server(boost::asio::io_service& service, net::ushort port)
				: m_usn("uuid:" + net::create_uuid())
				, m_http(service, m_usn, port)
				, m_notifier(service, INTERVAL, m_usn, port)
				, m_listener(m_notifier)
			{
			}

			bool is_valid() const { return m_notifier.is_valid(); }

			void start()
			{
				m_http.start();
				m_notifier.start();
				m_listener.start();
			}

			void stop()
			{
				m_listener.stop();
				m_notifier.stop();
				m_http.stop();
			}

		private:
			std::string  m_usn;
			http::server m_http;
			notifier     m_notifier;
			listener     m_listener;
		};
	}
}

#endif //__SSDP_HPP__
