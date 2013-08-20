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

#include "printer.hpp"

#include <dom.hpp>
#include <http/http.hpp>
#include <http/server.hpp>
#include <ssdp.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <service_description.hpp>

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
		"	ssvc service.xml service_definition.hpp\n"
		"\n"
		"Will also generate service_definition.ipp and service_definition_example.cpp\n";
}

inline bool is_file(const fs::path& p) { return fs::is_regular_file(p) || fs::is_symlink(p); }

int main(int argc, char* argv [])
{
	try
	{
		if (argc != 3)
		{
			if (argc < 3)
				help("File name missing.");
			else
				help("Too much arguments.");
			return 1;
		}
		fs::path in(argv[1]);
		fs::path out(argv[2]);

		fs::ifstream in_f(in);

		if (!in_f)
		{
			help("File `", in.string(), "` could not be read.");
			return 1;
		}

		std::cout << in.filename().string() << std::endl;

		auto doc = dom::XmlDocument::fromDataSource([&](void* buffer, size_t size) { return (size_t) in_f.read((char*) buffer, size).gcount(); });

		if (!doc)
		{
			help("File `", in.string(), "` is not an XML.");
			return 1;
		}

		net::ssdp::service_description descr;
		if (!descr.read_xml(doc))
		{
			help("The XML document does not describe SSDP service.");
			return 1;
		}

		std::string class_name;
		std::string class_type;
		std::string class_id;

		{
			static dom::NSData ns [] = { {"svc", "urn:schemas-upnp-org:service-1-0"}, { "idl", "urn:schemas-mbits-com:idl-info" } };
			auto info = doc->find("/svc:scpd/idl:info", ns);
			if (info)
			{
				auto name = info->find("idl:name", ns);
				auto type = info->find("idl:type", ns);
				auto id = info->find("idl:id", ns);

				if (name)
					class_name = name->stringValue();
				if (type)
					class_type = type->stringValue();
				if (id)
					class_id = id->stringValue();
			}
		}

		if (class_name.empty())
		{
			std::cerr << "Warning: Service name missing, inventing `Service`...\n";
			class_name = "Service";
		}
		if (class_type.empty())
			class_type = "urn:schemas-upnp-org:service:" + class_name + ":1";
		if (class_id.empty())
			class_id = "urn:upnp-org:serviceId:" + class_name;

		printer print(descr, out, class_name, class_type, class_id);
		print.print_interface();
		print.print_proxy();
		print.print_implementation();
	}
	catch (std::exception& e)
	{
		std::cerr << "Exception: " << e.what() << "\n";
	}
	return 0;
}
