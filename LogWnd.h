//
// WndLib
// Copyright (c) 1994-2014 Mark H. P. Lord. All rights reserved.
//
// See LICENSE.txt for license.
//

#ifndef WNDLIB_LOGWND_H
#define WNDLIB_LOGWND_H

#include "WndLib.h"
#include <stddef.h>

namespace WndLib
{
	//
	// LogWnd: A thread-safe logging window with colourised output.
	//

	class WNDLIB_EXPORT LogWnd : public Wnd
	{
		WND_WM_DECLARE(LogWnd, Wnd)
		WND_WM_FUNC(OnClose)
		WND_WM_FUNC(OnDestroy)
		WND_WM_FUNC(OnCreate)
		WND_WM_FUNC(OnSize)
		WND_WM_FUNC(OnUser)
		WND_WM_FUNC(OnSetFocus)

	public:

		LogWnd();

		~LogWnd();

		bool Create(LPCTSTR title, HWND parent = NULL);

		enum ShowCommand
		{
			// The window is not shown if it's hidden.
			SHOWCOMMAND_NO_CHANGE,

			// The window is shown if it's hidden, but it doesn't move to the foreground.
			SHOWCOMMAND_SHOW_IN_BACKGROUND,

			// The window is shown and activated if it's hidden.
			SHOWCOMMAND_SHOW_IN_FOREGROUND,

			// The window is shown if it's hidden, and moved to the foreground.
			SHOWCOMMAND_ALERT
		};

		// Write to the log. Can be called from any thread.
		void Log(const TCHAR *log, COLORREF colour, ShowCommand showCommand);

		// Write a printf formatted string to the log. Can be called from any thread.
		void Format(COLORREF colour, ShowCommand showCommand, const TCHAR *fmt, ...);

		// You don't need to call this manually.
		void ProcessQueue();

		// Pumps messages until the log window is closed.
		void WaitForUserToClose();

		// Old name.
		void AllowUserToReadLog()
		{
			WaitForUserToClose();
		}

		void SetVisible(bool visible, bool inBackground = false);

		bool IsVisible();

		// Make sure the edit control's caret is visible.
		void ScrollEditControl();

		// Wnd overrides
		virtual LPCTSTR GetClassName();

	private:

		void AppendEditControl(LPCTSTR string, ptrdiff_t length = -1);

		void AppendEditControlRaw(LPCTSTR string, ptrdiff_t length = -1);

		void SetColour(COLORREF colour);

		static void PumpMessages();

		struct LogEntry
		{
			WinString text;
			COLORREF colour;
			ShowCommand showCommand;
			LogEntry *next;
		};

		RichEdit2Wnd _edit;
		Font _font;
		CHARFORMAT _charFormat;

		CriticalSection _cs;
		LogEntry *_logEntry;
		LogEntry *_lastLogEntry;

		bool _userDidClose;
		ModuleIcons _icons;
	};
}

#endif

