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
			notifier(boost::asio::io_service& io_service, net::ushort port)
				: udp::datagram_socket(io_service, ipv4_multicast(), PORT)
				, m_local(net::iface::get_default_interface())
				, m_port(port)
			{}

			void notify(const std::string& usn, notification_type nts)
			{
				printf("Sending %s...\n", nts == ALIVE ? "ALIVE" : "BYEBYE");

				notify(usn, "upnp:rootdevice", nts);
				notify(usn, usn, nts);
				notify(usn, "urn:schemas-upnp-org:device:MediaServer:1", nts);
				notify(usn, "urn:schemas-upnp-org:service:ContentDirectory:1", nts);
			}
			bool isValid() const { return !m_local.is_unspecified(); }
		private:
			boost::asio::ip::address       m_local;
			net::ushort                    m_port;

			std::string buildNotifyMsg(const std::string& usn, const std::string& nt, notification_type nts) const
			{
				http::http_request req { "NOTIFY", "*" };
				req.append("host")->out() << net::to_string(ipv4_multicast()) << ":" << PORT;
				req.append("nt", nt);
				req.append("nts")->out() << nts;
				if (nt == usn)
					req.append("usn", usn);
				else
					req.append("usn")->out() << usn << "::" << nt;
				if (nts == ALIVE)
				{
					req.append("location")->out() << "http://" << net::to_string(m_local) << ":" << m_port << "/description/fetch";
					req.append("cache-control", "max-age=1800");
					req.append("server")->out() << http::get_server_version();
				}

				std::ostringstream os;
				os << req;
				return os.str();
			}

			void notify(const std::string& usn, const std::string& nt, notification_type nts)
			{
				auto message = buildNotifyMsg(usn, nt, nts);
				post(std::move(message));
			}
		};
	}
}

#endif //__SSDP_HPP__
