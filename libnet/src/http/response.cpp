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
#include <http/response.hpp>

namespace net
{
	namespace http
	{
		static const char CRLF [] = "\r\n";

		response_buffer::response_buffer(response& data)
			: m_data(data)
			, m_chunked(!(data.content() ? data.content()->size_known() : true))
			, m_status(header)
		{
		}

		bool response_buffer::advance(std::vector<char>& out_buffer)
		{
			if (m_status == header)
			{
				std::ostringstream o;
				o << m_data.header();
				auto str = o.str();
				out_buffer.assign(str.begin(), str.end());
				m_status = chunks;
				return m_data.content() != nullptr;
			}

			if (m_status == chunks)
			{
				char buffer[8192];
				if (m_chunked)
				{
					std::ostringstream o;
					auto chunk_size = m_data.content()->read(buffer);
					o << std::hex << chunk_size << std::dec << CRLF;
					o.write(buffer, chunk_size);
					o << CRLF;
					auto str = o.str();
					out_buffer.assign(str.begin(), str.end());

					if (chunk_size == 0)
						m_status = last_chunk;
				}
				else
				{
					auto to_read = sizeof(buffer);
					if (m_data.m_calculated_length < to_read)
						to_read = m_data.m_calculated_length;

					auto chunk_size = m_data.content()->read(buffer, to_read);

					m_data.m_calculated_length -= chunk_size;

					out_buffer.assign(buffer, chunk_size + buffer);
					if (chunk_size == 0)
						m_status = invalid;
				}
				return m_status != invalid;
			}

			if (m_status == last_chunk)
				m_status = invalid;

			return false;
		}

		void response::complete_header()
		{
			if (!m_completed && m_content)
			{
				m_completed = true;

				if (m_content->size_known())
				{
					auto size = m_content->get_size();
					if (has_range() && m_content->can_skip())
					{
						auto copy = m_range;
						if (copy.second < 0) // 500- means all but first 500 bytes
						{
							copy.second = size - 1;
						}
						else if (copy.first < 0) // -500 means last 500 bytes
						{
							copy.first = size - copy.second;
							copy.second = size - 1;
						}

						if (copy.first >= size || copy.second >= size)
						{
							m_response.m_status = 416; // Requested range not satisfiable
							m_content = nullptr;
							return;
						}

						m_response.m_status = 206; // Partial Content
						m_response.append("content-range")->out() << copy.first << "-" << copy.second << "/" << size;
						size = (size_t)(copy.second - copy.first + 1);

						m_content->skip((size_t)copy.first);
					}
					m_calculated_length = size;
					m_response.append("content-size")->out() << size;
				}
				else
					m_response.append("transfer-encoding")->out() << "chunked";
			}
		}

		response_buffer response::get_data()
		{
			complete_header();
			return response_buffer(*this);
		}
	}
}
