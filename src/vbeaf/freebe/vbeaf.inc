;
;        ______             ____  ______     _____  ______ 
;       |  ____|           |  _ \|  ____|   / / _ \|  ____|
;       | |__ _ __ ___  ___| |_) | |__     / / |_| | |__ 
;       |  __| '__/ _ \/ _ \  _ <|  __|   / /|  _  |  __|
;       | |  | | |  __/  __/ |_) | |____ / / | | | | |
;       |_|  |_|  \___|\___|____/|______/_/  |_| |_|_|
;
;
;       VBE/AF structure definitions and constants.
;
;       See freebe.txt for copyright information.
;
; Converted to NASM by Peter Johnson, 2001.
;
; $Id: vbeaf.inc,v 1.1 2001/02/28 22:34:11 pete Exp $

; mode attribute flags
afHaveMultiBuffer	equ	0001h	; multiple buffers
afHaveVirtualScroll	equ	0002h	; virtual scrolling
afHaveBankedBuffer	equ	0004h	; supports banked framebuffer
afHaveLinearBuffer	equ	0008h	; supports linear framebuffer
afHaveAccel2D		equ	0010h	; supports 2D acceleration
afHaveDualBuffers	equ	0020h	; uses dual buffers
afHaveHWCursor		equ	0040h	; supports a hardware cursor
afHave8BitDAC		equ	0080h	; 8 bit palette DAC
afNonVGAMode		equ	0100h	; not a VGA mode
afHaveDoubleScan	equ	0200h	; supports double scanning
afHaveInterlaced	equ	0400h	; supports interlacing
afHaveTripleBuffer	equ	0800h	; supports triple buffering
afHaveStereo		equ	1000h	; supports stereo LCD glasses
afHaveROP2		equ	2000h	; supports ROP2 mix codes
afHaveHWStereoSync	equ	4000h	; hardware stereo signalling
afHaveEVCStereoSync	equ	8000h	; HW stereo sync via EVC connector

; main VBE/AF driver structure
	STRUC AF_DRIVER 
; header
.Signature		resb	12
.Version		resd	1
.DriverRev		resd	1
.OemVendorName		resb	80
.OemCopyright		resb	80
.AvailableModes		resd	1
.TotalMemory		resd	1
.Attributes		resd	1
.BankSize		resd	1
.BankedBasePtr		resd	1
.LinearSize		resd	1
.LinearBasePtr		resd	1
.LinearGranularity	resd	1
.IOPortsTable		resd	1
.IOMemoryBase		resd	4
.IOMemoryLen		resd	4
.LinearStridePad	resd	1
.PCIVendorID		resw	1
.PCIDeviceID		resw	1
.PCISubSysVendorID	resw	1
.PCISubSysID		resw	1
.Checksum		resd	1
.res2			resd	6
; near pointers mapped by the application
.IOMemMaps		resd	4
.BankedMem		resd	1
.LinearMem		resd	1
.res3			resd	5
; driver state variables
.BufferEndX		resd	1
.BufferEndY		resd	1
.OriginOffset		resd	1
.OffscreenOffset	resd	1
.OffscreenStartY	resd	1
.OffscreenEndY		resd	1
.res4			resd	10
; relocatable 32 bit bank switch routine, for Windows (ugh!)
.SetBank32Len		resd	1
.SetBank32		resd	1
; callback functions provided by the application
.Int86			resd	1
.CallRealMode		resd	1
; main driver setup routine
.InitDriver		resd	1
; VBE/AF 1.0 asm interface (obsolete and not supported by Allegro)
.af10Funcs		resd	40
; VBE/AF 2.0 extensions
.PlugAndPlayInit	resd	1
; extension query function, specific to FreeBE/AF
.OemExt			resd	1
; extension hook for implementing additional VESA interfaces
.SupplementalExt	resd	1
; device driver functions
.GetVideoModeInfo	resd	1
.SetVideoMode		resd	1
.RestoreTextMode	resd	1
.GetClosestPixelClock	resd	1
.SaveRestoreState	resd	1
.SetDisplayStart	resd	1
.SetActiveBuffer	resd	1
.SetVisibleBuffer	resd	1
.GetDisplayStartStatus	resd	1
.EnableStereoMode	resd	1
.SetPaletteData		resd	1
.SetGammaCorrectData	resd	1
.SetBank		resd	1
; hardware cursor functions
.SetCursor		resd	1
.SetCursorPos		resd	1
.SetCursorColor		resd	1
.ShowCursor		resd	1
; 2D rendering functions
.WaitTillIdle		resd	1
.EnableDirectAccess	resd	1
.DisableDirectAccess	resd	1
.SetMix			resd	1
.Set8x8MonoPattern	resd	1
.Set8x8ColorPattern	resd	1
.Use8x8ColorPattern	resd	1
.SetLineStipple		resd	1
.SetLineStippleCount	resd	1
.SetClipRect		resd	1
.DrawScan		resd	1
.DrawPattScan		resd	1
.DrawColorPattScan	resd	1
.DrawScanList		resd	1
.DrawPattScanList	resd	1
.DrawColorPattScanList	resd	1
.DrawRect		resd	1
.DrawPattRect		resd	1
.DrawColorPattRect	resd	1
.DrawLine		resd	1
.DrawStippleLine	resd	1
.DrawTrap		resd	1
.DrawTri		resd	1
.DrawQuad		resd	1
.PutMonoImage		resd	1
.PutMonoImageLin	resd	1
.PutMonoImageBM		resd	1
.BitBlt			resd	1
.BitBltSys		resd	1
.BitBltLin		resd	1
.BitBltBM		resd	1
.SrcTransBlt		resd	1
.SrcTransBltSys		resd	1
.SrcTransBltLin		resd	1
.SrcTransBltBM		resd	1
.DstTransBlt		resd	1
.DstTransBltSys		resd	1
.DstTransBltLin		resd	1
.DstTransBltBM		resd	1
.StretchBlt		resd	1
.StretchBltSys		resd	1
.StretchBltLin		resd	1
.StretchBltBM		resd	1
.SrcTransStretchBlt	resd	1
.SrcTransStretchBltSys	resd	1
.SrcTransStretchBltLin	resd	1
.SrcTransStretchBltBM	resd	1
.DstTransStretchBlt	resd	1
.DstTransStretchBltSys	resd	1
.DstTransStretchBltLin	resd	1
.DstTransStretchBltBM	resd	1
; hardware video functions
.SetVideoInput		resd	1
.SetVideoOutput		resd	1
.StartVideoFrame	resd	1
.EndVideoFrame		resd	1
	ENDSTRUC

