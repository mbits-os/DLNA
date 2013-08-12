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

#ifndef __HTTP_MIME_HPP__
#define __HTTP_MIME_HPP__

#include <cctype>
#include <string>
#include <unordered_map>
#include <sstream>
#include <boost/utility.hpp>

namespace net
{
	namespace mime
	{
		namespace detail
		{
			class ostrrefstream: boost::noncopyable
			{
				std::string& m_dest;
				std::ostringstream m_oss;
				bool m_appending;
				bool m_valid;
			public:
				ostrrefstream(std::string& dest, bool appending)
					: m_dest(dest)
					, m_appending(appending)
					, m_valid(false)
				{}
				ostrrefstream(ostrrefstream && rhs)
					: m_dest(rhs.m_dest)
					, m_oss(std::move(rhs.m_oss))
					, m_appending(rhs.m_appending)
					, m_valid(rhs.m_valid)
				{
					rhs.m_valid = false;
				}
				~ostrrefstream()
				{
					if (m_valid)
						if (m_appending)
							m_dest.append(m_oss.str());
						else
							m_dest = m_oss.str();
				}

				template <typename T>
				ostrrefstream& operator << (T data)
				{
					m_valid = true;
					m_oss << data;
					return *this;
				}
			};
		}

		class header
		{
			std::string m_name;
			std::string m_value;
			static inline std::string tolower(std::string s)
			{
				for (auto&& c : s)
					c = std::tolower((unsigned char) c);
				return s;
			}
			static inline std::string camel_case(std::string s)
			{
				bool capitalize = true;
				for (auto&& c : s)
				{
					if (capitalize)
						c = std::toupper((unsigned char) c);
					capitalize = c == '-' || c == '.'; // Dot for SSDP Extension '.' domain headers
				}
				return s;
			}
		public:
			header(const std::string & name = std::string(), const std::string & value = std::string())
				: m_name(tolower(name))
				, m_value(value)
			{}
			bool operator == (const std::string& name) const { return m_name == tolower(name); }
			bool operator != (const std::string& name) const { return !(*this == name); }

			const std::string& name() const { return m_name; }
			const std::string& value() const { return m_value; }
			std::string& value() { return m_value; }
			detail::ostrrefstream out() { return detail::ostrrefstream(m_value, false); }
			detail::ostrrefstream append() { return detail::ostrrefstream(m_value, true); }
			void append(const std::string& s) { m_value.append(s); }

			friend std::ostream& operator << (std::ostream& o, const header& h)
			{
				return  o << camel_case(h.m_name) << ": " << h.m_value << "\r\n";
			}
		};

		class headers
		{
		public:
			typedef std::vector<header> cont_t;
			typedef cont_t::iterator iterator;
			typedef cont_t::const_iterator const_iterator;
			typedef cont_t::pointer pointer;
			typedef cont_t::reference reference;

			iterator begin() { return m_headers.begin(); }
			iterator end() { return m_headers.end(); }
			const_iterator begin() const { return m_headers.begin(); }
			const_iterator end() const { return m_headers.end(); }

			template <typename T>
			T find_as(const std::string& name, const T& def_value = T()) const
			{
				auto it = find(name);
				if (it == end())
					return def_value;

				T ret;
				std::istringstream i(it->value());
				i >> ret;
				return ret;
			}

			iterator find(const std::string& name)
			{
				auto c = begin(), e = end();
				while (c != e)
				{
					if (*c == name)
						break;
					++c;
				}
				return c;
			}

			const_iterator find(const std::string& name) const
			{
				auto c = begin(), e = end();
				while (c != e)
				{
					if (*c == name)
						break;
					++c;
				}
				return c;
			}

			iterator append(const std::string & name)
			{
				auto it = find(name);
				if (it == end())
					it = m_headers.emplace(it, name);

				return it;
			}

			iterator append(const std::string & name, const std::string & value)
			{
				auto it = find(name);
				if (it == end())
					it = m_headers.emplace(it, name, value);
				else
					it->value().append(value);

				return it;
			}

			void clear()
			{
				m_headers.clear();
			}
		private:
			cont_t m_headers;
		};

		inline std::ostream& operator << (std::ostream& o, const headers& hh)
		{
			for (auto&& h : hh)
				o << h;
			return o;
		}
	}
}
#endif // __HTTP_MIME_HPP__
