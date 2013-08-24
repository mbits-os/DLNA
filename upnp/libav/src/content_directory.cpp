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
#include <media_server.hpp>
#include "media_server_internal.hpp"

namespace net { namespace ssdp { namespace import { namespace av {

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

	static std::vector<std::string> parse_filter(const std::string& filter)
	{
		if (filter.empty() || filter == "*")
			return std::vector<std::string>();

		std::vector<std::string> out;

		std::string::size_type pos = 0;
		while (true)
		{
			auto comma = filter.find(',', pos);
			if (comma == std::string::npos)
			{
				out.push_back(filter.substr(pos));
				break;
			}
			out.push_back(filter.substr(pos, comma - pos));
			pos = comma + 1;
		}

		return out;
	}

	error_code ContentDirectory::GetSystemUpdateID(const client_info_ptr& client,
	                                               const http::http_request& http_request,
	                                               /* OUT */ ui4& Id)
	{
		Id = m_device->system_update_id();
		log::debug() << "GetSystemUpdateID() = " << Id;
		return error::no_error;
	}

	error_code ContentDirectory::Search(const client_info_ptr& client,
	                                    const http::http_request& http_request,
	                                    /* IN  */ const std::string& ContainerID,
	                                    /* IN  */ const std::string& SearchCriteria,
	                                    /* IN  */ const std::string& Filter,
	                                    /* IN  */ ui4 StartingIndex,
	                                    /* IN  */ ui4 RequestedCount,
	                                    /* IN  */ const std::string& SortCriteria,
	                                    /* OUT */ std::string& Result,
	                                    /* OUT */ ui4& NumberReturned,
	                                    /* OUT */ ui4& TotalMatches,
	                                    /* OUT */ ui4& UpdateID)
	{
		return error::not_implemented;
	}

	error_code ContentDirectory::GetSearchCapabilities(const client_info_ptr& client,
	                                                   const http::http_request& http_request,
	                                                   /* OUT */ std::string& SearchCaps)
	{
		//SearchCaps = "*";
		return error::no_error;
	}

	error_code ContentDirectory::GetSortCapabilities(const client_info_ptr& client,
	                                                 const http::http_request& http_request,
	                                                 /* OUT */ std::string& SortCaps)
	{
		//SortCaps = "*";
		return error::no_error;
	}

	error_code ContentDirectory::Browse(const client_info_ptr& client,
	                                    const http::http_request& http_request,
	                                    /* IN  */ const std::string& ObjectID,
	                                    /* IN  */ A_ARG_TYPE_BrowseFlag BrowseFlag,
	                                    /* IN  */ const std::string& Filter,
	                                    /* IN  */ ui4 StartingIndex,
	                                    /* IN  */ ui4 RequestedCount,
	                                    /* IN  */ const std::string& SortCriteria,
	                                    /* OUT */ std::string& Result,
	                                    /* OUT */ ui4& NumberReturned,
	                                    /* OUT */ ui4& TotalMatches,
	                                    /* OUT */ ui4& UpdateID)
	{
		if (BrowseFlag == A_ARG_TYPE_BrowseFlag_UNKNOWN)
			return error::invalid_action;

		std::ostringstream value;
		NumberReturned = 0;
		TotalMatches = 0;
		UpdateID = m_device->system_update_id();

		{
			value << R"(<DIDL-Lite xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/" xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/">)" "\n";
			auto item = m_device->get_item(ObjectID);

			if (item)
				log::debug() << type_info<A_ARG_TYPE_BrowseFlag>::to_string(BrowseFlag) << " [" << ObjectID << "] \"" << item->get_title() << "\"";
			else
				log::warning() << type_info<A_ARG_TYPE_BrowseFlag>::to_string(BrowseFlag) << " [" << ObjectID << "] failed";

			if (item)
			{
				auto filter = parse_filter(Filter);

				if (BrowseFlag == VALUE_BrowseDirectChildren)
				{
					auto children = item->list(StartingIndex, RequestedCount);
					for (auto && child : children)
					{
						log::debug() << "    [" << child->get_objectId_attr() << "] \"" << child->get_title() << "\"";
						child->output(value, filter, m_device->config());
					}

					NumberReturned = children.size();
					TotalMatches = item->predict_count(StartingIndex + NumberReturned);
				}
				else
				{
					item->check_updates();
					item->output(value, filter, m_device->config());
					NumberReturned = 1;
					TotalMatches = 1;
				}

				// could change during the item->list
				UpdateID = item->update_id();
			}

			value << "</DIDL-Lite>";
		}

		Result = std::move(value.str());
		return error::no_error;
	}

}}}}
