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
#include <device.hpp>

namespace net
{
	namespace ssdp
	{
		bool client_matcher::matches(const http::http_request& request) const
		{
			auto header = request.user_agent();
			if (std::regex_match(header, m_user_agent))
				return true;

			if (!m_other_header.empty())
			{
				auto other = request.simple(m_other_header);
				if (std::regex_match(header, m_user_agent))
					return true;
			}
			return false;
		}

		struct prop
		{
			std::string name;
			std::string attrs;
			std::string value;

			prop(const std::string& name, const std::string& value)
				: name(name), value(value)
			{}

			prop(const std::string& name, const std::string& attrs, const std::string& value)
				: name(name), attrs(attrs), value(value)
			{}

			friend std::ostream& operator << (std::ostream& o, const prop& rhs)
			{
				o << "<" << rhs.name;
				if (!rhs.attrs.empty())
					o << " " << rhs.attrs;
				if (rhs.value.empty())
					o << "/>";
				else
					o << ">" << rhs.value << "</" << rhs.name << ">";
				return o << R"(
)";
			}
		};

		std::string Device::get_configuration(const std::string& host) const
		{
			std::ostringstream o;
			o
				<< R"(<?xml version="1.0" encoding="utf-8"?>
<root xmlns:dlna="urn:schemas-dlna-org:device-1-0" xmlns="urn:schemas-upnp-org:device-1-0">
<specVersion>
  <major>1</major>
  <minor>0</minor>
</specVersion>
<URLBase>http://)" << host << R"(/</URLBase>
<device>
)";
			o
				<< prop("dlna:X_DLNADOC", R"(xmlns:dlna="urn:schemas-dlna-org:device-1-0")", "DMS-1.50")
				<< prop("dlna:X_DLNADOC", R"(xmlns:dlna="urn:schemas-dlna-org:device-1-0")", "M-DMS-1.50")
				<< prop("deviceType", get_type())
				<< prop("friendlyName", m_info.m_model.m_friendly_name)
				<< prop("manufacturer", m_info.m_manufacturer.m_name)
				<< prop("manufacturerURL", m_info.m_manufacturer.m_url)
				<< prop("modelDescription", get_description())
				<< prop("modelName", m_info.m_model.m_name)
				<< prop("modelNumber", m_info.m_model.m_number)
				<< prop("modelURL", m_info.m_model.m_url)
				<< prop("serialNumber", std::string())
				<< prop("UPC", std::string())
				<< prop("UDN", m_usn);
			o
				<< R"(<iconList>
  <icon>
    <mimetype>image/jpeg</mimetype>
    <width>120</width>
    <height>120</height>
    <depth>24</depth>
    <url>/images/icon-256.png</url>
  </icon>
</iconList>
<serviceList>
)";
			o
				<< prop("presentationURL", "http://" + host + "/console/index.html");

			auto shared = ((Device*) this)->shared_from_this();
			size_t int_id = 0;
			for (auto&& service: services(shared))
			{
				auto id = "service" + std::to_string(int_id++);
				o
					<< R"(<service>
)";
				o
					<< prop("serviceType", service->get_type())
					<< prop("serviceId", service->get_id())
					<< prop("SCPDURL", "/config/" + id)
					<< prop("controlURL", "/upnp/control/" + id)
					<< prop("eventSubURL", "/upnp/event/" + id);
				o
					<< R"(</service>
)";
			}

			o << R"(</serviceList>
</device>
</root>
)";
			return o.str();
		}
	}
}