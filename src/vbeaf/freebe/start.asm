;
;        ______             ____  ______     _____  ______ 
;       |  ____|           |  _ \|  ____|   / / _ \|  ____|
;       | |__ _ __ ___  ___| |_) | |__     / / |_| | |__ 
;       |  __| '__/ _ \/ _ \  _ <|  __|   / /|  _  |  __|
;       | |  | | |  __/  __/ |_) | |____ / / | | | | |
;       |_|  |_|  \___|\___|____/|______/_/  |_| |_|_|
;
;
;       Startup and relocation code.
;
;       See readme.txt for copyright information.
;
; Converted to NASM by Peter Johnson, 2001.
;
; $Id: start.asm,v 1.1 2001/01/08 18:21:31 pete Exp $

	BITS	32

	SECTION .data

GLOBAL _hdraddr, _reladdr

_hdraddr	dd 0
_reladdr	dd 0


	SECTION .text

GLOBAL StartDriver, PlugAndPlayInit, OemExt
EXTERN _SetupDriver, _InitDriver, _FreeBEX


; helper for relocating the driver

%macro DO_RELOCATION 0

	mov	[_reladdr+ebx], ebx	; store relocations
	mov	[_hdraddr+ebx], edx

	mov	ecx, [edx+876]		; get relocation count
	jecxz	.done_relocation

	push	esi			; get relocation data
	lea	esi, [edx+880]
	cld

%%next:
	lodsd
	add	[ebx+eax], ebx
	loop	%%next

	pop	esi			; relocation is complete
%endmacro


; Entry point for the standard VBE/AF interface. When used with our
; extension mechanism, the driver will already have been relocated by
; the OemExt function, so this routine must detect that and do the
; right thing in either case. After sorting out the relocation, it
; chains to a C function called SetupDriver.

PlugAndPlayInit
	mov	edx, ebx		; save base address in edx
	mov	eax, [ebx+576]		; retrieve our actual address
	sub	eax, PlugAndPlayInit	; subtract the linker address
	add	ebx, eax		; add to our base address

	cmp	dword [_reladdr+ebx], 0	; quit if we are already relocated
	jne	.done_relocation

	DO_RELOCATION			; relocate ourselves

.done_relocation:
	push	edx			; do high-level initialization
	call	_SetupDriver
	pop	edx
	ret



; Driver init function. This is just a wrapper for the C routine called
; InitDriver, with a safety check to bottle out if we are being used
; by a VBE/AF 1.0 program (in that case, we won't have been relocated yet
; so we must give up in disgust).

StartDriver
	push	ebx			; check that we have been relocated
	mov	eax, [ebx+412]
	sub	eax, StartDriver
	add	ebx, eax
	cmp	dword [_reladdr+ebx], 0
	je	.vbeaf1

	call	_InitDriver		; ok, this program is using VBE/AF 2.0
	pop	ebx
	ret

.vbeaf1:
	mov	eax, -1			; argh, trying to use VBE/AF 1.0!
	pop	ebx
	ret



; Extension function for the FreeBE/AF enhancements. This will be the
; very first thing called by a FreeBE/AF aware application, and must
; relocate the driver in response. On subsequent calls it will just 
; chain to the C function called FreeBEX.

OemExt
	cmp	dword [esp+8], 0x494E4954 ; check for FAFEXT_INIT parameter
	jne	.done_init

	push	ebx
	mov	ebx, [esp+8]		; read driver address from the stack
	mov	edx, ebx		; save base address in edx
	mov	eax, [ebx+580]		; retrieve our actual address
	sub	eax, OemExt		; subtract the linker address
	add	ebx, eax		; add to our base address

	cmp	dword [_reladdr+ebx], 0	; quit if we are already relocated
	jne	.done_relocation

	add	[edx+580], edx		; relocate the OemExt pointer

	DO_RELOCATION			; relocate the rest of the driver

.done_relocation:
	pop	ebx
	mov	eax, 0x45583031		; return FAFEXT_MAGIC1
	ret

.done_init:
	jmp	_FreeBEX		; let the driver handle this request

