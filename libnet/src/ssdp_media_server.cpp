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
#include <ssdp_media_server.hpp>
#include <response.hpp>
#include <dom.hpp>

namespace net
{
	namespace ssdp
	{
		namespace av
		{
			namespace service
			{
				void content_directory::control_GetSystemUpdateID(const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response)
				{
					auto & header = response.header();
					header.clear(m_device->server());
					header.append("content-type", "text/xml; charset=\"utf-8\"");
					response.content(http::content::from_string(
						R"(<?xml version="1.0" encoding="utf-8"?>)"
						R"(<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body>)"
						R"(<u:GetSystemUpdateIDResponse xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">)"
						R"(<Id>1</Id>)"
						R"(</u:GetSystemUpdateIDResponse>)"
						R"(</s:Body></s:Envelope>)"
						));
				}
				void content_directory::control_Browse(const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response)
				{
					std::string browse_flag;
					//auto children = env_body(doc);
					if (doc)
					{
						dom::NSData ns [] = { { "s", "http://schemas.xmlsoap.org/soap/envelope/" }, { "upnp", "urn:schemas-upnp-org:service:ContentDirectory:1" } };
						auto BrowseFlag = doc->find("/s:Envelope/s:Body/upnp:Browse/BrowseFlag", ns);
						if (BrowseFlag)
							browse_flag = BrowseFlag->stringValue();
					}

					auto & header = response.header();
					header.clear(m_device->server());
					header.append("content-type", "text/xml; charset=\"utf-8\"");
					auto msg =
						R"(<?xml version="1.0" encoding="utf-8"?>)"
						R"(<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/"><s:Body>)"

						R"(<u:BrowseResponse xmlns:u="urn:schemas-upnp-org:service:ContentDirectory:1">)"
						R"(<Result>)"
						R"(&lt;DIDL-Lite xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/"&gt;)"

						// EMPTY RESPONSE

						R"(&lt;/DIDL-Lite&gt;)"
						R"(</Result>)"
						R"(<NumberReturned>0</NumberReturned>)"
						// from upnp spec: If BrowseMetadata is specified in the BrowseFlags then TotalMatches = 1
						R"(<TotalMatches>1</TotalMatches>)"
						R"(<UpdateID>1</UpdateID>)"
						R"(</u:BrowseResponse>)"

						R"(</s:Body></s:Envelope>)";
					std::cout << msg << "\n";
					response.content(http::content::from_string(msg));
				}
			}
		}
	}
}
