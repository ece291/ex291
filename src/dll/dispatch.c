/* dispatch.c -- main dispatch routine
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

   $Id: dispatch.c,v 1.2 2000/12/18 06:28:00 pete Exp $
*/

#include "ex291srv.h"
#define USED_AS_C_HEADER
#include "ex291srv.rc"

static USHORT displaySegment;
static ULONG displayOffset;
static VDM_MODE displayMode;
static PVOID displayBuffer;

static USHORT inDispatchSegment;
static ULONG inDispatchOffset;
PBYTE inDispatch;
PBYTE VDD_int_wait;
PBYTE DOS_VDD_Mutex;
PBYTE WindowedMode;

VOID 	Extra291Dispatch(VOID)
{
	WORD  regAX = HIWORD(getEAX());

	switch (regAX)
	{
		case GET_MEMORY:
{
	setCF(0);
	setAX(DDraw_GetFreeMemory());
	break;
}
		case SET_MODE:
{
	inDispatchSegment = getCS();
	inDispatchOffset = getEBX() & 0xFFFF;

	inDispatch = (PBYTE) VdmMapFlat(inDispatchSegment, inDispatchOffset,
		VDM_V86);
	VDD_int_wait = inDispatch + 1;
	DOS_VDD_Mutex = inDispatch + 2;
	WindowedMode = inDispatch + 3;

	//LogMessage("Windowed Flag = %d", (int)*WindowedMode);

/*	LogMessage("Segment=0x%x, Offset=0x%x", (unsigned int)inDispatchSegment,
		inDispatchOffset);*/

	if(!InitKey()) {
		MessageBox(NULL,
			"Could not initialize keyboard driver.",
			"Extra BIOS Services for ECE 291",
			MB_OK | MB_ICONERROR);
		setCF(1);
		setAX(0xFFFF);
		VdmUnmapFlat(inDispatchSegment, inDispatchOffset, inDispatch,
			VDM_V86);
		break;
	}
	if(!InitMouse()) {
		MessageBox(NULL,
			"Could not initialize mouse driver.",
			"Extra BIOS Services for ECE 291",
			MB_OK | MB_ICONERROR);
		setCF(1);
		setAX(0xFFFF);
		CloseKey();
		VdmUnmapFlat(inDispatchSegment, inDispatchOffset, inDispatch,
			VDM_V86);
		break;
	}
	if(!InitMyWindow(GetInstance(), (int)HIWORD(getECX()),
		(int)LOWORD(getECX()))) {
		MessageBox(NULL,
			"Could not initialize output window.",
			"Extra BIOS Services for ECE 291",
			MB_OK | MB_ICONERROR);
		setCF(1);
		setAX(0xFFFF);
		CloseMouse();
		CloseKey();
		VdmUnmapFlat(inDispatchSegment, inDispatchOffset, inDispatch,
			VDM_V86);
		break;
	}

	displaySegment = getDX();
	displayOffset = getEDI();
	displayMode = VDM_PM;

/*	LogMessage("Segment=0x%x, Offset=0x%x", (unsigned int)displaySegment,
		displayOffset);*/

	displayBuffer = VdmMapFlat(displaySegment, displayOffset, displayMode);

	setCF(0);
	setAX(DDraw_SetMode((int)HIWORD(getECX()), (int)LOWORD(getECX()),
		displayBuffer));
	break;
}
		case UNSET_MODE:
{
	setCF(0);
	setAX(DDraw_UnSetMode());
	CloseMyWindow();
	CloseMouse();
	CloseKey();

	VdmUnmapFlat(displaySegment, displayOffset, displayBuffer, displayMode);
	VdmUnmapFlat(inDispatchSegment, inDispatchOffset, inDispatch, VDM_V86);

	break;
}
		case REFRESH_SCREEN:
{
	DDraw_RefreshScreen();
	setCF(0);

	break;
}
		case GET_MOUSE_CALLBACK_INFO:
{
	USHORT cond, bstate, col, row;
	GetCallbackInfo(&cond, &bstate, &col, &row);
	setAX(cond); setBX(bstate); setCX(col); setDX(row); setSI(0); setDI(0);
	break;
}
		case MOUSE_RESET_DRIVER:
{
	break;
}
		case MOUSE_SHOW_CURSOR:
{
	ShowMouse();
	break;
}
		case MOUSE_HIDE_CURSOR:
{
	HideMouse();
	break;
}
		case MOUSE_GET_POSITION:
{
	USHORT button, column, row;
	GetMousePosition(&button, &column, &row);
	setBX(button); setCX(column); setDX(row);
	break;
}
		case MOUSE_SET_POSITION:
{
	SetMousePosition(getCX(), getDX());
	break;
}
		case MOUSE_GET_PRESS_DATA:
{
	setAX(0); setBX(0); setCX(0); setDX(0);
	break;
}
		case MOUSE_GET_RELEASE_DATA:
{
	setAX(0); setBX(0); setCX(0); setDX(0);
	break;
}
		case MOUSE_DEFINE_HORIZ_RANGE:
{
	SetHorizontalMouseRange(getCX(), getDX());
	break;
}
		case MOUSE_DEFINE_VERT_RANGE:
{
	SetVerticalMouseRange(getCX(), getDX());
	break;
}
		case MOUSE_DEFINE_GRAPHICS_CURSOR:
{
	break;
}
		case MOUSE_GET_MOTION_COUNTERS:
{
	setCX(0); setDX(0);
	break;
}
		case MOUSE_SET_CALLBACK_PARMS:
{
	SetMouseCallbackMask(getCX());
	break;
}
		case MOUSE_SET_LIGHTPEN_ON:
{
	break;
}
		case MOUSE_SET_LIGHTPEN_OFF:
{
	break;
}
		case MOUSE_SET_MICKEY_RATIO:
{
	break;
}
		case MOUSE_DEFINE_UPDATE_REGION:
{
	break;
}
		default:
{
	LogMessage("Unknown call 0x%x", (int)regAX);
	setCF(1);
	setAX(0x7100);
	break;
}
	}

	return;
}
