// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the MEDIAINFO_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// MEDIAINFO_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef MEDIAINFO_EXPORTS
#define MEDIAINFO_API __declspec(dllexport)
#else
#define MEDIAINFO_API __declspec(dllimport)
#endif

#include <string>
#include <cctype>

namespace MediaInfo
{
	enum class TrackType
	{
		Unknown,
		General,
		Video,
		Audio,
		Text,
		Chapters,
		Image,
		Menu
	};

	std::istream& operator >> (std::istream& i, TrackType& type)
	{
		std::string s;
		i >> s;
		for (auto && c : s) c = std::tolower((unsigned char) c);

		if (s == "general")  { type = TrackType::General;  return i; }
		if (s == "video")    { type = TrackType::Video;    return i; }
		if (s == "audio")    { type = TrackType::Audio;    return i; }
		if (s == "text")     { type = TrackType::Text;     return i; }
		if (s == "chapters") { type = TrackType::Chapters; return i; }
		if (s == "image")    { type = TrackType::Image;    return i; }
		if (s == "menu")     { type = TrackType::Menu;     return i; }

		type = TrackType::Unknown;
		return i;
	}

	std::ostream& operator << (std::ostream& o, TrackType type)
	{
		switch (type)
		{
		case TrackType::General:  return o << "general";
		case TrackType::Video:    return o << "video";
		case TrackType::Audio:    return o << "audio";
		case TrackType::Text:     return o << "text";
		case TrackType::Chapters: return o << "chapters";
		case TrackType::Image:    return o << "image";
		case TrackType::Menu:     return o << "menu";
		case TrackType::Unknown:  return o << "unknown";
		};
		return o << "UNKNOWN(" << (int) type << ")";
	}

	struct ITrack
	{
		virtual TrackType get_type() const = 0;
		virtual int get_id() const = 0;
	};

	struct IContainer
	{
		virtual ITrack* create_track(TrackType type, int id) = 0;
	};

	typedef struct APIHandle{} *HMEDIAINFOAPI;
	typedef struct InfoHandle{} *HMEDIAINFO;

	MEDIAINFO_API HMEDIAINFOAPI __stdcall CreateApi();
	MEDIAINFO_API void __stdcall DestroyApi(HMEDIAINFOAPI hApi);
	MEDIAINFO_API HMEDIAINFO __stdcall CreateMediaInfo(HMEDIAINFOAPI hApi, LPCWSTR path);
	MEDIAINFO_API LPCWSTR __stdcall GetText(HMEDIAINFO hMediaInfo);
	MEDIAINFO_API bool __stdcall ExtractContainerDescription(HMEDIAINFO hMediaInfo, IContainer* env);
	MEDIAINFO_API void __stdcall DestroyMediaInfo(HMEDIAINFO hMediaInfo);

	struct API
	{
		HMEDIAINFOAPI m_hApi;
		API() : m_hApi(CreateApi()) {}
		~API() { DestroyApi(m_hApi); }
		std::wstring inform(const std::wstring& path) { return inform(path.c_str()); }
		std::wstring inform(LPCWSTR path)
		{
			if (!m_hApi)
				return std::wstring();

			auto info = CreateMediaInfo(m_hApi, path);
			if (!info)
				return std::wstring();

			std::wstring out { GetText(info) };
			DestroyMediaInfo(info);

			return out;
		}
		bool extract(const std::wstring& path, IContainer* env) { return extract(path.c_str(), env); }
		bool extract(LPCWSTR path, IContainer* env)
		{
			if (!m_hApi)
				return false;

			auto info = CreateMediaInfo(m_hApi, path);
			if (!info)
				return false;

			bool out { ExtractContainerDescription(info, env) };
			DestroyMediaInfo(info);

			return out;
		}
	};
}
