// Simple demonstration of WndLib.

#include "WndLib.h"
#include "Resource.h"
#pragma warning(disable:4201)
#include <mmsystem.h>

// Uncomment this to add a log window
#include "LogWnd.h"

#ifdef _MSC_VER
	#pragma comment(lib, "winmm.lib")
	#pragma comment(lib, "comctl32.lib")
	// Windows XP visual styles manifest
	#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

using namespace WndLib;

//
// StopwatchWnd
//

class StopwatchWnd : public Wnd
{
	WND_WM_DECLARE(StopwatchWnd, Wnd)
	WND_WM_FUNC(OnClose)
	WND_WM_FUNC(OnDestroy)
	WND_WM_FUNC(OnEraseBkgnd)
	WND_WM_FUNC(OnCreate)
	WND_WM_FUNC(OnSize)
	WND_WM_FUNC(OnStartStop)
	WND_WM_FUNC(OnReset)
	WND_WM_FUNC(OnTimer)
	WND_WM_FUNC(OnPaint)
	WND_WM_FUNC(OnCtlColorBtn)
	
public:

	// Control IDs.
	enum
	{
		ID_STARTSTOP = 100,
		ID_RESET = 101,
		ID_TIMERTICK = 102
	};
	
	StopwatchWnd();
	
	~StopwatchWnd();

	bool Create(HWND parent = NULL);

	DWORD GetElapsedSeconds() const;

	//
	// Wnd overrides
	//
	
	virtual LPCTSTR GetClassName();
	virtual void GetWndClass(WNDCLASSEX *wc);
	
private:

	static void FormatTime(DWORD elapsedSeconds, TCHAR *buf, size_t bufSize);

	void Draw(HDC hdc);
	void Redraw();
	void SetStartButtonText();
	void CreateBigFont();

	ButtonWnd _startButton;
	ButtonWnd _resetButton;
	RECT _time;
	bool _timerRunning;
	DWORD _startTime;
	DWORD _elapsed;
	HFONT _font;
	HFONT _bigFont;
	
	#ifdef WNDLIB_LOGWND_H
		LogWnd _logWnd;
	#endif
};

//
// StopwatchWnd	
//

WND_WM_BEGIN(StopwatchWnd, Wnd)
	WND_WM(WM_CLOSE, OnClose)
	WND_WM(WM_DESTROY, OnDestroy)
	WND_WM(WM_ERASEBKGND, OnEraseBkgnd)
	WND_WM(WM_CREATE, OnCreate)
	WND_WM(WM_SIZE, OnSize)
	WND_WM(WM_PAINT, OnPaint)
	WND_WM(WM_CTLCOLORBTN, OnCtlColorBtn)
	WND_WM_COMMAND(ID_STARTSTOP, OnStartStop)
	WND_WM_COMMAND(ID_RESET, OnReset)
	WND_WM(WM_TIMER, OnTimer)
WND_WM_END()

StopwatchWnd::StopwatchWnd()
{
	_timerRunning = false;
	_elapsed = 0;
	_font = _bigFont = NULL;
}

StopwatchWnd::~StopwatchWnd()
{
	if (_font)	
		DeleteObject(_font);
		
	if (_bigFont)
		DeleteObject(_bigFont);
}

bool StopwatchWnd::Create(HWND parent)
{
	return CreateEx(0, TEXT("WndLib Stopwatch"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 
		40, 30, 400, 200, 
		parent, NULL, GetInstanceHandle(), NULL, false);
}

LPCTSTR StopwatchWnd::GetClassName()
{
	return TEXT("StopwatchWnd");
}

void StopwatchWnd::GetWndClass(WNDCLASSEX *wc)
{
	// Wnd fills in most of this for us.
	Wnd::GetWndClass(wc);
	
	wc->hbrBackground = NULL;
	wc->hIcon = wc->hIconSm = LoadIcon(GetInstanceHandle(), MAKEINTRESOURCE(IDI_APPICON));
}

LRESULT StopwatchWnd::OnCreate(UINT msg, WPARAM wparam, LPARAM lparam)
{
	#ifdef WNDLIB_LOGWND_H
		if (! _logWnd.Create(TEXT("Stopwatch Log"), NULL))
			return -1;

		_logWnd.Log(TEXT("This is the log window. If you close it, it won't come back until you restart Stopwatch.\n"), 
			RGB(128, 128, 128), LogWnd::SHOWCOMMAND_NO_CHANGE);
	#endif
		
	_font = CreateShellFont();
	
	if (! _startButton.CreateEx(0, TEXT("&Start"), 
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_PUSHBUTTON | WS_TABSTOP,
		0, 0, 0, 0, HWnd(), NULL, NULL, NULL, false))
	{
		return -1;
	}
	
	_startButton.SetDlgCtrlID(ID_STARTSTOP);
	_startButton.SetFont(_font);

	if (! _resetButton.CreateEx(0, TEXT("&Reset"), 
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_PUSHBUTTON | WS_TABSTOP,
		0, 0, 0, 0, HWnd(), NULL, NULL, NULL, false))
	{
		return -1;
	}

	_resetButton.SetDlgCtrlID(ID_RESET);
	_resetButton.SetFont(_font);
	
	CentreWindow(NULL);
	
	return BaseWndProc(msg, wparam, lparam);
}

LRESULT StopwatchWnd::OnSize(UINT msg, WPARAM wparam, LPARAM lparam)
{
	RECT rect;
	GetClientRect(&rect);
	
	SIZE size;
	size.cx = RectWidth(rect);
	size.cy = RectHeight(rect);
	
	int gap = size.cy / 25;
	
	SIZE buttonSize;
	buttonSize.cx = size.cx / 4;
	buttonSize.cy = size.cy / 5;
	
	if (buttonSize.cy > 60)
		buttonSize.cy = 60;
		
	if (buttonSize.cx > 160)
		buttonSize.cx = 160;
	
	RECT button;
	button.left = rect.left + (size.cx - (buttonSize.cx * 2 + gap)) / 2;
	button.top = rect.top + gap;
	button.right = button.left + buttonSize.cx;
	button.bottom = button.top + buttonSize.cy;
	
	_startButton.SetWindowPos(NULL, button, SWP_NOZORDER | SWP_NOACTIVATE);
		
	button.left += buttonSize.cx + gap;
	button.right += buttonSize.cx + gap;

	_resetButton.SetWindowPos(NULL, button, SWP_NOZORDER | SWP_NOACTIVATE);
		
	_time.left = gap;
	_time.top = button.bottom + gap;
	_time.right = rect.right - gap;
	_time.bottom = rect.bottom - gap;
	
	if (_bigFont)
	{
		DeleteObject(_bigFont);
		_bigFont = NULL;
	}
	
	return BaseWndProc(msg, wparam, lparam);
}

LRESULT StopwatchWnd::OnClose(UINT, WPARAM, LPARAM)
{
	DestroyWindow();
	return 0;
}

LRESULT StopwatchWnd::OnDestroy(UINT msg, WPARAM wparam, LPARAM lparam)
{
	PostQuitMessage(0);
	return BaseWndProc(msg, wparam, lparam);
}

LRESULT StopwatchWnd::OnEraseBkgnd(UINT, WPARAM wparam, LPARAM)
{
	RECT rect;
	GetClientRect(&rect);
	
	FillRect((HDC) wparam, &rect, (HBRUSH) GetStockObject(BLACK_BRUSH));
	
	return TRUE;
}

LRESULT StopwatchWnd::OnStartStop(UINT, WPARAM, LPARAM)
{
	if (! _timerRunning)
	{
		_timerRunning = SetTimer(ID_TIMERTICK, 50, NULL) != 0;
			
		if (_timerRunning)
			_startTime = timeGetTime() - _elapsed;
	}
	else
	{
		_elapsed = timeGetTime() - _startTime;

		Redraw();
		
		KillTimer(ID_TIMERTICK);
		_timerRunning = false;
	}
	
	SetStartButtonText();
	
	return 0;
}

void StopwatchWnd::SetStartButtonText()
{
	if (_timerRunning)
		_startButton.SetWindowText(TEXT("&Stop"));
	else
		_startButton.SetWindowText(TEXT("&Start"));
}

LRESULT StopwatchWnd::OnReset(UINT, WPARAM, LPARAM)
{
	#ifdef WNDLIB_LOGWND_H
		TCHAR time[128];
		FormatTime(GetElapsedSeconds(), time, sizeof(time));
		_logWnd.Format(RGB(0, 128, 0), LogWnd::SHOWCOMMAND_SHOW_IN_BACKGROUND, TEXT("Reset at %s\n"), time);
	#endif

	if (_timerRunning)
	{
		_timerRunning = false;
		KillTimer(ID_TIMERTICK);
	}

	_elapsed = 0;
	
	SetStartButtonText();
	
	Redraw();
	
	return 0;
}

LRESULT StopwatchWnd::OnTimer(UINT msg, WPARAM wparam, LPARAM lparam)
{
	HDC hdc = GetDC();
	Draw(hdc);
	ReleaseDC(hdc);
	
	return BaseWndProc(msg, wparam, lparam);
}

LRESULT StopwatchWnd::OnPaint(UINT, WPARAM, LPARAM)
{
	PAINTSTRUCT ps;
	if (BeginPaint(&ps))
	{
		Draw(ps.hdc);
			
		EndPaint(&ps);
	}
	
	return 0;
}

LRESULT StopwatchWnd::OnCtlColorBtn(UINT, WPARAM, LPARAM)
{
	return (LRESULT) GetStockObject(BLACK_BRUSH);
}

void StopwatchWnd::CreateBigFont()
{
	if (_bigFont)
		DeleteObject(_bigFont);
		
	// Use a hacky calculation to compute font size based on the window size.
	LOGFONT lf;
	GetObject(_font, sizeof(LOGFONT), &lf);
	lf.lfHeight = RectWidth(_time) * 4 / 19;
	
	int maxHeight = RectHeight(_time) * 7 / 8;
	if (lf.lfHeight > maxHeight)
		lf.lfHeight = maxHeight;
	
	_bigFont = CreateFontIndirect(&lf);
}

void StopwatchWnd::FormatTime(DWORD elapsedSeconds, TCHAR *buf, size_t bufSize)
{
	int hs = ((int) elapsedSeconds / 10) % 100;
	elapsedSeconds /= 1000;
	int ss = (int) elapsedSeconds % 60;
	elapsedSeconds /= 60;
	int mm = (int) elapsedSeconds % 60;
	elapsedSeconds /= 60;
	int hh = (int) elapsedSeconds;
	
	if (! hh)
		SNTPrintf(buf, bufSize, TEXT("%02d:%02d.%02d"), mm, ss, hs);
	else
		SNTPrintf(buf, bufSize, TEXT("%02d:%02d:%02d.%02d"), hh, mm, ss, hs);
}

DWORD StopwatchWnd::GetElapsedSeconds() const
{
	DWORD elapsed = timeGetTime() - _startTime;
	if (! _timerRunning)
		elapsed = _elapsed;

	return elapsed;
}

void StopwatchWnd::Draw(HDC hdc)
{
	if (! _bigFont)
		CreateBigFont();
		
	HFONT oldFont = NULL;
	if (_bigFont)
		oldFont = (HFONT) SelectObject(hdc, _bigFont);	

	TCHAR time[128];
	FormatTime(GetElapsedSeconds(), time, sizeof(time));
	
	TCHAR buf[128];
	SNTPrintf(buf, sizeof(buf), TEXT("     %s     "), time); 
		
	SetTextColor(hdc, RGB(100, 255, 0));
	SetBkColor(hdc, RGB(0, 0, 0));
	SetBkMode(hdc, OPAQUE);
	
	DrawText(hdc, buf, lstrlen(buf), &_time, 
		DT_CENTER | DT_SINGLELINE | DT_VCENTER);
		
	if (oldFont)
		SelectObject(hdc, oldFont);
}

void StopwatchWnd::Redraw()
{
	HDC hdc = GetDC();
	Draw(hdc);
	ReleaseDC(hdc);
}

//
// WinMain
//

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int showCommand)
{
	INITCOMMONCONTROLSEX controls;
	memset(&controls, 0, sizeof(controls));
	controls.dwSize = sizeof(controls);
	controls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&controls);
	
	timeBeginPeriod(1);
	
	SetInstanceHandle(hinstance);
	
	StopwatchWnd myWnd;
	
	if (! myWnd.Create())
		return -1;
	
	myWnd.ShowWindow(showCommand);
	
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (IsDialogMessage(myWnd.HWnd(), &msg))
			continue;
			
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	timeEndPeriod(1);
	
	return 0;
}
