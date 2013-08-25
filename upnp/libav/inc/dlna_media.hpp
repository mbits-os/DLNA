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
#ifndef __DLNA_MEDIA_HPP__
#define __DLNA_MEDIA_HPP__

#include <string>
#include <utils.hpp>
#include <boost/filesystem.hpp>

namespace net { namespace dlna {

	struct init
	{
		init();
		~init();
	};

	typedef std::streamsize size_t;

	enum class Class
	{
		Unknown,
		Image,
		Audio,
		Video,
		Container
	};

	struct Profile
	{
		const char* m_name; // ORG_PN
		const char* m_mime;
		const char* m_label;
		Class m_class;
		const Profile* m_transcode_to;
		Profile() : m_name(nullptr), m_mime(nullptr), m_label(nullptr), m_class(Class::Container), m_transcode_to(nullptr) {}
		Profile(const char* name, const char* mime, const char* label, Class klass)
			: m_name(name)
			, m_mime(mime)
			, m_label(label)
			, m_class(klass)
			, m_transcode_to(nullptr)
		{}
		Profile(const char* name, const char* mime, const char* label, Class klass, const Profile& transcode_to)
			: m_name(name)
			, m_mime(mime)
			, m_label(label)
			, m_class(klass)
			, m_transcode_to(&transcode_to)
		{}

		void clear()
		{
			*this = Profile();
		}

		static const Profile* guess_from_file(const boost::filesystem::path& path);
	};

	struct ItemMetadata
	{
		std::string m_title;
		std::string m_artist;
		std::string m_album_artist;
		std::string m_composer;
		std::string m_album;
		std::string m_genre;
		std::string m_date;
		std::string m_comment;
		unsigned    m_track;
		void clear()
		{
			m_title.clear();
			m_artist.clear();
			m_album_artist.clear();
			m_composer.clear();
			m_album.clear();
			m_genre.clear();
			m_date.clear();
			m_comment.clear();
			m_track = 0;
		}
	};

	struct ItemProperties
	{
		time_t m_last_write_time;
		size_t m_size;
		ulong  m_bitrate;
		ulong  m_duration;
		ulong  m_sample_freq;
		ulong  m_channels;
		ulong  m_width;
		ulong  m_height;
		ulong  m_bps;
		ItemProperties()
			: m_last_write_time(0)
			, m_size(0)
			, m_bitrate(0)
			, m_duration(0)
			, m_sample_freq(0)
			, m_channels(0)
			, m_height(0)
			, m_width(0)
			, m_bps(0)
		{
		}

		void clear()
		{
			m_last_write_time = 0;
			m_size = 0;
			m_bitrate = 0;
			m_duration = 0;
			m_sample_freq = 0;
			m_channels = 0;
			m_height = 0;
			m_width = 0;
			m_bps = 0;
		}
	};

	struct Item
	{
		ItemMetadata m_meta;
		ItemProperties m_props;
		Profile m_profile;
		Class m_class;
		std::vector<char> m_cover;

		Item()
			: m_class(Class::Unknown)
		{
		}

		bool open(const boost::filesystem::path& path);
	};

}} // net::dlna

#endif // __DLNA_MEDIA_HPP__
