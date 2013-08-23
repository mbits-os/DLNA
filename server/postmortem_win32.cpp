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

#include "postmortem.hpp"
#include <sdkddkver.h>
#include <Windows.h>
#include <DbgHelp.h>
#include <log.hpp>

namespace lan
{
	extern Log::Module APP;

	struct log : public Log::basic_log<log>
	{
		static const Log::Module& module() { return APP; }
	};
}

namespace dbg
{
	struct postmortem::impl
	{
		typedef BOOL (WINAPI *MINIDUMPWRITEDUMP)(
			HANDLE hProcess, DWORD ProcessId, HANDLE hFile, MINIDUMP_TYPE DumpType,
			PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
			PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
			PMINIDUMP_CALLBACK_INFORMATION CallbackParam);
		HINSTANCE hDbgHelp;
		MINIDUMPWRITEDUMP pMiniDumpWriteDump;
		impl()
			: hDbgHelp(0)
		{
		}
		~impl()
		{
		}

		bool load()
		{
			hDbgHelp = LoadLibrary(L"DbgHelp.dll");
			if (!hDbgHelp)
				return false;

			pMiniDumpWriteDump = (MINIDUMPWRITEDUMP)GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
			return pMiniDumpWriteDump != NULL;
		}

		static BOOL WINAPI callback(_Inout_ PVOID, _In_ PMINIDUMP_CALLBACK_INPUT, _Inout_ PMINIDUMP_CALLBACK_OUTPUT) { return TRUE; }

		void dump(EXCEPTION_POINTERS* pep)
		{
			// Open the file 

			HANDLE hFile = CreateFile(L"server.dmp", GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile == INVALID_HANDLE_VALUE)
			{
				lan::log::error() << "Could not create dump file (server.dmp)";
				return;
			}

			MINIDUMP_EXCEPTION_INFORMATION mdei;
			mdei.ThreadId = GetCurrentThreadId();
			mdei.ExceptionPointers = pep;
			mdei.ClientPointers = FALSE;

			MINIDUMP_CALLBACK_INFORMATION mci;
			mci.CallbackRoutine = (MINIDUMP_CALLBACK_ROUTINE) callback;
			mci.CallbackParam = 0;

			BOOL rv = pMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
				hFile, MiniDumpWithFullMemory, (pep != 0) ? &mdei : nullptr, nullptr, &mci);

			CloseHandle(hFile);

			if (rv)
				lan::log::error() << "Minidump server.dmp created";
			else
				lan::log::error() << "Minidump failed: " << GetLastError();
		}
	};

	static postmortem::impl* s_pimpl = nullptr;
	static LONG WINAPI dump(EXCEPTION_POINTERS* pep)
	{
		if (s_pimpl)
			s_pimpl->dump(pep);
		return EXCEPTION_CONTINUE_SEARCH;
	}

	postmortem::postmortem()
		: pimpl(new (std::nothrow) impl())
	{
		if (!pimpl)
		{
			lan::log::error() << "Could not create postmortem guard";
			return;
		}

		if (!pimpl->load())
		{
			lan::log::error() << "Could not load DbgHelp.dll";
			return;
		}

		s_pimpl = pimpl;

		SetUnhandledExceptionFilter(dump);
	}

	postmortem::~postmortem()
	{
		s_pimpl = nullptr;
		delete pimpl;
	}
}