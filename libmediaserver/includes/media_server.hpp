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

#include <ssdp/device.hpp>

namespace net
{
	namespace ssdp
	{
		namespace av
		{
			struct media_server;
			namespace service
			{
				struct service_helper : service_impl
				{
					media_server* m_device;
					service_helper(media_server* device)
						: m_device(device)
					{}

					void soap_answer(const char* method, http::response& response, const std::string& body);
					void upnp_error(http::response& response, const ssdp::service_error& e);
				};

#define SOAP_CALL(type, name) \
	void type ## name(const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response) \
	{ \
		try \
		{ \
			soap_answer(#name, response, soap_ ## type ## name(req, doc)); \
		} \
		catch (ssdp::service_error& e) \
		{ \
			upnp_error(response, e); \
		}; \
	} \
	\
	std::string soap_ ## type ## name(const http::http_request& req, const dom::XmlDocumentPtr& doc)

#define SOAP_CONTROL_CALL(name) SOAP_CALL(control_, name)
#define SOAP_EVENT_CALL(name) SOAP_CALL(event_, name)

				struct content_directory : service_helper
				{
					content_directory(media_server* device)
						: service_helper(device)
					{
						typedef content_directory service_t;
						SSDP_ADD_CONTROL(GetSortCapabilities);
						SSDP_ADD_CONTROL(GetSystemUpdateID);
						SSDP_ADD_CONTROL(Browse);
					}
					const char* get_type() const override { return "urn:schemas-upnp-org:service:ContentDirectory:1"; }
					const char* get_id() const override { return "urn:upnp-org:serviceId:ContentDirectory"; }
					const char* get_config() const override { return "directory.xml"; }
					const char* get_uri() const override { return "content_directory"; }

					SOAP_CONTROL_CALL(GetSortCapabilities);
					SOAP_CONTROL_CALL(GetSystemUpdateID);
					SOAP_CONTROL_CALL(Browse);
				};
				typedef std::shared_ptr<content_directory> content_directory_ptr;

				struct connection_manager : service_helper
				{
					connection_manager(media_server* device)
						: service_helper(device)
					{
						typedef connection_manager service_t;
						SSDP_ADD_CONTROL(GetProtocolInfo);
					}
					const char* get_type() const override { return "urn:schemas-upnp-org:service:ConnectionManager:1"; }
					const char* get_id() const override { return "urn:upnp-org:serviceId:ConnectionManager"; }
					const char* get_config() const override { return "manager.xml"; }
					const char* get_uri() const override { return "connection_manager"; }

					SOAP_CONTROL_CALL(GetProtocolInfo);
				};
				typedef std::shared_ptr<connection_manager> connection_manager_ptr;
			}

			struct sort_criterion
			{
				bool m_ascending;
				std::string m_term;
			};
			typedef std::vector<sort_criterion> search_criteria;

			struct media_item;
			typedef std::shared_ptr<media_item> media_item_ptr;

			struct media_item
			{
				media_item() : m_id(0) {}
				virtual ~media_item() {}
				virtual std::vector<media_item_ptr> list(ulong start_from, ulong max_count, const search_criteria& sort) = 0;
				virtual ulong predict_count(ulong served) const = 0;
				virtual void check_updates() {}
				virtual ulong update_id() const = 0;
				virtual media_item_ptr get_item(const std::string& id) = 0;
				virtual void set_id(uint id) { m_id = id; }
				virtual uint get_id() const { return m_id; }
				virtual bool is_folder() const = 0;
				virtual void output(std::ostream& o, const std::vector<std::string>& filter) const = 0;

				virtual void set_objectId_attr(const std::string& object_id) { m_object_id = object_id; }
				virtual std::string get_objectId_attr() const { return m_object_id; }
				virtual std::string get_parent_attr() const;

				virtual void set_title(const std::string& title) { m_title = title; }
				virtual std::string get_title() const { return m_title; }

			private:
				uint m_id;
				std::string m_object_id;
				std::string m_title;
			};

			namespace items
			{
				static const char SEP = '-';
				static const ulong INVALID_ID = (ulong) - 1;

				struct root_item;
				typedef std::shared_ptr<root_item> root_item_ptr;

				std::pair<media_item_ptr, std::string> find_item(std::vector<media_item_ptr>& items, const std::string& id);
			}

			struct media_server : device
			{
				media_server(const device_info& info)
					: device(info)
					, m_root_item(create_root_item())
					, m_directory(std::make_shared<service::content_directory>(this))
					, m_manager(std::make_shared<service::connection_manager>(this))
					, m_system_update_id(1)
				{
					add(m_directory);
					add(m_manager);
				}

				const char* get_type() const override { return "urn:schemas-upnp-org:device:MediaServer:1"; }
				const char* get_description() const override { return "UPnP/AV 1.0 Compliant Media Server"; }

				ulong system_update_id() const { return m_system_update_id; }

				media_item_ptr get_item(const std::string& id);
				void add_root_element(media_item_ptr);
				void remove_root_element(media_item_ptr);

			private:
				items::root_item_ptr m_root_item;
				service::content_directory_ptr m_directory;
				service::connection_manager_ptr m_manager;
				ulong m_system_update_id;

				items::root_item_ptr create_root_item();
			};

			namespace items
			{
				struct common_props_item : media_item
				{
					virtual const char* get_upnp_class() const = 0;
					virtual time_t get_last_write_time() const { return 0; }
				protected:
					void output_open(std::ostream& o, const std::vector<std::string>& filter, ulong child_count) const;
					void output_close(std::ostream& o, const std::vector<std::string>& filter) const;
				};

				struct common_item : common_props_item
				{
					std::vector<media_item_ptr> list(ulong start_from, ulong max_count, const search_criteria& sort) override { return std::vector<media_item_ptr>(); }
					ulong predict_count(ulong served) const override { return served; }
					ulong update_id() const override { return 0; }
					media_item_ptr get_item(const std::string& id) override { return nullptr; }
					bool is_folder() const override { return false; }
				};

				struct photo_item : common_item
				{
					const char* get_upnp_class() const override { return "object.item.imageItem.photo"; }
				};

				struct video_item : common_item
				{
					const char* get_upnp_class() const override { return "object.item.videoItem"; }
				};

				struct audio_item : common_item
				{
					const char* get_upnp_class() const override { return "object.item.audioItem.musicTrack"; }
				};

				struct container_item : common_props_item
				{
					container_item()
						: m_update_id(1)
						, m_current_max(0)
					{
					}

					std::vector<media_item_ptr> list(ulong start_from, ulong max_count, const search_criteria& sort) override;
					ulong predict_count(ulong served) const override { return m_children.size(); }
					ulong update_id() const override { return m_update_id; }
					media_item_ptr get_item(const std::string& id) override;
					bool is_folder() const override { return true; }
					void output(std::ostream& o, const std::vector<std::string>& filter) const override;
					const char* get_upnp_class() const override { return "object.container.storageFolder"; }

					virtual void rescan_if_needed() {}
					virtual void folder_changed() { m_update_id++; /*notify?*/ }
					virtual void add_child(media_item_ptr);
					virtual void remove_child(media_item_ptr);

				private:
					ulong m_current_max;
					ulong m_update_id;
					std::vector<media_item_ptr> m_children;
				};
			}
		}
	}
}

#endif //__SSDP_MEDIA_SERVER_HPP__
