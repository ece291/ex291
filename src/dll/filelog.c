/* filelog.c -- Debug/Logging interface
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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "ex291srv.h"
#include <mmsystem.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static HANDLE logMutex = 0;

void vLogMessage(const char* format, va_list argptr) {
//  if (!g_isDebug) return;
	FILE *fLog;

	if(!logMutex)
		logMutex = CreateMutex(NULL, TRUE, "LoggerMutex");
	else
		WaitForSingleObject(logMutex, INFINITE);

	fLog = fopen("extra291.log", "at");

	if (1) {
		char buf[16];
		fprintf(fLog, "%s - ", _m_strtime(buf));
		vfprintf(fLog, format, argptr);
		fprintf(fLog, "\n");
		fclose(fLog);
	}

	ReleaseMutex(logMutex);
}

void LogMessage(const char* format, ...) {
  va_list argptr;
  va_start(argptr, format);

  vLogMessage(format, argptr);

  va_end(argptr);
}

char* _m_strtime(char* buf) {
  DWORD millis = timeGetTime();
  int mil, sec, min, hr;

  mil = (millis % 1000);
  sec = (millis / 1000) % 60;
  min = (millis / (60 * 1000)) % 60;
  hr  = (millis / (60 * 60 * 1000)) % 24;

  sprintf(buf, "%02d:%02d:%02d.%03d", hr, min, sec, mil);

  return buf;
}

