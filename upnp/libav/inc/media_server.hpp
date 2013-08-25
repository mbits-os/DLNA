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
#ifndef __SSDP_MEDIA_SERVER_HPP__
#define __SSDP_MEDIA_SERVER_HPP__

#include <device.hpp>
#include <directory.hpp>
#include <manager.hpp>
#include <dlna_media.hpp>
#include <zlib.h>

#ifdef _MSC_VER
#pragma comment(lib, "libz.lib")
#endif

namespace zlib
{
	extern "C" uLong __stdcall crc32(uLong crc, const Bytef *buf, uInt len);
}

class CRC
{
	net::uint _val;
public:
	CRC() : _val(zlib::crc32(0L, nullptr, 0))
	{
	}
	CRC& update(const net::uchar *buf, size_t len)
	{
		_val = zlib::crc32(_val, buf, (uInt) len);
		return *this;
	}

	template <class T>
	CRC& update(const T& val)
	{
		return update((net::uchar*) (void*) &val, sizeof(T));
	}

	CRC& update(const std::string& str)
	{
		return update((const net::uchar*)str.c_str(), str.length());
	}

	operator net::uint() const { return _val; }

	std::string str()
	{
		char buffer[20];

#ifndef _MSC_VER
#define sprintf_s _snprintf
#endif
		sprintf_s(buffer, sizeof(buffer), "%08X", _val);

#ifndef _MSC_VER
#undef sprintf_s
#endif
		return buffer;
	}
};

namespace net { namespace ssdp { namespace import { namespace av {

	struct MediaServer;

	struct client_interface: client_info
	{
		client_interface(const std::string& name) : client_info(name) {}
		virtual ~client_interface() {}
	};
	typedef std::shared_ptr<client_interface> client_interface_ptr;

	struct client : client_interface
	{
		client(const std::string& name,
			const std::string& user_agent_match,
			const std::string& other_header,
			const std::string& other_header_match)
			: client_interface(name)
			, m_matcher(user_agent_match, other_header, other_header_match)
		{}

		virtual bool matches(const http::http_request& request) const
		{
			return m_matcher.matches(request);
		}
	private:
		client_matcher m_matcher;
	};
	typedef std::shared_ptr<client> client_ptr;

	namespace items
	{
		struct media;
		struct media_file;
		typedef std::shared_ptr<media> media_ptr;

		struct media
		{
			virtual ~media() {}
			virtual bool prep_response(http::response& /*resp*/) { return true; }
			virtual const dlna::Profile* profile() const { return nullptr; }
			static media_ptr from_file(const boost::filesystem::path& path, bool main_resource);
		};

		struct media_item;
		typedef std::shared_ptr<media_item> media_item_ptr;

		struct media_item
		{
			typedef items::media          media;
			typedef items::media_ptr      media_ptr;
			typedef media_item_ptr        item_ptr;
			typedef std::vector<item_ptr> container_type;

			MediaServer* m_device;
			explicit media_item(MediaServer* device) : m_device(device), m_id(0) {}
			virtual ~media_item() {}

			//enumeration
			virtual container_type list(ulong start_from, ulong max_count)          = 0;
			virtual ulong          predict_count(ulong served) const                = 0;
			virtual void           check_updates()                                  {}
			virtual ulong          update_id() const                                = 0;

			//navigation
			virtual media_item_ptr get_item(const std::string& id)                  = 0;
			virtual void           set_id(uint id)                                  { m_id = id; }
			virtual uint           get_id() const                                   { return m_id; }

			//output
			virtual bool           is_folder() const                                = 0;
			virtual bool           is_image() const                                 = 0;
			virtual void           output(std::ostream& o,
			                              const std::vector<std::string>& filter,
			                              const client_interface_ptr& client,
			                              const config::config_ptr& config) const   = 0;

			//attributes
			virtual void           set_objectId_attr(const std::string& object_id) { m_object_id = object_id; }
			virtual std::string    get_objectId_attr() const                       { return m_token.empty() ? m_object_id : m_object_id + "," + m_token; }
			virtual std::string    get_raw_objectId_attr() const                   { return m_object_id; }
			virtual void           set_token(const std::string& token)             { m_token = token; }
			virtual std::string    get_token() const                               { return m_token; }
			virtual void           set_parent_attr(const std::string& id)          { m_parent_id = id; }
			virtual std::string    get_parent_attr() const                         { return m_parent_id; }
			virtual void           set_title(const std::string& title)             { m_title = title; }
			virtual std::string    get_title() const                               { return m_title; }
			virtual const dlna::ItemMetadata* get_metadata() const                 { return nullptr; }
			virtual const dlna::ItemProperties* get_properties() const             { return nullptr; }
			virtual const dlna::Profile* get_profile() const                       { return nullptr; }

			//media
			virtual media_ptr      get_media(bool /*main_resource*/) const         { return nullptr; }

		private:
			uint m_id;
			std::string m_object_id;
			std::string m_parent_id;
			std::string m_token;
			std::string m_title;
			std::string m_mime;
		};

		static const char SEP = '.';
		static const ulong INVALID_ID = (ulong) - 1;

		struct root_item;
		typedef std::shared_ptr<root_item> root_item_ptr;

		std::pair<media_item_ptr, std::string> find_item(std::vector<media_item_ptr>& items, const std::string& id);

		struct common_props_item : media_item
		{
			common_props_item(MediaServer* device) : media_item(device) {}

			void output(std::ostream& o,
				const std::vector<std::string>& filter,
				const client_interface_ptr& client,
				const net::config::config_ptr& config) const  override;
			virtual const char* get_upnp_class() const        = 0;
			virtual size_t      child_count() const           = 0;
			virtual time_t      get_last_write_time() const   { return 0; }
		protected:
			void main_res(std::ostream& o, const std::vector<std::string>& filter, const client_interface_ptr& client, const config::config_ptr& config) const;
			void cover(std::ostream& o, const std::vector<std::string>& filter, const client_interface_ptr& client, const config::config_ptr& config) const;
		};
	}

	using namespace import::directory;
	using namespace import::manager;

	struct ContentDirectory: ContentDirectoryServerProxy
	{
		MediaServer* m_device;
		ContentDirectory(MediaServer* device) : m_device(device) {}

		error_code GetSystemUpdateID(const client_info_ptr& client, const http::http_request& http_request,
		                             ui4& Id) override;

		error_code Search(const client_info_ptr& client, const http::http_request& http_request,
		                  const std::string& ContainerID, const std::string& SearchCriteria,
		                  const std::string& Filter, ui4 StartingIndex, ui4 RequestedCount,
		                  const std::string& SortCriteria, std::string& Result, ui4& NumberReturned,
		                  ui4& TotalMatches, ui4& UpdateID) override;

		error_code GetSearchCapabilities(const client_info_ptr& client, const http::http_request& http_request,
		                                 std::string& SearchCaps) override;

		error_code GetSortCapabilities(const client_info_ptr& client, const http::http_request& http_request,
		                               std::string& SortCaps) override;

		error_code Browse(const client_info_ptr& client, const http::http_request& http_request,
		                  const std::string& ObjectID, A_ARG_TYPE_BrowseFlag BrowseFlag,
		                  const std::string& Filter, ui4 StartingIndex, ui4 RequestedCount,
		                  const std::string& SortCriteria, std::string& Result, ui4& NumberReturned,
		                  ui4& TotalMatches, ui4& UpdateID) override;
	};

	struct ConnectionManager: ConnectionManagerServerProxy
	{
		MediaServer* m_device;
		ConnectionManager(MediaServer* device) : m_device(device) {}

		error_code GetCurrentConnectionInfo(const client_info_ptr& client, const http::http_request& http_request,
		                                    i4 ConnectionID, i4& RcsID, i4& AVTransportID,
		                                    std::string& ProtocolInfo, std::string& PeerConnectionManager,
		                                    i4& PeerConnectionID, A_ARG_TYPE_Direction& Direction,
		                                    A_ARG_TYPE_ConnectionStatus& Status) override;

		error_code ConnectionComplete(const client_info_ptr& client, const http::http_request& http_request,
		                              i4 ConnectionID) override;

		error_code PrepareForConnection(const client_info_ptr& client, const http::http_request& http_request,
		                                const std::string& RemoteProtocolInfo, const std::string& PeerConnectionManager,
		                                i4 PeerConnectionID, A_ARG_TYPE_Direction Direction,
		                                i4& ConnectionID, i4& AVTransportID, i4& RcsID) override;

		error_code GetProtocolInfo(const client_info_ptr& client, const http::http_request& http_request,
		                           std::string& Source, std::string& Sink) override;

		error_code GetCurrentConnectionIDs(const client_info_ptr& client, const http::http_request& http_request,
		                                   std::string& ConnectionIDs) override;
	};

	struct MediaServer : Device
	{
		MediaServer(const device_info& info, const config::config_ptr& config)
			: Device(info, config)
			, m_root_item(create_root_item())
			, m_directory(std::make_shared<ContentDirectory>(this))
			, m_manager(std::make_shared<ConnectionManager>(this))
			, m_system_update_id(1)
		{
			add(m_directory);
			add(m_manager);
		}

		bool                  call_http(const http::http_request& req,
		                                const boost::filesystem::path& root,
		                                const boost::filesystem::path& rest,
		                                http::response& resp)   override;
		const char*           get_type() const                  override { return "urn:schemas-upnp-org:device:MediaServer:1"; }
		const char*           get_description() const           override { return "UPnP/AV 1.0 Compliant Media Server"; }
		ulong                 system_update_id() const                   { return (ulong)m_system_update_id; }
		items::media_item_ptr get_item(const std::string& id);
		void                  add_root_element(items::media_item_ptr);
		void                  remove_root_element(items::media_item_ptr);
		void                  object_changed();
		void                  add_renderer_conf(const boost::filesystem::path& conf);
		client_info_ptr       match_from_request(const http::http_request& request) const override;

	private:
		items::root_item_ptr               m_root_item;
		std::shared_ptr<ContentDirectory>  m_directory;
		std::shared_ptr<ConnectionManager> m_manager;
		time_t                             m_system_update_id;
		std::vector<client_ptr>            m_known_clients;

		static client_interface_ptr create_default_client(const http::http_request& request);
		items::root_item_ptr create_root_item();
	};

}}}} // net::ssdp::import::av

#endif //__SSDP_MEDIA_SERVER_HPP__
