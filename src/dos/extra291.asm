; extra291.asm -- Extra BIOS services for ECE 291 loader
; Copyright (C) 2000-2001 Peter Johnson
;
; Use NASM to assemble.
;
; based on lfnload.asm (in TASM):
; Copyright (C) 1999-2000 Andrew Crabtree, Wojciech Galazka
;
; Original code written by Andrew Crabtree, 1997-1999
;
; This program is free software; you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation; either version 2, or (at your option)
; any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program; if not, write to the Free Software Foundation,
; Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
;
; This is the 16 bit DOS TSR portion of the Extra BIOS services for ECE 291
; for Windows 2000. It will register the DLL, hook various interrupts,
; and then terminate and stay resident. Then it waits for interrupt calls.
; Any calls will be first handled here, and then redirected to the DLL. Some
; calls need special processing in both the DLL and here.
;
; $Id: extra291.asm,v 1.17 2001/12/12 18:30:14 pete Exp $
%include "nasm_bop.inc"

;; dispatch what, where
%macro dispatch 2
	cmp	ax, %1
	je	%2
%endmacro

%macro dispatchALTMPX 2
	cmp	al, %1
	je	%2
%endmacro

%macro dispatchAPI 2
	cmp	bl, %1
	je	%2
%endmacro

%macro call_vdd 0
	shl 	eax, 16
	mov 	ax, [cs:dllHandle]
	VDD_Dispatch
%endmacro

%macro mutex_lock 1
	push	ax
	push	bx
	xor	bl, bl
	mov	bh, 1
%%loop:
	cmpxchg	[%1], bh
	jz	%%done
	mov	ax, 1680h		; [DPMI 1.0] Release Current Timeslice
	int	2Fh
	jmp	short %%loop
%%done:
	pop	bx
	pop	ax
%endmacro

%macro mutex_unlock 1
	mov	byte [%1], 0
%endmacro

%macro isp_header 1
	jmp	short .start_handler
.origint:
	dd	0
	dw	424Bh
	db	%1		; EOI flag - 80h if handler acks PIC, else 0
.hwreset:
	jmp	short .hwreset_handler
.hwreset_handler:
	retf
	times 6 db 0
.start_handler:
%endmacro

ALTMPX_GET_DLLHANDLE		EQU	10h
ALTMPX_GET_SOCKETINT		EQU	11h

GET_MEMORY			EQU	7001h
GET_MODES			EQU	7002h
SET_MODE			EQU	7003h
UNSET_MODE			EQU	7004h
REFRESH_SCREEN			EQU	7005h
GET_MOUSE_CALLBACK_INFO		EQU	7006h

MOUSE_SET_CALLBACK_PARMS	EQU	000Ch

LOADER_PRODUCTBUILD  		EQU	58

MAX_SIZE			equ	260

DRIVER_VERSION			equ	0100h
	
	ORG	100h
	jmp	Do_TSR_Load

dllHandle	dw	0	; return value of Register Module, used by Dispatch 

inDispatch	db	0	; These variables mapped by the VDD.  Don't move/reorder!
VDD_int_wait	db	0
DOS_VDD_Mutex	db	0
WindowedMode	db	0
Mouse_IRQ	db	0
Keyboard_IRQ	db	0
Keyboard_Port	dw	0

Mouse_INT	db	0
Keyboard_INT	db	0

inGraphics	db	0

userMouseCallback	dd	0

old_video_mode	db	0

	STRUC	VESASupp	; VESA supplemental info block
.Signature		resb	7
.Version		resw	1
.SubFunc		resb	8
.OEMSoftwareRev		resw	1
.OEMVendorNamePtr	resd	1
.OEMProductNamePtr	resd	1
.OEMProductRevPtr	resd	1
.OEMStringPtr		resd	1
.Reserved		resb	221
	ENDSTRUC

; VESA information
vesa_oem_name		db	'ECE 291',0
vesa_vendor_name	db	'ECE 291',0
vesa_product_name	db	'Extra BIOS Services',0
vesa_rev_string		db	'Revision 1.1',0

; Alternate Multiplexer data
ALTMPX_Signature	db	'ECE291  ', 'EX291   ', 0
ALTMPX_Version		dw	0100h
ALTMPX_Number		db	0
ALTMPX_ChainedInterrupts
	db 2Dh
	dw Interrupt2D_Handler
	db 2Dh
	dw Interrupt2D_Handler
	db 2Dh
	dw Interrupt2D_Handler
	db 2Dh
	dw Interrupt2D_Handler
	db 2Dh
	dw Interrupt2D_Handler
	db 2Dh
	dw Interrupt2D_Handler

;----------------------------------------
; Interrupt2D_Handler
; Purpose: Hook for Alternate Multiplex interface
;----------------------------------------
Interrupt2D_Handler
	isp_header	0

	pushf
	cmp	ah, [cs:ALTMPX_Number]
	je	.ours
	popf
	jmp	word far [cs:.origint]
.ours:
	dispatchALTMPX	00h, .InstallCheck
	dispatchALTMPX	02h, .Uninstall
	dispatchALTMPX	04h, .GetChainedInts
	dispatchALTMPX	ALTMPX_GET_DLLHANDLE, .GetDLLHandle
	dispatchALTMPX	ALTMPX_GET_SOCKETINT, .GetSocketINT

	; others not implemented, return 0
	mov	al, 0
	popf
	retf	2

.InstallCheck:
	mov	cx, [cs:ALTMPX_Version]
	mov	dx, cs
	mov	di, ALTMPX_Signature

	mov	al, 0FFh			; mark as in use
	popf
	retf	2

.Uninstall:
	cmp	byte [cs:inGraphics], 0
	jz	.notingraphics
	push	ax
	mov	ax, 4F23h
	mov	bl, 03h				; Unset VBE Mode
	int	10h
	pop	ax
.notingraphics:
	mov	bx, cs

	mov	al, 4				; Safe to remove, TSR disabled
	popf
	retf	2

.GetChainedInts:
	mov	dx, cs
	mov	bx, ALTMPX_ChainedInterrupts

	mov	al, 4				; List returned
	popf
	retf	2

.GetDLLHandle:
	mov	dx, [cs:dllHandle]

	mov	al, 1				; Return valid
	popf
	retf	2

.GetSocketINT:
	mov	dl, [cs:Mouse_INT]		; FIXME

	mov	al, 1				; Return valid
	popf
	retf	2

;----------------------------------------
; Interrupt10_Handler
; Purpose: Processes int 10h requests.  Passes request onto BIOS if not
;          handled internally.
;----------------------------------------
Interrupt10_Handler
	isp_header	0

	pushf
	test	ah, ah
	jnz	.dodispatch
	cmp	byte [cs:inGraphics], 0
	je	.dodispatch
	cmp	al, [cs:old_video_mode]
	je	near .UnSetVBEMode_with_oldBIOS
.dodispatch:
	dispatch	4F23h, .Supplemental291API
.original:
	popf
	jmp	word far [cs:.origint]		; jump to BIOS interrupt handler

.Supplemental291API:
	dispatchAPI	00h, .GetSupplementalInfo
	dispatchAPI	01h, near .GetSettings
	dispatchAPI	02h, near .SetVBEMode
	dispatchAPI	03h, near .UnSetVBEMode
	dispatchAPI	04h, near .RefreshScreen
	jmp	.original

.GetSupplementalInfo:
	; ES:DI => pointer to buffer
	mov	dword [es:di+VESASupp.Signature], 'VBE/'
	mov	dword [es:di+VESASupp.Signature+4], '291_'
	mov	word [es:di+VESASupp.Version], 0100h			; Version Number
	xor	ax, ax
	mov	byte [es:di+VESASupp.SubFunc], 00011111b		; Subfunction listing
	mov	byte [es:di+VESASupp.SubFunc+1], al
	mov	word [es:di+VESASupp.SubFunc+2], ax
	mov	word [es:di+VESASupp.SubFunc+4], ax
	mov	word [es:di+VESASupp.SubFunc+6], ax
	mov	word [es:di+VESASupp.OEMSoftwareRev], DRIVER_VERSION	; OEM software version
	mov	word [es:di+VESASupp.OEMVendorNamePtr], vesa_vendor_name
	mov	word [es:di+VESASupp.OEMVendorNamePtr+2], cs
	mov	word [es:di+VESASupp.OEMProductNamePtr], vesa_product_name
	mov	word [es:di+VESASupp.OEMProductNamePtr+2], cs
	mov	word [es:di+VESASupp.OEMProductRevPtr], vesa_rev_string
	mov	word [es:di+VESASupp.OEMProductRevPtr+2], cs
	mov	word [es:di+VESASupp.OEMStringPtr], vesa_oem_name	; OEM Name offset
	mov	word [es:di+VESASupp.OEMStringPtr+2], cs		; OEM Name segment

	; Indicate success to caller
	mov	ax, 004Fh
	popf
	retf	2

.GetSettings:
	mov	bl, [cs:Keyboard_INT]
	mov	bh, [cs:Keyboard_IRQ]
	mov	cx, [cs:dllHandle]
	mov	dx, [cs:Keyboard_Port]

	; Indicate success to caller
	mov	ax, 004Fh
	popf
	retf	2

.SetVBEMode:
	push	eax
	push	bx
	mov	bx, inDispatch
	mov	ax, SET_MODE
	call_vdd
	pop	bx
	pop	eax

	mov	byte [cs:inGraphics], 1

	; Modify BIOS value so DJGPP knows we're in a different mode
	; in segfault routine
	push	es
	xor	ax, ax
	mov	es, ax
	mov	al, [es:449h]
	mov	[cs:old_video_mode], al
	inc	al
	mov	byte [es:449h], al
	pop	es

	mov	ax, 004Fh
	popf
	retf	2

.UnSetVBEMode:
	push	eax
	mov	ax, UNSET_MODE
	call_vdd
	pop	eax

	mov	byte [cs:inGraphics], 0
	mov	dword [cs:userMouseCallback], 0

	; Restore BIOS value
	push	es
	xor	ax, ax
	mov	es, ax
	mov	al, [cs:old_video_mode]
	mov	[es:449h], al
	pop	es

	mov	ax, 004Fh
	popf
	retf	2

.UnSetVBEMode_with_oldBIOS:
	push	eax
	mov	ax, UNSET_MODE
	call_vdd
	pop	eax

	mov	byte [cs:inGraphics], 0
	mov	dword [cs:userMouseCallback], 0

	; Restore BIOS value
	push	ax
	push	es
	xor	ax, ax
	mov	es, ax
	mov	al, [cs:old_video_mode]
	mov	[es:449h], al
	pop	es
	pop	ax

	popf
	jmp	word far [cs:.origint]		; jump to BIOS interrupt handler

.RefreshScreen:
	push	ds
	mov	ax, cs
	mov	ds, ax

	mutex_lock DOS_VDD_Mutex
.waitforVDD:
	cmp	byte [VDD_int_wait], 0
	je	.donewaitforVDD
	mutex_unlock DOS_VDD_Mutex
	hlt
	mutex_lock DOS_VDD_Mutex
	jmp	short .waitforVDD
.donewaitforVDD:
	mov	byte [inDispatch], 1
	mutex_unlock DOS_VDD_Mutex
	push	eax
	mov	ax, REFRESH_SCREEN
	call_vdd
	pop	eax
	mutex_lock DOS_VDD_Mutex
	mov	byte [inDispatch], 0
	mutex_unlock DOS_VDD_Mutex

	pop	ds

	mov	ax, 004Fh
	popf
	retf	2

;----------------------------------------
; Interrupt33_Handler
; Purpose: Processes int 33h requests.
;          Jumps to old handler if not in graphics mode.
;----------------------------------------
Interrupt33_Handler
	isp_header	0

	pushf
	cmp	byte [cs:inGraphics], 0
	jnz	.ourhandler

	popf
	jmp	word far [cs:.origint]		; jump to old handler

.ourhandler:
	dispatch	MOUSE_SET_CALLBACK_PARMS, .definecallback
.callvdd:
	push	esi
	mov	esi, eax
	add	ax, 7010h
	call_vdd
	mov	si, ax
	mov	eax, esi
	pop	esi

	popf
	retf	2

.definecallback:
	mov 	[cs:userMouseCallback], dx
	mov 	[cs:userMouseCallback+2], es

	jmp	short .callvdd

;----------------------------------------
; MouseIRQ_Handler
; Purpose: Processes Mouse IRQ signals generated by VDD.
;          Jumps to old handler if not in graphics mode.
;----------------------------------------
MouseIRQ_Handler
	isp_header	80h

	pushf
	cmp	byte [cs:inGraphics], 0
	jnz	.ourhandler

	popf
	jmp	word far [cs:.origint]	; jump to old handler

.ourhandler:
	
	push	eax
	push	bx
	push	cx
	push	dx
	push	si
	push	di

	mov	ax, GET_MOUSE_CALLBACK_INFO
	call_vdd

	cmp	dword [cs:userMouseCallback], 0
	je	.nocallback

	call	word far [cs:userMouseCallback]

.nocallback:

	mov	al, 20h
	cmp	byte [cs:Mouse_IRQ], 8
	jb	.lowirq
	out	0A0h, al
.lowirq:
	out	20h, al

	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	eax
	popf
	retf	2

;----------------------------------------
; KeyboardIRQ_Handler
; Purpose: Processes Keyboard IRQ signals generated by VDD.
;          It doesn't do anything with the signals right now, this is just
;          present so that the keyboard won't break if keypresses are sent
;          by the VDD before the application has a chance to initialize its
;          keyboard handler to ACK the PICs.
;          TODO: Make this do something useful?
;          Jumps to old handler if not in graphics mode.
;----------------------------------------
KeyboardIRQ_Handler
	isp_header	80h

	pushf
	cmp	byte [cs:inGraphics], 0
	jnz	.ackpic

	popf
	jmp	word far [cs:.origint]	; jump to old handler

.ackpic:
	push	ax

	mov	al, 20h
	cmp	byte [cs:Keyboard_IRQ], 8
	jb	.lowirq
	out	0A0h, al
.lowirq:
	out	20h, al

	pop	ax
	popf
	retf	2

resident_end
; Start of non-resident section

VersionMsg
	db	'Extra BIOS services for ECE 291 v1.0, for Windows 2000',13,10
	db	'Copyright (C) 2000-2001 Peter Johnson',13,10
	db	'BETA SOFTWARE, ABSOLUTELY NO IMPLIED OR EXPRESSED WARRANTY',13,10,'$'

MustUnder2000	db	'This program requires Microsoft Windows 2000, terminating ...',13,10,'$'

; error codes from Register Module
errorNoDLL	db	'DLL not found, terminating ...',13,10,'$'
errorNoDispatch	db	'Dispatch routine not found, terminating ...',13,10,'$'
errorNoInit	db	'Initialization routine not found, terminating ...',13,10,'$'
errorNoMem	db	'Out of memory, terminating ...',13,10,'$'
errorNoUninst	db	'Cannot uninstall.',13,10,'$'
errorBadEnv	db	'Missing or invalid EX291 environment variable, terminating ...',13,10,'$'
errorBadPath	db	'Program directory not found, terminating ...',13,10,'$'

wrongVersion	db	'DLL & COM files version mismatch, terminating ...',13,10,'$'
errorUnknown	db	'Unknown error during module registration, terminating ...,',13,10,'$'
Installed	db	'Extra BIOS services installed.',13,10,'$'
Uninstalled	db	'Extra BIOS services uninstalled.',13,10,'$'
AlreadyInstalled db	'Extra BIOS services already present.',13,10,'$'
AlreadyUninstalled db	'Extra BIOS services not present.',13,10,'$'
RunningWindowed	db	'Running in windowed mode.',13,10,'$'
RunningFullscreen db	'Running in fullscreen mode.',13,10,'$'
GraphicsDisabled db	'Graphics functions disabled.',13,10,'$'

; VDD input strings (dllName is appended to the ex291.com directory path)
dllName		db	'ex291srv.dll',0
		db	13,10,'$'
dllDispatch	db	'Extra291Dispatch',0
		db	13,10,'$'
dllInitialize	db	'Extra291RegisterInit',0
		db	13,10,'$'

; variable name to scan for in environment
envString	db	0,'EX291'

; commandline parameter strings (should be caps here for case-insensitive match)
onlyInstallStr		db	'I',0
onlyUninstallStr	db	'U',0
forceWindowedStr	db	'W',0
forceFullscreenStr	db	'S',0
noGraphicsStr		db	'NOGRAPH',0
noNetworkStr		db	'NONET',0

; loader program settings (changed via commandline parameters)
onlyInstall	db	0
onlyUninstall	db	0
forceWindowed	db	0
forceFullscreen	db	0
noGraphics	db	0
noNetwork	db	0

; mapping between parameter strings and program settings
cmdLineArgStr
	dw	onlyInstallStr
	dw	onlyUninstallStr
	dw	forceWindowedStr
	dw	forceFullscreenStr
	dw	noGraphicsStr
	dw	noNetworkStr
	dw	0
cmdLineArgVar
	dw	onlyInstall
	dw	onlyUninstall
	dw	forceWindowed
	dw	forceFullscreen
	dw	noGraphics
	dw	noNetwork

;----------------------------------------
; InitScreen
; Purpose: Initializes the screen
; Inputs: None
; Outputs: None
;----------------------------------------
InitScreen
	push	ax
	push	es

	; Reset mode to current mode to clear screen
	xor	ax, ax
	mov	es, ax
	mov	al, [es:449h]		; 0000:0449 holds current video mode
	xor	ah, ah
	int	10h

	pop	es
	pop	ax
	ret

;----------------------------------------
; PrintStr
; Purpose: Prints a string to the screen.
; Inputs: dx=offset of string to print
; Outputs: None
;----------------------------------------
PrintStr
	push	ax
	mov	ah, 9h			; [DOS] Write String to Standard Output
	int	21h
	pop	ax
	ret

;----------------------------------------
; ChainInt
; Purpose: Installs an interrupt handler in the calling chain.
; Inputs: al=interrupt number
;         cx=placement in chained interrupts table
;         di=offset of handler function
; Outputs: None
;----------------------------------------
ChainInt
	push	ax
	push	bx
	push	dx
	push	es

	mov 	ah, 35h		; [DOS] Get Interrupt Vector
	int 	21h
	mov 	[di+2], bx	; store old vector in ISR header
	mov 	[di+4], es

	mov 	ah, 25h		; [DOS] Set Interrupt Vector
	mov	dx, di
	push	ds
	push	cs
	pop	ds
	int 	21h
	pop	ds

	; Save new vector in chained interrupts table
	mov	bx, cx
	shl	bx, 1		; cx*3 = offset into table
	add	bx, cx
	mov	[ALTMPX_ChainedInterrupts+bx], al
	mov	[ALTMPX_ChainedInterrupts+bx+1], di

	pop	es
	pop	dx
	pop	bx
	pop	ax
	ret

;----------------------------------------
; UnchainInt
; Purpose: Uninstalls an interrupt handler from the calling chain.
; Inputs: bx=offset of chained interrupt table
;         cx=placement in chained interrupts table
;         dx=segment of chained interrupt table
; Outputs: CF set if interrupt 2Dh (end of table), otherwise CF clear
;----------------------------------------
UnchainInt
	push	ax
	push	bx
	push	dx
	push	ds

	; Address into table
	mov	ds, dx
	mov	ax, cx		; cx*3 = offset into table
	shl	ax, 1
	add	ax, cx
	add	bx, ax
	mov	al, [bx]	; Get interrupt number from table
	mov	bx, [bx+1]	; Go to function offset
	lds	dx, [bx+2]	; Load old vector from ISR header
	mov 	ah, 25h		; [DOS] Set Interrupt Vector
	int 	21h

	cmp	al, 2Dh
	je	.last
	clc
	jmp	short .done

.last:
	stc

.done:
	pop	ds
	pop	dx
	pop	bx
	pop	ax
	ret

;----------------------------------------
; Check_If_Installed
; Inputs: DX:AX -> TSR signature string
; Outputs: CF clear if not installed
;	    AL = 00h if free multiplex number exists
;	     AH = multiplex number to use
;	    AL = 01h if all multiplex numbers are already in use
;            AH destroyed
;	    CX destroyed
;	   CF set if already installed
;	    AL = FFh
;	    AH = multiplex number being used
;	    CX = version number of resident TSR
; Public Domain 1992,1995 Ralf Brown (AMIS 0.92)
;----------------------------------------
Check_If_Installed
	push	ds
	push	si
	mov	ds, dx
	mov	si, ax
	push	es
	push	di
	push	dx
	push	bx
	xor	ax, ax			; AH=mpx #00h, AL=func 00h (instlchk)
	mov	bx, 0001h		; BH=00h, BL=01h: all mpx numbers in use
.loop:
	push	ax
	int	2Dh			; check if INT 2D/AH=xx is in use
	cmp	al, 0FFh		; multiplex number in use?
	pop	ax
	je	.inuse
	or	bl, bl			; if BL=00h, we've already seen a free mpx
	je	.next
	mov	bl,0
	mov	bh, ah
	jmp	short .next
.inuse:
	mov	es, dx
	push	cx			; remember version number
	mov	cx, 16/2		; length of signature string
	cld
	push	si			; remember start of desired signature
	rep	cmpsw			; did we get our signature?
	pop	si			; get back start of desired signature
	pop	cx			; retrieve version
	stc				; assume already installed
	jz	.done			;   and quit if it is
.next:
	inc	ah
	jnz	.loop
	; not yet installed
	clc
	mov	ah, bh			; AH <- multiplex number to use
	mov	al, bl			; AL <- 'available' flag
.done:
	pop	bx
	pop	dx
	pop	di
	pop	es
	pop	si
	pop	ds
	ret

;----------------------------------------
; Load_CmdLine_Settings
; Purpose: Parses command line.
; Inputs: None
; Outputs: None
;----------------------------------------
Load_CmdLine_Settings
	push	ax
	push	bx
	push	cx
	push	dx
	push	si
	push	di
	push	es

	cld

	mov	ah, 62h			; [DOS] Get Current PSP Address
	int	21h

	mov	es, bx
	mov	di, 81h			; Start of command line string
	xor	dh, dh
	mov	dl, [es:80h]		; Length of command line
.loopnext:
	test	dx, dx			; Have we reached the end of the string?
	jz	.done

	; Get to next parameter, skipping ' ', '/', '-'
	cmp	byte [es:di], ' '
	je	.nextchar
	cmp	byte [es:di], '/'
	je	.nextchar
	cmp	byte [es:di], '-'
	je	.nextchar

	xor	bx, bx
.nexttest:
	mov	si, [cmdLineArgStr+bx]
	test	si, si			; End of parameter string list
	jz	.findspace

	push	di
	mov	cx, dx
	inc	cx
.loopcmpstr:
	cmp	byte [si], 0
	je	.isparmend
	mov	al, [es:di]		; load character from commandline
	and	al, 05Fh		; convert to uppercase
	cmp	[si], al
	jne	.mismatch
	inc	si
	inc	di
	dec	cx
	jnz	.loopcmpstr
	; reached the end of parameter string, terminate
	add	sp, 2
	jmp	short .done

.isparmend:
	cmp	byte [es:di], ' '
	je	.foundmatch
	cmp	byte [es:di], 0Dh
	je	.foundmatch

.mismatch:
	pop	di			; Restore old pointer
	add	bx, 2
	jmp	short .nexttest

.foundmatch:
	add	sp, 2			; Throw away saved pointer
	mov	dx, cx			; Update length remaining

	; set value to 1 if parameter present
	mov	si, [cmdLineArgVar+bx]
	mov	byte [si], 1

.findspace:
	; Get to end of this parameter
	cmp	byte [es:di], ' '
	je	.nextchar
	inc	di
	dec	dx
	jz	.done
	jmp	short .findspace

.nextchar:
	inc	di
	dec	dx
	jmp	.loopnext

.done:
	pop	es
	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	ax
	ret

;----------------------------------------
; Load_Env_Settings
; Purpose: Parses EX291 environment variable.
; Inputs: None
; Outputs: ax=1 on error, 0 otherwise
;----------------------------------------
Load_Env_Settings
	push	bx
	push	cx
	push	si
	push	di
	push	es

	mov	ah, 62h			; [DOS] Get Current PSP Address
	int	21h

	mov	es, bx
	mov	ax, [es:002Ch]		; PSP:002Ch = environment's segment
	mov	es, ax

	xor	si, si
	xor	di, di

.GetEnv:
	mov	al, [es:di]		; search for 0, which comes before
	cmp	al, [envString+si]	;   each environment entry.
	je	.FoundOne
	xor	si, si
	inc	di
	cmp	di, 0FFFFh
	jne	.GetEnv

.FoundOne:
	inc	si			; upon finding the 0, check if the
	inc	di			;   rest of EX291 matches.
	cmp	si, 6
	jne	.GetEnv

	inc	di
	xor	cx, cx

.FindSettingsLoop:			; Now parse the line like w K7,300 M9 for current settings.
	xor	al, al			; clear windowed flag
	cmp	byte [es:di], 's'
	je	Settings_ScreenMode
	cmp	byte [es:di], 'S'
	je	Settings_ScreenMode
	inc	al			; set windowed flag
	cmp	byte [es:di], 'w'
	je	Settings_ScreenMode
	cmp	byte [es:di], 'W'
	je	Settings_ScreenMode

	cmp	byte [es:di], 'K'
	je	Settings_Keyboard
	cmp	byte [es:di], 'k'
	je	Settings_Keyboard

	cmp	byte [es:di], 'M'
	je	near Settings_MouseIRQ
	cmp	byte [es:di], 'm'
	je	near Settings_MouseIRQ

	cmp	cx, 0111b
	je	.AllFound
	cmp	byte [es:di], 0
	je	.EnvError
	inc	di
	jmp	.FindSettingsLoop

.AllFound:
	xor	ax, ax
	jmp	short .Return

.EnvError:
	xor	ax, ax
	inc	ax

.Return:
	pop	es
	pop	di
	pop	si
	pop	cx
	pop	bx
	ret

Settings_ScreenMode:
	mov	[WindowedMode], al
	or	cx, 0100b
	inc	di
	jmp	Load_Env_Settings.FindSettingsLoop

Settings_Keyboard:
	; First read Keyboard IRQ
	mov	bx, 1			; bx points to 1's place
	xor	ah, ah			; zero 10's place
	mov	al, [es:di+2]		; is there a second digit?
	cmp	al, '0'
	jb	.readones
	cmp	al, '9'
	ja	.readones
	inc	bx			; point to next digit
	mov	al, [es:di+1]		; read 10's digit
	sub	al, '0'
	cmp	al, 9			; check range
	ja	Load_Env_Settings.EnvError
	mov	ah, al			; ah=al*10
	shl	ah, 2
	add	ah, al
	shl	ah, 1
.readones:
	mov	al, [es:di+bx]		; read 1's digit
	sub	al, '0'
	cmp	al, 9			; check range
	ja	Load_Env_Settings.EnvError
	add	al, ah			; add in 10's (if any) to get IRQ
	mov	[Keyboard_IRQ], al
	cmp	al, 8			; high IRQ?
	jb	.lowirq
	add	al, 70h-8h-8h		; high IRQ maps to 70h.. subtract extra 8 for following add
.lowirq:
	add	al, 8h			; low IRQ maps to 8h
	mov	[Keyboard_INT], al
	add	di, bx			; move to next component
	inc	di

	cmp	byte [es:di], ','	; Check for , delimiter
	jne	Load_Env_Settings.EnvError

	; Read Keyboard I/O Port
	mov	ax, 0
	mov	bl, [es:di+3]		; get lowest nibble
	sub	bl, '0'
	cmp	bl, 9			; check range
	ja	Load_Env_Settings.EnvError
	or	al, bl
	mov	bl, [es:di+2]		; get middle nibble
	sub	bl, '0'
	cmp	bl, 9			; check range
	ja	Load_Env_Settings.EnvError
	shl	bl, 4
	or	al, bl
	mov	bl, [es:di+1]		; get high nibble
	sub	bl, '0'
	cmp	bl, 9			; check range
	ja	near Load_Env_Settings.EnvError
	or	ah, bl
	mov	[Keyboard_Port], ax
	or	cx, 010b
	add	di, 4			; move to next component
	jmp	Load_Env_Settings.FindSettingsLoop

Settings_MouseIRQ:
	mov	bx, 1			; bx points to 1's place
	xor	ah, ah			; zero 10's place
	mov	al, [es:di+2]		; is there a second digit?
	cmp	al, '0'
	jb	.readones
	cmp	al, '9'
	ja	.readones
	inc	bx			; point to next digit
	mov	al, [es:di+1]		; read 10's digit
	sub	al, '0'
	cmp	al, 9			; check range
	ja	near Load_Env_Settings.EnvError
	mov	ah, al			; ah=al*10
	shl	ah, 2
	add	ah, al
	shl	ah, 1
.readones:
	mov	al, [es:di+bx]		; read 1's digit
	sub	al, '0'
	cmp	al, 9			; check range
	ja	near Load_Env_Settings.EnvError
	add	al, ah			; add in 10's (if any) to get IRQ
	mov	[Mouse_IRQ], al
	cmp	al, 8			; high IRQ?
	jb	.lowirq
	add	al, 70h-8h-8h		; high IRQ maps to 70h.. subtract extra 8 for following add
.lowirq:
	add	al, 8h			; low IRQ maps to 8h
	mov	[Mouse_INT], al
	or	cx, 001b		; got mouse component
	add	di, bx			; move to next component
	inc	di
	jmp	Load_Env_Settings.FindSettingsLoop


;----------------------------------------
; Build_DLL_Name
; Purpose: Determines full pathname of DLL file
; Inputs: di, offset of destination buffer to put pathname in
; Outputs: ax=1 on error, 0 otherwise
;----------------------------------------
Build_DLL_Name
	push	bx
	push	cx
	push	si
	push	di
	push	es

	mov	ah, 62h			; [DOS] Get Current PSP Address
	int	21h

	mov	es, bx
	mov	ax, [es:002Ch]		; PSP:002Ch = environment's segment
	mov	es, ax

	; Scan until we reach the end of the environment data (two 0's)
	mov	si, -1
.findendenv
	inc	si
	cmp	byte [es:si], 0
	jnz	.findendenv
	inc	si
	cmp	byte [es:si], 0
	jnz	.findendenv

	; Copy zero-terminated pathname from there+3 to buffer
	add	si, 3
.argv0copy:
	mov	al, [es:si]
	mov	[di], al
	inc	si
	inc	di
	or	al, al
	jnz	.argv0copy

	; Find trailing slash before the filename
.findlastslash:
	cmp	byte [di], '/'
	je	.lastslashfound
	cmp	byte [di], '\'
	je	.lastslashfound
	cmp	di, temp_buf
	je	.error
	dec	di
	jmp	short .findlastslash
.lastslashfound:
	inc	di

	; Replace filename with dllName, and terminate
	mov	ax, cs
	mov	es, ax
	mov	cx, 15
	mov	si, dllName
	rep	movsb

	xor	ax, ax
	jmp	short .done
.error:
	xor	ax, ax
	inc	ax
.done:
	pop	es
	pop	di
	pop	si
	pop	cx
	pop	bx
	ret


;----------------------------------------
; Do_TSR_Load
; Purpose: Loads/unloads resident part of driver.
;----------------------------------------
Do_TSR_Load
	push	cs
	pop	ds

	; parse commandline
	call	Load_CmdLine_Settings

	; start screen output
	call	InitScreen

	mov	dx, VersionMsg
	call	PrintStr

	; check presence of Windows NT/2000
	mov   	ax, 3306h		; [DOS] Get True Version Number
	int   	21h
	cmp	bx, 3205h
	je	.check_presence

	mov	dx, MustUnder2000
	call	PrintStr
        jmp 	.Terminate	

.check_presence:
	mov	dx, ds
	mov	ax, ALTMPX_Signature
	call	Check_If_Installed
	jnc	.install

	cmp	byte [onlyInstall], 0
	jz	.do_uninstall

	mov	dx, AlreadyInstalled
	call	PrintStr
	jmp	.Terminate

.do_uninstall:
	mov	al, 02h			; [AMIS] Uninstall
	mov	dx, cs			; Return address on success
	mov	bx, .done_uninstall
	int	2Dh

	cmp	al, 04h			; Safe to remove, TSR disabled
	jne	.cant_uninstall

	push	bx			; Save installed program segment

	; release VDD
	mov	al, ALTMPX_GET_DLLHANDLE
	int	2Dh
	mov	ax, dx
	VDD_UnRegister

	; restore interrupts
	mov	al, 04h			; [AMIS] Determine Chained Interrupts
	mov	bl, 0
	int	2Dh

	cmp	al, 04h			; List returned
	jne	.cant_uninstall

	xor	cx, cx
.loop_restore_int:
	call	UnchainInt
	jc	.done_restore_int
	inc	cx
	jmp	short .loop_restore_int

.done_restore_int:
	pop	bx			; Restore installed program segment
	push	es
	mov	es, bx
	mov	ah, 49h			; [DOS] Free Memory
	int	21h
	pop	es

.done_uninstall:
	mov	dx, Uninstalled
	call	PrintStr
	jmp 	.Terminate

.cant_uninstall:
	mov	dx, errorNoUninst
	call	PrintStr
	jmp	.Terminate

.install:
	cmp	byte [onlyUninstall], 0
	jz	.do_install

	mov	dx, AlreadyUninstalled
	call	PrintStr
	jmp	.Terminate

.do_install:
	mov	[ALTMPX_Number], ah

	; parse EX291 environment variable
	call	Load_Env_Settings
	test	ax, ax
	jz	.check_overrides

	mov	dx, errorBadEnv
	call	PrintStr
	jmp	.Terminate

.check_overrides:
	; override settings from command line
	cmp	byte [forceWindowed], 0
	jz	.check_forceFullscreen
	mov	byte [WindowedMode], 1
	jmp	short .register_dll

.check_forceFullscreen:
	cmp	byte [forceFullscreen], 0
	jz	.register_dll
	mov	byte [WindowedMode], 0

.register_dll:
	; Print what mode we're running in
	cmp	byte [WindowedMode], 0
	jz	.fullscreen

	mov	dx, RunningWindowed
	call	PrintStr
	jmp	short .do_register

.fullscreen:
	mov	dx, RunningFullscreen
	call	PrintStr

.do_register:
	; Load DLL from same directory as program
	; first copy full pathname of program from environment
	mov	di, temp_buf
	call	Build_DLL_Name
	test	ax, ax
	jz	.registerDLL

	mov	dx, errorBadPath
	call	PrintStr
	jmp	.Terminate

.registerDLL:
	; register Our DLL - From VDD guide for Application Based Intercepts
	stc
	mov 	ax, 0
	mov 	si, temp_buf          	;dll name
	mov 	di, dllInitialize	;init routine
	mov 	bx, dllDispatch	 	;dispatch routine
	VDD_Register
	jnc	.registerOK

.registerError:
        cmp 	ax, 1
        jne 	.noterror1
	mov	dx, errorNoDLL
       	call	PrintStr
       	jmp 	short .Terminate
.noterror1:
        cmp 	ax, 2
	jne	.noterror2
	mov	dx, dllDispatch
        call	PrintStr
	mov	dx, errorNoDispatch
	call	PrintStr
        jmp 	short .Terminate
.noterror2:
        cmp 	ax, 3
        jne 	.noterror3
	mov	dx, dllInitialize
	call	PrintStr
	mov	dx, errorNoInit
	call	PrintStr
        jmp 	short .Terminate
.noterror3:
        cmp 	ax, 4
        jne 	.noterror4
	mov	dx, errorNoMem
	call	PrintStr
	jmp	short .Terminate
.noterror4:
	mov	dx, errorUnknown
	call	PrintStr

.Terminate:
        mov 	ah, 4Ch			; [DOS] Terminate Program
        xor 	al, al               	; Return Code
	int 	21h

.registerOK:
        mov 	[dllHandle], ax

	;check for version
;	mov	ax, LFNSRV_API
;	mov	bx, LFNSRV_GET_VERSION_INFO
;	call_vdd
;	cmp	bx, LOADER_PRODUCTBUILD
;	je	.installIRQ

;	mov	ax, [dllHandle]
;	VDD_UnRegister
;	printstr wrongversion
;	jmp 	short .Terminate

.installIRQ:
	xor	cx, cx

	; Check for disabled functionality
	cmp	byte [noGraphics], 0
	jz	.installGraphics

	mov	dx, GraphicsDisabled
	call	PrintStr
	jmp	short .done

.installGraphics
	; Install 10h handler
	mov 	al, 10h
	mov 	di, Interrupt10_Handler
	call	ChainInt
	inc	cx

	; Install 33h handler
	mov 	al, 33h
	mov 	di, Interrupt33_Handler
	call	ChainInt
	inc	cx

	; Install mouse IRQ handler
	mov 	al, [Mouse_INT]
	mov 	di, MouseIRQ_Handler
	call	ChainInt
	inc	cx

	; Install keyboard IRQ handler
	mov 	al, [Keyboard_INT]
	mov 	di, KeyboardIRQ_Handler
	call	ChainInt
	inc	cx

.done:
	; Install 2Dh handler
	mov	al, 2Dh
	mov	di, Interrupt2D_Handler
	call	ChainInt
	inc	cx

	mov	dx, Installed
	call	PrintStr

        ; Terminate and stay resident now

	mov	es, [cs:2Ch]
	mov	ah, 49h			; [DOS] Free Environment Memory
	int   	21h
	mov 	ah, 31h			; [DOS] TSR
	xor 	al, al               	; Return Code
	mov 	dx, resident_end+15
	mov	cl, 4
	shr	dx, cl
	int 	21h

	SECTION .bss
temp_buf
vesa_info_area	resb	256

