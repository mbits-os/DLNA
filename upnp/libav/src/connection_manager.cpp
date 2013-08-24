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
#include <media_server.hpp>
#include "media_server_internal.hpp"

namespace net { namespace ssdp { namespace import { namespace av {

	error_code ConnectionManager::GetCurrentConnectionInfo(const client_info_ptr& /*client*/,
	                                                       const http::http_request& /*http_request*/,
	                                                       /* IN  */ i4 /*ConnectionID*/,
	                                                       /* OUT */ i4& /*RcsID*/,
	                                                       /* OUT */ i4& /*AVTransportID*/,
	                                                       /* OUT */ std::string& /*ProtocolInfo*/,
	                                                       /* OUT */ std::string& /*PeerConnectionManager*/,
	                                                       /* OUT */ i4& /*PeerConnectionID*/,
	                                                       /* OUT */ A_ARG_TYPE_Direction& /*Direction*/,
	                                                       /* OUT */ A_ARG_TYPE_ConnectionStatus& /*Status*/)
	{
		return error::not_implemented;
	}

	error_code ConnectionManager::ConnectionComplete(const client_info_ptr& /*client*/,
	                                                 const http::http_request& /*http_request*/,
	                                                 /* IN  */ i4 /*ConnectionID*/)
	{
		return error::not_implemented;
	}

	error_code ConnectionManager::PrepareForConnection(const client_info_ptr& /*client*/,
	                                                   const http::http_request& /*http_request*/,
	                                                   /* IN  */ const std::string& /*RemoteProtocolInfo*/,
	                                                   /* IN  */ const std::string& /*PeerConnectionManager*/,
	                                                   /* IN  */ i4 /*PeerConnectionID*/,
	                                                   /* IN  */ A_ARG_TYPE_Direction /*Direction*/,
	                                                   /* OUT */ i4& /*ConnectionID*/,
	                                                   /* OUT */ i4& /*AVTransportID*/,
	                                                   /* OUT */ i4& /*RcsID*/)
	{
		return error::not_implemented;
	}

	error_code ConnectionManager::GetProtocolInfo(const client_info_ptr& /*client*/,
	                                              const http::http_request& /*http_request*/,
	                                              /* OUT */ std::string& Source,
	                                              /* OUT */ std::string& /*Sink*/)
	{
		Source =
			"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_SM,"
			"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_MED,"
			"http-get:*:image/jpeg:DLNA.ORG_PN=JPEG_LRG,"
			"http-get:*:audio/mpeg:DLNA.ORG_PN=MP3,"
			"http-get:*:audio/L16:DLNA.ORG_PN=LPCM,"
			"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_HD_24_AC3_ISO;SONY.COM_PN=AVC_TS_HD_24_AC3_ISO,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_24_AC3;SONY.COM_PN=AVC_TS_HD_24_AC3,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_24_AC3_T;SONY.COM_PN=AVC_TS_HD_24_AC3_T,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_PS_PAL,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_PS_NTSC,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_50_L2_T,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_60_L2_T,"
			"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_TS_SD_EU_ISO,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_EU,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_EU_T,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_50_AC3_T,"
			"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_TS_HD_50_L2_ISO;SONY.COM_PN=HD2_50_ISO,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_SD_60_AC3_T,"
			"http-get:*:video/mpeg:DLNA.ORG_PN=MPEG_TS_HD_60_L2_ISO;SONY.COM_PN=HD2_60_ISO,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_HD_50_L2_T;SONY.COM_PN=HD2_50_T,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=MPEG_TS_HD_60_L2_T;SONY.COM_PN=HD2_60_T,"
			"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_HD_50_AC3_ISO;SONY.COM_PN=AVC_TS_HD_50_AC3_ISO,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_50_AC3;SONY.COM_PN=AVC_TS_HD_50_AC3,"
			"http-get:*:video/mpeg:DLNA.ORG_PN=AVC_TS_HD_60_AC3_ISO;SONY.COM_PN=AVC_TS_HD_60_AC3_ISO,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_60_AC3;SONY.COM_PN=AVC_TS_HD_60_AC3,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_50_AC3_T;SONY.COM_PN=AVC_TS_HD_50_AC3_T,"
			"http-get:*:video/vnd.dlna.mpeg-tts:DLNA.ORG_PN=AVC_TS_HD_60_AC3_T;SONY.COM_PN=AVC_TS_HD_60_AC3_T,"
			"http-get:*:video/x-mp2t-mphl-188:*,"
			"http-get:*:*:*,"
			"http-get:*:video/*:*,"
			"http-get:*:audio/*:*,"
			"http-get:*:image/*:*";

		return error::no_error;
	}

	error_code ConnectionManager::GetCurrentConnectionIDs(const client_info_ptr& /*client*/,
	                                                      const http::http_request& /*http_request*/,
	                                                      /* OUT */ std::string& /*ConnectionIDs*/)
	{
		return error::not_implemented;
	}

}}}}
