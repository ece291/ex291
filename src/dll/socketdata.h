/* socketdata.h -- structures used in interfacing to pmodelib sockets driver
   Copyright (C) 2001 Peter Johnson

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

   $Id: socketdata.h,v 1.3 2001/04/11 20:49:56 pete Exp $
*/

#pragma pack(push, 1)

typedef struct PMODELIB_SOCKADDR
{
    unsigned short Port;
    unsigned int Address;
} PMODELIB_SOCKADDR;

typedef struct PMODELIB_SOCKINITDATA
{
    unsigned short Version;
    unsigned int STRING_MAX;
    unsigned int HOSTENT_ALIASES_MAX;
    unsigned int HOSTENT_ADDRLIST_MAX;
    int *LastError;
    char *NetAddr_static;
    char *HostEnt_Name_static;
    unsigned int *HostEnt_Aliases_static;
    unsigned int *HostEnt_AddrList_static;
    char *HostEnt_Aliases_data;
    unsigned int *HostEnt_AddrList_data;
} PMODELIB_SOCKINITDATA;

#pragma pack(pop)

