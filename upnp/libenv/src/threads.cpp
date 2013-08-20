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
#include <threads.hpp>
#include <thread>
#include <mutex>
#include <string>
#include <algorithm>
#include <sstream>

namespace threads
{
	class info
	{
		typedef std::pair<std::thread::id, std::string> name_t;
		std::list<name_t> m_names;
		std::mutex m_guard;
	public:
		static info& get()
		{
			static info obj;
			return obj;
		}

		std::string get_name()
		{
			auto id = std::this_thread::get_id();
			{
				std::lock_guard<std::mutex> guard(m_guard);
				for (auto && n : m_names)
				{
					if (n.first == id)
						return n.second;
				}
			}

			std::ostringstream o;
			o << "#" << id;
			return o.str();
		}

		void set_name(const std::string& name)
		{
			auto id = std::this_thread::get_id();
			std::lock_guard<std::mutex> guard(m_guard);
			for (auto && n : m_names)
			{
				if (n.first == id)
				{
					n.second = name;
					return;
				}
			}
			m_names.emplace_back(id, name);
		}
	};

	std::string get_name()
	{
		return info::get().get_name();
	}

	void set_name(const std::string& name)
	{
		info::get().set_name(name);
	}

}
