/* keyboard.c -- Keyboard interface
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

   $Id: keyboard.c,v 1.4 2001/01/10 05:56:08 pete Exp $
*/

#include "ex291srv.h"
#include "deque.h"

static queue keyQueue;
static int lastKey = 0;
static HANDLE keyMutex;

static VDD_IO_PORTRANGE kbPorts = {0, 0};

extern PBYTE inDispatch;
extern PBYTE VDD_int_wait;
extern PBYTE DOS_VDD_Mutex;
extern PBYTE Keyboard_IRQ;
extern PWORD Keyboard_Port;

static INT KeyboardIRQpic;
static BYTE KeyboardIRQline;
static WORD KeyboardPort;

BOOL InitKey(VOID)
{
	VDD_IO_HANDLERS kbIOHandlers = {KeyInIO, NULL, NULL, NULL,
		KeyOutIO, NULL, NULL, NULL};
	
	keyMutex = CreateMutex(NULL, TRUE, NULL);
	if(!keyMutex)
		return FALSE;
	Q_Init(&keyQueue);
	ReleaseMutex(keyMutex);

	KeyboardPort = *Keyboard_Port;

	kbPorts.First = KeyboardPort; kbPorts.Last = KeyboardPort;
	if(!VDDInstallIOHook(GetInstance(), 1, &kbPorts, &kbIOHandlers))
		return FALSE;

	if(*Keyboard_IRQ < 8) {
		KeyboardIRQpic = ICA_MASTER;
		KeyboardIRQline = (INT)*Keyboard_IRQ;
	} else {
		KeyboardIRQpic = ICA_SLAVE;
		KeyboardIRQline = (INT)*Keyboard_IRQ-8;
	}

	//LogMessage("Initialized keyboard");

	return TRUE;
}

VOID CloseKey(VOID)
{
	switch(WaitForSingleObject(keyMutex, 500L))
	{
	case WAIT_OBJECT_0:
		while(Q_PopTail(&keyQueue)) {}
		ReleaseMutex(keyMutex);
		break;
	}

	VDDDeInstallIOHook(GetInstance(), 1, &kbPorts);

	CloseHandle(keyMutex);

	//LogMessage("Shut down keyboard");
}

VOID AddKey(LONG key, BOOL Break)
{
	int scancode;
	int count = 1;

	scancode = (key >> 16) & 0xff;

	if (scancode < 0x80)	// sanity check
	{
		switch(WaitForSingleObject(keyMutex, 50L))
		{
		case WAIT_OBJECT_0:
			lastKey = (int)(scancode | (Break?0x80:0x00));

			/*if (key & 0x1000000) {	// 24th bit set = extended key
				Q_PushHead(&keyQueue, (void *)0xE0);
				LogMessage("Added 0xE0 to FIFO");
				count++;
			}*/
			Q_PushHead(&keyQueue,
				(void *)(scancode | (Break?0x80:0x00)));
			ReleaseMutex(keyMutex);
			/*LogMessage("New Key: 0x%x",
				(int)(scancode | (Break?0x80:0x00)));*/
			mutex_lock(DOS_VDD_Mutex);
			*VDD_int_wait = 1;
			while(*inDispatch)
			{
				mutex_unlock(DOS_VDD_Mutex);
				Sleep(0);
				mutex_lock(DOS_VDD_Mutex);
			}
			mutex_unlock(DOS_VDD_Mutex);
			//LogMessage("trigger keyboard IRQ");
			VDDSimulateInterrupt(KeyboardIRQpic, KeyboardIRQline, 1);
			//LogMessage("end trigger keyboard IRQ");
			mutex_lock(DOS_VDD_Mutex);
			*VDD_int_wait = 0;

			// wake up VDM thread here?

			mutex_unlock(DOS_VDD_Mutex);

			break;
		}
	}
}

int GetNextKey(VOID)
{
	int RetVal = 0;

	switch(WaitForSingleObject(keyMutex, INFINITE))
	{
	case WAIT_OBJECT_0:
		if(Q_Empty(&keyQueue))
			RetVal = 0x00;
		else
			RetVal = (int)Q_PopTail(&keyQueue);
		/*RetVal = lastKey;*/
		ReleaseMutex(keyMutex);
		break;
	case WAIT_TIMEOUT:
	case WAIT_ABANDONED:
		RetVal = 0x00;
	}

	//LogMessage("Returned Key: 0x%x", RetVal);

	return RetVal;
}

VOID KeyInIO(WORD iport, BYTE *data)
{
	if(iport == KeyboardPort)
		*data = (BYTE)GetNextKey();
	else
		*data = 0xFF;
}

VOID KeyOutIO(WORD iport, BYTE data)
{
}

