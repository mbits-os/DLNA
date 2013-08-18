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
#include <network/types.hpp>

namespace net
{
	std::string xmlencode(const std::string& in)
	{
		auto len = in.length();
		for (auto && c : in)
		{
			switch (c)
			{
			case '<':
			case '>':
				len += 3;
				break;
			case '&':
				len += 4;
				break;
			case '"':
				len += 5;
				break;
			}
		}

		if (len == in.length())
			return in;

		std::string out;
		out.reserve(len);

		for (auto && c : in)
		{
			switch (c)
			{
			case '<': out.append("&lt;"); break;
			case '>': out.append("&gt;"); break;
			case '&': out.append("&amp;"); break;
			case '"': out.append("&quot;"); break;
			default:  out.push_back(c);
			}
		}

		return out;
	}
}
