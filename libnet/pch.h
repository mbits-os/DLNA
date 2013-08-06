/*
 * Copyright (C) 2013 Marcin Zdun
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

#ifndef __LIBSTUDIO_PCH_H__
#define __LIBSTUDIO_PCH_H__

#include <memory>
#include <list>
#include <map>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>

#include <stdlib.h>
#ifdef _WIN32
#include <sdkddkver.h>
#include <process.h>
#else
#include <unistd.h>
#define _getpid getpid
#endif

namespace std
{
	inline std::ostream& operator << (std::ostream& o, const std::string& str)
	{
		return o << str.c_str();
	}
}

#endif //__LIBSTUDIO_PCH_H__