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
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <http.hpp>
#include <http_server.hpp>
#include <ssdp.hpp>

namespace net { namespace http {
	module_version get_server_module_version() { return {"lanRadio", 0, 1}; }
}}

int main(int argc, char* argv [])
{
	try
	{
		auto usn = net::create_uuid();
		boost::asio::io_service service;
		boost::asio::signal_set signals(service);

		net::http::server http{ service, 6001 };
		net::ssdp::notifier notifier{ service, 1800, usn, 6001 };

		signals.add(SIGINT);
		signals.add(SIGTERM);
#if defined(SIGQUIT)
		m_signals.add(SIGQUIT);
#endif // defined(SIGQUIT)

		signals.async_wait([&](boost::system::error_code, int) {
			notifier.stop();
			http.stop();
		});

		if (notifier.is_valid())
		{
			http.start();
			notifier.start();
		}

		service.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
	return 0;
}
