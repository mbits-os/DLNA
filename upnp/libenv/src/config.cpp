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
#include <log.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/any.hpp>
#include <map>
#include <regex>
#include <config.hpp>

namespace fs = boost::filesystem;

namespace net
{
	namespace config
	{
		namespace file
		{
			struct config;
			typedef std::weak_ptr<config> config_ptr;
			typedef std::shared_ptr<config> shared_config_ptr;

			struct section: net::config::base::section
			{
				typedef std::map<std::string, boost::any> map_t;
				map_t m_values;
				config_ptr m_parent;

				section(const shared_config_ptr& parent) : m_parent(parent) {}

				bool has_value(const std::string& name) const override { return m_values.find(name) != m_values.end(); }
				void set_value(const std::string& name, const std::string& svalue) override
				{
					bool store;
					auto it = find(name, store);
					it->second = svalue;
					if (store)
						this->store();
				}
				void set_value(const std::string& name, int ivalue) override
				{
					bool store;
					auto it = find(name, store);
					it->second = ivalue;
					if (store)
						this->store();
				}
				void set_value(const std::string& name, bool bvalue) override
				{
					bool store;
					auto it = find(name, store);
					it->second = bvalue;
					if (store)
						this->store();
				}
				std::string get_string(const std::string& name, const std::string& def_val) const override
				{
					auto it = m_values.find(name);
					if (it == m_values.end())
						return def_val;
					return boost::any_cast<std::string>(it->second);
				}
				int get_int(const std::string& name, int def_val) const override
				{
					auto it = m_values.find(name);
					if (it == m_values.end())
						return def_val;
					return boost::any_cast<int>(it->second);
				}
				bool get_bool(const std::string& name, bool def_val) const override
				{
					auto it = m_values.find(name);
					if (it == m_values.end())
						return def_val;
					return boost::any_cast<bool>(it->second);
				}

			private:
				map_t::iterator find(const std::string& name, bool& store)
				{
					store = false;
					auto pos = m_values.lower_bound(name);
					if (pos == m_values.end() || m_values.key_comp()(name, pos->first))
					{
						pos = m_values.insert(pos, std::make_pair(name, boost::any()));
						store = true;
					}
					return pos;
				}

				void store();
			};
			typedef std::shared_ptr<section> file_section_ptr;

			struct config: net::config::base::config, std::enable_shared_from_this<config>
			{
				typedef std::map<std::string, file_section_ptr> map_t;
				map_t m_sections;
				fs::path m_path;

				void open(const fs::path& path);
				void store();
				base::section_ptr get_section(const std::string& name) override;
			};

			void section::store()
			{
				auto ptr = m_parent.lock();
				if (ptr)
					ptr->store();
			}

			void config::open(const fs::path& path)
			{
				m_sections.clear();
				m_path = path;

				fs::ifstream in{ m_path };

				file_section_ptr curr;
				std::string line;
				std::regex header(R"(^\s*\[(.*)\]\s*$)");
				std::regex number(R"(^\s*([^=]+)\s*=\s*(\d+)\s*$)");
				std::regex boolean(R"(^\s*([^=]+)\s*=\s*(true|false|yes|no)\s*$)");
				std::regex text(R"(^\s*([^=]+)\s*=\s*(.*)\s*$)");

				while (std::getline(in, line))
				{
					std::smatch match;
					if (std::regex_match(line, match, header))
					{
						curr = std::static_pointer_cast<section>(get_section(match[1]));
						continue;
					}

					if (!curr) // default section not supported
						continue;

					if (std::regex_match(line, match, number))
					{
						std::istringstream num(match[2]);
						int val;
						num >> val;
						curr->m_values[match[1]] = val;
						continue;
					}

					if (std::regex_match(line, match, boolean))
					{
						curr->m_values[match[1]] = match[2] == "true" || match[2] == "yes";
						continue;
					}

					if (std::regex_match(line, match, text))
					{
						curr->m_values[match[1]] = match[2].str();
						continue;
					}
				}
			}

			void config::store()
			{
				fs::ofstream out{m_path};
				for (auto&& sec : m_sections)
				{
					out << "[" << sec.first << "]\n";
					for (auto && val : sec.second->m_values)
					{
						out << val.first << "=";
						if (val.second.type() == typeid(int))
							out << boost::any_cast<int>(val.second);
						else if (val.second.type() == typeid(bool))
							out << (boost::any_cast<bool>(val.second) ? "true" : "false");
						else
							out << boost::any_cast<std::string>(val.second);
						out << "\n";
					}
					out << "\n";
				}
			}

			base::section_ptr config::get_section(const std::string& name)
			{
				auto pos = m_sections.lower_bound(name);
				if (pos != m_sections.end() && !m_sections.key_comp()(name, pos->first))
					return pos->second;

				auto new_sec = std::make_shared<section>(shared_from_this());
				m_sections.insert(pos, std::make_pair(name, new_sec));
				return new_sec;
			}
		}

		namespace base
		{
			config_ptr file_config(const fs::path& path)
			{
				auto cfg = std::make_shared<file::config>();
				if (cfg)
					cfg->open(path);
				return cfg;
			}
		}
	}
}
