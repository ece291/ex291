/* debug.h -- debug macros
   Copyright (C) 1999-2000 Wojciech Galazka

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

   $Id: debug.h,v 1.2 2000/12/18 06:28:00 pete Exp $
*/

#ifndef __debug_h
#define __debug_h

#ifndef BYTE
#define BYTE	unsigned char
#endif 

#ifndef LARGE_BUF_SIZE	
#define LARGE_BUF_SIZE				128
#endif

#ifndef OutputDebugString
#define OutputDebugString(string)	printf("%s", string);
#endif

#ifndef wsprintf
#define wsprintf  sprintf
#endif  

extern char _DebugString[LARGE_BUF_SIZE]; 
extern char _DebugPos[LARGE_BUF_SIZE]; 
extern char _LastFunction[LARGE_BUF_SIZE];
extern BYTE _DebugStackLevel;

#if !defined DBG
#define START_DEBUGGING
#define DBGMSG(message)     
#define PROLOG(function) 
#define EPILOG(function, success) 
#define EPILOGSTR(function, success) 
#define LFNVERSION() 
#define ISNULL(variable)		if ((variable) == NULL) return ERROR_INVALID_DATA	
#define DBGVALUE(variable, type) 
#define VALUE(variable, type) 

#else
#define START_DEBUGGING	_DebugPos[0]='\0'

#define DBGMSG(message)     \
do {                                                             			\
	wsprintf(_DebugString, "%s%s: %s\n",_DebugPos, _LastFunction, message);		\
	OutputDebugString(_DebugString);                            			\
} while(0)

#define DBGVALUE(variable, type) \
do {                         						\
	wsprintf(_DebugString, "%s" #variable " = " type "\n",_DebugPos, variable);	\
	OutputDebugString(_DebugString);                            	\
} while (0)

#define VALUE(variable, type) \
do {                         						\
	wsprintf(_DebugString, "%s" #variable " = " type "\n",_DebugPos, variable);	\
	OutputDebugString(_DebugString);                            	\
} while (0)


#define PROLOG(function) \
do {                                                             	\
	_DebugPos[_DebugStackLevel]=' ';                          	\
	_DebugStackLevel++;                                             \
	_DebugPos[_DebugStackLevel]='\0';                            	\
	wsprintf(_DebugString, "%s%d Entering " #function "\n",_DebugPos, _DebugStackLevel);	\
	lstrcpy(_LastFunction, #function);				\
	OutputDebugString(_DebugString);                            	\
} while(0)

#define EPILOG(function, success) \
do {                          						\
	if ((success) == TRUE)                                   	\
		wsprintf(_DebugString, "%s%d Leaving " #function "\n",_DebugPos, _DebugStackLevel);	\
        else								\
		wsprintf(_DebugString, "%s%d Leaving " #function " (file %s line %d)\n",_DebugPos, _DebugStackLevel, __FILE__, __LINE__);	\
	OutputDebugString(_DebugString);                            	\
	_LastFunction[0] = '\0';                         		\
	_DebugStackLevel--;                                             \
	_DebugPos[_DebugStackLevel]='\0';                         	\
} while(0)

#define EPILOGSTR(function, success) \
do {                          						\
	if ((success) == TRUE)                                   	\
		wsprintf(_DebugString, "%s%d Leaving %s\n",_DebugPos, _DebugStackLevel, function);	\
        else								\
		wsprintf(_DebugString, "%s%d Leaving %s (file %s line %d)\n",_DebugPos, function, _DebugStackLevel, __FILE__, __LINE__);	\
	OutputDebugString(_DebugString);                            	\
	_LastFunction[0] = '\0';                         		\
	_DebugStackLevel--;                                             \
	_DebugPos[_DebugStackLevel]='\0';                         	\
} while(0)

#define LFNVERSION() \
do {	\
	wsprintf(_DebugString, "Build date %s time %s\n", __DATE__, __TIME__);	\
	OutputDebugString(_DebugString);                            	\
} while(0)

#define ISNULL(variable)	\
if ((variable) == NULL)	{                                		\
	wsprintf(_DebugString, "%s" #variable " is null\n",_DebugPos);	\
	OutputDebugString(_DebugString);                            	\
	EPILOGSTR(_LastFunction, FALSE);					\
	return ERROR_INVALID_DATA;					\
}
 
#endif	// DBG

#endif
 
