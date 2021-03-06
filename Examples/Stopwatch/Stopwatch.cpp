// Simple demonstration of WndLib.

#include "WndLib.h"
#include "Resource.h"
#pragma warning(disable:4201)
#include <mmsystem.h>

// Uncomment this to add a log window
#include "LogWnd.h"

#ifdef _MSC_VER
	#pragma comment(lib, "winmm.lib")
#endif

using namespace WndLib;

//
// StopwatchWnd
//

class StopwatchWnd : public MainWnd
{
	WND_WM_DECLARE(StopwatchWnd, MainWnd)
	WND_WM_FUNC(OnEraseBkgnd)
	WND_WM_FUNC(OnCreate)
	WND_WM_FUNC(OnSize)
	WND_WM_FUNC(OnStartStop)
	WND_WM_FUNC(OnReset)
	WND_WM_FUNC(OnTimer)
	WND_WM_FUNC(OnPaint)
	WND_WM_FUNC(OnCtlColorBtn)
	WND_WM_FUNC(OnLButtonDown)
	
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
	// MainWnd overrides
	//
	
	virtual LPCTSTR GetClassName();
	virtual void GetWndClass(WNDCLASSEX *wc);
	
private:

	static void FormatTime(DWORD elapsedSeconds, TCHAR *buf, size_t bufSize);

	void Draw(HDC hdc);
	void Redraw();
	void SetStartButtonText();
	void CreateBigFont();

	void Log(const TCHAR *event);

	ButtonWnd _startButton;
	ButtonWnd _resetButton;
	RECT _time;
	bool _timerRunning;
	DWORD _startTime;
	DWORD _elapsed;
	Font _font;
	Font _bigFont;
	
	#ifdef WNDLIB_LOGWND_H
		LogWnd _logWnd;
	#endif
};

//
// StopwatchWnd	
//

WND_WM_BEGIN(StopwatchWnd, MainWnd)
	WND_WM(WM_ERASEBKGND, OnEraseBkgnd)
	WND_WM(WM_CREATE, OnCreate)
	WND_WM(WM_SIZE, OnSize)
	WND_WM(WM_PAINT, OnPaint)
	WND_WM(WM_CTLCOLORBTN, OnCtlColorBtn)
	WND_WM_COMMAND(ID_STARTSTOP, OnStartStop)
	WND_WM_COMMAND(ID_RESET, OnReset)
	WND_WM(WM_TIMER, OnTimer)
	WND_WM(WM_LBUTTONDOWN, OnLButtonDown)
WND_WM_END()

StopwatchWnd::StopwatchWnd()
{
	_timerRunning = false;
	_elapsed = 0;
}

StopwatchWnd::~StopwatchWnd()
{
}

bool StopwatchWnd::Create(HWND parent)
{
	return CreateEx(0, TEXT("WndLib Stopwatch"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 
		40, 30, 400, 200, 
		parent, NULL, GetHInstance(), NULL, false);
}

LPCTSTR StopwatchWnd::GetClassName()
{
	return TEXT("StopwatchWnd");
}

void StopwatchWnd::GetWndClass(WNDCLASSEX *wc)
{
	// MainWnd fills in most of this for us.
	MainWnd::GetWndClass(wc);
	
	wc->hbrBackground = NULL;
	wc->hIcon = wc->hIconSm = LoadIcon(GetHInstance(), MAKEINTRESOURCE(IDI_APPICON));
}

LRESULT StopwatchWnd::OnCreate(UINT msg, WPARAM wparam, LPARAM lparam)
{
	#ifdef WNDLIB_LOGWND_H
		if (! _logWnd.Create(TEXT("Stopwatch Log"), NULL))
			return -1;

		_logWnd.Log(TEXT("This is the log window. If you close it, it won't come back unless there's an alert.\n"), 
			RGB(0, 128, 255), LogWnd::SHOWCOMMAND_NO_CHANGE);
	#endif
		
	_font.CreateShellFont();
	
	if (! _startButton.CreateEx(0, TEXT("&Start"), 
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_PUSHBUTTON | WS_TABSTOP,
		0, 0, 0, 0, GetHWnd(), NULL, NULL, NULL, false))
	{
		return -1;
	}
	
	_startButton.SetDlgCtrlID(ID_STARTSTOP);
	_startButton.SetFont(_font);

	if (! _resetButton.CreateEx(0, TEXT("&Reset"), 
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | BS_PUSHBUTTON | WS_TABSTOP,
		0, 0, 0, 0, GetHWnd(), NULL, NULL, NULL, false))
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
		Log(TEXT("Started"));

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

		Log(TEXT("Stopped"));
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
	Log(TEXT("Reset"));

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
	ClientDC dc(this);

	Draw(dc);
	
	return BaseWndProc(msg, wparam, lparam);
}

LRESULT StopwatchWnd::OnLButtonDown(UINT msg, WPARAM wparam, LPARAM lparam)
{
	#ifdef WNDLIB_LOGWND_H
		_logWnd.Format(RGB(255, 0, 0), LogWnd::SHOWCOMMAND_ALERT, TEXT("Don't click there!\n"));
	#endif

	return BaseWndProc(msg, wparam, lparam);
}

LRESULT StopwatchWnd::OnPaint(UINT, WPARAM, LPARAM)
{
	PaintDC dc;
	if (dc.BeginPaint(this))
		Draw(dc);
	
	return 0;
}

LRESULT StopwatchWnd::OnCtlColorBtn(UINT, WPARAM, LPARAM)
{
	return (LRESULT) GetStockObject(BLACK_BRUSH);
}

void StopwatchWnd::CreateBigFont()
{
	// Use a hacky calculation to compute font size based on the window size.
	LOGFONT lf;
	_font.GetLogFont(&lf);
	lf.lfHeight = RectWidth(_time) * 4 / 19;
	
	int maxHeight = RectHeight(_time) * 7 / 8;
	if (lf.lfHeight > maxHeight)
		lf.lfHeight = maxHeight;
	
	_bigFont.CreateFontIndirect(&lf);
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
		TCharStringFormat(buf, bufSize, TEXT("%02d:%02d.%02d"), mm, ss, hs);
	else
		TCharStringFormat(buf, bufSize, TEXT("%02d:%02d:%02d.%02d"), hh, mm, ss, hs);
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
	FormatTime(GetElapsedSeconds(), time, WNDLIB_COUNTOF(time));
	
	TCHAR buf[128];
	TCharStringFormat(buf, WNDLIB_COUNTOF(buf), TEXT("     %s     "), time); 
		
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
	ClientDC dc(this);
	Draw(dc);
}

void StopwatchWnd::Log(const TCHAR *event)
{
	#ifdef WNDLIB_LOGWND_H
		TCHAR time[128];
		FormatTime(GetElapsedSeconds(), time, WNDLIB_COUNTOF(time));
		_logWnd.Format(RGB(0, 128, 0), LogWnd::SHOWCOMMAND_SHOW_IN_BACKGROUND, TEXT("%s at %s\n"), event, time);
	#else
		(void) event;
	#endif
}

//
// WinMain
//

int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE, LPSTR, int showCommand)
{
	InitAllCommonControls();
	
	timeBeginPeriod(1);
	
	SetHInstance(hinstance);
	
	StopwatchWnd myWnd;
	
	if (! myWnd.Create())
		return -1;
	
	myWnd.ShowWindow(showCommand);
	
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (! FilterMessage(&msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	timeEndPeriod(1);
	
	return 0;
}
