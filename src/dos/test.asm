; Protected Mode EX291 Test
;  By Peter Johnson, 2000-2001
;
; $Id: test.asm,v 1.6 2001/03/20 04:51:04 pete Exp $
%include "lib291.inc"

	BITS 32

	GLOBAL	_main

SECTION .bss

_kbINT		resb	1
_kbIRQ		resb	1
_kbPort		resw	1

CurrentKey	resb	1
NewKey		resb	1
mouse_seg	resw	1
mouse_off	resw	1

buttonstatus	resw	1
prevstatus	resw	1
mousex		resw	1
mousey		resw	1
prevmousex	resw	1
prevmousey	resw	1

_VideoBlock	resd	1

SECTION .text

_main
	call	_LibInit

	invoke	_AllocMem, dword 640*480*4
	mov	[_VideoBlock], eax

	invoke	_InitGraphics, dword _kbINT, dword _kbIRQ, dword _kbPort

	invoke	_FindGraphicsMode, word 640, word 480, word 32, dword 1
	push	eax
	invoke	_LockArea, ds, dword _kbPort, dword 2
	invoke	_LockArea, ds, dword _kbIRQ, dword 1
	invoke	_LockArea, ds, dword CurrentKey, dword 1
	invoke	_LockArea, ds, dword NewKey, dword 1
	invoke	_LockArea, cs, dword KeyboardISR, dword KeyboardISR_end-KeyboardISR
	movzx	eax, byte [_kbINT]
	invoke	_Install_Int, eax, dword KeyboardISR
	pop	eax
	invoke	_SetGraphicsMode, ax

	xor	eax, eax
	int	33h

	invoke	_LockArea, ds, dword buttonstatus, dword 2
	invoke	_LockArea, ds, dword mousex, dword 2
	invoke	_LockArea, ds, dword mousey, dword 2
	invoke	_LockArea, cs, dword MouseCallback, dword MouseCallback_end-MouseCallback
	invoke	_Get_RMCB, dword mouse_seg, dword mouse_off, dword MouseCallback, dword 1
	cmp	eax, 0

	mov	dword [DPMI_EAX], 0Ch
	mov	dword [DPMI_ECX], 7Fh
	xor	edx, edx
	mov	dx, [mouse_off]
	mov	[DPMI_EDX], edx
	mov	ax, [mouse_seg]
	mov	[DPMI_ES], ax
	mov	bx, 33h
	call	DPMI_Int

	mov	ecx, 640*480
	xor	eax, eax
	mov	edi, [_VideoBlock]
	cld
	rep stosd

	invoke	_CopyToScreen, dword [_VideoBlock], dword 640*4, dword 0, dword 0, dword 640, dword 480, dword 0, dword 0

.test1:
	cmp	byte [NewKey], 0
	jz	.test1
;	mov	ah, 2h
;	mov	dl, [CurrentKey]
;	int	21h

	mov	byte [NewKey], 0

	xor	eax, eax
	push	eax

.test2:
	mov	ecx, 640*480
	pop	eax
	inc	eax
	push	eax
	mov	edi, [_VideoBlock]
	rep stosd

	invoke	_CopyToScreen, dword [_VideoBlock], dword 640*4, dword 10, dword 10, dword 20, dword 60, dword 5, dword 5

	cmp	byte [NewKey], 0
	jz	.test2
;	mov	ah, 2h
;	mov	dl, [CurrentKey]
;	int	21h
	pop	eax
	mov	byte [NewKey], 0

.test3:
	mov	cx, [buttonstatus]
	mov	dx, [prevstatus]
	cmp	cx, dx
	je	.nope

	mov	[prevstatus], cx

;	invoke	_WritePixel, word [mousex], word [mousey], dword 0FFFFFFh
	movzx	eax, word [mousex]
	movzx	edx, word [mousey]
	imul	edx, 640
	add	edx, eax
	shl	edx, 2
	add	edx, [_VideoBlock]
	mov	dword [edx], 0FFFFFFh

	invoke	_CopyToScreen, dword [_VideoBlock], dword 640*4, dword 0, dword 0, dword 640, dword 480, dword 0, dword 0
.nope:
	cmp	byte [NewKey], 0
	jz	.test3

	mov	byte [NewKey], 0
;	mov	ax, 1
;	mov	ds, ax
.test4:
	mov	cx, [mousex]
	mov	dx, [prevmousex]
	cmp	cx, dx
	je	.nope2

	mov	cx, [mousey]
	mov	dx, [prevmousey]
	cmp	cx, dx
	je	.nope2

	mov	[prevmousey], cx

	mov	cx, [mousex]
	mov	[prevmousex], cx

;	invoke	_WritePixel, word [mousex], word [mousey], dword 0FFFFFFh
	movzx	eax, word [mousex]
	movzx	edx, word [mousey]
	imul	edx, 640
	add	edx, eax
	shl	edx, 2
	add	edx, [_VideoBlock]
	mov	dword [edx], 0FFFFFFh

	invoke	_CopyToScreen, dword [_VideoBlock], dword 640*4, dword 0, dword 0, dword 640, dword 480, dword 0, dword 0
.nope2:
	cmp	byte [NewKey], 0
	jz	near .test4

	mov	dword [DPMI_EAX], 0Ch
	xor     edx, edx
	mov	[DPMI_ECX], edx
	mov	[DPMI_EDX], edx
	mov	[DPMI_ES], dx
	mov     bx, 33h
	call    DPMI_Int

	invoke	_Free_RMCB, word [mouse_seg], word [mouse_off]

	call	_UnsetGraphicsMode

	call	_ExitGraphics

	movzx	eax, byte [_kbINT]
	invoke	_Remove_Int, eax

	call	_LibExit
	ret

KeyboardISR

	mov	dx, [_kbPort]
        in      al, dx          ; Get value from keyboard

        mov     [CurrentKey], al
	test	al, 80h
	jnz	.done
	mov	byte [NewKey], 1
.done:
        mov     al, 20h
	cmp	byte [_kbIRQ], 8
	jb	.lowirq
	out	0A0h, al
.lowirq:
        out     20h, al

        xor     eax, eax        ; Don't chain!

        ret
KeyboardISR_end

proc MouseCallback
.DPMIRegsPtr	arg	4

	push	esi
	mov	esi, [ebp+.DPMIRegsPtr]

	mov	eax, [es:esi+DPMI_EBX_off]
	mov	[buttonstatus], ax
	mov	eax, [es:esi+DPMI_ECX_off]
	mov	[mousex], ax
	mov	eax, [es:esi+DPMI_EDX_off]
	mov	[mousey], ax

	pop	esi

	ret
endproc
MouseCallback_end

