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

#ifndef __HTTP_PARSER_HPP__
#define __HTTP_PARSER_HPP__

#include <memory>
#include <set>
#include <mime.hpp>
#include <http.hpp>

namespace net
{
	namespace http
	{
		enum class parser
		{
			finished,
			pending,
			error
		};

		class header_parser_base
		{
		protected:
			enum state
			{
				before_first_line,
				in_req_method,
				before_req_resource,
				in_req_resource,
				before_req_protocol,
				in_req_protocol,
				in_resp_protocol,
				in_resp_status,
				in_resp_message,
				in_first_line_end,
				line_start,
				before_continued_line,
				in_continued_line,
				in_name,
				before_colon,
				at_colon,
				before_value,
				in_value,
				in_CRLF
			};

			header_parser_base();
			parser parse_first_line(http_request_line& line, const char*& data, const char* end);
			parser parse_first_line(http_response_line& line, const char*& data, const char* end);
			parser parse_header(mime::headers& headers, const char*& data, const char* end);

			state get_state() const { return m_state; }
		private:
			state m_state;
			std::string m_protocol;
			std::string m_name;
			std::string m_value;

			http::protocol parse_protocol();

			bool skip_ws(state expected, state next, const char*& data, const char* end, std::string& dst);
			parser get_token(state expected, state next, const char*& data, const char* end, std::string& dst);
			parser get_token(state next, const char*& data, const char* end, std::string& dst, char delim = ' ');
		};

		template <typename Header>
		class header_parser : public header_parser_base
		{
			Header m_header;
		public:
			parser parse(const char*& data, const char* end)
			{
				parser ret = parser::pending;

				if (get_state() < line_start)
					ret = parse_first_line(m_header, data, end);

				while (ret == parser::pending && data != end)
					ret = parse_header(m_header, data, end);

				return ret;
			}
			Header& header() { return m_header; }
		};
	}
}

#endif // __HTTP_PARSER_HPP__
