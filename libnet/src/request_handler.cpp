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
#include <request_handler.hpp>
#include <response.hpp>
#include <regex>
#include <interface.hpp>
#include <expat.hpp>
#include <dom.hpp>

namespace net
{
	namespace http
	{
		namespace device
		{
			const char xml [] = R"(<?xml version="1.0" encoding="utf-8"?>
<root xmlns:dlna="urn:schemas-dlna-org:device-1-0" xmlns="urn:schemas-upnp-org:device-1-0">
	<specVersion>
		<major>1</major>
		<minor>0</minor>
	</specVersion>
	<URLBase>http://$host:$port/</URLBase>
	<device>
		<dlna:X_DLNADOC xmlns:dlna="urn:schemas-dlna-org:device-1-0">DMS-1.50</dlna:X_DLNADOC>
		<dlna:X_DLNADOC xmlns:dlna="urn:schemas-dlna-org:device-1-0">M-DMS-1.50</dlna:X_DLNADOC>
		<deviceType>urn:schemas-upnp-org:device:MediaServer:1</deviceType>
		<friendlyName>LAN Radio</friendlyName>
        <manufacturer>midnightBITS</manufacturer>
		<manufacturerURL>http://www.midnightbits.com</manufacturerURL>
		<modelDescription>UPnP/AV 1.0 Compliant Media Server</modelDescription>
		<modelName>lanRadio</modelName>
		<modelNumber>01</modelNumber>
		<modelURL>http://www.midnightbits.org/lanRadio</modelURL>
		<serialNumber/>
        <UPC/>
        <UDN>$uuid</UDN>
        <iconList>
			<icon>
				<mimetype>image/png</mimetype>
				<width>256</width>
				<height>256</height>
				<depth>24</depth>
				<url>/images/icon-256.png</url>
			</icon>
		</iconList>
		<presentationURL>http://$host:$port/console/index.html</presentationURL> 
		<serviceList>
			<service>
				<serviceType>urn:schemas-upnp-org:service:ContentDirectory:1</serviceType>
				<serviceId>urn:upnp-org:serviceId:ContentDirectory</serviceId>
				<SCPDURL>/config/directory.xml</SCPDURL>
				<controlURL>/upnp/control/content_directory</controlURL>
				<eventSubURL>/upnp/event/content_directory</eventSubURL>
			</service>
			<service>
				<serviceType>urn:schemas-upnp-org:service:ConnectionManager:1</serviceType>
				<serviceId>urn:upnp-org:serviceId:ConnectionManager</serviceId>
				<SCPDURL>/config/manager.xml</SCPDURL>
				<controlURL>/upnp/control/connection_manager</controlURL>
				<eventSubURL>/upnp/event/connection_manager</eventSubURL>
			</service>
		</serviceList>
	</device>
</root>
)";
		}

		class DOMParser : public xml::ExpatBase<DOMParser>
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
				return xml::ExpatBase<DOMParser>::create(cp);
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

		dom::XmlDocumentPtr create_from_socket(request_data_ptr data)
		{
			if (!data || !data->content_length())
				return nullptr;

			DOMParser parser;
			if (!parser.create(nullptr)) return nullptr;

			parser.enableElementHandler();
			parser.enableCharacterDataHandler();

			auto rest = data->content_length();
			char buffer[8192];
			while (rest)
			{
				auto chunk = sizeof(buffer);
				if (chunk > rest)
					chunk = rest;
				rest -= chunk;
				auto read = data->read(buffer, chunk);

				if (!parser.parse(buffer, read, false))
					return nullptr;
			}

			if (!parser.parse(buffer, 0))
				return nullptr;

			return parser.doc;
		}

		struct tmplt_chunk
		{
			template_vars::const_iterator m_var;
			const char* m_start;
			size_t m_size;
			tmplt_chunk(template_vars::const_iterator var, const char* start, size_t size)
				: m_var(var)
				, m_start(start)
				, m_size(size)
			{}
		};

		class template_content : public content
		{
			const char* m_tmplt;
			const template_vars& m_vars;
			std::vector<tmplt_chunk> m_chunks;
			std::vector<tmplt_chunk>::const_iterator m_cur;
			size_t m_ptr;
			template_vars::const_iterator find(const std::string& key)
			{
				auto cur = m_vars.begin(), end = m_vars.end();
				for (; cur != end; ++cur)
					if (cur->first == key)
						return cur;
				return cur;
			}
		public:
			template_content(const char* tmplt, const template_vars& vars)
				: m_tmplt(tmplt)
				, m_vars(vars)
			{
				while (*tmplt)
				{
					auto start = tmplt;
					while (*tmplt && *tmplt != '$') ++tmplt;

					auto var = *tmplt ? tmplt + 1 : tmplt;
					auto var_end = var;
					while (*var_end && std::isalpha((unsigned char) *var_end)) ++var_end;

					auto _var = find({var, var_end});

					m_chunks.emplace_back(_var, start, tmplt - start);

					tmplt = var_end;
				}

				m_cur = m_chunks.begin();
				m_ptr = 0;
			}

			bool size_known() override { return false; }
			std::size_t get_size() override { return 0; }
			std::size_t read(void* buffer, std::size_t size)
			{
				if (m_cur == m_chunks.end())
					return 0;

				size_t _read = 0;
				while (size > 0)
				{
					if (m_ptr < m_cur->m_size)
					{
						auto rest = m_cur->m_size - m_ptr;
						if (rest > size)
							rest = size;

						memcpy((char*) buffer + _read, m_cur->m_start + m_ptr, rest);
						_read += rest;
						size -= rest;
						m_ptr += rest;
					}

					if (m_ptr >= m_cur->m_size && size > 0)
					{
						if (m_cur->m_var != m_vars.end())
						{
							auto ptr = m_ptr - m_cur->m_size;
							auto rest = m_cur->m_var->second.length() - ptr;

							if (rest > size)
								rest = size;

							memcpy((char*) buffer + _read, m_cur->m_var->second.c_str() + ptr, rest);
							_read += rest;
							size -= rest;
							m_ptr += rest;
						}
					}

					size_t whole = m_cur->m_size;
					if (m_cur->m_var != m_vars.end())
						whole += m_cur->m_var->second.length();
					if (m_ptr >= whole)
					{
						++m_cur;
						m_ptr = 0;
						if (m_cur == m_chunks.end())
							break;
					}
				}
				return _read;
			}
		};

		request_handler::request_handler(const std::string& usn)
		{
			m_vars.emplace_back("host", to_string(iface::get_default_interface()));
			m_vars.emplace_back("port", "6001");
			m_vars.emplace_back("uuid", usn);
		}

		static dom::XmlNodeListPtr env_body(dom::XmlDocumentPtr& doc)
		{
			dom::NSData ns [] = { { "s", "http://schemas.xmlsoap.org/soap/envelope/" } };
			auto body = doc->find("/s:Envelope/s:Body", ns);
			return body ? body->childNodes() : nullptr;
		}

		static void print_debug(const http_request& header, const std::string& SOAPAction, dom::XmlDocumentPtr& doc)
		{
			std::ostringstream o;
			o << header.m_method << " ";
			if (header.m_resource != "*")
			{
				auto it = header.find("host");
				if (it != header.end())
					o << it->value();
			}
			o << header.m_resource << " " << header.m_protocol;
			o << "\n  [ " << to_string(header.m_remote_address) << ":" << header.m_remote_port << " ]";

			auto ua = header.find("user-agent");
			if (ua != header.end())
			{
				o << " [ " << ua->value();
				auto pui = header.find("x-av-physical-unit-info");
				auto ci = header.find("x-av-client-info");
				if (pui != header.end() || ci != header.end())
				{
					o << " | ";
					if (pui != header.end())
					{
						o << pui->value();
						if (ci != header.end())
							o << " | ";
					}
					if (ci != header.end())
					{
						o << ci->value();
					}
				}
				o << " ]";
			}
			o << "\n";

			if (!SOAPAction.empty())
				o << "  [ " << SOAPAction << " ]\n";

			if (doc)
			{
				o << "\nSOAP:\n";
				auto children = env_body(doc);
				if (children)
					dom::Print(o, children);
				else
					dom::Print(o, doc->documentElement());
				o << "\n";
			}

			std::cout << o.str();
		}

		std::pair<fs::path, fs::path> pop(const fs::path& p)
		{
			if (p.empty())
				return std::make_pair(p, p);
			auto it = p.begin();
			auto root = *it++;
			fs::path rest;
			while (it != p.end())
				rest /= *it++;

			return std::make_pair(root, rest);
		}

		std::tuple<std::string, std::string, std::string> break_action(const std::string& action)
		{
			auto hash = action.find('#');

			// no hash - no function; no function - no sense
			if (hash == std::string::npos)
				return std::make_tuple(std::string(), std::string(), std::string());

			auto colon = action.find_last_of(':', hash);
			if (colon != std::string::npos)
				colon = action.find_last_of(':', colon - 1);

			return std::make_tuple(action.substr(0, colon), action.substr(colon + 1, hash - colon - 1), action.substr(hash + 1));
		}

		void request_handler::handle(const http_request& req, response& resp)
		{
			auto SOAPAction = req.SOAPAction();
			auto res = req.resource();
			auto method = req.method();

			dom::XmlDocumentPtr doc;

			if (method == http_method::post && !SOAPAction.empty())
			{
				doc = create_from_socket(req.request_data());
			}

			std::string soap_domain, soap_object, soap_method;
			std::tie(soap_domain, soap_object, soap_method) = break_action(SOAPAction);

			print_debug(req, SOAPAction, doc);

			fs::path root, rest;
			std::tie(root, rest) = pop(res); // pop leading slash
			std::tie(root, rest) = pop(rest);

			if (method == http_method::get)
			{
				if (root == "config")
				{
					if (rest == "device.xml")
						return make_templated(device::xml, "text/xml", resp);

					return make_file(boost::filesystem::path("data") / root / rest, resp);
				}
				if (root == "images")
					return make_file(boost::filesystem::path("data") / root / rest, resp);
			}

			if (method == http_method::post)
			{
				if (root == "upnp")
				{
					std::tie(root, rest) = pop(rest);
					if (root == "control")
					{
						if (rest == "content_directory" && soap_object == "ContentDirectory:1")
						{
							if (soap_method == "GetSystemUpdateID")
								return ContentDirectory_GetSystemUpdateID(req, resp);
							if (soap_method == "Browse")
								return ContentDirectory_Browse(req, resp);
							return make_404(resp);
						}
						if (rest == "connection_manager" && soap_object == "ContentManager:1")
						{
							return make_404(resp);
						}
						return make_404(resp);
					}

					if (root == "event")
					{
						if (rest == "content_directory" && soap_object == "ContentDirectory:1")
						{
							return make_404(resp);
						}
						if (rest == "connection_manager" && soap_object == "ContentManager:1")
						{
							return make_404(resp);
						}
						return make_404(resp);
					}
				}
			}

			make_404(resp);
		}

		void request_handler::make_templated(const char* tmplt, const char* content_type, response& resp)
		{
			auto & header = resp.header();
			header.clear();
			header.append("content-type", content_type);
			resp.content(std::make_shared<template_content>(tmplt, std::ref(m_vars)));
		}

		static struct
		{
			std::string ext;
			const char* mime_type;
		} s_extensions [] = {
			{ ".xml", "text/xml" },
			{ ".png", "image/png" }
		};

		void request_handler::make_file(const fs::path& path, response& resp)
		{
			if (!fs::exists(path))
				return make_404(resp);

			const char* content_type = "text/html";
			if (path.has_extension())
			{
				std::string cmp = path.extension().string();
				for (auto && c : cmp) c = std::tolower((unsigned char) c);

				for (auto && ext : s_extensions)
					if (ext.ext == cmp)
					{
						content_type = ext.mime_type;
						break;
					}
			}

			auto & header = resp.header();
			header.clear();
			header.append("content-type", content_type);
			header.append("last-modified")->out() << to_string(time::last_write(path));
			resp.content(content::from_file(path));
		}

		void request_handler::ContentDirectory_GetSystemUpdateID(const http_request& req, response& resp)
		{
			auto & header = resp.header();
			header.clear();
			header.append("content-type", "text/xml; charset=\"utf-8\"");
			resp.content(content::from_string(
				R"(<?xml version="1.0" encoding="utf-8"?>)"
				R"(<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body>)"
				R"(<u:GetSystemUpdateIDResponse xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">)"
				R"(<Id>1</Id>)"
				R"(</u:GetSystemUpdateIDResponse>)"
				R"(</s:Body></s:Envelope>)"
				));
		}

		void request_handler::ContentDirectory_Browse(const http_request& req, response& resp)
		{
			make_404(resp);
		}

		void request_handler::make_404(response& resp)
		{
			auto & header = resp.header();
			header.clear();
			header.m_status = 404;
			header.append("content-type", "text/plain");
			resp.content(content::from_string("File not found...\n"));
		}
	}
}
