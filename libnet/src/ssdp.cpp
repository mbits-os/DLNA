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
#include <ssdp.hpp>
#include <thread>
#include <http.hpp>
#include <http_parser.hpp>

namespace net
{
	namespace ssdp
	{
		const boost::asio::ip::address & ipv4_multicast()
		{
			using boost::asio::ip::address;
			using boost::asio::ip::address_v4;

			// 239.255.255.250
			static address multicast_address { address_v4 { 0xEFFFFFFA } };
			return multicast_address;
		}

		notifier::notifier(boost::asio::io_service& io_service, long seconds, const std::string& usn, net::ushort port)
			: udp::datagram_socket(io_service, ipv4_multicast(), PORT)
			, m_timer(io_service, boost::posix_time::seconds(1))
			, m_interval(seconds)
			, m_local(net::iface::get_default_interface())
			, m_usn(usn)
			, m_port(port)
		{
		}
		void notifier::join_group()
		{
			m_socket.set_option(boost::asio::ip::udp::socket::reuse_address(true));
			m_socket.bind(boost::asio::ip::udp::endpoint(net::iface::get_default_interface(), PORT));
			m_socket.set_option(boost::asio::ip::multicast::join_group(m_endpoint.address()));
		}
		void notifier::leave_group()
		{
			m_socket.set_option(boost::asio::ip::multicast::leave_group(m_endpoint.address()));
		}

		std::string notifier::build_msg(const std::string& nt, notification_type nts) const
		{
			http::http_request req { "NOTIFY", "*" };
			req.append("host")->out() << net::to_string(ipv4_multicast()) << ":" << PORT;
			req.append("nt", nt);
			req.append("nts")->out() << nts;
			if (nt == m_usn)
				req.append("usn", m_usn);
			else
				req.append("usn")->out() << m_usn << "::" << nt;
			if (nts == ALIVE)
			{
				req.append("location")->out() << "http://" << net::to_string(m_local) << ":" << m_port << "/config/device.xml";
				req.append("cache-control")->out() << "max-age=" << m_interval;
				req.append("server")->out() << http::get_server_version();
			}

			std::ostringstream os;
			os << req;
			return os.str();
		}

		listener::listener(notifier& socket)
			: m_notifier(socket)
		{
		}

		void listener::start()
		{
			m_notifier.join_group();
			do_accept();
		}

		void listener::stop()
		{
			m_notifier.leave_group();
		}

		void listener::do_accept()
		{
			m_notifier.listen_from(boost::asio::buffer(m_buffer), m_remote_endpoint,
				[&](boost::system::error_code ec, size_t bytes_recvd)
			{
				if (!ec)
				{
					net::http::header_parser<net::http::http_request> parser;

					const char* data = m_buffer.data();
					
					if (parser.parse(data, data + bytes_recvd) == net::http::parser::finished)
					{
						auto && header = parser.header();
						if (header.m_method == "M-SEARCH")
						{
							bool printed = false;
							auto st_it = header.find("st");
							if (st_it != header.end())
							{
								auto && st = st_it->value();
								if (st == "urn:schemas-upnp-org:service:ContentDirectory:1" ||
									st == "urn:schemas-upnp-org:device:MediaServer:1" ||
									st == "upnp:rootdevice" ||
									st == m_notifier.usn())
								{
									std::cout << "[Listener] Remote: " << to_string(m_remote_endpoint.address()) << ":" << m_remote_endpoint.port() << "\r\nREQUEST\r\n" << header;
									printed = true;
								}
							}

							if (!printed)
							{
								std::ostringstream o;
								o << "[IGNORING] " << header.m_method << " ";
								if (header.m_resource != "*")
								{
									auto it = header.find("host");
									if (it != header.end())
										o << it->value();
								}
								o << header.m_resource << " " << header.m_protocol;
								o << " [ " << to_string(m_remote_endpoint.address()) << ":" << m_remote_endpoint.port() << " ]\n";
								std::cout << o.str();
							}
						}
					}

					do_accept();
				}
			});
		}
	}
}
