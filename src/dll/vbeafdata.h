/* vbeafdata.h -- structures used in interfacing to VBE/AF driver
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

   $Id: vbeafdata.h,v 1.1 2001/03/19 08:45:28 pete Exp $
*/

#pragma pack(push, 1)

typedef struct DISPATCH_DATA
{
    unsigned int    i[8];
    unsigned int    off[2];
    unsigned short  seg[2];
    unsigned short  s[4];
} DISPATCH_DATA;

#pragma pack(pop)

