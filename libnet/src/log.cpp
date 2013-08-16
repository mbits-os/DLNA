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
#include <log.hpp>
#include <boost/date_time.hpp>
#include <boost/filesystem.hpp>
#include <network/types.hpp>
#include <clocale>
#include <locale>
#include <vector>

namespace fs = boost::filesystem;

namespace Log
{
	Module Module::HTTP("HOST");
	Module Module::SSDP("SSDP");

	template <class Elem, class Traits>
	std::basic_ostream<Elem, Traits>& operator << (std::basic_ostream<Elem, Traits>& o, Severity sev)
	{
		switch (sev)
		{
		case Severity::Debug:   return o << "DBG";
		case Severity::Info:    return o << "INF";
		case Severity::Warning: return o << "WRN";
		case Severity::Error:   return o << "ERR";
		}
		throw std::runtime_error("Severity value " + std::to_string((int)sev) + " unknown");
	}

	struct sink
	{
		static void log(const std::string& msg);
		static void log(const std::wstring& msg);
	};

	namespace detail
	{
		void init_stream(line_stream& s, Severity sev, const Module& mod)
		{
			s << net::to_iso8601(net::time::now()) << " " << mod << "/" << sev << " ";
			s.reset_state();
		}
		void finalize_stream(line_stream& s)
		{
			if (s.has_written())
			{
				s << std::endl;
				sink::log(s.str());
			}
		}
		void init_stream(line_wstream& s, Severity sev, const Module& mod)
		{
			s << net::to_iso8601(net::time::now()).c_str() << " " << mod << "/" << sev << " ";
			s.reset_state();
		}
		void finalize_stream(line_wstream& s)
		{
			if (s.has_written())
			{
				s << std::endl;
				sink::log(s.str());
			}
		}
	}

	void sink::log(const std::string& msg)
	{
		std::ofstream out("libnet.log", std::ios::app | std::ios::out);
		out << msg;
	}

	void sink::log(const std::wstring& wmsg)
	{
		std::setlocale(LC_ALL, "");
		const std::locale locale("");
		typedef std::codecvt<wchar_t, char, std::mbstate_t> converter_type;

		const auto& converter = std::use_facet<converter_type>(locale);

		std::vector<char> data(wmsg.length() * converter.max_length());
		std::mbstate_t state;
		const wchar_t* from_next;
		char* to_next;

		auto result = converter.out(state, wmsg.data(), wmsg.data() + wmsg.length(), from_next, &data[0], &data[0] + data.size(), to_next);

		if (result == converter_type::ok || result == converter_type::noconv) {
			std::ofstream out("libnet.log", std::ios::app | std::ios::out);
			out << std::string(data.data(), to_next);
		}
	}
}
