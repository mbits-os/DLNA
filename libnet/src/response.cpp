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
#include <response.hpp>

namespace net
{
	namespace http
	{
		static const char CRLF [] = "\r\n";

		std::vector<char> response::get_data()
		{
			if (m_content)
			{
				if (m_content->size_known())
					m_response.append("content-size")->out() << m_content->get_size();
				else
					m_response.append("transfer-encoding", "chunked");
			}
			std::ostringstream o;
			o << m_response;
			if (m_content)
			{
				bool chunked = !m_content->size_known();
				char buffer[8192];
				size_t chunk_size;
				do
				{
					chunk_size = m_content->read(buffer);
					if (chunked)
						o << std::hex << chunk_size << std::dec << CRLF;
					o.write(buffer, chunk_size);
					if (chunked)
						o << CRLF;
				} while (chunk_size > 0);
			}

			auto str = o.str();
			std::vector<char> out;
			out.reserve(str.length());
			for (auto && c : str)
				out.push_back(c);
			return out;
		}
	}
}
