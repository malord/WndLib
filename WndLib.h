//
// WndLib
// Copyright (c) 1994-2013 Mark H. P. Lord. All rights reserved.
//
// See LICENSE.txt for license.
//
// - Define WNDLIB_DLL or _USE_DLLS in your projects if you're going to use WndLib from a DLL.
// - Define WNDLIB_STATIC to override WNDLIB_DLL and _USE_DLLS.
// - Define WNDLIB_DLL_EXPORT if you're compiling WndLib itself in to a DLL.
//

#ifndef WNDLIB_WNDLIB_H
#define WNDLIB_WNDLIB_H

#ifdef _MSC_VER

	// "class X needs to have dll-interface to be used by clients of class Y"
	#pragma warning(disable:4251)

	#if defined(WNDLIB_DLL_EXPORT)

		#define WNDLIB_EXPORT __declspec(dllexport)

		// "class X needs to have dll-interface to be used by clients of class Y"
		#pragma warning(disable:4251)

	#elif defined(WNDLIB_DLL) || (defined(_USE_DLLS) && ! defined(WNDLIB_STATIC))

		#define WNDLIB_EXPORT __declspec(dllimport)

	#else

		#define WNDLIB_EXPORT

	#endif

#elif defined(__GNUC__) && __GNUC__ >= 4

	#define WNDLIB_EXPORT __attribute__((visibility("default")))

#else

	#define WNDLIB_EXPORT

#endif

// Include windows.h with correct SDK constants. If you want to use some other Windows versions, either define
// WINVER, etc., yourself, or include windows.h before including this header. Don't change this header.

#ifndef _WINDOWS_

	// If not overriden elsehwere, require Windows 2000, XP or newer NT based system.

	#ifndef WINVER
		#define WINVER 0x0400
	#endif

	#ifndef _WIN32_WINDOWS
		#define _WIN32_WINDOWS 0x0400
	#endif

	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x0400
	#endif

	#ifndef _WIN32_IE
		#define _WIN32_IE 0x0400
	#endif

	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif

	#ifndef VC_EXTRALEAN
		#define VC_EXTRALEAN
	#endif

	#ifndef STRICT
		#define STRICT
	#endif

	#include <windows.h>

#endif

#ifdef __MINGW32__
	typedef DWORD UNDONAMEID;
	typedef DWORD TEXTMODE;
#endif

#if defined(UNICODE) || defined(_UNICODE)
	#define WNDLIB_UNICODE
#endif

#include <richedit.h>
#include <commctrl.h>
#include <assert.h>
#include <winreg.h>
#include <stdio.h>

#define WNDLIB_ASSERT assert
#define WNDLIB_COUNTOF(arr) (sizeof(arr) / sizeof((arr)[0]))

namespace WndLib
{
	//
	// Instance handle
	//

	namespace Private
	{
		WNDLIB_EXPORT extern HINSTANCE hInstance;
	}

	// Return the application's instance handle.
	inline HINSTANCE GetHInstance()
	{
		return Private::hInstance;
	}

	// Set the application's instance handle. You must call this before any
	// Wnd instances are created.
	inline void SetHInstance(HINSTANCE instance)
	{
		Private::hInstance = instance;
	}

	//
	// Utility functions
	//

	#if !defined(GetWindowLongPtr)
		#define GetWindowLongPtr GetWindowLong
		#define SetWindowLongPtr SetWindowLong
		#define GetClassLongPtr GetClassLong
		#define SetClassLongPtr SetClassLong

		typedef LONG LONG_PTR;

		#define GWLP_WNDPROC GWL_WNDPROC
		#define GCLP_WNDPROC GCL_WNDPROC
		#define DWLP_USER DWL_USER
	#endif

	inline const void *HelpGetWindowPtr(HWND hwnd, int position)
	{
		return (const void *) ::GetWindowLongPtr(hwnd, position);
	}

	inline void HelpSetWindowPtr(HWND hwnd, int position, const void *ptr)
	{
		::SetWindowLongPtr(hwnd, position, (LONG_PTR) ptr);
	}

	inline WNDPROC HelpGetWndProc(HWND hwnd)
	{
		return (WNDPROC) ::GetWindowLongPtr(hwnd, GWLP_WNDPROC);
	}

	inline void HelpSetWndProc(HWND hwnd, WNDPROC wndproc)
	{
		::SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR) wndproc);
	}

	inline WNDPROC HelpGetClassWndProc(HWND hwnd)
	{
		return (WNDPROC) ::GetClassLongPtr(hwnd, GCLP_WNDPROC);
	}

	// Returns the height of a RECT.
	inline int RectHeight(const RECT &rect)
	{
		return rect.bottom - rect.top;
	}

	// Returns the width of a RECT.
	inline int RectWidth(const RECT &rect)
	{
		return rect.right - rect.left;
	}

	// Centre a window within its parent or, if parent is NULL, within the
	// primary screen.
	WNDLIB_EXPORT void CentreWindow(HWND parent, HWND child);

	// Ensure the window is entirely within the primary screen.
	WNDLIB_EXPORT void ClampToDesktop(HWND window);

	// Returns true if "parent" is an ancestor of "child".
	WNDLIB_EXPORT bool IsAncestorOfWindow(HWND parent, HWND child);

	// boundary must be a power of 2
	inline void *AlignPtr(const void *ptr, size_t boundary)
	{
		return (char *) ((((size_t) (const char *) ptr) + (boundary - 1)) & (~(boundary - 1)));
	}

	// Allocate a copy of str using operator new[].
	WNDLIB_EXPORT TCHAR *TNewString(LPCTSTR str);

	WNDLIB_EXPORT void Tvsnprintf(TCHAR *buffer, size_t bufferSize, const TCHAR *fmt, va_list argptr);

	WNDLIB_EXPORT void Tsnprintf(TCHAR *buffer, size_t bufferSize, const TCHAR *fmt, ...);

	WNDLIB_EXPORT bool Tstrcpy(TCHAR *buffer, size_t bufferSize, const TCHAR *source);
	WNDLIB_EXPORT bool Tstrcat(TCHAR *buffer, size_t bufferSize, const TCHAR *source);

	// Returns the DPI scale factor for the system. Does not support per-monitor DPI.
	WNDLIB_EXPORT double GetDPIScale(HDC hdc = NULL);

	//
	// EasyCreateFont
	//

	// Flags for EasyCreateFont
	enum EasyCreateFontFlags
	{
		EASYFONT_BOLD = 1,
		EASYFONT_UNDERLINE = 2,
		EASYFONT_ITALIC = 4,
		EASYFONT_STRIKEOUT = 8,
		EASYFONT_MUSTEXIST = 256,
	};

	// Create font for the specified device context. "decipts" is points * 10, so 85 is 8.5 points.
	WNDLIB_EXPORT HFONT EasyCreateFont(HDC hdc, LPCTSTR facename, int decipts, int flags);

	WNDLIB_EXPORT HFONT CreateShellFont();

	WNDLIB_EXPORT HFONT CreateMenuFont();

	WNDLIB_EXPORT HFONT CreateStatusFont();

	//
	// ByteArray: A resizable array of bytes.
	//

	class WNDLIB_EXPORT ByteArray
	{
	public:

		ByteArray() :
			_bytes(NULL),
			_size(0)
		{}

		ByteArray(const void *bytes, size_t size);

		ByteArray(const ByteArray &copy);

		ByteArray &operator = (const ByteArray &copy);

		~ByteArray();

		char *Resize(size_t newSize);

		void Set(const void *bytes, size_t size);

		char *Get() const
		{
			return _bytes;
		}

		size_t GetSize() const
		{
			return _size;
		}

	private:

		char *_bytes;
		size_t _size;
	};

	//
	// DataArray: A resizable array of any POD type.
	//

	template<typename Type>
	class DataArray
	{
	public:

		DataArray()
		{}

		DataArray(const Type *array, size_t size) :
			_bytes(array, size * sizeof(Type))
		{}

		DataArray(const DataArray &copy) :
			_bytes(copy._bytes)
		{}

		DataArray &operator = (const DataArray &copy)
		{
			_bytes = copy._bytes;
			return *this;
		}

		Type *Resize(size_t newSize)
		{
			return (Type *) _bytes.Resize(newSize * sizeof(Type));
		}

		void Set(const void *bytes, size_t size)
		{
			_bytes.Set(bytes, size * sizeof(Type));
		}

		Type *Get() const
		{
			return (Type *) _bytes.Get();
		}

		size_t GetSize() const
		{
			return _bytes.GetSize() / sizeof(Type);
		}

	private:

		ByteArray _bytes;
	};

	//
	// CriticalSection: Wrapper around CRITICAL_SECTION.
	//

	namespace Private
	{
		// Locks a threading primitive until destructed.
		template<class LockType>
		class ScopedLock
		{
		public:

			// Magic type used for ScopedLock's DONT_LOCK constructor.
			enum ScopedLockDontLock
			{
				DONT_LOCK
			};

			ScopedLock() :
				lockable(NULL)
			{}

			// Immediately lock the specified object.
			ScopedLock(LockType *lock) :
				lockable(lock)
			{
				if (lockable)
					lockable->Lock();
			}

			// Immediately lock the specified object.
			ScopedLock(LockType &lock) :
				lockable(&lock)
			{
				if (lockable)
					lockable->Lock();
			}

			// Assign an object but don't lock it. It will be unlocked by the destructor.
			ScopedLock(LockType *dontLock, ScopedLockDontLock) :
				lockable(dontLock)
			{
			}

			// Assign an object but don't lock it. It will be unlocked by the destructor.
			ScopedLock(LockType &dontLock, ScopedLockDontLock) :
				lockable(&dontLock)
			{
			}

			~ScopedLock()
			{
				if (lockable)
					lockable->Unlock();
			}

			// Get the object we're managing.
			LockType *GetLockable() const
			{
				return lockable;
			}

			// Try to lock the specified lock. Returns false if it wasn't locked and doesn't attach the lock to this object.
			bool TryLock(LockType *lock)
			{
				WNDLIB_ASSERT(! lockable);

				if (! lock->TryLock())
					return false;

				lockable = lock;
				return true;
			}

			// Lock the specified lock and attach it to this object.
			void Lock(LockType *lock)
			{
				WNDLIB_ASSERT(! lockable);
				lock->Lock();
				lockable = lock;
			}

			// Assign a different object to this lock, but don't lock it.
			void Attach(LockType *lock)
			{
				WNDLIB_ASSERT(! lockable);
				lockable = lock;
			}

			// Unlock the object we're managing and detach it from this lock.
			void Release()
			{
				WNDLIB_ASSERT(lockable);
				lockable->Unlock();
				lockable = NULL;
			}

		private:

			LockType *lockable;
		};
	}

	class CriticalSection
	{
	public:

		// You can do: CriticalSection::ScopedLock lock(anyCriticalSection) to lock the critical section
		// and automatically unlock it when lock goes out of scope.
		typedef WndLib::Private::ScopedLock<CriticalSection> ScopedLock;

		CriticalSection()
		{
			InitializeCriticalSection(&_cs);
		}

		~CriticalSection()
		{
			DeleteCriticalSection(&_cs);
		}

		void Lock()
		{
			EnterCriticalSection(&_cs);
		}

		void Unlock()
		{
			LeaveCriticalSection(&_cs);
		}

	private:

		CRITICAL_SECTION _cs;
	};

	//
	// Message handling macros
	//
	// Example:
	//
	// // In the header file:
	//
	// class MyWnd : public BaseClassWnd
	// {
	//     WND_WM_DECLARE(MyWnd, BaseClassWnd)
	//     WND_WM_FUNC(OnCreate)
	//     WND_WM_FUNC(OnPaint)
	//     ...
	// public:
	//     ... remaining declarations go here ...
	// }
	//
	// // In the source file:
	//
	// WND_WM_BEGIN(MyWnd, BaseClassWnd)
	//     WND_WM(WM_CREATE, OnCreate)
	//     WND_WM(WM_PAINT, OnPaint)
	//     ...
	//     WND_WM_COMMAND(ID_HELP_ABOUT, OnHelpAbout)
	//     WND_WM_NOTIFY(NM_CUSTOMDRAW, OnCustomDraw)
	// WND_WM_END()
	//

	#define WND_WM_DECLARE(_Class, _BaseClass) \
		private: \
			bool WmDispatch(UINT msg, WPARAM wparam, LPARAM lparam, LRESULT *result); \
		public: \
			virtual LRESULT WndProc(UINT msg, WPARAM wparam, LPARAM lparam); \
			inline LRESULT BaseWndProc(UINT msg, WPARAM wparam, LPARAM lparam) \
			{ \
				return _BaseClass::WndProc(msg, wparam, lparam); \
			} \
		private:

	// Use this to define the WmDispatch method without defining WndProc.
	// This allows you to write your own WndProc and dispatch via the message
	// table only when needed.
	#define WND_WM_BEGIN_NO_WNDPROC(_Class, _BaseClass) \
		bool _Class::WmDispatch(UINT msg, WPARAM wparam, LPARAM lparam, LRESULT *result) \
		{ \
			(void) msg; \
			(void) wparam; \
			(void) lparam; \
			(void) result; \

	#define WND_WM_BEGIN(_Class, _BaseClass) \
		LRESULT _Class::WndProc(UINT msg, WPARAM wparam, LPARAM lparam) \
		{ \
			LRESULT result; \
			if (! WmDispatch(msg, wparam, lparam, &result)) \
				result = BaseWndProc(msg, wparam, lparam); \
			return result; \
		} \
		WND_WM_BEGIN_NO_WNDPROC(_Class, _BaseClass)

	#define WND_WM(_Msg, _Func) \
		if (msg == (_Msg)) \
		{ \
			*result = _Func(msg, wparam, lparam); \
			return true; \
		}

	#define WND_WM_COMMAND(_Cmd, _Func) \
		if (msg == WM_COMMAND && LOWORD(wparam) == (_Cmd)) \
		{ \
			*result = _Func(msg, wparam, lparam); \
			return true; \
		}

	#define WND_WM_NOTIFY(_NotifyCode, _Func) \
		if (msg == WM_NOTIFY) \
		{ \
			if (((const NMHDR *) lparam)->code == (_NotifyCode)) \
			{ \
				*result = _Func(msg, wparam, lparam); \
				return true; \
			} \
		}

	#define WND_WM_END() \
			return false; \
		}

	#define WND_WM_FUNC(_Name) LRESULT _Name(UINT msg, WPARAM wparam, LPARAM lparam);

	//
	// Wnd
	//

	class WNDLIB_EXPORT Wnd
	{
	public:

		Wnd();

		// This constructor attaches or subclasses the specified handle.
		Wnd(HWND hwnd, bool subclass = false);

		virtual ~Wnd();

		// Set the self destruct flag; if set then this object will delete itself
		// when it receives a WM_NCDESTROY. By default, this is false.
		void SetSelfDestruct(bool value = true)
		{
			_selfDestruct = value;
		}

		// Are we set to self destruct?
		bool GetSelfDestruct() const
		{
			return _selfDestruct;
		}

		// Attach the specified HWND to this object, optionally subclassing it.
		bool Attach(HWND hwnd, bool subclass = false);

		// Simple form of Attach, does not map the HWND to this object, which
		// means this function can be used to assign multiple objects to the
		// same HWND
		void SetHWnd(HWND hwnd);

		// Attach the specified window (which is specified via a dialogue handle
		// and a control ID)
		bool AttachDlgItem(HWND hdlg, int ctlid, bool subclass = false)
		{
			return Attach(::GetDlgItem(hdlg, ctlid), subclass);
		}

		// Detach this object from the window.
		HWND Detach();

		// Destroy the window.
		BOOL DestroyWindow();

		// Get our window handle
		HWND GetHWnd() const
		{
			return _hwndx;
		}

		// In this library, unlike MFC, a Wnd is designed to correspond to a
		// WNDCLASS. This function should return the name of this class, as a
		// unique name for a WNDCLASS.
		virtual LPCTSTR GetClassName();

		// Initialise a WNDCLASS structure prior to creating the window.
		virtual void GetWndClass(WNDCLASSEX *wc);

		// Work out which DefWindowProc to use
		virtual WNDPROC GetDefWindowProc(const CREATESTRUCT &cs);

		// Create a window, getting the parameters from the specified
		// CREATESTRUCT.
		virtual bool CreateIndirect(const CREATESTRUCT &cs, bool subclass);

		// Create a window. Called CreateEx, rather than Create, to allow
		// derived class to have their own specialised Create.
		bool CreateEx(DWORD exStyle, LPCTSTR windowName, DWORD style,
			int x, int y, int width, int height, HWND parent, HMENU menu = NULL,
			HINSTANCE instance = GetHInstance(), LPVOID params = NULL, 
			bool subclass = false);

		// Our window procedure.
		virtual LRESULT WndProc(UINT msg, WPARAM wparam, LPARAM lparam);

		// Set the size of this window's client area
		void SetClientAreaSize(int cx, int cy);

		//
		// API wrappers
		//

		LRESULT SendMessage(UINT msg, WPARAM wparam = 0, LPARAM lparam = 0)
		{
			return ::SendMessage(GetHWnd(), msg, wparam, lparam);
		}
		BOOL PostMessage(UINT msg, WPARAM wparam = 0, LPARAM lparam = 0)
		{
			return ::PostMessage(GetHWnd(), msg, wparam, lparam);
		}

		LONG GetWindowLong(int index)
		{
			return ::GetWindowLong(GetHWnd(), index);
		}
		LONG GetClassLong(int index)
		{
			return ::GetClassLong(GetHWnd(), index);
		}
		LONG SetWindowLong(int index, LONG value)
		{
			return ::SetWindowLong(GetHWnd(), index, value);
		}
		LONG SetClassLong(int index, LONG value)
		{
			return ::SetClassLong(GetHWnd(), index, value);
		}

		/*LONG_PTR GetWindowLongPtr(int index) { return ::GetWindowLongPtr(GetHWnd(), index); }
		LONG_PTR GetClassLongPtr(int index) { return ::GetClassLongPtr(GetHWnd(), index); }
		LONG_PTR SetWindowLongPtr(int index, LONG_PTR value) { return ::SetWindowLongPtr(GetHWnd(), index, value); }
		LONG_PTR SetClassLongPtr(int index, LONG_PTR value) { return ::SetClassLongPtr(GetHWnd(), index, value); }*/

		BOOL ShowWindow(int showCommand)
		{
			return ::ShowWindow(GetHWnd(), showCommand);
		}
		BOOL IsWindowVisible()
		{
			return ::IsWindowVisible(GetHWnd());
		}

		BOOL EnableWindow(BOOL enable)
		{
			return ::EnableWindow(GetHWnd(), enable);
		}
		BOOL IsWindowEnabled()
		{
			return ::IsWindowEnabled(GetHWnd());
		}

		BOOL SetWindowPos(HWND insertAfter, int x, int y, int width, int height, UINT flags)
		{
			return ::SetWindowPos(GetHWnd(), insertAfter, x, y, width, height, flags);
		}
		BOOL MoveWindow(int x, int y, int width, int height, BOOL repaint = TRUE)
		{
			return ::MoveWindow(GetHWnd(), x, y, width, height, repaint);
		}

		BOOL SetWindowPlacement(const WINDOWPLACEMENT *placement)
		{
			return ::SetWindowPlacement(GetHWnd(), placement);
		}
		BOOL GetWindowPlacement(WINDOWPLACEMENT *placement)
		{
			return ::GetWindowPlacement(GetHWnd(), placement);
		}

		HDC BeginPaint(LPPAINTSTRUCT paint)
		{
			return ::BeginPaint(GetHWnd(), paint);
		}
		BOOL EndPaint(const PAINTSTRUCT *paint)
		{
			return ::EndPaint(GetHWnd(), paint);
		}

		BOOL GetClientRect(LPRECT rect)
		{
			return ::GetClientRect(GetHWnd(), rect);
		}
		BOOL GetWindowRect(LPRECT rect)
		{
			return ::GetWindowRect(GetHWnd(), rect);
		}

		HDC GetDC()
		{
			return ::GetDC(GetHWnd());
		}
		BOOL ReleaseDC(HDC hdc)
		{
			return ::ReleaseDC(GetHWnd(), hdc);
		}

		BOOL ScreenToClient(LPPOINT pt)
		{
			return ::ScreenToClient(GetHWnd(), pt);
		}
		BOOL ScreenToClient(LPRECT r)
		{
			return ::ScreenToClient(GetHWnd(), (LPPOINT)r) && ::ScreenToClient(GetHWnd(), ((LPPOINT)r) + 1);
		}
		BOOL ClientToScreen(LPPOINT pt)
		{
			return ::ClientToScreen(GetHWnd(), pt);
		}
		BOOL ClientToScreen(LPRECT r)
		{
			return ::ClientToScreen(GetHWnd(), (LPPOINT)r) && ::ClientToScreen(GetHWnd(), ((LPPOINT)r) + 1);
		}

		BOOL SetWindowText(LPCTSTR string)
		{
			return ::SetWindowText(GetHWnd(), string);
		}
		BOOL GetWindowText(LPTSTR string, int maxChars)
		{
			return ::GetWindowText(GetHWnd(), string, maxChars);
		}
		int GetWindowTextLength()
		{
			return (int) ::GetWindowTextLength(GetHWnd());
		}
		LPCTSTR GetWindowText(ByteArray *buffer);

		HWND GetParent()
		{
			return ::GetParent(GetHWnd());
		}
		HWND SetParent(HWND newParent)
		{
			return ::SetParent(GetHWnd(), newParent);
		}
		HWND GetWindow(UINT cmd)
		{
			return ::GetWindow(GetHWnd(), cmd);
		}

		BOOL InvalidateRect(CONST RECT *rect = NULL, BOOL erase = TRUE)
		{
			return ::InvalidateRect(GetHWnd(), rect, erase);
		}
		BOOL ValidateRect(CONST RECT *rect = NULL)
		{
			return ::ValidateRect(GetHWnd(), rect);
		}
		BOOL UpdateWindow()
		{
			return ::UpdateWindow(GetHWnd());
		}

		// Sort-of-wrappers
		void CentreWindow(HWND parent)
		{
			WndLib::CentreWindow(parent, GetHWnd());
		}
		void ClampToDesktop()
		{
			WndLib::ClampToDesktop(GetHWnd());
		}

		void SetDlgItemText(int id, LPCTSTR string)
		{
			::SetDlgItemText(GetHWnd(), id, string);
		}
		void SetDlgItemInt(int id, UINT value, bool issigned = true)
		{
			::SetDlgItemInt(GetHWnd(), id, value, issigned ? TRUE : FALSE);
		}
		int GetDlgItemText(int id, LPTSTR stringout, int maxcount)
		{
			return ::GetDlgItemText(GetHWnd(), id, stringout, maxcount);
		}
		LPCTSTR GetDlgItemText(int id, ByteArray *buffer);

		UINT GetDlgItemInt(int id, BOOL *translated, bool issigned)
		{
			return ::GetDlgItemInt(GetHWnd(), id, translated, issigned ? TRUE : FALSE);
		}

		LRESULT SendDlgItemMessage(int id, UINT message, WPARAM wparam = 0, LPARAM lparam = 0)
		{
			return ::SendDlgItemMessage(GetHWnd(), id, message, wparam, lparam);
		}

		HWND SetFocus()
		{
			return ::SetFocus(GetHWnd());
		}
		HWND SetCapture()
		{
			return ::SetCapture(GetHWnd());
		}

		int GetTextLength()
		{
			return (int) ::SendMessage(GetHWnd(), WM_GETTEXTLENGTH, 0, 0);
		}

		HWND SetActiveWindow()
		{
			return ::SetActiveWindow(GetHWnd());
		}
		BOOL SetForegroundWindow()
		{
			return ::SetForegroundWindow(GetHWnd());
		}

		BOOL SetMenu(HMENU hmenu)
		{
			return ::SetMenu(GetHWnd(), hmenu);
		}
		BOOL DrawMenuBar()
		{
			return ::DrawMenuBar(GetHWnd());
		}

		BOOL IsIconic()
		{
			return ::IsIconic(GetHWnd());
		}
		BOOL IsZoomed()
		{
			return ::IsZoomed(GetHWnd());
		}

		void SetFont(HFONT hfont, bool redraw = true)
		{
			::SendMessage(GetHWnd(), WM_SETFONT, (WPARAM) hfont, redraw ? TRUE : FALSE);
		}

		HFONT GetFont()
		{
			return (HFONT) ::SendMessage(GetHWnd(), WM_GETFONT, 0, 0);
		}

		int GetDlgCtrlID()
		{
			return ::GetDlgCtrlID(GetHWnd());
		}
		void SetDlgCtrlID(int id)
		{
			::SetWindowLong(GetHWnd(), GWL_ID, (LONG) id);
		}

		UINT_PTR SetTimer(UINT eventId, UINT elapse, TIMERPROC timerProc)
		{
			return ::SetTimer(GetHWnd(), eventId, elapse, timerProc);
		}
		BOOL KillTimer(UINT eventId)
		{
			return ::KillTimer(GetHWnd(), eventId);
		}

		BOOL CheckDlgButton(int buttonId, BOOL check)
		{
			return ::CheckDlgButton(GetHWnd(), buttonId, check);
		}
		UINT IsDlgButtonChecked(int buttonId)
		{
			return ::IsDlgButtonChecked(GetHWnd(), buttonId);
		}
		BOOL CheckRadioButton(int firstId, int lastId, int checkedId)
		{
			return ::CheckRadioButton(GetHWnd(), firstId, lastId, checkedId);
		}

		HWND GetDlgItem(int id)
		{
			return ::GetDlgItem(GetHWnd(), id);
		}

		//
		// Helpers
		//

		// Send a message to the descendants of a window
		void SendMessageToDescendants(UINT msg, WPARAM wparam, LPARAM lparam, bool deep)
		{
			SendMessageToDescendants(GetHWnd(), msg, wparam, lparam, deep);
		}

		// Static version of the above
		static void SendMessageToDescendants(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, bool deep);

		BOOL SetWindowPos(HWND insertAfter, const RECT &rc, UINT flags)
		{
			return ::SetWindowPos(GetHWnd(), insertAfter, rc.left, rc.top, rc.right-rc.left, rc.bottom-rc.top, flags);
		}

		void GetSize(SIZE *sz);

		bool IsMouseInWindow();

		int MessageBox(LPCTSTR text, UINT type = MB_OK);

		bool GetDlgItemInt(int id, int *out);
		bool GetDlgItemInt(int id, unsigned *out);

		int GetFontHeightForWindow(HFONT hFont);

		//
		// Statics
		//

		static Wnd *FindWnd(HWND hwnd);

		static LRESULT WINAPI StaticWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
		static LRESULT WINAPI SubclassWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	protected:

		static bool Map(HWND hwnd, Wnd *wnd);
		static bool Unmap(Wnd *wnd);

		HWND DoCreateWindowEx(DWORD exStyle, LPCTSTR className, LPCTSTR windowName,
			DWORD style, int x, int y, int cx, int cy, HWND parent, HMENU menu,
			HINSTANCE instance, LPVOID lparam);

		// Ask the Wnd to destroy itself
		virtual void SelfDestruct();

	private:

		void Construct();

		WNDPROC _prevproc;

		// If this flag is true, the object will be deleted when it receives a
		// WM_NCDESTROY
		bool _selfDestruct;

		// Window procedure handler to call if a message is not handled
		WNDPROC _defWindowProc;

		HWND _hwndx;

		struct WndMapping
		{
			HWND hwnd;
			Wnd *wnd;
			WndMapping *next;
		};

		static WndMapping *mapStart;
		static CriticalSection mapLock;
	};

	//
	// Dlg
	//

	class WNDLIB_EXPORT Dlg : public Wnd
	{
	public:

		Dlg();
		Dlg(HWND attach, bool subclass);
		Dlg(UINT resourceid);
		Dlg(LPCTSTR resourcename);
		Dlg(HINSTANCE instance, UINT resourceid);
		Dlg(HINSTANCE instance, LPCTSTR resourcename);

		// Set the dialgoue resource
		void SetResource(UINT resourceid)
		{
			SetResource(GetHInstance(), resourceid);
		}
		void SetResource(LPCTSTR resourceid)
		{
			SetResource(GetHInstance(), resourceid);
		}
		void SetResource(HINSTANCE inst, UINT resourceid)
		{
			SetResource(inst, MAKEINTRESOURCE(resourceid));
		}
		void SetResource(HINSTANCE inst, LPCTSTR resourcename)
		{
			_resourceinst = inst;
			_resourcename = resourcename;
		}

		// Create the dialgoue (modeless)
		virtual bool Create(HWND parent);
		virtual bool CreateSetFont(HWND parent, LPCTSTR fontName, unsigned fontPointSize);

		// Run the dialogue modally
		INT_PTR DoModal(HWND parent);
		INT_PTR DoModalSetFont(HWND parent, LPCTSTR fontName, unsigned fontPointSize);

		// Do NOT call Wnd::WndProc, call Dlg::WndProc
		virtual LRESULT WndProc(UINT msg, WPARAM wparam, LPARAM lparam);

		// OK and Cancel messages are mapped to these methods. The default
		// implementations end the dialogue.
		virtual void OnOK();
		virtual void OnCancel();

		// Returns true if this dialogue was created modelessly
		bool IsModeless() const
		{
			return _modeless;
		}

		// Returns the dialogue's return code
		INT_PTR GetResult()
		{
			return _result;
		}

		// End this dialogue with the specified result. This can be used by
		// modeless dialogues to.
		bool EndDialog(int result);

	protected:

		// Our class's dialogue procedure
		static LRESULT WINAPI StaticDlgProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

		// Create/DoModal the dialogue, force the font if fontPointSize != 0
		bool DoDlg(HWND parent, LPCTSTR fontName, unsigned fontPointSize, bool modal, INT_PTR *output_ptr);

		bool _modeless;
		INT_PTR _result;
		HINSTANCE _resourceinst;
		LPCTSTR _resourcename;
	};

	//
	// StaticWnd
	//

	class WNDLIB_EXPORT StaticWnd : public Wnd
	{
	public:
		StaticWnd();
		virtual ~StaticWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		HICON GetIcon()
		{
			return (HICON) SendMessage(STM_GETICON, 0, 0);
		}
		HANDLE GetImage(UINT type)
		{
			return (HANDLE) SendMessage(STM_GETIMAGE, type, 0);
		}
		HICON SetIcon(HICON newicon)
		{
			return (HICON) SendMessage(STM_SETICON, (WPARAM) newicon, 0);
		}
		HANDLE SetImage(UINT type, HANDLE handle)
		{
			return (HANDLE) SendMessage(STM_SETIMAGE, type, (LPARAM) handle);
		}

	};

	//
	// ButtonWnd
	//

	class WNDLIB_EXPORT ButtonWnd : public Wnd
	{
	public:
		ButtonWnd();
		virtual ~ButtonWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		void Click()
		{
			SendMessage(BM_CLICK, 0, 0);
		}
		UINT GetCheck()
		{
			return (UINT) SendMessage(BM_GETCHECK, 0, 0);
		}
		HANDLE GetImage(WPARAM imagetype)
		{
			return (HANDLE) SendMessage(BM_GETIMAGE, imagetype, 0);
		}
		UINT GetState()
		{
			return (UINT) SendMessage(BM_GETSTATE, 0, 0);
		}
		void SetCheck(UINT check)
		{
			SendMessage(BM_SETCHECK, (WPARAM) check, 0);
		}
		HANDLE SetImage(WPARAM imagetype, HANDLE image)
		{
			return (HANDLE) SendMessage(BM_SETIMAGE, imagetype, (LPARAM) image);
		}
		void SetState(BOOL state)
		{
			SendMessage(BM_SETSTATE, (WPARAM) state, 0);
		}
		void SetStyle(DWORD style)
		{
			SendMessage(BM_SETSTYLE, (WPARAM) style, 0);
		}
	};

	//
	// EditWnd
	//

	class WNDLIB_EXPORT EditWnd : public Wnd
	{
	public:
		EditWnd();
		virtual ~EditWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		void Cut()
		{
			SendMessage(WM_CUT, 0, 0);
		}
		void Copy()
		{
			SendMessage(WM_COPY, 0, 0);
		}
		void Paste()
		{
			SendMessage(WM_PASTE, 0, 0); }

		BOOL CanUndo()
		{
			return (BOOL) SendMessage(EM_CANUNDO, 0, 0);
		}
		DWORD CharFromPos(WORD x, WORD y)
		{
			return (DWORD) SendMessage(EM_CHARFROMPOS, 0, MAKELPARAM(x, y));
		}
		void EmptyUndoBuffer()
		{
			SendMessage(EM_EMPTYUNDOBUFFER, 0, 0);
		}
		BOOL FmtLines(BOOL linebreak)
		{
			return (BOOL) SendMessage(EM_FMTLINES, (WPARAM) linebreak, 0);
		}
		UINT GetFirstVisibleLine()
		{
			return (UINT) SendMessage(EM_GETFIRSTVISIBLELINE, 0, 0);
		}
		HANDLE GetHandle()
		{
			return (HANDLE) SendMessage(EM_GETHANDLE, 0, 0);
		}
		UINT GetLimitText()
		{
			return (UINT) SendMessage(EM_GETLIMITTEXT, 0, 0);
		}
		UINT GetLine(UINT line, LPTSTR buffer, UINT bufferMaxChars)
		{
			*(DWORD *) buffer = bufferMaxChars;
			return (UINT) SendMessage(EM_GETLINE, (WPARAM) line, (LPARAM) buffer);
		}
		UINT GetLineCount()
		{
			return (UINT) SendMessage(EM_GETLINECOUNT, 0, 0);
		}
		DWORD GetMargins()
		{
			return (DWORD) SendMessage(EM_GETMARGINS, 0, 0);
		}
		BOOL GetModify()
		{
			return (BOOL) SendMessage(EM_GETMODIFY, 0, 0);
		}
		TCHAR GetPasswordChar()
		{
			return (TCHAR) SendMessage(EM_GETPASSWORDCHAR, 0, 0);
		}
		void GetRect(LPRECT rect)
		{
			SendMessage(EM_GETRECT, 0, (LPARAM) rect);
		}
		DWORD GetSel(LPDWORD startpos, LPDWORD endpos)
		{
			return (DWORD) SendMessage(EM_GETSEL, (WPARAM)startpos, (LPARAM) endpos);
		}
		UINT GetThumb()
		{
			return (UINT) SendMessage(EM_GETTHUMB, 0, 0);
		}
		void LimitText(UINT chars)
		{
			SendMessage(EM_LIMITTEXT, (WPARAM) chars, 0);
		}
		void SetLimitText(UINT chars)
		{
			SendMessage(EM_SETLIMITTEXT, (WPARAM) chars, 0);
		}
		UINT LineFromChar(INT lineindex)
		{
			return (UINT) SendMessage(EM_LINEFROMCHAR, (WPARAM) lineindex, 0);
		}
		INT LineIndex(INT linenumber)
		{
			return (INT) SendMessage(EM_LINEINDEX, (WPARAM) linenumber, 0);
		}
		INT LineLength(INT charindex)
		{
			return (INT) SendMessage(EM_LINELENGTH, (WPARAM) charindex, 0);
		}
		BOOL LineScroll(INT horz, INT vert)
		{
			return (BOOL) SendMessage(EM_LINESCROLL, (WPARAM) horz, (LPARAM) vert);
		}
		DWORD PosFromChar(INT charindex)
		{
			return (DWORD) SendMessage(EM_POSFROMCHAR, (WPARAM) charindex, 0);
		}
		void ReplaceSel(BOOL canundo, LPCTSTR string)
		{
			SendMessage(EM_REPLACESEL, (WPARAM) canundo, (LPARAM) string);
		}
		DWORD Scroll(UINT action)
		{
			return (DWORD) SendMessage(EM_SCROLL, (WPARAM) action, 0);
		}
		void ScrollCaret()
		{
			SendMessage(EM_SCROLLCARET, 0, 0);
		}
		void SetHandle(HANDLE h)
		{
			SendMessage(EM_SETHANDLE, (WPARAM) h, 0);
		}
		#ifdef EM_GETIMESTATUS
			LRESULT GetImeStatus(UINT status)
			{
				return SendMessage(EM_GETIMESTATUS, (WPARAM) status, 0);
			}
			LRESULT SetImeStatus(WPARAM statustype, LPARAM statusdata)
			{
				return SendMessage(EM_SETIMESTATUS, statustype, statusdata);
			}
		#endif
		void SetMargins(WPARAM margintoset, LPARAM newwidth)
		{
			SendMessage(EM_SETMARGINS, margintoset, newwidth);
		}
		void SetModify(BOOL modify)
		{
			SendMessage(EM_SETMODIFY, (WPARAM) modify, 0);
		}
		void SetPasswordChar(TCHAR c)
		{
			SendMessage(EM_SETPASSWORDCHAR, (WPARAM) c, 0);
		}
		BOOL SetReadOnly(BOOL readonly)
		{
			return (BOOL) SendMessage(EM_SETREADONLY, (WPARAM) readonly, 0);
		}
		void SetRect(const RECT *rect)
		{
			SendMessage(EM_SETRECT, 0, (LPARAM) rect);
		}
		void SetRectNP(const RECT *rect)
		{
			SendMessage(EM_SETRECTNP, 0, (LPARAM) rect);
		}
		void SetSel(DWORD start, DWORD end)
		{
			SendMessage(EM_SETSEL, (WPARAM) start, (LPARAM) end);
		}
		BOOL SetTabStops(INT count, const UINT *stops)
		{
			return (BOOL) SendMessage(EM_SETTABSTOPS, (WPARAM) count, (LPARAM) stops);
		}
		BOOL Undo()
		{
			return (BOOL) SendMessage(EM_UNDO, 0, 0);
		}

		//
		// Utility methods
		//

		void ScrollCaretToBottom();

		// Return the line number (zero based)
		int GetLineNumber();

		// Return the column number (zero based, requires tab size)
		int GetColumnNumber(int tabsize);

		// Return the character number (zero based)
		int GetCharNumber();
	};

	//
	// ListBoxWnd
	//

	class WNDLIB_EXPORT ListBoxWnd : public Wnd
	{
	public:
		ListBoxWnd();
		virtual ~ListBoxWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		INT AddFile(LPCTSTR filename)
		{
			return (INT) SendMessage(LB_ADDFILE, 0, (LPARAM) filename);
		}
		INT AddString(LPCTSTR string)
		{
			return (INT) SendMessage(LB_ADDSTRING, 0, (LPARAM) string);
		}
		INT DeleteString(INT index)
		{
			return (INT) SendMessage(LB_DELETESTRING, (WPARAM) index, 0);
		}
		INT Dir(UINT attribs, LPCTSTR filespec)
		{
			return (INT) SendMessage(LB_DIR, (WPARAM) attribs, (LPARAM) filespec);
		}
		INT FindString(INT startfrom, LPCTSTR string)
		{
			return (INT) SendMessage(LB_FINDSTRING, (WPARAM) startfrom, (LPARAM) string);
		}
		INT FindStringExact(INT startfrom, LPCTSTR string)
		{
			return (INT) SendMessage(LB_FINDSTRINGEXACT, (WPARAM) startfrom, (LPARAM) string);
		}
		INT GetAnchorIndex()
		{
			return (INT) SendMessage(LB_GETANCHORINDEX, 0, 0);
		}
		INT GetCaretIndex()
		{
			return (INT) SendMessage(LB_GETCARETINDEX, 0, 0);
		}
		INT GetCount()
		{
			return (INT) SendMessage(LB_GETCOUNT, 0, 0);
		}
		INT GetCurSel()
		{
			return (INT) SendMessage(LB_GETCURSEL, 0, 0);
		}
		INT GetHorizontalExtent()
		{
			return (INT) SendMessage(LB_GETHORIZONTALEXTENT, 0, 0);
		}
		LRESULT GetItemData(INT index)
		{
			return SendMessage(LB_GETITEMDATA, (WPARAM) index, 0);
		}
		INT GetItemHeight(INT index)
		{
			return (INT) SendMessage(LB_GETITEMHEIGHT, (WPARAM) index, 0);
		}
		INT GetItemRect(INT index, LPRECT rectout)
		{
			return (INT) SendMessage(LB_GETITEMRECT, (WPARAM) index, (LPARAM) rectout);
		}
		DWORD GetLocale()
		{
			return (DWORD) SendMessage(LB_GETLOCALE, 0, 0);
		}
		INT GetSel(INT index)
		{
			return (INT) SendMessage(LB_GETSEL, (WPARAM) index, 0); }

		INT GetSelCount()
		{
			return (INT) SendMessage(LB_GETSELCOUNT, 0, 0);
		}
		INT GetSelItems(INT maxcount, INT *out)
		{
			return (INT) SendMessage(LB_GETSELITEMS, (WPARAM) maxcount, (LPARAM) out);
		}
		INT GetText(INT item, LPTSTR buffer)
		{
			return (INT) SendMessage(LB_GETTEXT, (WPARAM) item, (LPARAM) buffer);
		}
		INT GetTextLen(INT item)
		{
			return (INT) SendMessage(LB_GETTEXTLEN, (WPARAM) item, 0);
		}
		INT GetTopIndex()
		{
			return (INT) SendMessage(LB_GETTOPINDEX, 0, 0);
		}
		INT InitStorage(INT itemcount, INT bytes)
		{
			return (INT) SendMessage(LB_INITSTORAGE, (WPARAM) itemcount, (LPARAM) bytes);
		}
		INT InsertString(INT index, LPCTSTR string)
		{
			return (INT) SendMessage(LB_INSERTSTRING, (WPARAM) index, (LPARAM) string);
		}
		DWORD ItemFromPoint(WORD x, WORD y)
		{
			return (DWORD) SendMessage(LB_ITEMFROMPOINT, 0, MAKELPARAM(x, y));
		}
		void ResetContent()
		{
			SendMessage(LB_RESETCONTENT, 0, 0);
		}
		INT SelectString(INT startat, LPCTSTR string)
		{
			return (INT) SendMessage(LB_SELECTSTRING, (WPARAM) startat, (LPARAM) string);
		}
		INT SelItemRange(BOOL select, WORD first, WORD last)
		{
			return (INT) SendMessage(LB_SELITEMRANGE, (WPARAM) select, MAKELPARAM(first, last));
		}
		INT SelItemRangeEx(INT first, INT last)
		{
			return (INT) SendMessage(LB_SELITEMRANGEEX, (WPARAM) first, (LPARAM) last);
		}
		INT SetAnchorIndex(INT index)
		{
			return (INT) SendMessage(LB_SETANCHORINDEX, (WPARAM) index, 0);
		}
		INT SetCaretIndex(INT index, BOOL scroll)
		{
			return (INT) SendMessage(LB_SETCARETINDEX, (WPARAM) index, (LPARAM) scroll);
		}
		void SetColumnWidth(INT width)
		{
			SendMessage(LB_SETCOLUMNWIDTH, (WPARAM) width, 0);
		}
		INT SetCount(INT count)
		{
			return (INT) SendMessage(LB_SETCOUNT, (WPARAM) count, 0);
		}
		INT SetCurSel(INT index)
		{
			return (INT) SendMessage(LB_SETCURSEL, (WPARAM) index, 0);
		}
		void SetHorizontalExtent(INT pixels)
		{
			SendMessage(LB_SETHORIZONTALEXTENT, (WPARAM) pixels, 0);
		}
		INT SetItemData(INT index, LPARAM data)
		{
			return (INT) SendMessage(LB_SETITEMDATA, (WPARAM) index, data);
		}
		INT SetItemHeight(INT index, INT pixels)
		{
			return (INT) SendMessage(LB_SETITEMHEIGHT, (WPARAM) index, (LPARAM) pixels);
		}
		DWORD SetLocale(DWORD locale)
		{
			return (DWORD) SendMessage(LB_SETLOCALE, (WPARAM) locale, 0);
		}
		INT SetSel(BOOL select, INT index)
		{
			return (INT) SendMessage(LB_SETSEL, (WPARAM) select, (LPARAM) index);
		}
		BOOL SetTabStops(INT count, const INT *stops)
		{
			return (BOOL) SendMessage(LB_SETTABSTOPS, (WPARAM) count, (LPARAM) stops);
		}
		INT SetTopIndex(INT index)
		{
			return (INT) SendMessage(LB_SETTOPINDEX, (WPARAM) index, 0);
		}
	};

	//
	// ComboBoxWnd
	//

	class WNDLIB_EXPORT ComboBoxWnd : public Wnd
	{
	public:
		ComboBoxWnd();
		virtual ~ComboBoxWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		INT AddString(LPCTSTR string)
		{
			return (INT) SendMessage(CB_ADDSTRING, 0, (LPARAM) string);
		}
		INT DeleteString(INT at)
		{
			return (INT) SendMessage(CB_DELETESTRING, (WPARAM) at, 0);
		}
		INT FindString(INT startat, LPCTSTR string)
		{
			return (INT) SendMessage(CB_FINDSTRING, (WPARAM) startat, (LPARAM) string);
		}
		INT FindStringExact(INT startat, LPCTSTR string)
		{
			return (INT) SendMessage(CB_FINDSTRINGEXACT, (WPARAM) startat, (LPARAM) string);
		}
		INT GetCount()
		{
			return (INT) SendMessage(CB_GETCOUNT, 0, 0);
		}
		INT GetCurSel()
		{
			return (INT) SendMessage(CB_GETCURSEL, 0, 0);
		}
		BOOL GetDroppedControlRect(LPRECT rectout)
		{
			return (BOOL) SendMessage(CB_GETDROPPEDCONTROLRECT, 0, (LPARAM) rectout);
		}
		BOOL GetDroppedState()
		{
			return (BOOL) SendMessage(CB_GETDROPPEDSTATE, 0, 0);
		}
		INT GetDroppedWidth()
		{
			return (INT) SendMessage(CB_GETDROPPEDWIDTH, 0, 0);
		}
		DWORD GetEditSel(LPDWORD startout, LPDWORD endout)
		{
			return (DWORD) SendMessage(CB_GETEDITSEL, (WPARAM) startout, (LPARAM) endout);
		}
		BOOL GetExtendedUI()
		{
			return (BOOL) SendMessage(CB_GETEXTENDEDUI, 0, 0);
		}
		INT GetHorizontalExtent()
		{
			return (INT) SendMessage(CB_GETHORIZONTALEXTENT, 0, 0);
		}
		LRESULT GetItemData(INT index)
		{
			return SendMessage(CB_GETITEMDATA, (WPARAM) index, 0);
		}
		INT GetItemHeight(INT of = 0)
		{
			return (INT) SendMessage(CB_GETITEMHEIGHT, (WPARAM) of, 0);
		}
		INT GetLBText(INT index, LPTSTR buffer)
		{
			return (INT) SendMessage(CB_GETLBTEXT, (WPARAM) index, (LPARAM) buffer);
		}
		INT GetLBTextLen(INT index)
		{
			return (INT) SendMessage(CB_GETLBTEXTLEN, (WPARAM) index, 0);
		}
		INT GetTopIndex()
		{
			return (INT) SendMessage(CB_GETTOPINDEX, 0, 0);
		}
		INT InitStorage(INT itemcount, INT bytecount)
		{
			return (INT) SendMessage(CB_INITSTORAGE, (WPARAM) itemcount, (LPARAM) bytecount);
		}
		INT InsertString(INT at, LPCTSTR string)
		{
			return (INT) SendMessage(CB_INSERTSTRING, (WPARAM) at, (LPARAM) string);
		}
		BOOL LimitText(INT to)
		{
			return (BOOL) SendMessage(CB_LIMITTEXT, (WPARAM) to, 0);
		}
		void ResetContent()
		{
			SendMessage(CB_RESETCONTENT, 0, 0);
		}
		INT SelectString(INT startat, LPCTSTR string)
		{
			return (INT) SendMessage(CB_SELECTSTRING, (WPARAM) startat, (LPARAM) string);
		}
		INT SetCurSel(INT index)
		{
			return (INT) SendMessage(CB_SETCURSEL, (WPARAM) index, 0);
		}
		INT SetDroppedWidth(INT width)
		{
			return (INT) SendMessage(CB_SETDROPPEDWIDTH, (WPARAM) width, 0);
		}
		INT SetEditSel(short start, short end)
		{
			return (INT) SendMessage(CB_SETEDITSEL, 0, MAKELPARAM((WORD)start, (WORD)end));
		}
		void SetHorizontalExtent(INT pixels)
		{
			SendMessage(CB_SETHORIZONTALEXTENT, (WPARAM) pixels, 0);
		}
		INT SetItemData(INT index, LPARAM value)
		{
			return (INT) SendMessage(CB_SETITEMDATA, (WPARAM) index, value);
		}
		INT SetItemHeight(INT index, INT height)
		{
			return (INT) SendMessage(CB_SETITEMHEIGHT, (WPARAM) index, (WPARAM) height);
		}
		INT SetTopIndex(INT index)
		{
			return (INT) SendMessage(CB_SETTOPINDEX, (WPARAM) index, 0);
		}
		BOOL ShowDropDown(BOOL show)
		{
			return (BOOL) SendMessage(CB_SHOWDROPDOWN, (WPARAM) show, 0);
		}

	};

	//
	// MDIFrameWnd
	//

	class WNDLIB_EXPORT MDIFrameWnd : public Wnd
	{
	public:

		MDIFrameWnd();

		// You must call this
		void SetMdiClient(HWND hwnd)
		{
			_mdiClient = hwnd;
		}

		HWND GetMdiClient() const
		{
			return _mdiClient ;
		}

		virtual LRESULT WndProc(UINT msg, WPARAM wparam, LPARAM lparam);

	protected:

		HWND _mdiClient;
	};

	//
	// MDIClientWnd
	//

	enum
	{
		MDICLIENT_SCROLLING = 1,
		MDICLIENT_SUBCLASS = 2,
		MDICLIENT_NO_CLIENTEDGE = 4,
	};

	class WNDLIB_EXPORT MDIClientWnd : public Wnd
	{
	public:

		enum
		{
			ID_FIRST_CHILD = 0xff00
		};

		MDIClientWnd();
		virtual ~MDIClientWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		virtual bool Create(HWND parent, unsigned flags);
		virtual bool CreateIndirect(const CREATESTRUCT &cs, bool subclass);

		// Override the default child ID
		void SetFirstChildId(UINT id)
		{
			_ccs.idFirstChild = id;
		}

		//
		// Wrappers
		//

		void MdiActivate(HWND child)
		{
			SendMessage(WM_MDIACTIVATE, (WPARAM) child, 0);
		}
		BOOL MdiCascade(WPARAM options = 0)
		{
			return (BOOL) SendMessage(WM_MDICASCADE, options, 0);
		}
		HWND MdiCreate(LPMDICREATESTRUCT cs)
		{
			return (HWND) SendMessage(WM_MDICREATE, 0, (LPARAM) cs);
		}
		void MdiDestroy(HWND child)
		{
			SendMessage(WM_MDIDESTROY, (WPARAM) child, 0);
		}
		HWND MdiGetActive(BOOL *maximized_out = 0)
		{
			return (HWND) SendMessage(WM_MDIGETACTIVE, 0, (LPARAM) maximized_out);
		}
		void MdiIconArrange()
		{
			SendMessage(WM_MDIICONARRANGE, 0, 0);
		}
		void MdiMaximize(HWND child)
		{
			SendMessage(WM_MDIMAXIMIZE, (WPARAM) child, 0);
		}
		void MdiNext(HWND nextTo, BOOL previous = FALSE)
		{
			SendMessage(WM_MDINEXT, (WPARAM) nextTo, previous);
		}
		void MdiRefreshMenu()
		{
			SendMessage(WM_MDIREFRESHMENU, 0, 0);
		}
		void MdiRestore(HWND child)
		{
			SendMessage(WM_MDIRESTORE, (WPARAM) child, 0);
		}
		HMENU MdiSetMenu(HMENU menu, HMENU windowMenu)
		{
			return (HMENU) SendMessage(WM_MDISETMENU, (WPARAM) menu, (LPARAM) windowMenu);
		}
		BOOL MdiTile(WPARAM options = MDITILE_VERTICAL)
		{
			return (BOOL) SendMessage(WM_MDITILE, options, 0);
		}

	protected:

		CLIENTCREATESTRUCT _ccs;
	};

	//
	// RichEditWnd
	//

	class WNDLIB_EXPORT RichEditWnd : public EditWnd
	{
	public:

		static LONG staticLoadLib;

		RichEditWnd();
		virtual ~RichEditWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		#if _WIN32_IE >= 0x500
			LRESULT SetTextEx(SETTEXTEX *options, TCHAR *text)
			{
				return (LRESULT) SendMessage(EM_SETTEXTEX, (WPARAM) options, (LPARAM) text);
			}
			LRESULT GetEditStyle()
			{
				return (LRESULT) SendMessage(EM_GETEDITSTYLE, (WPARAM) 0, (LPARAM) 0);
			}
			LRESULT GetScrollPos(POINT *pt)
			{
				return (LRESULT) SendMessage(EM_GETSCROLLPOS, (WPARAM) 0, (LPARAM) pt);
			}
			LRESULT GetTypographyOptions()
			{
				return (LRESULT) SendMessage(EM_GETTYPOGRAPHYOPTIONS, (WPARAM) 0, (LPARAM) 0);
			}
			BOOL GetZoom(WORD *numerator, WORD *denominator)
			{
				return (BOOL) SendMessage(EM_GETZOOM, (WPARAM) numerator, (LPARAM) denominator);
			}
			LRESULT Reconversion()
			{
				return (LRESULT) SendMessage(EM_RECONVERSION, (WPARAM) 0, (LPARAM) 0);
			}
			LRESULT SetEditStyle(WPARAM opt, LPARAM mask)
			{
				return (LRESULT) SendMessage(EM_SETEDITSTYLE, (WPARAM) opt, (LPARAM) mask);
			}
			BOOL SetFontSize(int delta)
			{
				return (BOOL) SendMessage(EM_SETFONTSIZE, (WPARAM) delta, (LPARAM) 0);
			}
			LRESULT SetScrollPos(POINT *pt)
			{
				return (LRESULT) SendMessage(EM_SETSCROLLPOS, (WPARAM) 0, (LPARAM) pt);
			}
			BOOL SetTypographyOptions(WPARAM option, LPARAM mask)
			{
				return (BOOL) SendMessage(EM_SETTYPOGRAPHYOPTIONS, (WPARAM) option, (LPARAM) mask);
			}
			BOOL SetZoom(WORD numer, WORD denomin)
			{
				return (BOOL) SendMessage(EM_SETZOOM, (WPARAM) numer, (LPARAM) denomin);
			}
			LRESULT ShowScrollBar(WPARAM which, BOOL vis)
			{
				return (LRESULT) SendMessage(EM_SHOWSCROLLBAR, (WPARAM) which, (LPARAM) vis);
			}
		#endif

		LRESULT AutoUrlDetect(BOOL enable)
		{
			return (LRESULT) SendMessage(EM_AUTOURLDETECT, (WPARAM) enable, (LPARAM) 0);
		}
		BOOL CanPaste(WPARAM clipfmt)
		{
			return (BOOL) SendMessage(EM_CANPASTE, (WPARAM) clipfmt, (LPARAM) 0);
		}
		BOOL CanRedo()
		{
			return (BOOL) SendMessage(EM_CANREDO, (WPARAM) 0, (LPARAM) 0);
		}
		BOOL DisplayBand(LPRECT rc)
		{
			return (BOOL) SendMessage(EM_DISPLAYBAND, (WPARAM) 0, (LPARAM) rc);
		}
		LRESULT ExGetSel(CHARRANGE *range)
		{
			return (LRESULT) SendMessage(EM_EXGETSEL, (WPARAM) 0, (LPARAM) range);
		}
		LRESULT ExLimitText(LRESULT limit)
		{
			return (LRESULT) SendMessage(EM_EXLIMITTEXT, (WPARAM) 0, (LPARAM) limit);
		}
		LRESULT ExLineFromChar(LPARAM index)
		{
			return (LRESULT) SendMessage(EM_EXLINEFROMCHAR, (WPARAM) 0, (LPARAM) index);
		}
		LRESULT ExSetSel(CHARRANGE *range)
		{
			return (LRESULT) SendMessage(EM_EXSETSEL, (WPARAM) 0, (LPARAM) range);
		}
		int FindText(WPARAM options, FINDTEXT *textinfo)
		{
			return (int) SendMessage(EM_FINDTEXT, (WPARAM) options, (LPARAM) textinfo);
		}
		int FindTextEx(WPARAM options, FINDTEXTEX *textinfo)
		{
			return (int) SendMessage(EM_FINDTEXTEX, (WPARAM) options, (LPARAM) textinfo);
		}
		int FindTextExW(WPARAM options, FINDTEXTEX *textinfo)
		{
			return (int) SendMessage(EM_FINDTEXTEXW, (WPARAM) options, (LPARAM) textinfo);
		}
		int FindTextW(WPARAM options, FINDTEXT *textinfo)
		{
			return (int) SendMessage(EM_FINDTEXTW, (WPARAM) options, (LPARAM) textinfo);
		}
		LRESULT FindWordBreak(WPARAM op, LPARAM startindex)
		{
			return (LRESULT) SendMessage(EM_FINDWORDBREAK, (WPARAM) op, (LPARAM) startindex);
		}
		DWORD FormatRange(BOOL render, FORMATRANGE *data)
		{
			return (DWORD) SendMessage(EM_FORMATRANGE, (WPARAM) render, (LPARAM) data);
		}
		BOOL GetAutoUrlDetect()
		{
			return (BOOL) SendMessage(EM_GETAUTOURLDETECT, (WPARAM) 0, (LPARAM) 0);
		}
		#ifndef __MINGW32__
			LRESULT GetBidiOptions(BIDIOPTIONS *options)
			{
				return (LRESULT) SendMessage(EM_GETBIDIOPTIONS, (WPARAM) 0, (LPARAM) options);
			}
		#endif
		DWORD GetCharFormat(BOOL options, CHARFORMAT *data)
		{
			return (DWORD) SendMessage(EM_GETCHARFORMAT, (WPARAM) options, (LPARAM) data);
		}
		LRESULT GetEventMask()
		{
			return (LRESULT) SendMessage(EM_GETEVENTMASK, (WPARAM) 0, (LPARAM) 0);
		}
		BOOL GetImeColor(COMPCOLOR *compcol)
		{
			return (BOOL) SendMessage(EM_GETIMECOLOR, (WPARAM) 0, (LPARAM) compcol);
		}
		LRESULT GetImeCompMode()
		{
			return (LRESULT) SendMessage(EM_GETIMECOMPMODE, (WPARAM) 0, (LPARAM) 0);
		}
		LRESULT GetImeOptions()
		{
			return (LRESULT) SendMessage(EM_GETIMEOPTIONS, (WPARAM) 0, (LPARAM) 0);
		}
		LRESULT GetLangOptions()
		{
			return (LRESULT) SendMessage(EM_GETLANGOPTIONS, (WPARAM) 0, (LPARAM) 0);
		}
		BOOL GetOleInterface(LPVOID *ptr)
		{
			return (BOOL) SendMessage(EM_GETOLEINTERFACE, (WPARAM) 0, (LPARAM) ptr);
		}
		LRESULT GetOptions()
		{
			return (LRESULT) SendMessage(EM_GETOPTIONS, (WPARAM) 0, (LPARAM) 0);
		}
		DWORD GetParaFormat(PARAFORMAT *fmt)
		{
			return (DWORD) SendMessage(EM_GETPARAFORMAT, (WPARAM) 0, (LPARAM) fmt);
		}
		BOOL GetPunctuation(WPARAM type, PUNCTUATION *pun)
		{
			return (BOOL) SendMessage(EM_GETPUNCTUATION, (WPARAM) type, (LPARAM) pun);
		}
		UNDONAMEID GetRedoName()
		{
			return (UNDONAMEID) SendMessage(EM_GETREDONAME, (WPARAM) 0, (LPARAM) 0);
		}
		DWORD GetSelText(LPSTR text)
		{
			return (DWORD) SendMessage(EM_GETSELTEXT, (WPARAM) 0, (LPARAM) text);
		}
		DWORD GetTextEx(GETTEXTEX *info, LPTSTR buffer)
		{
			return (DWORD) SendMessage(EM_GETTEXTEX, (WPARAM) info, (LPARAM) buffer);
		}
		DWORD GettextLengthEx(GETTEXTLENGTHEX *tl)
		{
			return (DWORD) SendMessage(EM_GETTEXTLENGTHEX, (WPARAM) tl, (LPARAM) 0);
		}
		TEXTMODE GetTextMode()
		{
			return (TEXTMODE) SendMessage(EM_GETTEXTMODE, (WPARAM) 0, (LPARAM) 0);
		}
		DWORD GetTextRange(TEXTRANGE *range)
		{
			return (DWORD) SendMessage(EM_GETTEXTRANGE, (WPARAM) 0, (LPARAM) range);
		}
		UNDONAMEID GetUndoName()
		{
			return (UNDONAMEID) SendMessage(EM_GETUNDONAME, (WPARAM) 0, (LPARAM) 0);
		}
		LRESULT GetWordBreakProcEx()
		{
			return (LRESULT) SendMessage(EM_GETWORDBREAKPROCEX, (WPARAM) 0, (LPARAM) 0);
		}
		LRESULT GetWordWrapMode()
		{
			return (LRESULT) SendMessage(EM_GETWORDWRAPMODE, (WPARAM) 0, (LPARAM) 0);
		}
		LRESULT HideSelection(BOOL hide)
		{
			return (LRESULT) SendMessage(EM_HIDESELECTION, (WPARAM) hide, (LPARAM) 0);
		}
		LRESULT PasteSpecial(WPARAM clipfmt, REPASTESPECIAL *aspect)
		{
			return (LRESULT) SendMessage(EM_PASTESPECIAL, (WPARAM) clipfmt, (LPARAM) aspect);
		}
		BOOL Redo()
		{
			return (BOOL) SendMessage(EM_REDO, (WPARAM) 0, (LPARAM) 0);
		}
		LRESULT RequestResize()
		{
			return (LRESULT) SendMessage(EM_REQUESTRESIZE, (WPARAM) 0, (LPARAM) 0);
		}
		LRESULT SelectionType()
		{
			return (LRESULT) SendMessage(EM_SELECTIONTYPE, (WPARAM) 0, (LPARAM) 0);
		}
		#ifndef __MINGW32__
			LRESULT SetBidiOptions(BIDIOPTIONS *opt)
			{
				return (LRESULT) SendMessage(EM_SETBIDIOPTIONS, (WPARAM) 0, (LPARAM) opt);
			}
		#endif
		COLORREF SetBkgndColor(int syscol, COLORREF clr)
		{
			return (COLORREF) SendMessage(EM_SETBKGNDCOLOR, (WPARAM) syscol, (LPARAM) clr);
		}
		BOOL SetCharFormat(WPARAM options, CHARFORMAT *fmt)
		{
			return (BOOL) SendMessage(EM_SETCHARFORMAT, (WPARAM) options, (LPARAM) fmt);
		}
		LRESULT SetEventMask(LPARAM mask)
		{
			return (LRESULT) SendMessage(EM_SETEVENTMASK, (WPARAM) 0, (LPARAM) mask);
		}
		BOOL SetImeColor(COMPCOLOR *color)
		{
			return (BOOL) SendMessage(EM_SETIMECOLOR, (WPARAM) 0, (LPARAM) color);
		}
		BOOL SetImeOptions(WPARAM operation, LPARAM options)
		{
			return (BOOL) SendMessage(EM_SETIMEOPTIONS, (WPARAM) operation, (LPARAM) options);
		}
		LRESULT SetLangOptions(LPARAM langopt)
		{
			return (LRESULT) SendMessage(EM_SETLANGOPTIONS, (WPARAM) 0, (LPARAM) langopt);
		}
		BOOL SetOleCallback(struct IRichEditOleCallback *object)
		{
			return (BOOL) SendMessage(EM_SETOLECALLBACK, (WPARAM) 0, (LPARAM) object);
		}
		LRESULT SetOptions(WPARAM operation, LPARAM options)
		{
			return (LRESULT) SendMessage(EM_SETOPTIONS, (WPARAM) operation, (LPARAM) options);
		}
		LRESULT SetPalette(HPALETTE pal)
		{
			return (LRESULT) SendMessage(EM_SETPALETTE, (WPARAM) pal, (LPARAM) 0);
		}
		BOOL SetParaFormat(PARAFORMAT *fmt)
		{
			return (BOOL) SendMessage(EM_SETPARAFORMAT, (WPARAM) 0, (LPARAM) fmt);
		}
		BOOL SetPunctuation(WPARAM type, PUNCTUATION *chars)
		{
			return (BOOL) SendMessage(EM_SETPUNCTUATION, (WPARAM) type, (LPARAM) chars);
		}
		BOOL SetTargetDevice(HDC dc, int linewidth)
		{
			return (BOOL) SendMessage(EM_SETTARGETDEVICE, (WPARAM) dc, (LPARAM) linewidth);
		}
		BOOL SetTextMode(WPARAM mode)
		{
			return (BOOL) SendMessage(EM_SETTEXTMODE, (WPARAM) mode, (LPARAM) 0);
		}
		UINT SetUndoLimit(UINT maxundo)
		{
			return (UINT) SendMessage(EM_SETUNDOLIMIT, (WPARAM) maxundo, (LPARAM) 0);
		}
		LRESULT SetWordBreakProcEx(LPARAM proc)
		{
			return (LRESULT) SendMessage(EM_SETWORDBREAKPROCEX, (WPARAM) 0, (LPARAM) proc);
		}
		LRESULT SetWordWrapMode(WPARAM options)
		{
			return (LRESULT) SendMessage(EM_SETWORDWRAPMODE, (WPARAM) options, (LPARAM) 0);
		}
		LRESULT StopGroupTyping()
		{
			return (LRESULT) SendMessage(EM_STOPGROUPTYPING, (WPARAM) 0, (LPARAM) 0);
		}
		DWORD StreamIn(WPARAM fmtopt, EDITSTREAM *data)
		{
			return (DWORD) SendMessage(EM_STREAMIN, (WPARAM) fmtopt, (LPARAM) data);
		}
		DWORD StreamOut(WPARAM fmtopt, EDITSTREAM *data)
		{
			return (DWORD) SendMessage(EM_STREAMOUT, (WPARAM) fmtopt, (LPARAM) data);
		}

		LRESULT ExSetSel(LONG start, LONG end)
		{
			CHARRANGE c;
			c.cpMin = start;
			c.cpMax = end;
			return (LRESULT) SendMessage(EM_EXSETSEL, (WPARAM) 0, (LPARAM) &c);
		}
	};

	//
	// RichEdit2Wnd
	//

	class WNDLIB_EXPORT RichEdit2Wnd : public RichEditWnd
	{
	public:

		static LONG staticLoadLib;

		RichEdit2Wnd();
		virtual ~RichEdit2Wnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();
	};

	//
	// ImageList
	//

	class ImageList
	{
	public:

		ImageList() : _handle(NULL)
		{
		}
		~ImageList()
		{
			Destroy();
		}

		HIMAGELIST GetHandle() const
		{
			return _handle;
		}
		void Attach(HIMAGELIST list)
		{
			Destroy();
			_handle = list;
		}
		HIMAGELIST Detach()
		{
			HIMAGELIST temp = _handle;
			_handle = NULL;
			return temp;
		}

		int Add(HBITMAP image, HBITMAP mask)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_Add(_handle, image, mask);
		}
		int AddMasked(HBITMAP image, COLORREF mask)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_AddMasked(_handle, image, mask);
		}
		bool Copy(int destindex, HIMAGELIST sourcelist, int sourceindex, UINT flags)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_Copy(_handle, destindex, sourcelist, sourceindex, flags) != FALSE;
		}
		bool Create(int width, int height, UINT flags, int initialsize, int maxsize)
		{
			Destroy();
			_handle = ImageList_Create(width, height, flags, initialsize, maxsize);
			return _handle != NULL;
		}
		void Destroy()
		{
			if (_handle)
			{
				ImageList_Destroy(_handle);
				_handle = NULL;
			}
		}
		bool Draw(int index, HDC dc, int x, int y, UINT style)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_Draw(_handle, index, dc, x, y, style) != FALSE;
		}
		bool DrawEx(int index, HDC dc, int x, int y, int dx, int dy, COLORREF bkcolor,
					COLORREF fgcolor, UINT style)
		{
			WNDLIB_ASSERT(_handle != NULL);
			return ImageList_DrawEx(_handle, index, dc, x, y, dx, dy, bkcolor, fgcolor, style) != FALSE;
		}
		bool DrawIndirect(IMAGELISTDRAWPARAMS *params)
		{
			WNDLIB_ASSERT(_handle != NULL);
			return ImageList_DrawIndirect(params) != FALSE;
		}
		#if _WIN32_IE >= 0x400
			HIMAGELIST Duplicate()
			{
				WNDLIB_ASSERT(_handle != NULL);
				return ImageList_Duplicate(_handle);
			}
		#endif
		COLORREF GetBkColor()
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_GetBkColor(_handle);
		}
		HICON GetIcon(int index, UINT flags)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_GetIcon(_handle, index, flags);
		}
		bool GetIconSize(int *cx, int *cy)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_GetIconSize(_handle, cx, cy) != FALSE;
		}
		int GetImageCount()
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_GetImageCount(_handle);
		}
		bool GetImageInfo(int i, IMAGEINFO *infoout)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_GetImageInfo(_handle, i, infoout) != FALSE;
		}
		bool LoadImage(HINSTANCE inst, LPCTSTR resourcename, int imagewidth,
			int maximages, COLORREF mask, UINT type, UINT flags)
		{
			Destroy();
			_handle = ImageList_LoadImage(inst, resourcename, imagewidth, maximages, mask, type, flags);
			return _handle != NULL;
		}
		bool Merge(HIMAGELIST list1, int i1, HIMAGELIST list2, int i2, int dx, int dy)
		{
			Destroy();
			_handle = ImageList_Merge(list1, i1, list2, i2, dx, dy);
			return _handle != NULL;
		}
		/*bool Read(LPSTREAM stream)
		{
			Destroy();
			_handle = ImageList_Read(stream);
			return _handle != NULL;
		}*/
		bool Remove(int i)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_Remove(_handle, i) != FALSE;
		}
		bool Replace(int i, HBITMAP bitmap, HBITMAP mask)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_Replace(_handle, i, bitmap, mask) != FALSE;
		}
		int ReplaceIcon(int i, HICON icon)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_ReplaceIcon(_handle, i, icon);
		}
		COLORREF SetBkColor(COLORREF bkcolor)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_SetBkColor(_handle, bkcolor);
		}
		bool SetIconSize(int width, int height)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_SetIconSize(_handle, width, height) != FALSE;
		}
		bool SetImageCount(UINT newcount)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_SetImageCount(_handle, newcount) != FALSE;
		}
		bool SetOverlayImage(int iimage, int ioverlay)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_SetOverlayImage(_handle, iimage, ioverlay) != FALSE;
		}
		/*bool Write(LPSTREAM stream)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_Write(_handle, stream) != FALSE;
		}*/
		int AddIcon(HICON icon)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_AddIcon(_handle, icon);
		}
		HICON ExtractIcon(int i)
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_ExtractIcon(NULL, _handle, i);
		}
		bool LoadBitmap(HINSTANCE inst, LPCTSTR resourcename, int imagewidth,
			int maximages, COLORREF mask)
			{
			Destroy();
			_handle = ImageList_LoadBitmap(inst, resourcename, imagewidth, maximages, mask);
			return _handle != NULL;
		}
		bool RemoveAll()
		{
			WNDLIB_ASSERT(_handle != NULL); return ImageList_RemoveAll(_handle) != FALSE;
		}
		static UINT IndexToOverlayMask(UINT overlay)
		{
			return INDEXTOOVERLAYMASK(overlay);
		}

	private:

		HIMAGELIST _handle;
	};

	//
	// ComboBoxExWnd
	//

	class WNDLIB_EXPORT ComboBoxExWnd : public ComboBoxWnd
	{
	public:

		ComboBoxExWnd();
		virtual ~ComboBoxExWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		INT DeleteItem(int index)
		{
			return (INT) SendMessage(CBEM_DELETEITEM, (WPARAM) index, 0);
		}
		HWND GetComboControl()
		{
			return (HWND) SendMessage(CBEM_GETCOMBOCONTROL, 0, 0);
		}
		HWND GetEditControl()
		{
			return (HWND) SendMessage(CBEM_GETEDITCONTROL, 0, 0);
		}
		HIMAGELIST GetImageList()
		{
			return (HIMAGELIST) SendMessage(CBEM_GETIMAGELIST, 0, 0);
		}
		BOOL GetItem(PCOMBOBOXEXITEM item)
		{
			return (BOOL) SendMessage(CBEM_GETITEM, 0, (LPARAM) item);
		}
		BOOL HasEditChanged()
		{
			return (BOOL) SendMessage(CBEM_HASEDITCHANGED, 0, 0);
		}
		INT InsertItem(const COMBOBOXEXITEM *item)
		{
			return (INT) SendMessage(CBEM_INSERTITEM, 0, (LPARAM) item);
		}
		HIMAGELIST SetImageList(HIMAGELIST imagelist)
		{
			return (HIMAGELIST) SendMessage(CBEM_SETIMAGELIST, 0, (LPARAM) imagelist);
		}
		BOOL SetItem(const COMBOBOXEXITEM *item)
		{
			return (BOOL) SendMessage(CBEM_SETITEM, 0, (LPARAM) item);
		}

		#if _WIN32_IE >= 0x400
			DWORD GetExtendedStyle()
			{
				return (DWORD) SendMessage(CBEM_GETEXTENDEDSTYLE, 0, 0);
			}
			BOOL GetUnicodeFormat()
			{
				return (BOOL) SendMessage(CBEM_GETUNICODEFORMAT, 0, 0);
			}
			DWORD SetExtendedStyle(DWORD mask, DWORD style)
			{
				return (DWORD) SendMessage(CBEM_SETEXTENDEDSTYLE, (WPARAM) mask, (WPARAM) style);
			}
			BOOL SetUnicodeFormat(BOOL unicode)
			{
				return (BOOL) SendMessage(CBEM_SETUNICODEFORMAT, (WPARAM) unicode, 0);
			}
		#endif
	};

	//
	// DateTimePickerWnd
	//

	class WNDLIB_EXPORT DateTimePickerWnd : public Wnd
	{
	public:

		DateTimePickerWnd();
		virtual ~DateTimePickerWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		#if _WIN32_IE >= 0x400
			HFONT GetMonthCalFont()
			{
				return (HFONT) SendMessage(DTM_GETMCFONT, 0, 0);
			}
			void SetMonthCalFont(HFONT font, BOOL redraw = TRUE)
			{
				SendMessage(DTM_SETMCFONT, (WPARAM) font, MAKELPARAM(redraw, 0));
			}
		#endif

		COLORREF GetMonthCalColor(INT color)
		{
			return (COLORREF) SendMessage(DTM_GETMCCOLOR, (WPARAM) color, 0);
		}
		HWND GetMonthCal()
		{
			return (HWND) SendMessage(DTM_GETMONTHCAL, 0, 0);
		}
		DWORD GetRange(SYSTEMTIME *arrayOf2)
		{
			return (DWORD) SendMessage(DTM_GETRANGE, 0, (LPARAM) arrayOf2);
		}
		DWORD GetSystemTime(LPSYSTEMTIME st)
		{
			return (DWORD) SendMessage(DTM_GETSYSTEMTIME, 0, (LPARAM) st);
		}
		BOOL SetFormat(LPCTSTR format)
		{
			return (BOOL) SendMessage(DTM_SETFORMAT, 0, (LPARAM) format);
		}
		COLORREF SetMonthCalColor(INT icolor, COLORREF color)
		{
			return (COLORREF) SendMessage(DTM_SETMCCOLOR, (WPARAM) icolor, (LPARAM) color);
		}
		BOOL SetRange(DWORD flags, const SYSTEMTIME *arrayOf2)
		{
			return (BOOL) SendMessage(DTM_SETRANGE, (WPARAM) flags, (LPARAM) arrayOf2);
		}
		BOOL SetSystemTime(DWORD flag, const SYSTEMTIME *st)
		{
			return (BOOL) SendMessage(DTM_SETSYSTEMTIME, (WPARAM) flag, (LPARAM) st);
		}
	};

	//
	// HotKeyWnd
	//

	class WNDLIB_EXPORT HotKeyWnd : public Wnd
	{
	public:

		HotKeyWnd();
		virtual ~HotKeyWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		UINT GetHotKey()
		{
			return (UINT) SendMessage(HKM_GETHOTKEY, 0, 0);
		}
		void SetHotKey(UINT virtkey, UINT mods)
		{
			SendMessage(HKM_SETHOTKEY, MAKEWORD(virtkey, mods), 0);
		}
		void SetRules(UINT invalidcomb, UINT modifier)
		{
			SendMessage(HKM_SETRULES, (WPARAM) invalidcomb, MAKELPARAM(modifier, 0));
		}
	};

	//
	// IPAddressWnd
	//

	#if _WIN32_IE >= 0x400

	class WNDLIB_EXPORT IPAddressWnd : public Wnd
	{
	public:

		IPAddressWnd();
		virtual ~IPAddressWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		void ClearAddress()
		{
			SendMessage(IPM_CLEARADDRESS, 0, 0);
		}
		int GetAddress(LPDWORD addressOut)
		{
			return (int) SendMessage(IPM_GETADDRESS, 0, (LPARAM) addressOut);
		}
		BOOL IsBlank()
		{
			return (BOOL) SendMessage(IPM_ISBLANK, 0, 0);
		}
		void SetAddress(DWORD address)
		{
			SendMessage(IPM_SETADDRESS, 0, (LPARAM) address); }

		void SetFocusOnField(int fieldindex)
		{
			SendMessage(IPM_SETFOCUS, (WPARAM) fieldindex, 0);
		}
		BOOL SetRange(int fieldindex, BYTE low, BYTE high)
		{
			return (BOOL) SendMessage(IPM_SETRANGE, (WPARAM) fieldindex, MAKEIPRANGE(low, high));
		}
	};

	#endif // _WIN32_IE >= 0x400

	//
	// ListViewWnd
	//

	class WNDLIB_EXPORT ListViewWnd : public Wnd
	{
	public:
		ListViewWnd();
		virtual ~ListViewWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// Helpers
		//

		// Select the i'th entry in the list (this is a WndLib extension)
		void SetSelection(int select);

		// Get the single selected item in the list, return -1 otherwise
		int GetSingleSelection();

		// Add a column, return it's index
		int AddColumn(LPCTSTR text, int width);

		// Insert an item in the list simply by providing text for each entry.
		// Note that by default, image is I_IMAGENONE.
		void InsertItem(int item, int numColumns, LPCTSTR *columns, int image = -2);

		//
		// API wrappers
		//

		#if _WIN32_IE >= 0x400
			BOOL GetBkImage(LPLVBKIMAGE bki)
			{
				return (BOOL) SendMessage(LVM_GETBKIMAGE, (WPARAM) bki, (LPARAM) 0);
			}
			BOOL SetBkImage(LPLVBKIMAGE bk)
			{
				return (BOOL) SendMessage(LVM_SETBKIMAGE, (WPARAM) 0, (LPARAM) bk);
			}
			DWORD GetHoverTime()
			{
				return (DWORD) SendMessage(LVM_GETHOVERTIME, (WPARAM) 0, (LPARAM) 0);
			}
			LRESULT GetNumberOfWorkAreas(LPUINT workareas)
			{
				return (LRESULT) SendMessage(LVM_GETNUMBEROFWORKAREAS, (WPARAM) 0, (LPARAM) workareas);
			}
			int GetSelectionMark()
			{
				return (int) SendMessage(LVM_GETSELECTIONMARK, (WPARAM) 0, (LPARAM) 0);
			}
			HANDLE GetToolTips()
			{
				return (HANDLE) SendMessage(LVM_GETTOOLTIPS, (WPARAM) 0, (LPARAM) 0);
			}
			LRESULT GetUnicodeFormat()
			{
				return (LRESULT) SendMessage(LVM_GETUNICODEFORMAT, (WPARAM) 0, (LPARAM) 0);
			}
			LRESULT GetWorkAreas(int workareas, LPRECT lprc)
			{
				return (LRESULT) SendMessage(LVM_GETWORKAREAS, (WPARAM) workareas, (LPARAM) lprc);
			}
			DWORD SetHoverTime(DWORD hovertime)
			{
				return (DWORD) SendMessage(LVM_SETHOVERTIME, (WPARAM) 0, (LPARAM) hovertime);
			}
			int SetSelectionMark(int index)
			{
				return (int) SendMessage(LVM_SETSELECTIONMARK, (WPARAM) 0, (LPARAM) index);
			}
			HWND SetToolTips(HWND ttwnd)
			{
				return (HWND) SendMessage(LVM_SETTOOLTIPS, (WPARAM) 0, (LPARAM) ttwnd);
			}
			BOOL SetUnicodeFormat(BOOL unicode)
			{
				return (BOOL) SendMessage(LVM_SETUNICODEFORMAT, (WPARAM) unicode, (LPARAM) 0);
			}
			LRESULT SetWorkAreas(int num, LPRECT lprc)
			{
				return (LRESULT) SendMessage(LVM_SETWORKAREAS, (WPARAM) num, (LPARAM) lprc);
			}
		#endif

		#if _WIN32_IE >= 0x500
			BOOL SortItemsEx(LPARAM lparam, PFNLVCOMPARE compare)
			{
				return (BOOL) SendMessage(LVM_SORTITEMSEX, (WPARAM) lparam, (LPARAM) compare);
			}
		#endif

		DWORD ApproximateViewRect(int count, int cx, int cy)
		{
			return (DWORD) SendMessage(LVM_APPROXIMATEVIEWRECT, (WPARAM) count, (LPARAM) MAKELPARAM(cx,cy));
		}
		BOOL Arrange(int code)
		{
			return (BOOL) SendMessage(LVM_ARRANGE, (WPARAM) code, (LPARAM) 0);
		}
		HIMAGELIST CreateDragImage(int item, LPPOINT upleft)
		{
			return (HIMAGELIST) SendMessage(LVM_CREATEDRAGIMAGE, (WPARAM) item, (LPARAM) upleft);
		}
		BOOL DeleteAllItems()
		{
			return (BOOL) SendMessage(LVM_DELETEALLITEMS, (WPARAM) 0, (LPARAM) 0);
		}
		LRESULT DeleteColumn(int col)
		{
			return (LRESULT) SendMessage(LVM_DELETECOLUMN, (WPARAM) col, (LPARAM) 0);
		}
		BOOL DeleteItem(int item)
		{
			return (BOOL) SendMessage(LVM_DELETEITEM, (WPARAM) item, (LPARAM) 0);
		}
		HWND EditLabel(int item)
		{
			return (HWND) SendMessage(LVM_EDITLABEL, (WPARAM) item, (LPARAM) 0);
		}
		BOOL EnsureVisible(int item, BOOL partialok)
		{
			return (BOOL) SendMessage(LVM_ENSUREVISIBLE, (WPARAM) item, (LPARAM) partialok);
		}
		/*int FindItem(int start, LPLVFINDINFO fi)
		{
			return (int) SendMessage(LVM_FINDITEM, (WPARAM) start, (LPARAM) fi);
		}*/
		COLORREF GetBkColor()
		{
			return (COLORREF) SendMessage(LVM_GETBKCOLOR, (WPARAM) 0, (LPARAM) 0);
		}
		LRESULT GetCallBackMask()
		{
			return (LRESULT) SendMessage(LVM_GETCALLBACKMASK, (WPARAM) 0, (LPARAM) 0);
		}
		BOOL GetColumn(int col, LVCOLUMN *pcol)
		{
			return (BOOL) SendMessage(LVM_GETCOLUMN, (WPARAM) col, (LPARAM) pcol);
		}
		BOOL GetColumnOrderArray(int count, int *array)
		{
			return (BOOL) SendMessage(LVM_GETCOLUMNORDERARRAY, (WPARAM) count, (LPARAM) array);
		}
		int GetColumnWidth(int col)
		{
			return (int) SendMessage(LVM_GETCOLUMNWIDTH, (WPARAM) col, (LPARAM) 0);
		}
		int GetCountPerPage()
		{
			return (int) SendMessage(LVM_GETCOUNTPERPAGE, (WPARAM) 0, (LPARAM) 0);
		}
		HWND GetEditControl()
		{
			return (HWND) SendMessage(LVM_GETEDITCONTROL, (WPARAM) 0, (LPARAM) 0);
		}
		DWORD GetExtendedListViewStyle()
		{
			return (DWORD) SendMessage(LVM_GETEXTENDEDLISTVIEWSTYLE, (WPARAM) 0, (LPARAM) 0);
		}
		HWND GetHeader()
		{
			return (HWND) SendMessage(LVM_GETHEADER, (WPARAM) 0, (LPARAM) 0);
		}
		HCURSOR GetHotCursor()
		{
			return (HCURSOR) SendMessage(LVM_GETHOTCURSOR, (WPARAM) 0, (LPARAM) 0);
		}
		int GetHotItem()
		{
			return (int) SendMessage(LVM_GETHOTITEM, (WPARAM) 0, (LPARAM) 0);
		}
		HIMAGELIST GetImageList(int imagelistindex)
		{
			return (HIMAGELIST) SendMessage(LVM_GETIMAGELIST, (WPARAM) imagelistindex, (LPARAM) 0);
		}
		int GetISearchString(LPTSTR lpsz)
		{
			return (int) SendMessage(LVM_GETISEARCHSTRING, (WPARAM) 0, (LPARAM) lpsz);
		}
		BOOL GetItem(LVITEM *item)
		{
			return (BOOL) SendMessage(LVM_GETITEM, (WPARAM) 0, (LPARAM) item);
		}
		int GetItemCount()
		{
			return (int) SendMessage(LVM_GETITEMCOUNT, (WPARAM) 0, (LPARAM) 0);
		}
		BOOL GetItemPosition(int item, POINT *pt)
		{
			return (BOOL) SendMessage(LVM_GETITEMPOSITION, (WPARAM) item, (LPARAM) pt);
		}
		BOOL GetItemRect(int i, LPRECT prc)
		{
			return (BOOL) SendMessage(LVM_GETITEMRECT, (WPARAM) i, (LPARAM) prc);
		}
		DWORD GetItemSpacing(BOOL _small)
		{
			return (DWORD) SendMessage(LVM_GETITEMSPACING, (WPARAM) _small, (LPARAM) 0);
		}
		DWORD GetItemState(int i, UINT mask)
		{
			return (DWORD) SendMessage(LVM_GETITEMSTATE, (WPARAM) i, (LPARAM) mask);
		}
		int GetItemText(int item, LVITEM *pitem)
		{
			return (int) SendMessage(LVM_GETITEMTEXT, (WPARAM) item, (LPARAM) pitem);
		}
		int GetNextItem(int start, UINT flags)
		{
			return (int) SendMessage(LVM_GETNEXTITEM, (WPARAM) start, (LPARAM) MAKELPARAM(flags, 0));
		}
		BOOL GetOrigin(LPPOINT pptorg)
		{
			return (BOOL) SendMessage(LVM_GETORIGIN, (WPARAM) 0, (LPARAM) pptorg);
		}
		int GetSelectedCount()
		{
			return (int) SendMessage(LVM_GETSELECTEDCOUNT, (WPARAM) 0, (LPARAM) 0);
		}
		int GetStringWidth(LPCTSTR str)
		{
			return (int) SendMessage(LVM_GETSTRINGWIDTH, (WPARAM) str, (LPARAM) 0);
		}
		BOOL GetSubItemRect(int item,LPRECT rect)
		{
			return (BOOL) SendMessage(LVM_GETSUBITEMRECT, (WPARAM) item, (LPARAM) rect);
		}
		COLORREF GetTextBkColor()
		{
			return (COLORREF) SendMessage(LVM_GETTEXTBKCOLOR, (WPARAM) 0, (LPARAM) 0);
		}
		COLORREF GetTextColor()
		{
			return (COLORREF) SendMessage(LVM_GETTEXTCOLOR, (WPARAM) 0, (LPARAM) 0);
		}
		int GetTopIndex()
		{
			return (int) SendMessage(LVM_GETTOPINDEX, (WPARAM) 0, (LPARAM) 0);
		}
		BOOL GetViewRect(RECT *prc)
		{
			return (BOOL) SendMessage(LVM_GETVIEWRECT, (WPARAM) 0, (LPARAM) prc);
		}
		int HitTest(LPLVHITTESTINFO pinfo)
		{
			return (int) SendMessage(LVM_HITTEST, (WPARAM) 0, (LPARAM) pinfo);
		}
		int InsertColumn(int col, const LVCOLUMN *pcol)
		{
			return (int) SendMessage(LVM_INSERTCOLUMN, (WPARAM) col, (LPARAM) pcol);
		}
		int InsertItem(const LVITEM *pitem)
		{
			return (int) SendMessage(LVM_INSERTITEM, (WPARAM) 0, (LPARAM) pitem);
		}
		BOOL RedrawItems(int first, int last)
		{
			return (BOOL) SendMessage(LVM_REDRAWITEMS, (WPARAM) first, (LPARAM) last);
		}
		BOOL Scroll(int dx, int dy)
		{
			return (BOOL) SendMessage(LVM_SCROLL, (WPARAM) dx, (LPARAM) dy);
		}
		BOOL SetBkColor(COLORREF bk)
		{
			return (BOOL) SendMessage(LVM_SETBKCOLOR, (WPARAM) 0, (LPARAM) bk);
		}
		BOOL SetCallBackMask(UINT mask)
		{
			return (BOOL) SendMessage(LVM_SETCALLBACKMASK, (WPARAM) mask, (LPARAM) 0);
		}
		BOOL SetColumn(int icol, const LVCOLUMN *pcol)
		{
			return (BOOL) SendMessage(LVM_SETCOLUMN, (WPARAM) icol, (LPARAM) pcol);
		}
		BOOL SetColumnOrderArray(int icount, LPINT lpiarray)
		{
			return (BOOL) SendMessage(LVM_SETCOLUMNORDERARRAY, (WPARAM) icount, (LPARAM) lpiarray);
		}
		BOOL SetColumnWidth(int col, int cx)
		{
			return (BOOL) SendMessage(LVM_SETCOLUMNWIDTH, (WPARAM) col, (LPARAM) MAKELPARAM(cx, 0));
		}
		DWORD SetExtendedListViewStyle(DWORD mask, DWORD style)
		{
			return (DWORD) SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, (WPARAM) mask, (LPARAM) style);
		}
		HCURSOR SetHotCursor(HCURSOR hc)
		{
			return (HCURSOR) SendMessage(LVM_SETHOTCURSOR, (WPARAM) 0, (LPARAM) hc);
		}
		int SetHotItem(int index)
		{
			return (int) SendMessage(LVM_SETHOTITEM, (WPARAM) index, (LPARAM) 0);
		}
		DWORD SetIconSpacing(int cx, int cy)
		{
			return (DWORD) SendMessage(LVM_SETICONSPACING, (WPARAM) 0, (LPARAM) MAKELONG(cx, cy));
		}
		HIMAGELIST SetImageList(int imagelist, HIMAGELIST h)
		{
			return (HIMAGELIST) SendMessage(LVM_SETIMAGELIST, (WPARAM) imagelist, (LPARAM) h);
		}
		BOOL SetItem(const LVITEM *item)
		{
			return (BOOL) SendMessage(LVM_SETITEM, (WPARAM) 0, (LPARAM) item);
		}
		BOOL SetItemCount(int citems, DWORD flags)
		{
			return (BOOL) SendMessage(LVM_SETITEMCOUNT, (WPARAM) citems, (LPARAM) flags);
		}
		BOOL SetItemPosition(int i, int x, int y)
		{
			return (BOOL) SendMessage(LVM_SETITEMPOSITION, (WPARAM) i, (LPARAM) MAKELPARAM(x, y));
		}
		LRESULT SetItemPosition32(int item, LPPOINT pt)
		{
			return (LRESULT) SendMessage(LVM_SETITEMPOSITION32, (WPARAM) item, (LPARAM) pt);
		}
		BOOL SetItemState(int i, LVITEM *item)
		{
			return (BOOL) SendMessage(LVM_SETITEMSTATE, (WPARAM) i, (LPARAM) item);
		}
		BOOL SetItemText(int i, LVITEM *item)
		{
			return (BOOL) SendMessage(LVM_SETITEMTEXT, (WPARAM) i, (LPARAM) item);
		}
		BOOL SetTextBkColor(COLORREF clr)
		{
			return (BOOL) SendMessage(LVM_SETTEXTBKCOLOR, (WPARAM) 0, (LPARAM) clr);
		}
		BOOL SetTextColor(COLORREF clr)
		{
			return (BOOL) SendMessage(LVM_SETTEXTCOLOR, (WPARAM) 0, (LPARAM) clr);
		}
		BOOL SortItems(LPARAM lparam, PFNLVCOMPARE compare)
		{
			return (BOOL) SendMessage(LVM_SORTITEMS, (WPARAM) lparam, (LPARAM) compare);
		}
		int SubItemHitTest(LVHITTESTINFO *info)
		{
			return (int) SendMessage(LVM_SUBITEMHITTEST, (WPARAM) 0, (LPARAM) info);
		}
		BOOL Update(int item)
		{
			return (BOOL) SendMessage(LVM_UPDATE, (WPARAM) item, (LPARAM) 0);
		}
	};

	//
	// ProgressBarWnd
	//

	class WNDLIB_EXPORT ProgressBarWnd : public Wnd
	{
	public:
		ProgressBarWnd();
		virtual ~ProgressBarWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		#if _WIN32_IE >= 0x400
			COLORREF SetBarColor(COLORREF clrbar)
			{
				return (COLORREF) SendMessage(PBM_SETBARCOLOR, (WPARAM) 0, (LPARAM) clrbar);
			}
			COLORREF SetBkColor(COLORREF clrbk)
			{
				return (COLORREF) SendMessage(PBM_SETBKCOLOR, (WPARAM) 0, (LPARAM) clrbk);
			}
		#endif

		LRESULT DeltaPos(INT nincrement)
		{
			return (LRESULT) SendMessage(PBM_DELTAPOS, (WPARAM) nincrement, (LPARAM) 0);
		}
		UINT GetPos()
		{
			return (UINT) SendMessage(PBM_GETPOS, (WPARAM) 0, (LPARAM) 0);
		}
		INT GetRange(BOOL whichlimit_true_is_high, PPBRANGE ppbrange)
		{
			return (INT) SendMessage(PBM_GETRANGE, (WPARAM) whichlimit_true_is_high, (LPARAM) ppbrange);
		}
		UINT SetPos(INT newpos)
		{
			return (UINT) SendMessage(PBM_SETPOS, (WPARAM) newpos, (LPARAM) 0);
		}
		DWORD SetRange(UINT minrange, UINT maxrange)
		{
			return (DWORD) SendMessage(PBM_SETRANGE, (WPARAM) 0, (LPARAM) MAKELPARAM(minrange, maxrange));
		}
		DWORD SetRange32(INT lowlim, INT highlim)
		{
			return (DWORD) SendMessage(PBM_SETRANGE32, (WPARAM) lowlim, (LPARAM) highlim);
		}
		INT SetStep(INT nstepinc)
		{
			return (INT) SendMessage(PBM_SETSTEP, (WPARAM) nstepinc, (LPARAM) 0);
		}
		INT StepIt()
		{
			return (INT) SendMessage(PBM_STEPIT, (WPARAM) 0, (LPARAM) 0);
		}
	};

	//
	// TabControlWnd
	//

	class WNDLIB_EXPORT TabControlWnd : public Wnd
	{
	public:
		TabControlWnd();
		virtual ~TabControlWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		#if _WIN32_IE >= 0x400
			DWORD GetExtendedStyle()
			{
				return (DWORD) SendMessage(TCM_GETEXTENDEDSTYLE, (WPARAM) 0, (LPARAM) 0);
			}
			BOOL GetUnicodeFormat()
			{
				return (BOOL) SendMessage(TCM_GETUNICODEFORMAT, (WPARAM) 0, (LPARAM) 0);
			}
			BOOL HighlightItem(int itemId, BOOL highlight)
			{
				return (BOOL) SendMessage(TCM_HIGHLIGHTITEM, (WPARAM) itemId, (LPARAM) MAKELONG(highlight, 0));
			}
			DWORD SetExtendedStyle(DWORD mask, DWORD style)
			{
				return (DWORD) SendMessage(TCM_SETEXTENDEDSTYLE, (WPARAM) mask, (LPARAM) style);
			}
			BOOL SetUnicodeFormat(BOOL unicode)
			{
				return (BOOL) SendMessage(TCM_SETUNICODEFORMAT, (WPARAM) unicode, (LPARAM) 0);
			}
		#endif

		LRESULT AdjustRect(BOOL larger, LPRECT rc)
		{
			return (LRESULT) SendMessage(TCM_ADJUSTRECT, (WPARAM) larger, (LPARAM) rc);
		}
		BOOL DeleteAllItems()
		{
			return (BOOL) SendMessage(TCM_DELETEALLITEMS, (WPARAM) 0, (LPARAM) 0);
		}
		BOOL DeleteItem(int item)
		{
			return (BOOL) SendMessage(TCM_DELETEITEM, (WPARAM) item, (LPARAM) 0);
		}
		LRESULT DeselectAll(BOOL excludefocus)
		{
			return (LRESULT) SendMessage(TCM_DESELECTALL, (WPARAM) excludefocus, (LPARAM) 0);
		}
		int GetCurFocus()
		{
			return (int) SendMessage(TCM_GETCURFOCUS, (WPARAM) 0, (LPARAM) 0);
		}
		int GetCurSel()
		{
			return (int) SendMessage(TCM_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
		}
		HIMAGELIST GetImageList()
		{
			return (HIMAGELIST) SendMessage(TCM_GETIMAGELIST, (WPARAM) 0, (LPARAM) 0);
		}
		BOOL GetItem(int item, LPTCITEM tcitem)
		{
			return (BOOL) SendMessage(TCM_GETITEM, (WPARAM) item, (LPARAM) tcitem);
		}
		int GetItemCount()
		{
			return (int) SendMessage(TCM_GETITEMCOUNT, (WPARAM) 0, (LPARAM) 0);
		}
		BOOL GetItemRect(int item, LPRECT rc)
		{
			return (BOOL) SendMessage(TCM_GETITEMRECT, (WPARAM) item, (LPARAM) rc);
		}
		int GetRowCount()
		{
			return (int) SendMessage(TCM_GETROWCOUNT, (WPARAM) 0, (LPARAM) 0);
		}
		HWND GetToolTips()
		{
			return (HWND) SendMessage(TCM_GETTOOLTIPS, (WPARAM) 0, (LPARAM) 0);
		}
		int HitTest(LPTCHITTESTINFO info)
		{
			return (int) SendMessage(TCM_HITTEST, (WPARAM) 0, (LPARAM) info);
		}
		int InsertItem(int item, const LPTCITEM tcitem)
		{
			return (int) SendMessage(TCM_INSERTITEM, (WPARAM) item, (LPARAM) tcitem);
		}
		LRESULT RemoveImage(int image)
		{
			return (LRESULT) SendMessage(TCM_REMOVEIMAGE, (WPARAM) image, (LPARAM) 0);
		}
		LRESULT SetCurFocus(int item)
		{
			return (LRESULT) SendMessage(TCM_SETCURFOCUS, (WPARAM) item, (LPARAM) 0);
		}
		int SetCurSel(int item)
		{
			return (int) SendMessage(TCM_SETCURSEL, (WPARAM) item, (LPARAM) 0);
		}
		HIMAGELIST SetImageList(HIMAGELIST iml)
		{
			return (HIMAGELIST) SendMessage(TCM_SETIMAGELIST, (WPARAM) 0, (LPARAM) iml);
		}
		BOOL SetItem(int item, LPTCITEM tcitem)
		{
			return (BOOL) SendMessage(TCM_SETITEM, (WPARAM) item, (LPARAM) tcitem);
		}
		BOOL SetItemExtra(int cb)
		{
			return (BOOL) SendMessage(TCM_SETITEMEXTRA, (WPARAM) cb, (LPARAM) 0);
		}
		DWORD SetItemSize(int cx, int cy)
		{
			return (DWORD) SendMessage(TCM_SETITEMSIZE, (WPARAM) 0, (LPARAM) MAKELPARAM(cx, cy));
		}
		int SetMinTabWidth(int cx)
		{
			return (int) SendMessage(TCM_SETMINTABWIDTH, (WPARAM) 0, (LPARAM) cx);
		}
		LRESULT SetPadding(int cx, int cy)
		{
			return (LRESULT) SendMessage(TCM_SETPADDING, (WPARAM) 0, (LPARAM) MAKELPARAM(cx, cy));
		}
		LRESULT SetToolTips(HWND hwnd)
		{
			return (LRESULT) SendMessage(TCM_SETTOOLTIPS, (WPARAM) hwnd, (LPARAM) 0);
		}
	};

	//
	// PropertySheetWnd
	//

	class WNDLIB_EXPORT PropertySheetWnd : public TabControlWnd
	{
		WND_WM_DECLARE(PropertySheetWnd, TabControlWnd)

	public:

		PropertySheetWnd();
		virtual ~PropertySheetWnd();

		// Create the property sheet
		bool Create(HWND parent, int x, int y, int width, int height, int dlgid = -1);

		// Add a new tab, returns it's index
		int AddTab(LPCTSTR tabtext);

		//
		// API wrappers
		//

		#if _WIN32_IE >= 0x500
			#ifdef PSM_GETRESULT
				int GetResult()
				{
					return (int) SendMessage(PSM_GETRESULT, (WPARAM) 0, (LPARAM) 0);
				}
			#endif
			#ifdef PSM_HWNDTOINDEX
				int HWndToIndex(HWND hpagedlg)
				{
					return (int) SendMessage(PSM_HWNDTOINDEX, (WPARAM) hpagedlg, (LPARAM) 0);
				}
			#endif
			#ifdef PSM_IDTOINDEX
				int IdToIndex(int pageid)
				{
					return (int) SendMessage(PSM_IDTOINDEX, (WPARAM) 0, (LPARAM) pageid);
				}
			#endif
			#ifdef PSM_INDEXTOHWND
				HWND IndexToHWnd(int ipageindex)
				{
					return (HWND) SendMessage(PSM_INDEXTOHWND, (WPARAM) ipageindex, (LPARAM) 0);
				}
			#endif
			#ifdef PSM_INDEXTOID
				int IndexToId(int ipageindex)
				{
					return (int) SendMessage(PSM_INDEXTOID, (WPARAM) ipageindex, (LPARAM) 0);
				}
			#endif
			#ifdef PSM_INDEXTOPAGE
				HPROPSHEETPAGE IndexToPage(int ipageindex)
				{
					return (HPROPSHEETPAGE) SendMessage(PSM_INDEXTOPAGE, (WPARAM) ipageindex, (LPARAM) 0);
				}
			#endif
			#ifdef PSM_PAGETOINDEX
				int PageToIndex(HPROPSHEETPAGE hpage)
				{
					return (int) SendMessage(PSM_PAGETOINDEX, (WPARAM) 0, (LPARAM) hpage);
				}
			#endif
			#ifdef PSM_RECALCPAGESIZES
				BOOL RecalcPageSizes()
				{
					return (BOOL) SendMessage(PSM_RECALCPAGESIZES, (WPARAM) 0, (LPARAM) 0);
				}
			#endif
			#ifdef PSM_SETHEADERSUBTITLE
				LRESULT SetHeaderSubTitle(int ipageindex, LPCTSTR pszheadersubtitle)
				{
					return (LRESULT) SendMessage(PSM_SETHEADERSUBTITLE, (WPARAM) ipageindex, (LPARAM) pszheadersubtitle);
				}
			#endif
			#ifdef PSM_SETHEADERTITLE
				LRESULT SetHeaderTitle(int ipageindex, LPCTSTR pszheadertitle)
				{
					return (LRESULT) SendMessage(PSM_SETHEADERTITLE, (WPARAM) ipageindex, (LPARAM) pszheadertitle);
				}
			#endif
			#ifdef PSM_INSERTPAGE
				BOOL InsertPage(int index, HPROPSHEETPAGE page)
				{
					return (BOOL) SendMessage(PSM_INSERTPAGE, (WPARAM) index, (LPARAM) page);
				}
			#endif
			#ifdef PSM_INSERTPAGE
				BOOL InsertPage(HPROPSHEETPAGE after, HPROPSHEETPAGE page)
				{
					return (BOOL) SendMessage(PSM_INSERTPAGE, (WPARAM) after, (LPARAM) page);
				}
			#endif
		#endif

		BOOL AddPage(HPROPSHEETPAGE hpage)
		{
			return (BOOL) SendMessage(PSM_ADDPAGE, (WPARAM) 0, (LPARAM) hpage);
		}
		BOOL Apply()
		{
			return (BOOL) SendMessage(PSM_APPLY, (WPARAM) 0, (LPARAM) 0);
		}
		LRESULT CancelToClose()
		{
			return (LRESULT) SendMessage(PSM_CANCELTOCLOSE, (WPARAM) 0, (LPARAM) 0);
		}
		LRESULT Changed(HWND hwndpage)
		{
			return (LRESULT) SendMessage(PSM_CHANGED, (WPARAM) hwndpage, (LPARAM) 0);
		}
		HWND GetCurrentPageHWnd()
		{
			return (HWND) SendMessage(PSM_GETCURRENTPAGEHWND, (WPARAM) 0, (LPARAM) 0);
		}
		HWND GetTabControl()
		{
			return (HWND) SendMessage(PSM_GETTABCONTROL, (WPARAM) 0, (LPARAM) 0);
		}
		BOOL IsDialogMessage(MSG *pmsg)
		{
			return (BOOL) SendMessage(PSM_ISDIALOGMESSAGE, (WPARAM) 0, (LPARAM) pmsg);
		}
		LRESULT PressButton(int ibutton)
		{
			return (LRESULT) SendMessage(PSM_PRESSBUTTON, (WPARAM) ibutton, (LPARAM) 0);
		}
		LRESULT QuerySiblings(WPARAM param1, LPARAM param2)
		{
			return (LRESULT) SendMessage(PSM_QUERYSIBLINGS, (WPARAM) param1, (LPARAM) param2);
		}
		LRESULT RebootSystem()
		{
			return (LRESULT) SendMessage(PSM_REBOOTSYSTEM, (WPARAM) 0, (LPARAM) 0);
		}
		LRESULT RemovePage(int index, HPROPSHEETPAGE hpage)
		{
			return (LRESULT) SendMessage(PSM_REMOVEPAGE, (WPARAM) index, (LPARAM) hpage);
		}
		LRESULT RestartWindows()
		{
			return (LRESULT) SendMessage(PSM_RESTARTWINDOWS, (WPARAM) 0, (LPARAM) 0);
		}
		BOOL SetCurSel(int index, HPROPSHEETPAGE hpage)
		{
			return (BOOL) SendMessage(PSM_SETCURSEL, (WPARAM) index, (LPARAM) hpage);
		}
		BOOL SetCurSelId(int id)
		{
			return (BOOL) SendMessage(PSM_SETCURSELID, (WPARAM) 0, (LPARAM) id);
		}
		LRESULT SetFinishText(LPTSTR lpsztext)
		{
			return (LRESULT) SendMessage(PSM_SETFINISHTEXT, (WPARAM) 0, (LPARAM) lpsztext);
		}
		LRESULT SetTitle(DWORD style, LPCTSTR text)
		{
			return (LRESULT) SendMessage(PSM_SETTITLE, (WPARAM) style, (LPARAM) text);
		}
		LRESULT SetWizButtons(DWORD flags)
		{
			return (LRESULT) SendMessage(PSM_SETWIZBUTTONS, (WPARAM) 0, (LPARAM) flags);
		}
		LRESULT Unchanged(HWND hwndpage)
		{
			return (LRESULT) SendMessage(PSM_UNCHANGED, (WPARAM) 0, (LPARAM) hwndpage);
		}
	};

	//
	// ToolbarWnd
	//

	class WNDLIB_EXPORT ToolbarWnd : public Wnd
	{
	public:

		enum
		{
			STANDARD_STYLE = WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
							 TBSTYLE_FLAT | TBSTYLE_TOOLTIPS |
							 CCS_NORESIZE | CCS_NOPARENTALIGN | CCS_NODIVIDER
		};

		ToolbarWnd();
		virtual ~ToolbarWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// Helpers
		//

		BOOL Create(HWND parent, BOOL inRebar, BOOL listStyle = TRUE);

		// Add a single button to the toolbar.
		BOOL AddButton(int commandID, int bitmapIndex, LPCTSTR text = NULL,
			BOOL enabled = TRUE);

		// Add a separator to the toolbar.
		BOOL AddSeparator();

		//
		// API wrappers
		//

		#if _WIN32_IE >= 0x400
			int GetButtonInfo(int commandid, LPTBBUTTONINFO buttonInfo)
			{
				return (int) SendMessage(TB_GETBUTTONINFO, (WPARAM) commandid, (LPARAM) buttonInfo);
			}
			BOOL SetButtonInfo(int id, LPTBBUTTONINFO info)
			{
				return (BOOL) SendMessage(TB_SETBUTTONINFO, (WPARAM)id, (LPARAM)info);
			}
			DWORD GetExtendedStyle()
			{
				return (DWORD) SendMessage(TB_GETEXTENDEDSTYLE, 0, 0);
			}
			int GetHotItem()
			{
				return (int) SendMessage(TB_GETHOTITEM, 0, 0);
			}
			BOOL GetMaxSize(LPSIZE sizeOut)
			{
				return (BOOL) SendMessage(TB_GETMAXSIZE, 0, (LPARAM) sizeOut);
			}
			void GetPadding(LPSIZE sizeOut)
			{
				DWORD d = (DWORD) SendMessage(TB_GETPADDING, 0, 0);
				sizeOut->cx = (INT) LOWORD(d);
				sizeOut->cy = (INT) HIWORD(d);
			}
			BOOL GetUnicodeFormat()
			{
				return (BOOL) SendMessage(TB_GETUNICODEFORMAT, 0, 0);
			}
			int HitTest(const POINT *pointIn)
			{
				return (int) SendMessage(TB_HITTEST, 0, (LPARAM)pointIn);
			}
			BOOL IsButtonHighlighted(int commandid)
			{
				return (BOOL) SendMessage(TB_ISBUTTONHIGHLIGHTED, (WPARAM)commandid, 0);
			}
			BOOL MapAccelerator(TCHAR accel, LPUINT idOut)
			{
				return (BOOL) SendMessage(TB_MAPACCELERATOR, (WPARAM)accel, (LPARAM)idOut);
			}
			BOOL MarkButton(int commandid, BOOL highlight)
			{
				return (BOOL) SendMessage(TB_MARKBUTTON, (WPARAM)commandid, (LPARAM)highlight);
			}
			BOOL MoveButton(int oldpos, int newpos)
			{
				return (BOOL) SendMessage(TB_MOVEBUTTON, (WPARAM)oldpos, (LPARAM)newpos);
			}
			DWORD SetDrawTextFlags(DWORD mask, DWORD flags)
			{
				return (DWORD) SendMessage(TB_SETDRAWTEXTFLAGS, (WPARAM)mask, (LPARAM)flags);
			}
			DWORD SetExtendedStyle(DWORD newstyle)
			{
				return (DWORD) SendMessage(TB_SETEXTENDEDSTYLE, 0, (LPARAM)newstyle);
			}
			int SetHotItem(int buttonindex)
			{
				return (int) SendMessage(TB_SETHOTITEM, (WPARAM)buttonindex, 0);
			}
			DWORD SetPadding(int width, int height)
			{
				return (DWORD) SendMessage(TB_SETPADDING, 0, MAKELPARAM(width, height));
			}
			BOOL SetUnicodeFormat(BOOL unicode)
			{
				return (BOOL) SendMessage(TB_SETUNICODEFORMAT, (WPARAM)unicode, 0);
			}
			BOOL SetAnchorHighlight(BOOL anchor)
			{
				return (BOOL) SendMessage(TB_SETANCHORHIGHLIGHT, anchor, 0);
			}
			BOOL GetAnchorHighlight()
			{
				return (BOOL) SendMessage(TB_GETANCHORHIGHLIGHT, 0, 0);
			}
		#endif

		int AddBitmap(int number, LPTBADDBITMAP bitmaps)
		{
			return (int) SendMessage(TB_ADDBITMAP, (WPARAM)number, (LPARAM)bitmaps);
		}
		BOOL AddButtons(int number, LPTBBUTTON buttons)
		{
			return (BOOL) SendMessage(TB_ADDBUTTONS, (WPARAM)number, (LPARAM)buttons);
		}
		int AddString(HINSTANCE instance, UINT stringid)
		{
			return (int) SendMessage(TB_ADDSTRING, (WPARAM) instance, (LPARAM) stringid);
		}
		int AddString(LPCTSTR string)
		{
			return (int) SendMessage(TB_ADDSTRING, 0, (LPARAM) string);
		}
		void AutoSize()
		{
			SendMessage(TB_AUTOSIZE, 0, 0);
		}
		int GetButtonCount()
		{
			return (int) SendMessage(TB_BUTTONCOUNT, 0, 0);
		}
		void SetButtonStructSize(int size)
		{
			SendMessage(TB_BUTTONSTRUCTSIZE, (WPARAM)size, 0);
		}
		BOOL ChangeBitmap(int commandid, int bitmapid)
		{
			return (BOOL) SendMessage(TB_CHANGEBITMAP, (WPARAM) commandid, (LPARAM) bitmapid);
		}
		BOOL CheckButton(int commandid, BOOL check)
		{
			return (BOOL) SendMessage(TB_CHECKBUTTON, (WPARAM)commandid, (LPARAM)check);
		}
		int CommandToIndex(int commandid)
		{
			return (int) SendMessage(TB_COMMANDTOINDEX, (WPARAM)commandid, 0);
		}
		void Customize()
		{
			SendMessage(TB_CUSTOMIZE, 0, 0);
		}
		BOOL DeleteButton(int buttonindex)
		{
			return (BOOL) SendMessage(TB_DELETEBUTTON, (WPARAM)buttonindex, 0);
		}
		BOOL EnableButton(int commandid, BOOL enable)
		{
			return (BOOL) SendMessage(TB_ENABLEBUTTON, (WPARAM)commandid, (LPARAM)enable);
		}
		int GetBitmap(int commandid)
		{
			return (int) SendMessage(TB_GETBITMAP, (WPARAM) commandid, 0);
		}
		BOOL GetButton(int buttonindex, LPTBBUTTON button)
		{
			return (BOOL) SendMessage(TB_GETBUTTON, (WPARAM) buttonindex, (LPARAM) button);
		}
		void GetButtonSize(SIZE *size)
		{
			DWORD d = (DWORD) SendMessage(TB_GETBUTTONSIZE, 0, 0);
			size->cx = (INT)LOWORD(d);
			size->cy = (INT)HIWORD(d);
		}
		int GetButtonText(int commandid, LPTSTR textOut)
		{
			return (int) SendMessage(TB_GETBUTTONTEXT, (WPARAM) commandid, (LPARAM) textOut);
		}
		HIMAGELIST GetDisableImageList()
		{
			return (HIMAGELIST) SendMessage(TB_GETDISABLEDIMAGELIST, 0, 0);
		}
		HIMAGELIST GetHotImageList()
		{
			return (HIMAGELIST) SendMessage(TB_GETHOTIMAGELIST, 0, 0);
		}
		HIMAGELIST GetImageList()
		{
			return (HIMAGELIST) SendMessage(TB_GETIMAGELIST, 0, 0);
		}
		BOOL GetItemRect(int buttonindex, LPRECT rectOut)
		{
			return (BOOL) SendMessage(TB_GETITEMRECT, (WPARAM) buttonindex, (LPARAM) rectOut);
		}
		BOOL GetRect(int commandid, LPRECT rectOut)
		{
			return (BOOL) SendMessage(TB_GETRECT, (WPARAM) commandid, (LPARAM) rectOut);
		}
		int GetRows()
		{
			return (int) SendMessage(TB_GETROWS, 0, 0);
		}
		DWORD GetState(int commandid)
		{
			return (DWORD) SendMessage(TB_GETSTATE, (WPARAM)commandid, 0);
		}
		#if _WIN32_IE >= 0x500
			int GetString(int stringindex, LPTSTR buffer, int maxChars)
			{
				return (int) SendMessage(TB_GETSTRING, MAKELONG(maxChars, stringindex), (LPARAM) buffer);
			}
		#endif
		DWORD GetStyle()
		{
			return (DWORD) SendMessage(TB_GETSTYLE, 0, 0);
		}
		int GetTextRows()
		{
			return (int) SendMessage(TB_GETTEXTROWS, 0, 0);
		}
		HWND GetToolTips()
		{
			return (HWND) SendMessage(TB_GETTOOLTIPS, 0, 0);
		}
		BOOL HideButton(int commandid, BOOL hide)
		{
			return (BOOL) SendMessage(TB_HIDEBUTTON, (WPARAM) commandid, (LPARAM) hide);
		}
		BOOL Indeterminate(int commandid, BOOL indeterminate)
		{
			return (BOOL) SendMessage(TB_INDETERMINATE, (WPARAM) commandid, (LPARAM) indeterminate);
		}
		BOOL InsertButton(int buttonindex, LPTBBUTTON button)
		{
			return (BOOL) SendMessage(TB_INSERTBUTTON, (WPARAM)buttonindex, (LPARAM)button);
		}
		BOOL IsButtonChecked(int commandid)
		{
			return (BOOL) SendMessage(TB_ISBUTTONCHECKED, (WPARAM)commandid, 0);
		}
		BOOL IsButtonEnabled(int commandid)
		{
			return (BOOL) SendMessage(TB_ISBUTTONENABLED, (WPARAM)commandid, 0);
		}
		BOOL IsButtonHidden(int commandid)
		{
			return (BOOL) SendMessage(TB_ISBUTTONHIDDEN, (WPARAM)commandid, 0);
		}
		BOOL IsButtonIndeterminate(int commandid)
		{
			return (BOOL) SendMessage(TB_ISBUTTONINDETERMINATE, (WPARAM)commandid, 0);
		}
		BOOL IsButtonPressed(int commandid)
		{
			return (BOOL) SendMessage(TB_ISBUTTONPRESSED, (WPARAM)commandid, 0);
		}
		INT LoadImages(int bitmapid, HINSTANCE hinst = HINST_COMMCTRL)
		{
			return (INT) SendMessage(TB_LOADIMAGES, (WPARAM) bitmapid, (LPARAM) hinst);
		}
		BOOL PressButton(int commandid, BOOL press)
		{
			return (BOOL) SendMessage(TB_PRESSBUTTON, (WPARAM)commandid, (LPARAM)press);
		}
		BOOL ReplaceBitmap(LPTBREPLACEBITMAP replace)
		{
			return (BOOL) SendMessage(TB_REPLACEBITMAP, 0, (LPARAM) replace);
		}
		void SaveRestore(BOOL saving, TBSAVEPARAMS *params)
		{
			SendMessage(TB_SAVERESTORE, (WPARAM)saving, (LPARAM)params);
		}
		BOOL SetBitmapSize(UINT x, UINT y)
		{
			return (BOOL) SendMessage(TB_SETBITMAPSIZE, 0, (LPARAM) MAKELONG(x, y));
		}
		BOOL SetButtonSize(UINT x, UINT y)
		{
			return (BOOL) SendMessage(TB_SETBUTTONSIZE, 0, (LPARAM) MAKELONG(x, y));
		}
		BOOL SetButtonWidth(UINT min, UINT max)
		{
			return (BOOL) SendMessage(TB_SETBUTTONWIDTH, 0, (LPARAM) MAKELONG(min, max));
		}
		BOOL SetCmdId(int index, int commandid)
		{
			return (BOOL) SendMessage(TB_SETCMDID, (WPARAM)index, (LPARAM)commandid);
		}
		HIMAGELIST SetDisabledImageList(HIMAGELIST newimagelist)
		{
			return (HIMAGELIST) SendMessage(TB_SETDISABLEDIMAGELIST, 0, (LPARAM) newimagelist);
		}
		HIMAGELIST SetHotImageList(HIMAGELIST newimagelist)
		{
			return (HIMAGELIST) SendMessage(TB_SETHOTIMAGELIST, 0, (LPARAM) newimagelist);
		}
		HIMAGELIST SetImageList(HIMAGELIST newimagelist)
		{
			return (HIMAGELIST) SendMessage(TB_SETIMAGELIST, 0, (LPARAM) newimagelist);
		}
		BOOL SetIndent(int indent)
		{
			return (BOOL) SendMessage(TB_SETINDENT, (WPARAM)indent, 0);
		}
		BOOL SetMaxTextRows(int maxRows)
		{
			return (BOOL) SendMessage(TB_SETMAXTEXTROWS, (WPARAM)maxRows, 0);
		}
		HWND TBSetParent(HWND parent)
		{
			return (HWND) SendMessage(TB_SETPARENT, (WPARAM) parent, 0);
		}
		void SetRows(int numrows, BOOL larger, LPRECT rectOut)
		{
			SendMessage(TB_SETROWS, MAKEWPARAM(numrows, larger), (LPARAM) rectOut);
		}
		BOOL SetState(int commandid, DWORD state)
		{
			return (BOOL) SendMessage(TB_SETSTATE, (WPARAM)commandid, (LPARAM)state);
		}
		void SetStyle(DWORD style)
		{
			SendMessage(TB_SETSTYLE, 0, (LPARAM) style);
		}
		void SetToolTips(HWND tooltip)
		{
			SendMessage(TB_SETTOOLTIPS, (WPARAM)tooltip, 0);
		}

	};

	//
	// RebarWnd
	//

	#if _WIN32_IE >= 0x400

	enum RebarInsertBarConstants
	{
		REBAR_INSERTBAR_SPLIT = 1,
		REBAR_INSERTBAR_HIDDEN = 2,
		REBAR_INSERTBAR_GRIPPER = 4, // Force a gripper always
		REBAR_INSERTBAR_NO_DYNAMIC_SIZE = 8
	};

	class WNDLIB_EXPORT RebarWnd : public Wnd
	{
	public:

		RebarWnd();
		virtual ~RebarWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// Helpers
		//

		BOOL Create(HWND parent);

		// Call SetBarInfo with the default (empty) REBARINFO.
		BOOL SetBarInfo();

		// Returns -1 on failure, or the ID of the new bar on success
		int InsertBar(ToolbarWnd *bar, LPCTSTR title, UINT flags);

		// Maximize the last bar on each row
		void Rearrange();

		//
		// API wrappers
		//

		int HitTest(LPRBHITTESTINFO hittest)
		{
			return (int) SendMessage(RB_HITTEST, 0, (LPARAM) hittest);
		}
		BOOL DeleteBand(UINT band)
		{
			return (BOOL) SendMessage(RB_DELETEBAND, (WPARAM) band, 0);
		}
		void GetBandBorders(UINT band, LPRECT rect)
		{
			SendMessage(RB_GETBANDBORDERS, (WPARAM) band, (LPARAM) rect);
		}
		UINT GetBandCount()
		{
			return (UINT) SendMessage(RB_GETBANDCOUNT, 0, 0);
		}
		BOOL GetBandInfo(UINT band, LPREBARBANDINFO infoOut)
		{
			return (BOOL) SendMessage(RB_GETBANDINFO, (WPARAM) band, (LPARAM) infoOut);
		}
		UINT GetBarHeight()
		{
			return (UINT) SendMessage(RB_GETBARHEIGHT, 0, 0);
		}
		BOOL GetBarInfo(LPREBARINFO infoOut)
		{
			return (BOOL) SendMessage(RB_GETBARINFO, 0, (LPARAM) infoOut);
		}
		COLORREF GetBkColor()
		{
			return (COLORREF) SendMessage(RB_GETBKCOLOR, 0, 0);
		}
		BOOL GetBandRect(UINT band, LPRECT rectOut)
		{
			return (BOOL) SendMessage(RB_GETRECT, (WPARAM) band, (LPARAM) rectOut);
		}
		UINT GetRowCount()
		{
			return (UINT) SendMessage(RB_GETROWCOUNT, 0, 0);
		}
		UINT GetRowHeight(UINT band)
		{
			return (UINT) SendMessage(RB_GETROWHEIGHT, (WPARAM) band, 0);
		}
		HWND GetToolTips()
		{
			return (HWND) SendMessage(RB_GETTOOLTIPS, 0, 0);
		}
		BOOL GetUnicodeFormat()
		{
			return (BOOL) SendMessage(RB_GETUNICODEFORMAT, 0, 0);
		}
		int IdToIndex(UINT bandid)
		{
			return (int) SendMessage(RB_IDTOINDEX, (WPARAM) bandid, 0);
		}
		BOOL InsertBand(int index, LPREBARBANDINFO infoIn)
		{
			return (BOOL) SendMessage(RB_INSERTBAND, (WPARAM) (UINT) index, (LPARAM) infoIn);
		}
		void MaximizeBand(UINT band, BOOL ideal)
		{
			SendMessage(RB_MAXIMIZEBAND, (WPARAM) band, (LPARAM) ideal);
		}
		void MinimizeBand(UINT band)
		{
			SendMessage(RB_MINIMIZEBAND, (WPARAM) band, 0);
		}
		BOOL MoveBand(UINT fromIndex, UINT toIndex)
		{
			return (BOOL) SendMessage(RB_MOVEBAND, (WPARAM) fromIndex, (LPARAM) toIndex);
		}
		#if _WIN32_IE >= 0x500
			#ifdef RB_PUSHCHEVRON
				void PushChevron(UINT band, LPARAM appvalue)
				{
					SendMessage(RB_PUSHCHEVRON, (WPARAM) band, appvalue);
				}
			#endif
		#endif
		BOOL SetBandInfo(UINT band, LPREBARBANDINFO infoin)
		{
			return (BOOL) SendMessage(RB_SETBANDINFO, (WPARAM) band, (LPARAM) infoin);
		}
		BOOL SetBarInfo(LPREBARINFO infoin)
		{
			return (BOOL) SendMessage(RB_SETBARINFO, 0, (LPARAM) infoin);
		}
		HWND RBSetParent(HWND parent)
		{
			return (HWND) SendMessage(RB_SETPARENT, (WPARAM) parent, 0);
		}
		void SetToolTips(HWND tooltipwnd)
		{
			SendMessage(RB_SETTOOLTIPS, (WPARAM) tooltipwnd, 0);
		}
		BOOL SetUnicodeFormat(BOOL unicode)
		{
			return (BOOL) SendMessage(RB_SETUNICODEFORMAT, (WPARAM) unicode, 0);
		}
		BOOL ShowBand(UINT band, BOOL show)
		{
			return (BOOL) SendMessage(RB_SHOWBAND, (WPARAM) band, (LPARAM) show);
		}
		BOOL SizeToRect(LPRECT rect)
		{
			return (BOOL) SendMessage(RB_SIZETORECT, 0, (LPARAM) rect); }

		COLORREF SetBkColor(COLORREF clr)
		{
			return (COLORREF) SendMessage(RB_SETBKCOLOR, 0, (LPARAM) clr);
		}

	protected:

		unsigned _nextBandId;
	};

	#endif // _WIN32_IE >= 0x400

	//
	// StatusBarWnd
	//

	class WNDLIB_EXPORT StatusBarWnd : public Wnd
	{
	public:
		StatusBarWnd();
		virtual ~StatusBarWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		BOOL GetBorders(int borders[3])
		{
			return (BOOL) SendMessage(SB_GETBORDERS, 0, (LPARAM) borders);
		}
		int GetParts(int nparts, INT *rightcoords)
		{
			return (int) SendMessage(SB_GETPARTS, (WPARAM) nparts, (LPARAM) rightcoords);
		}
		BOOL GetRect(int part, RECT *rectOut)
		{
			return (BOOL) SendMessage(SB_GETRECT, (WPARAM) part, (LPARAM) rectOut);
		}
		DWORD GetText(int part, LPTSTR buffer)
		{
			return (DWORD) SendMessage(SB_GETTEXT, (WPARAM) part, (LPARAM) buffer);
		}
		DWORD GetTextLength(int part)
		{
			return (DWORD) SendMessage(SB_GETTEXTLENGTH, (WPARAM) part, 0);
		}
		BOOL IsSimple()
		{
			return (BOOL) SendMessage(SB_ISSIMPLE, 0, 0);
		}
		void SetMinHeight(int pixels)
		{
			SendMessage(SB_SETMINHEIGHT, (WPARAM) pixels, 0);
		}
		BOOL SetParts(int count, const INT *widths)
		{
			return (BOOL) SendMessage(SB_SETPARTS, (WPARAM) count, (LPARAM) widths);
		}
		BOOL SetText(int part, UINT type, LPCTSTR text)
		{
			return (BOOL) SendMessage(SB_SETTEXT, (WPARAM) (((UINT)part) | type), (LPARAM) text);
		}
		void Simple(BOOL simple)
		{
			SendMessage(SB_SIMPLE, (WPARAM) simple, 0); }

		#if _WIN32_IE >= 0x400
			HICON GetIcon(int part)
			{
				return (HICON) SendMessage(SB_GETICON, (WPARAM) part, 0);
			}
			void GetTipText(int part, LPTSTR *buffer, int bufferlength)
			{
				SendMessage(SB_GETTIPTEXT, MAKEWPARAM(part, bufferlength), (LPARAM) buffer);
			}
			BOOL GetUnicodeFormat()
			{
				return (BOOL) SendMessage(SB_GETUNICODEFORMAT, 0, 0);
			}
			COLORREF SetBkColor(COLORREF color)
			{
				return (COLORREF) SendMessage(SB_SETBKCOLOR, 0, (LPARAM) color);
			}
			BOOL SetIcon(int part, HICON icon)
			{
				return (BOOL) SendMessage(SB_SETICON, (WPARAM) part, (LPARAM) icon);
			}
			void SetTipText(int part, LPCTSTR tiptext)
			{
				SendMessage(SB_SETTIPTEXT, (WPARAM) part, (LPARAM) tiptext);
			}
			BOOL SetUnicodeFormat(BOOL unicode)
			{
				return (BOOL) SendMessage(SB_SETUNICODEFORMAT, (WPARAM) unicode, 0);
			}
		#endif

		//
		// Extensions
		//

		// Process WM_MENUSELECT for the parent window
		void OnMenuSelect(WPARAM wparam, LPARAM lparam);

	protected:

		bool _menuSelecting;
		TCHAR *_prevText;
	};

	//
	// ToolTipWnd
	//

	class WNDLIB_EXPORT ToolTipWnd : public Wnd
	{
	public:

		ToolTipWnd();

		virtual ~ToolTipWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		#if _WIN32_IE >= 0x500
			BOOL AdjustRect(BOOL larger, LPRECT rc)
			{
				return (BOOL) SendMessage(TTM_ADJUSTRECT, (WPARAM) larger, (LPARAM) rc);
			}
			DWORD GetBubbleSize(LPTOOLINFO ti)
			{
				return (DWORD) SendMessage(TTM_GETBUBBLESIZE, (WPARAM) 0, (LPARAM) ti);
			}
			#ifdef TTM_SETTITLE
				LRESULT SetTitle(int icon, LPCTSTR title)
				{
					return (LRESULT) SendMessage(TTM_SETTITLE, (WPARAM) icon, (LPARAM) title);
				}
			#endif
		#endif

		#if _WIN32_IE >= 0x400
			LRESULT Update()
			{
				return (LRESULT) SendMessage(TTM_UPDATE, (WPARAM) 0, (LPARAM) 0);
			}
		#endif

		LRESULT Activate(BOOL activate)
		{
			return (LRESULT) SendMessage(TTM_ACTIVATE, (WPARAM) activate, (LPARAM) 0);
		}
		BOOL AddTool(LPTOOLINFO ti)
		{
			return (BOOL) SendMessage(TTM_ADDTOOL, (WPARAM) 0, (LPARAM) ti);
		}
		LRESULT DelTool(LPTOOLINFO ti)
		{
			return (LRESULT) SendMessage(TTM_DELTOOL, (WPARAM) 0, (LPARAM) ti);
		}
		BOOL EnumTools(int itool, LPTOOLINFO lpti)
		{
			return (BOOL) SendMessage(TTM_ENUMTOOLS, (WPARAM) itool, (LPARAM) lpti);
		}
		BOOL GetCurrentTool(LPTOOLINFO ti)
		{
			return (BOOL) SendMessage(TTM_GETCURRENTTOOL, (WPARAM) 0, (LPARAM) ti);
		}
		int GetDelayTime(DWORD which)
		{
			return (int) SendMessage(TTM_GETDELAYTIME, (WPARAM) which, (LPARAM) 0);
		}
		LRESULT GetMargin(LPRECT rc)
		{
			return (LRESULT) SendMessage(TTM_GETMARGIN, (WPARAM) 0, (LPARAM) rc);
		}
		int GetMaxTipWidth()
		{
			return (int) SendMessage(TTM_GETMAXTIPWIDTH, (WPARAM) 0, (LPARAM) 0);
		}
		LRESULT GetText(LPTOOLINFO ti)
		{
			return (LRESULT) SendMessage(TTM_GETTEXT, (WPARAM) 0, (LPARAM) ti);
		}
		COLORREF GetTipBkColor()
		{
			return (COLORREF) SendMessage(TTM_GETTIPBKCOLOR, (WPARAM) 0, (LPARAM) 0);
		}
		COLORREF GetTipTextColor()
		{
			return (COLORREF) SendMessage(TTM_GETTIPTEXTCOLOR, (WPARAM) 0, (LPARAM) 0);
		}
		int GetToolCount()
		{
			return (int) SendMessage(TTM_GETTOOLCOUNT, (WPARAM) 0, (LPARAM) 0);
		}
		BOOL GetToolInfo(LPTOOLINFO ti)
		{
			return (BOOL) SendMessage(TTM_GETTOOLINFO, (WPARAM) 0, (LPARAM) ti);
		}
		BOOL HitTest(LPHITTESTINFO hti)
		{
			return (BOOL) SendMessage(TTM_HITTEST, (WPARAM) 0, (LPARAM) hti);
		}
		LRESULT NewToolRect(LPTOOLINFO ti)
		{
			return (LRESULT) SendMessage(TTM_NEWTOOLRECT, (WPARAM) 0, (LPARAM) ti);
		}
		LRESULT Pop()
		{
			return (LRESULT) SendMessage(TTM_POP, (WPARAM) 0, (LPARAM) 0);
		}
		LRESULT RelayEvent(LPMSG msg)
		{
			return (LRESULT) SendMessage(TTM_RELAYEVENT, (WPARAM) 0, (LPARAM) msg);
		}
		LRESULT SetDelayTime(DWORD duration, int itime)
		{
			return (LRESULT) SendMessage(TTM_SETDELAYTIME, (WPARAM) duration, (LPARAM) MAKELONG(itime, 0));
		}
		LRESULT SetMargin(LPRECT rc)
		{
			return (LRESULT) SendMessage(TTM_SETMARGIN, (WPARAM) 0, (LPARAM) rc);
		}
		int SetMaxTipWidth(int width)
		{
			return (int) SendMessage(TTM_SETMAXTIPWIDTH, (WPARAM) 0, (LPARAM) width);
		}
		LRESULT SetTipBkColor(COLORREF clr)
		{
			return (LRESULT) SendMessage(TTM_SETTIPBKCOLOR, (WPARAM) clr, (LPARAM) 0);
		}
		LRESULT SetTipTextColor(COLORREF clr)
		{
			return (LRESULT) SendMessage(TTM_SETTIPTEXTCOLOR, (WPARAM) clr, (LPARAM) 0);
		}
		LRESULT SetToolInfo(LPTOOLINFO ti)
		{
			return (LRESULT) SendMessage(TTM_SETTOOLINFO, (WPARAM) 0, (LPARAM) ti);
		}
		LRESULT TrackActivate(BOOL activate, LPTOOLINFO ti)
		{
			return (LRESULT) SendMessage(TTM_TRACKACTIVATE, (WPARAM) activate, (LPARAM) ti);
		}
		LRESULT TrackPosition(int xpos, int ypos)
		{
			return (LRESULT) SendMessage(TTM_TRACKPOSITION, (WPARAM) 0, (LPARAM) MAKELONG(xpos, ypos));
		}
		LRESULT UpdateTipText(LPTOOLINFO ti)
		{
			return (LRESULT) SendMessage(TTM_UPDATETIPTEXT, (WPARAM) 0, (LPARAM) ti);
		}
		HWND WindowFromPoint(POINT FAR *pt)
		{
			return (HWND) SendMessage(TTM_WINDOWFROMPOINT, (WPARAM) 0, (LPARAM) pt);
		}
	};

	//
	// TrackBarWnd
	//

	class WNDLIB_EXPORT TrackBarWnd : public Wnd
	{
	public:
		TrackBarWnd();
		virtual ~TrackBarWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		#if _WIN32_IE >= 0x400
			BOOL GetUnicodeFormat()
			{
				return (BOOL) SendMessage(TBM_GETUNICODEFORMAT, (WPARAM) 0, (LPARAM) 0);
			}
			BOOL SetUnicodeFormat(BOOL unicode)
			{
				return (BOOL) SendMessage(TBM_SETUNICODEFORMAT, (WPARAM) unicode, (LPARAM) 0);
			}
		#endif

		LRESULT ClearSel(BOOL redraw)
		{
			return (LRESULT) SendMessage(TBM_CLEARSEL, (WPARAM) redraw, (LPARAM) 0);
		}
		LRESULT ClearTics(BOOL redraw)
		{
			return (LRESULT) SendMessage(TBM_CLEARTICS, (WPARAM) redraw, (LPARAM) 0);
		}
		HWND GetBuddy(BOOL location)
		{
			return (HWND) SendMessage(TBM_GETBUDDY, (WPARAM) location, (LPARAM) 0);
		}
		LRESULT GetChannelRect(LPRECT rc)
		{
			return (LRESULT) SendMessage(TBM_GETCHANNELRECT, (WPARAM) 0, (LPARAM) rc);
		}
		DWORD GetLineSize()
		{
			return (DWORD) SendMessage(TBM_GETLINESIZE, (WPARAM) 0, (LPARAM) 0);
		}
		int GetNumTics()
		{
			return (int) SendMessage(TBM_GETNUMTICS, (WPARAM) 0, (LPARAM) 0);
		}
		int GetPageSize()
		{
			return (int) SendMessage(TBM_GETPAGESIZE, (WPARAM) 0, (LPARAM) 0);
		}
		int GetPos()
		{
			return (int) SendMessage(TBM_GETPOS, (WPARAM) 0, (LPARAM) 0);
		}
		LPDWORD GetPTics()
		{
			return (LPDWORD) SendMessage(TBM_GETPTICS, (WPARAM) 0, (LPARAM) 0);
		}
		int GetRangeMax()
		{
			return (int) SendMessage(TBM_GETRANGEMAX, (WPARAM) 0, (LPARAM) 0);
		}
		int GetRangeMin()
		{
			return (int) SendMessage(TBM_GETRANGEMIN, (WPARAM) 0, (LPARAM) 0);
		}
		int GetSelEnd()
		{
			return (int) SendMessage(TBM_GETSELEND, (WPARAM) 0, (LPARAM) 0);
		}
		int GetSelStart()
		{
			return (int) SendMessage(TBM_GETSELSTART, (WPARAM) 0, (LPARAM) 0);
		}
		int GetThumbLength()
		{
			return (int) SendMessage(TBM_GETTHUMBLENGTH, (WPARAM) 0, (LPARAM) 0);
		}
		LRESULT GetThumbRect(LPRECT rc)
		{
			return (LRESULT) SendMessage(TBM_GETTHUMBRECT, (WPARAM) 0, (LPARAM) rc);
		}
		int GetTic(WORD tic)
		{
			return (int) SendMessage(TBM_GETTIC, (WPARAM) tic, (LPARAM) 0);
		}
		int GetTicPos(WORD tic)
		{
			return (int) SendMessage(TBM_GETTICPOS, (WPARAM) tic, (LPARAM) 0);
		}
		HWND GetToolTips()
		{
			return (HWND) SendMessage(TBM_GETTOOLTIPS, (WPARAM) 0, (LPARAM) 0);
		}
		HWND SetBuddy(BOOL location, HWND buddy)
		{
			return (HWND) SendMessage(TBM_SETBUDDY, (WPARAM) location, (LPARAM) buddy);
		}
		int SetLineSize(int linesize)
		{
			return (int) SendMessage(TBM_SETLINESIZE, (WPARAM) 0, (LPARAM) linesize);
		}
		int SetPageSize(int pagesize)
		{
			return (int) SendMessage(TBM_SETPAGESIZE, (WPARAM) 0, (LPARAM) pagesize);
		}
		LRESULT SetPos(BOOL fpos, int lpos)
		{
			return (LRESULT) SendMessage(TBM_SETPOS, (WPARAM) fpos, (LPARAM) lpos);
		}
		LRESULT SetRange(BOOL redraw, int minim, int maxim)
		{
			return (LRESULT) SendMessage(TBM_SETRANGE, (WPARAM) redraw, (LPARAM) MAKELONG(minim, maxim));
		}
		LRESULT SetRangeMax(BOOL redraw, int maximum)
		{
			return (LRESULT) SendMessage(TBM_SETRANGEMAX, (WPARAM) redraw, (LPARAM) maximum);
		}
		LRESULT SetRangeMin(BOOL redraw, int minimum)
		{
			return (LRESULT) SendMessage(TBM_SETRANGEMIN, (WPARAM) redraw, (LPARAM) minimum);
		}
		LRESULT SetSel(BOOL redraw, int minim, int maxim)
		{
			return (LRESULT) SendMessage(TBM_SETSEL, (WPARAM) redraw, (LPARAM) MAKELONG(minim, maxim));
		}
		LRESULT SetSelEnd(BOOL redraw, int end)
		{
			return (LRESULT) SendMessage(TBM_SETSELEND, (WPARAM) redraw, (LPARAM) end);
		}
		LRESULT SetSelStart(BOOL redraw, int start)
		{
			return (LRESULT) SendMessage(TBM_SETSELSTART, (WPARAM) redraw, (LPARAM) start);
		}
		LRESULT SetThumbLength(int length)
		{
			return (LRESULT) SendMessage(TBM_SETTHUMBLENGTH, (WPARAM) length, (LPARAM) 0);
		}
		BOOL SetTic(int position)
		{
			return (BOOL) SendMessage(TBM_SETTIC, (WPARAM) 0, (LPARAM) position);
		}
		LRESULT SetTicFreq(int freq)
		{
			return (LRESULT) SendMessage(TBM_SETTICFREQ, (WPARAM) freq, (LPARAM) 0);
		}
		int SetTipSide(int location)
		{
			return (int) SendMessage(TBM_SETTIPSIDE, (WPARAM) location, (LPARAM) 0);
		}
		LRESULT SetToolTips(HWND hwnd)
		{
			return (LRESULT) SendMessage(TBM_SETTOOLTIPS, (WPARAM) hwnd, (LPARAM) 0);
		}
	};

	//
	// TreeViewWnd
	//

	class WNDLIB_EXPORT TreeViewWnd : public Wnd
	{
	public:
		TreeViewWnd();
		virtual ~TreeViewWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// Helpers
		//

		// Get the single selected item in the tree, return NULL otherwise
		HTREEITEM GetSingleSelection();

		// Expand all
		void ExpandAll();

		// Expand the specified tree item and its siblings
		void RecursivelyExpand(HTREEITEM expand);

		//
		// API wrappers
		//

		#if _WIN32_IE >= 0x400
			BOOL GetItem(LPTVITEMEX pitem)
			{
				return (BOOL) SendMessage(TVM_GETITEM, (WPARAM) 0, (LPARAM) pitem);
			}
			BOOL SetItem(const LPTVITEMEX pitem)
			{
				return (BOOL) SendMessage(TVM_SETITEM, (WPARAM) 0, (LPARAM) pitem);
			}
			COLORREF GetBkColor()
			{
				return (COLORREF) SendMessage(TVM_GETBKCOLOR, (WPARAM) 0, (LPARAM) 0);
			}
			COLORREF GetInsertMarkColor()
			{
				return (COLORREF) SendMessage(TVM_GETINSERTMARKCOLOR, (WPARAM) 0, (LPARAM) 0);
			}
			int GetItemHeight()
			{
				return (int) SendMessage(TVM_GETITEMHEIGHT, (WPARAM) 0, (LPARAM) 0);
			}
			DWORD GetScrollTime()
			{
				return (DWORD) SendMessage(TVM_GETSCROLLTIME, (WPARAM) 0, (LPARAM) 0);
			}
			COLORREF GetTextColor()
			{
				return (COLORREF) SendMessage(TVM_GETTEXTCOLOR, (WPARAM) 0, (LPARAM) 0);
			}
			BOOL GetUnicodeFormat()
			{
				return (BOOL) SendMessage(TVM_GETUNICODEFORMAT, (WPARAM) 0, (LPARAM) 0);
			}
			COLORREF SetBkColor(COLORREF clrbk)
			{
				return (COLORREF) SendMessage(TVM_SETBKCOLOR, (WPARAM) 0, (LPARAM) clrbk);
			}
			BOOL SetInsertMark(BOOL fafter, HTREEITEM htiinsert)
			{
				return (BOOL) SendMessage(TVM_SETINSERTMARK, (WPARAM) fafter, (LPARAM) htiinsert);
			}
			COLORREF SetInsertMarkColor(COLORREF clr)
			{
				return (COLORREF) SendMessage(TVM_SETINSERTMARKCOLOR, (WPARAM) 0, (LPARAM) clr);
			}
			int SetItemHeight(int cyitem)
			{
				return (int) SendMessage(TVM_SETITEMHEIGHT, (WPARAM) cyitem, (LPARAM) 0);
			}
			DWORD SetScrollTime(DWORD uscrolltime)
			{
				return (DWORD) SendMessage(TVM_SETSCROLLTIME, (WPARAM) uscrolltime, (LPARAM) 0);
			}
			COLORREF SetTextColor(COLORREF clr)
			{
				return (COLORREF) SendMessage(TVM_SETTEXTCOLOR, (WPARAM) 0, (LPARAM) clr);
			}
			BOOL SetUnicodeFormat(BOOL funicode)
			{
				return (BOOL) SendMessage(TVM_SETUNICODEFORMAT, (WPARAM) funicode, (LPARAM) 0);
			}
		#endif

		HIMAGELIST CreateDragImage(HTREEITEM hitem)
		{
			return (HIMAGELIST) SendMessage(TVM_CREATEDRAGIMAGE, (WPARAM) 0, (LPARAM) hitem);
		}
		BOOL DeleteItem(HTREEITEM hitem)
		{
			return (BOOL) SendMessage(TVM_DELETEITEM, (WPARAM) 0, (LPARAM) hitem);
		}
		HWND EditLabel(HTREEITEM hitem)
		{
			return (HWND) SendMessage(TVM_EDITLABEL, (WPARAM) 0, (LPARAM) hitem);
		}
		BOOL EndEditLabelNow(BOOL fcancel)
		{
			return (BOOL) SendMessage(TVM_ENDEDITLABELNOW, (WPARAM) fcancel, (LPARAM) 0);
		}
		BOOL EnsureVisible(HTREEITEM hitem)
		{
			return (BOOL) SendMessage(TVM_ENSUREVISIBLE, (WPARAM) 0, (LPARAM) hitem);
		}
		BOOL Expand(UINT flag, HTREEITEM hitem)
		{
			return (BOOL) SendMessage(TVM_EXPAND, (WPARAM) flag, (LPARAM) hitem);
		}
		int GetCount()
		{
			return (int) SendMessage(TVM_GETCOUNT, (WPARAM) 0, (LPARAM) 0);
		}
		HWND GetEditControl()
		{
			return (HWND) SendMessage(TVM_GETEDITCONTROL, (WPARAM) 0, (LPARAM) 0);
		}
		HIMAGELIST GetImageList(int iimage)
		{
			return (HIMAGELIST) SendMessage(TVM_GETIMAGELIST, (WPARAM) iimage, (LPARAM) 0);
		}
		int GetIndent()
		{
			return (int) SendMessage(TVM_GETINDENT, (WPARAM) 0, (LPARAM) 0);
		}
		int GetISearchString(LPTSTR lpsz)
		{
			return (int) SendMessage(TVM_GETISEARCHSTRING, (WPARAM) 0, (LPARAM) lpsz);
		}
		BOOL GetItemRect(BOOL fitemrect, LPRECT prc)
		{
			return (BOOL) SendMessage(TVM_GETITEMRECT, (WPARAM) fitemrect, (LPARAM) prc);
		}
		HTREEITEM GetNextItem(UINT flag, HTREEITEM hitem)
		{
			return (HTREEITEM) SendMessage(TVM_GETNEXTITEM, (WPARAM) flag, (LPARAM) hitem);
		}
		HWND GetToolTips()
		{
			return (HWND) SendMessage(TVM_GETTOOLTIPS, (WPARAM) 0, (LPARAM) 0);
		}
		int GetVisibleCount()
		{
			return (int) SendMessage(TVM_GETVISIBLECOUNT, (WPARAM) 0, (LPARAM) 0);
		}
		HTREEITEM HitTest(LPTVHITTESTINFO lpht)
		{
			return (HTREEITEM) SendMessage(TVM_HITTEST, (WPARAM) 0, (LPARAM) lpht);
		}
		HTREEITEM InsertItem(LPTVINSERTSTRUCT lpis)
		{
			return (HTREEITEM) SendMessage(TVM_INSERTITEM, (WPARAM) 0, (LPARAM) lpis);
		}
		BOOL SelectItem(UINT flag, HTREEITEM hitem)
		{
			return (BOOL) SendMessage(TVM_SELECTITEM, (WPARAM) flag, (LPARAM) hitem);
		}
		HIMAGELIST SetImageList(int iimage, HIMAGELIST himl)
		{
			return (HIMAGELIST) SendMessage(TVM_SETIMAGELIST, (WPARAM) iimage, (LPARAM) himl);
		}
		LRESULT SetIndent(int indent)
		{
			return (LRESULT) SendMessage(TVM_SETINDENT, (WPARAM) indent, (LPARAM) 0);
		}
		HWND SetToolTips(HWND hwndtooltip)
		{
			return (HWND) SendMessage(TVM_SETTOOLTIPS, (WPARAM) hwndtooltip, (LPARAM) 0);
		}
		BOOL SortChildren(BOOL frecurse, HTREEITEM hitem)
		{
			return (BOOL) SendMessage(TVM_SORTCHILDREN, (WPARAM) frecurse, (LPARAM) hitem);
		}
		BOOL SortChildrenCB(BOOL frecurse, LPTVSORTCB psort)
		{
			return (BOOL) SendMessage(TVM_SORTCHILDRENCB, (WPARAM) frecurse, (LPARAM) psort);
		}

		#if _WIN32_IE >= 0x500
			UINT GetItemState(HTREEITEM hitem, UINT statemask)
			{
				return (UINT) SendMessage(TVM_GETITEMSTATE, (WPARAM) hitem, (LPARAM) statemask);
			}
			COLORREF GetLineColor()
			{
				return (COLORREF) SendMessage(TVM_GETLINECOLOR, (WPARAM) 0, (LPARAM) 0);
			}
			COLORREF SetLineColor(COLORREF clr)
			{
				return (COLORREF) SendMessage(TVM_SETLINECOLOR, (WPARAM) 0, (LPARAM) clr);
			}
		#endif

	protected:

		HTREEITEM RecursivelyFindSelected(HTREEITEM start);
	};

	//
	// UpDownWnd
	//

	class WNDLIB_EXPORT UpDownWnd : public Wnd
	{
	public:
		UpDownWnd();
		virtual ~UpDownWnd();

		// Wnd overrides.
		virtual LPCTSTR GetClassName();

		//
		// API wrappers
		//

		#if _WIN32_IE >= 0x500
			int GetPos32(LPBOOL pferror)
			{
				return (int) SendMessage(UDM_GETPOS32, (WPARAM) 0, (LPARAM) pferror);
			}
			int SetPos32(int npos)
			{
				return (int) SendMessage(UDM_SETPOS32, (WPARAM) 0, (LPARAM) npos);
			}
		#endif

		#if _WIN32_IE >= 0x400
			LRESULT GetRange32(LPINT plow, LPINT phigh)
			{
				return (LRESULT) SendMessage(UDM_GETRANGE32, (WPARAM) plow, (LPARAM) phigh);
			}
			LRESULT SetRange32(int lower, int upper)
			{
				return (LRESULT) SendMessage(UDM_SETRANGE32, (WPARAM) lower, (LPARAM) upper);
			}
		#endif

		#if _WIN32_IE >= 0x400 && defined(UDM_GETUNICODEFORMAT)
			BOOL GetUnicodeFormat()
			{
				return (BOOL) SendMessage(UDM_GETUNICODEFORMAT, (WPARAM) 0, (LPARAM) 0);
			}
			BOOL SetUnicodeFormat(BOOL unicode)
			{
				return (BOOL) SendMessage(UDM_SETUNICODEFORMAT, (WPARAM) unicode, (LPARAM) 0);
			}
		#endif

		int GetAccel(int caccels, LPUDACCEL paaccels)
		{
			return (int) SendMessage(UDM_GETACCEL, (WPARAM) caccels, (LPARAM) paaccels);
		}
		int GetBase()
		{
			return (int) SendMessage(UDM_GETBASE, (WPARAM) 0, (LPARAM) 0);
		}
		HWND GetBuddy()
		{
			return (HWND) SendMessage(UDM_GETBUDDY, (WPARAM) 0, (LPARAM) 0);
		}
		DWORD GetPos()
		{
			return (DWORD) SendMessage(UDM_GETPOS, (WPARAM) 0, (LPARAM) 0);
		}
		DWORD GetRange()
		{
			return (DWORD) SendMessage(UDM_GETRANGE, (WPARAM) 0, (LPARAM) 0);
		}
		BOOL SetAccel(int naccels, LPUDACCEL aaccels)
		{
			return (BOOL) SendMessage(UDM_SETACCEL, (WPARAM) naccels, (LPARAM) aaccels);
		}
		int SetBase(int nbase)
		{
			return (int) SendMessage(UDM_SETBASE, (WPARAM) nbase, (LPARAM) 0);
		}
		HWND SetBuddy(HWND hwndbuddy)
		{
			return (HWND) SendMessage(UDM_SETBUDDY, (WPARAM) hwndbuddy, (LPARAM) 0);
		}
		int SetPos(short npos)
		{
			return (int) SendMessage(UDM_SETPOS, (WPARAM) 0, (LPARAM) MAKELONG((short)npos, 0));
		}
		LRESULT SetRange(short lower, short upper)
		{
			return (LRESULT) SendMessage(UDM_SETRANGE, (WPARAM) 0, (LPARAM) MAKELONG(upper, lower));
		}
	};

	//
	// ModuleIcons: Loads icons from an .exe or .dll.
	//

	class WNDLIB_EXPORT ModuleIcons
	{
	public:

		ModuleIcons();

		~ModuleIcons();

		// Load the icons from this application's executable.
		bool LoadExeIcons();

		// Load the icons from the specified module.
		bool LoadIcons(const TCHAR *modulePath);

		// Free the memory allocated to the icons.
		void Unload();

		// Get the large icon. Returns NULL if no icons have been loaded or
		// the module did not contain a large icon.
		HICON GetLargeIcon() const
		{
			return _icons[0];
		}

		// Get the small icon. Returns NULL if no icons have been loaded or
		// the module did not contain a small icon.
		HICON GetSmallIcon() const
		{
			return _icons[1];
		}

	private:

		HICON _icons[2];
	};

}

#endif
