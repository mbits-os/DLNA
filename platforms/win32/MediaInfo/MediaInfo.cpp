// MediaInfo.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "MediaInfo.h"
#include <MediaInfo/MediaInfo.h>
#include <dom.hpp>
#include <sstream>
#include <iostream>

#pragma comment(lib, "libz.lib")
#pragma comment(lib, "libzen.lib")
#pragma comment(lib, "mediainfo.lib")
#pragma comment(lib, "curl.lib")
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "libdom.lib")

namespace MediaInfo
{
	template <typename T>
	bool ref_get(dom::XmlNodePtr& val, T& out)
	{
		if (!val)
			return false;
		std::istringstream i(val->stringValue());
		i >> out;
		return true;
	}

	bool ref_get(dom::XmlNodePtr& val, std::string& out)
	{
		if (!val)
			return false;
		out = val->stringValue();
		return true;
	}

	template <typename T>
	T get(dom::XmlNodePtr& node, const std::string& xpath, const T& defaultValue)
	{
		T out;
		if (node && ref_get(node->find(xpath), out))
			return out;
		return defaultValue;
	}

	template <typename T>
	T get(dom::XmlNodePtr& node, const T& defaultValue)
	{
		T out;
		if (ref_get(node, out))
			return out;
		return defaultValue;
	}

	namespace {
		struct Setter
		{
			typedef bool setter_type(ITrack* dst, const dom::XmlNodePtr& src);
			const char* m_name;
			bool (*m_setter)(ITrack* dst, const dom::XmlNodePtr& src);
			static bool dummy(ITrack* dst, const dom::XmlNodePtr& src) { return true; }

			Setter(const char* name, bool (*setter)(ITrack*, const dom::XmlNodePtr&) = dummy)
				: m_name(name)
				, m_setter(setter)
			{
			}

			bool operator() (ITrack* dst, const dom::XmlNodePtr& src) const { return m_setter(dst, src); }
		};
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
			{
				opened = m_info.Open(path) != 0;
			}
			~Session()
			{
				if (opened)
					m_info.Close();
			}

			bool isOpened() const { return opened; }

			LPCWSTR text()
			{
				if (!opened)
					return nullptr;

				if (m_text.empty())
					m_text = m_info.Inform();

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
					if (m_text.empty())
						m_text = m_info.Inform();

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
					//if (attrs.first == TrackType::Unknown)
					//{
					//	auto type = get(track, "@type", std::string());
					//	std::cout << "UNKNOWN TRACK: " << type << "\n";
					//}
					if (!dst)
						continue;
					if (!extract_track(dst, track))
						return false;
				}
				//std::cout << "\n";

				return true;
			}

			bool extract_track(ITrack* dst, const dom::XmlNodePtr& src);
		};

		MediaInfoAPI()
		{
			m_info.Option(L"Complete", L"1");
			m_info.Option(L"Language", L"raw");
			m_info.Option(L"Output", L"XML");
		}

		HMEDIAINFO newSession(LPCWSTR path)
		{
			return new (std::nothrow) Session(m_info, path);
		}
	};

	namespace {
		const Setter names [] =
		{
			Setter("Format"),
			Setter("Duration"),
			Setter("Format_Settings_RefFrames.String"),
			Setter("Format_Settings_QPel"),
			Setter("Format_Settings_GMC"),
			Setter("MuxingMode"),
			Setter("CodecID"),
			Setter("Language"),
			Setter("Language.String"),
			Setter("Title"),
			Setter("Width"),
			Setter("Encryption"),
			Setter("Height"),
			Setter("DisplayAspectRatio.String"),
			Setter("DisplayAspectRatio_Original.Stri"),
			Setter("FrameRate"),
			Setter("FrameRateMode"),
			Setter("OverallBitRate"),
			Setter("Channels"),
			Setter("BitRate"),
			Setter("SamplingRate"),
			Setter("ID"),
			Setter("Cover_Data"),
			Setter("Track"),
			Setter("Album"),
			Setter("Performer"),
			Setter("Genre"),
			Setter("Recorded_Date"),
			Setter("Track.Position"),
			Setter("BitDepth"),
			Setter("Video_Delay")
		};
	}

	bool MediaInfoAPI::Session::extract_track(ITrack* dst, const dom::XmlNodePtr& src)
	{
		//std::ostringstream msg;
		//bool printed = false;
		for (auto && field : src->childNodes())
		{
			std::string name = field->nodeName();
			auto len = name.length();
			bool show = false;

			for (auto && known : names)
			{
				if (name == known.m_name)
				{
					if (!known(dst, src))
						return false;
					show = true;
					break;
				}
			}

			if (!show)
			{
				if (name.substr(0, 14) == "Format_Version" || name.substr(0, 14) == "Format_Profile" || name.substr(0, 5) == "Track")
					show = true;
			}

			if (!show)
				continue;

			//if (!m_printed)
			//{
			//	m_printed = true;
			//	msg << m_path << "\n";
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
