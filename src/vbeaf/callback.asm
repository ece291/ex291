; Callback implementations
; Copyright (C) 2001 Peter Johnson
;
; $Id: callback.asm,v 1.1 2001/03/19 01:09:31 pete Exp $
%include "../dos/nasm_bop.inc"

	BITS	32

; structure used for passing data to and from the VDD
	STRUC	DISPATCH_DATA
.i0	resd	1
.i1	resd	1
.i2	resd	1
.i3	resd	1
.i4	resd	1
.i5	resd	1
.i6	resd	1
.i7	resd	1
.off0	resd	1
.off1	resd	1
.seg0	resw	1
.seg1	resw	1
.s0	resw	1
.s1	resw	1
.s2	resw	1
.s3	resw	1
	ENDSTRUC

; mode information structure
	STRUC	VIDEO_MODE
.w		resd	1
.h		resd	1
.bpp		resd	1
.redsize	resd	1
.redpos		resd	1
.greensize	resd	1
.greenpos	resd	1
.bluesize	resd	1
.bluepos	resd	1
.rsvdsize	resd	1
.rsvdpos	resd	1
.emulated	resd	1
	ENDSTRUC

; Maximum number of modes
MAX_MODES	equ	64

	SECTION .data

EXTERN _mode_list, _available_modes, _num_modes

GLOBAL _rm_callback_routine, _rm_callback_routine_end
GLOBAL _rm_callback_routine_off, _rm_callback_routine_seg
GLOBAL _rm_dispatch_call, _rm_dispatch_call_end

; 16-bit callback function to be copied into real mode segment for use with VDD
; Simulate16().  It calls the protected mode callback through a far call to a
; DPMI real-mode callback set in the _off and _seg variables.
_rm_callback_routine
	BITS	16

	; Offset the memory location to 0-based.
	call	far [cs:_rm_callback_routine_off-_rm_callback_routine]
	VDD_UnSimulate16

_rm_callback_routine_off	dw	0
_rm_callback_routine_seg	dw	0

_rm_dispatch_call
	VDD_Dispatch
	retf

	BITS	32
_rm_dispatch_call_end
_rm_callback_routine_end

	SECTION .text

GLOBAL _mode_callback

; mode_callback
;  Callback for the VDD to add a new resolution to the table of available
;  modes.
; Inputs: FS:ESI - Pointer to DISPATCH_DATA structure
_mode_callback
	push	ds
	push	fs
	push	esi

	; Get original ds from DPMI regs structure
	mov	ds, [es:edi+24h]

	; Get passed fs, esi from DPMI regs structure
	mov	fs, [es:edi+26h]
	mov	esi, [es:edi+04h]

	; Check for full mode table
	cmp	dword [_num_modes], MAX_MODES
	jge	near .ret

	mov	ax, [fs:esi+DISPATCH_DATA.s0]	; get BPP from s[0]
	; Accept 8, 15, 16, 24, and 32 bpp modes
	cmp	al, 8
	je	.bppok
	cmp	al, 15
	je	.bppok
	cmp	al, 16
	je	.bppok
	cmp	al, 24
	je	.bppok
	cmp	al, 32
	je	.bppok
	jmp	.ret
.bppok:
	; Save mode information
	mov	ebx, [_num_modes]
	imul	ebx, VIDEO_MODE_size

	movzx	eax, ax
	mov	[_mode_list+ebx+VIDEO_MODE.bpp], eax
	movzx	eax, word [fs:esi+DISPATCH_DATA.s1]
	mov	[_mode_list+ebx+VIDEO_MODE.w], eax
	movzx	eax, word [fs:esi+DISPATCH_DATA.s2]
	mov	[_mode_list+ebx+VIDEO_MODE.h], eax
	movzx	eax, word [fs:esi+DISPATCH_DATA.s3]
	mov	[_mode_list+ebx+VIDEO_MODE.emulated], eax
	mov	eax, [fs:esi+DISPATCH_DATA.i0]
	mov	[_mode_list+ebx+VIDEO_MODE.redsize], eax
	mov	eax, [fs:esi+DISPATCH_DATA.i1]
	mov	[_mode_list+ebx+VIDEO_MODE.redpos], eax
	mov	eax, [fs:esi+DISPATCH_DATA.i2]
	mov	[_mode_list+ebx+VIDEO_MODE.greensize], eax
	mov	eax, [fs:esi+DISPATCH_DATA.i3]
	mov	[_mode_list+ebx+VIDEO_MODE.greenpos], eax
	mov	eax, [fs:esi+DISPATCH_DATA.i4]
	mov	[_mode_list+ebx+VIDEO_MODE.bluesize], eax
	mov	eax, [fs:esi+DISPATCH_DATA.i5]
	mov	[_mode_list+ebx+VIDEO_MODE.bluepos], eax
	mov	eax, [fs:esi+DISPATCH_DATA.i6]
	mov	[_mode_list+ebx+VIDEO_MODE.rsvdsize], eax
	mov	eax, [fs:esi+DISPATCH_DATA.i7]
	mov	[_mode_list+ebx+VIDEO_MODE.rsvdpos], eax

	mov	ebx, [_num_modes]
	inc	ebx
	mov	[_available_modes+ebx*2-2], bx
	mov	word [_available_modes+ebx*2], -1
	mov	[_num_modes], ebx
.ret:
	pop	esi
	pop	fs
	pop	ds

	; Perform RETF return
	mov	eax, [esi]
	mov	[es:edi+2Ah], eax		; CS:IP = [SS:SP]
	add	word [es:edi+2Eh], 4		; SP+=4
	iret

