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
#include <http/response.hpp>
#include <dom.hpp>
#include <algorithm>

#include <iomanip>

namespace net { namespace ssdp { namespace import { namespace av { namespace items {

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

			case ',':
				return make_pair(current_id, std::string(data, end));

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
			{
				if (!rest_of_id.empty() && rest_of_id[0] == ',')
				{
					if (item->get_token() == rest_of_id.c_str() + 1)
						return make_pair(item, std::string());
					else
						return make_pair(media_item_ptr(), rest_of_id);
				}
				return make_pair(item, rest_of_id);
			}

		return make_pair(media_item_ptr(), rest_of_id);
	}

	bool contains(const std::vector<std::string>& filter, const char* key)
	{
		if (filter.empty()) return true;
		return std::find(filter.begin(), filter.end(), key) != filter.end();
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

	template <typename T>
	bool is_empty(T t){ return t == 0; }
	bool is_empty(const std::string& t){ return t.empty(); }

	template <typename T>
	void output(std::ostream& o, T t){ o << t; }
	void output(std::ostream& o, const std::string& t){ o << net::xmlencode(t); }

#define PROPERTY(name, item) \
	if (!is_empty(metadata->name) && contains(filter, item)) \
	{\
		o << "    <" item ">"; \
		items::output(o, metadata->name); \
		o << "</" item ">\n"; \
	}

	void common_props_item::output(std::ostream& o, const std::vector<std::string>& filter, const client_interface_ptr& client, const config::config_ptr& config) const
	{
		const char* name = is_folder() ? "container" : "item";
		auto date = get_last_write_time();
		auto metadata = get_metadata();

		o << "  <" << name << " id=\"" << get_objectId_attr() << "\"";
		if (is_folder() && contains(filter, "@childCount"))
			o << " childCount=\"" << child_count() << "\"";
		o << " parentId=\"" << get_parent_attr() << "\" restricted=\"true\">\n    <dc:title>" << net::xmlencode(get_title()) << "</dc:title>\n";

		if (date && contains(filter, "dc:date"))
			o << "    <dc:date>" << to_iso8601(time::from_time_t(date)) << "</dc:date>\n";

		if (metadata)
		{
			if (!is_empty(metadata->m_artist) && contains(filter, "upnp:artist"))
			{
				o << "    <" "upnp:artist" ">";
				items::output(o, metadata->m_artist);
				o << "</" "upnp:artist" ">\n";
			};
			PROPERTY(m_artist,       "upnp:artist");
			PROPERTY(m_artist,       "dc:creator");
			//PROPERTY(m_album_artist, "");
			//PROPERTY(m_composer,     "");
			PROPERTY(m_album,        "upnp:album");
			PROPERTY(m_genre,        "upnp:genre");
			//PROPERTY(m_date,         "");
			PROPERTY(m_comment,      "dc:description");
			PROPERTY(m_track,        "upnp:originalTrackNumber");
		}

		o << "    <upnp:class>" << get_upnp_class() << "</upnp:class>\n";

		cover(o, filter, client, config);
		main_res(o, filter, client, config);

		o << "  </" << name << ">\n";
	}
#undef PROPERTY

	/*item->u.resource.cnv*/
	namespace dlna_org
	{
		enum flag: unsigned int
		{
			SENDER_PACED               = (1U << 31),
			TIME_BASED_SEEK            = (1U << 30),
			BYTE_BASED_SEEK            = (1U << 29),
			PLAY_CONTAINER             = (1U << 28),
			S0_INCREASE                = (1U << 27),
			SN_INCREASE                = (1U << 26),
			RTSP_PAUSE                 = (1U << 25),
			STREAMING_TRANSFER_MODE    = (1U << 24),
			INTERACTIVE_TRANSFERT_MODE = (1U << 23),
			BACKGROUND_TRANSFERT_MODE  = (1U << 22),
			CONNECTION_STALL           = (1U << 21),
			DLNA_V15                   = (1U << 20),
		};
	}

	static void protocol_info(std::ostream& o, const dlna::Profile* profile, const client_interface_ptr & /*client*/)
	{
		static const unsigned int flags = dlna_org::DLNA_V15 | dlna_org::BYTE_BASED_SEEK | dlna_org::STREAMING_TRANSFER_MODE;
		auto mime = profile && profile->m_mime && *profile->m_mime ? profile->m_mime : "video/mpeg";
		o << "http-get:*:" << mime << ":" << "DLNA.ORG_PS=1;DLNA.ORG_CI=0;DLNA.ORG_OP=01;";
		if (profile)
			o << "DLNA.ORG_PN=" << profile->m_name << ";";
		o << "DLNA.ORG_FLAGS=" << std::setfill('0') << std::setw(8) << std::hex << flags << "000000000000000000000000" << std::dec;
	}

	void common_props_item::main_res(std::ostream& o, const std::vector<std::string>& filter, const client_interface_ptr& client, const config::config_ptr& config) const
	{
		auto properties = get_properties();
		auto profile = get_profile();

		auto size        = properties ? properties->m_size : 0;
		auto bitrate     = properties ? properties->m_bitrate : 0;
		auto duration    = properties ? properties->m_duration * 1000 : 0;
		auto sample_freq = properties ? properties->m_sample_freq : 0;
		auto channels    = properties ? properties->m_channels : 0;
		auto width       = properties ? properties->m_width : 0;
		auto height      = properties ? properties->m_height : 0;

		if (!is_folder() && contains(filter, "res"))
		{
			o << "    <res xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0/\"";
			if (contains(filter, "res@protocolInfo"))
			{
				o << " protocolInfo=\"";
				protocol_info(o, profile, client);
				o << "\"";
			}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4127) // "conditional expression is constant" due to "do { ... } while(0)"
#endif

#define SIMPLE_RES_ATTR2(name, val) \
	do { if (val && contains(filter, "res@" #name)) { o << " " #name "=\"" << val << "\""; } } while (0)

#define SIMPLE_RES_ATTR(name) SIMPLE_RES_ATTR2(name, name)

			SIMPLE_RES_ATTR(bitrate);

			if (duration && contains(filter, "res@duration"))
			{
				auto millis = duration % 1000;
				auto secs = duration / 1000;
				auto mins = secs / 60;
				auto hours = mins / 60;

				secs %= 60;
				mins %= 60;

				o << " duration=\"" << part(hours) << ":" << part(mins) << ":" << part(secs) << "." << part(millis, 3) << "\"";
			}

			SIMPLE_RES_ATTR2(sampleFrequency, sample_freq);
			SIMPLE_RES_ATTR2(nrAudioChannels, channels);
			SIMPLE_RES_ATTR(size);

#ifdef _MSC_VER
#pragma warning(pop)
#endif

			if (width && height && contains(filter, "res@resolution"))
			{
				o << " resolution=\"" << width << "x" << height << "\"";
			}

			o << ">http://" << net::to_string(config->iface) << ":" << (int) config->port << "/upnp/media/" << get_objectId_attr() << "</res>\n";
		}
	}

	void common_props_item::cover(std::ostream& o, const std::vector<std::string>& filter, const client_interface_ptr& client, const config::config_ptr& config) const
	{
		if (is_image())
			return;

		auto cover = get_media(false);
		if (!cover)
			return;

		dlna::Profile tn_profile { "JPEG_TN", "image/jpeg", "", dlna::Class::Image };
		std::string forced_pn;
		auto profile = cover->profile();
		if (profile)
		{
			tn_profile = *profile;
			forced_pn = tn_profile.m_name;
			auto pos = forced_pn.find('_');
			forced_pn = forced_pn.substr(0, pos) + "_TN";
			tn_profile.m_name = forced_pn.c_str();
		}

		if (contains(filter, "upnp:albumArtURI"))
		{
			o << "    <upnp:albumArtURI";
			if (contains(filter, "upnp:albumArtURI@dlna:profileID"))
				o << " dlna:profileID=\"" << tn_profile.m_name << "\"";
			o << ">http://" << net::to_string(config->iface) << ":" << (int) config->port << "/upnp/thumb/" << get_objectId_attr() << "</upnp:albumArtURI>\n";
		}

		if (contains(filter, "res"))
		{
			o << "    <res xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0/\"";
			if (contains(filter, "res@protocolInfo"))
			{
				o << " protocolInfo=\"";
				protocol_info(o, &tn_profile, client);
				o << "\"";
			}

			o << ">http://" << net::to_string(config->iface) << ":" << (int) config->port << "/upnp/thumb/" << get_objectId_attr() << "</res>\n";
		}
	}

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

	void root_item::add_child(media_item_ptr child)
	{
		m_children.push_back(child);
		auto id = ++m_current_max;
		child->set_id(id);
		child->set_objectId_attr(get_raw_objectId_attr() + SEP + std::to_string(id));
		child->set_parent_attr("0");
	}

	void root_item::remove_child(media_item_ptr child)
	{
		auto pos = std::find(m_children.begin(), m_children.end(), child);
		if (pos != m_children.end())
			m_children.erase(pos);
	}

	struct media_file : media
	{
		fs::path      m_path;
		dlna::Profile m_profile;
		bool          m_main_resource;

		media_file(const fs::path& path, const dlna::Profile& profile, bool main)
			: m_path(path)
			, m_profile(profile)
			, m_main_resource(main)
		{}

		bool prep_response(http::response& resp) override
		{
			if (!fs::exists(m_path))
				return false;


			auto& header = resp.header();
			header.append("content-type", m_profile.m_mime);
			header.append("last-modified")->out() << to_string(time::last_write(m_path));
			resp.content(http::content::from_file(m_path));

			if (m_main_resource && resp.first_range())
				log::info() << "Serving " << m_path;

			return true;
		}

		const dlna::Profile* profile() const override { return &m_profile; }
	};

	media_ptr media::from_file(const boost::filesystem::path& path, bool main_resource)
	{
		auto profile = dlna::Profile::guess_from_file(path);
		if (!profile)
			return nullptr;
		return std::make_shared<media_file>(path, *profile, main_resource);
	}

}}}}}
