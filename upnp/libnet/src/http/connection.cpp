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
#include <http/connection.hpp>
#include <utils.hpp>
#include <iostream>
#include <log.hpp>
#include <threads.hpp>

namespace net
{
	namespace http
	{
		struct log : public Log::basic_log<log>
		{
			static const Log::Module& module() { return Log::Module::HTTP; }
		};

		namespace queue
		{
			Queue::Queue(const std::string& name)
				: m_done(false)
				, m_name(name)
			{
			}

			void Queue::stop()
			{
				post([&] { m_done = true; });

				std::unique_lock<std::mutex> lock(m_mutex);
				m_cv.wait(lock, [&]() { return m_here == std::thread::id(); });
			}

			void Queue::run()
			{
				auto shared = shared_from_this();
				auto th = std::thread([this, shared]
				{
					threads::set_name(m_name);
					m_here = std::this_thread::get_id();
					m_cv.notify_one();

					log::info() << "Worker started";

					if (m_onStart)
						m_onStart();

					while (!m_done)
						call_next();

					if (m_onStop)
						m_onStop();

					log::info() << "Worker stopped";
					m_here = std::thread::id();
					m_cv.notify_one();
				});

				{
					std::unique_lock<std::mutex> lock(m_mutex);
					m_cv.wait(lock, [&]() { return m_here != std::thread::id(); });
				}

				th.detach();
			}

			void Queue::post(const std::function< void() >& ev)
			{
				{
					std::lock_guard<std::mutex> lock(m_mutex);

					auto thread = std::this_thread::get_id();
					if (thread == m_here)
					{
						ev();
						return;
					}

					m_queue.emplace_back(ev);
				}
				m_cv.notify_one();
			}

			void Queue::call_next()
			{
				{
					std::unique_lock<std::mutex> lock(m_mutex);
					m_cv.wait(lock, [&]() { return !m_queue.empty(); });
				}

				m_mutex.lock();

				auto ev = std::move(m_queue.front());
				m_queue.pop_front();

				m_mutex.unlock();

				ev();
			}
		}

		connection_manager::connection_manager()
		{
			int count = 5; // TODO: should be based on OS capabilities
			for (int i = 0; i < count; ++i)
				m_pool.push_back(std::make_shared<queue::Queue>("Web Thread #" + std::to_string(i)));

			for (auto&& worker: m_pool)
				worker->run();

			m_worker = m_pool.end();
		}

		void connection_manager::start(connection_ptr c)
		{
			if (m_worker == m_pool.end())
				m_worker = m_pool.begin();

			m_connections.insert(c);

			if (m_worker != m_pool.end()) // empty pool?
			{
				(*m_worker)->post([c]{ c->run(); });
				++m_worker;
			}
			else
				c->start();
		}

		void connection_manager::stop(connection_ptr c)
		{
			m_connections.erase(c);
			c->stop();
		}

		void connection_manager::stop_all()
		{
			for (auto c : m_connections)
				c->stop();
			m_connections.clear();

			for (auto worker : m_pool)
				worker->stop();
			m_pool.clear();
		}

		connection::connection(boost::asio::ip::tcp::socket && socket, connection_manager& manager, const request_handler_ptr& handler)
			: m_socket(std::move(socket))
			, m_manager(manager)
			, m_handler(handler)
			, m_pos(0)
		{
		}

		struct buffer
		{
			const char* data;
			size_t len;
			buffer(const char* data, size_t len) : data(data), len(len) {}
		};

		std::ostream& operator << (std::ostream& o, const buffer& b)
		{
			const char* c = b.data;
			const char* e = c + b.len;
			for (; c != e; ++c)
				o.put(*c >= ' ' && *c <= 127 ? *c : '.');

			return o;
		};

		bool parse_range(const std::string& range_s, long long& lower, long long& upper)
		{
			const char* c = range_s.c_str();
			const char* e = c + range_s.length();

			while (c != e && *c == ' ') ++c;
			if (c == e || *c++ != 'b') return false;
			if (c == e || *c++ != 'y') return false;
			if (c == e || *c++ != 't') return false;
			if (c == e || *c++ != 'e') return false;
			if (c == e || *c++ != 's') return false;
			while (c != e && *c == ' ') ++c;
			if (c == e || *c++ != '=') return false;
			while (c != e && *c == ' ') ++c;

			if (c == e) return false;

			lower = -1;
			if (std::isdigit((unsigned char) *c))
			{
				lower = 0;
				while (std::isdigit((unsigned char) *c))
				{
					lower *= 10;
					lower += *c++ - '0';

					if (c == e)
						return false;
				}
			}

			if (*c++ != '-')
				return false;

			upper = -1;

			if (c == e)
				return upper != lower; // eiter "-" or the "500-" case

			if (std::isdigit((unsigned char) *c))
			{
				upper = 0;
				while (std::isdigit((unsigned char) *c))
				{
					upper *= 10;
					upper += *c++ - '0';

					if (c == e)
						break;
				}

				if (upper < lower)
					return false;

				if (c == e)
					return true;
			}

			while (c != e && *c == ' ') ++c; // if this is multi-range, the comma will be cought here
			return c == e; // we do not support multi-ranges.
		}
		bool connection::parse_header(std::size_t bytes_transferred)
		{
			const char* data = m_buffer.data();
			auto end = data + bytes_transferred;
			auto ret = m_parser.parse(data, end);
			m_pos += bytes_transferred;

			if (ret == parser::finished)
			{
				auto request = m_parser.header();

				auto range_it = request.find("range");
				if (range_it != request.end())
				{
					long long lower = -1;
					long long upper = -1;
					if (parse_range(range_it->value(), lower, upper))
						m_response.set_range(lower, upper);
				}

				auto content_length = request.find_as<size_t>("content-length");

				request.remote_endpoint(m_socket.remote_endpoint());
				request.request_data(make_request_data(m_socket, content_length, data, end - data));

				m_handler->handle(request, m_response);
				send_reply(request.method() != http_method::head);
				return true;
			}
			else if (ret == parser::error)
			{
				log::error()
					<< "[CONNECTION] Parse error: " << buffer(m_buffer.data(), std::min(bytes_transferred, (size_t) 100)) << " (starting at " << (m_pos - bytes_transferred) << " bytes)";
				m_handler->make_404(m_response);
				send_reply(true);
				return true;
			}
			return false;
		}
		void connection::run()
		{
			while (true)
			{
				boost::system::error_code ec;
				size_t bytes_transferred = m_socket.read_some(boost::asio::buffer(m_buffer), ec);
				if (!ec)
				{
					if (parse_header(bytes_transferred))
						return;
				}
				else if (ec != boost::asio::error::operation_aborted)
				{
					return m_manager.stop(shared_from_this());
				}
			}
		}
		void connection::read_some_more()
		{
			auto self(shared_from_this());
			m_socket.async_read_some(boost::asio::buffer(m_buffer),
				[this, self](const boost::system::error_code& ec, std::size_t bytes_transferred)
			{
				if (!ec)
				{
					if (!parse_header(bytes_transferred))
						read_some_more();
				}
				else if (ec != boost::asio::error::operation_aborted)
				{
					m_manager.stop(shared_from_this());
				}
			});
		}

		void connection::continue_sending(connection_ptr self, response_buffer buffer, boost::system::error_code ec, std::size_t)
		{
			if (!ec)
			{
				if (buffer.advance(self->m_response_chunk))
				{
					//std::cout.write(self->m_response_chunk.data(), self->m_response_chunk.size());
					boost::asio::async_write(
						self->m_socket, boost::asio::buffer(self->m_response_chunk),
						[self, buffer](boost::system::error_code ec, std::size_t size)
					{
						continue_sending(self, buffer, ec, size);
					});
					return;
				}
				else
				{
					// Initiate graceful connection closure.
					boost::system::error_code ignored_ec;
					self->m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
				}
			}

			if (ec != boost::asio::error::operation_aborted)
			{
				self->m_manager.stop(self->shared_from_this());
			}
		}

		void connection::send_reply(bool send_body)
		{
			if (!send_body)
			{
				m_response.complete_header();
				m_response.content(nullptr);
			}

			auto self(shared_from_this());
			continue_sending(self, m_response.get_data(), boost::system::error_code(), 0);
		}
	}
}
