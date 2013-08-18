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
#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <string>
#include <memory>
#include <boost/utility.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <network/interface.hpp>

namespace net
{
	namespace config
	{
		struct section
		{
			virtual ~section() {}
			virtual bool has_value(const std::string& name) const = 0;
			virtual void set_value(const std::string& name, const std::string& svalue) = 0;
			virtual void set_value(const std::string& name, int ivalue) = 0;
			virtual void set_value(const std::string& name, bool bvalue) = 0;
			virtual std::string get_string(const std::string& name, const std::string& def_val) const = 0;
			virtual int get_int(const std::string& name, int def_val) const = 0;
			virtual bool get_bool(const std::string& name, bool def_val) const = 0;
		};
		typedef std::shared_ptr<section> section_ptr;

		template <typename T>
		struct get_value;

		template <>
		struct get_value<std::string>
		{
			static std::string helper(const section_ptr& sec, const std::string& name, const std::string& def_val)
			{
				return sec->get_string(name, def_val);
			}
		};

		template <>
		struct get_value<int>
		{
			static int helper(const section_ptr& sec, const std::string& name, int def_val)
			{
				return sec->get_int(name, def_val);
			}
		};

		template <>
		struct get_value<bool>
		{
			static bool helper(const section_ptr& sec, const std::string& name, bool def_val)
			{
				return sec->get_bool(name, def_val);
			}
		};

		struct config;

		static inline void ensure_section(config& parent, section_ptr& dst, const std::string& name);

		template <typename Value>
		struct setting: public boost::noncopyable
		{
			typedef setting<Value> my_type;
			config& m_parent;
			mutable section_ptr m_sec;
			std::string m_section;
			std::string m_name;
			Value m_def_val;

			setting(config& parent, const std::string& section, const std::string& name, const Value& def_val = Value())
				: m_parent(parent)
				, m_section(section)
				, m_name(name)
				, m_def_val(def_val)
			{}

			bool is_set() const
			{
				ensure_section();
				return m_sec->has_value(m_name);
			}

			operator Value() const
			{
				ensure_section();
				return get_value<Value>::helper(m_sec, m_name, m_def_val);
			}

			my_type& operator = (const Value& v)
			{
				ensure_section();
				m_sec->set_value(m_name, v);
				return *this;
			}

		private:
			void ensure_section() const
			{
				net::config::ensure_section(m_parent, m_sec, m_section);
			}
		};

		template <typename Value>
		std::ostream& operator << (std::ostream& o, const setting<Value>& rhs)
		{
			return o << static_cast<Value>(rhs);
		}

		template <>
		struct setting<boost::asio::ip::address_v4> : public boost::noncopyable
		{
			typedef boost::asio::ip::address_v4 Value;
			typedef setting<Value> my_type;
			config& m_parent;
			mutable section_ptr m_sec;
			std::string m_section;
			std::string m_name;
			mutable bool m_set;
			mutable Value m_cached;

			setting(config& parent, const std::string& section, const std::string& name)
				: m_parent(parent)
				, m_section(section)
				, m_name(name)
				, m_set(false)
			{}

			bool is_set() const
			{
				ensure_section();
				return m_sec->has_value(m_name);
			}

			operator Value() const
			{
				ensure_section();
				if (!m_set)
				{
					m_set = true;
					auto str = get_value<std::string>::helper(m_sec, m_name, "0.0.0.0");
					m_cached = Value::from_string(str);
					if (m_cached.is_unspecified())
						m_cached = net::iface::get_default_interface();
				}
				return m_cached;
			}

			my_type& operator = (const Value& v)
			{
				ensure_section();
				m_sec->set_value(m_name, v.to_string());
				return *this;
			}

		private:
			void ensure_section() const
			{
				net::config::ensure_section(m_parent, m_sec, m_section);
			}
		};

		struct config
		{
			config()
				: uuid(*this, "Server", "UUID")
				, port(*this, "Server", "Port", 6001)
				, iface(*this, "Server", "Interface")
			{}
			virtual ~config() {}
			virtual section_ptr get_section(const std::string& name) = 0;

			setting<std::string> uuid;
			setting<int> port;
			setting<boost::asio::ip::address_v4> iface;
		};
		typedef std::shared_ptr<config> config_ptr;

		static inline void ensure_section(config& parent, section_ptr& dst, const std::string& name)
		{
			if (!dst)
				dst = parent.get_section(name);
			if (!dst)
				throw std::runtime_error("Section " + name + " was not created");
		}

		config_ptr file_config(const boost::filesystem::path&);
	}
}

#endif // __LOG_HPP__