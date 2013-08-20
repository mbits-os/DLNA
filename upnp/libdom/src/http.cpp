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

#include <http.hpp>
#include <utils.hpp>
#include <string.h>

#if defined WIN32
#define PLATFORM L"Win32"
#elif defined _MACOSX_
#define PLATFORM L"Mac OSX"
#elif defined ANDROID
#define PLATFORM L"Android"
#elif defined __GNUC__
#define PLATFORM "UNIX"
#endif

#include <string>
#include "curl_http.hpp"
#include <expat.hpp>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>


namespace http
{
	struct Encoding
	{
		const char* m_name;
		const int* m_table;
		size_t m_size;
		Encoding(const char* name, const int* table, size_t size)
			: m_name(name)
			, m_table(table)
			, m_size(size)
		{
		}
	};

	class EncodingDB
	{
		typedef std::vector<Encoding> Encodings;

		Encodings m_encodings;
		std::string m_dbPath;
		char* m_data;
		bool m_opened;

		struct File
		{
			FILE* m_ptr;
			size_t m_size;
			File(const std::string& path)
			{
				m_ptr = fopen(path.c_str(), "rb");

				m_size = 0;
				struct stat st;
				if (!stat(path.c_str(), &st))
					m_size = st.st_size;
			}
			~File()
			{
				if (*this)
					fclose(m_ptr);
			}

			operator bool () { return m_ptr != nullptr; }

			size_t read(void* dst, size_t bytes) { return fread(dst, 1, bytes, m_ptr); }
			template <typename T> bool read(T& t) { return read(&t, sizeof(T)) == sizeof(T); }

			size_t size() { return m_size; }
		};

		struct AutoCloser
		{
			char*& data;
			bool close;
			AutoCloser(char*& data): data(data), close(true) {}
			~AutoCloser()
			{
				if (close)
				{
					free(data);
					data = nullptr;
				}
			}
			void release() { close = false; }
		};

		struct FileHeader
		{
			unsigned int magic, count, strings;
		};

		struct TableHeader
		{
			unsigned int string, table, size;
		};

		void _close()
		{
			if (!m_opened)
				return;

			m_encodings.clear();

			free(m_data);
			m_data = nullptr;
		}

		bool _open()
		{
			if (m_opened)
				return true;

			File  file(m_dbPath);
			if (!file)
				return false;

			FileHeader header;
			if (!file.read(header)) return false;

			static const unsigned int MAGIC = 0x54455343;
			if (header.magic != MAGIC) return false;
			size_t stringsStart = sizeof(header) + sizeof(TableHeader)*header.count;
			size_t minimalOffset = stringsStart + header.strings;
			minimalOffset = ((minimalOffset + 3) >> 2) << 2;
			if (minimalOffset > file.size())
				return false;

			size_t buffer = file.size() - stringsStart;
			m_data = (char*)malloc(buffer);
			if (!m_data)
				return false;
			AutoCloser guard(m_data);

			m_encodings.reserve(header.count);
			for (unsigned int i = 0; i < header.count; ++i)
			{
				TableHeader head;
				if (!file.read(head)) return false;

				if (head.table < minimalOffset)
					return false;
				if (head.table + head.size > file.size())
					return false;
				if (head.string < stringsStart || head.string >= (stringsStart + header.strings))
					return false;

				const char* name = m_data + head.string - stringsStart;
				const int* table = (int*)(m_data + head.table - stringsStart);

				m_encodings.emplace_back(name, table, head.size);
			}

			if (file.read(m_data, buffer) != buffer)
				return false;

			guard.release();
			return m_opened = true;
		}

		static EncodingDB& get()
		{
			static EncodingDB db;
			return db;
		}

		const Encoding* _find(std::string enc)
		{
			std::tolower(enc);
			auto it = std::lower_bound(m_encodings.begin(), m_encodings.end(), enc,
				[](const Encoding& enc, const std::string& key)
				{
					// this supposed to be equivalent of "less(enc.m_name, key)" (notice reversed arguments)
					return key.compare(enc.m_name) > 0;
				});
			if (it == m_encodings.end() || enc.compare(it->m_name) != 0)
				return nullptr;
			return &*it;
		}

		EncodingDB()
			: m_data(nullptr)
			, m_opened(false)
		{
		}
	public:
		static void init(const char* path) { get().m_dbPath = path; close(); }
		static void close() { get()._close(); }
		static bool open() { return get()._open(); }
		static const Encoding* find(const std::string& enc) { return get()._find(enc); }
	};

	namespace { std::string charsetPath; }

	void init(const char* path)
	{
		EncodingDB::init(path);
	}

	bool loadCharset(const std::string& encoding, int (&table)[256])
	{
		if (!EncodingDB::open())
			return false;

		auto obj = EncodingDB::find(encoding);

		if (!obj)
			return false;

		if (obj->m_size < sizeof(table))
			return false;

		memcpy(table, obj->m_table, sizeof(table));
		return true;
	}

	namespace impl
	{
		struct ContentData
		{
			void* content;
			size_t content_length;
			ContentData()
				: content(nullptr)
				, content_length(0)
			{
			}
			~ContentData()
			{
				free(content);
			}

			bool append(const void* data, size_t len)
			{
				void* copy = nullptr;
				if (data && len)
				{
					void* copy = realloc(content, content_length + len);
					if (!copy) return false;
					content = copy;
					memcpy((char*)content + content_length, data, len);
					content_length += len;
				}
				return true;
			}

			void clear()
			{
				free(content);
				content = nullptr;
				content_length = 0;
			}
		};

		class XmlHttpRequest
			: public http::XmlHttpRequest
			, public http::HttpCallback
		{
			std::weak_ptr<HttpCallback> m_self;

			ONREADYSTATECHANGE handler;
			void* handler_data;

			HTTP_METHOD http_method;
			std::string url;
			bool async;
			Headers request_headers;
			READY_STATE ready_state;
			ContentData body;

			int http_status;
			std::string reason;
			Headers response_headers;
			ContentData response;
			dom::XmlDocumentPtr doc;

			bool send_flag, done_flag, debug;
			ConnectionCallback* m_connection;

			bool m_followRedirects;
			size_t m_redirects;

			bool m_wasRedirected;
			std::string m_finalLocation;

			void onReadyStateChange()
			{
				if (handler)
					handler(this, handler_data);
			}
			bool rebuildDOM(const std::string& mimeType, const std::string& encoding);
			bool rebuildDOM(const char* forcedType = nullptr);
			bool parseXML(const std::string& encoding);

			void clear_response()
			{
				http_status = 0;
				reason.clear();
				response_headers.clear();
				response.clear();
			}
		public:

			XmlHttpRequest()
				: handler(nullptr)
				, handler_data(nullptr)
				, http_method(HTTP_GET)
				, async(true)
				, ready_state(UNSENT)
				, http_status(0)
				, send_flag(false)
				, done_flag(false)
				, debug(false)
				, m_connection(nullptr)
				, m_followRedirects(true)
				, m_redirects(10)
				, m_wasRedirected(false)
			{
			}

			~XmlHttpRequest()
			{
			}

			void setSelfRef(HttpCallbackPtr self) { m_self = self; }
			void onreadystatechange(ONREADYSTATECHANGE handler, void* userdata);
			READY_STATE getReadyState() const;

			void open(HTTP_METHOD method, const std::string& url, bool async = true);
			void setRequestHeader(const std::string& header, const std::string& value);

			void setBody(const void* body, size_t length);
			void send();
			void abort();

			int getStatus() const;
			std::string getStatusText() const;
			std::string getResponseHeader(const std::string& name) const;
			std::map<std::string, std::string> getResponseHeaders() const;
			size_t getResponseTextLength() const;
			const char* getResponseText() const;
			dom::XmlDocumentPtr getResponseXml();

			bool wasRedirected() const;
			const std::string getFinalLocation() const;

			void setDebug(bool debug);
			void setMaxRedirects(size_t redirects);
			void setShouldFollowLocation(bool follow);

			void onStart(ConnectionCallback* cb);
			void onError();
			void onFinish();
			size_t onData(const void* data, size_t count);
			void onFinalLocation(const std::string& location);
			void onHeaders(const std::string& reason, int http_status, const Headers& headers);

			void appendHeaders();
			std::string getUrl();
			void* getContent(size_t& length);
			bool getDebug();
			bool shouldFollowLocation();
			long getMaxRedirs();
		};

		void XmlHttpRequest::onreadystatechange(ONREADYSTATECHANGE handler, void* userdata)
		{
			//Synchronize on(*this);
			this->handler = handler;
			this->handler_data = userdata;
			if (ready_state != UNSENT)
				onReadyStateChange();
		}

		http::XmlHttpRequest::READY_STATE XmlHttpRequest::getReadyState() const
		{
			return ready_state;
		}

		void XmlHttpRequest::open(HTTP_METHOD method, const std::string& url, bool async)
		{
			abort();

			//Synchronize on(*this);

			send_flag = false;
			clear_response();
			request_headers.empty();

			http_method = method;
			this->url = url;
			this->async = async;

			ready_state = OPENED;
			onReadyStateChange();
		}

		void XmlHttpRequest::setRequestHeader(const std::string& header, const std::string& value)
		{
			//Synchronize on(*this);

			if (ready_state != OPENED || send_flag) return;

			auto _h = std::tolower(header);
			Headers::iterator _it = request_headers.find(_h);
			if (_it == request_headers.end())
			{
				if (_h == "accept-charset") return;
				if (_h == "accept-encoding") return;
				if (_h == "connection") return;
				if (_h == "content-length") return;
				if (_h == "cookie") return;
				if (_h == "cookie2") return;
				if (_h == "content-transfer-encoding") return;
				if (_h == "date") return;
				if (_h == "expect") return;
				if (_h == "host") return;
				if (_h == "keep-alive") return;
				if (_h == "referer") return;
				if (_h == "te") return;
				if (_h == "trailer") return;
				if (_h == "transfer-encoding") return;
				if (_h == "upgrade") return;
				if (_h == "user-agent") return;
				if (_h == "via") return;
				if (_h.substr(0, 6) == "proxy-") return;
				if (_h.substr(0, 4) == "sec-") return;
				request_headers[_h] = header +": " + value;
			}
			else
			{
				_it->second += ", " + value;
			}
		}

		void XmlHttpRequest::setBody(const void* body, size_t length)
		{
			//Synchronize on(*this);

			if (ready_state != OPENED || send_flag) return;

			this->body.clear();

			if (http_method != HTTP_POST) return;

			this->body.append(body, length);
		}

		void XmlHttpRequest::send()
		{
			abort();

			//Synchronize on(*this);

			if (ready_state != OPENED || send_flag) return;

			send_flag = true;
			done_flag = false;
			http::Send(m_self.lock(), async);
		}

		void XmlHttpRequest::abort()
		{
			if (m_connection)
				m_connection->abort();
			m_connection = nullptr;
		}

		int XmlHttpRequest::getStatus() const
		{
			return http_status;
		}

		std::string XmlHttpRequest::getStatusText() const
		{
			return reason;
		}

		std::string XmlHttpRequest::getResponseHeader(const std::string& name) const
		{
			Headers::const_iterator _it = response_headers.find(std::tolower(name));
			if (_it == response_headers.end()) return std::string();
			return _it->second;
		}

		std::map<std::string, std::string> XmlHttpRequest::getResponseHeaders() const
		{
			return response_headers;
		}

		size_t XmlHttpRequest::getResponseTextLength() const
		{
			return response.content_length;
		}

		const char* XmlHttpRequest::getResponseText() const
		{
			return (const char*)response.content;
		}

		dom::XmlDocumentPtr XmlHttpRequest::getResponseXml()
		{
			// Synchronize on (*this);

			if (ready_state != DONE || done_flag) return false;

			if (!doc && response.content && response.content_length)
				if (!rebuildDOM("text/xml"))
					return dom::XmlDocumentPtr();

			return doc; 
		}

		bool XmlHttpRequest::wasRedirected() const { return m_wasRedirected; }
		const std::string XmlHttpRequest::getFinalLocation() const { return m_finalLocation; }

		void XmlHttpRequest::setDebug(bool debug)
		{
			this->debug = debug;
		}

		void XmlHttpRequest::setMaxRedirects(size_t redirects)
		{
			m_redirects = redirects;
		}

		void XmlHttpRequest::setShouldFollowLocation(bool follow)
		{
			m_followRedirects = follow;
		}

		void XmlHttpRequest::onStart(ConnectionCallback* connection)
		{
			m_connection = connection;

			if (!body.content || !body.content_length)
				http_method = HTTP_GET;

			//onReadyStateChange();
		}

		void XmlHttpRequest::onError()
		{
			ready_state = DONE;
			done_flag = true;
			onReadyStateChange();
		}

		void XmlHttpRequest::onFinish()
		{
			ready_state = DONE;
			onReadyStateChange();
		}

		size_t XmlHttpRequest::onData(const void* data, size_t count)
		{
			//Synchronize on(*this);

			size_t ret = response.append(data, count) ? count : 0;
			if (ret)
			{
				if (ready_state == HEADERS_RECEIVED) ready_state = LOADING;
				if (ready_state == LOADING)          onReadyStateChange();
			}
			return ret;
		}

		void XmlHttpRequest::onFinalLocation(const std::string& location)
		{
			m_wasRedirected = true;
			m_finalLocation = location;
		}

		void XmlHttpRequest::onHeaders(const std::string& reason, int http_status, const Headers& headers)
		{
			//Synchronize on(*this);
			this->http_status = http_status;
			this->reason = reason;
			this->response_headers = headers;

			ready_state = HEADERS_RECEIVED;
			onReadyStateChange();
		}

		void XmlHttpRequest::appendHeaders()
		{
			if (!m_connection)
				return;
			//Synchronize on(*this);
			for (Headers::const_iterator _cur = request_headers.begin(); _cur != request_headers.end(); ++_cur)
				m_connection->appendHeader(_cur->second);
		}

		std::string XmlHttpRequest::getUrl() { return url; }
		void* XmlHttpRequest::getContent(size_t& length) { if (http_method == HTTP_POST) { length = body.content_length; return body.content; } return nullptr; }
		bool XmlHttpRequest::getDebug() { return debug; }
		bool XmlHttpRequest::shouldFollowLocation() { return m_followRedirects; }
		long XmlHttpRequest::getMaxRedirs() { return m_redirects; }

		static void getMimeAndEncoding(std::string ct, std::string& mime, std::string& enc)
		{
			mime.empty();
			enc.empty();
			size_t pos = ct.find_first_of(';');
			mime = std::tolower(ct.substr(0, pos));
			if (pos == std::string::npos) return;
			ct = ct.substr(pos+1);

			while(1)
			{
				pos = ct.find_first_of('=');
				if (pos == std::string::npos) return;
				std::string cand = ct.substr(0, pos);
				ct = ct.substr(pos+1);
				size_t low = 0, hi = cand.length();
				while (cand[low] && isspace((unsigned char)cand[low])) ++low;
				while (hi > low && isspace((unsigned char)cand[hi-1])) --hi;

				if (std::tolower(cand.substr(low, hi - low)) == "charset")
				{
					pos = ct.find_first_of(';');
					enc = ct.substr(0, pos);
					low = 0; hi = enc.length();
					while (enc[low] && isspace((unsigned char)enc[low])) ++low;
					while (hi > low && isspace((unsigned char)enc[hi-1])) --hi;
					enc = enc.substr(low, hi-low);
					return;
				}
				pos = ct.find_first_of(';');
				if (pos == std::string::npos) return;
				ct = ct.substr(pos+1);
			};
		}

		bool XmlHttpRequest::rebuildDOM(const std::string& mime, const std::string& encoding)
		{
			if (mime == "" ||
				mime == "text/xml" ||
				mime == "application/xml" || 
				(mime.length() > 4 && mime.substr(mime.length()-4) == "+xml"))
			{
				return parseXML(encoding);
			}

			return true;
		}

		bool XmlHttpRequest::rebuildDOM(const char* forcedType)
		{
			std::string mime;
			std::string enc;
			getMimeAndEncoding(getResponseHeader("Content-Type"), mime, enc);
			if (forcedType)
				return rebuildDOM(forcedType, enc);
			return rebuildDOM(mime, enc);
		}

		class XHRParser: public xml::ExpatBase<XHRParser>
		{
			dom::XmlElementPtr elem;
			std::string text;

			void addText()
			{
				if (text.empty()) return;
				if (elem)
					elem->appendChild(doc->createTextNode(text));
				text.clear();
			}
		public:

			dom::XmlDocumentPtr doc;

			bool create(const char* cp)
			{
				doc = dom::XmlDocument::create();
				if (!doc) return false;
				return xml::ExpatBase<XHRParser>::create(cp);
			}

			bool onUnknownEncoding(const XML_Char* name, XML_Encoding* info)
			{
				info->data = nullptr;
				info->convert = nullptr;
				info->release = nullptr;

				if (!loadCharset(name, info->map))
				{
					printf("Unknown encoding: %s\n", name);
					return false;
				}

				return true;
			}

			void onStartElement(const XML_Char *name, const XML_Char **attrs)
			{
				addText();
				auto current = doc->createElement(name);
				if (!current) return;
				for (; *attrs; attrs += 2)
				{
					auto attr = doc->createAttribute(attrs[0], attrs[1]);
					if (!attr) continue;
					current->setAttribute(attr);
				}
				if (elem)
					elem->appendChild(current);
				else
					doc->setDocumentElement(current);
				elem = current;
			}

			void onEndElement(const XML_Char *name)
			{
				addText();
				if (!elem) return;
				dom::XmlNodePtr node = elem->parentNode();
				elem = std::static_pointer_cast<dom::XmlElement>(node);
			}

			void onCharacterData(const XML_Char *pszData, int nLength)
			{
				text += std::string(pszData, nLength);
			}
		};

		bool XmlHttpRequest::parseXML(const std::string& encoding)
		{
			const char* cp = NULL;
			if (!encoding.empty()) cp = encoding.c_str();
			XHRParser parser;
			if (!parser.create(cp)) return false;
			parser.enableElementHandler();
			parser.enableCharacterDataHandler();
			parser.enableUnknownEncodingHandler();
			if (!parser.parse((const char*)response.content, response.content_length))
				return false;
			doc = parser.doc;
			return true;
		} 
	} // http::impl

	XmlHttpRequestPtr XmlHttpRequest::Create()
	{
		try {
		auto xhr = std::make_shared<impl::XmlHttpRequest>();
		HttpCallbackPtr self(xhr);
		xhr->setSelfRef(self);
		return xhr;
		} catch(std::bad_alloc) { return nullptr; }
	}
}
