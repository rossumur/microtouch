// MicrotouchSim.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "MicrotouchSim.h"


void SimInit(HWND hWnd);
void SimTimer(HWND hWnd);
void SimPaint(HDC hdc);
void SimChar(int c);

extern int _mousedown;
extern int _mousex;
extern int _mousey;
extern int _mousepressure;

HWND _console = 0;
HWND _textEdit = 0;
WNDPROC _textProc;

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	Console(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	EditProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_MICROTOUCHSIM, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_MICROTOUCHSIM));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MICROTOUCHSIM));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_MICROTOUCHSIM);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, CW_USEDEFAULT, 240 + 48, 320 + 96, NULL, NULL, hInstance, NULL);

   if (!hWnd)
   {
      return FALSE;
   }
   SimInit(hWnd);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

	// create console but don't show it yet
	_console = CreateDialog(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_CONSOLE), hWnd, Console);
	_textEdit = GetDlgItem(_console,IDC_EDIT1);
	_textProc = (WNDPROC)SetWindowLong(_textEdit, GWL_WNDPROC, (LONG)EditProc);

   return TRUE;
}

void SimConsole(unsigned char c)
{
	int ndx = GetWindowTextLength (_textEdit);
	SetFocus (_textEdit);
    SendMessage (_textEdit, EM_SETSEL, (WPARAM)ndx, (LPARAM)ndx);
	SendMessage (_textEdit, EM_REPLACESEL, 0, (LPARAM) ((LPSTR) &c));
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;

		case IDM_CONSOLE:
			ShowWindow(_console, SW_SHOW);
  			break;

		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;

	case WM_LBUTTONDOWN:
		_mousedown = 1;

	case WM_MOUSEMOVE:
		_mousex = LOWORD(lParam); 
		_mousey = HIWORD(lParam); 
		break;

	case WM_LBUTTONUP:
		_mousedown = 0;
		break;

	case WM_TIMER:
		SimTimer(hWnd);
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		SimPaint(hdc);
		EndPaint(hWnd, &ps);
		break;

	case WM_CHAR:
		if (wParam >= '1' && wParam <= '9')
			_mousepressure = 24*(wParam - '0');
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}


// Message handler for a console
INT_PTR CALLBACK Console(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			::ShowWindow(hDlg,0);
			return (INT_PTR)TRUE;
		}
		if (LOWORD(wParam) == IDC_CLEAR_CONSOLE)
		{
			::SetWindowText(_textEdit,"");
			return (INT_PTR)TRUE;
		}
		break;

	case WM_KEYDOWN:
		SimChar(LOWORD(wParam));
		break;
	}
	return (INT_PTR)FALSE;
}

// Message handler for text
INT_PTR CALLBACK EditProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_CHAR)
		SimChar(LOWORD(wParam));
	return _textProc(hDlg,message,wParam,lParam);
}
