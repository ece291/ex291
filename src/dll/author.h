/* author.h -- info for driver builds		
   From Microsoft DDK NT 4.0
   Modified parts Copyright (C) 1999-2000 Wojciech Galazka
   More modifications Copyright (C) 2000 Peter Johnson

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  */


/*---------------------------------------------------*/
/* the following section defines values used in the  */
/* version data structure for all files, and which   */
/* do not change.                                    */
/*---------------------------------------------------*/

#ifndef __author_h
#define __author_h

#define VER_COMPANYNAME_STR		"Peter Johnson"

#define VER_LEGALTRADEMARKS_STR	\
"This program is free software; you can redistribute it and/or modify \
it under the terms of the GNU General Public License as published by \
the Free Software Foundation; either version 2, or (at your option) \
any later version."

#define VER_LEGALCOPYRIGHT_YEARS    "2000. "

#define VER_LEGALCOPYRIGHT_STR			\
	"Copyright \251 Peter Johnson."	\
	VER_LEGALCOPYRIGHT_YEARS		\
	VER_LEGALTRADEMARKS_STR

/* default is nodebug */
#if DBG
#define VER_DEBUG			VS_FF_DEBUG
#else
#define VER_DEBUG			0
#endif

/* default is release */
#if BETA
#define VER_PRERELEASE			VS_FF_PRERELEASE
#else
#define VER_PRERELEASE			0
#endif

#define VER_FILEFLAGSMASK  		VS_FFI_FILEFLAGSMASK
#define VER_FILEOS			VOS_NT_WINDOWS32
#define VER_FILEFLAGS	   		(VER_PRERELEASE | VER_DEBUG)

#endif

