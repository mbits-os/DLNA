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
#ifndef __LOG_HPP__
#define __LOG_HPP__

#include <string>
#include <sstream>
#include <boost/utility.hpp>

namespace Log
{
	enum class Severity
	{
		Debug,
		Info,
		Warning,
		Error
	};

	class Module
	{
		std::string m_name;
	public:
		Module(const std::string& name) : m_name(name) {}

		friend std::ostream& operator << (std::ostream& o, const Module& mod) { return o << mod.m_name; }
		static Module HTTP;
		static Module SSDP;
	};

	class line: public boost::noncopyable
	{
		std::ostringstream out;
		bool called;
	public:
		line(Severity sev, const Module& mod);
		~line();

		template <typename T>
		line& operator << (const T& o)
		{
			called = true;
			out << o;
			return *this;
		}

		std::ostream& stream() { return out; }
	};

	template <typename T>
	class basic_log
	{
	public:
		template <Log::Severity sev>
		struct sev_line: line
		{
			sev_line(): line(sev, T::module()) {}
		};
		typedef sev_line<Log::Severity::Debug> debug;
		typedef sev_line<Log::Severity::Info> info;
		typedef sev_line<Log::Severity::Warning> warning;
		typedef sev_line<Log::Severity::Error> error;
	};
}

#endif // __LOG_HPP__