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

#ifndef __DBCONN_DRIVER_HPP__
#define __DBCONN_DRIVER_HPP__

#include <memory>
#include <map>
#include <string>

namespace db
{
	struct connection;
	typedef std::shared_ptr<connection> connection_ptr;

	struct driver
	{
		typedef std::map<std::string, std::string> Props;
		static bool getProp(const Props& props, const std::string& name, std::string& value)
		{
			Props::const_iterator _it = props.find(name);
			if (_it == props.end()) return false;
			value = _it->second;
			return true;
		}
		static bool readProps(const std::string& path, Props& props);

		virtual ~driver() {}
		virtual connection_ptr open(const std::string& ini_path, const Props& props) = 0;
	};

	typedef std::shared_ptr<driver> driver_ptr;
	typedef std::map<std::string, driver_ptr> driver_map;
	class drivers
	{
		static drivers& get()
		{
			static drivers instance;
			return instance;
		}

		driver_map m_drivers;
		driver_ptr _driver(const std::string& name)
		{
			driver_map::iterator _it = m_drivers.find(name);
			if (_it == m_drivers.end()) return driver_ptr();
			return _it->second;
		}

		void _register(const std::string& name, const driver_ptr& ptr)
		{
			m_drivers[name] = ptr;
		}
	public:
		static driver_ptr driver(const std::string& name)
		{
			return get()._driver(name);
		}

		static void register_raw(const std::string& name, db::driver* rawPtr)
		{
			if (!rawPtr)
				return;

			driver_ptr ptr(rawPtr);
			get()._register(name, ptr);
		}
	};

	template <typename DriverImpl>
	struct driver_registrar
	{
		driver_registrar(const std::string& resource)
		{
			try { drivers::register_raw(resource, new DriverImpl()); } catch (std::bad_alloc) {}
		}
	};
}

#define REGISTRAR_NAME_2(name, line) name ## _ ## line
#define REGISTRAR_NAME_1(name, line) REGISTRAR_NAME_2(name, line)
#define REGISTRAR_NAME(name) REGISTRAR_NAME_1(name, __LINE__)
#define REGISTER_DRIVER(resource, type) static db::driver_registrar<type> REGISTRAR_NAME(driver) (resource)

#endif //__DBCONN_DRIVER_HPP__
