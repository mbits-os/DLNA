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
 * connection WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __DBCONN_H__
#define __DBCONN_H__

#include <memory>
#include <list>
#include <vector>
#include <mutex>

namespace db
{
	struct cursor;
	struct statement;
	struct connection;
	typedef std::shared_ptr<connection> connection_ptr;
	typedef std::shared_ptr<statement> statement_ptr;
	typedef std::shared_ptr<cursor> cursor_ptr;

	struct time
	{
		time_t val;
	};

	struct cursor
	{
		virtual ~cursor() {}
		virtual bool next() = 0;
		virtual size_t columnCount() = 0;
		virtual int getInt(int column) = 0;
		virtual long getLong(int column) = 0;
		virtual long long getLongLong(int column) = 0;
		virtual time getTimestamp(int column) = 0;
		virtual const char* getText(int column) = 0;
		virtual bool isNull(int column) = 0;
		virtual connection_ptr get_connection() const = 0;
		virtual statement_ptr get_statement() const = 0;
	};

	template <typename Type> struct selector;
	template <typename Type> struct struct_def;

	template <>
	struct selector<int> { static int get(const cursor_ptr& c, int column) { return c->getInt(column); } };

	template <>
	struct selector<long> { static long get(const cursor_ptr& c, int column) { return c->getLong(column); } };

	template <>
	struct selector<long long> { static long long get(const cursor_ptr& c, int column) { return c->getLongLong(column); } };

	template <>
	struct selector<time> { static time get(const cursor_ptr& c, int column) { return c->getTimestamp(column); } };

	template <>
	struct selector<const char*> { static const char* get(const cursor_ptr& c, int column) { return c->getText(column); } };

	template <>
	struct selector<std::string> { static std::string get(const cursor_ptr& c, int column) { return c->isNull(column) ? std::string() : c->getText(column); } };

	struct selector_base
	{
		virtual ~selector_base() {}
		virtual bool get(const cursor_ptr& c, void* context) = 0;
	};
	typedef std::shared_ptr<selector_base> selector_base_ptr;

	template <typename Type, typename Member>
	struct member_selector: selector_base
	{
		int m_column;
		Member Type::* m_member;
		member_selector(int column, Member Type::* member)
			: m_column(column)
			, m_member(member)
		{
		}

		bool get(const cursor_ptr& c, void* context)
		{
			Type* ctx = (Type*)context;
			if (!ctx)
				return false;

			ctx->*m_member = selector<Member>::get(c, m_column);
			return true;
		}
	};

	template <typename Type>
	struct cursor_struct
	{
		std::list<selector_base_ptr> m_selectors;

		template <typename Member>
		void add(int column, Member Type::* dest)
		{
			m_selectors.push_back(std::make_shared< member_selector<Type, Member> >(column, dest));
		}

		bool get(const cursor_ptr& c, Type& ctx)
		{
			auto cur = m_selectors.begin(), end = m_selectors.end();
			for (; cur != end; ++cur)
			{
				selector_base_ptr& selector = *cur;
				if (!selector->get(c, &ctx))
					return false;
			}
			return true;
		};

		bool get(const cursor_ptr& c, std::list<Type>& ctx)
		{
			while (c->next())
			{
				Type item;
				if (!get(c, item))
					return false;
				ctx.push_back(item);
			}
			return true;
		};

		bool get(const cursor_ptr& c, std::vector<Type>& ctx)
		{
			while (c->next())
			{
				Type item;
				if (!get(c, item))
					return false;
				ctx.push_back(item);
			}
			return true;
		};
	};

	template <typename Type> 
	static inline bool get(const cursor_ptr& c, Type& t)
	{
		return struct_def<Type>().get(c, t);
	}

	template <typename Type> 
	static inline bool get(const cursor_ptr& c, std::list<Type>& l)
	{
		return struct_def<Type>().get(c, l);
	}

	template <typename Type> 
	static inline bool get(const cursor_ptr& c, std::vector<Type>& l)
	{
		return struct_def<Type>().get(c, l);
	}

	struct statement
	{
		virtual ~statement() {}
		virtual bool bind(int arg, int value) = 0;
		virtual bool bind(int arg, short value) = 0;
		virtual bool bind(int arg, long value) = 0;
		virtual bool bind(int arg, long long value) = 0;
		virtual bool bind(int arg, const char* value) = 0;
		virtual bool bindTime(int arg, time value) = 0;
		virtual bool bindNull(int arg) = 0;
		virtual bool execute() = 0;
		virtual cursor_ptr query() = 0;
		virtual const char* errorMessage() = 0;
		virtual connection_ptr get_connection() const = 0;
	};

#define TRANSACTION_API_v2
#ifdef TRANSACTION_API_v2
	struct connection;
	class transaction_mutex
	{
		enum state
		{
			UNKNOWN,
			BEGAN,
			COMMITED,
			REVERTED
		};

		connection* m_conn;
		std::recursive_mutex m_mutex;
		unsigned long m_counter;
		state m_state;
		bool m_commited;

		bool finish_transaction(bool commit);
	public:
		transaction_mutex(connection*);
		void lock();
		void unlock();
		void commit();
	};

	struct transaction_error : std::runtime_error
	{
		explicit transaction_error(const std::string& msg, unsigned long counter) : runtime_error("Transaction error: " + msg), m_counter(counter) {}
		unsigned long m_counter;

		bool nested()
		{
			if (m_counter <= 1)
				return false;
			--m_counter;
			return true;
		}
	};

	struct transaction
	{
		transaction(transaction_mutex& m)
			: mutex(m)
			, on_return_commit(false)
			, throwing(false)
		{
			__lock();
		}
		inline transaction(const connection_ptr& conn);
		~transaction()
		{
			if (!throwing && on_return_commit)
			{
				mutex.commit();
			}
			mutex.unlock();
		}
		transaction(transaction&) = delete;
		transaction& operator = (transaction&) = delete;
		void commit()
		{
			try
			{
				mutex.commit();
			}
			catch (...)
			{
				throwing = true;
				throw;
			}
		}

		void on_return(bool commit) { on_return_commit = commit; }
	private:
		transaction_mutex& mutex;
		bool on_return_commit;
		bool throwing;

		void __lock()
		{
			try
			{
				mutex.lock();
			}
			catch (...)
			{
				throwing = true;
				throw;
			}
		}

	};

#define BEGIN_TRANSACTION(source) try { db::transaction tr{ source }
#define ON_RETURN_COMMIT() tr.on_return(true)
#define ON_RETURN_ROLLBACK() tr.on_return(false)
#define RETURN_AND_COMMIT(value) { auto ret_val = (value); tr.commit(); return ret_val; }
#define END_TRANSACTION(action) } catch (db::transaction_error& e) { if (e.nested()) throw; action }
#define END_TRANSACTION_LOG_RETURN(value) END_TRANSACTION({ log::error() << e.what(); return (value); })
#define END_TRANSACTION_RETURN(value) END_TRANSACTION(return (value);)

#endif

	struct connection
	{
		virtual ~connection() {}
		virtual bool isStillAlive() const = 0;
		virtual bool exec(const char* sql) const = 0;
		virtual statement_ptr prepare(const char* sql) const = 0;
		virtual statement_ptr prepare(const char* sql, long lowLimit, long hiLimit) const = 0;
		virtual const char* errorMessage() const = 0;
		virtual bool reconnect() = 0;
		virtual int version() const = 0;
		virtual void version(int val) = 0;
		virtual long long last_rowid() const = 0;
		static connection_ptr open(const char* path);
#ifdef TRANSACTION_API_v2
		virtual transaction_mutex& transaction() = 0;
	private:
		friend class transaction_mutex;
#endif
		virtual bool beginTransaction() const = 0;
		virtual bool rollbackTransaction() const = 0;
		virtual bool commitTransaction() const = 0;
	};

#ifdef TRANSACTION_API_v2
	inline transaction::transaction(const connection_ptr& conn)
		: mutex(conn->transaction())
		, on_return_commit(false)
		, throwing(false)
	{
		__lock();
	}
#endif

#ifndef TRANSACTION_API_v2
	struct transaction
	{
		enum state
		{
			UNKNOWN,
			BEGAN,
			COMMITED,
			REVERTED
		};

		state m_state;
		connection_ptr m_conn;
		transaction(const connection_ptr& conn): m_state(UNKNOWN), m_conn(conn) {}
		~transaction()
		{
			if (m_state == BEGAN)
				m_conn->rollbackTransaction();
		}
		bool begin()
		{
			if (m_state != UNKNOWN)
				return false;
			if (!m_conn->beginTransaction())
				return false;
			m_state = BEGAN;
			return true;
		}
		bool commit()
		{
			if (m_state != BEGAN)
				return false;
			m_state = COMMITED;
			return m_conn->commitTransaction();
		}
		bool rollback()
		{
			if (m_state != BEGAN)
				return false;
			m_state = REVERTED;
			return m_conn->rollbackTransaction();
		}

		bool commited() const { return m_state == COMMITED; }
	};
#endif

	class database_helper
	{
		std::string m_path;
		int m_version;
		connection_ptr m_db;

	protected:
		database_helper(const std::string& path, int version)
			: m_path(path)
			, m_version(version)
		{
		}

		virtual bool upgrade_schema(int current_version, int new_version) = 0;
		const connection_ptr& db() const { return m_db; }

	public:
		bool open();
	};
};

#define CURSOR_RULE(type) \
	template <> \
	struct struct_def<type>: cursor_struct<type> \
	{ \
		typedef type Type; \
		struct_def(); \
	}; \
	struct_def<type>::struct_def()
#define CURSOR_ADD(column, name) add(column, &Type::name)

#endif //__DBCONN_H__
