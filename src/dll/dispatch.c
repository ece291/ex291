/* dispatch.c -- main dispatch routine
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

   $Id: dispatch.c,v 1.14 2001/12/12 18:28:06 pete Exp $
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
PBYTE Mouse_IRQ;
PBYTE Keyboard_IRQ;
PWORD Keyboard_Port;

BOOL usingVBEAF = FALSE;
int VBEAF_width = 0;
int VBEAF_height = 0;
int VBEAF_depth = 0;

extern PBYTE programStack;

static VOID VBEAF_SetMode(DISPATCH_DATA *);
static VOID Legacy_SetMode(VOID);
static VOID Legacy_UnsetMode(VOID);

#define SOCKET_INIT				0x5001
#define Socket_FUNCTIONS			0x5002
typedef VOID (__cdecl *pSocketFunc)(UINT, UINT, UINT, UINT, UINT);
static pSocketFunc SocketFunctionMap[] = {
    (pSocketFunc)&CloseMySockets,		// 0x5002
    (pSocketFunc)&Socket_accept,		// 0x5003
    (pSocketFunc)&Socket_bind,			// 0x5004
    (pSocketFunc)&Socket_close,			// 0x5005
    (pSocketFunc)&Socket_connect,		// 0x5006
    (pSocketFunc)&Socket_getpeername,		// 0x5007
    (pSocketFunc)&Socket_getsockname,		// 0x5008
    (pSocketFunc)&Socket_inetaddr,		// 0x5009
    (pSocketFunc)&Socket_inetntoa,		// 0x500A
    (pSocketFunc)&Socket_listen,		// 0x500B
    (pSocketFunc)&Socket_recv,			// 0x500C
    (pSocketFunc)&Socket_recvfrom,		// 0x500D
    (pSocketFunc)&Socket_send,			// 0x500E
    (pSocketFunc)&Socket_sendto,		// 0x500F
    (pSocketFunc)&Socket_shutdown,		// 0x5010
    (pSocketFunc)&Socket_create,		// 0x5011
    (pSocketFunc)&Socket_gethostbyaddr,		// 0x5012
    (pSocketFunc)&Socket_gethostbyname,		// 0x5013
    (pSocketFunc)&Socket_gethostname,		// 0x5014
    (pSocketFunc)&Socket_InstallCallback,	// 0x5015
    (pSocketFunc)&Socket_RemoveCallback,	// 0x5016
    (pSocketFunc)&Socket_AddCallback,		// 0x5017
    (pSocketFunc)&Socket_GetCallbackInfo,	// 0x5018
    (pSocketFunc)&Socket_getsockopt,		// 0x5019
    (pSocketFunc)&Socket_setsockopt		// 0x5020
};
#define SOCKET_FUNCTIONS_NUM	(sizeof(SocketFunctionMap)/sizeof(pSocketFunc))

#define VBEAF_FUNCTIONS				0x6001
typedef VOID (*pVBEAFFunc)(DISPATCH_DATA *);
static pVBEAFFunc VBEAFFunctionMap[] = {
    &VBEAF_GetMemory,				// 0x6001
    &VBEAF_GetModelist,				// 0x6002
    &VBEAF_SetMode,				// 0x6003
    &VBEAF_SetPalette,				// 0x6004
    &VBEAF_BitBltVideo,				// 0x6005
    &VBEAF_BitBltSys,				// 0x6006
    &VBEAF_SetCursorShape,			// 0x6007
    &VBEAF_SetCursorPos,			// 0x6008
    &VBEAF_ShowCursor				// 0x6009
};

#define General_FUNCTIONS			0x7001
typedef VOID (*pGeneralFunc)(VOID);
static pGeneralFunc GeneralFunctionMap[] = {
    &Legacy_GetMemory,				// 0x7001
    0,						// 0x7002
    &Legacy_SetMode,				// 0x7003
    &Legacy_UnsetMode,				// 0x7004
    &Legacy_RefreshScreen,			// 0x7005
    &Mouse_GetCallbackInfo			// 0x7006
};

#define Mouse_FUNCTIONS				0x7010
typedef VOID (*pMouseFunc)(VOID);
static pMouseFunc MouseFunctionMap[] = {
    &Mouse_ResetDriver,				// 0x7010
    &Mouse_ShowCursor,				// 0x7011
    &Mouse_HideCursor,				// 0x7012
    &Mouse_GetPosition,				// 0x7013
    &Mouse_SetPosition,				// 0x7014
    &Mouse_GetPressData,			// 0x7015
    &Mouse_GetReleaseData,			// 0x7016
    &Mouse_DefineHorizRange,			// 0x7017
    &Mouse_DefineVertRange,			// 0x7018
    &Mouse_DefineGraphicsCursor,		// 0x7019
    0,						// 0x701A
    &Mouse_GetMotionCounters,			// 0x701B
    &Mouse_SetCallbackParms,			// 0x701C
    &Mouse_SetLightpenOn,			// 0x701D
    &Mouse_SetLightpenOff,			// 0x701E
    &Mouse_SetMickeyRatio,			// 0x701F
    &Mouse_DefineUpdateRegion			// 0x7020
};

#define	TestFunctionMap(off, map)					    \
    ((off >= map##_FUNCTIONS) &&					    \
    (off <= map##_FUNCTIONS+sizeof(map##FunctionMap)/sizeof(p##map##Func)) && \
    map##FunctionMap[off-map##_FUNCTIONS])
#define FunctionMap(off, map)	    map##FunctionMap[off-map##_FUNCTIONS]

VOID Extra291Dispatch(VOID)
{
    WORD regAX = HIWORD(getEAX());

    if(regAX == SOCKET_INIT) {
	if(!InitMySockets(getECX()))
	    setCF(1);
	else
	    setCF(0);
    } else if(TestFunctionMap(regAX, Socket)) {
	// call function with stack parameters
	(*FunctionMap(regAX, Socket))(
	    *((UINT *)(programStack + getEBP() +  8)),
	    *((UINT *)(programStack + getEBP() + 12)),
	    *((UINT *)(programStack + getEBP() + 16)),
	    *((UINT *)(programStack + getEBP() + 20)),
	    *((UINT *)(programStack + getEBP() + 24)));
    } else if(TestFunctionMap(regAX, VBEAF)) {
	// map data parameter
	USHORT data_sel = getFS();
	ULONG data_off = getESI();
	DISPATCH_DATA *data = (DISPATCH_DATA *) VdmMapFlat(data_sel, data_off,
	    VDM_PM);
	// call function
	(*FunctionMap(regAX, VBEAF))(data);
	// unmap data parameter
	VdmUnmapFlat(data_sel, data_off, data, VDM_PM);
    } else if(TestFunctionMap(regAX, General)) {
	(*FunctionMap(regAX, General))();	// do the call
    } else if(TestFunctionMap(regAX, Mouse)) {
	(*FunctionMap(regAX, Mouse))();		// do the call
    } else {
	LogMessage("Unknown call 0x%x", (int)regAX);
	setCF(1);
	setAX(0x7100);
    }
}

static VOID VBEAF_SetMode(DISPATCH_DATA *data)
{
    setCF(0);

    VBEAF_width = data->i[0];
    VBEAF_height = data->i[1];
    VBEAF_depth = data->i[2];
    usingVBEAF = TRUE;
}

static VOID Legacy_SetMode(VOID)
{
    inDispatchSegment = getCS();
    inDispatchOffset = getEBX() & 0xFFFF;

    inDispatch = (PBYTE) VdmMapFlat(inDispatchSegment, inDispatchOffset,
	VDM_V86);
    VDD_int_wait = inDispatch + 1;
    DOS_VDD_Mutex = inDispatch + 2;
    WindowedMode = inDispatch + 3;
    Mouse_IRQ = inDispatch + 4;
    Keyboard_IRQ = inDispatch + 5;
    Keyboard_Port = (PWORD)(inDispatch + 6);

    //LogMessage("Windowed Flag = %d", (int)*WindowedMode);
    //LogMessage("Mouse IRQ = %d", (int)*Mouse_IRQ);
    //LogMessage("Keyboard IRQ = %d", (int)*Keyboard_IRQ);
    //LogMessage("Keyboard Port = %d", (int)*Keyboard_Port);

/*  LogMessage("Segment=0x%x, Offset=0x%x", (unsigned int)inDispatchSegment,
	inDispatchOffset);*/

    if(!InitKey()) {
	MessageBox(NULL, "Could not initialize keyboard driver.",
	    "Extra BIOS Services for ECE 291", MB_OK | MB_ICONERROR);
	setCF(1);
	setAX(0xFFFF);
	VdmUnmapFlat(inDispatchSegment, inDispatchOffset, inDispatch, VDM_V86);
	return;
    }
    if(!InitMouse()) {
	MessageBox(NULL, "Could not initialize mouse driver.",
	    "Extra BIOS Services for ECE 291", MB_OK | MB_ICONERROR);
	setCF(1);
	setAX(0xFFFF);
	CloseKey();
	VdmUnmapFlat(inDispatchSegment, inDispatchOffset, inDispatch, VDM_V86);
	return;
    }

    if(!usingVBEAF) {
	VBEAF_width = (int)HIWORD(getECX());
	VBEAF_height = (int)LOWORD(getECX());
    }

    Mouse_HideCursor();

    if(!InitMyWindow(GetInstance(), VBEAF_width, VBEAF_height)) {
	MessageBox(NULL, "Could not initialize output window.",
	    "Extra BIOS Services for ECE 291", MB_OK | MB_ICONERROR);
	setCF(1);
	setAX(0xFFFF);
	CloseMouse();
	CloseKey();
	VdmUnmapFlat(inDispatchSegment, inDispatchOffset, inDispatch, VDM_V86);
	return;
    }

    if(usingVBEAF) {
	if(DDraw_SetMode(VBEAF_width, VBEAF_height, VBEAF_depth))
	    setCF(1);
    } else {
	displaySegment = getDX();
	displayOffset = getEDI();
	displayMode = VDM_PM;

	displayBuffer = VdmMapFlat(displaySegment, displayOffset, displayMode);

	setCF(0);
	setAX(DDraw_SetMode_Old((int)HIWORD(getECX()), (int)LOWORD(getECX()),
	    displayBuffer));
    }
}

static VOID Legacy_UnsetMode(VOID)
{
    setCF(0);
    setAX(DDraw_UnSetMode());
    CloseMyWindow();
    CloseMouse();
    CloseKey();

    Mouse_ShowCursor();

    if(!usingVBEAF)
	VdmUnmapFlat(displaySegment, displayOffset, displayBuffer, displayMode);
    VdmUnmapFlat(inDispatchSegment, inDispatchOffset, inDispatch, VDM_V86);
    usingVBEAF = FALSE;
}

