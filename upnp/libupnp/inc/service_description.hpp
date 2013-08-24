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

#ifndef __SSDP_SERVICE_DESCRIPTION_HPP_
#define __SSDP_SERVICE_DESCRIPTION_HPP_

#include <dom.hpp>
#include <iostream>
#include <vector>
#include <sstream>

namespace net
{
	namespace ssdp
	{
		struct xml_helper
		{
			inline static const dom::Namespaces svc()
			{
				static dom::NSData ns[] = { {"svc", "urn:schemas-upnp-org:service-1-0"} };
				return ns;
			}

			inline static std::string find_string(const dom::XmlNodePtr& node, const std::string& xpath, const dom::Namespaces ns)
			{
				auto tmp = node->find(xpath, ns);
				if (tmp) return tmp->stringValue();
				return std::string();
			}

			inline static std::string find_string(const dom::XmlNodePtr& node, const std::string& xpath)
			{
				auto tmp = node->find(xpath);
				if (tmp) return tmp->stringValue();
				return std::string();
			}
		};

		struct state_variable : private xml_helper
		{
			bool m_event;
			bool m_referenced;
			std::string m_name;
			std::string m_type;
			std::vector<std::string> m_values;

			std::string getType() const { return m_values.empty() ? m_type : m_event ? m_name + "_Values" : m_name; }
			std::string getCType(bool input) const
			{
				if (input)
				{
					if (m_values.empty())
					{
						if (m_type == "string") return "const std::string&";
						if (m_type == "bin.base64") return "const base64&";
						return m_type;
					}
					else
						return m_event ? m_name + "_Values" : m_name;
				}
				else
				{
					if (m_values.empty())
					{
						if (m_type == "string") return "std::string&";
						if (m_type == "bin.base64") return "base64&";
						return m_type + "&";
					}
					else
						return (m_event ? m_name + "_Values" : m_name) + "&";
				}
			}
			std::string getCType() const
			{
				if (m_values.empty())
				{
					if (m_type == "string") return "std::string";
					if (m_type == "bin.base64") return "base64";
					return m_type;
				}
				else
					return m_event ? m_name + "_Values" : m_name;
			}

			bool read_xml(const dom::XmlNodePtr& variable)
			{
				auto sendEvent = find_string(variable, "@sendEvents");

				m_event = sendEvent == "1" || sendEvent == "yes";
				m_name = find_string(variable, "svc:name", svc());
				m_type = find_string(variable, "svc:dataType", svc());
				m_referenced = false;

				auto allowedValues = variable->findall("svc:allowedValueList/svc:allowedValue", svc());
				for (auto&& value : allowedValues)
				{
					auto val = value->stringValue();
					if (val.empty())
						return false;
					m_values.emplace_back(std::move(val));
				}

				if (m_name.empty() || m_type.empty())
					return false;

				return true;
			}

			static bool read_xml(std::vector<state_variable>& out, const dom::XmlDocumentPtr& doc)
			{
				out.clear();
				auto list = doc->find("/svc:scpd/svc:serviceStateTable", svc());
				if (!list)
					return false;

				auto var_list = list->findall("svc:stateVariable", svc());
				size_t count = var_list ? var_list->length() : 0;
				out.reserve(count);

				for (auto && var : var_list)
				{
					state_variable dst;
					if (!dst.read_xml(var))
						return false;
					out.push_back(dst);
				}

				return true;
			}
		};

		struct action_arg : private xml_helper
		{
			bool m_input;
			std::string m_name;
			std::string m_type_ref;
			std::string getType(const std::vector<state_variable>& refs) const
			{
				for (auto && ref : refs)
				{
					if (m_type_ref == ref.m_name)
						return ref.getType();
				}
				return m_type_ref;
			}
			std::string getCType(const std::vector<state_variable>& refs, bool input) const
			{
				for (auto && ref : refs)
				{
					if (m_type_ref == ref.m_name)
						return ref.getCType(input);
				}
				if (input)
					return m_type_ref;
				return m_type_ref + "&";
			}
			std::string getCType(const std::vector<state_variable>& refs) const
			{
				for (auto && ref : refs)
				{
					if (m_type_ref == ref.m_name)
						return ref.getCType();
				}
				return m_type_ref;
			}

			void mark_ref(std::vector<state_variable>& refs)
			{
				for (auto && ref : refs)
				{
					if (m_type_ref == ref.m_name)
					{
						ref.m_referenced = true;
						return;
					}
				}

			}
		};

		struct action : private xml_helper
		{
			std::string m_name;
			std::vector<action_arg> m_args;

			void mark_refs(std::vector<state_variable>& refs)
			{
				for (auto && arg : m_args)
					arg.mark_ref(refs);
			}

			bool read_xml(const dom::XmlNodePtr& action)
			{
				m_name = std::move(find_string(action, "svc:name", svc()));
				if (m_name.empty())
					return false;

				auto args = action->findall("svc:argumentList/svc:argument", svc());
				for (auto&& arg : args)
				{
					action_arg action_arg;

					action_arg.m_name     = find_string(arg, "svc:name", svc());
					action_arg.m_type_ref = find_string(arg, "svc:relatedStateVariable", svc());
					auto direction        = find_string(arg, "svc:direction", svc());
					action_arg.m_input    = direction == "in";

					if (direction != "out" && direction != "in")
						return false;

					if (action_arg.m_name.empty() || action_arg.m_type_ref.empty())
						return false;

					m_args.push_back(action_arg);
				}
				return true;
			}

			static bool read_xml(std::vector<action>& out, const dom::XmlDocumentPtr& doc)
			{
				out.clear();
				auto list = doc->find("/svc:scpd/svc:actionList", svc());
				if (!list)
					return false;

				auto action_list = list->findall("svc:action", svc());
				size_t count = action_list ? action_list->length() : 0;
				out.reserve(count);

				for (auto&& act : action_list)
				{
					action dst;
					if (!dst.read_xml(act))
						return false;
					out.push_back(dst);
				}

				return true;
			}
		};

		struct spec_version : private xml_helper
		{
			int m_major;
			int m_minor;

			bool read_xml(const dom::XmlDocumentPtr& doc)
			{
				auto version = doc->find("/svc:scpd/svc:specVersion", svc());
				if (!version)
					return false;

				auto major = find_string(version, "svc:major", svc());
				auto minor = find_string(version, "svc:minor", svc());
				if (major.empty() || minor.empty())
					return false;

				{
					std::istringstream i(major);
					i >> m_major;
				}
				{
					std::istringstream i(minor);
					i >> m_minor;
				}

				return true;
			}
		};

		struct service_description : private xml_helper
		{
			spec_version m_version;
			std::vector<action> m_actions;
			std::vector<state_variable> m_variables;

			bool read_xml(const dom::XmlDocumentPtr& doc)
			{
				if (!m_version.read_xml(doc))
					return false;

				if (!action::read_xml(m_actions, doc))
					return false;

				if (!state_variable::read_xml(m_variables, doc))
					return false;

				for (auto& action : m_actions)
					action.mark_refs(m_variables);

				return true;
			}
		};
	}
}

#endif // __SSDP_SERVICE_DESCRIPTION_HPP_