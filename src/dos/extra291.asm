; extra291.asm -- Extra BIOS services for ECE 291 loader
; Copyright (C) 2000 Peter Johnson
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
; Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */
;
; This is the 16 bit DOS TSR portion of the Extra BIOS services for ECE 291
; for Windows NT/2000. It will register the NT dll, hook various interrupts,
; and then terminate and stay resident Then it waits for interrupt calls.
; Any calls will be first handled here, and then redirected to the DLL. Some
; calls need special processing in both the DLL and here.
;
;  ";;;;" meens code that is not neccessary
;
; $Id: extra291.asm,v 1.2 2000/12/18 06:28:00 pete Exp $
%include "nasm_bop.inc"

%macro printstr 1
	mov	ah, 9h
	mov	dx, %1
	int	21h               
%endmacro

;; dispatch what, where
%macro dispatch 2
	cmp	ax, %1
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
;%%loop:
;	cmp	byte [%1], 1
;	je	%%done
;
;	cmp	byte [%1], 0
;	jnz	%%loop
;
;	mov	byte [%1], 1
;	jmp	short %%loop
;%%done:
	push	ax
	xor	al, al
	mov	ah, 1
%%loop:
	cmpxchg	[%1], ah
	jz	%%done
;	hlt
	jmp	short %%loop
%%done:
	pop	ax
%endmacro

%macro mutex_unlock 1
	mov	byte [%1], 0
%endmacro

SET_INTERRUPT_VECTOR		EQU	  25h
GO_STAY_TSR			EQU	  31h
GET_TRUE_VERSION_NUMBER		EQU	3306h
GET_INTERRUPT_VECTOR		EQU	  35h
FREE_ENVIRONM_MEMORY		EQU	  49h	
TERMINATE_PROGRAM		EQU       4Ch
GET_SYSVARS_TABLE		EQU       52h
GET_CURRENT_PSP			EQU       62h

GET_MEMORY			EQU	7001h
GET_MODES			EQU	7002h
SET_MODE			EQU	7003h
UNSET_MODE			EQU	7004h
REFRESH_SCREEN			EQU	7005h
GET_MOUSE_CALLBACK_INFO		EQU	7006h

MOUSE_SET_CALLBACK_PARMS	EQU	000Ch

LOADER_PRODUCTBUILD  		EQU	58

MAX_SIZE			equ	260

DRIVER_VERSION			equ	0001h
	
	ORG	100h
	jmp	Do_TSR_Load

dllHandle	dw	0	; return value of Register Module, used by Dispatch 
oldint10h	dd	0
oldint33h	dd	0
oldint74h	dd	0

inDispatch	db	0
VDD_int_wait	db	0
DOS_VDD_Mutex	db	0
%ifdef WINDOWEDMODE
WindowedMode	db	1
%else
WindowedMode	db	0
%endif

inGraphics	db	0

userMouseCallback	dd	0

old_video_mode	db	0

	STRUC	VESAInfo	; VESA information block
.Signature		resb	4
.Version		resw	1
.OEMStringPtr		resd	1
.Capabilities		resb	4
.VideoModePtr		resd	1
.TotalMemory		resw	1
.OEMSoftwareRev		resw	1
.OEMVendorNamePtr	resd	1
.OEMProductNamePtr	resd	1
.OEMProductRevPtr	resd	1
.Reserved		resb	222
.OEMData		resb	256
	ENDSTRUC

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
vesa_oem_name	db	'ECE 291',0
vesa_mode_list	dw	0FFFFh
; VBE 2.0 information
vesa_vendor_name	db	'ECE 291',0
vesa_product_name	db	'Extra BIOS Services',0
vesa_rev_string		db	'Revision 0.1',0

;----------------------------------------
; Interrupt10_Handler
; Purpose: Processes int 10h requests.  Passes request onto BIOS if not
;          handled internally.
;----------------------------------------
Interrupt10_Handler
	pushf
	test	ah, ah
	jnz	.dodispatch
	cmp	byte [cs:inGraphics], 0
	je	.dodispatch
	cmp	al, [cs:old_video_mode]
	je	near .UnSetVBEMode_with_oldBIOS
.dodispatch:
	dispatch	4F00h, .GetSVGAinfo
	dispatch	4F23h, near .Supplemental291API
.original:
	popf
	jmp	word far [cs:oldint10h]		; jump to BIOS interrupt handler

.GetSVGAinfo:
	; ES:DI => pointer to buffer
	mov	word [es:di+VESAInfo.Version], 0300h			; Version Number
	mov	word [es:di+VESAInfo.OEMStringPtr], vesa_oem_name	; OEM Name offset
	mov	word [es:di+VESAInfo.OEMStringPtr+2], cs		; OEM Name segment
	xor	ax, ax
	mov	word [es:di+VESAInfo.Capabilities], ax			; Capabilities flags
	mov	word [es:di+VESAInfo.Capabilities+2], ax
	mov	word [es:di+VESAInfo.VideoModePtr], vesa_mode_list	; Mode list offset
	mov	word [es:di+VESAInfo.VideoModePtr+2], cs		; Mode list segment
	mov	word [es:di+VESAInfo.TotalMemory], 0FFFFh		; # 64k pages of video RAM
	mov	word [es:di+VESAInfo.OEMSoftwareRev], DRIVER_VERSION	; OEM software version
	mov	word [es:di+VESAInfo.OEMVendorNamePtr], vesa_vendor_name
	mov	word [es:di+VESAInfo.OEMVendorNamePtr+2], cs
	mov	word [es:di+VESAInfo.OEMProductNamePtr], vesa_product_name
	mov	word [es:di+VESAInfo.OEMProductNamePtr+2], cs
	mov	word [es:di+VESAInfo.OEMProductRevPtr], vesa_rev_string
	mov	word [es:di+VESAInfo.OEMProductRevPtr+2], cs

	; Pass uninstall information if incoming Signature='VBE2'
	cmp	dword [es:di+VESAInfo.Signature], 'VBE2'
	jne	.GetSVGAinfo_done
	mov	word [es:di+VESAInfo.OEMData], 1
	mov	ax, [cs:oldint10h]
	mov	word [es:di+VESAInfo.OEMData+2], ax
	mov	ax, [cs:oldint10h+2]
	mov	word [es:di+VESAInfo.OEMData+4], ax
	mov	ax, [cs:oldint33h]
	mov	word [es:di+VESAInfo.OEMData+6], ax
	mov	ax, [cs:oldint33h+2]
	mov	word [es:di+VESAInfo.OEMData+8], ax
	mov	ax, [cs:oldint74h]
	mov	word [es:di+VESAInfo.OEMData+10], ax
	mov	ax, [cs:oldint74h+2]
	mov	word [es:di+VESAInfo.OEMData+12], ax
	mov	ax, [cs:dllHandle]
	mov	word [es:di+VESAInfo.OEMData+14], ax

.GetSVGAinfo_done:
	; Call VDD to determine free memory
	push	eax
	mov	ax, GET_MEMORY
	call_vdd
	mov	word [es:di+VESAInfo.TotalMemory], ax
	pop	eax
	
	; Indicate success to caller
	mov	dword [es:di+VESAInfo.Signature], 'VESA'
	mov	ax, 004Fh
	popf
	retf	2

.Supplemental291API:
	dispatchAPI	00h, .GetSupplementalInfo
	dispatchAPI	02h, .SetVBEMode
	dispatchAPI	03h, near .UnSetVBEMode
	dispatchAPI	04h, near .RefreshScreen
	jmp	.original

.GetSupplementalInfo:
	; ES:DI => pointer to buffer
	mov	dword [es:di+VESASupp.Signature], 'VBE/'
	mov	dword [es:di+VESASupp.Signature+4], '291_'
	mov	word [es:di+VESASupp.Version], 0100h			; Version Number
	xor	ax, ax
	mov	byte [es:di+VESASupp.SubFunc], 00011101b		; Subfunction listing
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
	jmp	word far [cs:oldint10h]		; jump to BIOS interrupt handler

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
	pushf
	cmp	byte [cs:inGraphics], 0
	jnz	.ourhandler

	popf
	jmp	word far [cs:oldint33h]		; jump to old handler

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
; Interrupt74_Handler
; Purpose: Processes IRQ 12 signals generated by Win32 driver.
;          Jumps to old handler if not in graphics mode.
;----------------------------------------
Interrupt74_Handler
	pushf
	cmp	byte [cs:inGraphics], 0
	jnz	.ourhandler

	popf
	jmp	word far [cs:oldint74h]		; jump to old handler

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
	out	0A0h, al
	out	20h, al

	pop	di
	pop	si
	pop	dx
	pop	cx
	pop	bx
	pop	eax
	popf
	retf	2

resident_end
;; Start of non-resident section

versionmsg1	db	'Extra BIOS services for ECE 291 v0.1, for Windows NT/2000',13,10,'$'
versionmsg2	db	'Copyright (C) 2000 Peter Johnson',13,10,'$'
versionmsg3	db	'BETA SOFTWARE, ABSOLUTELY NO IMPLIED OR EXPRESSED WARRANTY',13,10,'$'

;startmsg	db	'Searching module ... ','$'
MustUnderNT	db	'This program requires Microsoft Windows NT/2000, terminating ...',13,10,'$'

;; error codes from Register Module
error1		db	'DLL not found, terminating ...',13,10,'$'
error2		db	'Dispatch routine not found, terminating ...',13,10,'$'
error3		db	'Initialization routine not found, terminating ...',13,10,'$'
error4		db	'Out of memory, terminating ...',13,10,'$'
error5		db	'Sanity check: didnt get uninstall info even though driver is installed ...',13,10,'$'

wrongversion	db	'DLL & COM files version mismatch, terminating ...',13,10,'$'
errorUnknown	db	'Unknown error during module registration, terminating ...,',13,10,'$'
registered	db	'Extra BIOS services installed',13,10,'$'
unregister	db	'Extra BIOS services uninstalled.',13,10,'$'

dllName		db	'ex291srv.dll',0
		db	13,10,'$'
dllDispatch	db	'Extra291Dispatch',0
		db	13,10,'$'
dllInitialize	db	'Extra291RegisterInit',0
		db	13,10,'$'

Do_TSR_Load	; CS=DS=ES
	printstr   versionmsg1
	printstr   versionmsg2
	printstr   versionmsg3

	; check presence of Windows NT
	mov   	ax, GET_TRUE_VERSION_NUMBER
	int   	21h
	cmp	bx, 3205h
	je	.check_presence

	printstr MustUnderNT
        jmp 	.Terminate	

.check_presence:
;	printstr   startmsg
;	printstr   dllName
	
	;check if srv already running
	mov	ax, 4F00h
	push	cs
	pop	es
	mov	di, vesa_info_area
	mov	dword [es:di+VESAInfo.Signature], 'VBE2'
	mov	word [es:di+VESAInfo.OEMData], 0
	int	10h
	cmp	dword [es:di+VESAInfo.Signature], 'VESA'	; Is any VESA present?
	jne	.install_dll
	les	di, [vesa_info_area+VESAInfo.OEMStringPtr]	; Is our driver present?
	cmp	dword [es:di+VESAInfo.Signature], 'ECE '
	;services not running
	jne	.install_dll
	cmp	word [vesa_info_area+VESAInfo.OEMData], 1		; Did we get the uninstall info?
	jne	near .Lerror5

;; restore interrupts
	mov 	ah, SET_INTERRUPT_VECTOR
	mov 	al, 10h            	; video Handler
	lds	dx, [vesa_info_area+VESAInfo.OEMData+2]
	int 	21h

	mov 	ah, SET_INTERRUPT_VECTOR
	mov 	al, 33h            	; mouse Handler
	lds	dx, [vesa_info_area+VESAInfo.OEMData+6]
	int 	21h

	mov 	ah, SET_INTERRUPT_VECTOR
	mov 	al, 74h            	; IRQ12 Handler
	lds	dx, [vesa_info_area+VESAInfo.OEMData+10]
	int 	21h

;;;; release VDD
	push	cs
	pop	ds
	mov	ax, [vesa_info_area+VESAInfo.OEMData+14]
	VDD_UnRegister

	push	cs
	pop	ds
	printstr unregister
	jmp 	short .Terminate

.install_dll:
	push	cs
	pop	es
	stc
;;	register Our DLL - From VDD guide for Application Based Intercepts
	mov 	ax, 0
	mov 	si, dllName          	;dll name
	mov 	di, dllInitialize	;init routine
	mov 	bx, dllDispatch	 	;dispatch routine
	VDD_Register
	jnc	.registerOK

.registerError:
        cmp 	ax, 1
        je 	.Lerror1
        cmp 	ax, 2
        je 	.Lerror2
        cmp 	ax, 3
        je 	.Lerror3
        cmp 	ax, 4
        je 	.Lerror4
	printstr errorUnknown
        jmp 	short .Terminate

.Lerror1:
        printstr error1
        jmp 	short .Terminate

.Lerror2:
        printstr dllDispatch
        printstr error2
        jmp 	short .Terminate

.Lerror3:
        printstr dllInitialize
        printstr error3
        jmp 	short .Terminate

.Lerror4:
        printstr error4
        jmp 	short .Terminate

.Lerror5:
        printstr error5

.Terminate:
        mov 	ah, TERMINATE_PROGRAM
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
	printstr registered

	; Install 10h handler
	mov 	ah, GET_INTERRUPT_VECTOR
	mov 	al, 10h
	int 	21h
	mov 	[oldint10h], bx
	mov 	[oldint10h+2], es
	mov 	ah, SET_INTERRUPT_VECTOR
	mov 	al, 10h
	push	cs
	pop	ds
	mov 	dx, Interrupt10_Handler
	int 	21h

	; Install 33h handler
	mov 	ah, GET_INTERRUPT_VECTOR
	mov 	al, 33h
	int 	21h
	mov 	[oldint33h], bx
	mov 	[oldint33h+2], es
	mov 	ah, SET_INTERRUPT_VECTOR
	mov 	al, 33h
	push	cs
	pop	ds
	mov 	dx, Interrupt33_Handler
	int 	21h

	; Install 74h handler
	mov 	ah, GET_INTERRUPT_VECTOR
	mov 	al, 74h
	int 	21h
	mov 	[oldint74h], bx
	mov 	[oldint74h+2], es
	mov 	ah, SET_INTERRUPT_VECTOR
	mov 	al, 74h
	push	cs
	pop	ds
	mov 	dx, Interrupt74_Handler
	int 	21h

	push	cs
	pop	ds

        ; Terminate and stay resident now

	mov	es, [cs:2Ch]
	mov	ah, FREE_ENVIRONM_MEMORY
	int   	21h
	mov 	ah, GO_STAY_TSR		; TSR
	xor 	al, al               	; Return Code
	mov 	dx, resident_end+15
	mov	cl, 4
	shr	dx, cl
	int 	21h

	SECTION .bss
vesa_info_area	resb	512
