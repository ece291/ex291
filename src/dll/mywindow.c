/* mywindow.c -- Window creator/manager
   Copyright (C) 2000 Peter Johnson

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   $Id: mywindow.c,v 1.5 2001/01/10 06:03:50 pete Exp $
*/

#include "ex291srv.h"
#include <ddrawex.h>

static HWND hWnd = NULL;
static DWORD windowThreadId;
static HANDLE hWindowThread;
static HANDLE childEvent;

static int windowWidth;
static int windowHeight;

extern PBYTE WindowedMode;

BOOL InitMyWindow(HINSTANCE hInstance, int width, int height)
{
	windowWidth = width;
	windowHeight = height;

	childEvent = CreateEvent(NULL, FALSE, FALSE, "Extra291_hWndReady");

	hWindowThread = CreateThread(NULL, 0, MyWndThread, (LPVOID)&childEvent,
		0, &windowThreadId);
	if (!hWindowThread)
		return FALSE;
	if(WaitForSingleObject(childEvent, 2000) == WAIT_TIMEOUT)
		return FALSE;
	return TRUE;
}

VOID CloseMyWindow(VOID)
{
	PostMessage(hWnd, WM_CLOSE, 0, 0);
	WaitForSingleObject(childEvent, 2000);
	if(hWindowThread)
		CloseHandle(hWindowThread);
}

HWND GetMyWindow(VOID)
{
	return hWnd;
}

DWORD WINAPI MyWndThread(LPVOID lpParameter)
{
	MSG msg;
	HANDLE hMemory;
	PWNDCLASS pWndClass;
	BOOL bSuccess;
	CHAR lpBuffer[128];
	HANDLE myEvent = *((HANDLE *)lpParameter);
	RECT windowRect;

	hMemory = LocalAlloc(LPTR, sizeof(WNDCLASS));
	if(!hMemory){
		return(FALSE);
	}

	pWndClass = (PWNDCLASS) LocalLock(hMemory);
	pWndClass->hCursor = LoadCursor(NULL, IDC_ARROW);
	pWndClass->hIcon = LoadIcon(NULL, IDI_APPLICATION);
	pWndClass->lpszMenuName = NULL;
	pWndClass->lpszClassName = (LPSTR) "ECE291Render";
	pWndClass->hbrBackground = NULL;//GetStockObject(WHITE_BRUSH);
	pWndClass->hInstance = GetInstance();
	pWndClass->style = 0;
	pWndClass->lpfnWndProc = (WNDPROC)MyWndProc;

	bSuccess = RegisterClass(pWndClass);

	LocalUnlock(hMemory);
	LocalFree(hMemory);

	if(*WindowedMode) {
		RECT windowRect;
		windowRect.left = 100; windowRect.top = 100;
		windowRect.right = 100 + windowWidth - 1;
		windowRect.bottom = 100 + windowHeight - 1;
		AdjustWindowRect(&windowRect, WS_OVERLAPPED | WS_CAPTION, FALSE);

		hWnd = CreateWindowEx(WS_EX_TOPMOST,
			"ECE291Render",
			"ECE 291 Graphics Driver Display",
			WS_OVERLAPPED | WS_CAPTION,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			windowRect.right - windowRect.left,//CW_USEDEFAULT,
			windowRect.bottom - windowRect.top,//CW_USEDEFAULT,
			NULL,
			NULL,
			GetInstance(),
			NULL);
	} else {
		hWnd = CreateWindowEx(WS_EX_TOPMOST,
			"ECE291Render",
			"ECE 291 Graphics Driver Display",
			//WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
			WS_POPUP,
			0,//CW_USEDEFAULT,
			0,//CW_USEDEFAULT,
			windowWidth,//CW_USEDEFAULT,
			windowHeight,//CW_USEDEFAULT,
			NULL,
			NULL,
			GetInstance(),
			NULL);
	}

	if (!hWnd)
		return FALSE;

	ShowWindow(hWnd, SW_SHOW);
	SetForegroundWindow(hWnd);
	UpdateWindow(hWnd);

	SetEvent(myEvent);

	for(;;)
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_CLOSE)
				break;
		}
	}

	DestroyWindow(hWnd);
	UnregisterClass("ECE291Render", GetInstance());

	SetEvent(myEvent);

	ExitThread(0);
}

LONG APIENTRY MyWndProc(HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
	RECT r;
	PAINTSTRUCT ps;

	switch(message)
	{
	// Keyboard Handling
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		AddKey (lParam, FALSE);
		return 0;
	
	case WM_KEYUP:
	case WM_SYSKEYUP:
		AddKey (lParam, TRUE);
		return 0;
	
	case WM_SYSDEADCHAR:
	case WM_DEADCHAR:
	case WM_SYSCHAR:
	case WM_CHAR:
		return 0;

	// Mouse Handling
	case WM_MOUSEMOVE:
		DoMouseCallback(0x01, wParam, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONDOWN:
		DoMouseCallback(0x02, wParam, LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_LBUTTONUP:
		DoMouseCallback(0x04, wParam, LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_RBUTTONDOWN:
		DoMouseCallback(0x08, wParam, LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_RBUTTONUP:
		DoMouseCallback(0x10, wParam, LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_MBUTTONDOWN:
		DoMouseCallback(0x20, wParam, LOWORD(lParam), HIWORD(lParam));
		return 0;
	case WM_MBUTTONUP:
		DoMouseCallback(0x40, wParam, LOWORD(lParam), HIWORD(lParam));
		return 0;

	// Repaint handling
	case WM_PAINT:
		if(GetUpdateRect(hWnd, &r, FALSE)) {
			BeginPaint(hWnd, &ps);
			DDraw_RefreshScreen();	// FIXME: only update needed
			EndPaint(hWnd, &ps);
		}
		return 0;
	}
	return (DefWindowProc(hWnd, message, wParam, lParam));
}

