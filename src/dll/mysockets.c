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

   $Id: mysockets.c,v 1.1 2001/04/03 04:57:41 pete Exp $
*/

#include "ex291srv.h"

USHORT programDataSel;
PBYTE programData = 0;
USHORT programStackSel;
PBYTE programStack = 0;
PMODELIB_SOCKINITDATA *SocketSettings = 0;

static BOOL SocketsReady = FALSE;

BOOL InitMySockets(unsigned int InitDataOff)
{
    WORD VersionRequested = MAKEWORD(1, 1);	// request WinSock 1.1
    WSADATA data;

    if(SocketsReady)
	return FALSE;

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

    SocketSettings = 0;
    VdmUnmapFlat(programDataSel, 0, programData, VDM_PM);
    programData = 0;
    VdmUnmapFlat(programStackSel, 0, programStack, VDM_PM);
    programStack = 0;

    SocketsReady = FALSE;
}

