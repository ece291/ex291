/* copybuf.c -- Translates 32-bit buffer to 24-bit buffer (or vice-versa)
   Copyright (C) 2000, Justin Quek
   Copyright (C) 2001, Peter Johnson

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

   $Id: copybuf.c,v 1.5 2001/03/30 02:12:20 pete Exp $
*/

#include "ex291srv.h"

VOID Copy32To24 (PVOID src, PVOID dest, int width, int height)
{
    int si = 0;
    int di = 0;
    int i;

    for(i = 0; i < width*height; i++) {
	*((PDWORD) (((PBYTE)dest)+di)) = *((PDWORD) (((PBYTE)src)+si));
	si += 4;
	di += 3;
    }
}

VOID Copy32To24_pitched(PVOID src, PVOID dest, int pitch, int width, int height, int left, int top)
{
    int si = top*pitch+left*4;
    int di = 0;
    int i, j;

    for(j = 0; j < height; j++) {
	for(i = 0; i < width; i++) {
	    *((PDWORD) (((PBYTE)dest)+di)) = *((PDWORD) (((PBYTE)src)+si));
	    si += 4;
	    di += 3;
	}
	si += pitch-width*4;
    }
}

VOID Copy24To32_pitched(PVOID src, PVOID dest, int pitch, int width, int height, int left, int top)
{
    int si = top*pitch+left*3;
    int di = 0;
    int i, j;

    for(j = 0; j < height; j++) {
	for(i = 0; i < width; i++) {
	    *((PDWORD) (((PBYTE)dest)+di)) = *((PDWORD) (((PBYTE)src)+si));
	    si += 3;
	    di += 4;
	}
	si += pitch-width*3;
    }
}

