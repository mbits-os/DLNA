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
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <limits>

namespace fs = boost::filesystem;

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
			virtual std::size_t read(void* buffer, std::size_t size) = 0;
			template <std::size_t size>
			std::size_t read(char (&buffer)[size]) { return read(buffer, size); }

			inline static content_ptr from_string(const std::string& text);
			inline static content_ptr from_file(const fs::path& path);
		};

		class string_content : public content
		{
			std::string m_text;
			std::size_t m_pointer;
		public:
			string_content(const std::string& text) : m_text(text), m_pointer(0) {}
			bool size_known() override { return true; }
			std::size_t get_size() override { return m_text.size(); }
			std::size_t read(void* buffer, std::size_t size) override
			{
				auto rest = m_text.size() - m_pointer;
				if (size > rest)
					size = rest;
				memcpy(buffer, m_text.c_str() + m_pointer, size);
				m_pointer += size;
				return size;
			}
		};

		class file_content : public content
		{
			std::size_t m_size;
			fs::ifstream m_stream;
		public:
			file_content(const fs::path& path)
				: m_size(0)
				, m_stream(path, std::ios::binary | std::ios::in)
			{
				auto size = fs::file_size(path);

				// clip instead of overflow
				decltype(size) max = std::numeric_limits<std::size_t>::max();
				if (size > max) size = max;
				m_size = (std::size_t)size;
			}
			bool size_known() override { return true; }
			std::size_t get_size() override { return m_size; }
			std::size_t read(void* buffer, std::size_t size) override
			{
				return (std::size_t)m_stream.read((char*) buffer, size).gcount();
			}
		};

		inline content_ptr content::from_string(const std::string& text)
		{
			return std::make_shared<string_content>(text);
		}
		inline content_ptr content::from_file(const fs::path& path)
		{
			return std::make_shared<file_content>(path);
		}

		class response;

		class response_buffer
		{
			enum status
			{
				header,
				chunks,
				last_chunk,
				invalid
			};
			response& m_data;
			bool m_chunked;
			status m_status;

			response_buffer& operator = (response_buffer && rhs);
			response_buffer& operator = (const response_buffer & rhs);

		public:
			explicit response_buffer(response& data);

			bool advance(std::vector<char>& buffer);
		};

		class response : boost::noncopyable
		{
			http_response m_response;
			content_ptr m_content;
		public:
			http_response& header() { return m_response; }
			content_ptr content() { return m_content; }
			void content(content_ptr c) { m_content = c; }
			response_buffer get_data();
		};
	}
}

#endif // __RESPONSE_HPP__
