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
				namespace
				{
					const char* SOAP_BODY_START = 
						R"(<?xml version="1.0" encoding="utf-8"?>)" "\n"
						R"(<s:Envelope xmlns:s="http://schemas.xmlsoap.org/soap/envelope/" s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">)" "\n"
						R"(<s:Body>)" "\n";
					const char* SOAP_BODY_STOP = "\n</s:Body>\n</s:Envelope>\n";
				}

				void service_helper::soap_answer(const char* method, http::response& response, const std::string& body)
				{
					auto & header = response.header();
					header.clear(m_device->server());
					header.append("content-type", "text/xml; charset=\"utf-8\"");
					std::ostringstream o;
					o
						<< SOAP_BODY_START
						<< "<u:" << method << "Response xmlns:u=\"" << get_type() << "\">\n"
						<< body
						<< "\n</u:" << method << "Response>"
						<< SOAP_BODY_STOP;

					response.content(http::content::from_string(o.str()));
				}

				void service_helper::upnp_error(http::response& response, const ssdp::service_error& e)
				{
					auto & header = response.header();
					header.clear(m_device->server());
					header.m_status = 500;
					header.append("content-type", "text/xml; charset=\"utf-8\"");
					std::ostringstream o;
					o
						<< SOAP_BODY_START
						<< "  <s:Fault>\n"
						   "    <faultcode>s:Client</faultcode>\n"
						   "    <faultstring>UPnPError</faultstring>\n"
						   "    <detail>\n"
						   "      <UPnPError xmlns=\"urn:schemas-upnp-org:control-1-0\">\n"
						   "        <errorCode>" << e.code() << "</errorCode>\n"
						   "        <errorDescription>" << e.message() << "</errorDescription>\n"
						   "      </UPnPError>\n"
						   "    </detail>\n" 
						   "  </s:Fault>"
						<< SOAP_BODY_STOP;

					response.content(http::content::from_string(o.str()));

				}

				std::string content_directory::soap_control_GetSortCapabilities(const http::http_request& req, const dom::XmlDocumentPtr& doc)
				{
					return "<SortCaps>*</SortCaps>";
				}
				std::string content_directory::soap_control_GetSystemUpdateID(const http::http_request& req, const dom::XmlDocumentPtr& doc)
				{
					std::ostringstream o;
					o << "<Id>" << m_device->system_update_id() << "</Id>";
					return o.str();
				}
				template <typename T, typename Node>
				T extract(const Node& node, const std::string& xpath, const T& def = T())
				{
					auto ptr = node->find(xpath);
					if (!ptr)
						return def;

					auto text = ptr->stringValue();
					if (text.empty())
						return def;

					T val;
					std::istringstream i(text);
					i >> val;
					return val;
				}

				static search_criteria parse_sort(const std::string& sort)
				{
					return search_criteria();
				}

				std::string content_directory::soap_control_Browse(const http::http_request& req, const dom::XmlDocumentPtr& doc)
				{
					if (!doc)
						throw ssdp::service_error(ssdp::error::invalid_args);

					dom::NSData ns [] = { { "s", "http://schemas.xmlsoap.org/soap/envelope/" }, { "upnp", "urn:schemas-upnp-org:service:ContentDirectory:1" } };
					auto browse = doc->find("/s:Envelope/s:Body/upnp:Browse", ns);
					if (!browse)
						throw ssdp::service_error(ssdp::error::invalid_args);

					auto object_id       = extract<std::string>(browse, "ObjectID", "0");
					auto browse_flag     = extract<std::string>(browse, "BrowseFlag");
					auto filter          = extract<std::string>(browse, "Filter");
					auto starting_index  = extract<unsigned long long>(browse, "StartingIndex", 0);
					auto requested_count = extract<unsigned long long>(browse, "RequestedCount", (unsigned long long)-1);
					auto sort_criteria   = extract<std::string>(browse, "SortCriteria");

					bool browse_children = browse_flag == "BrowseDirectChildren";
					if (!browse_children && browse_flag != "BrowseMetadata")
						throw ssdp::service_error(ssdp::error::invalid_action);

					std::ostringstream value;
					unsigned long long returned = 0;
					unsigned long long contains = 0;
					unsigned long long update_id = m_device->system_update_id();

					{
						value<< "<Result>" << R"(&lt;DIDL-Lite xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/"&gt;)";
						auto item = m_device->get_item(object_id);

						if (item)
						{
							update_id = item->update_id();

							if (browse_children)
							{
								auto children = item->list(starting_index, requested_count, parse_sort(sort_criteria));
								// for each child in children: filter and output the child
								returned = children.size();
								contains = item->predict_count(starting_index + returned);
							}
							else
							{
								// filter and output the item
								returned = 1;
								contains = 1;
							}
						}

						value
							<< R"(&lt;/DIDL-Lite&gt;)" << "</Result>\n"
							<< "<NumberReturned>" << returned << "</NumberReturned>\n"
							<< "<TotalMatches>" << contains << "</TotalMatches>\n"
							<< "<UpdateID>" << update_id << "</UpdateID>\n";
					}

					return value.str();
				}

				std::string connection_manager::soap_control_GetProtocolInfo(const http::http_request& req, const dom::XmlDocumentPtr& doc)
				{
					return
						"<Source>"
							"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_SM,"
							"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_MED,"
							"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_LRG,"
							"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3,"
							"http-get:*:audio/L16:DLNA.ORG_PN=LPCM,"
							"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_HD_24_AC3_ISO;SONY.COM_PN=AVC_TS_HD_24_AC3_ISO,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_24_AC3;SONY.COM_PN=AVC_TS_HD_24_AC3,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_24_AC3_T;SONY.COM_PN=AVC_TS_HD_24_AC3_T,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_PS_PAL,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_PS_NTSC,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_50_L2_T,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_60_L2_T,"
							"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_TS_SD_EU_ISO,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_EU,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_EU_T,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_50_AC3_T,"
							"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_TS_HD_50_L2_ISO;SONY.COM_PN=HD2_50_ISO,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_60_AC3_T,"
							"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_TS_HD_60_L2_ISO;SONY.COM_PN=HD2_60_ISO,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_HD_50_L2_T;SONY.COM_PN=HD2_50_T,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_HD_60_L2_T;SONY.COM_PN=HD2_60_T,"
							"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_HD_50_AC3_ISO;SONY.COM_PN=AVC_TS_HD_50_AC3_ISO,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_50_AC3;SONY.COM_PN=AVC_TS_HD_50_AC3,"
							"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_HD_60_AC3_ISO;SONY.COM_PN=AVC_TS_HD_60_AC3_ISO,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_60_AC3;SONY.COM_PN=AVC_TS_HD_60_AC3,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_50_AC3_T;SONY.COM_PN=AVC_TS_HD_50_AC3_T,"
							"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_60_AC3_T;SONY.COM_PN=AVC_TS_HD_60_AC3_T,"
							"http-get:*:video/x-mp2t-mphl-188:*,"
							"http-get:*:*:*,"
							"http-get:*:video/*:*,"
							"http-get:*:audio/*:*,"
							"http-get:*:image/*:*"
						"</Source>"
						"<Sink></Sink>"
						;
				}
			}

			std::vector<media_item_ptr> media_item::list(unsigned long long start_from, unsigned long long max_count, const search_criteria& sort) const
			{
				return std::vector<media_item_ptr>();
			}

			unsigned long long media_item::predict_count(unsigned long long served) const
			{
				return served;
			}

			unsigned long long media_item::update_id() const
			{
				return 0;
			}

			media_item_ptr media_server::get_item(const std::string& id)
			{
				return nullptr;
			}
		}
	}
}
