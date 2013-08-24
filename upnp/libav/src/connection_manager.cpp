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
			"http-get:*:image/jpeg:*,"
			"http-get:*:image/png:*,"

			"http-get:*:audio/vnd.dolby.dd-raw:*,"
			"http-get:*:audio/mp4:*,"
			"http-get:*:audio/3gpp:*,"
			"http-get:*:audio/x-sony-oma:*,"
			"http-get:*:audio/vnd.dlna.adts:*,"
			"http-get:*:audio/L16:*,"
			"http-get:*:audio/mpeg:*,"
			"http-get:*:audio/x-ms-wma:*,"

			"http-get:*:video/mpeg:*,"
			"http-get:*:video/mp4:*,"
			"http-get:*:video/vnd.dlna.mpeg-tts:*,"
			"http-get:*:video/x-ms-wmv:*";

		return error::no_error;
	}

	error_code ConnectionManager::GetCurrentConnectionIDs(const client_info_ptr& /*client*/,
	                                                      const http::http_request& /*http_request*/,
	                                                      /* OUT */ std::string& /*ConnectionIDs*/)
	{
		return error::not_implemented;
	}

}}}}
