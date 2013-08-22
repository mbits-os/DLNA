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
#include <boost/date_time/posix_time/posix_time.hpp>

namespace fs = boost::filesystem;
namespace av = net::ssdp::import::av;

#if defined(DBG_ALLOCS)
static size_t all_allocs = 0;

void* operator new (size_t size)
{
	void *out = ::malloc(size + sizeof(size_t));
	if (!out) throw new std::bad_alloc();
	size_t* p = (size_t*) out;
	*p = size;
	all_allocs += size;
	return p + 1;
}

void operator delete (void* ptr)
{
	if (!ptr)
		return;

	size_t* p = (size_t*) ptr;
	--p;
	all_allocs -= *p;
	::free(p);
}

void* operator new[] (size_t size)
{
	void *out = ::malloc(size + sizeof(size_t));
	if (!out) throw new std::bad_alloc();
	size_t* p = (size_t*) out;
	*p = size;
	all_allocs += size;
	return p + 1;
}

void operator delete[] (void* ptr)
{
	if (!ptr)
		return;

	size_t* p = (size_t*) ptr;
	--p;
	all_allocs -= *p;
	::free(p);
}
#endif // defined(DBG_ALLOCS)

namespace lan
{
	extern Log::Module APP;
#if defined(DBG_ALLOCS)
	extern Log::Module Memory{ "Memory" };
#endif

	struct log : public Log::basic_log<log>
	{
		static const Log::Module& module() { return APP; }
	};

#if defined(DBG_ALLOCS)
	struct memlog : public Log::basic_log<memlog>
	{
		static const Log::Module& module() { return Memory; }
	};
#endif

	namespace item
	{
		av::items::media_item_ptr from_path(av::MediaServer* device, const fs::path& path);
	}

	struct radio
	{
		radio(const net::ssdp::device_ptr& device, const net::config::config_ptr& config)
			: m_service()
			, m_signals(m_service)
#if defined(DBG_ALLOCS)
			, m_timer(m_service, boost::posix_time::seconds(1))
#endif
			, m_upnp(m_service, device, config)
		{
			m_signals.add(SIGINT);
			m_signals.add(SIGTERM);
#if defined(SIGQUIT)
			m_signals.add(SIGQUIT);
#endif // defined(SIGQUIT)

			m_signals.async_wait([&](boost::system::error_code, int) {
				m_upnp.stop();
#if defined(DBG_ALLOCS)
				m_timer.cancel();
#endif
			});

#if defined(DBG_ALLOCS)
			m_timer.async_wait([this](const boost::system::error_code & /*e*/){
				memlog::info() << all_allocs;
				m_timer.expires_at(m_timer.expires_at() + boost::posix_time::seconds(30));
			});
#endif

			m_upnp.start();
		}

		void run() { m_service.run(); }

	private:
		boost::asio::io_service m_service;
		boost::asio::signal_set m_signals;
#if defined(DBG_ALLOCS)
		boost::asio::deadline_timer m_timer;
#endif
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

		auto config = net::config::config::from_file("lanradio.conf");
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

		fs::path confs("build");
		confs /= "resources";
		confs /= "renderers";

		struct contents
		{
			fs::path m_path;
			contents(const fs::path& path) : m_path(path) {}
			fs::directory_iterator begin() const { return fs::directory_iterator(m_path); }
			fs::directory_iterator end() const { return fs::directory_iterator(); }
		};
		for (auto&& entry : contents(confs))
		{
			if (!fs::is_directory(entry) && fs::extension(entry) == ".conf")
			{
				server->add_renderer_conf(entry.path());
			}
		}

		lan::radio lanRadio(server, config);

		lanRadio.run();
#if defined(DBG_ALLOCS)
		lan::memlog::info() << all_allocs;
#endif
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
	return 0;
}
