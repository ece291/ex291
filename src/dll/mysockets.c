/* mysockets.c -- Sockets pmodelib driver interface
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

   $Id: mysockets.c,v 1.2 2001/04/10 09:06:45 pete Exp $
*/

#include "ex291srv.h"
#include "deque.h"

USHORT programDataSel;
PBYTE programData = 0;
USHORT programStackSel;
PBYTE programStack = 0;
PMODELIB_SOCKINITDATA *SocketSettings = 0;

static queue socketSocketQueue, socketEventQueue;
static HANDLE socketMutex;
static BOOL SocketsReady = FALSE;
static BOOL SocketsCallbacksEnabled = FALSE;
static INT SocketIRQpic;
static BYTE SocketIRQline;

BOOL InitMySockets(unsigned int InitDataOff)
{
    WORD VersionRequested = MAKEWORD(1, 1);	// request WinSock 1.1
    WSADATA data;

    if(SocketsReady)
	return FALSE;

    socketMutex = CreateMutex(NULL, TRUE, NULL);
    if(!socketMutex)
	return FALSE;
    Q_Init(&socketSocketQueue);
    Q_Init(&socketEventQueue);
    ReleaseMutex(socketMutex);

    if(WSAStartup(VersionRequested, &data) != 0) {
	return FALSE;
    }

    // does this WinSock support version 1.1?
    if(LOBYTE(data.wVersion) != 1 || HIBYTE(data.wVersion) != 1) {
	WSACleanup();
	return FALSE;
    }

    programDataSel = getDS();
    programData = (PBYTE) VdmMapFlat(programDataSel, 0, VDM_PM);
    programStackSel = getSS();
    programStack = (PBYTE) VdmMapFlat(programStackSel, 0, VDM_PM);

    SocketSettings = (PMODELIB_SOCKINITDATA *)(programData+InitDataOff);
    SocketSettings->LastError = (int *)(programData+
	(unsigned int)SocketSettings->LastError);
    SocketSettings->NetAddr_static = programData+
	(unsigned int)SocketSettings->NetAddr_static;
    SocketSettings->HostEnt_static = (PMODELIB_HOSTENT *)(programData+
	(unsigned int)SocketSettings->HostEnt_static);

    SocketsReady = TRUE;
    return TRUE;
}

VOID CloseMySockets(VOID)
{
    if(!SocketsReady)
	return;

    WSACleanup();

    if(SocketsCallbacksEnabled)
	SocketsEnableCallbacks(0, FALSE);

    switch(WaitForSingleObject(socketMutex, 500L)) {
	case WAIT_OBJECT_0:
	    while(Q_PopTail(&socketSocketQueue)) {}
	    while(Q_PopTail(&socketEventQueue)) {}
	    ReleaseMutex(socketMutex);
	    break;
    }
    CloseHandle(socketMutex);

    SocketSettings = 0;
    VdmUnmapFlat(programDataSel, 0, programData, VDM_PM);
    programData = 0;
    VdmUnmapFlat(programStackSel, 0, programStack, VDM_PM);
    programStack = 0;

    SocketsReady = FALSE;
}

VOID SocketsEnableCallbacks(BYTE Int, BOOL enable)
{
    if(!SocketsReady)
	return;
    SocketsCallbacksEnabled = enable;
    if(enable) {
	if(Int < 0x50) {
	    SocketIRQpic = ICA_MASTER;
	    SocketIRQline = (INT)Int-0x08;
	} else {
	    SocketIRQpic = ICA_SLAVE;
	    SocketIRQline = (INT)Int-0x70;
	}
    }
}

VOID DoSocketsCallback(UINT socket, LONG event)
{
    if(!SocketsReady || !SocketsCallbacksEnabled)
	return;

    switch(WaitForSingleObject(socketMutex, 50L)) {
	case WAIT_OBJECT_0:
	    Q_PushHead(&socketSocketQueue, (void *)socket);
	    Q_PushHead(&socketEventQueue, (void *)event);
	    ReleaseMutex(socketMutex);
	    //LogMessage("trigger socket IRQ");
	    VDDSimulateInterrupt(SocketIRQpic, SocketIRQline, 1);
	    //LogMessage("end trigger socket IRQ");

	    break;
	}
}

VOID SocketsGetCallbackInfo(VOID)
{
    if(!SocketsReady || !SocketsCallbacksEnabled) {
	setEAX(1);
	return;
    }

    switch(WaitForSingleObject(socketMutex, INFINITE)) {
	case WAIT_OBJECT_0:
	    if(Q_Empty(&socketSocketQueue)) {
		setEAX(1);
	    } else {
		unsigned int val = (unsigned int)Q_PopTail(&socketEventQueue);
		unsigned int retval = 0;
		setECX((unsigned int)Q_PopTail(&socketSocketQueue));
		if(val & FD_READ)
		    retval |= 0x01;
		if(val & FD_WRITE)
		    retval |= 0x02;
		if(val & FD_OOB)
		    retval |= 0x04;
		if(val & FD_ACCEPT)
		    retval |= 0x08;
		if(val & FD_CONNECT)
		    retval |= 0x10;
		if(val & FD_CLOSE)
		    retval |= 0x20;
		setEDX(retval);
	    }
	    ReleaseMutex(socketMutex);
	    setEAX(0);
	    break;
	case WAIT_TIMEOUT:
	case WAIT_ABANDONED:
	    setEAX(1);
    }
}

