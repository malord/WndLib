#include "VerInfo.h"

#ifdef _MSC_VER
	#pragma warning(disable:4996)
#endif

namespace WndLib
{
	VerInfo::VerInfo()
	{
		_version[0] = _version[1] = _version[2] = _version[3] = -1;
		_title[0] = _copyright[0] = _comments[0] = 0;
	}

	bool VerInfo::Read(const TCHAR *filename)
	{
		DWORD zerohandle = 0;
		DWORD verinfosize = GetFileVersionInfoSize((LPTSTR) filename, &zerohandle);

		if (! verinfosize)
			return false;

		ByteArray verinfobuf;
		verinfobuf.Resize(verinfosize);

		if (! GetFileVersionInfo(filename, zerohandle, verinfosize, verinfobuf.Get()))
			return false;

		void *block = verinfobuf.Get();

		{
			{
				void *data;
				UINT datalen;
				TCHAR strbuf[512];

				// Extract version number
				if (VerQueryValue(block, TEXT("\\"), &data, &datalen))
				{
					const VS_FIXEDFILEINFO *ffi =
						(const VS_FIXEDFILEINFO *) data;

					_version[0] = (ffi->dwFileVersionMS >> 16);
					_version[1] = (ffi->dwFileVersionMS & 0xffff);
					_version[2] = (ffi->dwFileVersionLS >> 16);
					_version[3] = (ffi->dwFileVersionLS & 0xffff);
				}

				struct LangCodePage
				{
					WORD language;
					WORD codepage;
				};

				// Extract language code, which we need for remaining details
				if (VerQueryValue(block, TEXT("\\VarFileInfo\\Translation"), &data, &datalen))
				{
					LangCodePage lcp = *(const LangCodePage *) data;

					// Extract product name
					SNTPrintf(strbuf, WNDLIB_COUNTOF(strbuf),
						TEXT("\\StringFileInfo\\%04x%04x\\FileDescription"),
						lcp.language, lcp.codepage);
					if (VerQueryValue(block, strbuf, &data, &datalen))
						lstrcpy(_title, (LPCTSTR) data);

					// Extract copyright notice
					SNTPrintf(strbuf, WNDLIB_COUNTOF(strbuf),
						TEXT("\\StringFileInfo\\%04x%04x\\LegalCopyright"),
						lcp.language, lcp.codepage);
					if (VerQueryValue(block, strbuf, &data, &datalen))
						lstrcpy(_copyright, (LPCTSTR) data);

					// Extract comments
					SNTPrintf(strbuf, WNDLIB_COUNTOF(strbuf),
						TEXT("\\StringFileInfo\\%04x%04x\\Comments"),
						lcp.language, lcp.codepage);
					if (VerQueryValue(block, strbuf, &data, &datalen))
						lstrcpy(_comments, (LPCTSTR) data);
				}
			}
		}

		return true;
	}

	bool VerInfo::ReadCurrentModule()
	{
		TCHAR buf[2048];

		DWORD result = GetModuleFileName(GetModuleHandle(NULL), buf, WNDLIB_COUNTOF(buf));
		if (! result || result >= WNDLIB_COUNTOF(buf))
			return false;

		return Read(buf);
	}
}
