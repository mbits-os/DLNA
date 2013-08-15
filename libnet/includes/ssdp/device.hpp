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

#include <http/http.hpp>
#include <config.hpp>
#include <log.hpp>

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

		struct ServiceInterface
		{
			virtual ~ServiceInterface() {}
			virtual const char* get_type() const = 0;
			virtual const char* get_id() const = 0;
			virtual bool answer(const std::string& name, const http::http_request& req, const dom::XmlDocumentPtr& doc, http::response& response) { return false; }
			virtual std::string get_configuration() const { return std::string(); };
		};
		typedef std::shared_ptr<ServiceInterface> service_ptr;

		struct Device : std::enable_shared_from_this<Device>
		{
			Device(const device_info& info, const config::config_ptr& config)
				: m_info(info)
				, m_config(config)
				, m_usn(get_usn(config))
			{
			}
			virtual ~Device() {}

			virtual const net::http::module_version& server() const { return m_info.m_server; }
			virtual const std::string& usn() const { return m_usn; }
			virtual const char* get_type() const = 0;
			virtual const char* get_description() const = 0;
			virtual size_t get_service_count() const { return m_services.size(); }
			virtual service_ptr get_service(size_t i) const { return m_services[i]; }
			virtual std::string get_configuration(const std::string& host) const;

			config::config_ptr config() const { return m_config; }
		protected:
			void add(const service_ptr& service)
			{
				if (!service)
					return;
				m_services.push_back(service);
			}

		private:
			config::config_ptr m_config;
			std::vector<service_ptr> m_services;
			const device_info m_info;
			const std::string m_usn;

			static std::string get_usn(const config::config_ptr& config)
			{
				if (config)
				{
					bool was_set = config->uuid.is_set();
					auto uuid = was_set ? config->uuid.val() : net::create_uuid();
					if (!was_set)
					{
						Log::line_stream(Log::Severity::Info, Log::Module::SSDP)
							<< "Creating new device identifier: " << uuid;
						config->uuid.val(uuid);
					}

					return "uuid:" + uuid;
				}
				return "uuid:" + net::create_uuid();
			}
		};
		typedef std::shared_ptr<Device> device_ptr;

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

		namespace error
		{
			enum error_code
			{
				no_error                               = 0,
				not_implemented,
				internal_error,
				invalid_action                         = 401,
				invalid_args                           = 402,
				action_failed                          = 501,
				argument_value_invalid                 = 600,
				argument_value_out_of_range            = 601,
				optional_action_not_implemented        = 602,
				out_of_memory                          = 603,
				human_intervention_required            = 604,
				string_argument_too_long               = 605,
				no_such_object                         = 701,
				unsupported_or_invalid_search_criteria = 708,
				unsupported_or_invalid_sort_criteria   = 709,
				no_such_container                      = 710,
				cannot_process_the_request             = 720,
			};
		}

		struct service_error : std::runtime_error
		{
			static inline const char* error_message(error::error_code code)
			{
				switch (code)
				{
				case error::invalid_action:                         return "Invalid Action";
				case error::invalid_args:                           return "Invalid Args";
				case error::action_failed:                          return "Action Failed";
				case error::argument_value_invalid:                 return "Argument Value Invalid";
				case error::argument_value_out_of_range:            return "Argument Value Out of Range";
				case error::optional_action_not_implemented:        return "Optional Action Not Implemented";
				case error::out_of_memory:                          return "Out of Memory";
				case error::human_intervention_required:            return "Human Intervention Required";
				case error::string_argument_too_long:               return "String Argument Too Long";
				case error::no_such_object:                         return "No such object";
				case error::unsupported_or_invalid_search_criteria: return "Unsupported or invalid search criteria";
				case error::unsupported_or_invalid_sort_criteria:   return "Unsupported or invalid sort criteria";
				case error::no_such_container:                      return "No such container";
				case error::cannot_process_the_request:             return "Cannot process the request";
				}
				return "";
			}

			service_error(error::error_code code)
				: std::runtime_error(error_message(code))
				, m_code(code)
			{}
			service_error(error::error_code code, const char* message)
				: std::runtime_error(message)
				, m_code(code)
			{}

			error::error_code code() const throw() { return m_code; }
			const char * message() const throw() { return std::runtime_error::what(); }
			const char * what() const throw()
			{
				if (m_what.empty())
				{
					try
					{
						const char* message = std::runtime_error::what();
						m_what = std::to_string(m_code);
						if (message && *message)
						{
							m_what += ": ";
							m_what += message;
						}
					}
					catch (...) { return std::runtime_error::what(); }
				}
				return m_what.c_str();
			}
		private:
			error::error_code   m_code;
			mutable std::string m_what;
		};
	}
}

#endif //__SSDP_DEVICE_HPP__
