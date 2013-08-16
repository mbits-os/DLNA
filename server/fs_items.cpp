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
#include "fs_items.hpp"
#include <MediaInfo/MediaInfo.h>
#include <regex>

namespace mi = MediaInfoLib;

namespace lan
{
	Log::Module APP { "APPL" };

	struct MI
	{
		struct Open
		{
			bool opened;
			Open(const fs::path& file)
			{
				opened = get()->Open(fs::absolute(file).native()) != 0;
			}
			~Open()
			{
				if (opened)
					get()->Close();
			}

			operator bool() const { return opened; }
		};

		static std::wstring inform(const fs::path& file)
		{

			Open session(file);
			if (!session)
				return std::wstring();
			return get()->Inform();
		}
	private:
		static std::shared_ptr<mi::MediaInfo> get()
		{
			static auto s_mi = std::make_shared<mi::MediaInfo>();
			static bool s_inited = false;
			if (!s_inited)
			{
				s_inited = true;
				s_mi->Option(L"Complete", L"1");
				s_mi->Option(L"Language", L"raw");
			}
			return s_mi;
		}
	};

	namespace item
	{
		av::items::media_item_ptr from_path(const fs::path& path)
		{
			if (fs::is_directory(path))
			{
				if (path.filename() == ".")
					return nullptr;
				return std::make_shared<directory_item>(path);
			}
			auto text = MI::inform(path);
			log::wwarning() << path << "\n" << std::regex_replace(text, std::wregex(L"\r\n"), L"\n");
			std::cout << path << "\n";
			return nullptr;
		}

#pragma region container_file

		std::vector<av::items::media_item_ptr> container_file::list(net::ulong start_from, net::ulong max_count, const av::items::search_criteria& sort)
		{
			rescan_if_needed();

			std::vector<av::items::media_item_ptr> out;
			if (start_from > m_children.size())
				start_from = m_children.size();

			auto end_at = m_children.size() - start_from;
			if (end_at > max_count)
				end_at = max_count;
			end_at += start_from;

			for (auto i = start_from; i < end_at; ++i)
				out.push_back(m_children.at(i));

			if (!sort.empty())
			{
				// TODO: sort the result
			}

			return out;
		}

		av::items::media_item_ptr container_file::get_item(const std::string& id)
		{
			av::items::media_item_ptr candidate;
			std::string rest_of_id;

			std::tie(candidate, rest_of_id) = av::items::find_item(m_children, id);

			if (!candidate || rest_of_id.empty())
				return candidate;

			if (!candidate->is_folder())
				return nullptr;

			return candidate->get_item(rest_of_id);
		}

		void container_file::output(std::ostream& o, const std::vector<std::string>& filter) const
		{
			output_open(o, filter, m_children.size());
			output_close(o, filter);
		}

		void container_file::add_child(av::items::media_item_ptr child)
		{
			remove_child(child);
			m_children.push_back(child);
			auto id = ++m_current_max;
			child->set_id(id);
			child->set_objectId_attr(get_objectId_attr() + av::items::SEP + std::to_string(id));
		}

		void container_file::remove_child(av::items::media_item_ptr child)
		{
			folder_changed();

			auto pos = std::find(m_children.begin(), m_children.end(), child);
			if (pos != m_children.end())
				m_children.erase(pos);
		}
#pragma endregion

		void directory_item::rescan_if_needed()
		{
			auto curr_t = fs::last_write_time(m_path);
			if (m_last_scan == curr_t)
				return;

			log::debug() << "Rescanning " << m_path;

			m_last_scan = curr_t;

			std::vector<std::pair<fs::path, int>> entries;
			for (auto && file : contents(m_path))
				entries.emplace_back(file.path(), 1);

			std::vector<std::pair<fs::path, av::items::media_item_ptr>> current;
			for (auto && ptr : m_children)
				current.emplace_back(get_path(ptr), ptr);
			auto update = m_update_id + 1;

			std::sort(entries.begin(), entries.end(), [](const std::pair<fs::path, int>& lhs, const std::pair<fs::path, int>& rhs) { return lhs.first < rhs.first; });
			std::sort(current.begin(), current.end(), [](const std::pair<fs::path, av::items::media_item_ptr>& lhs, const std::pair<fs::path, av::items::media_item_ptr>& rhs) { return lhs.first < rhs.first; });

			auto entry = entries.begin();
			auto curr = current.begin();

			while ((entry != entries.end()) && (curr != current.end()))
			{
				if (entry->first == curr->first)
				{
					entry->second = 0; // do not add
					curr->second = nullptr; // do not remove
					++entry;
					++curr;
				}
				else if (entry->first < curr->first)
				{
					++entry;
				}
				else
				{
					++curr;
				}
			}

			for (auto && curr : current)
				if (curr.second)
					remove_child(curr.second);

			for (auto && entry : entries)
				if (entry.second)
				{
					auto item = from_path(entry.first);
					if (item)
						add_child(item);
				}

		}
	}
}
