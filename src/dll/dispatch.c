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

   $Id: dispatch.c,v 1.8 2001/04/03 04:17:36 pete Exp $
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

static USHORT programDataSel;
static PBYTE programData = 0;
static USHORT programStackSel;
static PBYTE programStack = 0;
static int *SocketLastError = 0;
static PMODELIB_SOCKINITDATA *SocketSettings;

VOID Extra291Dispatch(VOID)
{
    WORD regAX = HIWORD(getEAX());

    switch (regAX) {
	case SOCKET_INIT:
{
    programDataSel = getDS();
    programData = (PBYTE) VdmMapFlat(programDataSel, 0, VDM_PM);
    programStackSel = getSS();
    programStack = (PBYTE) VdmMapFlat(programStackSel, 0, VDM_PM);

    SocketSettings = (PMODELIB_SOCKINITDATA *)(programData+getECX());
    SocketSettings->LastError = (int *)(programData+
	(unsigned int)SocketSettings->LastError);
    SocketSettings->NetAddr_static = programData+
	(unsigned int)SocketSettings->NetAddr_static;
    SocketSettings->HostEnt_static = (PMODELIB_HOSTENT *)(programData+
	(unsigned int)SocketSettings->HostEnt_static);
    break;
}
	case SOCKET_EXIT:
{
    VdmUnmapFlat(programDataSel, 0, programData, VDM_PM);
    programData = 0;
    VdmUnmapFlat(programStackSel, 0, programStack, VDM_PM);
    programStack = 0;
    break;
}
	case SOCKET_ACCEPT:
{
    unsigned int *SocketSP = (unsigned int *)(programStack + getEBP() + 8);
    unsigned int *pNameSP = (unsigned int *)(programStack + getEBP() + 12);
    unsigned int *pNameLenSP = (unsigned int *)(programStack + getEBP() + 16);

    unsigned int Socket = *SocketSP;
    PMODELIB_SOCKADDR *pName = (PMODELIB_SOCKADDR *)(programData + *pNameSP);
    int *pNameLen = (int *)(programData + *pNameLenSP);

    struct sockaddr_in in_Name;
    unsigned int retval;

    in_Name.sin_family = AF_INET;
    in_Name.sin_port = pName->Port;
    in_Name.sin_addr.s_addr = pName->Address;

    retval = accept(Socket, (struct sockaddr *)&in_Name, pNameLen);

    if(retval == INVALID_SOCKET) {
	setEAX(0xFFFFFFFF);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(retval);
	pName->Port = in_Name.sin_port;
	pName->Address = in_Name.sin_addr.s_addr;
    }

    break;
}
	case SOCKET_BIND:
{
    unsigned int *SocketSP = (unsigned int *)(programStack + getEBP() + 8);
    unsigned int *pNameSP = (unsigned int *)(programStack + getEBP() + 12);
    int *NameLenSP = (int *)(programStack + getEBP() + 16);

    unsigned int Socket = *SocketSP;
    PMODELIB_SOCKADDR *pName = (PMODELIB_SOCKADDR *)(programData + *pNameSP);
    int NameLen = *NameLenSP;

    struct sockaddr_in in_Name;
    int retval;

    in_Name.sin_family = AF_INET;
    in_Name.sin_port = pName->Port;
    in_Name.sin_addr.s_addr = pName->Address;

    retval = bind(Socket, (const struct sockaddr *)&in_Name, NameLen);

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
    }

    break;
}
	case SOCKET_CLOSE:
{
    unsigned int *SocketSP = (unsigned int *)(programStack + getEBP() + 8);

    unsigned int Socket = *SocketSP;

    int retval = closesocket(Socket);

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
    }

    break;
}
	case SOCKET_CONNECT:
{
    unsigned int *SocketSP = (unsigned int *)(programStack + getEBP() + 8);
    unsigned int *pNameSP = (unsigned int *)(programStack + getEBP() + 12);
    int *NameLenSP = (int *)(programStack + getEBP() + 16);

    unsigned int Socket = *SocketSP;
    PMODELIB_SOCKADDR *pName = (PMODELIB_SOCKADDR *)(programData + *pNameSP);
    int NameLen = *NameLenSP;

    struct sockaddr_in in_Name;
    int retval;

    in_Name.sin_family = AF_INET;
    in_Name.sin_port = pName->Port;
    in_Name.sin_addr.s_addr = pName->Address;

    retval = connect(Socket, (const struct sockaddr *)&in_Name, NameLen);

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
    }

    break;
}
	case SOCKET_GETPEERNAME:
{
    unsigned int *SocketSP = (unsigned int *)(programStack + getEBP() + 8);
    unsigned int *pNameSP = (unsigned int *)(programStack + getEBP() + 12);
    unsigned int *pNameLenSP = (unsigned int *)(programStack + getEBP() + 16);

    unsigned int Socket = *SocketSP;
    PMODELIB_SOCKADDR *pName = (PMODELIB_SOCKADDR *)(programData + *pNameSP);
    int *pNameLen = (int *)(programData + *pNameLenSP);

    struct sockaddr_in in_Name;
    int retval;

    in_Name.sin_family = AF_INET;
    in_Name.sin_port = pName->Port;
    in_Name.sin_addr.s_addr = pName->Address;

    retval = getpeername(Socket, (struct sockaddr *)&in_Name, pNameLen);

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
	pName->Port = in_Name.sin_port;
	pName->Address = in_Name.sin_addr.s_addr;
    }

    break;
}
	case SOCKET_GETSOCKNAME:
{
    unsigned int *SocketSP = (unsigned int *)(programStack + getEBP() + 8);
    unsigned int *pNameSP = (unsigned int *)(programStack + getEBP() + 12);
    unsigned int *pNameLenSP = (unsigned int *)(programStack + getEBP() + 16);

    unsigned int Socket = *SocketSP;
    PMODELIB_SOCKADDR *pName = (PMODELIB_SOCKADDR *)(programData + *pNameSP);
    int *pNameLen = (int *)(programData + *pNameLenSP);

    struct sockaddr_in in_Name;
    int retval;

    in_Name.sin_family = AF_INET;
    in_Name.sin_port = pName->Port;
    in_Name.sin_addr.s_addr = pName->Address;

    retval = getsockname(Socket, (struct sockaddr *)&in_Name, pNameLen);

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
	pName->Port = in_Name.sin_port;
	pName->Address = in_Name.sin_addr.s_addr;
    }

    break;
}
	case SOCKET_INETADDR:
{
    unsigned int *pDottedAddrSP = (unsigned int *)(programStack + getEBP() + 8);

    char *pDottedAddr = (char *)(programData + *pDottedAddrSP);

    unsigned int retval = inet_addr(pDottedAddr);

    if(retval == INVALID_SOCKET) {
	setEAX(0xFFFFFFFF);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(retval);
    }

    break;
}
	case SOCKET_INETNTOA:
{
    unsigned int *AddressSP = (unsigned int *)(programStack + getEBP() + 8);

    unsigned int Address = *AddressSP;

    struct in_addr in;
    char *retval;

    in.s_addr = Address;

    retval = inet_ntoa(in);

    strncpy(SocketSettings->NetAddr_static, retval,
	SocketSettings->STRING_MAX);
    SocketSettings->NetAddr_static[SocketSettings->STRING_MAX-1] = 0;

    break;
}
	case SOCKET_LISTEN:
{
    unsigned int *SocketSP = (unsigned int *)(programStack + getEBP() + 8);
    int *BackLogSP = (int *)(programStack + getEBP() + 12);

    unsigned int Socket = *SocketSP;
    int BackLog = *BackLogSP;

    int retval = listen(Socket, BackLog);

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
    }

    break;
}
	case SOCKET_RECV:
{
    unsigned int *SocketSP = (unsigned int *)(programStack + getEBP() + 8);
    unsigned int *pBufSP = (unsigned int *)(programStack + getEBP() + 12);
    int *MaxLenSP = (int *)(programStack + getEBP() + 16);
    unsigned int *FlagsSP = (unsigned int *)(programStack + getEBP() + 20);

    unsigned int Socket = *SocketSP;
    unsigned char *pBuf = (unsigned char *)(programData + *pBufSP);
    int MaxLen = *MaxLenSP;
    unsigned int Flags = *FlagsSP;

    int actflags = 0;
    int retval;

    if(Flags & 0x01)
	actflags |= MSG_PEEK;
    if(Flags & 0x02)
	actflags |= MSG_OOB;

    retval = recv(Socket, pBuf, MaxLen, actflags);

    if(retval < 0) {
	setEAX(0xFFFFFFFF);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(retval);
    }

    break;
}
	case SOCKET_RECVFROM:
{
    unsigned int *SocketSP = (unsigned int *)(programStack + getEBP() + 8);
    unsigned int *pBufSP = (unsigned int *)(programStack + getEBP() + 12);
    int *MaxLenSP = (int *)(programStack + getEBP() + 16);
    unsigned int *FlagsSP = (unsigned int *)(programStack + getEBP() + 20);
    unsigned int *pFromSP = (unsigned int *)(programStack + getEBP() + 24);
    unsigned int *pFromLenSP = (unsigned int *)(programStack + getEBP() + 28);

    unsigned int Socket = *SocketSP;
    unsigned char *pBuf = (unsigned char *)(programData + *pBufSP);
    int MaxLen = *MaxLenSP;
    unsigned int Flags = *FlagsSP;
    PMODELIB_SOCKADDR *pFrom = (PMODELIB_SOCKADDR *)(programData + *pFromSP);
    int *pFromLen = (int *)(programData + *pFromLenSP);

    int actflags = 0;
    struct sockaddr_in in_From;
    int retval;

    if(Flags & 0x01)
	actflags |= MSG_PEEK;
    if(Flags & 0x02)
	actflags |= MSG_OOB;

    in_From.sin_family = AF_INET;
    in_From.sin_port = pFrom->Port;
    in_From.sin_addr.s_addr = pFrom->Address;

    retval = recvfrom(Socket, pBuf, MaxLen, actflags,
	(struct sockaddr *)&in_From, pFromLen);

    if(retval < 0) {
	setEAX(0xFFFFFFFF);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(retval);
	pFrom->Port = in_From.sin_port;
	pFrom->Address = in_From.sin_addr.s_addr;
    }

    break;
}
	case SOCKET_SEND:
{
    unsigned int *SocketSP = (unsigned int *)(programStack + getEBP() + 8);
    unsigned int *pBufSP = (unsigned int *)(programStack + getEBP() + 12);
    int *LenSP = (int *)(programStack + getEBP() + 16);
    unsigned int *FlagsSP = (unsigned int *)(programStack + getEBP() + 20);

    unsigned int Socket = *SocketSP;
    unsigned char *pBuf = (unsigned char *)(programData + *pBufSP);
    int Len = *LenSP;
    unsigned int Flags = *FlagsSP;

    int actflags = 0;
    int retval;

    if(Flags & 0x01)
	actflags |= MSG_OOB;

    retval = send(Socket, pBuf, Len, actflags);

    if(retval < 0) {
	setEAX(0xFFFFFFFF);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(retval);
    }

    break;
}
	case SOCKET_SENDTO:
{
    unsigned int *SocketSP = (unsigned int *)(programStack + getEBP() + 8);
    unsigned int *pBufSP = (unsigned int *)(programStack + getEBP() + 12);
    int *LenSP = (int *)(programStack + getEBP() + 16);
    unsigned int *FlagsSP = (unsigned int *)(programStack + getEBP() + 20);
    unsigned int *pToSP = (unsigned int *)(programStack + getEBP() + 24);
    int *ToLenSP = (int *)(programStack + getEBP() + 28);

    unsigned int Socket = *SocketSP;
    unsigned char *pBuf = (unsigned char *)(programData + *pBufSP);
    int Len = *LenSP;
    unsigned int Flags = *FlagsSP;
    PMODELIB_SOCKADDR *pTo = (PMODELIB_SOCKADDR *)(programData + *pToSP);
    int ToLen = *ToLenSP;

    int actflags = 0;
    struct sockaddr_in in_To;
    int retval;

    if(Flags & 0x01)
	actflags |= MSG_OOB;

    in_To.sin_family = AF_INET;
    in_To.sin_port = pTo->Port;
    in_To.sin_addr.s_addr = pTo->Address;

    retval = sendto(Socket, pBuf, Len, actflags,
	(const struct sockaddr *)&in_To, ToLen);

    if(retval < 0) {
	setEAX(0xFFFFFFFF);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(retval);
    }

    break;
}
	case SOCKET_SHUTDOWN:
{
    unsigned int *SocketSP = (unsigned int *)(programStack + getEBP() + 8);
    unsigned int *FlagsSP = (unsigned int *)(programStack + getEBP() + 12);

    unsigned int Socket = *SocketSP;
    unsigned int Flags = *FlagsSP;

    int actflags = Flags - 1;

    int retval = shutdown(Socket, actflags);

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
    }

    break;
}
	case SOCKET_CREATE:
{
    unsigned int *TypeSP = (unsigned int *)(programStack + getEBP() + 8);

    unsigned int Type = *TypeSP;

    int acttype = 0;
    unsigned int retval;

    switch(Type) {
	case 1:
	    acttype = SOCK_STREAM;
	    break;
	case 2:
	    acttype = SOCK_DGRAM;
	    break;
    }

    retval = socket(AF_INET, acttype, IPPROTO_IP);

    if(retval == INVALID_SOCKET) {
	setEAX(0xFFFFFFFF);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(retval);
    }

    break;
}
	case SOCKET_GETHOSTBYADDR:
{
    setEAX(0);
    break;
}
	case SOCKET_GETHOSTBYNAME:
{
    setEAX(0);
    break;
}
	case SOCKET_GETHOSTNAME:
{
    unsigned int *pNameSP = (unsigned int *)(programStack + getEBP() + 8);
    int *NameLenSP = (int *)(programStack + getEBP() + 12);

    char *pName = (char *)(programData + *pNameSP);
    int NameLen = *NameLenSP;

    int retval = gethostname(pName, NameLen);

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
    }

    break;
}
	case VBEAF_GET_MEMORY:
{
    USHORT data_sel = getFS();
    ULONG data_off = getESI();
    DISPATCH_DATA *data = (DISPATCH_DATA *) VdmMapFlat(data_sel, data_off,
	VDM_PM);
    data->i[0] = DDraw_GetFreeMemory();
    if(data->i[0] == 0xFFFFFFFF || data->i[0] == 0xFFFEFFFF) {
	setCF(1);
	data->i[0] = 0;
    } else {
	setCF(0);
	data->i[0] >>= 10;	// Return in K
    }
    VdmUnmapFlat(data_sel, data_off, data, VDM_PM);
    break;
}
	case VBEAF_GET_MODELIST:
{
    USHORT data_sel = getFS();
    ULONG data_off = getESI();
    DISPATCH_DATA *data = (DISPATCH_DATA *) VdmMapFlat(data_sel, data_off,
	VDM_PM);
    if(!DDraw_GetModelist(data->seg[0], (WORD)data->off[0], data))
	setCF(1);
    else
	setCF(0);
    VdmUnmapFlat(data_sel, data_off, data, VDM_PM);
    break;
}
	case VBEAF_SET_MODE:
{
    USHORT data_sel = getFS();
    ULONG data_off = getESI();
    DISPATCH_DATA *data = (DISPATCH_DATA *) VdmMapFlat(data_sel, data_off,
	VDM_PM);
    setCF(0);

    VBEAF_width = data->i[0];
    VBEAF_height = data->i[1];
    VBEAF_depth = data->i[2];
    usingVBEAF = TRUE;
    VdmUnmapFlat(data_sel, data_off, data, VDM_PM);
    break;
}
	case VBEAF_SET_PALETTE:
{
    setCF(1);
    break;
}
	case VBEAF_BITBLT_VIDEO:
{
    setCF(1);
    break;
}
	case VBEAF_BITBLT_SYS:
{
    USHORT data_sel = getFS();
    ULONG data_off = getESI();
    DISPATCH_DATA *data = (DISPATCH_DATA *) VdmMapFlat(data_sel, data_off,
    	VDM_PM);
    PVOID source = VdmMapFlat(data->seg[0], data->off[0], VDM_PM);

    if(DDraw_BitBltSys(source, data))
	setCF(1);

    VdmUnmapFlat(data->seg[0], data->off[0], source, VDM_PM);
    VdmUnmapFlat(data_sel, data_off, data, VDM_PM);
    break;
}
	case VBEAF_SET_CURSOR_SHAPE:
{
    setCF(1);
    break;
}
	case VBEAF_SET_CURSOR_POS:
{
    USHORT data_sel = getFS();
    ULONG data_off = getESI();
    DISPATCH_DATA *data = (DISPATCH_DATA *) VdmMapFlat(data_sel, data_off,
    	VDM_PM);

    SetMousePosition((USHORT)data->i[0], (USHORT)data->i[1]);

    VdmUnmapFlat(data_sel, data_off, data, VDM_PM);
    break;
}
	case VBEAF_SHOW_CURSOR:
{
    USHORT data_sel = getFS();
    ULONG data_off = getESI();
    DISPATCH_DATA *data = (DISPATCH_DATA *) VdmMapFlat(data_sel, data_off,
    	VDM_PM);

    if(data->i[0])
	ShowMouse();
    else
	HideMouse();

    VdmUnmapFlat(data_sel, data_off, data, VDM_PM);
    break;
}
	case GET_MEMORY:
{
    setCF(0);
    setAX((WORD)(DDraw_GetFreeMemory()>>16));	// Return in 64K
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
	break;
    }
    if(!InitMouse()) {
	MessageBox(NULL, "Could not initialize mouse driver.",
	    "Extra BIOS Services for ECE 291", MB_OK | MB_ICONERROR);
	setCF(1);
	setAX(0xFFFF);
	CloseKey();
	VdmUnmapFlat(inDispatchSegment, inDispatchOffset, inDispatch, VDM_V86);
	break;
    }

    if(!usingVBEAF) {
	VBEAF_width = (int)HIWORD(getECX());
	VBEAF_height = (int)LOWORD(getECX());
    }

    HideMouse();

    if(!InitMyWindow(GetInstance(), VBEAF_width, VBEAF_height)) {
	MessageBox(NULL, "Could not initialize output window.",
	    "Extra BIOS Services for ECE 291", MB_OK | MB_ICONERROR);
	setCF(1);
	setAX(0xFFFF);
	CloseMouse();
	CloseKey();
	VdmUnmapFlat(inDispatchSegment, inDispatchOffset, inDispatch, VDM_V86);
	break;
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

    break;
}
	case UNSET_MODE:
{
    setCF(0);
    setAX(DDraw_UnSetMode());
    CloseMyWindow();
    CloseMouse();
    CloseKey();

    ShowMouse();

    if(!usingVBEAF)
	VdmUnmapFlat(displaySegment, displayOffset, displayBuffer, displayMode);
    VdmUnmapFlat(inDispatchSegment, inDispatchOffset, inDispatch, VDM_V86);
    usingVBEAF = FALSE;

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

