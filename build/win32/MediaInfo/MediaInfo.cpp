// MediaInfo.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "MediaInfo.h"
#include <MediaInfo/MediaInfo.h>
#include <dom.hpp>
#include <sstream>
#include <iostream>
#include <vector>

#pragma comment(lib, "libz.lib")
#pragma comment(lib, "libzen.lib")
#pragma comment(lib, "mediainfo.lib")
#pragma comment(lib, "curl.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libdom.lib")

namespace MediaInfo
{
	template <typename T>
	bool ref_get(const dom::XmlNodePtr& val, T& out)
	{
		if (!val)
			return false;
		std::istringstream i(val->stringValue());
		i >> out;
		return true;
	}

	bool ref_get(const dom::XmlNodePtr& val, std::string& out)
	{
		if (!val)
			return false;
		out = val->stringValue();
		return true;
	}

	template <typename T>
	T get(const dom::XmlNodePtr& node, const std::string& xpath, const T& defaultValue)
	{
		T out;
		if (node && ref_get(node->find(xpath), out))
			return out;
		return defaultValue;
	}

	template <typename T>
	T get(const dom::XmlNodePtr& node, const T& defaultValue)
	{
		T out;
		if (ref_get(node, out))
			return out;
		return defaultValue;
	}

	struct MediaInfoAPI : APIHandle
	{
		MediaInfoLib::MediaInfo m_info;
		struct Session : InfoHandle
		{
			MediaInfoLib::MediaInfo& m_info;
			bool opened;
			std::wstring m_text;
			dom::XmlDocumentPtr m_doc;
			LPCWSTR m_path;
			bool m_printed;

			Session(MediaInfoLib::MediaInfo& info, LPCWSTR path)
				: m_info(info)
				, m_path(path)
				, m_printed(false)
				, opened(false)
			{
				try
				{
					opened = m_info.Open(path) != 0;
				}
				catch(...)
				{
					fwprintf(stderr, L"[EXCEPTION] MediaInfo::MediaInfoAPI::Session(%s)\n", path);
				}
			}
			~Session()
			{
				try
				{
					if (opened)
						m_info.Close();
				}
				catch(...)
				{
					std::cerr << "[EXCEPTION] MediaInfo::MediaInfoAPI::~Session();\n";
				}
			}

			bool isOpened() const { return opened; }

			LPCWSTR text()
			{
				if (!opened)
					return nullptr;

				try
				{
					if (m_text.empty())
						m_text = m_info.Inform();
				}
				catch(...)
				{
					std::cerr << "[EXCEPTION] MediaInfo::MediaInfoAPI::Session::text();\n";
				}

				return m_text.c_str();
			}

			static std::pair<TrackType, int> readTrackType(dom::XmlNodePtr& track)
			{
				return std::make_pair(
					get(track, "@type", TrackType::Unknown),
					get(track, "@streamid", 0)
					);
			}

			bool extract(IContainer* env)
			{
				if (!opened || !env)
					return false;

				if (!m_doc)
				{
					try
					{
						if (m_text.empty())
							m_text = m_info.Inform();
					}
					catch(...)
					{
						std::cerr << "[EXCEPTION] MediaInfo::MediaInfoAPI::Session::extract();\n";
					}

					size_t rest = m_text.length() * sizeof(wchar_t) ;
					const char* data = (const char*) m_text.c_str();
					m_doc = dom::XmlDocument::fromDataSource([&](void* buffer, size_t size)
					{
						if (size > rest)
							size = rest;
						memcpy(buffer, data, size);
						data += size;
						rest -= size;
						return size;
					});

					if (!m_doc)
						return false;
				}

				auto tracks = m_doc->findall("/File/track");
				for (auto && track : tracks)
				{
					auto attrs = readTrackType(track);
					auto dst = env->create_track(attrs.first, attrs.second);

					if (!dst)
						continue;
					if (!extract_track(dst, track))
						return false;
				}

				return analyze_tracks(env);
			}

			bool extract_track(ITrack* dst, const dom::XmlNodePtr& src);
			bool analyze_tracks(IContainer* env);
		};

		MediaInfoAPI()
		{
			try
			{
				m_info.Option(L"Complete", L"1");
				m_info.Option(L"Language", L"raw");
				m_info.Option(L"Output", L"XML");
			}
			catch(...)
			{
				std::cerr << "[EXCEPTION] MediaInfo::MediaInfoAPI();\n";
			}
		}

		HMEDIAINFO newSession(LPCWSTR path)
		{
			return new (std::nothrow) Session(m_info, path);
		}
	};

	namespace {
		struct Format
		{
			const char* m_name;
			std::vector<const char*> m_values;
			Format(const char* name)
				: m_name(name)
			{}
			Format(const char* name, const std::vector<const char*>& values)
				: m_name(name)
				, m_values(values)
			{}
		};

		// commands:
		// ! exact
		// < starts with
		// ~ contains
		const Format codecs  [] = {
			Format("aac",     { "!m4a", "!40", "!a_aac", "!aac" }),
			Format("ac3",     { "!ac-3", "!a_ac3", "!2000" }),
			Format("aiff",    { "~aiff" }),
			Format("alac",    { "!alac" }),
			Format("ape",     { "!monkey's audio" }),
			Format("atrac",   { "~atrac3" }),
			Format("avi",     { "!avi", "!opendml" }),
			Format("bmp",     { "!bitmap" }),
			Format("divx",    { "~div", "~dx" }),
			//Format("dts",     { "!dts" }),
			Format("dtshd",   { "!dts", "!a_dts", "!8" }),
			Format("dtshd+",  { "!ma" }),
			Format("dv",      { "!dv", "<cdv", "!dc25", "!dcap", "<dvc", "<dvs", "!dvrs", "!dv25", "!dv50", "!dvan", "<dvh", "!dvis", "<dvl", "!dvnm", "<dvp", "!mdvf", "!pdvc", "!r411", "!r420", "!sdcc", "!sl25", "!sl50", "!sldv" }),
			Format("eac3",    { "!e-ac-3" }),
			Format("flac",    { "!flac" }),
			Format("flv",     { "<flash" }),
			Format("gif",     { "!gif" }),
			Format("h264",    { "<avc", "~h264" }),
			Format("jpg",     { "!jpeg" }),
			Format("lpcm",    { "!pcm", "!1" }),
			Format("mkv",     { "!matroska" }),
			//Format("gmc",     {  }),
			//Format("qpel",    {  }),
			Format("mjpeg",   { "~mjpg", "~m-jpeg" }),
			Format("mlp",     { "~mlp" }),
			Format("mov",     { "!qt", "!quicktime" }),
			Format("mp3",     { "!55", "!a_mpeg/l3" }),
			Format("mp3+",    { "!layer 3" }),
			Format("mp4",     { "!isom", "<mp4", "!20", "!m4v", "<mpeg-4", "~xvid" }),
			Format("mpa",     { "!mpeg audio" }),
			Format("mpc",     { "~musepack" }),
			Format("mpeg1",   { "!version 1" }),
			Format("mpeg2",   { "~mpeg video" }),
			Format("mpegps",  { "~mpeg-ps" }),
			Format("mpegts",  { "~mpeg-ts", "!bdav" }),
			Format("ogg",     { "~ogg", "!vorbis", "!a_vorbis" }),
			Format("png",     { "!png" }),
			//Format("ra",      {  }),
			Format("rm",      { "~realmedia", "<rv", "<cook" }),
			Format("shn",     { "!shorten" }),
			Format("tiff",    { "!tiff" }),
			Format("truehd",  { "~truehd" }),
			Format("vc1",     { "!vc-1", "!vc1", "!wvc1", "!wmv3", "!wmv9", "!wmva" }),
			Format("wavpack", { "~wavpack" }),
			Format("wav",     { "!wave" }),
			Format("WebM",    { "!webm" }),
			Format("wma",     { "!161", "<wma" }),
			Format("wmv",     { "~windows media", "!wmv1", "!wmv2", "!wmv7", "!wmv8" })
		};

		bool equals(const char* value, const char* tmplt)
		{
			return strcmp(value, tmplt) == 0;
		}

		bool starts_with(const char* value, const char* tmplt)
		{
			auto len1 = strlen(value);
			auto len2 = strlen(tmplt);
			if (len2 > len1)
				return false;
			if (len2 == len1)
				return equals(value, tmplt);
			return strncmp(value, tmplt, len2) == 0;
		}

		bool contains(const char* value, const char* tmplt)
		{
			return strstr(value, tmplt) != nullptr;
		}

		bool matches(const char* value, const char* tmplt)
		{
			switch (*tmplt)
			{
			case '!': return equals(value, tmplt + 1);
			case '<': return starts_with(value, tmplt + 1);
			case '~': return contains(value, tmplt + 1);
			}
			return false;
		}

		const char* find_format(const char* value)
		{
			if (!value)
				return nullptr;

			for (auto&& codec : codecs)
			{
				for (auto&& tmplt : codec.m_values)
				{
					if (matches(value, tmplt))
						return codec.m_name;
				}
			}

			return nullptr;
		}

		template <typename Value>
		struct setter
		{
			static std::function<bool (ITrack* dst, const dom::XmlNodePtr& src)> function(bool (ITrack::*method)(Value))
			{
				return [method](ITrack* dst, const dom::XmlNodePtr& src) -> bool
				{
					Value val;
					if (!ref_get(src, val))
						return false;
					return (dst->*method)(val);
				};
			}
			static std::function<bool (ITrack* dst, const dom::XmlNodePtr& src)> function(bool (ITrack::*method)(const Value&))
			{
				return [method](ITrack* dst, const dom::XmlNodePtr& src) -> bool
				{
					Value val;
					if (!ref_get(src, val))
						return false;
					return (dst->*method)(val);
				};
			}
			static std::function<bool (ITrack* dst, const dom::XmlNodePtr& src)> function(bool (*func)(ITrack*, const Value&))
			{
				return [func](ITrack* dst, const dom::XmlNodePtr& src) -> bool
				{
					Value val;
					if (!ref_get(src, val))
						return false;
					return func(dst, val);
				};
			}
		};
		template <>
		struct setter<const char*>
		{
			static std::function<bool (ITrack* dst, const dom::XmlNodePtr& src)> function(bool (ITrack::*method)(const char*))
			{
				return [method](ITrack* dst, const dom::XmlNodePtr& src) -> bool
				{
					std::string val;
					if (!ref_get(src, val))
						return false;
					return (dst->*method)(val.c_str());
				};
			}
			static std::function<bool (ITrack* dst, const dom::XmlNodePtr& src)> function(bool (*func)(ITrack*, const char*) )
			{
				return [func](ITrack* dst, const dom::XmlNodePtr& src) -> bool
				{
					std::string val;
					if (!ref_get(src, val))
						return false;
					return func(dst, val.c_str());
				};
			}
		};

		struct Setter
		{
			typedef std::function<bool (ITrack* dst, const dom::XmlNodePtr& src)> function_type;
			const char* m_name;
			function_type m_setter;
			static bool dummy(ITrack* dst, const dom::XmlNodePtr& src) { return true; }

			Setter(const char* name)
				: m_name(name)
				, m_setter(dummy)
			{
			}

			template <typename Value>
			Setter(const char* name, bool (ITrack::*method)(Value) )
				: m_name(name)
				, m_setter(setter<Value>::function(method))
			{
			}

			template <typename Value>
			Setter(const char* name, bool (ITrack::*method) (const Value&))
				: m_name(name)
				, m_setter(setter<Value>::function(method))
			{
			}

			template <typename Value>
			Setter(const char* name, bool (*func) (ITrack*, const Value&))
				: m_name(name)
				, m_setter(setter<Value>::function(func))
			{
			}

			Setter(const char* name, bool (ITrack::*method) (const char*) )
				: m_name(name)
				, m_setter(setter<const char*>::function(method))
			{
			}

			Setter(const char* name, bool (*func)(ITrack*, const char*) )
				: m_name(name)
				, m_setter(setter<const char*>::function(func))
			{
			}

			bool operator() (ITrack* dst, const dom::XmlNodePtr& src) const { return m_setter(dst, src); }
		};

		bool set_format(ITrack* dst, const std::string& value)
		{
			std::string v = value;
			for (auto && c : v) c = std::tolower((unsigned char) c);

			auto format = find_format(v.c_str());
			if (!format)
				return true; // dst->set_format(value.c_str());

			//... additional analysis needed
			return dst->set_format(format);
		}

		const Setter names [] =
		{
			Setter("Format",            set_format),
			Setter("Duration",          &ITrack::set_duration),
			Setter("InternetMediaType", &ITrack::set_mime),
			Setter("Format_Settings_RefFrames.String", &ITrack::set_ref_frame_count),
			Setter("Format_Settings_QPel"),
			Setter("Format_Settings_GMC"),
			Setter("MuxingMode"),
			Setter("CodecID",          set_format),
			Setter("Language"),
			Setter("Title",            &ITrack::set_title),
			Setter("Width",            &ITrack::set_width),
			Setter("Encryption"),
			Setter("Height",           &ITrack::set_height),
			Setter("DisplayAspectRatio.String"),
			Setter("DisplayAspectRatio_Original.Stri"),
			Setter("FrameRate"),
			Setter("FrameRateMode"),
			Setter("OverallBitRate",   &ITrack::set_bitrate),
			Setter("Channels",         &ITrack::set_channels),
			Setter("BitRate",          &ITrack::set_bitrate),
			Setter("SamplingRate",     &ITrack::set_sample_freq),
			Setter("ID"),
			Setter("Cover_Data",       &ITrack::set_cover),
			Setter("Track",            &ITrack::set_title),
			Setter("Album",            &ITrack::set_album),
			Setter("Performer",        &ITrack::set_artist),
			Setter("Genre",            &ITrack::set_genre),
			Setter("Recorded_Date"),
			Setter("Track.Position",   &ITrack::set_track_position),
			Setter("BitDepth"),
			Setter("Video_Delay")
		};
	}

	bool MediaInfoAPI::Session::extract_track(ITrack* dst, const dom::XmlNodePtr& src)
	{
		std::ostringstream msg;
		bool printed = false;
		for (auto && field : src->childNodes())
		{
			if (field->nodeType() != dom::ELEMENT_NODE)
				continue;

			std::string name = field->nodeName();
			auto len = name.length();
			bool show = false;

			//if (name == "Cover_Data")
			//{
			//	auto fld = field->stringValue();
			//	std::cout << "Cover_Data\n";
			//	std::cout << "    " << fld << "\n";
			//}

			for (auto && known : names)
			{
				if (name == known.m_name)
				{
					if (!known(dst, field))
						return false;
					show = true;
					break;
				}
			}

			if (!show)
			{
				auto sub = name.substr(0, 14);
				if (sub == "Format_Version" || sub == "Format_Profile")
				{
					if (!names[0](dst, field))
						return false;
					show = true;
				}
			}

			if (!show)
				continue;

			//if (!m_printed)
			//{
			//	m_printed = true;
			//	char buffer[1024];
			//	WideCharToMultiByte(CP_ACP, 0, m_path, -1, buffer, sizeof(buffer), nullptr, nullptr);
			//	msg << buffer << "\n";
			//}

			//if (!printed)
			//{
			//	printed = true;
			//	msg << "    " << dst->get_type() << "[" << dst->get_id() << "] ";
			//}
			//msg << " " << name << "=\"" << field->stringValue() << "\"";
		}
		//msg << "\n";
		//if (printed) std::cout << msg.str();

		return true;
	}

	struct tracks_iterator
	{
		IContainer* m_env;
		size_t m_index;
	public:

		tracks_iterator(IContainer* env, size_t index) : m_env(env), m_index(index) {}

		ITrack* operator*() { return m_env->get_item(m_index); }

		tracks_iterator operator ++()
		{
			++m_index;
			return *this;
		}

		tracks_iterator operator ++(int)
		{
			tracks_iterator tmp(*this);
			m_index++;
			return tmp;
		}

		bool operator == (const tracks_iterator& rhs) const { return m_index == rhs.m_index; }
		bool operator != (const tracks_iterator& rhs) const { return m_index != rhs.m_index; }
	};

	inline tracks_iterator begin(IContainer* env)
	{
		return tracks_iterator(env, 0);
	}

	inline tracks_iterator end(IContainer* env)
	{
		return tracks_iterator(env, env ? env->get_length() : 0);
	}

	bool MediaInfoAPI::Session::analyze_tracks(IContainer* env)
	{
		ITrack* general = nullptr;
		ITrack* video = nullptr;
		ITrack* audio = nullptr;
		ITrack* photo = nullptr;
		for (auto && track : env)
		{
			if (!general && track->get_type() == TrackType::General) general = track;
			if (!video && track->get_type()   == TrackType::Video)   video = track;
			if (!audio && track->get_type()   == TrackType::Audio)   audio = track;
			if (!photo && track->get_type()   == TrackType::Image)   photo = track;
		}

		if (!general)
			return false;

		std::string format, video_format, audio_format;
		std::string mime, video_mime, audio_mime;

		format = general->get_format_c();
		mime = general->get_mime_c();

		if (video)
		{
			video_format = video->get_format_c();
			video_mime = video->get_mime_c();
		}
		if (audio)
		{
			audio_format = audio->get_format_c();
			audio_mime = audio->get_mime_c();
		}

		if (!video && !audio && !photo)
			return true;

		//std::cout << "[" << format << ":" << video_format << ":" << audio_format << "][" << mime << ":" << video_mime << ":" << audio_mime << "] -> ";

		for (auto && c : format) c = std::tolower((unsigned char) c);
		for (auto && c : video_format) c = std::tolower((unsigned char) c);
		for (auto && c : audio_format) c = std::tolower((unsigned char) c);

#define UNKNOWN_VIDEO_TYPEMIME "video/mpeg"
#define UNKNOWN_IMAGE_TYPEMIME "image/jpeg"
#define UNKNOWN_AUDIO_TYPEMIME "audio/mpeg"
#define AUDIO_MP3_TYPEMIME "audio/mpeg"
#define AUDIO_MP4_TYPEMIME "audio/x-m4a"
#define AUDIO_WAV_TYPEMIME "audio/wav"
#define AUDIO_WMA_TYPEMIME "audio/x-ms-wma"
#define AUDIO_FLAC_TYPEMIME "audio/x-flac"
#define AUDIO_OGG_TYPEMIME "audio/x-ogg"
#define AUDIO_LPCM_TYPEMIME "audio/L16"
#define MPEG_TYPEMIME "video/mpeg"
#define MP4_TYPEMIME "video/mp4"
#define AVI_TYPEMIME "video/avi"
#define WMV_TYPEMIME "video/x-ms-wmv"
#define ASF_TYPEMIME "video/x-ms-asf"
#define MATROSKA_TYPEMIME "video/x-matroska"
#define VIDEO_TRANSCODE "video/transcode"
#define AUDIO_TRANSCODE "audio/transcode"
#define PNG_TYPEMIME "image/png"
#define JPEG_TYPEMIME "image/jpeg"
#define TIFF_TYPEMIME "image/tiff"
#define GIF_TYPEMIME "image/gif"
#define BMP_TYPEMIME "image/bmp"

		if (format == "avi")
			mime = AVI_TYPEMIME;
		else if (format == "asf" || format == "wmv")
			mime = WMV_TYPEMIME;
		else if (format == "matroska" || format == "mkv")
			mime = MATROSKA_TYPEMIME;
		else if (video_format == "mjpeg")
			mime = JPEG_TYPEMIME;
		else if ("png" == video_format || "png" == format)
			mime = PNG_TYPEMIME;
		else if ("gif" == video_format || "gif" == format)
			mime = GIF_TYPEMIME;
		else if (video_format == "h264" || video_format == "h263" || video_format == "mpeg4" || video_format == "mp4")
			mime = MP4_TYPEMIME;
		else if (video_format.find("mpeg") != std::string::npos || video_format.find("mpg") != std::string::npos)
			mime = MPEG_TYPEMIME;
		else if (video_format.empty() && (audio_format == "mpa" || audio_format == "mp3+" || audio_format.find("mp3") != std::string::npos))
			mime = AUDIO_MP3_TYPEMIME;
		else if (video_format.empty() && audio_format.find("aac") != std::string::npos)
			mime = AUDIO_MP4_TYPEMIME;
		else if (video_format.empty() && audio_format.find("flac") != std::string::npos)
			mime = AUDIO_FLAC_TYPEMIME;
		else if (video_format.empty() && audio_format.find("vorbis") != std::string::npos)
			mime = AUDIO_OGG_TYPEMIME;
		else if (video_format.empty() && (audio_format.find("asf") != std::string::npos || starts_with(audio_format.c_str(), "wm")))
			mime = AUDIO_WMA_TYPEMIME;
		else if (video_format.empty() && (audio_format.find("wav") != std::string::npos || starts_with(audio_format.c_str(), "pcm")))
			mime = AUDIO_WAV_TYPEMIME;
		else if (video)
			mime = UNKNOWN_VIDEO_TYPEMIME;
		else if (audio)
			mime = UNKNOWN_AUDIO_TYPEMIME;
		else if (photo)
			mime = UNKNOWN_IMAGE_TYPEMIME;

		general->set_mime(mime.c_str());

		//char buffer[1024];
		//WideCharToMultiByte(CP_ACP, 0, m_path, -1, buffer, sizeof(buffer), nullptr, nullptr);
		//std::cout << "[" << mime << "] " << buffer << "\n";

		return true;
	}

	MEDIAINFO_API HMEDIAINFOAPI __stdcall CreateApi()
	{
		return new (std::nothrow) MediaInfoAPI();
	}
	MEDIAINFO_API void __stdcall DestroyApi(HMEDIAINFOAPI hApi)
	{
		delete static_cast<MediaInfoAPI*>(hApi);
	}
	MEDIAINFO_API HMEDIAINFO __stdcall CreateMediaInfo(HMEDIAINFOAPI hApi, LPCWSTR path)
	{
		return static_cast<MediaInfoAPI*>(hApi)->newSession(path);
	}
	MEDIAINFO_API LPCWSTR __stdcall GetText(HMEDIAINFO hMediaInfo)
	{
		return static_cast<MediaInfoAPI::Session*>(hMediaInfo)->text();
	}
	MEDIAINFO_API bool __stdcall ExtractContainerDescription(HMEDIAINFO hMediaInfo, IContainer* env)
	{
		return static_cast<MediaInfoAPI::Session*>(hMediaInfo)->extract(env);
	}
	MEDIAINFO_API void __stdcall DestroyMediaInfo(HMEDIAINFO hMediaInfo)
	{
		delete static_cast<MediaInfoAPI::Session*>(hMediaInfo);
	}
}
