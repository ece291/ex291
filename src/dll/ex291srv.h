/* ex291srv.h -- constant definitions and function prototypes
   Copyright (C) 1999-2000 Wojciech Galazka
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

   $Id: ex291srv.h,v 1.2 2000/12/18 06:28:00 pete Exp $
*/

#ifndef __ex291srv_h
#define __ex291srv_h

#define TESTING 	1
#ifdef WIN_32
#define WIN
#define FLAT_32
#define TRUE_IF_WIN32	1
#else
#define TRUE_IF_WIN32	0
#endif

#ifdef WIN
#define _WINDOWS
#include <windows.h>
#endif

#include <stdarg.h>

#ifdef UNICODE
#error cannot handle UNICODE 
#endif

#include <vddsvc.h>		//mem buf translation

#define	MSG_BUF_SIZE				256
#define SMALL_BUF_SIZE				32
#define LARGE_BUF_SIZE				128
#define	MAX_PATHNAME_SIZE			255
#define	MAX_FILENAME_SIZE			260

#include "debug.h"

#include "mymutex.h"

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

#define	MAKEADDRESS(seg, ofs)	((ULONG)(((seg) << 16) | (ofs)))

#define	SFT_SIZE        0x21
#define	JFT_SIZE	20

HINSTANCE GetInstance(VOID);

BOOL	InitDirectDraw(VOID);
VOID	CloseDirectDraw(VOID);
WORD	DDraw_GetFreeMemory(VOID);
WORD	DDraw_SetMode(int, int, PVOID);
WORD	DDraw_UnSetMode(VOID);
VOID	DDraw_RefreshScreen(VOID);

BOOL	InitMyWindow(HINSTANCE, int, int);
VOID	CloseMyWindow(VOID);
HWND	GetMyWindow(VOID);

DWORD WINAPI MyWndThread(LPVOID);
LONG APIENTRY MyWndProc(HWND, UINT, UINT, LONG);

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

char ttoupper(char c);
char tisalpha(char c);
#endif
