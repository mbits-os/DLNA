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
#ifndef __NETWORK_TYPES_HPP__
#define __NETWORK_TYPES_HPP__

#include <boost/asio.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <string>

namespace net
{
	typedef unsigned char uchar;
	typedef unsigned short ushort;
	typedef unsigned int uint;
	typedef unsigned long ulong;

	inline std::string to_string(const boost::asio::ip::address_v4& addr)
	{
		return addr.to_string();
	}

	inline std::string to_string(const boost::asio::ip::address_v6& addr)
	{
		return "[" + addr.to_string() + "]";
	}

	inline std::string to_string(const boost::asio::ip::address& addr)
	{
		auto tmp = addr.to_string();
		if (addr.is_v6())
			return "[" + tmp + "]";
		return tmp;
	}

	inline std::string to_string(const boost::local_time::local_date_time& time)
	{
		std::ostringstream o;

		boost::local_time::local_time_facet* lf { new boost::local_time::local_time_facet("%a, %d %b %Y %H:%M:%S GMT") };
		o.imbue(std::locale(o.getloc(), lf));

		o << time;
		return o.str();
	}

	inline std::string to_iso8601(const boost::local_time::local_date_time& time)
	{
		std::ostringstream o;

		boost::local_time::local_time_facet* lf { new boost::local_time::local_time_facet("%Y-%m-%dT%H:%M:%S%F%Q") };
		o.imbue(std::locale(o.getloc(), lf));

		o << time;
		return o.str();
	}

	std::string create_uuid();
	std::string xmlencode(const std::string& in);

	namespace time
	{
		inline boost::local_time::local_date_time now()
		{
			return boost::local_time::local_sec_clock::local_time(boost::local_time::time_zone_ptr());
		}

		inline boost::local_time::local_date_time from_time_t(time_t time)
		{
			boost::posix_time::ptime pt = boost::posix_time::from_time_t(time);
			return boost::local_time::local_date_time(pt, boost::local_time::time_zone_ptr());
		}

		inline boost::local_time::local_date_time last_write(const boost::filesystem::path& path)
		{
			return from_time_t(boost::filesystem::last_write_time(path));
		}
	}
}

#endif // __NETWORK_TYPES_HPP__