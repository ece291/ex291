/* copybuf.c -- Translates 32-bit buffer to 24-bit buffer
   Copyright (C) 2000, Justin Quek

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

   $Id: copybuf.c,v 1.2 2000/12/18 06:28:00 pete Exp $
*/

#include "ex291srv.h"

VOID Copy32To24 (PVOID src, PVOID dest, int width, int height)
{
  int si = 0;
  int di = 0;
  int i;

  for (i = 0;i < width*height;i++)
  {
    *((PDWORD) (((PBYTE)dest)+di)) = *((PDWORD) (((PBYTE)src)+si));
    si += 4;
    di += 3;
  }
}
