/* mywindow2.c -- Secondary Window creator/manager
   Copyright (C) 2001 Peter Johnson

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

   $Id: mywindow2.c,v 1.1 2001/04/10 09:06:45 pete Exp $
*/

#include "ex291srv.h"
#include <ddrawex.h>

static HWND hWnd = NULL;
static DWORD windowThreadId;
static HANDLE hWindowThread;
static HANDLE childEvent;

static BOOL WindowReady = FALSE;

BOOL InitMyWindow2(HINSTANCE hInstance)
{
    if(WindowReady)
	return FALSE;

    childEvent = CreateEvent(NULL, FALSE, FALSE, "Extra291_hWndReady2");

    hWindowThread = CreateThread(NULL, 0, MyWndThread2, (LPVOID)&childEvent,
	0, &windowThreadId);
    if (!hWindowThread)
	return FALSE;
    if(WaitForSingleObject(childEvent, 2000) == WAIT_TIMEOUT)
	return FALSE;
    WindowReady = TRUE;
    return TRUE;
}

VOID CloseMyWindow2(VOID)
{
    if(!WindowReady)
	return;

    PostMessage(hWnd, WM_CLOSE, 0, 0);
    WaitForSingleObject(childEvent, 2000);
    if(hWindowThread)
	CloseHandle(hWindowThread);

    WindowReady = FALSE;
}

HWND GetMyWindow2(VOID)
{
    if(!WindowReady)
	return 0;

    return hWnd;
}

DWORD WINAPI MyWndThread2(LPVOID lpParameter)
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
    pWndClass->lpszClassName = (LPSTR) "ECE291Socket";
    pWndClass->hbrBackground = NULL;//GetStockObject(WHITE_BRUSH);
    pWndClass->hInstance = GetInstance();
    pWndClass->style = 0;
    pWndClass->lpfnWndProc = (WNDPROC)MyWndProc2;

    bSuccess = RegisterClass(pWndClass);

    LocalUnlock(hMemory);
    LocalFree(hMemory);

    hWnd = CreateWindowEx(0,
	"ECE291Socket",
	"ECE 291 Sockets Handler",
	WS_ICONIC,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	CW_USEDEFAULT,
	NULL,
	NULL,
	GetInstance(),
	NULL);

    if (!hWnd)
	return FALSE;

    SetEvent(myEvent);

    for(;;) {
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	    if (msg.message == WM_CLOSE)
		break;
	}
    }

    DestroyWindow(hWnd);
    UnregisterClass("ECE291Socket", GetInstance());

    SetEvent(myEvent);

    ExitThread(0);
}

LONG APIENTRY MyWndProc2(HWND hWnd, UINT message, UINT wParam, LONG lParam)
{
    switch(message) {
	// Sockets Handling
	case WM_USER:
	    DoSocketsCallback(wParam, WSAGETSELECTEVENT(lParam));
	    break;
    }
    return (DefWindowProc(hWnd, message, wParam, lParam));
}

