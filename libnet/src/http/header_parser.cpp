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
#include <http/header_parser.hpp>
#include <cctype>

namespace net
{
	namespace http
	{
		enum
		{
			CR = '\r',
			LF = '\n',
			SP = ' ',
			NAME_LEN = 4 + 1,          // "HTTP/"
			LENGTH = NAME_LEN + 3,     // "HTTP/1.1" or "HTTP/1.0"
			STATIC_PART = NAME_LEN + 2 // "HTTP/1."
		};

		header_parser_base::header_parser_base()
			: m_state(before_first_line)
		{
		}

#define SWS while (data < end && *data == ' ') { ++data; }
#define SNWS while (data < end && !std::isspace((unsigned int)*data)) { ++data; }
#define SNWS2(c) while (data < end && !std::isspace((unsigned int)*data) && (*data != (c))) { ++data; }
#define S2CRLF while (data < end && *data != CR) { ++data; }

		bool header_parser_base::skip_ws(state expected, state next, const char*& data, const char* end, std::string& dst)
		{
			if (m_state == expected)
			{
				SWS;
				if (data == end)
					return true;
				m_state = next;
				dst.clear();
			}
			return false;
		}

		parser header_parser_base::get_token(state expected, state next, const char*& data, const char* end, std::string& dst)
		{
			auto ret = parser::finished;

			if (m_state == expected)
				ret = get_token(next, data, end, dst);

			return ret;
		}

		parser header_parser_base::get_token(state next, const char*& data, const char* end, std::string& dst, char delim)
		{
			auto ptr = data;

			SNWS;
			dst.append(ptr, data);

			if (data == end)
				return parser::pending;
			if (*data != delim)
				return parser::error;

			m_state = next;

			return parser::finished;
		}

		parser header_parser_base::parse_first_line(http_request_line& line, const char*& data, const char* end)
		{
			// MEHOD RESOURCE HTTP/MAJ.MIN
			if (m_state == before_first_line)
			{
				line.m_method.clear();
				m_state = in_req_method;
			}

			auto ret = get_token(in_req_method, before_req_resource, data, end, line.m_method);
			if (ret != parser::finished)
				return ret;

			if (skip_ws(before_req_resource, in_req_resource, data, end, line.m_resource))
				return parser::pending;

			ret = get_token(in_req_resource, before_req_protocol, data, end, line.m_resource);
			if (ret != parser::finished)
				return ret;

			if (skip_ws(before_req_protocol, in_req_protocol, data, end, m_protocol))
				return parser::pending;

			if (m_state == in_req_protocol)
			{
				ret = get_token(in_first_line_end, data, end, m_protocol, CR);
				if (ret != parser::finished)
					return ret;

				line.m_protocol = parse_protocol();
				if (line.m_protocol == http::undeclared)
					return parser::error;

				++data;
				if (data == end)
					return parser::pending;
			}

			if (m_state == in_first_line_end)
			{
				if (data == end)
					return parser::pending;
				if (*data != LF)
					return parser::error;

				m_state = line_start;
				m_name.clear();

				++data;
				if (data == end)
					return parser::pending;
			}

			return parser::pending;
		}

		parser header_parser_base::parse_first_line(http_response_line& line, const char*& data, const char* end)
		{
			// HTTP/MAJ.MIN STATUS MESSAGE[ignored] 
			if (m_state == before_first_line)
			{
				m_protocol.clear();
				m_state = in_resp_protocol;
			}

			parser ret;
			if (m_state == in_resp_protocol)
			{
				ret = get_token(before_resp_status, data, end, m_protocol);
				if (ret != parser::finished)
					return ret;

				line.m_protocol = parse_protocol();
				if (line.m_protocol == http::undeclared)
					return parser::error;
			}

			if (skip_ws(before_resp_status, in_resp_status, data, end, m_protocol))
				return parser::pending;

			if (m_state == in_resp_status)
			{
				ret = get_token(in_resp_message, data, end, m_protocol);
				if (ret != parser::finished)
					return ret;

				std::istringstream i(m_protocol);
				i >> line.m_status;
				std::ostringstream o;
				o << (int)line.m_status;

				if (m_protocol != o.str())
					return parser::error;
			}

			if (m_state == in_resp_message)
			{
				while (data != end && *data != CR) ++data;
				if (data == end)
					return parser::pending;

				m_state = in_first_line_end;

				++data;
				if (data == end)
					return parser::pending;
			}

			if (m_state == in_first_line_end)
			{
				if (data == end)
					return parser::pending;
				if (*data != LF)
					return parser::error;

				m_state = line_start;
				m_name.clear();

				++data;
				if (data == end)
					return parser::pending;
			}

			return parser::pending;
		}

		parser header_parser_base::parse_header(mime::headers& headers, const char*& data, const char* end)
		{
			if (m_state == line_start)
			{
				if (*data == CR)
				{
					m_name.clear();
					m_value.clear();

					m_state = in_CRLF;
					++data;
					if (data == end)
						return parser::pending;
				}
				else if (*data == SP)
				{
					m_state = before_continued_line;
				}
				else
				{
					m_name.clear();
					m_state = in_name;
				}
			}

			if (m_state == in_name)
			{
				auto ptr = data;

				SNWS2(':');
				m_name.append(ptr, data);

				if (data == end)
					return parser::pending;
				if (*data == SP)
				{
					m_state = before_colon;
				}
				else if (*data == ':')
				{
					m_state = before_value;
					++data;
					if (data == end)
						return parser::pending;
				}
				else
					return parser::error;
			}

			std::string dummy;
			if (skip_ws(before_colon, at_colon, data, end, dummy))
				return parser::pending;

			if (m_state == at_colon)
			{
				if (*data != ':')
					return parser::error;

				m_state = before_value;
				++data;
				if (data == end)
					return parser::pending;
			}

			if (skip_ws(before_value, in_value, data, end, m_value))
				return parser::pending;

			if (skip_ws(before_continued_line, in_continued_line, data, end, m_value))
				return parser::pending;

			if (m_state == in_value || m_state == in_continued_line)
			{
				auto ptr = data;
				S2CRLF;
				m_value.append(ptr, data);
				if (data == end)
					return parser::pending;

				auto last = m_value.rbegin();
				while (last != m_value.rend() && std::isspace((unsigned char) *last))
					++last;

				std::string tmp{ m_value.begin(), last.base() };
				m_value = std::move(tmp);

				auto pos = headers.find(m_name);
				if (pos != headers.end())
					pos->value().append(" ").append(m_value);
				else
					headers.append(m_name, m_value);

				m_state = in_CRLF;
				++data;
				if (data == end)
					return parser::pending;
			}

			if (m_state == in_CRLF)
			{
				if (data == end)
					return parser::pending;
				if (*data != LF)
					return parser::error;

				++data;

				if (m_name.empty() && m_value.empty())
					return parser::finished;

				m_state = line_start;

				if (data == end)
					return parser::pending;
			}

			return parser::pending;
		}

		http::protocol header_parser_base::parse_protocol()
		{
			if (m_protocol.length() == LENGTH &&
				m_protocol.substr(0, STATIC_PART) == "HTTP/1.")
			{
				switch (m_protocol[LENGTH - 1])
				{
				case '0': return http::http_1_0;
				case '1': return http::http_1_1;
				};
			}
			return http::undeclared;
		}
	}
}
