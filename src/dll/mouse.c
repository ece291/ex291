/* mouse.c -- Mouse interface
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

   $Id: mouse.c,v 1.2 2000/12/18 06:28:00 pete Exp $
*/

#include "ex291srv.h"

#define MOUSE_IRQ_LINE	4
#define MOUSE_IRQ_PIC	ICA_SLAVE

extern PBYTE inDispatch;
extern PBYTE VDD_int_wait;
extern PBYTE DOS_VDD_Mutex;
extern PBYTE WindowedMode;

extern int BackBufWidth;
extern int BackBufHeight;

static USHORT CallbackMask;

static USHORT CallbackCondition;
static USHORT CallbackButtonState;
static USHORT CallbackCursorColumn;
static USHORT CallbackCursorRow;
static HANDLE mouseMutex;

BOOL InitMouse(VOID)
{
	mouseMutex = CreateMutex(NULL, TRUE, NULL);
	if(!mouseMutex)
		return FALSE;
	ReleaseMutex(mouseMutex);

	CallbackMask = 0;

//	LogMessage("Initialized mouse");

	return TRUE;
}

VOID CloseMouse(VOID)
{
	CloseHandle(mouseMutex);
//	LogMessage("Shut down mouse");
}

VOID ShowMouse(VOID)
{
	ShowCursor(TRUE);
}

VOID HideMouse(VOID)
{
	ShowCursor(FALSE);
}

VOID GetMousePosition(USHORT *button, USHORT *column, USHORT *row)
{
	POINT point;
	USHORT bm;

	// Get cursor position
	if(!GetCursorPos(&point))
	{
		button = 0; column = 0; row = 0;
		return;
	}

	*column = (USHORT)point.x;
	*row = (USHORT)point.y;

	// Get button status
	bm = 0;
	if (GetAsyncKeyState(VK_LBUTTON))
		bm |= 0x01;
	if (GetAsyncKeyState(VK_RBUTTON))
		bm |= 0x02;
	if (GetAsyncKeyState(VK_MBUTTON))
		bm |= 0x04;

	*button = bm;
}

VOID SetMousePosition(USHORT column, USHORT row)
{
/*	INPUT input;

	memset(&input, 0, sizeof(INPUT));
	input.type = INPUT_MOUSE;
	input.mi.dx = column*(65535 / BackBufWidth);
	input.mi.dy = row*(65535 / BackBufHeight);
	input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;

	SendInput(1, &input, sizeof(INPUT));*/

	SetCursorPos(column, row);
}

VOID SetHorizontalMouseRange(USHORT min, USHORT max)
{
	RECT rect;

	if(!GetClipCursor(&rect))
		return;

	if(*WindowedMode)
		return;

	rect.left = min;
	rect.right = max;
	ClipCursor(&rect);
}

VOID SetVerticalMouseRange(USHORT min, USHORT max)
{
	RECT rect;

	if(!GetClipCursor(&rect))
		return;

	if(*WindowedMode) 
		return;
	
	rect.top = min;
	rect.bottom = max;
	ClipCursor(&rect);
}

VOID SetMouseCallbackMask(USHORT mask)
{
	CallbackMask = mask;
}

VOID DoMouseCallback(USHORT condition, UINT keys, USHORT column, USHORT row)
{
	// is it a global callback condition?
	if ((condition & CallbackMask) == 0)
		return;

	switch(WaitForSingleObject(mouseMutex, 50L))
	{
	case WAIT_OBJECT_0:
		CallbackCondition = condition;

		// calculate button state
		CallbackButtonState = 0;
		if (keys & MK_LBUTTON)
			CallbackButtonState |= 0x01;
		if (keys & MK_RBUTTON)
			CallbackButtonState |= 0x02;
		if (keys & MK_MBUTTON)
			CallbackButtonState |= 0x04;

		CallbackCursorColumn = column;
		CallbackCursorRow = row;

		ReleaseMutex(mouseMutex);

/*		LogMessage("Mouse Callback: AX=%x, BX=%x, CX=%x, DX=%x",
			(int)condition, (int)CallbackButtonState, (int)column,
			(int)row);*/

		mutex_lock(DOS_VDD_Mutex);
		*VDD_int_wait = 1;
		while(*inDispatch)
		{
			mutex_unlock(DOS_VDD_Mutex);
			Sleep(0);
			mutex_lock(DOS_VDD_Mutex);
		}
		mutex_unlock(DOS_VDD_Mutex);
		//LogMessage("Simulate IRQ 12");
		VDDSimulateInterrupt(MOUSE_IRQ_PIC, MOUSE_IRQ_LINE, 1);
		//LogMessage("Done Simulate IRQ 12");
		mutex_lock(DOS_VDD_Mutex);
		*VDD_int_wait = 0;

		// wake up VDM thread here?

		mutex_unlock(DOS_VDD_Mutex);
		//LogMessage("Done Mouse Callback Trigger");

		break;
	}
}

VOID GetCallbackInfo(USHORT *cond, USHORT *bstate, USHORT *col, USHORT *row)
{
	//LogMessage("GetCallbackInfo start");
	switch(WaitForSingleObject(mouseMutex, INFINITE))
	{
	case WAIT_OBJECT_0:
		*cond = CallbackCondition;
		*bstate = CallbackButtonState;
		*col = CallbackCursorColumn;
		*row = CallbackCursorRow;
/*		LogMessage("Got CallbackInfo: AX=%x BX=%x CX=%x DX=%x",
			(int)CallbackCondition, (int)CallbackButtonState,
			(int)CallbackCursorColumn, (int)CallbackCursorRow);*/
		ReleaseMutex(mouseMutex);
		break;
	case WAIT_TIMEOUT:
	case WAIT_ABANDONED:
		*cond = 0;
		*bstate = 0;
		*col = 0;
		*row = 0;
	}
}

