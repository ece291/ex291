; Protected Mode EX291 Test
;  By Peter Johnson, 2000
;
; $Id: test.asm,v 1.2 2000/12/18 06:28:00 pete Exp $
%include "lib291.inc"

	BITS 32

	GLOBAL	_main

SECTION .bss

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

SECTION .text

_main
	call	_LibInit

	invoke	_SetGraphics, word 640, word 480

	invoke	_Install_Int, dword 15, dword KeyboardISR ;9

;	xor	eax, eax
;	int	33h

;	invoke	_Get_RMCB, dword mouse_seg, dword mouse_off, dword MouseCallback, dword 1
;	cmp	eax, 0

;	mov	dword [DPMI_EAX], 0Ch
;	mov	dword [DPMI_ECX], 7Fh
;	xor	edx, edx
;	mov	dx, [mouse_off]
;	mov	[DPMI_EDX], edx
;	mov	ax, [mouse_seg]
;	mov	[DPMI_ES], ax
;	mov	bx, 33h
;	call	DPMI_Int

	mov	ecx, 640*480
	xor	eax, eax
	mov	es, [_VideoBlock]
	xor	edi, edi
	rep stosd

	call	_RefreshVideoBuffer

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
	mov	es, [_VideoBlock]
	xor	edi, edi
	rep stosd

	call	_RefreshVideoBuffer

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

	invoke	_WritePixel, word [mousex], word [mousey], dword 0FFFFFFh
	call	_RefreshVideoBuffer
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

	invoke	_WritePixel, word [mousex], word [mousey], dword 0FFFFFFh
	call	_RefreshVideoBuffer
.nope2:
	cmp	byte [NewKey], 0
	jz	.test4

;	mov	dword [DPMI_EAX], 0Ch
;	xor     edx, edx
;	mov	[DPMI_ECX], edx
;	mov	[DPMI_EDX], edx
;	mov	[DPMI_ES], dx
 ;       mov     bx, 33h
  ;      call    DPMI_Int

;	invoke	_Free_RMCB, word [mouse_seg], word [mouse_off]

	call	_UnsetGraphics

	invoke	_Remove_Int, dword 15;9

	call	_LibExit
	ret

KeyboardISR

	mov	dx, 300h
        in      al, dx;60h         ; Get value from keyboard

        mov     [CurrentKey], al
	test	al, 80h
	jnz	.done
	mov	byte [NewKey], 1
.done:
        mov     al, 20h
        out     20h, al

        xor     eax, eax        ; Don't chain!

        ret
KeyboardISR_end

proc MouseCallback
%$DPMIRegsPtr	arg	4

	push	esi
	mov	esi, [ebp+%$DPMIRegsPtr]

	mov	eax, [esi+DPMI_EBX_off]
	mov	[buttonstatus], ax
	mov	eax, [esi+DPMI_ECX_off]
	mov	[mousex], ax
	mov	eax, [esi+DPMI_EDX_off]
	mov	[mousey], ax

	pop	esi

endproc
MouseCallback_end

