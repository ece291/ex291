/* driver.c -- EX291 VBE/AF driver implementation file.
   Copyright (C) 2001 Peter Johnson

   $Id: driver.c,v 1.2 2001/12/12 06:08:23 pete Exp $
*/

#include <pc.h>
#include <sys/farptr.h>
#include "vbeaf.h"

/* extension for remapped keyboard */
#define FAFEXT_KEYBOARD		FAF_ID('K','E','Y','S')

/* required exported DispatchCall function */
#define FAFEXT_DISPATCHCALL	FAF_ID('D','I','C','L')

/* EX291-specific emulation flag for video capabilities */
#define EX291_EMULATED		FAF_ID('E','M','U','L')

/* interface structure containing remapped keyboard data */
typedef struct FAF_KEYBOARD_DATA
{
    unsigned char   INT			__attribute__ ((packed));
    unsigned char   IRQ			__attribute__ ((packed));
    unsigned short  Port		__attribute__ ((packed));
} FAF_KEYBOARD_DATA;

/* driver data for keyboard */
FAF_KEYBOARD_DATA kbdata;

/* VESA Supplementary API info block */
typedef struct VESA_SUPP_INFO
{
    unsigned char   Signature[7]	__attribute__ ((packed));
    unsigned short  Version		__attribute__ ((packed));
    unsigned char   SubFunc[8]		__attribute__ ((packed));
    unsigned short  OEMSoftwareRev	__attribute__ ((packed));
    unsigned long   OEMVendorNamePtr	__attribute__ ((packed));
    unsigned long   OEMProductNamePtr	__attribute__ ((packed));
    unsigned long   OEMProductRevPtr	__attribute__ ((packed));
    unsigned long   OEMStringPtr	__attribute__ ((packed));
    unsigned char   Reserved[221]	__attribute__ ((packed));
} VESA_SUPP_INFO;

#define SAFETY_BUFFER	4096

/* driver function prototypes */
long GetVideoModeInfo(AF_DRIVER *af, short mode, AF_MODE_INFO *modeInfo);
long SetVideoMode(AF_DRIVER *af, short mode, long virtualX, long virtualY, long *bytesPerLine, int numBuffers, AF_CRTCInfo *crtc);
void RestoreTextMode(AF_DRIVER *af);
void SetPaletteData(AF_DRIVER *af, AF_PALETTE *pal, long num, long index, long waitVRT);
void SetMix(AF_DRIVER *af, long foreMix, long backMix);
void DrawScan(AF_DRIVER *af, long color, long y, long x1, long x2);
void BitBlt(AF_DRIVER *af, long left, long top, long width, long height, long dstLeft, long dstTop, long op);
void BitBltSys(AF_DRIVER *af, void *srcAddr, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op);



/* list which ports we are going to access (only needed under Linux) */
unsigned short ports_table[] = { 0xFFFF };

/* data structure passed to VDD */
typedef struct DISPATCH_DATA
{
    unsigned int    i[8]		__attribute__ ((packed));
    unsigned int    off[2]		__attribute__ ((packed));
    unsigned short  seg[2]		__attribute__ ((packed));
    unsigned short  s[4]		__attribute__ ((packed));
} DISPATCH_DATA;

/* VDD commands */
#define	    VBEAF_GET_MEMORY		0x6001
#define	    VBEAF_GET_MODELIST		0x6002
#define	    VBEAF_SET_MODE		0x6003
#define	    VBEAF_SET_PALETTE		0x6004
#define	    VBEAF_BITBLT_VIDEO		0x6005
#define	    VBEAF_BITBLT_SYS		0x6006
#define	    VBEAF_SET_CURSOR_SHAPE	0x6007
#define	    VBEAF_SET_CURSOR_POS	0x6008
#define	    VBEAF_SHOW_CURSOR		0x6009


/* at startup we ask the VDD to find out what resolutions the driver provides,
 * storing them in a table for future use.
 */

typedef struct VIDEO_MODE
{
    int w				__attribute__ ((packed));
    int h				__attribute__ ((packed));
    int bpp				__attribute__ ((packed));
    int redsize				__attribute__ ((packed));
    int redpos				__attribute__ ((packed));
    int greensize			__attribute__ ((packed));
    int greenpos			__attribute__ ((packed));
    int bluesize			__attribute__ ((packed));
    int bluepos				__attribute__ ((packed));
    int rsvdsize			__attribute__ ((packed));
    int rsvdpos				__attribute__ ((packed));
    int emulated			__attribute__ ((packed));
} VIDEO_MODE;

#define MAX_MODES    128

VIDEO_MODE mode_list[MAX_MODES];

short available_modes[MAX_MODES+1] = { -1 };

int num_modes = 0;


/* direct access to VDD */
unsigned short dllHandle;


/* internal driver state variables */
int af_bpp;
int af_width;
int af_height;
int af_fore_mix;
int af_back_mix;


extern unsigned char rm_callback_routine, rm_callback_routine_end;
extern unsigned short rm_callback_routine_off, rm_callback_routine_seg;
extern unsigned char rm_dispatch_call;


/* mode_callback:
 *  Callback for the VDD to add a new resolution to the table of
 *  available modes.
 */
extern void mode_callback(void);


/* DispatchCall:
 *  Passes control to the VDD, with parameters.
 */
unsigned int (*DispatchCall)(unsigned int Handle, unsigned int Command, DISPATCH_DATA *Data) = 0;

/* DispatchCallRM:
 *  Passes control to the VDD from real mode, with parameters.
 */
unsigned int DispatchCallRM(unsigned short command, DISPATCH_DATA *data, unsigned short seg, unsigned short off)
{
    RM_REGS r;

    r.x.cs = seg;		/* CS:IP = real-mode procedure */
    r.x.ip = off;
    r.x.ds = _my_ds();		/* DS = pmode DS (for pmode callback) */
    r.x.fs = _my_ds();		/* FS:ESI = DISPATCH_DATA structure */
    r.d.esi = (unsigned int)data;
    r.x.ax_hi = command;	/* command, DLL handle in EAX */
    r.x.ax = dllHandle;
    r.x.ss = 0;			/* use DPMI-provided stack */
    r.x.sp = 0;
    r.x.flags = 0;		/* clear all flags */

    __asm__ ("int $0x31" : : "a" (0x301), "b" (0), "c" (0), "D" (&r));

    return(r.x.flags & 1);	/* carry flag set = error */
}

/* InitDriver:
 *  The second thing to be called during the init process, after the 
 *  application has mapped all the memory and I/O resources we need.
 *  This is in charge of finding the card, returning 0 on success or
 *  -1 to abort.
 */
int InitDriver(AF_DRIVER *af)
{
    VESA_SUPP_INFO vesa_supp_info;
    int	seg, sel;
    unsigned int rmcb_seg, rmcb_off;
    int i;
    RM_REGS r;
    DISPATCH_DATA d;

    /* see if our 16-bit driver & DLL have been loaded */

    /* fetch the VESA supplementary API info block */
    seg = allocate_dos_memory(sizeof(VESA_SUPP_INFO)+SAFETY_BUFFER, &sel);
    if (seg < 0)
	return -1;

    for (i=0; i<(int)sizeof(VESA_SUPP_INFO); i++)
	_farpokeb(sel, i, 0);

    r.x.ax = 0x4F23;	/* our API is at 23h */
    r.x.bx = 0;		/* subfunction 0 = get API info */
    r.x.di = 0;
    r.x.es = seg;
    rm_int(0x10, &r);
    if (r.h.ah) {
	/* no API at 23h */
	free_dos_memory(sel);
	return -1;
    }

    for (i=0; i<(int)sizeof(VESA_SUPP_INFO); i++)
	((char *)&vesa_supp_info)[i] = _farpeekb(sel, i);

    /* Is it our API? */
    if ((vesa_supp_info.Signature[0] != 'V') ||
	(vesa_supp_info.Signature[1] != 'B') ||
	(vesa_supp_info.Signature[2] != 'E') ||
	(vesa_supp_info.Signature[3] != '/') ||
	(vesa_supp_info.Signature[4] != '2') ||
	(vesa_supp_info.Signature[5] != '9') ||
	(vesa_supp_info.Signature[6] != '1')) {
	free_dos_memory(sel);
	return -1;
    }

    /* Check driver version and support of function we'll be using */
    if ((vesa_supp_info.Version < 0x0100) ||
	!(vesa_supp_info.SubFunc[0] & 0x02)) {
	free_dos_memory(sel);
	return -1;
    }

    /* get driver settings */
    r.x.ax = 0x4F23;
    r.x.bx = 1;		/* subfunction 1 = get settings */
    rm_int(0x10, &r);
    if (r.h.ah) {
	free_dos_memory(sel);
	return -1;
    }
    kbdata.INT = r.h.bl;
    kbdata.IRQ = r.h.bh;
    dllHandle = r.x.cx;
    kbdata.Port = r.x.dx;

    /* get free video memory (from VDD) */
    if(!DispatchCall || DispatchCall(dllHandle, VBEAF_GET_MEMORY, &d)) {
	free_dos_memory(sel);
	return -1;
    }
    af->TotalMemory = d.i[0];

    /* allocate real-mode callback for mode callback */
    __asm__ __volatile__ (
	"pushw	%%ds		\n"
	"movl	%%cs, %%eax	\n"
	"movl	%%eax, %%ds	\n"
	"movw	$0x303, %%ax	\n"
	"int	$0x31		\n"
	"popw	%%ds		\n"
    : "=c" (rmcb_seg), "=d" (rmcb_off)
    : "S" (&mode_callback), "D" (&r)
    : "ax"
    );

    /* set up real-mode callback using UnSimulate16() in our dos memory */
    rm_callback_routine_seg = (unsigned short)rmcb_seg;
    rm_callback_routine_off = (unsigned short)rmcb_off;
    for (i=0; i<(int)(&rm_callback_routine_end-&rm_callback_routine); i++)
	_farpokeb(sel, i, *(&rm_callback_routine+i));

    /* get mode list (from VDD) via real-mode procedure */
    d.seg[0] = seg;
    d.off[0] = 0;
    if(DispatchCallRM(VBEAF_GET_MODELIST, &d, seg,
	(unsigned short)(&rm_dispatch_call-&rm_callback_routine))) {
	free_dos_memory(sel);
	return -1;
    }

    /* free real-mode callback */
    __asm__ __volatile__ ("int $0x31\n"
    : : "a" (0x304), "c" (rmcb_seg), "d" (rmcb_off));

    free_dos_memory(sel);

    return 0;
}



/* FreeBEX:
 *  Returns an interface structure for the requested FreeBE/AF extension.
 */
void *FreeBEX(AF_DRIVER *af, unsigned long id)
{
    switch (id) {
	case FAFEXT_KEYBOARD:
	    return &kbdata;
	case FAFEXT_DISPATCHCALL:
	    return &DispatchCall;
	default:
	    return NULL;
    }
}



/* GetVideoModeInfo:
 *  Retrieves information about this video mode, returning zero on success
 *  or -1 if the mode is invalid.
 */
long GetVideoModeInfo(AF_DRIVER *af, short mode, AF_MODE_INFO *modeInfo)
{
    VIDEO_MODE *info;
    int i;

    if ((mode <= 0) || (mode > num_modes))
	return -1;

    info = &mode_list[mode-1];

    /* clear the structure to zero */
    for (i=0; i<(int)sizeof(AF_MODE_INFO); i++)
	((char *)modeInfo)[i] = 0;

    /* mark all modes as accelerated */
    modeInfo->Attributes |= afHaveAccel2D;

    /* copy saved mode parameters from internal structure */
    modeInfo->XResolution = info->w;
    modeInfo->YResolution = info->h;
    modeInfo->BytesPerScanLine = info->w*(info->bpp/8);
    modeInfo->MaxBytesPerScanLine = info->w*(info->bpp/8);
    modeInfo->LinBytesPerScanLine = info->w*(info->bpp/8);
    modeInfo->BitsPerPixel = info->bpp;
    modeInfo->MaxBuffers = 1;

    /* signal if emulated mode */
    if(info->emulated)
	modeInfo->VideoCapabilities = EX291_EMULATED;

    return 0;
}



/* SetVideoMode:
 *  Sets the specified video mode, returning zero on success.
 *
 *  Possible flag bits that may be or'ed with the mode number:
 *
 *    0x8000 = don't clear video memory
 *    0x4000 = enable linear framebuffer
 *    0x2000 = enable multi buffering
 *    0x1000 = enable virtual scrolling
 *    0x0800 = use refresh rate control
 *    0x0400 = use hardware stereo
 */
long SetVideoMode(AF_DRIVER *af, short mode, long virtualX, long virtualY, long *bytesPerLine, int numBuffers, AF_CRTCInfo *crtc)
{
    VIDEO_MODE *info;
    RM_REGS r;
    DISPATCH_DATA d;

    /* mask off all flag bits */
    mode &= 0x3FF;

    if ((mode <= 0) || (mode > num_modes))
	return -1;

    info = &mode_list[mode-1];

    /* first set directly through VDD */
    d.i[0] = info->w;
    d.i[1] = info->h;
    d.i[2] = info->bpp;
    if(!DispatchCall || DispatchCall(dllHandle, VBEAF_SET_MODE, &d))
	return -1;

    /* set mode through DOS driver -- DOS driver will call VDD, but VDD will
     * ignore the call because we called it first.  Backwards compat is fun! :)
     */
    r.x.ax = 0x4F23;
    r.x.bx = 2;		/* subfunction 2 = set mode */
    r.x.cx_hi = (unsigned short)virtualX;
    r.x.cx = (unsigned short)virtualY;
    r.x.dx = _my_ds();
    r.d.edi = 0;

    rm_int(0x10, &r);
    if (r.h.ah)
       return -1;

    af_fore_mix = AF_REPLACE_MIX;
    af_back_mix = AF_FORE_MIX;

   return 0;
}



/* RestoreTextMode:
 *  Returns to text mode, shutting down the accelerator hardware.
 */
void RestoreTextMode(AF_DRIVER *af)
{
    RM_REGS r;

    r.x.ax = 0x4F23;
    r.x.bx = 3;		/* subfunction 3 = unset mode */
    rm_int(0x10, &r);
}



/* SetPaletteData:
 *  Palette setting routine.  Currently ignores waitVRT.
 */
void SetPaletteData(AF_DRIVER *af, AF_PALETTE *pal, long num, long index, long waitVRT)
{
    DISPATCH_DATA d;

    d.seg[0] = _my_ds();
    d.off[0] = (unsigned int)pal;
    d.i[0] = num;
    d.i[1] = index;

    if(DispatchCall)
	DispatchCall(dllHandle, VBEAF_SET_PALETTE, &d);
}



/* SetCursor:
 *  Sets the hardware cursor shape.
 */
void SetCursor(AF_DRIVER *af, AF_CURSOR *cursor)
{
    DISPATCH_DATA d;

    d.seg[0] = _my_ds();
    d.off[0] = (unsigned int)cursor;

    if(DispatchCall)
	DispatchCall(dllHandle, VBEAF_SET_CURSOR_SHAPE, &d);
}



/* SetCursorPos:
 *  Sets the hardware cursor position.
 */
void SetCursorPos(AF_DRIVER *af, long x, long y)
{
    DISPATCH_DATA d;

    d.i[0] = x;
    d.i[1] = y;

    if(DispatchCall)
	DispatchCall(dllHandle, VBEAF_SET_CURSOR_POS, &d);
}



/* ShowCursor:
 *  Turns the hardware cursor on or off.
 */
void ShowCursor(AF_DRIVER *af, long visible)
{
    DISPATCH_DATA d;

    d.i[0] = visible;

    if(DispatchCall)
	DispatchCall(dllHandle, VBEAF_SHOW_CURSOR, &d);
}



/* SetMix:
 *  Specifies the pixel mix mode to be used for hardware drawing functions
 *  (not the blit routines: they take an explicit mix parameter). Both
 *  parameters should be one of the AF_mixModes enum defined in vbeaf.h.
 *
 *  VBE/AF requires all drivers to support the REPLACE, AND, OR, XOR,
 *  and NOP foreground mix types, and a background mix of zero (same as
 *  the foreground) or NOP. This file implements all the required types, 
 *  but Allegro only actually uses the REPLACE and XOR modes for scanline 
 *  and rectangle fills, REPLACE mode for blitting and color pattern 
 *  drawing, and either REPLACE or foreground REPLACE and background NOP 
 *  for mono pattern drawing.
 *
 *  If you want, you can set the afHaveROP2 bit in the mode attributes
 *  field and then implement all the AF_R2_* modes as well, but that isn't
 *  required by the spec, and Allegro never uses them.
 */
void SetMix(AF_DRIVER *af, long foreMix, long backMix)
{
   af_fore_mix = foreMix;

   if (backMix == AF_FORE_MIX)
      af_back_mix = foreMix;
   else
      af_back_mix = backMix;
}



/* DrawScan:
 *  Fills a scanline in the current foreground mix mode. Draws up to but
 *  not including the second x coordinate. If the second coord is less
 *  than the first, they are swapped. If they are equal, nothing is drawn.
 */
void DrawScan(AF_DRIVER *af, long color, long y, long x1, long x2)
{
}



/* BitBlt:
 *  Blits from one part of video memory to another, using the specified
 *  mix operation. This must correctly handle the case where the two
 *  regions overlap.
 */
void BitBlt(AF_DRIVER *af, long left, long top, long width, long height, long dstLeft, long dstTop, long op)
{
}



/* BitBltSys:
 *  Copies an image from system memory onto the screen.
 */
void BitBltSys(AF_DRIVER *af, void *srcAddr, long srcPitch, long srcLeft, long srcTop, long width, long height, long dstLeft, long dstTop, long op)
{
    DISPATCH_DATA d;

    d.seg[0] = _my_ds();
    d.off[0] = (unsigned int)srcAddr;
    d.i[0] = srcPitch;
    d.i[1] = srcLeft;
    d.i[2] = srcTop;
    d.i[3] = width;
    d.i[4] = height;
    d.i[5] = dstLeft;
    d.i[6] = dstTop;
    d.i[7] = op;

    if(DispatchCall)
	DispatchCall(dllHandle, VBEAF_BITBLT_SYS, &d);
}

