#include "WndLib.h"
#include <memory>
#include <shellapi.h>
#include <algorithm>

#ifdef _MSC_VER
	#pragma comment(lib, "comctl32.lib")
	#pragma comment(lib, "version.lib")
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

	// Callback routine for font enumeration
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
	// Instance handle
	//

	namespace Private
	{
		HINSTANCE wndHInstance;
	}

	//
	// Utility functions
	//

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

	// Returns true if "parent" is the parent, grandparent, great-grandparent,
	// etc., of "child"
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

	TCHAR *NewTStr(LPCTSTR str)
	{
		size_t count = lstrlen(str) + 1;

		TCHAR *buf = new TCHAR[count];
		if (! buf)
			return 0;

		memcpy(buf, str, count * sizeof(TCHAR));
		return buf;
	}

	void VSNTPrintf(TCHAR *buf, size_t bufSize, const TCHAR *fmt, va_list argptr)
	{
		buf[0] = 0;
		#ifdef _MSC_VER
		#pragma warning(disable:4996)
		#endif
		#ifdef WNDLIB_UNICODE
			_vsnwprintf(buf, bufSize, fmt, argptr);
		#else
			_vsnprintf(buf, bufSize, fmt, argptr);
		#endif
		#ifdef _MSC_VER
		#pragma warning(default:4996)
		#endif
		buf[bufSize - 1] = 0;
	}

	void SNTPrintf(TCHAR *buf, size_t bufSize, const TCHAR *fmt, ...)
	{
		va_list argptr;
		va_start(argptr, fmt);
		VSNTPrintf(buf, bufSize, fmt, argptr);
		va_end(argptr);
	}

	//
	// EasyCreateFont
	//

	// Create font for the specified device context. "decipts" is
	// points * 10, so 80 is 8 points.
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

	//
	// Bytes
	//

	Bytes::~Bytes()
	{
		if (_bytes)
			free(_bytes);
	}

	Bytes::Bytes(const void *bytes, size_t size)
	{
		_bytes = NULL;
		Set(bytes, size);
	}

	Bytes::Bytes(const Bytes &copy)
	{
		_bytes = NULL;
		Set(copy._bytes, copy._size);
	}

	void Bytes::Set(const void *bytes, size_t size)
	{
		if (_bytes)
			free(_bytes);

		_size = size;
		if (! _size)
			_bytes = NULL;
		else
		{
			_bytes = (char *) malloc(_size);
			if (_bytes)
				memcpy(_bytes, bytes, _size);
			else
				_size = 0;
		}
	}

	Bytes &Bytes::operator = (const Bytes &copy)
	{
		if (this != &copy)
			Set(copy._bytes, copy._size);

		return *this;
	}

	char *Bytes::Realloc(size_t newSize)
	{
		_bytes = (char *) realloc(_bytes, newSize);
		_size = newSize;
		return _bytes;
	}

	//
	// Wnd
	//

	Wnd::WndMapping *Wnd::mapStart;
	CriticalSection Wnd::mapLock;

	static Wnd *creatingWnd = 0;

	Wnd::Wnd()
	{
		Construct();
	}

	// This constructor attaches or subclasses the specified handle.
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
		_autoDestroy = true;
	}

	Wnd::~Wnd()
	{
		_selfDestruct = true;

		// You should call DestroyWindow in your destructor (if its called here then
		// the wrong window procedure will run as the vtable has been destructed)
		WNDLIB_ASSERT(_hwnd == NULL);

		Unmap(this);
	}

	// Ask the Wnd to destroy itself
	void Wnd::SelfDestruct()
	{
		delete this;
	}

	// Attach the specified HWND to this object, optionally subclassing it.
	bool Wnd::Attach(HWND hwnd, bool subclass)
	{
		bool alreadyAttached = FindWnd(hwnd) != 0;

		if (! hwnd)
			return false;

		Detach();

		if (! alreadyAttached && ! Map(hwnd, this))
			return false;

		_hwnd = hwnd;

		if (subclass)
		{
			_prevproc = HelpGetWndProc(_hwnd);
			HelpSetWndProc(_hwnd, &SubclassWndProc);
		}

		return true;
	}

	// Simple form of Attach, does not map the HWND to this object, which
	// means this function can be used to assign multiple objects to the
	// same HWND
	void Wnd::SetHWnd(HWND hwnd)
	{
		_hwnd = hwnd;
	}

	// Detach this object from the window.
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
				// Oh dear...
				OutputDebugStringA(
					"Wnd::Detach: unable to un-subclass window "
					"(using window's window class).\r\n");
				HelpSetWndProc(_hwnd, HelpGetClassWndProc(_hwnd));
			}

			_prevproc = NULL;
		}

		HWND hwnd = _hwnd;
		_hwnd = NULL;
		if (FindWnd(hwnd) == this)
			Unmap(hwnd);
		return hwnd;
	}

	// Destroy the window.
	BOOL Wnd::DestroyWindow()
	{
		BOOL result = ::DestroyWindow(_hwnd);

		_hwnd = NULL;
		_prevproc = NULL;

		return result;
	}

	// In this library, unlike MFC, a Wnd is designed to correspond to a
	// WNDCLASS. This function should return the name of this class, as a
	// unique name for a WNDCLASS.
	LPCTSTR Wnd::GetClassName()
	{
		return TEXT("Wnd");
	}

	// Initialise a WNDCLASS structure prior to creating the window.
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
		wc->hInstance = GetInstanceHandle();
		wc->lpfnWndProc = (WNDPROC) &StaticWndProc;
		wc->lpszMenuName = NULL;
		//wc->style = CS_HREDRAW | CS_VREDRAW | CS_GLOBALCLASS;
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

	// Work out which DefWindowProc to use
	WNDPROC Wnd::GetDefWindowProc(const CREATESTRUCT &cs)
	{
		if (cs.dwExStyle & WS_EX_MDICHILD)
			return ::DefMDIChildProc;
		else
			return ::DefWindowProc;
	}

	// Create a window, getting the parameters from the specified
	// CREATESTRUCT.
	bool Wnd::CreateIndirect(const CREATESTRUCT &cs, bool subclass)
	{
		// If "true", then we will have to attach to the newly created window.
		bool attach = true;

		// Work out which DefWindowProc to use
		_defWindowProc = GetDefWindowProc(cs);

		// Check the window class
		if (cs.hInstance == GetInstanceHandle())
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

			return Attach(hwnd, subclass);
		}
		else
		{
			// TODO: synchronise creatingWnd
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
			creatingWnd = 0;

			if (! wnd)
			{
				OutputDebugStringA("Wnd::StaticWndProc: used incorrectly.\r\n");
				return 0;
			}

			Map(hwnd, wnd);

			HelpSetWindowPtr(hwnd, 0, (const void *) wnd);
			wnd->_hwnd = hwnd;
		}

		LRESULT result;

		result = wnd->WndProc(msg, wparam, lparam);

		if (msg == WM_NCDESTROY)
		{
			wnd->_hwnd = NULL;
			Unmap(hwnd);

			if (wnd->_selfDestruct)
			{
				wnd->SelfDestruct();
			}
		}

		return result;
	}

	// Create a window. Called CreateEx, rather than Create, to allow
	// derived class to have their own specialised Create.
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

	// Our window procedure.
	LRESULT Wnd::WndProc(UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (_prevproc)
		{
   			return ::CallWindowProc(_prevproc, _hwnd, msg, wparam, lparam);
		}

		return (*_defWindowProc) (_hwnd, msg, wparam, lparam);
	}

	bool Wnd::Map(HWND hwnd, Wnd *wnd)
	{
		CriticalSection::ScopedLock lock(mapLock);

		for (WndMapping *m = mapStart; m; m = m->next)
		{
			if (m->hwnd == hwnd)
			{
				m->wnd = wnd;
				return true;
			}
		}

		WndMapping *newMapping = new(std::nothrow) WndMapping;
		if (! newMapping)
			return false;

		newMapping->next = mapStart;
		mapStart = newMapping;
		newMapping->hwnd = hwnd;
		newMapping->wnd = wnd;

		return true;
	}

	bool Wnd::Unmap(HWND hwnd)
	{
		CriticalSection::ScopedLock lock(mapLock);

		WndMapping **prev = &mapStart;
		for (WndMapping *m = mapStart; m; prev = &m->next, m = m->next)
		{
			if (m->hwnd == hwnd)
			{
				*prev = m->next;
				delete m;
				return true;
			}
		}

		return false;
	}

	bool Wnd::Unmap(Wnd *wnd)
	{
		CriticalSection::ScopedLock lock(mapLock);

		WndMapping **prev = &mapStart;
		for (WndMapping *m = mapStart; m; prev = &m->next, m = m->next)
		{
			if (m->wnd == wnd)
			{
				*prev = m->next;
				delete m;
				return true;
			}
		}

		return false;
	}

	Wnd *Wnd::FindWnd(HWND hwnd)
	{
		CriticalSection::ScopedLock lock(mapLock);

		for (WndMapping *m = mapStart; m; m = m->next)
		{
			if (m->hwnd == hwnd)
				return m->wnd;
		}

		return 0;
	}

	LRESULT WINAPI Wnd::SubclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
	{
		Wnd *wnd = FindWnd(hwnd);

		if (wnd)
		{
			LRESULT res = wnd->WndProc(msg, wparam, lparam);

			if (msg == WM_NCDESTROY)
			{
				Unmap(hwnd);
				wnd->_hwnd = NULL;

				if (wnd->_selfDestruct)
				{
					wnd->SelfDestruct();
				}
			}

			return res;
		}

		//return ::CallWindowProc(HelpGetClassWndProc(hwnd), hwnd, msg, wparam, lparam);
		return ::DefWindowProc(hwnd, msg, wparam, lparam);
	}

	// Set the size of this window's client area
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
		return ::MessageBox(_hwnd, text, title, type);
	}

	bool Wnd::GetDlgItemInt(int id, int *out)
	{
		WNDLIB_ASSERT(0 != out);

		BOOL translated;
		*out = (INT)::GetDlgItemInt(_hwnd, id, &translated, TRUE);

		if (! translated)
			return FALSE;
		else
			return TRUE;
	}

	bool Wnd::GetDlgItemInt(int id, unsigned *out)
	{
		WNDLIB_ASSERT(0 != out);

		BOOL translated;
		*out = (UINT)::GetDlgItemInt(_hwnd, id, &translated, FALSE);

		if (! translated)
			return FALSE;
		else
			return TRUE;
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

	//
	// Dlg
	//

	Dlg::Dlg() :
		_resourceinst(GetInstanceHandle()),
		_resourcename(0)
	{
	}

	Dlg::Dlg(HWND attach, bool subclass) :
		_resourceinst(GetInstanceHandle()),
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

	// Create the dialogue (modeless)
	bool Dlg::Create(HWND parent)
	{
		INT_PTR result;
		return DoDlg(parent, 0, 0, false, &result);
	}

	// Create the dialogue (modeless)
	bool Dlg::CreateSetFont(HWND parent, LPCTSTR fontName, unsigned fontPointSize)
	{
		INT_PTR result;
		return DoDlg(parent, fontName, fontPointSize, false, &result);
	}

	// Run the dialogue modally
	INT_PTR Dlg::DoModal(HWND parent)
	{
		INT_PTR result;
		if (! DoDlg(parent, 0, 0, true, &result))
			return IDCANCEL;

		_hwnd = NULL;

		return result;
	}

	INT_PTR Dlg::DoModalSetFont(HWND parent, LPCTSTR fontName, unsigned fontPointSize)
	{
		INT_PTR result;
		if (! DoDlg(parent, fontName, fontPointSize, true, &result))
			return IDCANCEL;

		_hwnd = NULL;

		return result;
	}

	// Create/DoModal the dialogue, force the font if fontPointSize != 0
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

	// Our class's dialogue procedure
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

			wnd->_hwnd = hwnd;
			HelpSetWindowPtr(hwnd, DWLP_USER, (const void *) wnd);
		}

		if (msg == WM_DESTROY)
			Unmap(hwnd);

		wnd = (Dlg *) HelpGetWindowPtr(hwnd, DWLP_USER);
		if (! wnd)
			return FALSE;

		LRESULT result = wnd->WndProc(msg, wparam, lparam);

		if (msg == WM_NCDESTROY)
		{
			wnd->_hwnd = NULL;
			Unmap(hwnd);

			if (wnd->GetSelfDestruct())
			{
				wnd->SelfDestruct();
			}
		}

		return result;
	}

	// Do NOT call Wnd::WndProc
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

	// End this dialogue with the specified result. This can be used by
	// modeless dialogues to.
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
			return ::EndDialog(_hwnd, result) != FALSE;
	}

	//
	// StaticWnd
	//

	StaticWnd::StaticWnd()
	{
	}

	StaticWnd::~StaticWnd()
	{
		DestroyOrDetach();
	}

	// Get our class name
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
		DestroyOrDetach();
	}

	// Get our class name
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
		DestroyOrDetach();
	}

	// Get our class name
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

	// Return the line number (zero based)
	int EditWnd::GetLineNumber()
	{
		return LineFromChar(-1);
	}

	// Return the column number (zero based) (requires tab size)
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

	// Return the character number (zero based)
	int EditWnd::GetCharNumber()
	{
		INT line = LineFromChar(-1);

		DWORD startpos, endpos;
		GetSel(&startpos, &endpos);

		return (INT)startpos - LineIndex(line);
	}

	//
	// ListBoxWnd
	//

	ListBoxWnd::ListBoxWnd()
	{
	}

	ListBoxWnd::~ListBoxWnd()
	{
		DestroyOrDetach();
	}

	// Get our class name
	LPCTSTR ListBoxWnd::GetClassName()
	{
		return TEXT("LISTBOX");
	}

	//
	// ComboBoxWnd
	//

	ComboBoxWnd::ComboBoxWnd()
	{
	}

	ComboBoxWnd::~ComboBoxWnd()
	{
		DestroyOrDetach();
	}

	// Get our class name
	LPCTSTR ComboBoxWnd::GetClassName()
	{
		return TEXT("COMBOBOX");
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
		return ::DefFrameProc(_hwnd, _mdiClient, msg, wparam, lparam);
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
		DestroyOrDetach();
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

	LONG RichEditWnd::staticLoadLib = 0;

	RichEditWnd::RichEditWnd()
	{
		if (InterlockedIncrement(&staticLoadLib) == 1)
			LoadLibrary(TEXT("riched32.dll"));
	}

	RichEditWnd::~RichEditWnd()
	{
		DestroyOrDetach();
	}

	// Wnd overrides.
	LPCTSTR RichEditWnd::GetClassName()
	{
		return TEXT("RichEdit");
	}

	//
	// RichEdit2Wnd
	//

	LONG RichEdit2Wnd::staticLoadLib = 0;

	RichEdit2Wnd::RichEdit2Wnd()
	{
		if (InterlockedIncrement(&staticLoadLib) == 1)
			LoadLibrary(TEXT("riched20.dll"));
	}

	RichEdit2Wnd::~RichEdit2Wnd()
	{
		DestroyOrDetach();
	}

	// Wnd overrides.
	LPCTSTR RichEdit2Wnd::GetClassName()
	{
		return RICHEDIT_CLASS;
	}

	//
	// ComboBoxExWnd
	//

	ComboBoxExWnd::ComboBoxExWnd()
	{
	}

	ComboBoxExWnd::~ComboBoxExWnd()
	{
		DestroyOrDetach();
	}

	// Get our class name
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
		DestroyOrDetach();
	}

	// Get our class name
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
		DestroyOrDetach();
	}

	// Get our class name
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
		DestroyOrDetach();
	}

	// Get our class name
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
		DestroyOrDetach();
	}

	// Get our class name
	LPCTSTR ListViewWnd::GetClassName()
	{
		return WC_LISTVIEW;
	}

	// Get the single selected item in the list, return -1 otherwise
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

	// Select the i'th entry in the list (this is a WndLib extension)
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

	// Add a column, return it's index
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

	// Insert an item in the list simply by providing text for each entry.
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
		DestroyOrDetach();
	}

	// Wnd overrides.
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
		DestroyOrDetach();
	}

	// Create the PropertySheetWnd using the default options for the class
	bool PropertySheetWnd::Create(HWND parent, int x, int y, int width, int height, int dlgid)
	{
		#pragma warning(disable: 4312) // stupid bloody warning
		return TabControlWnd::CreateEx(WS_EX_CONTROLPARENT, NULL, WS_CHILD | WS_VISIBLE
				| WS_CLIPSIBLINGS | WS_CLIPCHILDREN | TCS_FOCUSONBUTTONDOWN
				| TCS_MULTILINE | TCS_TABS | WS_TABSTOP, x, y, width, height, parent,
				(HMENU) dlgid, GetInstanceHandle(), NULL, false);
		#pragma warning(default: 4312) // stupid bloody warning
	}

	// Add a new tab, returns it's index
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
		DestroyOrDetach();
	}

	// Get our class name
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

	// Maximize the last bar on each row
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
		WNDLIB_ASSERT(bar->HWnd());

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
		band.hwndChild = bar->HWnd();
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
		_prevText = 0;
	}

	StatusBarWnd::~StatusBarWnd()
	{
		DestroyOrDetach();

		if (_prevText)
			delete[] _prevText;
	}

	// Get our class name
	LPCTSTR StatusBarWnd::GetClassName()
	{
		return STATUSCLASSNAME;
	}

	// Process WM_MENUSELECT for the parent window
	void StatusBarWnd::OnMenuSelect(WPARAM wparam, LPARAM lparam)
	{
		TCHAR buf[512];

		UINT item = (UINT) LOWORD(wparam);
		UINT flags = (UINT) HIWORD(wparam);
		HMENU menu = (HMENU) lparam;

		if (! _menuSelecting)
		{
			_menuSelecting = true;
			GetText(0, buf);

			if (_prevText)
				delete[] _prevText;

			_prevText = NewTStr(buf);
		}

		if (flags == 0xffff && ! menu)
		{
			_menuSelecting = false;

			if (_prevText)
				SetText(0, SBT_NOBORDERS, _prevText);
		}
		else
		{
			_menuSelecting = true;

			TCHAR *ptr;

			if ((flags & MF_POPUP) || (flags & MF_SEPARATOR) || ! ::LoadString(GetInstanceHandle(), item, buf, WNDLIB_COUNTOF(buf)))
				ptr = TEXT("");
			else
			{
				ptr = buf;
				TCHAR *ptr2 = ptr;

				while (*ptr2 && *ptr2 != '\n')
					++ptr2;

				*ptr2 = 0;
			}

			SetText(0, SBT_NOBORDERS, ptr);
		}
	}

	//
	// TabControlWnd
	//

	TabControlWnd::TabControlWnd()
	{
	}

	TabControlWnd::~TabControlWnd()
	{
		DestroyOrDetach();
	}

	// Get our class name
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
		DestroyOrDetach();
	}

	// Get our class name
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

	//
	// ToolTipWnd
	//

	ToolTipWnd::ToolTipWnd()
	{
	}

	ToolTipWnd::~ToolTipWnd()
	{
		DestroyOrDetach();
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
		DestroyOrDetach();
	}

	// Get our class name
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
		DestroyOrDetach();
	}

	// Get our class name
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

	// Get the single selected item in the tree, return NULL otherwise
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
		DestroyOrDetach();
	}

	// Get our class name
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

	bool ModuleIcons::LoadExeIcons()
	{
		TCHAR buf[2048];

		DWORD result = GetModuleFileName(GetModuleHandle(NULL), buf, WNDLIB_COUNTOF(buf));
		if (! result || result >= WNDLIB_COUNTOF(buf))
			return false;

		return LoadIcons(buf);
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


