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
#include <http_handler.hpp>
#include <http/response.hpp>
#include <regex>
#include <interface.hpp>
#include <dom.hpp>
#include <log.hpp>

namespace net
{
	namespace http
	{
		struct log : public Log::basic_log<log>
		{
			static const Log::Module& module() { return Log::Module::HTTP; }
		};

		dom::XmlDocumentPtr create_from_socket(request_data_ptr data)
		{
			if (!data || !data->content_length())
				return nullptr;

			size_t rest = data->content_length();
			return dom::XmlDocument::fromDataSource([&](void* buffer, size_t chunk) -> size_t
			{
				if (!rest)
					return 0;

				if (chunk > rest)
					chunk = rest;

				auto tmp = data->read(buffer, chunk);
				rest -= tmp;

				return tmp;
			});
		}

		struct tmplt_chunk
		{
			template_vars::const_iterator m_var;
			const char* m_start;
			size_t m_size;
			tmplt_chunk(template_vars::const_iterator var, const char* start, size_t size)
				: m_var(var)
				, m_start(start)
				, m_size(size)
			{}
		};

		class template_content : public content
		{
			const char* m_tmplt;
			const template_vars& m_vars;
			std::vector<tmplt_chunk> m_chunks;
			std::vector<tmplt_chunk>::const_iterator m_cur;
			size_t m_ptr;
			template_vars::const_iterator find(const std::string& key)
			{
				auto cur = m_vars.begin(), end = m_vars.end();
				for (; cur != end; ++cur)
					if (cur->first == key)
						return cur;
				return cur;
			}
		public:
			template_content(const char* tmplt, const template_vars& vars)
				: m_tmplt(tmplt)
				, m_vars(vars)
			{
				while (*tmplt)
				{
					auto start = tmplt;
					while (*tmplt && *tmplt != '$') ++tmplt;

					auto var = *tmplt ? tmplt + 1 : tmplt;
					auto var_end = var;
					while (*var_end && std::isalpha((unsigned char) *var_end)) ++var_end;

					auto _var = find({var, var_end});

					m_chunks.emplace_back(_var, start, tmplt - start);

					tmplt = var_end;
				}

				m_cur = m_chunks.begin();
				m_ptr = 0;
			}

			bool can_skip() override { return false; }
			bool size_known() override { return false; }
			std::size_t get_size() override { return 0; }
			std::size_t skip(std::size_t) { return 0; }
			std::size_t read(void* buffer, std::size_t size)
			{
				if (m_cur == m_chunks.end())
					return 0;

				size_t _read = 0;
				while (size > 0)
				{
					if (m_ptr < m_cur->m_size)
					{
						auto rest = m_cur->m_size - m_ptr;
						if (rest > size)
							rest = size;

						memcpy((char*) buffer + _read, m_cur->m_start + m_ptr, rest);
						_read += rest;
						size -= rest;
						m_ptr += rest;
					}

					if (m_ptr >= m_cur->m_size && size > 0)
					{
						if (m_cur->m_var != m_vars.end())
						{
							auto ptr = m_ptr - m_cur->m_size;
							auto rest = m_cur->m_var->second.length() - ptr;

							if (rest > size)
								rest = size;

							memcpy((char*) buffer + _read, m_cur->m_var->second.c_str() + ptr, rest);
							_read += rest;
							size -= rest;
							m_ptr += rest;
						}
					}

					size_t whole = m_cur->m_size;
					if (m_cur->m_var != m_vars.end())
						whole += m_cur->m_var->second.length();
					if (m_ptr >= whole)
					{
						++m_cur;
						m_ptr = 0;
						if (m_cur == m_chunks.end())
							break;
					}
				}
				return _read;
			}
		};

		http_handler::http_handler(const ssdp::device_ptr& device, const config::config_ptr& config)
			: m_device(device)
			, m_config(config)
		{
			m_vars.emplace_back("host", to_string(config->iface));
			m_vars.emplace_back("port", std::to_string(config->port));
			m_vars.emplace_back("uuid", m_device->usn());
		}

		struct log_request
		{
			response& resp;
			const http_request& header;
			const std::string& SOAPAction;
			dom::XmlDocumentPtr doc;
			ssdp::client_info_ptr _client;
			bool with_header;
			bool printed;

			log_request(response& resp, const http_request& header, const std::string& SOAPAction, dom::XmlDocumentPtr& doc)
				: resp(resp)
				, header(header)
				, SOAPAction(SOAPAction)
				, doc(doc)
				, with_header(false)
				, printed(false)
			{
			}

			~log_request()
			{
				print(true);
				if (with_header)
				{
					resp.complete_header();
					log::info() << resp.header();
				}
			}

			log_request& client(const ssdp::client_info_ptr& cli) { _client = cli; return *this; }
			log_request& withHeader() { with_header = true; return *this; }
			log_request& print(bool finished = false)
			{
				if (printed)
					return *this;
				printed = true;

				log::debug dbg;
				log::info info;

				info << to_string(header.m_remote_address);
				if (_client)
					info << " [" << _client->get_name() << "]";
				info << " \"" << header.m_method << " " << header.m_resource << " " << header.m_protocol << "\"";

				if (!SOAPAction.empty())
					info << " \"" << SOAPAction << "\"";

				if (finished)
					info << " " << resp.header().m_status;
				else
					info << " -";

				auto ua = header.user_agent();
				if (!ua.empty())
				{
					dbg << ua;
					auto pui = header.simple("x-av-physical-unit-info");
					auto ci = header.simple("x-av-client-info");
					if (!pui.empty() || !ci.empty())
					{
						dbg << " | " << pui;
						if (!pui.empty() && !ci.empty())
							dbg << " | ";
						dbg << ci;
					}
				}

				if (with_header)
					info << "\n" << header;

				return *this;
			}
		private:
			log_request(const log_request&);
			log_request& operator=(const log_request&);
		};

		std::pair<fs::path, fs::path> pop(const fs::path& p)
		{
			if (p.empty())
				return std::make_pair(p, p);
			auto it = p.begin();
			auto root = *it++;
			fs::path rest;
			while (it != p.end())
				rest /= *it++;

			return std::make_pair(root, rest);
		}

		std::pair<std::string, std::string> break_action(const std::string& action)
		{
			auto hash = action.find('#');

			// no hash - no function; no function - no sense
			if (hash == std::string::npos)
				return std::make_pair(std::string(), std::string());

			return std::make_pair(action.substr(0, hash), action.substr(hash + 1));
		}

		void http_handler::handle(const http_request& req, response& resp)
		{
			auto SOAPAction = req.SOAPAction();
			auto res = req.resource();
			auto method = req.method();

			dom::XmlDocumentPtr doc;

			if (method == http_method::post && !SOAPAction.empty())
				doc = create_from_socket(req.request_data());

			//log_request __{ resp, req, SOAPAction, doc };
			//__.client(client_from_request(req, false));
			//__.withHeader().print();

			if (req.expecting_continue())
			{
				auto & header = resp.header();
				header.clear(m_device->server());
				header.m_status = 100;
				return;
			}

			fs::path root, rest;
			std::tie(root, rest) = pop(res); // pop leading slash
			std::tie(root, rest) = pop(rest);

			if (method == http_method::get || method == http_method::head)
			{
				if (root == "config")
				{
					auto client = client_from_request(req, false);
					if (rest == "device.xml")
						return make_device_xml(client, resp);
					else if (rest.string().substr(0, 7) == "service")
					{
						auto id = rest.string().substr(7);

						size_t int_id = 0;
						for (auto&& service: ssdp::services(m_device))
						{
							if (id == std::to_string(int_id++))
								return make_service_xml(client, resp, service);
						}
					}
				}
				if (root == "images")
					return make_file(boost::filesystem::path("data") / root / rest, resp);

				if (root == "upnp")
				{
					//__.withHeader().print();

					auto client = client_from_request(req);
					std::tie(root, rest) = pop(rest);
					if (m_device->call_http(req, root, rest, resp))
						return;
				}
			}

			if (method == http_method::post)
			{
				if (root == "upnp")
				{
					auto client = client_from_request(req);
					std::string soap_type, soap_method;
					std::tie(soap_type, soap_method) = break_action(SOAPAction);

					ssdp::service_ptr service;
					size_t int_id = 0;
					for (auto&& candidate: ssdp::services(m_device))
					{
						if (soap_type == candidate->get_type())
						{
							service = candidate;
							break;
						}
						++int_id;
					}

					if (!service)
					{
						log::warning() << "Unknown service requested: " << soap_type;
						return make_404(resp);
					}

					std::tie(root, rest) = pop(rest);
					if (root == "control")
					{
						if (rest == "service" + std::to_string(int_id)) try
						{
							resp.header().clear(m_device->server());
							if (service->answer(soap_method, client, req, doc, resp, m_device->server()))
								return;
							log::warning() << "Unimplemented SOAP method called: " << SOAPAction;
						}
						catch (std::exception& e)
						{
							log::error() << "Exception: " << e.what();
							return make_500(resp);
						}
						catch (...) { return make_500(resp); }
					}
				}
			}

			make_404(resp);
		}

		void http_handler::make_templated(const char* tmplt, const char* content_type, response& resp)
		{
			auto & header = resp.header();
			header.clear(m_device->server());
			header.append("content-type", content_type);
			resp.content(std::make_shared<template_content>(tmplt, std::ref(m_vars)));
		}

		void http_handler::make_device_xml(const ssdp::client_info_ptr& client, response& resp)
		{
			auto & header = resp.header();
			header.clear(m_device->server());
			header.append("content-type", "text/xml; charset=\"utf-8\"");
			resp.content(content::from_string(m_device->get_configuration(client, to_string(m_config->iface) + ":" + std::to_string(m_config->port))));
		}

		void http_handler::make_service_xml(const ssdp::client_info_ptr& client, response& resp, const ssdp::service_ptr& service)
		{
			auto & header = resp.header();
			header.clear(m_device->server());
			header.append("content-type", "text/xml; charset=\"utf-8\"");
			resp.content(content::from_string(service->get_configuration(client)));
		}

		static struct
		{
			std::string ext;
			const char* mime_type;
		} s_extensions [] = {
			{ ".xml", "text/xml" },
			{ ".png", "image/png" }
		};

		void http_handler::make_file(const fs::path& path, response& resp)
		{
			if (!fs::exists(path))
				return make_404(resp);

			const char* content_type = "text/html";
			if (path.has_extension())
			{
				std::string cmp = path.extension().string();
				for (auto&& c : cmp) c = (char) std::tolower((unsigned char) c);

				for (auto&& ext : s_extensions)
					if (ext.ext == cmp)
					{
						content_type = ext.mime_type;
						break;
					}
			}

			auto & header = resp.header();
			header.clear(m_device->server());
			header.append("content-type", content_type);
			header.append("last-modified")->out() << to_string(time::last_write(path));
			resp.content(content::from_file(path));
		}

		void http_handler::make_404(response& resp)
		{
			auto & header = resp.header();
			header.clear(m_device->server());
			header.m_status = 404;
			header.append("content-type", "text/plain");
			resp.content(content::from_string("File not found...\n"));
		}

		void http_handler::make_500(response& resp)
		{
			auto & header = resp.header();
			header.clear(m_device->server());
			header.m_status = 500;
		}

		ssdp::client_info_ptr http_handler::client_from_request(const http_request& req, bool save)
		{
			for (auto&& seen : m_clients_seen)
			{
				if (seen.m_address == req.m_remote_address && seen.m_client->matches(req))
					return seen.m_client;
			}

			auto client = m_device->match_from_request(req);
			if (client && save)
			{
				log::info() << "New client at " << to_string(req.m_remote_address) <<": " << client->get_name();
				m_clients_seen.emplace_back(req.m_remote_address, client);
				if (!client->from_config())
				{
					log::info() << req;
				}
				else
					std::cout << to_string(req.m_remote_address) << ": " << client->get_name() << std::endl;
			}
			return client;
		}
	}
}
