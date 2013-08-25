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

#ifndef __HTTP_CONNECTION_HPP__
#define __HTTP_CONNECTION_HPP__

#include <memory>
#include <set>
#include <vector>
#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <boost/asio.hpp>
#include <http/header_parser.hpp>
#include <http/http.hpp>
#include <http/request_handler.hpp>
#include <http/response.hpp>

namespace net
{
	namespace http
	{
		namespace queue
		{
			struct Event
			{
				std::function < void() > m_call;
				void operator()() { m_call(); }

				Event(const std::function< void() >& call)
					: m_call(call)
				{}
			};

			struct Queue: std::enable_shared_from_this<Queue>
			{
				Queue(const std::string& name);
				void stop();
				void run();
				void post(const std::function< void() >& ev);

				template <typename T>
				void on_start(T ev) { m_onStart = ev; }

				template <typename T>
				void on_stop(T ev) { m_onStop = ev; }

				const std::string& name() const { return m_name; }
				bool is_valid() const { return m_valid; }
			private:
				void call_next();

				std::deque<Event> m_queue;
				std::thread::id m_here;
				std::mutex m_mutex;
				std::condition_variable m_cv;
				bool m_done;
				bool m_valid;
				std::string m_name;

				std::function < void() > m_onStart, m_onStop;
			};
			typedef std::shared_ptr<Queue> worker_ptr;
		}

		struct connection;
		typedef std::shared_ptr<connection> connection_ptr;

		class connection_manager : private boost::noncopyable
		{
		public:
			connection_manager();

			/// Add the specified connection to the manager and start it.
			void start(connection_ptr c);

			/// Stop the specified connection.
			void stop(connection_ptr c);

			/// Stop all connections.
			void stop_all();

		private:
			/// The managed connections.
			std::set<connection_ptr> m_connections;
			std::vector<queue::worker_ptr> m_pool;
			std::vector<queue::worker_ptr>::iterator m_worker;
			std::mutex m_guard;
		};

		struct connection : private boost::noncopyable, public std::enable_shared_from_this<connection>
		{
			explicit connection(boost::asio::ip::tcp::socket && socket, connection_manager& manager, const request_handler_ptr& handler);

			void start() { read_some_more(); }
			void run();
			void stop() { m_socket.close(); }
		private:
			void read_some_more();
			bool parse_header(std::size_t bytes_transferred);
			void send_reply(bool send_body);
			static void continue_sending(connection_ptr self, response_buffer buffer, boost::system::error_code ec, std::size_t);

			std::array<char, 8192> m_buffer;
			boost::asio::ip::tcp::socket m_socket;
			http::header_parser<http::http_request> m_parser;
			connection_manager& m_manager;
			request_handler_ptr m_handler;
			response m_response;
			std::vector<char> m_response_chunk;
			int m_pos;
		};

	}
}

#endif // __HTTP_CONNECTION_HPP__
