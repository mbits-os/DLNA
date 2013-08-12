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
#include <network/interface.hpp>
#include <windows.h>
#include <winsock2.h>
#include <iphlpapi.h>

#pragma comment(lib, "IPHLPAPI.lib")

namespace net
{
	namespace iface
	{
		namespace
		{
			template <typename T>
			struct Table
			{
				typedef DWORD(WINAPI Fetcher)(T*, PULONG, BOOL);
				T* data;

				Table(Fetcher fetcher) : data((T*) malloc(sizeof(T)))
				{
					if (!data)
						return;

					ULONG size = sizeof(T);
					if (fetcher(data, &size, FALSE) == ERROR_INSUFFICIENT_BUFFER)
					{
						free(data);
						data = (T*) malloc(size);

						if (data == nullptr)
							return;
					}

					if (fetcher(data, &size, TRUE) != NO_ERROR)
					{
						free(data);
						data = nullptr;
					}
				}

				~Table()
				{
					free(data);
				}
			};
		}

		boost::asio::ip::address_v4 get_default_interface()
		{
			using boost::asio::ip::address_v4;
			Table<MIB_IFTABLE> ifaces(GetIfTable);
			Table<MIB_IPADDRTABLE> addresses(GetIpAddrTable);

			if (ifaces.data == nullptr || addresses.data == nullptr)
				return address_v4 { INADDR_ANY };

			for (DWORD i = 0; i < ifaces.data->dwNumEntries; i++)
			{
				auto row = ifaces.data->table + i;

				if (row->dwOperStatus != IF_OPER_STATUS_OPERATIONAL &&
					row->dwOperStatus != IF_OPER_STATUS_CONNECTED)
					continue;
				if (row->dwType == MIB_IF_TYPE_LOOPBACK)
					continue;

				for (DWORD j = 0; j < addresses.data->dwNumEntries; ++j)
				{
					auto addr_row = addresses.data->table + j;
					if (addr_row->dwIndex != row->dwIndex)
						continue;

					return address_v4 { ntohl(addr_row->dwAddr) };
				}
			}

			return address_v4 { INADDR_ANY };
		}
	}
}
