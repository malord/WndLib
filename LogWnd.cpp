#include "LogWnd.h"
#include <memory>

namespace WndLib
{
	//
	// LogWnd
	//

	WND_WM_BEGIN(LogWnd, Wnd)
		WND_WM(WM_CLOSE, OnClose)
		WND_WM(WM_DESTROY, OnDestroy)
		WND_WM(WM_CREATE, OnCreate)
		WND_WM(WM_SIZE, OnSize)
		WND_WM(WM_USER, OnUser)
		WND_WM(WM_SETFOCUS, OnSetFocus)
	WND_WM_END()

	LogWnd::LogWnd()
	{
		_font = NULL;
		_logEntry = _lastLogEntry = 0;
		_userDidClose = false;
	}

	LogWnd::~LogWnd()
	{
		DestroyWindow();

		if (_font)
			DeleteObject(_font);

		CriticalSection::ScopedLock lock(_cs);

		while (_logEntry)
		{
			LogEntry *next = _logEntry->next;
			delete _logEntry;
			_logEntry = next;
		}
	}

	bool LogWnd::Create(LPCTSTR title, HWND parent)
	{
		return CreateEx(0, title,
			WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
			40, 30,
			GetSystemMetrics(SM_CXSCREEN) * 40 / 100,
			GetSystemMetrics(SM_CYSCREEN) * 40 / 100,
			parent, NULL, GetHInstance(), NULL, false);
	}

	LPCTSTR LogWnd::GetClassName()
	{
		return TEXT("LogWnd");
	}

	LRESULT LogWnd::OnCreate(UINT msg, WPARAM wparam, LPARAM lparam)
	{
		if (! _font)
			_font = CreateShellFont();

		_icons.LoadExeIcons();

		SendMessage(WM_SETICON, ICON_BIG, (LPARAM) _icons.GetLargeIcon());
		SendMessage(WM_SETICON, ICON_SMALL, (LPARAM) _icons.GetSmallIcon());

		const bool hscroll = false;
		const bool vscroll = true;
		const bool border = false;
		const bool clientedge = false;

		if (! _edit.CreateEx(clientedge ? WS_EX_CLIENTEDGE : 0, TEXT(""),
			WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY |
			ES_AUTOVSCROLL |
			WS_CLIPCHILDREN | WS_CLIPSIBLINGS |
			(hscroll ? (WS_HSCROLL | ES_AUTOHSCROLL) : 0) |
			(vscroll ? WS_VSCROLL : 0) |
			(border ? WS_BORDER : 0), 0, 0, 0, 0, GetHWnd(), NULL, NULL, NULL, true))
		{
			return -1;
		}

		_edit.SetFont(_font);

		RECT desktopRect;
		SystemParametersInfo(SPI_GETWORKAREA, 0, (PVOID) &desktopRect, FALSE);
		SetWindowPos(NULL, desktopRect.left + 16, desktopRect.top + 16, 0, 0,
			SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);

		return BaseWndProc(msg, wparam, lparam);
	}

	LRESULT LogWnd::OnSize(UINT msg, WPARAM wparam, LPARAM lparam)
	{
		RECT rect;
		GetClientRect(&rect);

		_edit.SetWindowPos(NULL, 0, 0, rect.right-rect.left, rect.bottom-rect.top,
			SWP_NOZORDER);

		return BaseWndProc(msg, wparam, lparam);
	}

	LRESULT LogWnd::OnSetFocus(UINT, WPARAM, LPARAM)
	{
		_edit.SetFocus();
		return 0;
	}

	LRESULT LogWnd::OnClose(UINT, WPARAM, LPARAM)
	{
		ShowWindow(SW_HIDE);
		_userDidClose = true;
		return 0;
	}

	LRESULT LogWnd::OnDestroy(UINT, WPARAM, LPARAM)
	{
		return 0;
	}

	void LogWnd::AppendEditControl(LPCTSTR string, ptrdiff_t length)
	{
		if (length < 0)
			length = lstrlen(string);

		LPCTSTR end = string + length;
		LPCTSTR start;

		for (;;)
		{
			start = string;

			while (string != end && *string != '\n' && *string != '\r')
				string++;

			if (string != start)
				AppendEditControlRaw(start, string - start);

			if (string == end)
				break;

			if (*string == '\n')
				AppendEditControlRaw(TEXT("\r\n"), 2);

			++string;
		}
	}

	void LogWnd::AppendEditControlRaw(LPCTSTR string, ptrdiff_t length)
	{
		if (length < 0)
			length = lstrlen(string);

		DWORD len = _edit.GetTextLength();
		_edit.ExSetSel(len, len);
		_edit.ReplaceSel(FALSE, TEXT(""));

		TCHAR buf[256];

		while (length)
		{
			int thistime;
			if (length >= WNDLIB_COUNTOF(buf))
				thistime = (int) (WNDLIB_COUNTOF(buf) - 1);
			else
				thistime = (int) length;

			lstrcpyn(buf, string, thistime + 1);
			buf[thistime] = 0;

			len = _edit.GetTextLength();
			_edit.ExSetSel(len, len);
			_edit.SetCharFormat(SCF_SELECTION, &_charFormat);
			_edit.ReplaceSel(FALSE, buf);

			string += thistime;
			length -= thistime;
		}

		ScrollEditControl();
	}

	void LogWnd::ScrollEditControl()
	{
		DWORD len = _edit.GetTextLength();
		_edit.ExSetSel(len, len);
		_edit.SendMessage(WM_VSCROLL, SB_BOTTOM, 0);
		_edit.ScrollCaret();
		//_edit.UpdateWindow();
	}

	void LogWnd::Log(const TCHAR *log, COLORREF colour, ShowCommand showCommand)
	{
		CriticalSection::ScopedLock lock(_cs);

		std::auto_ptr<LogEntry> logEntry(new LogEntry);
		logEntry->text.Set(log, lstrlen(log) + 1);
		logEntry->colour = colour;
		logEntry->showCommand = showCommand;
		logEntry->next = 0;

		if (_lastLogEntry)
			_lastLogEntry->next = logEntry.get();
		else
			_logEntry = logEntry.get();

		_lastLogEntry = logEntry.release();

		PostMessage(WM_USER);
	}

	void LogWnd::Format(COLORREF colour, ShowCommand showCommand, const TCHAR *fmt, ...)
	{
		va_list argPtr;
		va_start(argPtr, fmt);
		TCHAR buffer[1024];
		Tvsnprintf(buffer, WNDLIB_COUNTOF(buffer), fmt, argPtr);
		va_end(argPtr);

		Log(buffer, colour, showCommand);
	}

	LRESULT LogWnd::OnUser(UINT, WPARAM, LPARAM)
	{
		ProcessQueue();
		return 0;
	}

	void LogWnd::ProcessQueue()
	{
		ShowCommand highestShowCommand = SHOWCOMMAND_NO_CHANGE;

		{
			CriticalSection::ScopedLock lock(_cs);

			while (_logEntry)
			{
				SetColour(_logEntry->colour);

				if (_logEntry->showCommand > highestShowCommand)
					highestShowCommand = _logEntry->showCommand;

				AppendEditControl(_logEntry->text.Get());

				LogEntry *next = _logEntry->next;
				delete _logEntry;
				_logEntry = next;
			}

			_lastLogEntry = 0;
		}

		if (! IsWindowVisible())
		{
			// If the user has explicitly closed us, don't reappear except
			// for an alert message.
			if (highestShowCommand == SHOWCOMMAND_ALERT ||
				(highestShowCommand >= SHOWCOMMAND_SHOW_IN_BACKGROUND && ! _userDidClose))
			{
				ShowWindow(highestShowCommand == SHOWCOMMAND_SHOW_IN_BACKGROUND ? SW_SHOWNOACTIVATE : SW_SHOWNORMAL);
				_edit.ExSetSel(0, 0);
				_edit.ScrollCaret();
				ScrollEditControl();
			}
		}

		if (highestShowCommand == SHOWCOMMAND_ALERT)
			SetForegroundWindow();
	}

	void LogWnd::SetColour(COLORREF colour)
	{
		memset(&_charFormat, 0, sizeof(_charFormat));
		_charFormat.cbSize = sizeof(_charFormat);
		_charFormat.dwMask = CFM_COLOR;
		_charFormat.crTextColor = colour;
	}

	void LogWnd::WaitForUserToClose()
	{
		// Pump any remaining messages since we use a WM_USER to pass logs
		// across threads.
		PumpMessages();

		if (IsWindowVisible())
			SetForegroundWindow();

		while (IsWindowVisible())
		{
			WaitMessage();
			PumpMessages();
		}
	}

	void LogWnd::PumpMessages()
	{
		MSG msg;
		while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	void LogWnd::SetVisible(bool visible, bool inBackground)
	{
		if (visible)
		{
			if (! IsVisible())
			{
				if (inBackground)
					ShowWindow(SW_SHOWNOACTIVATE);
				else
					ShowWindow(SW_SHOWNORMAL);
			}
		}
		else
		{
			if (IsVisible())
				ShowWindow(SW_HIDE);
		}
	}

	bool LogWnd::IsVisible()
	{
		return GetHWnd() && IsWindowVisible();
	}
}

