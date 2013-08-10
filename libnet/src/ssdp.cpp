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

		const boost::asio::ip::udp::endpoint & ipv4_multicast_endpoint()
		{
			static boost::asio::ip::udp::endpoint multicast_endpoint { ipv4_multicast(), PORT };
			return multicast_endpoint;
		}

		static void print_debug(bool ignoring, const net::http::http_request& header)
		{
			std::string ssdp_ST = header.ssdp_ST();

			if (ssdp_ST == "urn:schemas-upnp-org:device:InternetGatewayDevice:1")
				return;

			std::ostringstream o;
			if (ignoring)
				o << "[...] ";
			else
				o << "[SSDP] ";
			o << header.m_method << " " << header.m_resource << " " << header.m_protocol << " [" << to_string(header.m_remote_address) << ":" << header.m_remote_port << "]";

			if (!ssdp_ST.empty())
				o << " [" << ssdp_ST << "]";

			auto ssdp_MAN = header.ssdp_MAN();
			if (!ssdp_MAN.empty())
				o << " [" << ssdp_MAN << "]";

			auto ssdp_NTS = header.quoted("nts");
			if (!ssdp_NTS.empty())
			{
				auto location = header.simple("location");
				if (ssdp_NTS == "ssdp:alive" && !location.empty())
					o << " [" << location << "]";
				else
					o << " [" << ssdp_NTS << "]";
			}

			auto ssdp_NT = header.quoted("nt");
			if (!ssdp_NT.empty())
				o << " [" << ssdp_NT << "]";

			o << "\n";

			if (ssdp_ST.empty() && ssdp_MAN.empty() && ssdp_NT.empty() && ssdp_NTS.empty())
				o << header;

			std::cout << o.str();
		}

		ticker::ticker(boost::asio::io_service& io_service, const device_ptr& device, long seconds, net::ushort port)
			: m_service(io_service)
			, m_device(device)
			, m_timer(io_service, boost::posix_time::seconds(seconds / 3))
			, m_interval(seconds)
			, m_local(net::iface::get_default_interface())
			, m_port(port)
		{
		}

		void ticker::start()
		{
			notify(ALIVE);

			m_timer.async_wait([this](boost::system::error_code ec)
			{
				if (!ec)
					stillAlive();
			});
		}

		void ticker::stop()
		{
			m_timer.cancel();
			notify(BYEBYE);
		}

		std::string ticker::build_msg(const std::string& nt, notification_type nts) const
		{
			http::http_request req { "NOTIFY", "*" };
			req.append("host")->out() << net::to_string(ipv4_multicast()) << ":" << PORT;
			req.append("nt", nt);
			req.append("nts")->out() << nts;

			auto&& usn = m_device->usn();
			if (nt == usn)
				req.append("usn", usn);
			else
				req.append("usn")->out() << usn << "::" << nt;

			if (nts == ALIVE)
			{
				req.append("location")->out() << "http://" << net::to_string(m_local) << ":" << m_port << "/config/device.xml";
				req.append("cache-control")->out() << "max-age=" << m_interval;
				req.append("server")->out() << http::get_ssdp_server_version(m_device->server());
			}

			std::ostringstream os;
			os << req;
			return os.str();
		}

		void ticker::notify(notification_type nts) const
		{
			auto socket = std::make_shared<udp::multicast_socket>(m_service, ipv4_multicast_endpoint());

			printf("Sending %s...\n", nts == ALIVE ? "ALIVE" : "BYEBYE"); fflush(stdout);

			socket->send(build_msg("upnp:rootdevice", nts));
			socket->send(build_msg(m_device->usn(), nts));
			socket->send(build_msg(m_device->get_type(), nts));
			for (auto&& service : services(m_device))
				socket->send(build_msg(service->get_type(), nts));
		}

		void ticker::stillAlive()
		{
			notify(ALIVE);
			m_timer.expires_at(m_timer.expires_at() + boost::posix_time::seconds(m_interval));
			m_timer.async_wait([this](boost::system::error_code ec)
			{
				if (!ec)
					stillAlive();
			});
		}

		receiver::receiver(boost::asio::io_service& io_service, const device_ptr& device, net::ushort port)
			: m_impl(io_service, boost::asio::ip::udp::endpoint(ipv4_multicast(), PORT))
			, m_device(device)
			, m_service(io_service)
			, m_local(net::iface::get_default_interface())
			, m_port(port)
		{
		}

		namespace mcast = boost::asio::ip::multicast;
		void receiver::start()
		{
			m_impl.start([this]
			{
				net::http::header_parser<net::http::http_request> parser;

				const char* data = m_impl.data();

				if (parser.parse(data, data + m_impl.received()) == net::http::parser::finished)
				{
					auto&& header = parser.header();
					header.remote_endpoint(m_impl.remote());

					bool printed = false;

					if (header.m_method == "M-SEARCH")
					{
						std::string st = header.ssdp_ST();
						std::string man = header.ssdp_MAN();
						if (!st.empty())
						{
							bool interesting = false;
							if (!interesting) interesting = st == "ssdp:all";
							if (!interesting) interesting = st == "ssdp:rootdevice";
							if (!interesting) interesting = st == m_device->usn();
							if (!interesting) interesting = st == m_device->get_type();
							if (!interesting)
								for (auto&& service : ssdp::services(m_device))
									if ((interesting = (st == service->get_type())) == true)
										break;

							if (interesting)
							{
								print_debug(false, header);
								printed = true;

								if (st == "ssdp:all")
									st = m_device->get_type();

								discovery(st);
							}
						}
					}

					if (!printed)
					{
						print_debug(true, header);
					}
				}
				else
				{
					std::cout << "[ " << m_impl.remote().address() << ":" << m_impl.remote().port() << " ]\n";
					std::cout.write(m_impl.data(), m_impl.received());
					std::cout << "\n";
				}

				return true;
			});
		}

		void receiver::stop()
		{
			m_impl.stop();
		}

		void receiver::discovery(const std::string& st)
		{
			printf("Replying to DISCOVER...\n");
			auto datagram = std::make_shared<udp::unicast_socket>(m_service, m_impl.remote());
			datagram->send(build_discovery_msg(st));
		}

		std::string receiver::build_discovery_msg(const std::string& st) const
		{
			http::http_response resp {};
			resp.append("date")->out() << net::to_string(net::time::now());
			resp.append("location")->out() << "http://" << net::to_string(m_local) << ":" << m_port << "/config/device.xml";
			resp.append("cache-control")->out() << "max-age=1800";
			resp.append("server")->out() << http::get_ssdp_server_version(m_device->server());
			resp.append("st", st);
			resp.append("ext");

			auto&& usn = m_device->usn();
			if (st == usn)
				resp.append("usn", usn);
			else
				resp.append("usn")->out() << usn << "::" << st;

			resp.append("content-length", "0");

			std::ostringstream os;
			os << resp;
			return os.str();
		}

	}
}
