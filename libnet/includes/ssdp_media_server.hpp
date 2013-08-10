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
#ifndef __SSDP_MEDIA_SERVER_HPP__
#define __SSDP_MEDIA_SERVER_HPP__

#include <ssdp_device.hpp>

namespace net
{
	namespace ssdp
	{
		namespace av
		{
			struct media_server : device
			{
				media_server(const device_info& info) : device(info) {}
			};
		}

		/*
		struct service
		{
			virtual ~service() = 0;
		};
		typedef std::shared_ptr<service> service_ptr;

		struct device
		{
			device(const device_info& info)
				: m_info(info)
			{

			}
			virtual ~device() {}

			virtual net::http::module_version server() const { return m_info.m_server; }
		private:
			const device_info m_info;
		};
		typedef std::shared_ptr<device> device_ptr;
		*/

	}
}

#endif //__SSDP_MEDIA_SERVER_HPP__
