#include "WndLib.h"
#include <memory>
#include <algorithm>
#include <ShlObj.h>
#include <commdlg.h>

#ifdef _MSC_VER
	#pragma comment(lib, "comctl32.lib")
#endif

#ifndef WM_CHANGEUISTATE
	#define WM_CHANGEUISTATE 0x0127
	#define WM_UPDATEUISTATE 0x0128
	#define WM_QUERYUISTATE 0x0129
#endif

#ifndef UISF_HIDEFOCUS
	#define UISF_HIDEFOCUS 0x1
	#define UISF_HIDEACCEL 0x2
#endif

#ifndef UIS_SET
	#define UIS_SET 1
	#define UIS_CLEAR 2
	#define UIS_INITIALIZE 3
#endif

#if defined(_MSC_VER) && _MSC_VER >= 1400
	#pragma comment(linker,"\"/manifestdependency:type='win32' \
	name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
	processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

namespace
{
	//
	// 64-bit compatibility
	//

	#if _MSC_VER >= 1300
		#pragma warning(disable:4244)
		#pragma warning(disable:4312)
	#endif

	#if _MSC_VER >= 1300
		#pragma warning(default:4244)
		#pragma warning(default:4312)
	#endif

	//
	// Dialogue helpers
	//

	#pragma pack(push, 2)

	typedef struct
	{
		WORD dlgVer;
		WORD signature;
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
		WORD cDlgItems;
		short x;
		short y;
		short cx;
		short cy;

		// Followed by:
		// sz_Or_Ord menu;
		// sz_Or_Ord windowClass;
		// WCHAR title[titleLen];
		// // The following members exist only if the style member is
		// // set to DS_SETFONT or DS_SHELLFONT.
		// WORD pointsize;
		// WORD weight;
		// BYTE italic;
		// BYTE charset;
		// WCHAR typeface[stringLen];
	} DLGTEMPLATEEX;

	typedef struct
	{
		DWORD helpID;
		DWORD exStyle;
		DWORD style;
		short x;
		short y;
		short cx;
		short cy;
		DWORD id;
		// Followed by:
		// sz_Or_Ord windowClass;
		// sz_Or_Ord title;
		// WORD extraCount;
	} DLGITEMTEMPLATEEX;

	#pragma pack(pop)

	//
	// Font support
	//

	// Structure for font enumeration
	struct FontEnumInfo
	{
		LPCTSTR facename;
		bool found;
	};

	static int CALLBACK FontEnumProc(const LOGFONT *lf, const TEXTMETRIC *,
		DWORD, LPARAM lpData)
	{
		FontEnumInfo *enuminfo = (FontEnumInfo *) lpData;

		if (lstrcmpi(lf->lfFaceName, enuminfo->facename) == 0)
		{
			enuminfo->found = true;
			return FALSE; // stop searching
		}

		return TRUE; // continue
	}
}

namespace WndLib
{
	//
	// Strings
	//

	bool TCharStringFormat(TCHAR *buffer, size_t bufferSize, const TCHAR *format, ...)
	{
		va_list argptr;
		va_start(argptr, format);
		bool result = TCharStringFormatVA(buffer, bufferSize, format, argptr);
		va_end(argptr);
		return result;
	}

	bool TCharStringFormatVA(TCHAR *buffer, size_t bufferSize, const TCHAR *format, va_list argptr)
	{
		size_t length;
		return TCharStringFormatVA(&length, buffer, bufferSize, format, argptr);
	}

	bool TCharStringFormatVA(size_t *length, TCHAR *buffer, size_t bufferSize, const TCHAR *format, va_list argptr)
	{
		buffer[0] = 0;
		#ifdef _MSC_VER
		#pragma warning(disable:4996)
		#endif
		#ifdef WNDLIB_UNICODE
			ptrdiff_t result = _vsnwprintf(buffer, bufferSize, format, argptr);
		#else
			ptrdiff_t result = _vsnprintf(buffer, bufferSize, format, argptr);
		#endif
		#ifdef _MSC_VER
		#pragma warning(default:4996)
		#endif
		buffer[bufferSize - 1] = 0;
		if (result < 0 || result >= (ptrdiff_t) bufferSize)
		{
			*length = (size_t) -1;
			return false;
		}

		*length = result;
		return true;
	}

	bool TCharStringFormat(size_t *length, TCHAR *buffer, size_t bufferSize, const TCHAR *format, ...)
	{
		va_list argptr;
		va_start(argptr, format);
		bool result = TCharStringFormatVA(length, buffer, bufferSize, format, argptr);
		va_end(argptr);
		return result;
	}

	bool TCharStringCopy(TCHAR *buffer, size_t bufferSize, const TCHAR *src)
	{
		if (! bufferSize)
			return false;

		TCHAR *terminator = buffer + bufferSize - 1;

		while (*src) {
			if (buffer == terminator) {
				*buffer = 0;
				return false;
			}

			*buffer++ = *src++;
		}

		*buffer = 0;
		return true;
	}

	bool TCharStringAppend(TCHAR *buffer, size_t bufferSize, const TCHAR *src)
	{
		size_t length = lstrlen(buffer);

		if (length >= bufferSize)
			return false;

		return TCharStringCopy(buffer + length, bufferSize - length, src);
	}

	TCharString TCharFormat(LPCTSTR format, ...)
	{
		va_list argptr;
		va_start(argptr, format);
		TCharString result = TCharFormatVA(format, argptr);
		va_end(argptr);
		return result;
	}

	TCharString TCharFormatVA(LPCTSTR format, va_list argptr)
	{
		const size_t MAX_LENGTH = 1u << 16;
		TCharString temp(128, 0);
		for (;;)
		{
			va_list argptr2;
			WNDLIB_VA_COPY(argptr2, argptr);
			size_t length;
			bool ok = TCharStringFormatVA(&length, &temp[0], temp.size() - 1, format, argptr);
			va_end(argptr);

			if (ok)
				return TCharString(temp, 0, length);

			if (temp.size() > MAX_LENGTH)
				return TCharString();

			temp.resize(temp.size() * 2);
		}
	}

	std::string WideToChar(UINT codepage, const WCHAR *wstring)
	{
		int len = WideCharToMultiByte(codepage, 0, wstring, -1, 0, 0, 0, 0);		
		if (len < 0) 
		{
			WNDLIB_ASSERT(0);
			return std::string();
		}

		std::string string(len, 0);
		WideCharToMultiByte(codepage, 0, wstring, -1, &string[0], len, 0, 0);
		string.resize(len - 1);
		return string;
	}

	WCharString CharToWide(UINT codepage, const char *string)
	{
		int len = MultiByteToWideChar(codepage, 0, string, -1, 0, 0);
		if (len < 0) 
		{
			WNDLIB_ASSERT(0);
			return WCharString();
		}

		WCharString wstring(len, 0);
		MultiByteToWideChar(codepage, 0, string, -1, &wstring[0], len);
		wstring.resize(len - 1);
		return wstring;
	}

	WCharString CharToWide(UINT codepage, const std::string &string)
	{
		return CharToWide(codepage, string.c_str());
	}

	std::string WideToChar(UINT codepage, const WCharString &wstring)
	{
		return WideToChar(codepage, wstring.c_str());
	}

	#ifdef WNDLIB_UNICODE

		std::string ToUTF8(LPCTSTR string)
		{
			return WideToChar(CP_UTF8, string);
		}

		std::string ToUTF8(const TCharString &string)
		{
			return WideToChar(CP_UTF8, string.c_str());
		}

		TCharString FromUTF8(const char *string)
		{
			return CharToWide(CP_UTF8, string);
		}

		TCharString FromUTF8(const std::string string)
		{
			return CharToWide(CP_UTF8, string.c_str());
		}

	#else

		std::string ToUTF8(LPCTSTR string)
		{
			return WideToChar(CP_UTF8, CharToWide(CP_ACP, string));
		}

		std::string ToUTF8(const TCharString &string)
		{
			return WideToChar(CP_UTF8, CharToWide(CP_ACP, string.c_str()));
		}

		TCharString FromUTF8(const char *string)
		{
			return WideToChar(CP_ACP, CharToWide(CP_UTF8, string));
		}

		TCharString FromUTF8(const std::string string)
		{
			return WideToChar(CP_ACP, CharToWide(CP_UTF8, string.c_str()));
		}

	#endif

	//
	// Instance handle
	//

	namespace Private
	{
		HINSTANCE hInstance;
	}

	//
	// Utility functions
	//

	bool FilterMessage(MSG *msg)
	{
		Wnd *target = Wnd::FindWnd(msg->hwnd);
		if (target && target->FilterMessage(msg))
			return true;

		HWND hwnd = msg->hwnd;
		for (;;)
		{
			HWND parent = GetParent(hwnd);
			if (! parent)
				break;

			hwnd = parent;

			if (Wnd *found = Wnd::FindWnd(hwnd))
			{
				if (found->FilterMessage(msg))
					return true;
			}
		}

		if (target)
			return target->PreTranslateMessage(msg);

		return false;
	}

	void CentreWindow(HWND parent, HWND child)
	{
		RECT rparent;
		RECT rchild;

		if (parent)
			::GetWindowRect(parent, &rparent);
		else
			SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID) &rparent, FALSE);

		::GetWindowRect(child, &rchild);

		::SetWindowPos(child, NULL,
			((rparent.right-rparent.left) - (rchild.right-rchild.left)) / 2 + rparent.left,
			((rparent.bottom-rparent.top) - (rchild.bottom-rchild.top)) / 2 + rparent.top,
			0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	void ClampToDesktop(HWND window)
	{
		RECT rwnd;
		GetWindowRect(window, &rwnd);

		RECT rdesk;
		SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID) &rdesk, FALSE);

		if (rwnd.right > rdesk.right)
			OffsetRect(&rwnd, rdesk.right - rwnd.right, 0);

		if (rwnd.bottom > rdesk.bottom)
			OffsetRect(&rwnd, 0, rdesk.bottom - rwnd.bottom);

		if (rwnd.left < rdesk.left)
			OffsetRect(&rwnd, rdesk.left - rwnd.left, 0);

		if (rwnd.top < rdesk.top)
			OffsetRect(&rwnd, 0, rdesk.top - rwnd.top);

		SetWindowPos(window, NULL, rwnd.left, rwnd.top, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE);
	}

	bool IsAncestorOfWindow(HWND parent, HWND child)
	{
		if (child == parent)
			return true;

		for (;;)
		{
			child = ::GetParent(child);
			if (child == parent)
				return true;
			if (! child)
				return false;
		}
	}

	double GetDPIScale(HDC hdc)
	{
		BOOL releaseDC = FALSE;

		if (! hdc)
		{
			hdc = GetDC(NULL);
			releaseDC = TRUE;
		}

		int y = GetDeviceCaps(hdc, LOGPIXELSY);
		double scale = (double) y / 96.0;

		if (releaseDC)
			ReleaseDC(NULL, hdc);

		return scale;
	}

	HFONT EasyCreateFont(HDC hdc, LPCTSTR facename, int decipts, int flags)
	{
		POINT pt;
		bool releaseDC;
		HFONT hfont;
		FontEnumInfo enumInfo;
		int logpixelsy;

		if (hdc == NULL)
		{
			hdc = GetDC(NULL);
			releaseDC = true;
		}
		else
			releaseDC = false;

		if (flags & EASYFONT_MUSTEXIST)
		{
			enumInfo.facename = facename;
			enumInfo.found = false;
   			EnumFonts(hdc, facename, FontEnumProc, (LPARAM) &enumInfo);

			if (! enumInfo.found)
			{
				hfont = NULL;
				goto failed;
			}
		}

		pt.x = 0;
		logpixelsy = GetDeviceCaps(hdc, LOGPIXELSY);
		pt.y = (int)((decipts*logpixelsy+320)/720);
		//DPtoLP(hdc, &pt, 1);
		hfont = CreateFont(-pt.y, 0, 0, 0,
			(flags & EASYFONT_BOLD) ? FW_BOLD : FW_NORMAL,
			(flags & EASYFONT_ITALIC) ? TRUE : FALSE,
			(flags & EASYFONT_UNDERLINE) ? TRUE : FALSE,
			(flags & EASYFONT_STRIKEOUT) ? TRUE : FALSE,
			0, 0, 0, PROOF_QUALITY, 0, facename);

	failed:

		if (releaseDC)
			ReleaseDC(NULL, hdc);

		return hfont;
	}

	static HFONT CreateFontFromNonClientMetrics(LOGFONT NONCLIENTMETRICS::*font)
	{
		NONCLIENTMETRICS ncm;
		memset(&ncm, 0, sizeof(ncm));
		ncm.cbSize = sizeof(ncm);

		if (! SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, FALSE))
			return NULL;

		return CreateFontIndirect(&(ncm.*font));
	}

	HFONT CreateShellFont()
	{
		return CreateFontFromNonClientMetrics(&NONCLIENTMETRICS::lfMessageFont);
	}

	HFONT CreateMenuFont()
	{
		return CreateFontFromNonClientMetrics(&NONCLIENTMETRICS::lfMenuFont);
	}

	HFONT CreateStatusFont()
	{
		return CreateFontFromNonClientMetrics(&NONCLIENTMETRICS::lfStatusFont);
	}

	void InitAllCommonControls()
	{
		INITCOMMONCONTROLSEX controls;
		memset(&controls, 0, sizeof(controls));
		controls.dwSize = sizeof(controls);
		controls.dwICC = 0xffff;
		InitCommonControlsEx(&controls);
	}

	//
	// Common dialogs
	//

	TCharString BrowseForFolder(HWND parent, LPCTSTR prompt)
	{
		for (;;)
		{
			BROWSEINFO browseInfo;
			ZeroMemory(&browseInfo, sizeof (BROWSEINFO));

			browseInfo.hwndOwner = parent;
			browseInfo.pidlRoot = NULL;
			browseInfo.pszDisplayName = NULL;
			browseInfo.lpszTitle = prompt;
			browseInfo.ulFlags = BIF_RETURNONLYFSDIRS;
			browseInfo.lpfn = NULL;
			browseInfo.lParam = NULL;

			LPITEMIDLIST idList = SHBrowseForFolder (&browseInfo);
			if (! idList) {
				return TCharString();
			}

			TCHAR path[MAX_PATH];
			if (! SHGetPathFromIDList(idList, path)) {
				::MessageBox(parent, TEXT("Please select a filesystem folder."), NULL, MB_OK | MB_ICONSTOP);
				continue;
			}

			return path;
		}
	}

	TCharString BrowseForExistingFile(HWND parent, LPCTSTR title, LPCTSTR initialPath, LPCTSTR filters, int initialFilter, LPCTSTR initialDir)
	{
		OPENFILENAME ofn;
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = parent;
		TCHAR szFile[260];
		TCharStringCopy(szFile, WNDLIB_COUNTOF(szFile), initialPath ? initialPath : TEXT(""));
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = sizeof(szFile);
		ofn.lpstrFilter = filters;
		ofn.nFilterIndex = initialFilter;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = initialDir;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
		ofn.lpstrTitle = title;

		if (! GetOpenFileName(&ofn)) 
			return TCharString();

		return ofn.lpstrFile;
	}

	TCharString BrowseForNewFile(HWND parent, LPCTSTR title, LPCTSTR initialPath, LPCTSTR filters, int initialFilter, LPCTSTR initialDir)
	{
		OPENFILENAME ofn;
		memset(&ofn, 0, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = parent;
		TCHAR szFile[260];
		TCharStringCopy(szFile, WNDLIB_COUNTOF(szFile), initialPath);
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = WNDLIB_COUNTOF(szFile);
		ofn.lpstrFilter = filters;
		ofn.nFilterIndex = initialFilter;
		ofn.lpstrFileTitle = NULL;
		ofn.nMaxFileTitle = 0;
		ofn.lpstrInitialDir = initialDir;
		ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT;
		ofn.lpstrTitle = title;

		if (! GetSaveFileName(&ofn)) 
			return TCharString();

		return ofn.lpstrFile;
		
	}

	//
	// Wnd
	//

	Wnd::WndMap Wnd::wndMap;
	CriticalSection Wnd::mapLock;

	static Wnd *creatingWnd = 0;
	static CriticalSection creationLock;

	Wnd::Wnd()
	{
		Construct();
	}

	Wnd::Wnd(HWND hwnd, bool subclass)
	{
		Construct();
		Attach(hwnd, subclass);
	}

	void Wnd::Construct()
	{
		_hwnd = NULL;
		_prevproc = NULL;
		_selfDestruct = false;
	}

	Wnd::~Wnd()
	{
		Unmap(this);
	}

	void Wnd::SelfDestruct()
	{
		delete this;
	}

	void Wnd::Attach(HWND hwnd, bool subclass)
	{
		WNDLIB_ASSERT(hwnd != NULL);

		Detach();

		bool alreadyAttached = FindWnd(hwnd) != NULL;
		if (! alreadyAttached)
			Map(hwnd, this);

		_hwnd = hwnd;

		if (subclass)
		{
			_prevproc = HelpGetWndProc(_hwnd);
			HelpSetWndProc(_hwnd, &SubclassWndProc);
		}
	}

	void Wnd::SetHWnd(HWND hwnd)
	{
		_hwnd = hwnd;
	}

	HWND Wnd::Detach()
	{
		if (_prevproc)
		{
			if (HelpGetWndProc(_hwnd) == &SubclassWndProc)
			{
				HelpSetWndProc(_hwnd, _prevproc);
			}
			else
			{
				OutputDebugStringA(
					"Wnd::Detach: unable to un-subclass window "
					"(using window's window class).\r\n");
				HelpSetWndProc(_hwnd, HelpGetClassWndProc(_hwnd));
			}

			_prevproc = NULL;
		}

		HWND hwnd = _hwnd;
		Unmap(this);
		return hwnd;
	}

	BOOL Wnd::DestroyWindow()
	{
		BOOL result = ::DestroyWindow(GetHWnd());

		Unmap(this);

		_prevproc = NULL;

		return result;
	}

	LPCTSTR Wnd::GetClassName()
	{
		return TEXT("Wnd");
	}

	void Wnd::GetWndClass(WNDCLASSEX *wc)
	{
		memset(wc, 0, sizeof(*wc));

		wc->cbSize = sizeof(*wc);
		wc->cbClsExtra = 0;
		wc->cbWndExtra = sizeof(Wnd *);
		wc->hbrBackground = (HBRUSH) (COLOR_3DFACE + 1);
		wc->hCursor = LoadCursor(NULL, IDC_ARROW);
		wc->hIcon = LoadIcon(NULL, IDI_WINLOGO);
		wc->hIconSm = LoadIcon(NULL, IDI_WINLOGO);
		wc->hInstance = GetHInstance();
		wc->lpfnWndProc = (WNDPROC) &StaticWndProc;
		wc->lpszMenuName = NULL;
		wc->style = CS_HREDRAW | CS_VREDRAW;
	}

	HWND Wnd::DoCreateWindowEx(DWORD exStyle, LPCTSTR className, LPCTSTR windowName,
		DWORD style, int x, int y, int cx, int cy, HWND parent, HMENU menu,
		HINSTANCE instance, LPVOID param)
	{
	#if 0
		if (exStyle & WS_EX_MDICHILD)
		{
			exStyle &= ~WS_EX_MDICHILD;

			MDICREATESTRUCT cs;

			cs.hOwner = (HINSTANCE) ::GetWindowLong(parent, GWL_HINSTANCE);
			cs.szClass = className;
			cs.szTitle = windowName;
			cs.x = x;
			cs.y = y;
			cs.cx = cx;
			cs.cy = cy;
			cs.style = style;
			cs.lParam = (LPARAM) param;

			HWND child = (HWND) ::SendMessage(parent, WM_MDICREATE, 0, (LPARAM) &cs);
			if (! child)
				return NULL;

			::SetWindowLong(child, GWL_EXSTYLE, exStyle);
			::SetWindowPos(child, NULL, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE | SWP_NOREDRAW | SWP_FRAMECHANGED);

			return child;
		}
		else
	#endif
		{
			return CreateWindowEx(exStyle, className, windowName, style, x, y,
				cx, cy, parent, menu, instance, param);
		}
	}

	WNDPROC Wnd::GetDefWindowProc(const CREATESTRUCT &cs)
	{
		if (cs.dwExStyle & WS_EX_MDICHILD)
			return ::DefMDIChildProc;
		else
			return ::DefWindowProc;
	}

	bool Wnd::CreateIndirect(const CREATESTRUCT &cs, bool subclass)
	{
		// If "true", then we will have to attach to the newly created window.
		bool attach = true;

		// Work out which DefWindowProc to use
		_defWindowProc = GetDefWindowProc(cs);

		// Check the window class
		if (cs.hInstance == GetHInstance())
		{
			WNDCLASSEX wc;
			memset(&wc, 0, sizeof(wc));
			wc.cbSize = sizeof(wc);

			if (! GetClassInfoEx(cs.hInstance, cs.lpszClass, &wc))
			{
				// Need to register the class
				GetWndClass(&wc);

				wc.lpszClassName = GetClassName();

				if (! RegisterClassEx(&wc))
					return false;
			}

			if (wc.lpfnWndProc == &StaticWndProc)
				attach = false;
		}

		if (attach)
		{
			HWND hwnd = DoCreateWindowEx(cs.dwExStyle, cs.lpszClass, cs.lpszName, cs.style,
				cs.x, cs.y, cs.cx, cs.cy, cs.hwndParent, cs.hMenu, cs.hInstance,
				cs.lpCreateParams);

			if (! hwnd)
			{
				OutputDebugStringA(
					"Wnd::CreateIndirect: failed to create window.\r\n");
				return false;
			}

			Attach(hwnd, subclass);
			return true;
		}
		else
		{
			// CriticalSections are recursive mutexes, so our thread can create
			// multiple window while holding the lock.
			CriticalSection::ScopedLock lock(creationLock);
			creatingWnd = this;

			HWND hwnd = DoCreateWindowEx(cs.dwExStyle, cs.lpszClass, cs.lpszName, cs.style,
				cs.x, cs.y, cs.cx, cs.cy, cs.hwndParent, cs.hMenu, cs.hInstance,
				cs.lpCreateParams);

			if (creatingWnd != 0)
			{
				OutputDebugStringA(
					"Wnd::CreateIndirect: failed to create window.\r\n");
				return false;
			}

			if (hwnd == NULL)
			{
				OutputDebugStringA(
					"Wnd::CreateIndirect: WM_CREATE or WM_NCCREATE failed.\r\n");
				return false;
			}

			return true;
		}
	}

	LRESULT WINAPI Wnd::StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		Wnd *wnd = (Wnd *) HelpGetWindowPtr(hwnd, 0);
		if (! wnd)
		{
			wnd = creatingWnd;
			creatingWnd = NULL;

			if (! wnd)
			{
				OutputDebugStringA("Wnd::StaticWndProc: used incorrectly.\r\n");
				return 0;
			}

			Map(hwnd, wnd);

			HelpSetWindowPtr(hwnd, 0, (const void *) wnd);
			wnd->_hwnd = hwnd;
		}

		LRESULT result = wnd->WndProc(msg, wparam, lparam);

		if (msg == WM_NCDESTROY)
		{
			Unmap(wnd);

			if (wnd->_selfDestruct)
			{
				wnd->SelfDestruct();
			}
		}

		return result;
	}

	bool Wnd::CreateEx(DWORD exStyle, LPCTSTR windowName, DWORD style,
		int x, int y, int width, int height, HWND parent, HMENU menu,
		HINSTANCE instance, LPVOID params, bool subclass)
	{
		CREATESTRUCT cs;

		cs.dwExStyle = exStyle;
		cs.lpszName = windowName;
		cs.style = style;
		cs.x = x;
		cs.y = y;
		cs.cx = width;
		cs.cy = height;
		cs.hwndParent = parent;
		cs.hMenu = menu;
		cs.hInstance = instance;
		cs.lpCreateParams = params;

		cs.lpszClass = GetClassName();

		return CreateIndirect(cs, subclass);
	}

	LRESULT Wnd::WndProc(UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (_prevproc)
		{
   			return ::CallWindowProc(_prevproc, GetHWnd(), msg, wparam, lparam);
		}

		return (*_defWindowProc) (GetHWnd(), msg, wparam, lparam);
	}

	bool Wnd::PreTranslateMessage(MSG *)
	{
		return false;
	}

	bool Wnd::FilterMessage(MSG *)
	{
		return false;
	}

	void Wnd::Map(HWND hwnd, Wnd *wnd)
	{
		CriticalSection::ScopedLock lock(mapLock);
		wndMap[hwnd] = wnd;
	}

	void Wnd::Unmap(Wnd *wnd)
	{
		CriticalSection::ScopedLock lock(mapLock);

		WndMap::iterator i = wndMap.find(wnd->GetHWnd());
		if (i != wndMap.end())
			wndMap.erase(i);
	}

	Wnd *Wnd::FindWnd(HWND hwnd)
	{
		CriticalSection::ScopedLock lock(mapLock);
		return wndMap[hwnd];
	}

	LRESULT WINAPI Wnd::SubclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (Wnd *wnd = FindWnd(hwnd))
		{
			LRESULT result = wnd->WndProc(msg, wparam, lparam);

			if (msg == WM_NCDESTROY)
			{
				Unmap(wnd);

				if (wnd->_selfDestruct)
				{
					wnd->SelfDestruct();
				}
			}

			return result;
		}

		return ::DefWindowProc(hwnd, msg, wparam, lparam);
	}

	void Wnd::SetClientAreaSize(int cx, int cy)
	{
		RECT rwnd;
		RECT rcli;

		GetWindowRect(&rwnd);
		GetClientRect(&rcli);

		SetWindowPos(NULL, 0, 0,
			(rwnd.right - rwnd.left) - (rcli.right - rcli.left) + cx,
			(rwnd.bottom - rwnd.top) - (rcli.bottom - rcli.top) + cy,
			SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);

		GetClientRect(&rcli);

		// Second try in case menu bar changes size of client area
		if (rcli.right - rcli.left != cx || rcli.bottom - rcli.top != cy)
		{
			GetWindowRect(&rwnd);

			SetWindowPos(NULL, 0, 0,
				(rwnd.right - rwnd.left) - (rcli.right - rcli.left) + cx,
				(rwnd.bottom - rwnd.top) - (rcli.bottom - rcli.top) + cy,
				SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOMOVE);
		}
	}

	void Wnd::GetSize(SIZE *sz)
	{
		WNDLIB_ASSERT(0 != sz);

		RECT rect;
		GetWindowRect(&rect);
		sz->cx = rect.right - rect.left;
		sz->cy = rect.bottom - rect.top;
	}

	bool Wnd::IsMouseInWindow()
	{
		POINT pt;

		GetCursorPos(&pt);
		ScreenToClient(&pt);

		RECT rcli;
		GetClientRect(&rcli);

		return PtInRect(&rcli, pt) != FALSE;
	}

	int Wnd::MessageBox(LPCTSTR text, UINT type)
	{
		TCHAR title[256] = { 0 };
		GetWindowText(title, WNDLIB_COUNTOF(title));
		return ::MessageBox(GetHWnd(), text, title, type);
	}

	bool Wnd::GetDlgItemInt(int id, int *out)
	{
		WNDLIB_ASSERT(0 != out);

		BOOL translated;
		*out = (INT)::GetDlgItemInt(GetHWnd(), id, &translated, TRUE);

		return translated != FALSE;
	}

	bool Wnd::GetDlgItemInt(int id, unsigned *out)
	{
		WNDLIB_ASSERT(0 != out);

		BOOL translated;
		*out = (UINT)::GetDlgItemInt(GetHWnd(), id, &translated, FALSE);

		return translated != FALSE;
	}

	void Wnd::SendMessageToDescendants(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, bool deep)
	{
		HWND child = ::GetWindow(hwnd, GW_CHILD);
		for (; child; child = ::GetWindow(child, GW_HWNDNEXT))
		{
			::SendMessage(child, msg, wparam, lparam);
			if (deep)
				SendMessageToDescendants(child, msg, wparam, lparam, true);
		}
	}

	TCharString Wnd::GetWindowText()
	{
		TCharString string;
		string.resize(GetTextLength());
		if (! string.empty())
			GetWindowText(&string[0], string.size() + 1);
		return string;
	}

	int Wnd::GetFontHeightForWindow(HFONT hFont)
	{
		TEXTMETRIC tm;
		HDC hdc = GetDC();
		HFONT hOldFont = (HFONT) SelectObject(hdc, hFont);
		GetTextMetrics(hdc, &tm);
		SelectObject(hdc, hOldFont);
		ReleaseDC(hdc);

		return tm.tmHeight;
	}

	int Wnd::GetAverageCharWidthForWindow(HFONT hFont)
	{
		TEXTMETRIC tm;
		HDC hdc = GetDC();
		HFONT hOldFont = (HFONT) SelectObject(hdc, hFont);
		GetTextMetrics(hdc, &tm);
		SelectObject(hdc, hOldFont);
		ReleaseDC(hdc);

		return tm.tmAveCharWidth;
	}

	TCharString Wnd::GetDlgItemText(int id)
	{
		TCharString string;
		string.resize(128);
		for (;;) {
			UINT uGot = GetDlgItemText(id, (TCHAR *) &string[0], string.size() + 1);

			if (uGot < string.size()) // Intentional off by one to account for \0
				return string;

			string.resize(string.size() * 2);
		}
	}

	//
	// MainWnd
	//

	WND_WM_BEGIN(MainWnd, Wnd)
		WND_WM(WM_CLOSE, OnClose)
		WND_WM(WM_DESTROY, OnDestroy)
	WND_WM_END()

	LRESULT MainWnd::OnClose(UINT, WPARAM, LPARAM)
	{
		DestroyWindow();
		return 0;
	}

	LRESULT MainWnd::OnDestroy(UINT msg, WPARAM wparam, LPARAM lparam)
	{
		PostQuitMessage(0);
		return BaseWndProc(msg, wparam, lparam);
	}

	bool MainWnd::FilterMessage(MSG *msg)
	{
		if (_accel.TranslateAccelerator(GetHWnd(), msg))
			return true;

		if (IsDialogMessage(GetHWnd(), msg))
			return true;

		return false;
	}

	//
	// Dlg
	//

	Dlg::Dlg() :
		_resourceinst(GetHInstance()),
		_resourcename(0)
	{
	}

	Dlg::Dlg(HWND attach, bool subclass) :
		_resourceinst(GetHInstance()),
		_resourcename(0)
	{
		Attach(attach, subclass);
	}

	Dlg::Dlg(UINT resourceid)
	{
		SetResource(resourceid);
	}

	Dlg::Dlg(LPCTSTR resourceid)
	{
		SetResource(resourceid);
	}

	Dlg::Dlg(HINSTANCE inst, UINT resourceid)
	{
		SetResource(inst, resourceid);
	}

	Dlg::Dlg(HINSTANCE inst, LPCTSTR resourceid)
	{
		SetResource(inst, resourceid);
	}

	bool Dlg::Create(HWND parent)
	{
		INT_PTR result;
		return DoDlg(parent, 0, 0, false, &result);
	}

	bool Dlg::CreateSetFont(HWND parent, LPCTSTR fontName, unsigned fontPointSize)
	{
		INT_PTR result;
		return DoDlg(parent, fontName, fontPointSize, false, &result);
	}

	INT_PTR Dlg::DoModal(HWND parent)
	{
		INT_PTR result;
		if (! DoDlg(parent, 0, 0, true, &result))
			return IDCANCEL;

		SetHWnd(NULL);

		return result;
	}

	INT_PTR Dlg::DoModalSetFont(HWND parent, LPCTSTR fontName, unsigned fontPointSize)
	{
		INT_PTR result;
		if (! DoDlg(parent, fontName, fontPointSize, true, &result))
			return IDCANCEL;

		SetHWnd(NULL);

		return result;
	}

	bool Dlg::DoDlg(HWND parent, LPCTSTR fontName, unsigned fontPointSize, bool modal, INT_PTR *result_out)
	{
		if (fontPointSize)
		{
			HRSRC rsrc = FindResource(_resourceinst, Dlg::_resourcename, RT_DIALOG);
			if (! rsrc)
				return false;

			DWORD size = SizeofResource(_resourceinst, rsrc);

			// Allocate room for new dialogue template
			HGLOBAL newGlobal = GlobalAlloc(GMEM_FIXED, size + lstrlen(fontName) * 4 + 16);
			LPWORD output = (LPWORD) GlobalLock(newGlobal);

			HGLOBAL global = LoadResource(_resourceinst, rsrc);
			LPVOID locked = LockResource(global);

			LPDLGTEMPLATE templ = (LPDLGTEMPLATE) locked;
			LPWORD wordptr = (LPWORD) templ;

			DLGTEMPLATEEX *templex = (DLGTEMPLATEEX *) templ;

			bool extended = templex->signature == 0xffff;

			// Warning: This may actually point to a DLGTEMPLATEEX
			LPDLGTEMPLATE newTempl = (LPDLGTEMPLATE) output;

			if (extended)
			{
				wordptr = (LPWORD) ((DLGTEMPLATEEX *) templ + 1);

				DLGTEMPLATEEX *newTemplex = (DLGTEMPLATEEX *) newTempl;
				newTemplex = (DLGTEMPLATEEX *) output;
				*newTemplex = *templex;
				newTemplex->style |= DS_SETFONT;
				output = (LPWORD) AlignPtr((LPWORD) (newTemplex + 1), 2);
				newTempl = (LPDLGTEMPLATE) newTemplex;
			}
			else
			{
				wordptr = (LPWORD) (templ + 1);
				*newTempl = *templ;
				newTempl->style |= DS_SETFONT;
				output = (LPWORD) AlignPtr((LPWORD) (newTempl + 1), 2);
			}

			unsigned count;

			// Menu?
			while (0 != (*output++ = *wordptr++))
				{}

			// Class?
			while (0 != (*output++ = *wordptr++))
				{}

			// Title?
			while (0 != (*output++ = *wordptr++))
				{}

			// Font?
			if (extended)
			{
				if (templex->style & DS_SETFONT)
				{
					// Extended dialogue template (DLGTEMPLATEEX) has
					// size, face, weight, italicness and charset

					// Point size
					++wordptr;

					// Weight
					++wordptr;

					// Italic/charset
					++wordptr;

					// Name
					while (*wordptr++)
						{}
				}
			}
			else
			{
				if (templ->style & DS_SETFONT)
				{
					// Original DLGTEMPLATE only had size and face

					// Point size
					++wordptr;

					// Name
					while (*wordptr++)
						{}
				}
			}

			// Output font
			if (extended)
			{
				*output++ = (WORD) fontPointSize;
				*output++ = (WORD) 0;
				((BYTE *) output)[0] = 0;
				((BYTE *) output)[1] = 0;
				++output;
			}
			else
			{
				*output++ = (WORD) fontPointSize;
			}

			#ifndef WNDLIB_UNICODE
				output += MultiByteToWideChar(CP_ACP, 0, fontName, -1, (LPWSTR) output, 50);
				*output = 0;
			#else
				lstrcpy((LPWSTR) output, fontName);
				output += lstrlen(fontName) + 1;
			#endif

			// Controls
			if (extended)
			{
				count = templex->cDlgItems;
				while (count--)

				{
					wordptr = (LPWORD) AlignPtr(wordptr, 4);
					DLGITEMTEMPLATEEX *item = (DLGITEMTEMPLATEEX *) wordptr;
					wordptr = (LPWORD) (item + 1);

					output = (LPWORD) AlignPtr(output, 4);
					DLGITEMTEMPLATEEX *newItem = (DLGITEMTEMPLATEEX *) output;
					output = (LPWORD) (newItem + 1);

					*newItem = *item;

					// Window class
					output = (LPWORD) AlignPtr(output, 2);
					if (*wordptr == 0xffff)
					{
						*output++ = *wordptr++;
						*output++ = *wordptr++;
					}
					else
					{
						while (0 != (*output++ = *wordptr++))
							{}
					}

					// Title or resource ordinal
					output = (LPWORD) AlignPtr(output, 2);
					if (*wordptr == 0xffff)
					{
						*output++ = *wordptr++;
						*output++ = *wordptr++;
					}
					else
					{
						while (0 != (*output++ = *wordptr++))
							{}
					}

					// Data
					output = (LPWORD) AlignPtr(output, 2);
					size_t extralen = *wordptr++;
					*output++ = (WORD) extralen;
					memcpy(output, wordptr, extralen);
					output = (WORD *) ((BYTE *) output + extralen);
					wordptr = (WORD *) ((BYTE *) wordptr + extralen);
				}
			}
			else
			{
				count = templ->cdit;
				while (count--)
				{
					wordptr = (LPWORD) AlignPtr(wordptr, 4);
					LPDLGITEMTEMPLATE item = (LPDLGITEMTEMPLATE) wordptr;
					wordptr = (LPWORD) (item + 1);

					output = (LPWORD) AlignPtr(output, 4);
					LPDLGITEMTEMPLATE newItem = (LPDLGITEMTEMPLATE) output;
					output = (LPWORD) (newItem + 1);

					*newItem = *item;

					// Window class
					output = (LPWORD) AlignPtr(output, 2);
					if (*wordptr == 0xffff)
					{
						*output++ = *wordptr++;
						*output++ = *wordptr++;
					}
					else
					{
						while (0 != (*output++ = *wordptr++))
							{}
					}

					// Title or resource ordinal
					output = (LPWORD) AlignPtr(output, 2);
					if (*wordptr == 0xffff)
					{
						*output++ = *wordptr++;
						*output++ = *wordptr++;
					}
					else
					{
						while (0 != (*output++ = *wordptr++))
							{}
					}

					// Data
					output = (LPWORD) AlignPtr(output, 2);
					size_t extralen = *wordptr++;
					*output++ = (WORD) extralen;
					memcpy(output, wordptr, extralen);
					output = (WORD *) ((BYTE *) output + extralen);
					wordptr = (WORD *) ((BYTE *) wordptr + extralen);
				}
			}

			if (modal)
			{
				_modeless = false;
				INT_PTR result = ::DialogBoxIndirectParam(_resourceinst, newTempl, parent, (DLGPROC) &StaticDlgProc, (LPARAM) this);
				GlobalFree(newGlobal);
				*result_out = result;
				return true;
			}
			else
			{
				_modeless = true;
				HWND hwnd = ::CreateDialogIndirectParam(_resourceinst, newTempl, parent,
					(DLGPROC) &StaticDlgProc, (LPARAM) this);
				GlobalFree(newGlobal);
				return hwnd != NULL;
			}
		}
		else
		{
			if (modal)
			{
				_modeless = false;
				INT_PTR result = ::DialogBoxParam(_resourceinst, _resourcename, parent, (DLGPROC) &StaticDlgProc, (LPARAM) this);
				*result_out = result;
				return true;
			}
			else
			{
				_modeless = true;
				HWND hwnd = ::CreateDialogParam(_resourceinst, _resourcename, parent,
					(DLGPROC) &StaticDlgProc, (LPARAM) this);
				return hwnd != NULL;
			}
		}
	}

	LRESULT WINAPI Dlg::StaticDlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		Dlg *wnd;

		if (msg == WM_INITDIALOG)
		{
			wnd = (Dlg *) lparam;
			if (! wnd)
			{
				OutputDebugStringA(
					"Dlg::StaticDlgProc: Wnd parameter to WM_INITDIALOG is null.\r\n");
				return 0;
			}

			Map(hwnd, wnd);

			wnd->SetHWnd(hwnd);
			HelpSetWindowPtr(hwnd, DWLP_USER, (const void *) wnd);
		}

		wnd = (Dlg *) HelpGetWindowPtr(hwnd, DWLP_USER);
		if (! wnd)
			return FALSE;

		if (msg == WM_DESTROY)
			Unmap(wnd);

		LRESULT result = wnd->WndProc(msg, wparam, lparam);

		if (msg == WM_NCDESTROY)
		{
			wnd->SetHWnd(NULL);
			Unmap(wnd);

			if (wnd->GetSelfDestruct())
			{
				wnd->SelfDestruct();
			}
		}

		return result;
	}

	LRESULT Dlg::WndProc(UINT msg, WPARAM wparam, LPARAM)
	{
		switch (msg)
		{
			case WM_INITDIALOG:
				CentreWindow(GetParent());
				ClampToDesktop();
				return TRUE;

			case WM_COMMAND:
				switch (LOWORD(wparam))
				{
					case IDOK:
						if (HIWORD(wparam) == BN_CLICKED)
						{
							OnOK();
							return TRUE;
						}
						break;

					case IDCANCEL:
						if (HIWORD(wparam) == BN_CLICKED)
						{
							OnCancel();
							return TRUE;
						}
						break;
				}
				break;
		}

		return FALSE;
	}

	void Dlg::OnOK()
	{
		EndDialog(IDOK);
	}

	void Dlg::OnCancel()
	{
		EndDialog(IDCANCEL);
	}

	bool Dlg::EndDialog(int result)
	{
		if (_modeless)
		{
			if (IsWindowVisible())
				ShowWindow(SW_HIDE);
			_result = result;
			return true;
		}
		else
			return ::EndDialog(GetHWnd(), result) != FALSE;
	}

	//
	// StaticWnd
	//

	StaticWnd::StaticWnd()
	{
	}

	StaticWnd::~StaticWnd()
	{
	}

	LPCTSTR StaticWnd::GetClassName()
	{
		return TEXT("STATIC");
	}

	//
	// ButtonWnd
	//

	ButtonWnd::ButtonWnd()
	{
	}

	ButtonWnd::~ButtonWnd()
	{
	}

	LPCTSTR ButtonWnd::GetClassName()
	{
		return TEXT("BUTTON");
	}

	//
	// EditWnd
	//

	EditWnd::EditWnd()
	{
	}

	EditWnd::~EditWnd()
	{
	}

	LPCTSTR EditWnd::GetClassName()
	{
		return TEXT("EDIT");
	}

	void EditWnd::ScrollCaretToBottom()
	{
		SetSel(0, 0);
		ScrollCaret();
		DWORD l = GetTextLength();
		SetSel(l, l);
		ScrollCaret();
	}

	int EditWnd::GetLineNumber()
	{
		return LineFromChar(-1);
	}

	int EditWnd::GetColumnNumber(int tab)
	{
		INT line = LineFromChar(-1);

		DWORD startpos, endpos;
		GetSel(&startpos, &endpos);

		INT len = LineLength((INT) startpos);
		len += 4;

		TCHAR buffer[256];
		LPCTSTR linestr;
		TCHAR *newbuf = 0;

		if (len < WNDLIB_COUNTOF(buffer) - 1)
		{
			INT len = GetLine((UINT) line, buffer, WNDLIB_COUNTOF(buffer));
			if (len >= WNDLIB_COUNTOF(buffer))
				return -1;

			linestr = buffer;
		}
		else
		{
			newbuf = new TCHAR[len + 1];

			if (newbuf)
			{
				GetLine((UINT) line, newbuf, len);
				linestr = newbuf;
			}
			else
				linestr = TEXT("");
		}

		int charnum = (int)startpos - LineIndex(line);
		if (charnum > len) // This happens on 98...
		{
			if (newbuf)
				delete[] newbuf;

			return -1;
		}

		int colnum = 0;
		const TCHAR *p = linestr;
		const TCHAR *e = linestr + charnum;
		while (p != e)
		{
			if (*p == '\t')
			{
				do
				{
					++colnum;
				} while (colnum % tab);
			}
			else
				++colnum;

			++p;
		}

		if (newbuf)
			delete[] newbuf;

		return colnum;
	}

	int EditWnd::GetCharNumber()
	{
		INT line = LineFromChar(-1);

		DWORD startpos, endpos;
		GetSel(&startpos, &endpos);

		return (INT)startpos - LineIndex(line);
	}

	TCharString EditWnd::GetLine(UINT line)
	{
		TCharString string;
		string.resize(256);
		for (;;)
		{
			UINT got = GetLine(line, &string[0], string.size());
			if (got < string.size()) 
			{
				string.resize(got);
				return string;
			}

			string.resize(string.size() * 2);
		}
	}

	//
	// ListBoxWnd
	//

	ListBoxWnd::ListBoxWnd()
	{
	}

	ListBoxWnd::~ListBoxWnd()
	{
	}

	LPCTSTR ListBoxWnd::GetClassName()
	{
		return TEXT("LISTBOX");
	}

	TCharString ListBoxWnd::GetText(INT item)
	{
		TCharString string;
		string.resize(GetTextLen(item));
		if (! string.empty())
			GetText(item, &string[0]);
		return string;
	}

	//
	// ComboBoxWnd
	//

	ComboBoxWnd::ComboBoxWnd()
	{
	}

	ComboBoxWnd::~ComboBoxWnd()
	{
	}

	LPCTSTR ComboBoxWnd::GetClassName()
	{
		return TEXT("COMBOBOX");
	}

	TCharString ComboBoxWnd::GetLBText(INT index)
	{
		TCharString string;
		string.resize(GetLBTextLen(index));
		if (! string.empty())
			GetLBText(index, &string[0]);
		return string;
	}

	//
	// MDIFrameWnd
	//

	MDIFrameWnd::MDIFrameWnd()
	{
		_mdiClient = NULL;
	}

	LRESULT MDIFrameWnd::WndProc(UINT msg, WPARAM wparam, LPARAM lparam)
	{
		return ::DefFrameProc(GetHWnd(), _mdiClient, msg, wparam, lparam);
	}

	//
	// MDIClientWnd
	//

	MDIClientWnd::MDIClientWnd()
	{
		_ccs.hWindowMenu = NULL;
		_ccs.idFirstChild = ID_FIRST_CHILD;
	}

	MDIClientWnd::~MDIClientWnd()
	{
	}

	LPCTSTR MDIClientWnd::GetClassName()
	{
		return TEXT("MDICLIENT");
	}

	bool MDIClientWnd::Create(HWND parent, unsigned flags)
	{
		return Wnd::CreateEx((flags & MDICLIENT_NO_CLIENTEDGE) ? 0 : WS_EX_CLIENTEDGE,
			TEXT(""), WS_CHILD | WS_VISIBLE | ((flags & MDICLIENT_SCROLLING) ? (WS_VSCROLL | WS_HSCROLL) : 0),
			0, 0, 0, 0, parent, (HMENU)0xcac, NULL, (LPVOID) &_ccs, (flags & MDICLIENT_SUBCLASS) ? true : false);
	}

	bool MDIClientWnd::CreateIndirect(const CREATESTRUCT &cs, bool subclass)
	{
		CREATESTRUCT cs2;

		cs2 = cs;
		cs2.hMenu = (HMENU) 0xcac;
		cs2.style |= WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

		return Wnd::CreateIndirect(cs2, subclass);
	}

	//
	// RichEditWnd
	//

	CriticalSection richeditLoadLibLock;

	RichEditWnd::RichEditWnd()
	{
		CriticalSection::ScopedLock lock(richeditLoadLibLock);
		static bool loaded = false;
		if (! loaded)
		{
			loaded = true;
			LoadLibrary(TEXT("riched32.dll"));
		}
	}

	RichEditWnd::~RichEditWnd()
	{
	}

	LPCTSTR RichEditWnd::GetClassName()
	{
		return TEXT("RichEdit");
	}

	//
	// RichEdit2Wnd
	//

	static CriticalSection richedit2LoadLibLock;
	static LPCTSTR richedit2ClassName = NULL;

	LPCTSTR RichEdit2Wnd::LoadLibrary()
	{
		CriticalSection::ScopedLock lock(richedit2LoadLibLock);
		if (! richedit2ClassName)
		{
			if (! ::LoadLibrary(TEXT("msftedit.dll")))
				::LoadLibrary(TEXT("riched20.dll"));

			#ifdef WNDLIB_UNICODE
				#define RICHCLASSSUFFIX "W"
			#else
				#define RICHCLASSSUFFIX "W"
			#endif

			// The UNICODE controls work correctly as long as they're
			// sent ANSI messages.
			static LPCTSTR classNames[] = 
			{
				//TEXT("RICHEDIT60W"), // Flickering issues
				TEXT("RICHEDIT50W"),
				TEXT("RICHEDIT41W"),
				TEXT("RICHEDIT40W"),
				TEXT("RichEdit20W"),

				//TEXT("RICHEDIT60A"),
				TEXT("RICHEDIT50A"),
				TEXT("RICHEDIT41A"),
				TEXT("RICHEDIT40A"),
				TEXT("RichEdit20A"),
			};

			for (size_t i = 0; i != WNDLIB_COUNTOF(classNames); ++i)
			{
				WNDCLASSEX wc;
				memset(&wc, 0, sizeof(wc));
				wc.cbSize = sizeof(wc);

				if (GetClassInfoEx(NULL, classNames[i], &wc) != 0)
				{
					richedit2ClassName = classNames[i];
					break;
				}
			}
		}

		return richedit2ClassName;
	}

	RichEdit2Wnd::RichEdit2Wnd()
	{
		LoadLibrary();
	}

	RichEdit2Wnd::~RichEdit2Wnd()
	{
	}

	LPCTSTR RichEdit2Wnd::GetClassName()
	{
		return richedit2ClassName;
	}

	//
	// ComboBoxExWnd
	//

	ComboBoxExWnd::ComboBoxExWnd()
	{
	}

	ComboBoxExWnd::~ComboBoxExWnd()
	{
	}

	LPCTSTR ComboBoxExWnd::GetClassName()
	{
		return WC_COMBOBOXEX;
	}

	//
	// DateTimePickerWnd
	//

	DateTimePickerWnd::DateTimePickerWnd()
	{
	}

	DateTimePickerWnd::~DateTimePickerWnd()
	{
	}

	LPCTSTR DateTimePickerWnd::GetClassName()
	{
		return DATETIMEPICK_CLASS;
	}

	//
	// HotKeyWnd
	//

	HotKeyWnd::HotKeyWnd()
	{
	}

	HotKeyWnd::~HotKeyWnd()
	{
	}

	LPCTSTR HotKeyWnd::GetClassName()
	{
		return HOTKEY_CLASS;
	}

	//
	// IPAddressWnd
	//

	#if _WIN32_IE >= 0x400

	IPAddressWnd::IPAddressWnd()
	{
	}

	IPAddressWnd::~IPAddressWnd()
	{
	}

	LPCTSTR IPAddressWnd::GetClassName()
	{
		return WC_IPADDRESS;
	}

	#endif // _WIN32_IE >= 0x400

	//
	// ListViewWnd
	//

	ListViewWnd::ListViewWnd()
	{
	}

	ListViewWnd::~ListViewWnd()
	{
	}

	LPCTSTR ListViewWnd::GetClassName()
	{
		return WC_LISTVIEW;
	}

	int ListViewWnd::GetSingleSelection()
	{
		UINT nselected = this->GetSelectedCount();
		if (1 != nselected)
			return -1;

		int nitems = this->GetItemCount();
		LVITEM lvitem;
		for (int i = 0; i != nitems; ++i)
		{
			memset(&lvitem, 0, sizeof(lvitem));
			lvitem.mask = LVIF_STATE;
			lvitem.iItem = i;
			lvitem.stateMask = LVIS_SELECTED;
			this->GetItem(&lvitem);

			if (lvitem.state & LVIS_SELECTED)
				return i;
		}

		return -1;
	}

	void ListViewWnd::SetSelection(int select)
	{
		int n = this->GetItemCount();
		for (int i = 0; i != n; ++i)
		{
			LVITEM item;

			item.iItem = i;
			item.iSubItem = 0;
			item.mask = LVIF_STATE;
			item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
			this->GetItem(&item);

			if (i != select)
			{
				item.state &= ~ (LVIS_SELECTED | LVIS_FOCUSED);
			}
			else
			{
				item.state |= LVIS_SELECTED | LVIS_FOCUSED;
			}

			item.mask = LVIF_STATE;
			item.iItem = i;
			item.iSubItem = 0;
			item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
			this->SetItem(&item);
		}

		this->EnsureVisible(select, FALSE);
		this->SetFocus();
	}

	int ListViewWnd::AddColumn(LPCTSTR text, int width)
	{
		int colnum = 0;
		LVCOLUMN column;

		for (; ; colnum++)
		{
			column.mask = LVCF_SUBITEM;
			if (! GetColumn(colnum, &column))
			{
				break;
			}
		}

		column.mask = LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
		column.cx = width;
		column.pszText = (LPTSTR) text;
		column.iSubItem = colnum;

		InsertColumn(colnum, &column);

		return colnum;
	}

	void ListViewWnd::InsertItem(int item, int numColumns, LPCTSTR *columns, int image)
	{
		WNDLIB_ASSERT(0 != columns);
		WNDLIB_ASSERT(numColumns > 0);

		LVITEM lvitem;
		for (int i = 0; i != numColumns; ++i)
		{
			memset(&lvitem, 0, sizeof(lvitem));
			lvitem.mask = LVIF_TEXT;
			lvitem.iItem = item;
			lvitem.iSubItem = i;
			lvitem.pszText = (LPTSTR) columns[i];
			lvitem.iImage = image;
			if (i == 0)
			{
				lvitem.mask |= LVIF_IMAGE;
				InsertItem(&lvitem);
			}
			else
			{
				SetItem(&lvitem);
			}
		}
	}

	//
	// ProgressBarWnd
	//

	ProgressBarWnd::ProgressBarWnd()
	{
	}

	ProgressBarWnd::~ProgressBarWnd()
	{
	}

	LPCTSTR ProgressBarWnd::GetClassName()
	{
		return PROGRESS_CLASS;
	}

	//
	// PropertySheetWnd
	//

	WND_WM_BEGIN(PropertySheetWnd, TabControlWnd)
	WND_WM_END()

	PropertySheetWnd::PropertySheetWnd()
	{
	}

	PropertySheetWnd::~PropertySheetWnd()
	{
	}

	bool PropertySheetWnd::Create(HWND parent, int x, int y, int width, int height, int dlgid)
	{
		#pragma warning(disable: 4312)
		return TabControlWnd::CreateEx(WS_EX_CONTROLPARENT, NULL, WS_CHILD | WS_VISIBLE
				| WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSONBUTTONDOWN
				| TCS_MULTILINE | TCS_TABS | WS_TABSTOP, x, y, width, height, parent,
				(HMENU) dlgid, GetHInstance(), NULL, false);
		#pragma warning(default: 4312)
	}

	int PropertySheetWnd::AddTab(LPCTSTR tabtext)
	{
		int index = GetItemCount();

		TCITEM tab;
		tab.mask = TCIF_TEXT;
		tab.pszText = (LPTSTR) tabtext;
		InsertItem(index, &tab);

		return index;
	}

	//
	// RebarWnd
	//

	#if _WIN32_IE >= 0x400

	RebarWnd::RebarWnd()
	{
		_nextBandId = 50;
	}

	RebarWnd::~RebarWnd()
	{
	}

	LPCTSTR RebarWnd::GetClassName()
	{
		return REBARCLASSNAME;
	}

	BOOL RebarWnd::Create(HWND parent)
	{
		if (! CreateEx(0, NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | RBS_VARHEIGHT | CCS_NOPARENTALIGN | RBS_BANDBORDERS | CCS_NODIVIDER,
			0, 0, 0, 0, parent, NULL, NULL, NULL, false))
		{
			return FALSE;
		}

		return TRUE;
	}

	BOOL RebarWnd::SetBarInfo()
	{
		REBARINFO rbi;

		rbi.cbSize = sizeof(rbi);
		rbi.fMask = 0;
		rbi.himl = NULL;

		return SetBarInfo(&rbi);
	}

	void RebarWnd::Rearrange()
	{
		UINT n = GetBandCount();
		UINT i;

		for (i = 0; i != n; ++i)
		{
			REBARBANDINFO info;
			info.cbSize = sizeof(info);
			info.fMask = RBBIM_STYLE;
			GetBandInfo(i, &info);

			if (! (info.fStyle & RBBS_HIDDEN))
				MaximizeBand(i, FALSE);
		}
	}

	int RebarWnd::InsertBar(ToolbarWnd *bar, LPCTSTR title, UINT flags)
	{
		WNDLIB_ASSERT(0 != bar);
		WNDLIB_ASSERT(bar->GetHWnd());

		REBARBANDINFO band;
		memset(&band, 0, sizeof(band));

		SIZE buttonsize;
		bar->GetButtonSize(&buttonsize);

		// This is more accurate than GetMaxSize
		RECT rect;
		int buttoncount = bar->GetButtonCount();
		if (buttoncount)
			bar->GetItemRect(buttoncount-1, &rect);
		else
			rect.right = buttonsize.cx;

		band.cbSize = sizeof(band);
		band.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_ID | RBBIM_IMAGE;
		band.wID = _nextBandId;
		band.iImage = -1;
		if (title)
			band.fMask |= RBBIM_TEXT;
		band.fStyle = 0;//RBBS_USECHEVRON // chevron;
		if (flags & REBAR_INSERTBAR_SPLIT)
			band.fStyle |= RBBS_BREAK;
		if (flags & REBAR_INSERTBAR_HIDDEN)
			band.fStyle |= RBBS_HIDDEN;
		if (flags & REBAR_INSERTBAR_GRIPPER)
		{
			band.fStyle |= RBBS_GRIPPERALWAYS;
		}
		else
		{
			band.fStyle |= RBBS_NOGRIPPER;
		}
		band.lpText = (LPTSTR) title;
		band.hwndChild = bar->GetHWnd();
		band.cyMinChild = band.cyChild = band.cyMaxChild = buttonsize.cy;
		band.cx = rect.right + 16;
		band.cxMinChild = rect.right; //0 // chevron;
		band.cxIdeal = band.cx;
		band.cyIntegral = 1;

		if (flags & REBAR_INSERTBAR_NO_DYNAMIC_SIZE)
		{
			band.fMask |= RBBIM_SIZE | RBBIM_IDEALSIZE | RBBIM_HEADERSIZE;
			band.cx = 16384;
			band.cxIdeal = 16384;
			band.cxHeader = 8;
		}

		int bandIndex = GetBandCount();

		if (! InsertBand(bandIndex, &band))
			return -1;

		// This ensures that the last band on each line is maximized (left aligned)
		MaximizeBand(bandIndex, FALSE);

		return (int) (_nextBandId++);
	}

	#endif // _WIN32_IE >= 0x400

	//
	// StatusBarWnd
	//

	StatusBarWnd::StatusBarWnd()
	{
		_menuSelecting = false;
	}

	StatusBarWnd::~StatusBarWnd()
	{
	}

	LPCTSTR StatusBarWnd::GetClassName()
	{
		return STATUSCLASSNAME;
	}

	void StatusBarWnd::OnMenuSelect(WPARAM wparam, LPARAM lparam)
	{
		TCHAR buffer[512];

		UINT item = (UINT) LOWORD(wparam);
		UINT flags = (UINT) HIWORD(wparam);
		HMENU menu = (HMENU) lparam;

		if (! _menuSelecting)
		{
			_menuSelecting = true;
			GetText(0, buffer);

			_prevText = buffer;
		}

		if (flags == 0xffff && ! menu)
		{
			_menuSelecting = false;

			if (! _prevText.empty())
				SetText(0, SBT_NOBORDERS, _prevText.c_str());
		}
		else
		{
			_menuSelecting = true;

			TCHAR *ptr;

			if ((flags & MF_POPUP) || (flags & MF_SEPARATOR) || ! ::LoadString(GetHInstance(), item, buffer, WNDLIB_COUNTOF(buffer)))
				ptr = TEXT("");
			else
			{
				ptr = buffer;
				TCHAR *ptr2 = ptr;

				while (*ptr2 && *ptr2 != '\n')
					++ptr2;

				*ptr2 = 0;
			}

			SetText(0, SBT_NOBORDERS, ptr);
		}
	}

	TCharString StatusBarWnd::GetText(int part)
	{
		TCharString string;
		string.resize(GetTextLength(part));
		if (! string.empty()) 
			GetText(part, &string[0]);
		return string;
	}

	//
	// TabControlWnd
	//

	TabControlWnd::TabControlWnd()
	{
	}

	TabControlWnd::~TabControlWnd()
	{
	}

	LPCTSTR TabControlWnd::GetClassName()
	{
		return WC_TABCONTROL;
	}

	//
	// ToolbarWnd
	//

	ToolbarWnd::ToolbarWnd()
	{
	}

	ToolbarWnd::~ToolbarWnd()
	{
	}

	LPCTSTR ToolbarWnd::GetClassName()
	{
		return TOOLBARCLASSNAME;
	}

	BOOL ToolbarWnd::Create(HWND parent, BOOL inRebar, BOOL listStyle)
	{
		DWORD moreStyles;
		DWORD exStyle = 0;

		if (inRebar)
			moreStyles = CCS_NODIVIDER;
		else
			moreStyles = 0;

		if (listStyle)
			moreStyles |= TBSTYLE_LIST;

		if (! CreateEx(exStyle, NULL,
			WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TBSTYLE_FLAT |
				TBSTYLE_TOOLTIPS | CCS_NORESIZE | CCS_NOPARENTALIGN | moreStyles,
			0, 0, 0, 0, parent, NULL, NULL, NULL, false))
		{
			return FALSE;
		}

		return TRUE;
	}

	BOOL ToolbarWnd::AddSeparator()
	{
		TBBUTTON button;
		memset(&button, 0, sizeof(button));
		button.iBitmap = -1;
		button.idCommand = -1;
		button.fsState = 0;
		button.fsStyle = TBSTYLE_SEP;
		button.dwData = 0;
		button.iString = 0;

		return AddButtons(1, &button);
	}

	BOOL ToolbarWnd::AddButton(int commandID, int bitmapIndex,
		LPCTSTR text, BOOL enabled)
	{
		TBBUTTON button;
		memset(&button, 0, sizeof(button));
		button.iBitmap = bitmapIndex;
		button.idCommand = commandID;
		button.fsState = (BYTE) (enabled ? TBSTATE_ENABLED : 0);
		button.fsStyle = TBSTYLE_BUTTON | TBSTYLE_AUTOSIZE;
		button.dwData = 0;
		if (text)
			button.iString = AddString(text);
		else
			button.iString = -1;

		return AddButtons(1, &button);
	}

	TCharString ToolbarWnd::GetButtonText(int commandid)
	{
		TCharString string;
		string.resize(GetButtonText(commandid, NULL));
		if (! string.empty())
			GetButtonText(commandid, &string[0]);
		return string;
	}

	//
	// ToolTipWnd
	//

	ToolTipWnd::ToolTipWnd()
	{
	}

	ToolTipWnd::~ToolTipWnd()
	{
	}

	LPCTSTR ToolTipWnd::GetClassName()
	{
		return TOOLTIPS_CLASS;
	}

	//
	// TrackBarWnd
	//

	TrackBarWnd::TrackBarWnd()
	{
	}

	TrackBarWnd::~TrackBarWnd()
	{
	}

	LPCTSTR TrackBarWnd::GetClassName()
	{
		return TRACKBAR_CLASS;
	}

	//
	// TreeViewWnd
	//

	TreeViewWnd::TreeViewWnd()
	{
	}

	TreeViewWnd::~TreeViewWnd()
	{
	}

	LPCTSTR TreeViewWnd::GetClassName()
	{
		return WC_TREEVIEW;
	}

	HTREEITEM TreeViewWnd::RecursivelyFindSelected(HTREEITEM test)
	{
		for (; test; test = GetNextItem(TVGN_NEXT, test))
		{
			TVITEM tvi;
			tvi.mask = TVIF_STATE;
			tvi.stateMask = TVIS_SELECTED;

			if (! SendMessage(TVM_GETITEM, 0, (LPARAM) &tvi))
				continue;

			if (tvi.state & TVIS_SELECTED)
				return test;

			// Run through children
			HTREEITEM item = GetNextItem(TVGN_CHILD, test);
			if (item)
			{
				item = RecursivelyFindSelected(item);
				if (item)
					return item;
			}
		}

		return NULL;
	}

	HTREEITEM TreeViewWnd::GetSingleSelection()
	{
		HTREEITEM item = GetNextItem(TVGN_ROOT, NULL);

		return RecursivelyFindSelected(item);
	}

	void TreeViewWnd::RecursivelyExpand(HTREEITEM expand)
	{
		for (; expand; expand = GetNextItem(TVGN_NEXT, expand))
		{
			// Run through children
			HTREEITEM item = GetNextItem(TVGN_CHILD, expand);
			if (item)
			{
				Expand(TVE_EXPAND, expand);
				RecursivelyExpand(item);
			}
		}
	}

	void TreeViewWnd::ExpandAll()
	{
		RecursivelyExpand(GetNextItem(TVGN_ROOT, NULL));
	}

	//
	// UpDownWnd
	//

	UpDownWnd::UpDownWnd()
	{
	}

	UpDownWnd::~UpDownWnd()
	{
	}

	LPCTSTR UpDownWnd::GetClassName()
	{
		return UPDOWN_CLASS;
	}

	//
	// ModuleIcons
	//

	ModuleIcons::ModuleIcons()
	{
		std::fill(_icons, _icons + WNDLIB_COUNTOF(_icons), (HICON) NULL);
	}

	ModuleIcons::~ModuleIcons()
	{
		Unload();
	}

	bool ModuleIcons::LoadModuleIcons()
	{
		TCHAR buffer[2048];

		DWORD result = GetModuleFileName(GetModuleHandle(NULL), buffer, WNDLIB_COUNTOF(buffer));
		if (! result || result >= WNDLIB_COUNTOF(buffer))
			return false;

		return LoadIcons(buffer);
	}

	bool ModuleIcons::LoadIcons(const TCHAR *modulePath)
	{
		Unload();

		UINT iconsExtracted = ExtractIconEx(modulePath, 0, &_icons[0], &_icons[1], 1);

		return iconsExtracted != 0;
	}

	void ModuleIcons::Unload()
	{
		for (unsigned int i = 0; i != WNDLIB_COUNTOF(_icons); ++i)
		{
			if (_icons[i])
			{
				DestroyIcon(_icons[i]);
				_icons[i] = NULL;
			}
		}
	}
}


