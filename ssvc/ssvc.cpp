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
#ifdef _WIN32
#include <sdkddkver.h>
#endif

#include <dom.hpp>
#include <http/http.hpp>
#include <http/server.hpp>
#include <ssdp/ssdp.hpp>
#include <boost/filesystem.hpp>
#include <iostream>

namespace fs = boost::filesystem;

inline void push() {}

template <typename T, typename... Args>
inline void push(T && arg, Args && ... rest)
{
	std::cerr << arg;
	push(std::forward<Args>(rest)...);
}

template <typename... Args>
void help(Args&& ... args)
{
	std::cerr << "svcc 1.0 -- SSDP service compiler.\n";

	push(std::forward<Args>(args)...);
	if (sizeof...(args) > 0)
		std::cerr << "\n";

	std::cerr <<
		"\nusage:\n"
		"	ssvc INFILE OUTFILE\n"
		"e.g.\n"
		"	ssvc service.xml service.idl\n"
		"	ssvc service.idl service.hpp\n"
		;
}

inline bool is_file(const fs::path& p) { return fs::is_regular_file(p) || fs::is_symlink(p); }

struct state_variable
{
	bool m_event;
	std::string m_name;
	std::string m_type;
	std::vector<std::string> m_values;

	std::string getType() const { return m_values.empty() ? m_type : m_event ? m_name + "_Values" : m_name; }
};

struct action_arg
{
	bool m_input;
	std::string m_name;
	std::string m_type_ref;
	std::string getType(const std::vector<state_variable>& refs) const
	{
		for (auto&& ref : refs)
		{
			if (m_type_ref == ref.m_name)
				return ref.getType();
		}
		return m_type_ref;
	}
};

struct action
{
	std::string m_name;
	std::vector<action_arg> m_args;
};

inline std::string find_string(const dom::XmlNodePtr& node, const std::string& xpath)
{
	auto tmp = node->find(xpath);
	if (tmp) return tmp->stringValue();
	return std::string();
}

inline std::string find_string(const dom::XmlNodePtr& node, const std::string& xpath, dom::Namespaces ns)
{
	auto tmp = node->find(xpath, ns);
	if (tmp) return tmp->stringValue();
	return std::string();
}

int main(int argc, char* argv [])
{
	try
	{
		std::cout << fs::current_path().string() << std::endl;

		if (argc != 3)
		{
			help("File name missing.");
			return 1;
		}
		fs::path in(argv[1]);
		fs::path out(argv[2]);

		if (is_file(in) && is_file(out) && fs::last_write_time(in) < fs::last_write_time(out))
			return 0;

		fs::ifstream in_f(in);

		if (!in_f)
		{
			help("File `", in, "` could not be read.");
			return 1;
		}

		std::cout << in.filename().string() << std::endl;

		auto doc = dom::XmlDocument::fromDataSource([&](void* buffer, size_t size) { return (size_t)in_f.read((char*)buffer, size).gcount(); });
		if (doc)
		{
			std::vector<state_variable> types_variables;
			std::vector<action> actions;
			int spec_major = 0, spec_minor = 0;

			// try XML -> IDL
			dom::NSData ns[] = { {"svc", "urn:schemas-upnp-org:service-1-0"} };
			auto version = doc->find("/svc:scpd/svc:specVersion", ns);
			if (version)
			{
				auto major = find_string(version, "svc:major", ns);
				auto minor = find_string(version, "svc:minor", ns);
				{
					std::istringstream i(major);
					i >> spec_major;
				}
				{
					std::istringstream i(minor);
					i >> spec_minor;
				}
			}

			auto action_list = doc->findall("/svc:scpd/svc:actionList/svc:action", ns);
			for (auto&& act : action_list)
			{
				action action;
				action.m_name = find_string(act, "svc:name", ns);

				auto args = act->findall("svc:argumentList/svc:argument", ns);
				for (auto&& arg : args)
				{
					action_arg action_arg;
					action_arg.m_input = find_string(arg, "svc:direction", ns) == "out";
					action_arg.m_name = find_string(arg, "svc:name", ns);
					action_arg.m_type_ref = find_string(arg, "svc:relatedStateVariable", ns);
					action.m_args.push_back(action_arg);
				}
				actions.push_back(action);
			}

			auto variables = doc->findall("/svc:scpd/svc:serviceStateTable/svc:stateVariable", ns);
			for (auto&& variable : variables)
			{
				auto sendEvent = find_string(variable, "@sendEvents");

				state_variable var;
				var.m_event = sendEvent == "1" || sendEvent == "yes";
				var.m_name = find_string(variable, "svc:name", ns);
				var.m_type = find_string(variable, "svc:dataType", ns);

				auto allowedValues = variable->findall("svc:allowedValueList/svc:allowedValue", ns);
				for (auto&& value : allowedValues)
					var.m_values.push_back(value->stringValue());

				types_variables.push_back(var);
			}

			size_t events = 0;
			for (auto&& var : types_variables)
			{
				if (var.m_values.empty())
				{
					++events;
					continue;
				}

				std::cout << "enum " << var.getType() << " { // " << var.m_type << "\n";
				for (auto&& val : var.m_values)
				{
					std::cout << "    " << val << ";\n";
				}
				std::cout << "};\n\n";
			}

			std::cout << "[version(" << spec_major << "." << spec_minor << ")] interface <name missing> {\n";

			for (auto&& action : actions)
			{
				bool first = true;
				std::cout << "    void " << action.m_name << "(";

				size_t  out_count = 0;
				for (auto&& arg : action.m_args)
				{
					if (!arg.m_input)
						++out_count;
				}

				for (auto&& arg : action.m_args)
				{
					if (first) first = false;
					else std::cout << ", ";
					std::cout << (arg.m_input ? "[in] " : out_count == 1 ? "[retval] " : "[out] ") << arg.getType(types_variables) << " " << arg.m_name;
				}
				std::cout << ");\n";
			}

			if (!types_variables.empty() && events > 0)
				std::cout << "\n";

			for (auto && var : types_variables)
			{
				if (!var.m_event)
					continue;

				std::cout << "    ";
				std::cout << var.getType() << " " << var.m_name << ";\n";
			}

			std::cout << "};\n";
		}
		else
		{
			// try IDL -> HPP
		}
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
	return 0;
}
