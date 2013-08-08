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
		void request_handler::handle(const http_request& req, response& resp)
		{
			auto res = req.resource();

			// THIS should be mapping
			if (res == "/config/device.xml")
				return make_templated(device::xml, "text/xml", resp);
			if (res == "/config/directory.xml")
				return make_file(boost::filesystem::path("data") / res, "text/xml", resp);
			if (res == "/config/manager.xml")
				return make_file(boost::filesystem::path("data") / res, "text/xml", resp);
			if (res == "/images/icon-256.png")
				return make_file(boost::filesystem::path("data") / res, "image/png", resp);
			make_404(resp);
		}

		void request_handler::make_templated(const char* tmplt, const char* content_type, response& resp)
		{
			auto & header = resp.header();
			header.clear();
			header.append("content-type", content_type);
			resp.content(std::make_shared<template_content>(tmplt, std::ref(m_vars)));
		}

		void request_handler::make_file(const fs::path& path, const char* content_type, response& resp)
		{
			if (!fs::exists(path))
				return make_404(resp);
			auto & header = resp.header();
			header.clear();
			header.append("content-type", content_type);
			resp.content(content::from_file(path));
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
