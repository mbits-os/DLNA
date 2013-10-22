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

#include "schema.hpp"
#include <log.hpp>

namespace db
{
	Log::Module SQLite{ "SQLite" };
	struct log : public Log::basic_log<log>
	{
		static const Log::Module& module() { return SQLite; }
	};
#define SQLITE(ev) if (!(ev)) { log::error() << db()->errorMessage(); return err_value; }
#define SQLITE_(ev, db) if (!(ev)) { log::error() << (db)->errorMessage(); return err_value; }

	enum { DB_VERSION = 1 };
	schema::schema()
		: database_helper("data.ini", DB_VERSION)
	{
	}

	template <size_t length>
	bool run(const db::connection_ptr& db, const char* (&program)[length])
	{
		bool err_value = false;
		for (auto&& line : program)
		{
			SQLITE_(db->exec(line), db);
		}
		return true;
	}

	bool schema::upgrade_schema(int current_version, int /*new_version*/)
	{
		if (current_version < 1)
		{
#include "schema_v1_sql.hpp"
			if (!run(db(), schema_v1_sql))
				return false;
		}
		return true;
	}

	static inline boost::filesystem::path normalize(const boost::filesystem::path& file)
	{
		return file.filename() == "." ? file.parent_path() : file;
	}

	long long schema::add_root(const boost::filesystem::path& file) const
	{
		auto tmp = normalize(file);

		BEGIN_TRANSACTION( db() );
		RETURN_AND_COMMIT(add_file(tmp, 0));
		END_TRANSACTION_RETURN(-1);
	}

	long long schema::add_file(const boost::filesystem::path& file) const
	{
		auto tmp = normalize(file);
		auto parent = tmp.parent_path();

		BEGIN_TRANSACTION(db());

		auto parent_id = file_id(parent);
		if (parent_id == -1)
			return -1;

		RETURN_AND_COMMIT(add_file(tmp, parent_id));
		END_TRANSACTION_RETURN(-1);
	}

	long long schema::add_file(const boost::filesystem::path& file, long long parent) const
	{
		long long err_value = -1;
		BEGIN_TRANSACTION(db());

		auto here = file_id(file);
		if (here != -1)
			return here;

		auto name = file.filename();
		auto query = db()->prepare("INSERT INTO entries (filepath, parent, last_write_time, title) VALUES (?, ?, 0, ?)");
		SQLITE(query);

		SQLITE(query->bind(0, file.string().c_str()));
		SQLITE(query->bind(1, parent));
		SQLITE(query->bind(2, name.string().c_str()));

		SQLITE(query->execute());

		RETURN_AND_COMMIT(db()->last_rowid());
		END_TRANSACTION_RETURN(err_value);
	}

	long long schema::file_id(const boost::filesystem::path& file) const
	{
		long long err_value = -1;
		auto query = db()->prepare("SELECT _id FROM entries WHERE filepath=?");
		SQLITE(query && query->bind(0, file.string().c_str()));

		auto c = query->query();
		if (!c || !c->next())
			return -1;

		return c->getLongLong(0);
	}

}