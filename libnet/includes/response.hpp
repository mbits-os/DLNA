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

#ifndef __RESPONSE_HPP__
#define __RESPONSE_HPP__

#include <http.hpp>
#include <boost/utility.hpp>

namespace net
{
	namespace http
	{
		class content;
		typedef std::shared_ptr<content> content_ptr;

		class content : boost::noncopyable
		{
		public:
			virtual ~content() {}

			virtual bool size_known() = 0;
			virtual std::size_t get_size() = 0;
			virtual const char* data() = 0;

			inline static content_ptr from_string(const std::string& text);
		};

		class string_content : public content
		{
			std::string m_text;
		public:
			string_content(const std::string& text): m_text(text) {}
			bool size_known() override { return true; }
			std::size_t get_size() override { return m_text.size(); }
			const char* data() override { return m_text.c_str(); }
		};

		inline content_ptr content::from_string(const std::string& text)
		{
			return std::make_shared<string_content>(text);
		}

		class response : boost::noncopyable
		{
			http_response m_response;
			content_ptr m_content;
		public:
			http_response& header() { return m_response; }
			content_ptr content() { return m_content; }
			void content(content_ptr c) { m_content = c; }
			std::vector<char> get_data();
		};
	}
}

#endif // __RESPONSE_HPP__
