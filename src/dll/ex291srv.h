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

   $Id: ex291srv.h,v 1.12 2001/12/12 18:28:06 pete Exp $
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

HINSTANCE GetInstance(VOID);

BOOL	InitMySockets(unsigned int);
VOID __cdecl CloseMySockets(VOID);
VOID	SocketsEnableCallbacks(BYTE, BOOL);
VOID	DoSocketsCallback(UINT, LONG);

VOID __cdecl Socket_accept(UINT, UINT);
VOID __cdecl Socket_bind(UINT, UINT);
VOID __cdecl Socket_close(UINT);
VOID __cdecl Socket_connect(UINT, UINT);
VOID __cdecl Socket_getpeername(UINT, UINT);
VOID __cdecl Socket_getsockname(UINT, UINT);
VOID __cdecl Socket_inetaddr(UINT);
VOID __cdecl Socket_inetntoa(UINT);
VOID __cdecl Socket_listen(UINT, int);
VOID __cdecl Socket_recv(UINT, UINT, int, UINT);
VOID __cdecl Socket_recvfrom(UINT, UINT, int, UINT, UINT);
VOID __cdecl Socket_send(UINT, UINT, int, UINT);
VOID __cdecl Socket_sendto(UINT, UINT, int, UINT, UINT);
VOID __cdecl Socket_shutdown(UINT, UINT);
VOID __cdecl Socket_create(UINT);
VOID __cdecl Socket_gethostbyaddr(UINT);
VOID __cdecl Socket_gethostbyname(UINT);
VOID __cdecl Socket_gethostname(UINT, int);
VOID __cdecl Socket_InstallCallback(VOID);
VOID __cdecl Socket_RemoveCallback(VOID);
VOID __cdecl Socket_AddCallback(UINT, UINT);
VOID __cdecl Socket_GetCallbackInfo(VOID);
VOID __cdecl Socket_getsockopt(UINT, int, int, UINT, UINT);
VOID __cdecl Socket_setsockopt(UINT, int, int, UINT, int);

BOOL	InitDirectDraw(VOID);
VOID	CloseDirectDraw(VOID);
HRESULT WINAPI	DDraw_GetModelist_Callback(LPDDSURFACEDESC, LPVOID);
int	DDraw_SetMode(int, int, int);
WORD	DDraw_SetMode_Old(int, int, PVOID);
WORD	DDraw_UnSetMode(VOID);
VOID	DDraw_RefreshScreen(VOID);
VOID	DDraw_UpdateWindow(RECT *);

VOID	VBEAF_GetMemory(DISPATCH_DATA *);
VOID	VBEAF_GetModelist(DISPATCH_DATA *);
VOID	VBEAF_SetPalette(DISPATCH_DATA *);
VOID	VBEAF_BitBltVideo(DISPATCH_DATA *);
VOID	VBEAF_BitBltSys(DISPATCH_DATA *);
VOID	Legacy_GetMemory(VOID);
VOID	Legacy_GetModes(VOID);
VOID	Legacy_RefreshScreen(VOID);

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
VOID	DoMouseCallback(USHORT, UINT, USHORT, USHORT);

VOID	Mouse_GetCallbackInfo(VOID);
VOID	Mouse_ResetDriver(VOID);
VOID	Mouse_ShowCursor(VOID);
VOID	Mouse_HideCursor(VOID);
VOID	Mouse_GetPosition(VOID);
VOID	Mouse_SetPosition(VOID);
VOID	Mouse_GetPressData(VOID);
VOID	Mouse_GetReleaseData(VOID);
VOID	Mouse_DefineHorizRange(VOID);
VOID	Mouse_DefineVertRange(VOID);
VOID	Mouse_DefineGraphicsCursor(VOID);
VOID	Mouse_GetMotionCounters(VOID);
VOID	Mouse_SetCallbackParms(VOID);
VOID	Mouse_SetLightpenOn(VOID);
VOID	Mouse_SetLightpenOff(VOID);
VOID	Mouse_SetMickeyRatio(VOID);
VOID	Mouse_DefineUpdateRegion(VOID);
VOID	VBEAF_SetCursorShape(DISPATCH_DATA *);
VOID	VBEAF_SetCursorPos(DISPATCH_DATA *);
VOID	VBEAF_ShowCursor(DISPATCH_DATA *);

void	vLogMessage(const char *, va_list);
void	LogMessage(const char *, ...);
char *	_m_strtime(char *);

VOID Copy32To24(PVOID, PVOID, int, int);
VOID Copy32To24_pitched(PVOID, PVOID, int, int, int, int, int);
VOID Copy24To32_pitched(PVOID, PVOID, int, int, int, int, int);

char ttoupper(char c);
char tisalpha(char c);
#endif
