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

#ifndef __HTTP_SERVER_HPP__
#define __HTTP_SERVER_HPP__

#include <http/http.hpp>
#include <http/connection.hpp>
#include <memory>
#include <http/request_handler.hpp>
#include <ssdp/device.hpp>

namespace net
{
	namespace http
	{
		struct server
		{
			server(boost::asio::io_service& service, const ssdp::device_ptr& device, net::ushort port);

			void start() { do_accept(); }
			void stop();

		private:
			ssdp::device_ptr m_device;
			request_handler m_handler;
			boost::asio::io_service& m_io_service;
			boost::asio::ip::tcp::acceptor m_acceptor;
			net::ushort m_port;

			boost::asio::ip::tcp::socket m_socket;
			http::connection_manager m_manager;

			void do_accept();
		};
	}
}

#endif // __HTTP_SERVER_HPP__
