/* ex291srv.c -- dll entry and exit point 
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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "ex291srv.h"
  
static HINSTANCE hInst;

BOOL
WINAPI Extra291Initialize(
	IN HINSTANCE DllHandle,
	IN DWORD Reason,
	IN LPVOID Context)
{
	BYTE i;
	switch (Reason) {
		case DLL_PROCESS_ATTACH:
			START_DEBUGGING;
			LFNVERSION();
/*			for (i=0; i<MAX_HANDLES+2; i++)	
				hFindFile[i] = INVALID_HANDLE_VALUE;
			nHandlesUsed = 0;*/
			SetFileApisToOEM();
			DisableThreadLibraryCalls(DllHandle);
			if(!InitDirectDraw()) {
				MessageBox(NULL,
					"Could not initialize DirectDraw.",
					"Extra BIOS Services for ECE 291",
					MB_OK | MB_ICONERROR);
				return FALSE;
			}
			hInst = DllHandle;

			break;
		case DLL_PROCESS_DETACH :
			SetFileApisToANSI();
			CloseDirectDraw();
			break;
		default :
			DBGMSG("Extra291Initialize: UNKNOWN CALL");
			break;
	}
	return TRUE;
}

VOID
Extra291RegisterInit(VOID)
{
	setCF(0);
	return;
}

VOID 
Extra291TerminateVDM(VOID)
{
	setCF(0);
	return;
}

HINSTANCE GetInstance(VOID)
{
	return hInst;
}

#ifdef DBG
char _DebugString[LARGE_BUF_SIZE]; 
char _DebugPos[LARGE_BUF_SIZE]; 
char _LastFunction[LARGE_BUF_SIZE];
BYTE _DebugStackLevel = 0;
#endif
