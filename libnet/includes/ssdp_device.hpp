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
#ifndef __SSDP_DEVICE_HPP__
#define __SSDP_DEVICE_HPP__

#include <http.hpp>

namespace dom
{
	struct XmlDocument;
	typedef std::shared_ptr<XmlDocument> XmlDocumentPtr;
}

namespace net
{
	namespace http
	{
		class response;
	}

	namespace ssdp
	{
		struct device_info
		{
			net::http::module_version m_server;

			struct model_info
			{
				std::string m_name;
				std::string m_friendly_name;
				std::string m_number;
				std::string m_url;
			} m_model;

			struct manufacturer_info
			{
				std::string m_name;
				std::string m_url;
			} m_manufacturer;
		};

		struct service
		{
			virtual ~service() {}
			virtual const char* get_type() const = 0;
			virtual const char* get_uri() const = 0;
			virtual bool control_call_by_name(const std::string& name, const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response)
			{
				return call_by_name(controls, name, req, doc, response);
			}
			virtual bool event_call_by_name(const std::string& name, const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response)
			{
				return call_by_name(events, name, req, doc, response);
			}
		protected:
			template <typename Klass>
			void add_control(const char* name, void (Klass::* method)(const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response)) { controls.emplace_back(name, make_call(method)); }
			template <typename Klass>
			void add_event(const char* name, void (Klass::* method)(const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response)) { events.emplace_back(name, make_call(method)); }
			template <typename Klass>
			void add_control(const char* name, void (*function)(Klass*, const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response)) { controls.emplace_back(name, make_call(function)); }
			template <typename Klass>
			void add_event(const char* name, void (*function)(Klass*, const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response)) { events.emplace_back(name, make_call(function)); }

		private:
			typedef std::function<void (service*, const http::http_request&, const dom::XmlDocumentPtr&, http::response&)> call;
			typedef std::vector<std::pair<const char*, call>> events_t;
			events_t controls;
			events_t events;

			template <typename Klass>
			call make_call(void (Klass::* method)(const http::http_request&, const dom::XmlDocumentPtr&, http::response&))
			{
				return [method](service* self, const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response)
				{
					(static_cast<Klass*>(self)->*method)(req, doc, response);
				};
			}

			template <typename Klass>
			call make_call(void (*function) (Klass* self, const http::http_request&, const dom::XmlDocumentPtr&, http::response&))
			{
				return [function](service* self, const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response)
				{
					(*function)(static_cast<Klass*>(self), req, doc, response);
				};
			}

			virtual bool call_by_name(events_t& collection, const std::string& name, const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response)
			{
				auto call = by_name(collection, name);
				if (!call)
					return false;

				call(this, req, doc, response);
				return true;
			}

			call by_name(events_t& collection, const std::string& name)
			{
				for (auto&& call: collection)
				{
					if (name == call.first)
						return call.second;
				}
				return nullptr;
			}
		};
		typedef std::shared_ptr<service> service_ptr;

#define SSDP_ADD_CONTROL(name) add_control(#name, &service_t::control_##name);
#define SSDP_ADD_EVENT(name) add_event(#name, &service_t::event_##name);
#define SSDP_ADD_CONTROL_F(name) add_control<service_t>(#name, control_##name);
#define SSDP_ADD_EVENT_F(name) add_event<service_t>(#name, event_##name);

		struct device
		{
			device(const device_info& info)
				: m_info(info)
				, m_usn("uuid:" + net::create_uuid())
			{
			}
			virtual ~device() {}

			virtual const net::http::module_version& server() const { return m_info.m_server; }
			virtual const std::string& usn() const { return m_usn; }
			virtual const char* get_type() const = 0;
			virtual size_t get_service_count() const { return m_services.size(); }
			virtual service_ptr get_service(size_t i) const { return m_services[i]; }
		protected:
			void add(const service_ptr& service)
			{
				if (!service)
					return;
				m_services.push_back(service);
			}

		private:
			std::vector<service_ptr> m_services;
			const device_info m_info;
			const std::string m_usn;
		};
		typedef std::shared_ptr<device> device_ptr;
		
		struct services
		{
			struct const_iterator
			{
				device_ptr m_device;
				size_t m_pos;
				const_iterator(const device_ptr& device, size_t pos)
					: m_device(device)
					, m_pos(pos)
				{}

				service_ptr operator* () { return m_device->get_service(m_pos); }

				const_iterator& operator++()
				{
					++m_pos;
					return *this;
				}
				const_iterator operator++(int)
				{
					const_iterator tmp(m_device, m_pos);
					m_pos++;
					return tmp;
				}

				bool operator == (const const_iterator& rhs) const
				{
					return m_pos == rhs.m_pos;
				}
				bool operator != (const const_iterator& rhs) const
				{
					return m_pos != rhs.m_pos;
				}
			};

			device_ptr m_device;
			services(const device_ptr& device) : m_device(device) {}
			const_iterator begin() const { return const_iterator(m_device, 0); }
			const_iterator end() const { return const_iterator(m_device, m_device->get_service_count()); }
		};

	}
}

#endif //__SSDP_DEVICE_HPP__
