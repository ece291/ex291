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

   $Id: mysockets.c,v 1.4.2.2 2001/04/24 19:05:24 pete Exp $
*/

#include "ex291srv.h"
#include "deque.h"

static USHORT programDataSel;
static PBYTE programData = 0;
static USHORT programStackSel;
PBYTE programStack = 0;		// needs to be global for dispatch.c to access
static PMODELIB_SOCKINITDATA *SocketSettings = 0;

static queue socketSocketQueue, socketEventQueue;
static HANDLE socketMutex;
static BOOL SocketsReady = FALSE;
static BOOL SocketsCallbacksEnabled = FALSE;
static INT SocketIRQpic;
static BYTE SocketIRQline;

static VOID CopyHOSTENT(struct hostent *);

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
    SocketSettings->HostEnt_Name_static = programData+
	(unsigned int)SocketSettings->HostEnt_Name_static;
    SocketSettings->HostEnt_Aliases_static = (unsigned int *)(programData+
	(unsigned int)SocketSettings->HostEnt_Aliases_static);
    SocketSettings->HostEnt_AddrList_static = (unsigned int *)(programData+
	(unsigned int)SocketSettings->HostEnt_AddrList_static);
    SocketSettings->HostEnt_Aliases_data = programData+
	(unsigned int)SocketSettings->HostEnt_Aliases_data;
    SocketSettings->HostEnt_AddrList_data = (unsigned int *)(programData+
	(unsigned int)SocketSettings->HostEnt_AddrList_data);

    SocketsReady = TRUE;
    return TRUE;
}

VOID __cdecl CloseMySockets(VOID)
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

VOID __cdecl Socket_accept(UINT Socket, UINT pNameOff)
{
    PMODELIB_SOCKADDR *pName = (PMODELIB_SOCKADDR *)(programData + pNameOff);

    struct sockaddr_in in_Name;
    unsigned int retval = sizeof(struct sockaddr_in);

    in_Name.sin_family = AF_INET;
    in_Name.sin_port = pName->Port;
    in_Name.sin_addr.s_addr = pName->Address;

    retval = accept(Socket, (struct sockaddr *)&in_Name, &retval);

    if(retval == INVALID_SOCKET) {
	setEAX(0xFFFFFFFF);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(retval);
	if(pName) {
	    pName->Port = in_Name.sin_port;
	    pName->Address = in_Name.sin_addr.s_addr;
	}
    }
}

VOID __cdecl Socket_bind(UINT Socket, UINT pNameOff)
{
    PMODELIB_SOCKADDR *pName = (PMODELIB_SOCKADDR *)(programData + pNameOff);

    struct sockaddr_in in_Name;
    int retval;

    in_Name.sin_family = AF_INET;
    in_Name.sin_port = pName->Port;
    in_Name.sin_addr.s_addr = pName->Address;

    retval = bind(Socket, (const struct sockaddr *)&in_Name,
	sizeof(struct sockaddr_in));

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
    }
}

VOID __cdecl Socket_close(UINT Socket)
{
    int retval = closesocket(Socket);

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
    }
}

VOID __cdecl Socket_connect(UINT Socket, UINT pNameOff)
{
    PMODELIB_SOCKADDR *pName = (PMODELIB_SOCKADDR *)(programData + pNameOff);

    struct sockaddr_in in_Name;
    int retval;

    in_Name.sin_family = AF_INET;
    in_Name.sin_port = pName->Port;
    in_Name.sin_addr.s_addr = pName->Address;

    retval = connect(Socket, (const struct sockaddr *)&in_Name,
	sizeof(struct sockaddr_in));

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
    }
}

VOID __cdecl Socket_getpeername(UINT Socket, UINT pNameOff)
{
    PMODELIB_SOCKADDR *pName = (PMODELIB_SOCKADDR *)(programData + pNameOff);

    struct sockaddr_in in_Name;
    int retval = sizeof(struct sockaddr_in);

    in_Name.sin_family = AF_INET;
    in_Name.sin_port = pName->Port;
    in_Name.sin_addr.s_addr = pName->Address;

    retval = getpeername(Socket, (struct sockaddr *)&in_Name, &retval);

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
	if(pName) {
	    pName->Port = in_Name.sin_port;
	    pName->Address = in_Name.sin_addr.s_addr;
	}
    }
}

VOID __cdecl Socket_getsockname(UINT Socket, UINT pNameOff)
{
    PMODELIB_SOCKADDR *pName = (PMODELIB_SOCKADDR *)(programData + pNameOff);

    struct sockaddr_in in_Name;
    int retval = sizeof(struct sockaddr_in);

    in_Name.sin_family = AF_INET;
    in_Name.sin_port = pName->Port;
    in_Name.sin_addr.s_addr = pName->Address;

    retval = getsockname(Socket, (struct sockaddr *)&in_Name, &retval);

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
	if(pName) {
	    pName->Port = in_Name.sin_port;
	    pName->Address = in_Name.sin_addr.s_addr;
	}
    }
}

VOID __cdecl Socket_inetaddr(UINT pDottedAddrOff)
{
    char *pDottedAddr = (char *)(programData + pDottedAddrOff);

    unsigned int retval = inet_addr(pDottedAddr);

    if(retval == INVALID_SOCKET) {
	setEAX(0xFFFFFFFF);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(retval);
    }
}

VOID __cdecl Socket_inetntoa(UINT Address)
{
    struct in_addr in;
    char *retval;

    in.s_addr = Address;

    retval = inet_ntoa(in);

    strncpy(SocketSettings->NetAddr_static, retval,
	SocketSettings->STRING_MAX);
    SocketSettings->NetAddr_static[SocketSettings->STRING_MAX-1] = 0;
}

VOID __cdecl Socket_listen(UINT Socket, int BackLog)
{
    int retval = listen(Socket, BackLog);

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
    }
}

VOID __cdecl Socket_recv(UINT Socket, UINT pBufOff, int MaxLen, UINT Flags)
{
    unsigned char *pBuf = (unsigned char *)(programData + pBufOff);

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
}

VOID __cdecl Socket_recvfrom(UINT Socket, UINT pBufOff, int MaxLen, UINT Flags,
    UINT pFromOff)
{
    unsigned char *pBuf = (unsigned char *)(programData + pBufOff);
    PMODELIB_SOCKADDR *pFrom = (PMODELIB_SOCKADDR *)(programData + pFromOff);

    int actflags = 0;
    struct sockaddr_in in_From;
    int retval = sizeof(struct sockaddr_in);

    if(Flags & 0x01)
	actflags |= MSG_PEEK;
    if(Flags & 0x02)
	actflags |= MSG_OOB;

    in_From.sin_family = AF_INET;
    in_From.sin_port = pFrom->Port;
    in_From.sin_addr.s_addr = pFrom->Address;

    retval = recvfrom(Socket, pBuf, MaxLen, actflags,
	(struct sockaddr *)&in_From, &retval);

    if(retval < 0) {
	setEAX(0xFFFFFFFF);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(retval);
	if(pFrom) {
	    pFrom->Port = in_From.sin_port;
	    pFrom->Address = in_From.sin_addr.s_addr;
	}
    }
}

VOID __cdecl Socket_send(UINT Socket, UINT pBufOff, int Len, UINT Flags)
{
    unsigned char *pBuf = (unsigned char *)(programData + pBufOff);

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
}

VOID __cdecl Socket_sendto(UINT Socket, UINT pBufOff, int Len, UINT Flags,
    UINT pToOff)
{
    unsigned char *pBuf = (unsigned char *)(programData + pBufOff);
    PMODELIB_SOCKADDR *pTo = (PMODELIB_SOCKADDR *)(programData + pToOff);

    int actflags = 0;
    struct sockaddr_in in_To;
    int retval;

    if(Flags & 0x01)
	actflags |= MSG_OOB;

    in_To.sin_family = AF_INET;
    in_To.sin_port = pTo->Port;
    in_To.sin_addr.s_addr = pTo->Address;

    retval = sendto(Socket, pBuf, Len, actflags,
	(const struct sockaddr *)&in_To, sizeof(struct sockaddr_in));

    if(retval < 0) {
	setEAX(0xFFFFFFFF);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(retval);
    }
}

VOID __cdecl Socket_shutdown(UINT Socket, UINT Flags)
{
    int actflags = Flags - 1;

    int retval = shutdown(Socket, actflags);

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
    }
}

VOID __cdecl Socket_create(UINT Type)
{
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
}

static VOID CopyHOSTENT(struct hostent *retval)
{
    unsigned int i;

    // copy official name
    strncpy(SocketSettings->HostEnt_Name_static, retval->h_name,
	SocketSettings->STRING_MAX);
    SocketSettings->HostEnt_Name_static[SocketSettings->STRING_MAX-1] = 0;

    // loop over all aliases
    for(i=0; retval->h_aliases[i] &&
	(i<SocketSettings->HOSTENT_ALIASES_MAX); i++) {
	// copy string to program list
	strncpy(SocketSettings->HostEnt_Aliases_data+
	    i*SocketSettings->STRING_MAX, retval->h_aliases[i],
	    SocketSettings->STRING_MAX);
	SocketSettings->HostEnt_Aliases_data[i*SocketSettings->STRING_MAX+
	    SocketSettings->STRING_MAX-1] = 0;
	// add pointer to string to program aliases list
	SocketSettings->HostEnt_Aliases_static[i] = (char *)
	    (SocketSettings->HostEnt_Aliases_data+
	    i*SocketSettings->STRING_MAX)-programData;
    }
    // zero-terminate program alias list
    SocketSettings->HostEnt_Aliases_static[i] = 0;

    // loop over all addresses
    for(i=0; retval->h_addr_list[i] &&
	(i<SocketSettings->HOSTENT_ADDRLIST_MAX); i++) {
	// copy address to program list
	SocketSettings->HostEnt_AddrList_data[i] =
	    *((unsigned int *)retval->h_addr_list[i]);
	// add pointer to address to program address list
	SocketSettings->HostEnt_AddrList_static[i] = (char *)
	    (&SocketSettings->HostEnt_AddrList_data[i])-programData;
    }
    // zero-terminate program alias list
    SocketSettings->HostEnt_AddrList_static[i] = 0;
}

VOID __cdecl Socket_gethostbyaddr(UINT Address)
{
    struct hostent *retval = gethostbyaddr((const char *)&Address, 4, PF_INET);

    if(!retval) {
	setEAX(0);
	*SocketSettings->LastError = WSAGetLastError();
	return;
    }
    CopyHOSTENT(retval);
    setEAX(1);	// indicate valid data
}

VOID __cdecl Socket_gethostbyname(UINT pNameOff)
{
    char *pName = (char *)(programData + pNameOff);

    struct hostent *retval = gethostbyname(pName);

    if(!retval) {
	setEAX(0);
	*SocketSettings->LastError = WSAGetLastError();
	return;
    }
    CopyHOSTENT(retval);
    setEAX(1);	// indicate valid data
}

VOID __cdecl Socket_gethostname(UINT pNameOff, int NameLen)
{
    char *pName = (char *)(programData + pNameOff);

    int retval = gethostname(pName, NameLen);

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
    }
}

VOID __cdecl Socket_InstallCallback(VOID)
{
    if(!InitMyWindow2(GetInstance())) {
	MessageBox(NULL, "Could not initialize socket window.",
	    "Extra BIOS Services for ECE 291", MB_OK | MB_ICONERROR);
	setEAX(1);
	return;
    }
    SocketsEnableCallbacks(getDL(), TRUE);
    setEAX(0);
}

VOID __cdecl Socket_RemoveCallback(VOID)
{
    CloseMyWindow2();
    SocketsEnableCallbacks(0, FALSE);
}

VOID __cdecl Socket_AddCallback(UINT Socket, UINT EventMask)
{
    int acteventmask = 0, retval = 0;

    HWND hWnd = GetMyWindow2();

    if(!hWnd) {
	setEAX(1);
	*SocketSettings->LastError = 0xFFFF;
	return;
    }

    if(EventMask & 0x01)
	acteventmask |= FD_READ;
    if(EventMask & 0x02)
	acteventmask |= FD_WRITE;
    if(EventMask & 0x04)
	acteventmask |= FD_OOB;
    if(EventMask & 0x08)
	acteventmask |= FD_ACCEPT;
    if(EventMask & 0x10)
	acteventmask |= FD_CONNECT;
    if(EventMask & 0x20)
	acteventmask |= FD_CLOSE;

    retval = WSAAsyncSelect(Socket, hWnd, WM_USER, acteventmask);

    if(retval != 0) {
	setEAX(1);
	*SocketSettings->LastError = WSAGetLastError();
    } else {
	setEAX(0);
    }
}

VOID __cdecl Socket_GetCallbackInfo(VOID)
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
		setEAX(0);
	    }
	    ReleaseMutex(socketMutex);
	    break;
	case WAIT_TIMEOUT:
	case WAIT_ABANDONED:
	    setEAX(1);
    }
}

