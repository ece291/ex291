/* ex291srv.h -- constant definitions and function prototypes
   Copyright (C) 1999-2000 Wojciech Galazka
   Copyright (C) 2000-2001 Peter Johnson

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

   $Id: ex291srv.h,v 1.9 2001/04/10 09:06:45 pete Exp $
*/

#ifndef __ex291srv_h
#define __ex291srv_h

#define _WINDOWS
#include <windows.h>

#include <winsock.h>

#include <stdarg.h>

#include <vddsvc.h>		//mem buf translation

#include "mymutex.h"

#include "vbeafdata.h"

#include "socketdata.h"

#define SOCKET_INIT				0x5001
#define SOCKET_EXIT				0x5002
#define SOCKET_ACCEPT				0x5003
#define SOCKET_BIND				0x5004
#define SOCKET_CLOSE				0x5005
#define SOCKET_CONNECT				0x5006
#define SOCKET_GETPEERNAME			0x5007
#define SOCKET_GETSOCKNAME			0x5008
#define SOCKET_INETADDR				0x5009
#define SOCKET_INETNTOA				0x5010
#define SOCKET_LISTEN				0x5011
#define SOCKET_RECV				0x5012
#define SOCKET_RECVFROM				0x5013
#define SOCKET_SEND				0x5014
#define SOCKET_SENDTO				0x5015
#define SOCKET_SHUTDOWN				0x5016
#define SOCKET_CREATE				0x5017
#define SOCKET_GETHOSTBYADDR			0x5018
#define SOCKET_GETHOSTBYNAME			0x5019
#define SOCKET_GETHOSTNAME			0x5020
#define SOCKET_INSTALLCALLBACK			0x5021
#define SOCKET_REMOVECALLBACK			0x5022
#define SOCKET_ADDCALLBACK			0x5023
#define SOCKET_GETCALLBACKINFO			0x5024

#define VBEAF_GET_MEMORY			0x6001
#define VBEAF_GET_MODELIST			0x6002
#define VBEAF_SET_MODE				0x6003
#define	VBEAF_SET_PALETTE			0x6004
#define	VBEAF_BITBLT_VIDEO			0x6005
#define	VBEAF_BITBLT_SYS			0x6006
#define	VBEAF_SET_CURSOR_SHAPE			0x6007
#define	VBEAF_SET_CURSOR_POS			0x6008
#define	VBEAF_SHOW_CURSOR			0x6009

#define GET_MEMORY				0x7001
#define GET_MODES				0x7002
#define SET_MODE				0x7003
#define UNSET_MODE				0x7004
#define REFRESH_SCREEN				0x7005
#define GET_MOUSE_CALLBACK_INFO			0x7006

#define MOUSE_RESET_DRIVER			0x7010
#define MOUSE_SHOW_CURSOR			0x7011
#define MOUSE_HIDE_CURSOR			0x7012
#define MOUSE_GET_POSITION			0x7013
#define MOUSE_SET_POSITION			0x7014
#define MOUSE_GET_PRESS_DATA			0x7015
#define MOUSE_GET_RELEASE_DATA			0x7016
#define MOUSE_DEFINE_HORIZ_RANGE		0x7017
#define MOUSE_DEFINE_VERT_RANGE			0x7018
#define MOUSE_DEFINE_GRAPHICS_CURSOR		0x7019
#define MOUSE_GET_MOTION_COUNTERS		0x701B
#define MOUSE_SET_CALLBACK_PARMS		0x701C
#define MOUSE_SET_LIGHTPEN_ON			0x701D
#define MOUSE_SET_LIGHTPEN_OFF			0x701E
#define MOUSE_SET_MICKEY_RATIO			0x701F
#define MOUSE_DEFINE_UPDATE_REGION		0x7020

HINSTANCE GetInstance(VOID);

BOOL	InitMySockets(unsigned int);
VOID	CloseMySockets(VOID);
VOID	SocketsEnableCallbacks(BYTE, BOOL);
VOID	DoSocketsCallback(UINT, LONG);
VOID	SocketsGetCallbackInfo(VOID);

BOOL	InitDirectDraw(VOID);
VOID	CloseDirectDraw(VOID);
DWORD	DDraw_GetFreeMemory(VOID);
BOOL	DDraw_GetModelist(WORD, WORD, DISPATCH_DATA *);
HRESULT WINAPI	DDraw_GetModelist_Callback(LPDDSURFACEDESC, LPVOID);
int	DDraw_SetMode(int, int, int);
WORD	DDraw_SetMode_Old(int, int, PVOID);
WORD	DDraw_UnSetMode(VOID);
VOID	DDraw_RefreshScreen(VOID);
int	DDraw_BitBltSys(PVOID, DISPATCH_DATA *);
VOID	DDraw_UpdateWindow(RECT *);

BOOL	InitMyWindow(HINSTANCE, int, int);
VOID	CloseMyWindow(VOID);
HWND	GetMyWindow(VOID);

DWORD WINAPI MyWndThread(LPVOID);
LONG APIENTRY MyWndProc(HWND, UINT, UINT, LONG);

BOOL	InitMyWindow2(HINSTANCE);
VOID	CloseMyWindow2(VOID);
HWND	GetMyWindow2(VOID);

DWORD WINAPI MyWndThread2(LPVOID);
LONG APIENTRY MyWndProc2(HWND, UINT, UINT, LONG);

BOOL	InitKey(VOID);
VOID	CloseKey(VOID);
VOID	SuspendKey(VOID);
VOID	ResumeKey(VOID);
VOID	AddKey(LONG, BOOL);
int	GetNextKey(VOID);

VOID	KeyInIO(WORD, BYTE *);
VOID	KeyOutIO(WORD, BYTE);

BOOL	InitMouse(VOID);
VOID	CloseMouse(VOID);
VOID	ShowMouse(VOID);
VOID	HideMouse(VOID);
VOID	GetMousePosition(USHORT *, USHORT *, USHORT *);
VOID	SetMousePosition(USHORT, USHORT);
VOID	SetHorizontalMouseRange(USHORT, USHORT);
VOID	SetVerticalMouseRange(USHORT, USHORT);
VOID	SetMouseCallbackMask(USHORT);
VOID	DoMouseCallback(USHORT, UINT, USHORT, USHORT);
VOID	GetCallbackInfo(USHORT *, USHORT *, USHORT *, USHORT *);

void	vLogMessage(const char *, va_list);
void	LogMessage(const char *, ...);
char *	_m_strtime(char *);

VOID Copy32To24(PVOID, PVOID, int, int);
VOID Copy32To24_pitched(PVOID, PVOID, int, int, int, int, int);
VOID Copy24To32_pitched(PVOID, PVOID, int, int, int, int, int);

char ttoupper(char c);
char tisalpha(char c);
#endif
