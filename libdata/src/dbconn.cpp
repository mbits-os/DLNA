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

#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <dbconn.hpp>
#include <dbconn_driver.hpp>
#include <log.hpp>

namespace db
{
	Log::Module DATA { "DATA" };
	struct log: public Log::basic_log<log>
	{
		static const Log::Module& module() { return DATA; }
	};

	bool driver::readProps(const std::string& path, Props& props)
	{
		std::ifstream ini(path);
		if (!ini.is_open())
			return false;

		std::string line;
		while (!ini.eof())
		{
			ini >> line;
			std::string::size_type enter = line.find('=');
			if (enter != std::string::npos)
				props[line.substr(0, enter)] = line.substr(enter + 1);
		}
		return true;
	}

#ifdef TRANSACTION_API_v2
	transaction_mutex::transaction_mutex(connection* conn)
		: m_conn(conn)
		, m_counter(0)
		, m_state(UNKNOWN)
		, m_commited(false)
	{
	}
	void transaction_mutex::lock()
	{
		m_mutex.lock();
		if (!m_counter)
		{
			if (!m_conn->beginTransaction())
				throw transaction_error(m_conn->errorMessage(), m_counter);
			m_state = BEGAN;
		}
		++m_counter;
	}

	void transaction_mutex::unlock()
	{
		(void) finish_transaction(false);
		m_mutex.unlock();
	}

	void transaction_mutex::commit()
	{
		if (!finish_transaction(true))
			throw transaction_error(m_conn->errorMessage(), m_counter);
	}

	bool transaction_mutex::finish_transaction(bool commit)
	{
		if (m_state != BEGAN)
			return true;

		--m_counter;
		m_state = commit ? COMMITED : REVERTED;
		if (!m_counter)
			return commit ? m_conn->commitTransaction() : m_conn->rollbackTransaction();

		return true;
	}
#endif

	connection_ptr connection::open(const char* path)
	{
		driver::Props props;
		if (!driver::readProps(path, props))
		{
			log::error() << "Cannot open " << path << std::endl;
			return nullptr;
		}

		std::string driver_id;
		if (!driver::getProp(props, "driver", driver_id))
		{
			log::error() << "Connection configuration is missing the `driver' key\n";
			return nullptr;
		}

		driver_ptr driver = drivers::driver(driver_id);
		if (driver.get() == nullptr)
		{
			log::error() << "Cannot find `" << driver_id << "' database driver\n";
			return nullptr;
		}

		return driver->open(path, props);
	}

	bool database_helper::open()
	{
		m_db = connection::open(m_path.c_str());
		if (!m_db)
			return false;
		int version = m_db->version();
		if (version >= m_version)
			return version == m_version;

		BEGIN_TRANSACTION( m_db );
		if (upgrade_schema(version, m_version))
		{
			ON_RETURN_COMMIT();
			m_db->version(m_version);
		}
		return true;
		END_TRANSACTION_LOG_RETURN(false);
	}
}