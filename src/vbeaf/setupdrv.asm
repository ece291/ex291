; int SetupDriver(AF_DRIVER *af) implementation
; Copyright (C) 2001 Peter Johnson
;
; Written in assembly because the GCC-generated code is HUGE due to
; the many 0 assignments in the code (4 extra bytes per instruction).
; This code is large enough without doing that!
;
; $Id: setupdrv.asm,v 1.1 2001/03/01 19:27:51 pete Exp $
%include "freebe/vbeaf.inc"

	BITS	32

	SECTION .data

EXTERN _available_modes, _ports_table

	SECTION .text

GLOBAL _SetupDriver
EXTERN _GetVideoModeInfo, _SetVideoMode, _RestoreTextMode, _SetPaletteData
EXTERN _SetCursor, _SetCursorPos, _ShowCursor
EXTERN _SetMix, _DrawScan, _BitBlt, _BitBltSys

; int SetupDriver(AF_DRIVER *af)
;  The first thing ever to be called after our code has been relocated.
;  This is in charge of filling in the driver header with all the required
;  information and function pointers. We do not yet have access to the
;  video memory, so we can't talk directly to the card.
_SetupDriver
	push	edi
	mov	edi, [esp+8]		; Get pointer to AF_DRIVER structure
	xor	eax, eax		; Keep eax 0 throughout to shorten code

	; pointer to a list of the available mode numbers, ended by -1.
	; Our mode numbers just count up from 1, so the mode numbers can
	; be used as indexes into the mode_list[] table (zero is an
	; invalid mode number, so this indexing must be offset by 1).
	mov	dword [edi+AF_DRIVER.AvailableModes], _available_modes

	; amount of video memory in K
	mov	dword [edi+AF_DRIVER.TotalMemory], 64*1024

	; driver attributes (see definitions in vbeaf.h)
	mov	dword [edi+AF_DRIVER.Attributes], afHaveAccel2D | afHaveHWCursor | afHave8BitDAC

	; banked memory size and location: zero if not supported
	mov	[edi+AF_DRIVER.BankSize], eax
	mov	[edi+AF_DRIVER.BankedBasePtr], eax

	; linear framebuffer size and location: zero if not supported
	mov	[edi+AF_DRIVER.LinearSize], eax
	mov	[edi+AF_DRIVER.LinearBasePtr], eax

	; list which ports we are going to access (only needed under Linux)
	mov	dword [edi+AF_DRIVER.IOPortsTable], _ports_table

	; list physical memory regions that we need to access (zero for none)
	mov	ecx, 4
.loopio:
	mov	[edi+AF_DRIVER.IOMemoryBase-4+ecx*4], eax
	mov	[edi+AF_DRIVER.IOMemoryLen-4+ecx*4], eax
	dec	ecx
	jnz	.loopio

	; driver state variables (initialised later during the mode set)
	mov	[edi+AF_DRIVER.BufferEndX], eax
	mov	[edi+AF_DRIVER.BufferEndY], eax
	mov	[edi+AF_DRIVER.OriginOffset], eax
	mov	[edi+AF_DRIVER.OffscreenOffset], eax
	mov	[edi+AF_DRIVER.OffscreenStartY], eax
	mov	[edi+AF_DRIVER.OffscreenEndY], eax

	; relocatable bank switcher (not required by Allgero)
	mov	[edi+AF_DRIVER.SetBank32], eax
	mov	[edi+AF_DRIVER.SetBank32Len], eax

	; Zero all function pointers -- only set those implemented
	add	edi, AF_DRIVER.SupplementalExt
	mov	ecx, (AF_DRIVER_size-AF_DRIVER.SupplementalExt)/4
	rep stosd
	sub	edi, AF_DRIVER_size

	; extension functions
;	mov	[edi+AF_DRIVER.SupplementalExt], eax

	; device driver functions
	mov	dword [edi+AF_DRIVER.GetVideoModeInfo], _GetVideoModeInfo
	mov	dword [edi+AF_DRIVER.SetVideoMode], _SetVideoMode
	mov	dword [edi+AF_DRIVER.RestoreTextMode], _RestoreTextMode
;	mov	[edi+AF_DRIVER.GetClosestPixelClock], eax
;	mov	[edi+AF_DRIVER.SaveRestoreState], eax
;	mov	[edi+AF_DRIVER.SetDisplayStart], eax
;	mov	[edi+AF_DRIVER.SetActiveBuffer], eax
;	mov	[edi+AF_DRIVER.SetVisibleBuffer], eax
;	mov	[edi+AF_DRIVER.GetDisplayStartStatus], eax
;	mov	[edi+AF_DRIVER.EnableStereoMode], eax
	mov	dword [edi+AF_DRIVER.SetPaletteData], _SetPaletteData
;	mov	[edi+AF_DRIVER.SetGammaCorrectData], eax
;	mov	[edi+AF_DRIVER.SetBank], eax

	; hardware cursor functions (impossible to emulate in software!)
	mov	dword [edi+AF_DRIVER.SetCursor], _SetCursor
	mov	dword [edi+AF_DRIVER.SetCursorPos], _SetCursorPos
;	mov	[edi+AF_DRIVER.SetCursorColor], eax
	mov	dword [edi+AF_DRIVER.ShowCursor], _ShowCursor

	; wait until the accelerator hardware has finished drawing
;	mov	[edi+AF_DRIVER.WaitTillIdle], eax

	; on some cards the CPU cannot access the framebuffer while it is in
	; hardware drawing mode. If this is the case, you should fill in these 
	; functions with routines to switch in and out of the accelerator mode. 
	; The application will call EnableDirectAccess() whenever it is about
	; to write to the framebuffer directly, and DisableDirectAccess() 
	; before it calls any hardware drawing routines. If this arbitration is 
	; not required, leave these routines as NULL.
;	mov	[edi+AF_DRIVER.EnableDirectAccess], eax
;	mov	[edi+AF_DRIVER.DisableDirectAccess], eax

	; sets the hardware drawing mode (solid, XOR, etc)
	mov	dword [edi+AF_DRIVER.SetMix], _SetMix

	; pattern download functions. May be NULL if patterns not supported
;	mov	[edi+AF_DRIVER.Set8x8MonoPattern], eax
;	mov	[edi+AF_DRIVER.Set8x8ColorPattern], eax
;	mov	[edi+AF_DRIVER.Use8x8ColorPattern], eax

	; not supported: not used by Allegro
;	mov	[edi+AF_DRIVER.SetLineStipple], eax
;	mov	[edi+AF_DRIVER.SetLineStippleCount], eax

	; not supported. There really isn't much point in this function because
	; a lot of hardware can't do clipping at all, and even when it can there
	; are usually problems with very large or negative coordinates. This
	; means that a software clip is still required, so you may as well
	; ignore this routine.
;	mov	[edi+AF_DRIVER.SetClipRect], eax

	; accelerated drivers must support DrawScan(), but patterns may be NULL
	mov	dword [edi+AF_DRIVER.DrawScan], _DrawScan
;	mov	[edi+AF_DRIVER.DrawPattScan], eax
;	mov	[edi+AF_DRIVER.DrawColorPattScan], eax

	; not supported: not used by Allegro
;	mov	[edi+AF_DRIVER.DrawScanList], eax
;	mov	[edi+AF_DRIVER.DrawPattScanList], eax
;	mov	[edi+AF_DRIVER.DrawColorPattScanList], eax

	; rectangle filling: may be NULL
;	mov	[edi+AF_DRIVER.DrawRect], eax
;	mov	[edi+AF_DRIVER.DrawPattRect], eax
;	mov	[edi+AF_DRIVER.DrawColorPattRect], eax

	; line drawing: may be NULL
;	mov	[edi+AF_DRIVER.DrawLine], eax

	; not supported: not used by Allegro
;	mov	[edi+AF_DRIVER.DrawStippleLine], eax

	; trapezoid filling: may be NULL
;	mov	[edi+AF_DRIVER.DrawTrap], eax

	; not supported: not used by Allegro
;	mov	[edi+AF_DRIVER.DrawTri], eax
;	mov	[edi+AF_DRIVER.DrawQuad], eax

	; monochrome character expansion: may be NULL
;	mov	[edi+AF_DRIVER.PutMonoImage], eax

	; not supported: not used by Allegro
;	mov	[edi+AF_DRIVER.PutMonoImageLin], eax
;	mov	[edi+AF_DRIVER.PutMonoImageBM], eax

	; opaque blitting: may be NULL
	mov	dword [edi+AF_DRIVER.BitBlt], _BitBlt
	mov	dword [edi+AF_DRIVER.BitBltSys], _BitBltSys

	; not supported: not used by Allegro
;	mov	[edi+AF_DRIVER.BitBltLin], eax
;	mov	[edi+AF_DRIVER.BitBltBM], eax

	; masked blitting: may be NULL
;	mov	[edi+AF_DRIVER.SrcTransBlt], eax
;	mov	[edi+AF_DRIVER.SrcTransBltSys], eax

	; not supported: not used by Allegro
;	mov	[edi+AF_DRIVER.SrcTransBltLin], eax
;	mov	[edi+AF_DRIVER.SrcTransBltBM], eax
;	mov	[edi+AF_DRIVER.DstTransBlt], eax
;	mov	[edi+AF_DRIVER.DstTransBltSys], eax
;	mov	[edi+AF_DRIVER.DstTransBltLin], eax
;	mov	[edi+AF_DRIVER.DstTransBltBM], eax
;	mov	[edi+AF_DRIVER.StretchBlt], eax
;	mov	[edi+AF_DRIVER.StretchBltSys], eax
;	mov	[edi+AF_DRIVER.StretchBltLin], eax
;	mov	[edi+AF_DRIVER.StretchBltBM], eax
;	mov	[edi+AF_DRIVER.SrcTransStretchBlt], eax
;	mov	[edi+AF_DRIVER.SrcTransStretchBltSys], eax
;	mov	[edi+AF_DRIVER.SrcTransStretchBltLin], eax
;	mov	[edi+AF_DRIVER.SrcTransStretchBltBM], eax
;	mov	[edi+AF_DRIVER.DstTransStretchBlt], eax
;	mov	[edi+AF_DRIVER.DstTransStretchBltSys], eax
;	mov	[edi+AF_DRIVER.DstTransStretchBltLin], eax
;	mov	[edi+AF_DRIVER.DstTransStretchBltBM], eax
;	mov	[edi+AF_DRIVER.SetVideoInput], eax
;	mov	[edi+AF_DRIVER.SetVideoOutput], eax
;	mov	[edi+AF_DRIVER.StartVideoFrame], eax
;	mov	[edi+AF_DRIVER.EndVideoFrame], eax
	pop	edi
	ret

