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
#include <http/response.hpp>
#include <dom.hpp>
#include <algorithm>
#include <log.hpp>

#include <iomanip>

namespace net { namespace ssdp { namespace import { namespace av {

	extern Log::Module Multimedia {"AVMS"};
	struct log : public Log::basic_log<log>
	{
		static const Log::Module& module() { return Multimedia; }
	};

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

	static std::vector<std::string> parse_filter(const std::string& sort)
	{
		if (sort.empty() || sort == "*")
			return std::vector<std::string>();

		return std::vector<std::string>();
	}

	error_code ContentDirectory::GetSystemUpdateID(const http::http_request& http_request,
	                                               /* OUT */ ui4& Id)
	{
		Id = m_device->system_update_id();
		log::debug() << "GetSystemUpdateID() = " << Id;
		return error::no_error;
	}

	error_code ContentDirectory::Search(const http::http_request& http_request,
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

	error_code ContentDirectory::GetSearchCapabilities(const http::http_request& http_request,
	                                                   /* OUT */ std::string& SearchCaps)
	{
		//SearchCaps = "*";
		return error::no_error;
	}

	error_code ContentDirectory::GetSortCapabilities(const http::http_request& http_request,
	                                                 /* OUT */ std::string& SortCaps)
	{
		//SortCaps = "*";
		return error::no_error;
	}

	error_code ContentDirectory::Browse(const http::http_request& http_request,
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
						child->output(value, filter);
					}

					NumberReturned = children.size();
					TotalMatches = item->predict_count(StartingIndex + NumberReturned);
				}
				else
				{
					item->check_updates();
					item->output(value, filter);
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

	error_code ConnectionManager::GetCurrentConnectionInfo(const http::http_request& http_request,
	                                                       /* IN  */ i4 ConnectionID,
	                                                       /* OUT */ i4& RcsID,
	                                                       /* OUT */ i4& AVTransportID,
	                                                       /* OUT */ std::string& ProtocolInfo,
	                                                       /* OUT */ std::string& PeerConnectionManager,
	                                                       /* OUT */ i4& PeerConnectionID,
	                                                       /* OUT */ A_ARG_TYPE_Direction& Direction,
	                                                       /* OUT */ A_ARG_TYPE_ConnectionStatus& Status)
	{
		return error::not_implemented;
	}

	error_code ConnectionManager::ConnectionComplete(const http::http_request& http_request,
	                                                 /* IN  */ i4 ConnectionID)
	{
		return error::not_implemented;
	}

	error_code ConnectionManager::PrepareForConnection(const http::http_request& http_request,
	                                                   /* IN  */ const std::string& RemoteProtocolInfo,
	                                                   /* IN  */ const std::string& PeerConnectionManager,
	                                                   /* IN  */ i4 PeerConnectionID,
	                                                   /* IN  */ A_ARG_TYPE_Direction Direction,
	                                                   /* OUT */ i4& ConnectionID,
	                                                   /* OUT */ i4& AVTransportID,
	                                                   /* OUT */ i4& RcsID)
	{
		return error::not_implemented;
	}

	error_code ConnectionManager::GetProtocolInfo(const http::http_request& http_request,
	                                              /* OUT */ std::string& Source,
	                                              /* OUT */ std::string& Sink)
	{
		Source =
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
			"http-get:*:image/*:*";

		return error::no_error;
	}

	error_code ConnectionManager::GetCurrentConnectionIDs(const http::http_request& http_request,
	                                                      /* OUT */ std::string& ConnectionIDs)
	{
		return error::not_implemented;
	}


	namespace items
	{
		std::pair<ulong, std::string> pop_id(const std::string& id)
		{
			ulong current_id = 0;

			const char* data = id.c_str();
			const char* end = data + id.length();

			if (data == end || !std::isdigit((unsigned int) *data))
				return make_pair(INVALID_ID, std::string());

			while (data < end)
			{
				switch (*data)
				{
				case '0': case '1': case '2': case '3': case '4':
				case '5': case '6': case '7': case '8': case '9':
					current_id *= 10;
					current_id += *data - '0';
					break;

				case SEP:
					return make_pair(current_id, std::string(data + 1, end));

				default:
					return make_pair(INVALID_ID, std::string());
				}
				++data;
			}

			// if we are here, there was no SEP until the end of the id
			return make_pair(current_id, std::string());
		}

		std::pair<media_item_ptr, std::string> find_item(std::vector<media_item_ptr>& items, const std::string& id)
		{
			ulong current_id;
			std::string rest_of_id;

			std::tie(current_id, rest_of_id) = pop_id(id);

			if (current_id == INVALID_ID)
				return make_pair(media_item_ptr(), rest_of_id);

			for (auto && item : items)
				if (item->get_id() == current_id)
					return make_pair(item, rest_of_id);

			return make_pair(media_item_ptr(), rest_of_id);
		}

		bool contains(const std::vector<std::string>& filter, const char* key)
		{
			if (filter.empty()) return true;
			return std::find(filter.begin(), filter.end(), key) != filter.end();
		}

		void common_props_item::output_open(std::ostream& o, const std::vector<std::string>& filter, ulong child_count) const
		{
			const char* name = is_folder() ? "container" : "item";
			o << "  <" << name << " id=\"" << get_objectId_attr() << "\"";
			if (is_folder() && contains(filter, "@childCount"))
				o << " childCount=\"" << child_count << "\"";
			o << " parentId=\"" << get_parent_attr() << "\" restricted=\"true\">\n    <dc:title>" << net::xmlencode(get_title()) << "</dc:title>\n";
		}

		struct part
		{
			part(net::ulong val, int w = 2) : val(val), w(w) {}
			net::ulong val;
			int w;
		};
		std::ostream& operator << (std::ostream& o, const part& op)
		{
			return o << std::setfill('0') << std::setw(op.w) << op.val;
		}

		void common_props_item::output_close(std::ostream& o, const std::vector<std::string>& filter) const
		{
			const char* name = is_folder() ? "container" : "item";
			auto date = get_last_write_time();
			auto duration = get_duration();

			if (date && contains(filter, "dc:date"))
				o << "    <dc:date>" << to_iso8601(time::from_time_t(date)) << "</dc:date>\n";

			if (duration && contains(filter, "duration"))
			{
				auto millis = duration % 1000;
				auto secs = duration / 1000;
				auto mins = secs / 60;
				auto hours = mins / 60;

				secs %= 60;
				mins %= 60;

				o << "    <duration>" << part(hours) << ":" << part(mins) << ":" << part(secs) << "." << part(millis, 3) << "</duration>\n";
			}
			o << "    <upnp:class>" << get_upnp_class() << "</upnp:class>  </" << name << ">\n";
		}

#pragma region root_item
		struct root_item : common_props_item
		{
			root_item(MediaServer* device)
				: common_props_item(device)
				, m_update_id(1)
				, m_current_max(0)
			{
			}

			container_type list(ulong start_from, ulong max_count)           override;
			ulong          predict_count(ulong served) const                 override { return m_children.size(); }
			media_item_ptr get_item(const std::string& id)                   override;
			bool           is_folder() const                                 override { return true; }
			void           output(std::ostream& o,
			                   const std::vector<std::string>& filter) const override;
			const char*    get_upnp_class() const                            override { return "object.container.storageFolder"; }
			ulong          update_id() const                                 override { return m_device->system_update_id(); }
			virtual void   add_child(media_item_ptr);
			virtual void   remove_child(media_item_ptr);

			

			std::string get_parent_attr() const override {
				static std::string parent_id { "-1" };
				return parent_id;
			}

		private:
			ulong          m_current_max;
			time_t         m_update_id;
			container_type m_children;
		};

		root_item::container_type root_item::list(ulong start_from, ulong max_count)
		{
			std::vector<media_item_ptr> out;
			if (start_from > m_children.size())
				start_from = m_children.size();

			auto end_at = m_children.size() - start_from;
			if (end_at > max_count)
				end_at = max_count;
			end_at += start_from;

			for (auto i = start_from; i < end_at; ++i)
				out.push_back(m_children.at(i));

			return out;
		}

		media_item_ptr root_item::get_item(const std::string& id)
		{
			media_item_ptr candidate;
			std::string rest_of_id;

			std::tie(candidate, rest_of_id) = find_item(m_children, id);

			if (!candidate || rest_of_id.empty())
				return candidate;

			if (!candidate->is_folder())
				return nullptr;

			return candidate->get_item(rest_of_id);
		}

		void root_item::output(std::ostream& o, const std::vector<std::string>& filter) const
		{
			output_open(o, filter, m_children.size());
			output_close(o, filter);
		}

		void root_item::add_child(media_item_ptr child)
		{
			m_children.push_back(child);
			auto id = ++m_current_max;
			child->set_id(id);
			child->set_objectId_attr(get_objectId_attr() + SEP + std::to_string(id));
		}

		void root_item::remove_child(media_item_ptr child)
		{
			auto pos = std::find(m_children.begin(), m_children.end(), child);
			if (pos != m_children.end())
				m_children.erase(pos);
		}
#pragma endregion

		std::string media_item::get_parent_attr() const
		{
			auto pos = m_object_id.find_last_of(items::SEP);
			if (pos == std::string::npos)
				return "-1";
			return m_object_id.substr(0, pos);
		}

	}

#pragma region media_server
	items::media_item_ptr MediaServer::get_item(const std::string& id)
	{
		items::media_item_ptr candidate;
		ulong current_id;
		std::string rest_of_id;

		std::tie(current_id, rest_of_id) = items::pop_id(id);
		if (current_id == 0)
			candidate = m_root_item;

		if (!candidate || rest_of_id.empty())
			return candidate;

		return candidate->get_item(rest_of_id);
	}

	items::root_item_ptr MediaServer::create_root_item()
	{
		auto root = std::make_shared<items::root_item>(this);
		root->set_title("root");
		root->set_objectId_attr("0");
		return root;
	}

	void MediaServer::object_changed()
	{
		::time(&m_system_update_id);
	}

	void MediaServer::add_root_element(items::media_item_ptr ptr)
	{
		m_root_item->add_child(ptr);
	}

	void MediaServer::remove_root_element(items::media_item_ptr ptr)
	{
		m_root_item->remove_child(ptr);
	}
#pragma endregion
}}}}
