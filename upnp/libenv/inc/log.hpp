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

		template <class Elem, class Traits>
		friend std::basic_ostream<Elem, Traits>& operator << (std::basic_ostream<Elem, Traits>& o, const Module& mod) { return o << mod.m_name.c_str(); }

		static Module HTTP;
		static Module SSDP;
		static Module UPnP;
	};

	template<class Elem, class Traits, class Alloc>
	class basic_linebuf : public std::basic_stringbuf<Elem, Traits, Alloc>, public boost::noncopyable
	{
		bool m_written;
	public:
		typedef basic_linebuf<Elem, Traits, Alloc> my_t;
		typedef std::basic_stringbuf<Elem, Traits, Alloc> mybase_t;
		typedef std::basic_string<Elem, Traits, Alloc> string_t;

		basic_linebuf()
			: mybase_t(std::ios_base::out)
			, m_written(false)
		{
		}

		bool has_written() const { return m_written; }
		void reset_state() { m_written = false; }

	protected:
		int_type overflow(int_type c = Traits::eof()) override
		{
			int_type ret = mybase_t::overflow(c);
			if (Traits::eq_int_type(ret, c))
				m_written = true;
			return ret;
		}

		std::streamsize xsputn(const char_type* s, std::streamsize n) override
		{
			if (n)
				m_written = true;
			return mybase_t::xsputn(s, n);
		}
	};

	template<class Elem, class Traits, class Alloc>
	class basic_line_stream;

	typedef basic_line_stream<char, std::char_traits<char>, std::allocator<char>> line_stream;
	typedef basic_line_stream<wchar_t, std::char_traits<wchar_t>, std::allocator<wchar_t>> line_wstream;

	namespace detail
	{
		void init_stream(line_stream&, Severity sev, const Module& mod);
		void finalize_stream(line_stream&);
		void init_stream(line_wstream&, Severity sev, const Module& mod);
		void finalize_stream(line_wstream&);
	}

	template<class Elem, class Traits, class Alloc>
	class basic_line_stream : public std::basic_ostream<Elem, Traits>, public boost::noncopyable
	{
	public:
		typedef Alloc allocator_type;

		typedef basic_line_stream<Elem, Traits, Alloc> my_t;
		typedef std::basic_ostream<Elem, Traits> mybase_t;
		typedef basic_linebuf<Elem, Traits, Alloc> mysb_t;
		typedef std::basic_string<Elem, Traits, Alloc> string_t;

		basic_line_stream(Severity sev, const Module& mod)
			: mybase_t(&m_linebuf)
		{
			detail::init_stream(*this, sev, mod);
		}
		~basic_line_stream()
		{
			detail::finalize_stream(*this);
		}

		bool has_written() const { return m_linebuf.has_written(); }
		void reset_state() { m_linebuf.reset_state(); }
		string_t str() const { return m_linebuf.str(); }

		mysb_t *rdbuf() const
		{
			// return pointer to buffer
			return ((mysb_t *) &m_linebuf);
		}

		std::string prefix;
	private:
		mysb_t m_linebuf;
	};

	template <typename T>
	class basic_log
	{
	public:
		template <Log::Severity sev>
		struct sev_line: line_stream
		{
			sev_line(): line_stream(sev, T::module()) {}
		};
		template <Log::Severity sev>
		struct mute_line : line_stream
		{
			mute_line() : line_stream(sev, T::module()) {}
			~mute_line() { reset_state(); }
		};
		template <Log::Severity sev>
		struct sev_wline: line_wstream
		{
			sev_wline() : line_wstream(sev, T::module()) {}
		};
		template <Log::Severity sev>
		struct mute_wline : line_wstream
		{
			mute_wline() : line_wstream(sev, T::module()) {}
			~mute_wline() { reset_state(); }
		};

#ifdef LOG_DEBUG
		typedef sev_line<Log::Severity::Debug> debug;
#else
		typedef mute_line<Log::Severity::Debug> debug;
#endif
		typedef sev_line<Log::Severity::Info> info;
		typedef sev_line<Log::Severity::Warning> warning;
		typedef sev_line<Log::Severity::Error> error;
#ifdef LOG_DEBUG
		typedef sev_wline<Log::Severity::Debug> wdebug;
#else
		typedef mute_wline<Log::Severity::Debug> wdebug;
#endif
		typedef sev_wline<Log::Severity::Info> winfo;
		typedef sev_wline<Log::Severity::Warning> wwarning;
		typedef sev_wline<Log::Severity::Error> werror;
	};
}

#endif // __LOG_HPP__