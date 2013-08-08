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
#include <http_connection.hpp>
#include <types.hpp>
#include <iostream>

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

					if (ret == parser::finished)
					{
						auto header = m_parser.header();
						if (header.m_method != "POST")
						{
							auto ua = header.find("user-agent");
							std::ostringstream o;
							o << header.m_method << " ";
							if (header.m_resource != "*")
							{
								auto it = header.find("host");
								if (it != header.end())
									o << it->value();
							}
							o << header.m_resource << " " << header.m_protocol;
							o << " [ " << to_string(m_socket.remote_endpoint().address()) << ":" << m_socket.remote_endpoint().port() << " ]";
							if (ua != header.end())
							{
								o << " [ " << ua->value();
								auto pui = header.find("x-av-physical-unit-info");
								auto ci = header.find("x-av-client-info");
								if (pui != header.end() || ci != header.end())
								{
									o << " | ";
									if (pui != header.end())
									{
										o << pui->value();
										if (ci != header.end())
											o << " | ";
									}
									if (ci != header.end())
									{
										o << ci->value();
									}
								}
								o << " ]";
							}
							o << "\n";
							std::cout << o.str();
						}
						else
						{
							std::cout << header;

							auto len_it = header.find("content-length");
							if (len_it != header.end())
							{
								size_t len;
								std::istringstream in(len_it->value());
								in >> len;

								auto seen = end - data;
								auto rest = len - seen;
								std::cout << len << " bytes, " << seen << " seen, " << rest << " to go\n";
								std::cout.write(data, seen);

								while (rest)
								{
									auto chunk = m_buffer.size();
									if (chunk > rest)
										chunk = rest;
									rest -= chunk;
									boost::system::error_code sub_ec;
									auto read = m_socket.read_some(boost::asio::buffer(m_buffer, chunk), sub_ec);
									if (sub_ec)
									{
										m_manager.stop(shared_from_this());
										return;
									}
									std::cout.write(m_buffer.data(), read);
								}
								std::cout << "\n";
							}
						}
						m_handler.handle(header, m_response);
						send_reply();
					}
					else if (ret == parser::error)
					{
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
