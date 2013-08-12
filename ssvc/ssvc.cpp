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
#include <ssdp/service_description.hpp>

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

void print_idl(std::ostream& o, const net::ssdp::service_description& descr, const std::string& type_name)
{
	size_t events = 0;
	for (auto&& var : descr.m_variables)
	{
		if (var.m_values.empty())
		{
			++events;
			continue;
		}

		o << "enum " << var.getType() << " { // " << var.m_type << "\n";
		for (auto&& val : var.m_values)
		{
			o << "    " << val << ";\n";
		}
		o << "};\n\n";
	}

	o << "[version(" << descr.m_version.m_major << "." << descr.m_version.m_minor << ")] interface " << type_name << " {\n";

	for (auto&& action : descr.m_actions)
	{
		bool first = true;
		o << "    void " << action.m_name << "(";

		size_t  out_count = 0;
		for (auto&& arg : action.m_args)
		{
			if (!arg.m_input)
				++out_count;
		}

		for (auto&& arg : action.m_args)
		{
			if (first) first = false;
			else o << ", ";
			o << (arg.m_input ? "[in] " : out_count == 1 ? "[retval] " : "[out] ") << arg.getType(descr.m_variables) << " " << arg.m_name;
		}
		o << ");\n";
	}

	if (!descr.m_variables.empty() && events > 0)
		o << "\n";

	for (auto&& var : descr.m_variables)
	{
		if (!var.m_event)
			continue;

		o << "    ";
		o << var.getType() << " " << var.m_name << ";\n";
	}

	o << "};\n";

}

int main(int argc, char* argv [])
{
	try
	{
		std::cout << fs::current_path().string() << std::endl;

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
			net::ssdp::service_description descr;

			if (!descr.read_xml(doc))
			{
				help("The XML document does not describe SSDP service.");
				return 1;
			}

			auto stem = in.stem().string();
			print_idl(std::cout, descr, stem + "_service");
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
