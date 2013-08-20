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

#include <http/http.hpp>
#include <http/server.hpp>
#include <ssdp.hpp>
#include <media_server.hpp>
#include <boost/filesystem.hpp>
#include <log.hpp>
#include <config.hpp>
#include <threads.hpp>

namespace fs = boost::filesystem;
namespace av = net::ssdp::import::av;

namespace lan
{
	extern Log::Module APP;
	struct log : public Log::basic_log<log>
	{
		static const Log::Module& module() { return APP; }
	};

	namespace item
	{
		av::items::media_item_ptr from_path(av::MediaServer* device, const fs::path& path);
	}

	struct radio
	{
		radio(const net::ssdp::device_ptr& device, const net::config::config_ptr& config)
			: m_service()
			, m_signals(m_service)
			, m_upnp(m_service, device, config)
		{
			m_signals.add(SIGINT);
			m_signals.add(SIGTERM);
#if defined(SIGQUIT)
			m_signals.add(SIGQUIT);
#endif // defined(SIGQUIT)

			m_signals.async_wait([&](boost::system::error_code, int) {
				m_upnp.stop();
			});

			m_upnp.start();
		}

		void run() { m_service.run(); }

	private:
		boost::asio::io_service m_service;
		boost::asio::signal_set m_signals;
		net::ssdp::server       m_upnp;
	};
}

#ifdef _WIN32
#  define HAS_TERMINAL_API
void set_terminal_title(const std::string& title)
{
	SetConsoleTitleA(title.c_str());
}
#endif

void set_terminal_title(const net::config::config_ptr& config)
{
#ifdef HAS_TERMINAL_API
	std::ostringstream title;
	title << "LAN Radio [" << boost::asio::ip::host_name() << ", " << config->iface << ":" << config->port << "]";
	set_terminal_title(title.str());
#endif
}

int main(int argc, char* argv [])
{
	try
	{
		threads::set_name("main");

		lan::log::info() << "\nStarting...\n";

		net::ssdp::device_info info = {
			{ "lanRadio", 0, 1 },
			{ "lanRadio", "LAN Radio [" + boost::asio::ip::host_name() + "]", "01", "http://www.midnightbits.org/lanRadio" },
			{ "midnightBITS", "http://www.midnightbits.com" }
		};

		auto config = net::config::file_config("lanradio.conf");
		set_terminal_title(config);

		auto server = std::make_shared<av::MediaServer>(info, config);

		for (int arg = 1; arg < argc; ++arg)
		{
			auto path = fs::absolute(argv[arg]);
			if (!fs::exists(path))
				continue;

			if (fs::is_directory(path) && path.filename() == ".")
				path = path.parent_path();
			auto item = lan::item::from_path(server.get(), path);
			if (item)
			{
				lan::log::info() << "Adding " << path;
				server->add_root_element(item);
			}
		}

		lan::radio lanRadio(server, config);

		lanRadio.run();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
	return 0;
}
