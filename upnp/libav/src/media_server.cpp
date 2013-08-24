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

namespace net { namespace ssdp { namespace import { namespace av {

	Log::Module Multimedia {"AVMS"};

	bool MediaServer::call_http(const http::http_request& req, const boost::filesystem::path& root, const boost::filesystem::path& rest, http::response& resp)
	{
		bool main_resource = root == "media";
		if (!main_resource && root != "thumb")
			return false;

		auto item = get_item(rest.string());
		if (!item)
			return false;

		auto info = item->get_media(main_resource);
		if (!info)
			return false;

		resp.header().clear(server());
		return info->prep_response(resp);
	}

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

	struct default_client_info : client_interface
	{
		default_client_info(const http::http_request& request)
			: client_interface("Unknown")
			, m_request(request)
		{}

		bool from_config() const override { return false; }
		bool matches(const http::http_request& request) const override
		{
			for (auto&& here : m_request)
				if (request.find(here.name()) == request.end())
					return false;

			for (auto && there : request)
			{
				auto it = m_request.find(there.name());
				if (it == m_request.end() || it->value() != there.value())
					return false;
			}

			return true;
		}
		http::http_request m_request;
	};

	client_interface_ptr MediaServer::create_default_client(const http::http_request& request)
	{
		return std::make_shared<default_client_info>(request);
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

	const char* yn(bool b) { return b ? "yes;" : "no;"; }

	void MediaServer::add_renderer_conf(const boost::filesystem::path& conf)
	{
		auto ptr = net::config::base::file_config(conf);
		if (!ptr)
			return;

		net::config::renderer config(ptr);
		std::string name = config.general.name;
		std::string ua_match = config.recognize.ua_match;
		std::string additional_header = config.recognize.additional_header;
		std::string additional_header_match = config.recognize.additional_header_match;

		// if there is no name OR the info couldn't match anything
		if (name.empty() || (
			ua_match.empty() &&
			additional_header.empty() &&
			additional_header_match.empty()
			))
		{
			return;
		}

		auto client_info = std::make_shared<client>(name, ua_match, additional_header, additional_header_match);
		m_known_clients.push_back(client_info);

		// other attributes

		log::info() << "Client configuration for \"" << name << "\"";
#if 0
		log::info info;
		info << "UA: \"";
		info << ua_match;
		info << "\", other:(\"";
		info << additional_header;
		info << "\", \"";
		info << additional_header_match;
		info << "\")\n";
		if (config.basic_capabilities.video)
			info << "video ";
		if (config.basic_capabilities.audio)
			info << "audio ";
		if (config.basic_capabilities.image)
			info << "image ";
		if (!config.basic_capabilities.video && !config.basic_capabilities.audio && !config.basic_capabilities.image)
			info << "none";
		info << "\n";
		info << "seek by " << (config.media_server.seek_by_time ? "time;" : "pos;");

		if (config.media_server.protocol_localization)
			info << " localize;";

		if (config.media_server.send_ORG_PN)
			info << " ORG_PN;";
		info << " [";
		info << config.media_server.profile_patches;
		info << "]\n";
		info << "transcode:\n   ";
		if (config.transcode.audio)
			info << " audio;";
		if (config.transcode.video)
			info << " video;";
		if (config.transcode.max_h264_level_41)
			info << " h264@L41;";
		if (config.transcode.audio_441kHz)
			info << " 441kHz;";
		if (config.transcode.audio || config.transcode.video || config.transcode.max_h264_level_41 || config.transcode.audio_441kHz)
			info << "\n   ";
		if (config.transcode.fast_start)
			info << " fast start;";
		if (config.transcode.thumbnails_as_resource)
			info << " res thumbs;";
		if (config.transcode.force_jpg_thumbnails)
			info << " jpg thumbs;";
		if (config.transcode.allow_chunked_transfer)
			info << " can chunk;";
		if (config.transcode.exif_auto_rotate)
			info << " EXIF auto-rotate;";
		if (config.transcode.fast_start || config.transcode.thumbnails_as_resource || config.transcode.force_jpg_thumbnails || config.transcode.allow_chunked_transfer || config.transcode.exif_auto_rotate)
			info << "\n   ";
		info << " max bitrate (Mbps) ";
		info << config.transcode.max_video_bitrate_mbps;
		info << "; max video ";
		info << config.transcode.max_video_width;
		info << "x";
		info << config.transcode.max_video_height;
		info << "; video size ";
		info << config.transcode.video_size;
		info << "\n";
#endif
	}

	client_info_ptr MediaServer::match_from_request(const http::http_request& request) const
	{
		log::debug() << "[MATCHING] User agent: " << request.user_agent();
		for (auto&& candidate : m_known_clients)
			if (candidate->matches(request))
				return candidate;

		return create_default_client(request);
	}

}}}}
