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

#include <sqlite3.hpp>
#include <sstream>
#include <log.hpp>

#ifdef _MSC_VER
#pragma comment(lib, "libsqlite.lib")
#endif

//REGISTER_DRIVER("sqlite", db::sqlite3::sqlite3_driver);
//REGISTER_DRIVER("sqlite3", db::sqlite3::sqlite3_driver);

namespace db
{
	extern Log::Module DATA;
	struct log : public Log::basic_log<log>
	{
		static const Log::Module& module() { return DATA; }
	};
}
namespace db { namespace sqlite3 {

	struct DriverData
	{
		std::string path;
		bool read(const driver::Props& props)
		{
			return 
				driver::getProp(props, "path", path) && !path.empty();
		}
	};

	connection_ptr sqlite3_driver::open(const std::string& ini_path, const Props& props)
	{
		DriverData data;
		if (!data.read(props))
		{
			log::error() << "SQLite3: invalid configuration";
			return nullptr;
		}

		try {
			auto conn = std::make_shared<sqlite3_connection>(ini_path);

			if (!conn->connect(data.path))
			{
				log::error() << "SQLite3: cannot connect to " << data.path << std::endl;
				return nullptr;
			}

			return conn;
		} catch(std::bad_alloc) { return nullptr; }
	}

	sqlite3_connection::sqlite3_connection(const std::string& path)
		: m_connected(false)
		, m_path(path)
		, m_db(nullptr)
#ifdef TRANSACTION_API_v2
		, m_transaction(this)
#endif
	{
	}

	sqlite3_connection::~sqlite3_connection()
	{
		if (m_connected)
			sqlite3_close(m_db);
	}

	bool sqlite3_connection::connect(const std::string& filename)
	{
		m_connected = sqlite3_open(filename.c_str(), &m_db) == SQLITE_OK;
		return m_connected;
	}

	bool sqlite3_connection::reconnect()
	{
		driver::Props props;
		if (!driver::readProps(m_path, props))
			return false;

		DriverData data;
		if (!data.read(props))
			return false;

		return connect(data.path);
	}

	bool sqlite3_connection::isStillAlive() const
	{
		return true;
	}

	bool sqlite3_connection::beginTransaction() const
	{
		return query(m_db, "BEGIN TRANSACTION");
	}

	bool sqlite3_connection::rollbackTransaction() const
	{
		return query(m_db, "ROLLBACK");
	}

	bool sqlite3_connection::commitTransaction() const
	{
		return query(m_db, "COMMIT");
	}

	statement_ptr sqlite3_connection::prepare(const char* sql) const
	{
		sqlite3_stmt* stmtptr = nullptr;
		size_t len = sql ? strlen(sql) + 1 : 0;
		if (sqlite3_prepare(m_db, sql, (int)len, &stmtptr, nullptr) || stmtptr == nullptr)
			return nullptr;

		try {
			auto stmt = std::make_shared<sqlite3_statement>(m_db, stmtptr,
				const_cast<sqlite3_connection*>(this)->shared_from_this());
			return stmt;
		} catch(std::bad_alloc) {
			sqlite3_finalize(stmtptr);
			return nullptr;
		}
	}

	statement_ptr sqlite3_connection::prepare(const char* sql, long lowLimit, long hiLimit) const
	{
		std::ostringstream s;
		s << sql << " LIMIT " << lowLimit << ", " << hiLimit;
		return prepare(s.str().c_str());
	}

	bool sqlite3_connection::exec(const char* sql) const
	{
		return query(m_db, sql);
	}

	const char* sqlite3_connection::errorMessage() const
	{
		return sqlite3_errmsg(m_db);
	}

	int sqlite3_connection::version() const
	{
		auto stmt = prepare("PRAGMA user_version");
		if (!stmt)
			return 0;

		auto cur = stmt->query();
		if (!cur || !cur->next())
			return 0;

		return cur->getInt(0);
	}

	void sqlite3_connection::version(int val)
	{
		std::ostringstream cmd;
		cmd << "PRAGMA user_version=" << val;
		query(m_db, cmd.str().c_str());
	}

	long long sqlite3_connection::last_rowid() const
	{
		auto ret = sqlite3_last_insert_rowid(m_db);
		if (!ret) ret = -1;
		return ret;
	}

	bool sqlite3_connection::query(db_handle* db, const char* stmt)
	{
		return sqlite3_exec(db, stmt, nullptr, nullptr, nullptr) == SQLITE_OK;
	}

	bool sqlite3_statement::bind(int arg, short value)
	{
		return sqlite3_bind_int(m_stmt, arg + 1, value) == SQLITE_OK;
	}

	bool sqlite3_statement::bind(int arg, long value)
	{
		return sqlite3_bind_int(m_stmt, arg + 1, value) == SQLITE_OK;
	}

	bool sqlite3_statement::bind(int arg, long long value)
	{
		return sqlite3_bind_int64(m_stmt, arg + 1, value) == SQLITE_OK;
	}

	bool sqlite3_statement::bind(int arg, const char* value)
	{
		if (!value)
			return bindNull(arg);

		size_t len = strlen(value);
		return sqlite3_bind_text(m_stmt, arg + 1, value, (int) len, SQLITE_TRANSIENT) == SQLITE_OK;
	}

	bool sqlite3_statement::bindTime(int arg, time value)
	{
		return sqlite3_bind_int64(m_stmt, arg + 1, value.val) == SQLITE_OK;
	}

	bool sqlite3_statement::bindNull(int arg)
	{
		return sqlite3_bind_null(m_stmt, arg + 1) == SQLITE_OK;
	}

	bool sqlite3_statement::execute()
	{
		auto ret = sqlite3_step(m_stmt);
		return ret == SQLITE_OK || ret == SQLITE_DONE;
	}

	cursor_ptr sqlite3_statement::query()
	{
		try {
			return std::make_shared<sqlite3_cursor>(m_db, m_stmt, shared_from_this());
		} catch(std::bad_alloc) { return nullptr; }
	}

	const char* sqlite3_statement::errorMessage()
	{
		return m_parent->errorMessage();
	}

	bool sqlite3_cursor::next()
	{
		return sqlite3_step(m_stmt) == SQLITE_ROW;
	}

	size_t sqlite3_cursor::columnCount()
	{
		return sqlite3_column_count(m_stmt);
	}

	long sqlite3_cursor::getLong(int column)
	{
		return sqlite3_column_int(m_stmt, column);
	}

	long long sqlite3_cursor::getLongLong(int column)
	{
		return sqlite3_column_int64(m_stmt, column);
	}

	time sqlite3_cursor::getTimestamp(int column)
	{
		return { (time_t) getLongLong(column) };
	}

	const char* sqlite3_cursor::getText(int column)
	{
		return (const char*)sqlite3_column_text(m_stmt, column);
	}

	bool sqlite3_cursor::isNull(int column)
	{
		return sqlite3_column_type(m_stmt, column) == SQLITE_NULL;
	}

}}
