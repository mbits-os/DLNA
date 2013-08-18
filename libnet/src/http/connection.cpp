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
#include <network/utils.hpp>
#include <iostream>
#include <log.hpp>

namespace net
{
	namespace http
	{
		void connection_manager::start(connection_ptr c)
		{
			m_connections.insert(c);
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
		}

		connection::connection(boost::asio::ip::tcp::socket && socket, connection_manager& manager, request_handler& handler)
			: m_socket(std::move(socket))
			, m_manager(manager)
			, m_handler(handler)
			, m_pos(0)
		{
		}

		struct log : public Log::basic_log<log>
		{
			static const Log::Module& module() { return Log::Module::HTTP; }
		};

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

		void connection::read_some_more()
		{
			auto self(shared_from_this());
			m_socket.async_read_some(boost::asio::buffer(m_buffer),
				[this, self](boost::system::error_code ec, std::size_t bytes_transferred)
			{
				if (!ec)
				{
					const char* data = m_buffer.data();
					auto end = data + bytes_transferred;
					auto ret = m_parser.parse(data, end);
					m_pos += bytes_transferred;

					if (ret == parser::finished)
					{
						auto request = m_parser.header();

						auto content_length = request.find_as<size_t>("content-length");

						request.remote_endpoint(m_socket.remote_endpoint());
						request.request_data(make_request_data(m_socket, content_length, data, end - data));

						m_handler.handle(request, m_response);
						send_reply();
					}
					else if (ret == parser::error)
					{
						log::error()
							<< "[CONNECTION] Parse error: " << buffer(m_buffer.data(), std::min(bytes_transferred, (size_t)100)) << " (starting at " << (m_pos - bytes_transferred) << " bytes)";
						m_handler.make_404(m_response);
						send_reply();
					}
					else
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

		void connection::send_reply()
		{
			auto self(shared_from_this());
			continue_sending(self, m_response.get_data(), boost::system::error_code(), 0);
		}
	}
}
