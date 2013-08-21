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
#ifndef __CONFIG_HPP__
#define __CONFIG_HPP__

#include <string>
#include <memory>
#include <boost/utility.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <interface.hpp>

namespace net
{
	namespace config
	{
		namespace base
		{
			struct section
			{
				virtual ~section() {}
				virtual bool has_value(const std::string& name) const = 0;
				virtual void set_value(const std::string& name, const std::string& svalue) = 0;
				virtual void set_value(const std::string& name, int ivalue) = 0;
				virtual void set_value(const std::string& name, bool bvalue) = 0;
				virtual std::string get_string(const std::string& name, const std::string& def_val) const = 0;
				virtual int get_int(const std::string& name, int def_val) const = 0;
				virtual bool get_bool(const std::string& name, bool def_val) const = 0;
			};
			typedef std::shared_ptr<section> section_ptr;

			struct config
			{
				virtual ~config() {}
				virtual section_ptr get_section(const std::string& name) = 0;
			};
			typedef std::shared_ptr<config> config_ptr;

			config_ptr file_config(const boost::filesystem::path&);
		}

		namespace wrapper
		{
			template <typename T> struct get_value;
			template <typename Value> struct setting;

			struct section : public boost::noncopyable
			{
				template <typename Value> friend struct get_value;
				template <typename Value> friend struct setting;

				base::config_ptr m_impl;
				mutable base::section_ptr m_section;
				std::string m_section_name;

				section(const base::config_ptr& impl, const std::string& section)
					: m_impl(impl)
					, m_section_name(section)
				{}

			private:
				bool has_value(const std::string& name) const
				{
					ensure_section();
					return m_section->has_value(name);
				}
				void set_value(const std::string& name, const std::string& svalue)
				{
					ensure_section();
					return m_section->set_value(name, svalue);
				}
				void set_value(const std::string& name, int ivalue)
				{
					ensure_section();
					return m_section->set_value(name, ivalue);
				}
				void set_value(const std::string& name, bool bvalue)
				{
					ensure_section();
					return m_section->set_value(name, bvalue);
				}
				std::string get_string(const std::string& name, const std::string& def_val) const
				{
					ensure_section();
					return m_section->get_string(name, def_val);
				}
				int get_int(const std::string& name, int def_val) const
				{
					ensure_section();
					return m_section->get_int(name, def_val);
				}
				bool get_bool(const std::string& name, bool def_val) const
				{
					ensure_section();
					return m_section->get_bool(name, def_val);
				}
				void ensure_section() const
				{
					if (!m_section)
						m_section = m_impl->get_section(m_section_name);
					if (!m_section)
						throw std::runtime_error("Section " + m_section_name + " was not created");
				}
			};

			template <>
			struct get_value<std::string>
			{
				static std::string helper(section& sec, const std::string& name, const std::string& def_val)
				{
					return sec.get_string(name, def_val);
				}
			};

			template <>
			struct get_value<int>
			{
				static int helper(section& sec, const std::string& name, int def_val)
				{
					return sec.get_int(name, def_val);
				}
			};

			template <>
			struct get_value<bool>
			{
				static bool helper(section& sec, const std::string& name, bool def_val)
				{
					return sec.get_bool(name, def_val);
				}
			};

			template <typename Value>
			struct setting : public boost::noncopyable
			{
				typedef setting<Value> my_type;
				section& m_parent;
				std::string m_name;
				Value m_def_val;

				setting(section& parent, const std::string& name, const Value& def_val = Value())
					: m_parent(parent)
					, m_name(name)
					, m_def_val(def_val)
				{}

				bool is_set() const
				{
					return m_parent.has_value(m_name);
				}

				operator Value() const
				{
					return get_value<Value>::helper(m_parent, m_name, m_def_val);
				}

				my_type& operator = (const Value& v)
				{
					m_parent.set_value(m_name, v);
					return *this;
				}
			};

			template <typename Value>
			std::ostream& operator << (std::ostream& o, const setting<Value>& rhs)
			{
				return o << static_cast<Value>(rhs);
			}

			template <>
			struct setting<boost::asio::ip::address_v4> : public boost::noncopyable
			{
				typedef boost::asio::ip::address_v4 Value;
				typedef setting<Value> my_type;
				section& m_parent;
				std::string m_name;
				mutable bool m_set;
				mutable Value m_cached;

				setting(section& parent, const std::string& name)
					: m_parent(parent)
					, m_name(name)
					, m_set(false)
				{}

				bool is_set() const
				{
					return m_parent.has_value(m_name);
				}

				operator Value() const
				{
					if (!m_set)
					{
						m_set = true;
						auto str = get_value<std::string>::helper(m_parent, m_name, "0.0.0.0");
						m_cached = Value::from_string(str);
						if (m_cached.is_unspecified())
							m_cached = net::iface::get_default_interface();
					}
					return m_cached;
				}

				my_type& operator = (const Value& v)
				{
					m_parent.set_value(m_name, v.to_string());
					return *this;
				}
			};
		}

		struct config;
		typedef std::shared_ptr<config> config_ptr;

		struct config
		{
		private:
			wrapper::section server;

		public:
			config(const base::config_ptr& impl)
				: server(impl, "Server")
				, uuid (server, "UUID")
				, port (server, "Port", 6001)
				, iface(server, "Interface")
			{}
			virtual ~config() {}

			wrapper::setting<std::string> uuid;
			wrapper::setting<int> port;
			wrapper::setting<boost::asio::ip::address_v4> iface;

			static inline config_ptr from_file(const boost::filesystem::path& path)
			{
				auto impl = base::file_config(path);
				if (!impl)
					return nullptr;

				return std::make_shared<config>(impl);
			}
		};

		struct renderer
		{
			struct general_wrapper : wrapper::section
			{
				general_wrapper(const base::config_ptr& impl)
					: wrapper::section(impl, "General")
					, name(*this, "Name")
					, icon(*this, "Icon")
				{
				}
				wrapper::setting<std::string> name;
				wrapper::setting<std::string> icon;
			};
			struct recognize_wrapper : wrapper::section
			{
				recognize_wrapper(const base::config_ptr& impl)
					: wrapper::section(impl, "Recognize")
					, ua_match               (*this, "UAMatch")
					, additional_header      (*this, "AdditionalHeader")
					, additional_header_match(*this, "AdditionalHeaderMatch")
				{
				}
				wrapper::setting<std::string> ua_match;
				wrapper::setting<std::string> additional_header;
				wrapper::setting<std::string> additional_header_match;
			};
			struct basic_capabilites_wrapper : wrapper::section
			{
				basic_capabilites_wrapper(const base::config_ptr& impl)
					: wrapper::section(impl, "Basic capabilites")
					, video(*this, "Video", false)
					, audio(*this, "Audio", false)
					, image(*this, "Image", false)
				{
				}
				wrapper::setting<bool> video;
				wrapper::setting<bool> audio;
				wrapper::setting<bool> image;
			};
			struct media_server_wrapper : wrapper::section
			{
				media_server_wrapper(const base::config_ptr& impl)
					: wrapper::section(impl, "MediaServer")
					, seek_by_time         (*this, "SeekByTime", false)
					, protocol_localization(*this, "ProtocolLocalization", false)
					, profile_patches      (*this, "ProfilePatches")
					, send_ORG_PN          (*this, "Send_ORG_PN", false)
				{
				}
				wrapper::setting<bool>        seek_by_time;
				wrapper::setting<bool>        protocol_localization;
				wrapper::setting<std::string> profile_patches;
				wrapper::setting<bool>        send_ORG_PN;
			};
			struct transcode_wrapper : wrapper::section
			{
				transcode_wrapper(const base::config_ptr& impl)
					: wrapper::section(impl, "Transcode")
					, video                 (*this, "Video", false)
					, audio                 (*this, "Audio", false)
					, max_video_bitrate_mbps(*this, "MaxVideoBitrateMbps", 0)
					, max_video_width       (*this, "MaxVideoWidth", 0)
					, max_video_height      (*this, "MaxVideoHeight", 0)
					, max_h264_level_41     (*this, "MaxH264Level41", false)
					, audio_441kHz          (*this, "441kHzAudio", false)
					, fast_start            (*this, "FastStart", false)
					, video_size            (*this, "VideoFileSize", false)
					, force_jpg_thumbnails  (*this, "ForceJPGThumbnails", false)
					, thumbnails_as_resource(*this, "ThumbnailAsResource", false)
					, allow_chunked_transfer(*this, "AllowChunkedTransfer", false)
					, exif_auto_rotate      (*this, "ExifAutoRotate", false)
				{
				}

				wrapper::setting<bool> video;
				wrapper::setting<bool> audio;
				wrapper::setting<int>  max_video_bitrate_mbps;
				wrapper::setting<int>  max_video_width;
				wrapper::setting<int>  max_video_height;
				wrapper::setting<bool> max_h264_level_41;
				wrapper::setting<bool> audio_441kHz;
				wrapper::setting<bool> fast_start;
				wrapper::setting<int>  video_size;
				wrapper::setting<bool> force_jpg_thumbnails;
				wrapper::setting<bool> thumbnails_as_resource;
				wrapper::setting<bool> allow_chunked_transfer;
				wrapper::setting<bool> exif_auto_rotate;
			};
			struct supported_wrapper : wrapper::section
			{
				supported_wrapper(const base::config_ptr& impl, const std::string& name)
					: wrapper::section(impl, name)
				{
				}
			};

			renderer(const base::config_ptr& impl)
				: general(impl)
				, recognize(impl)
				, basic_capabilities(impl)
				, media_server(impl)
				, transcode(impl)
				, supported_video(impl, "Video formats")
				, supported_audio(impl, "Audio formats")
				, supported_image(impl, "Image formats")
			{
			}

			general_wrapper           general;
			recognize_wrapper         recognize;
			basic_capabilites_wrapper basic_capabilities;
			media_server_wrapper      media_server;
			transcode_wrapper         transcode;
			supported_wrapper         supported_video;
			supported_wrapper         supported_audio;
			supported_wrapper         supported_image;
		};
	}
}

#endif // __LOG_HPP__