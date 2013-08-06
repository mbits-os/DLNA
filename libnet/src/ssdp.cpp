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
				req.append("location")->out() << "http://" << net::to_string(m_local) << ":" << m_port << "/description/fetch";
				req.append("cache-control")->out() << "max-age=" << m_interval;
				req.append("server")->out() << http::get_server_version();
			}

			std::ostringstream os;
			os << req;
			return os.str();
		}
	}
}
