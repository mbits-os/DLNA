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

#ifndef __SQLITE3_HPP__
#define __SQLITE3_HPP__

#include "sqlite3.h"
#include <dbconn.hpp>
#include <dbconn_driver.hpp>

namespace db
{
	namespace sqlite3
	{
		typedef ::sqlite3 db_handle;
		class sqlite3_cursor : public cursor
		{
			statement_ptr m_parent;
			db_handle* m_db;
			sqlite3_stmt* m_stmt;
		public:
			sqlite3_cursor(db_handle *db, sqlite3_stmt *stmt, const statement_ptr& parent)
				: m_parent(parent)
				, m_db(db)
				, m_stmt(stmt)
			{
			}
			~sqlite3_cursor()
			{
			}
			bool next() override;
			size_t columnCount() override;
			int getInt(int column) override { return getLong(column); }
			long getLong(int column) override;
			long long getLongLong(int column) override;
			time getTimestamp(int column) override;
			const char* getText(int column) override;
			bool isNull(int column) override;
			connection_ptr get_connection() const override { return m_parent->get_connection(); }
			statement_ptr get_statement() const override { return m_parent; }
		};

		class sqlite3_statement : public statement, public std::enable_shared_from_this<sqlite3_statement>
		{
			connection_ptr m_parent;
			db_handle* m_db;
			sqlite3_stmt* m_stmt;
		public:
			sqlite3_statement(db_handle* db, sqlite3_stmt *stmt, std::shared_ptr<connection> parent)
				: m_parent(parent)
				, m_db(db)
				, m_stmt(stmt)
			{
			}
			~sqlite3_statement()
			{
				sqlite3_finalize(m_stmt);
				m_stmt = nullptr;

			}
			bool bind(int arg, int value) override { return bind(arg, (long) value); }
			bool bind(int arg, short value) override;
			bool bind(int arg, long value) override;
			bool bind(int arg, long long value) override;
			bool bind(int arg, const char* value) override;
			bool bindTime(int arg, time value) override;
			bool bindNull(int arg) override;
			bool execute() override;
			cursor_ptr query() override;
			const char* errorMessage() override;
			connection_ptr get_connection() const override { return m_parent; }
		};

		class sqlite3_connection : public connection, public std::enable_shared_from_this<connection>
		{
			db_handle* m_db;
			bool m_connected;
			std::string m_path;
		public:
			sqlite3_connection(const std::string& path);
			~sqlite3_connection();
			bool connect(const std::string& filename);
			bool isStillAlive() const override;
			bool reconnect() override;
			bool beginTransaction() const override;
			bool rollbackTransaction() const override;
			bool commitTransaction() const override;
			statement_ptr prepare(const char* sql) const override;
			statement_ptr prepare(const char* sql, long lowLimit, long hiLimit) const override;
			bool exec(const char* sql) const override;
			const char* errorMessage() const override;
			int version() const override;
			void version(int val) override;

			static bool query(db_handle* db, const char* stmt);
		};

		class sqlite3_driver : public driver
		{
			connection_ptr open(const std::string& ini_path, const Props& props) override;
		};
	}
}

#endif //__SQLITE3_HPP__