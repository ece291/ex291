/* directdraw.c -- DirectDraw interface
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

   $Id: directdraw.c,v 1.3 2001/03/19 08:45:28 pete Exp $
*/

#include "ex291srv.h"
#include <ddrawex.h>

#define	MAX_MODES	128

static IDirectDraw3 *pDD3 = NULL;
static IDirectDrawFactory *pDDF = NULL;
static IDirectDrawSurface3 *pDDSPrimary = NULL;
static IDirectDrawSurface3 *pDDSBack = NULL;
static RECT screenRect;
static BOOL samePixelFormat = FALSE;
static PVOID BackBuf32 = NULL;	// these two pointers used when
static PVOID BackBuf24 = NULL;	// samePixelFormat = FALSE
static PVOID BackBuf = NULL;
int BackBufWidth = 0;
int BackBufHeight = 0;

extern PBYTE WindowedMode;

extern BOOL usingVBEAF;
extern int VBEAF_depth;

typedef struct MODELIST_DATA
{
	DISPATCH_DATA *data;
	WORD cb_seg;
	WORD cb_off;
	int num_truecolor;
	struct {
		unsigned int width;
		unsigned int height;
		int is24:1;
		int is32:1;
	} truecolor_modes[MAX_MODES];
} MODELIST_DATA;

BOOL InitDirectDraw(VOID)
{
	HRESULT hr;
	IDirectDraw *pDD;

	CoInitialize(NULL);

	CoCreateInstance(&CLSID_DirectDrawFactory, NULL, CLSCTX_INPROC_SERVER,
		&IID_IDirectDrawFactory, (void **)&pDDF);

	hr = pDDF->lpVtbl->CreateDirectDraw(pDDF, NULL, GetDesktopWindow(),
		DDSCL_NORMAL, 0, NULL, &pDD);
	if(hr != DD_OK)
		return FALSE;

	hr = IDirectDraw_QueryInterface(pDD, &IID_IDirectDraw3, (LPVOID *)&pDD3);
	if(hr != DD_OK)
		return FALSE;

	IDirectDraw_Release(pDD);

	return TRUE;
}

VOID CloseDirectDraw(VOID)
{
	IDirectDraw3_Release(pDD3);
	pDDF->lpVtbl->Release(pDDF);

	CoUninitialize();
}

DWORD DDraw_GetFreeMemory(VOID)
{
	DDSCAPS dispcaps;
	DWORD memamt = 0;
	int retval;

	if(!pDD3)
		return 0xFFFFFFFF;

	dispcaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
	retval = IDirectDraw3_GetAvailableVidMem(pDD3, &dispcaps, NULL,
		&memamt);
	if (retval != DD_OK) {
		MessageBox(NULL, "Error in GetFreeMemory",
			"Extra BIOS Services for ECE 291",
			MB_OK | MB_ICONERROR);
		return 0xFFFEFFFF;
	}

	return memamt;
}

BOOL DDraw_GetModelist(WORD cb_seg, WORD cb_off, DISPATCH_DATA *data)
{
	MODELIST_DATA mldata;
	WORD cs, ip;
	int retval;
	int i;

	if(!pDD3)
		return FALSE;

	cs = getCS();
	ip = getIP();

	mldata.data = data;
	mldata.cb_seg = cb_seg;
	mldata.cb_off = cb_off;
	mldata.num_truecolor = 0;

	retval = IDirectDraw3_EnumDisplayModes(pDD3, 0, NULL, &mldata,
		&DDraw_GetModelist_Callback);
	if(retval != DD_OK) {
		MessageBox(NULL, "Error in GetModelist",
			"Extra BIOS Services for ECE 291",
			MB_OK | MB_ICONERROR);
		setCS(cs);
		setIP(ip);
		return FALSE;
	}

	// fill in emulated modes
	for(i=0; i<mldata.num_truecolor; i++) {
		if(mldata.truecolor_modes[i].is24 &&
			mldata.truecolor_modes[i].is32)
			continue;
		if(mldata.truecolor_modes[i].is24)
			data->s[0] = 32;
		if(mldata.truecolor_modes[i].is32)
			data->s[0] = 24;
		data->s[1] = (USHORT)mldata.truecolor_modes[i].width;
		data->s[2] = (USHORT)mldata.truecolor_modes[i].width;
		data->s[3] = 1;
		data->i[0] = 0;
		data->i[1] = 0;
		data->i[2] = 0;
		data->i[3] = 0;
		data->i[4] = 0;
		data->i[5] = 0;
		data->i[6] = 0;
		data->i[7] = 0;

		// call DOS callback
/*		LogMessage("W/H/D/E = %d/%d/%d/%d", (unsigned int)data->s[1],
			(unsigned int)data->s[2],
			(unsigned int)data->s[0],
			(unsigned int)data->s[3]);*/
		setCS(cb_seg);
		setIP(cb_off);
		VDDSimulate16();
	}

	setCS(cs);
	setIP(ip);
	return TRUE;
}

HRESULT WINAPI DDraw_GetModelist_Callback(LPDDSURFACEDESC pddsd, LPVOID pc)
{
	MODELIST_DATA *mldata = (MODELIST_DATA *)pc;
	int i;
	int found;

	// reject modes that don't have the info we need
	if(!(pddsd->dwFlags & DDSD_HEIGHT) ||
		!(pddsd->dwFlags & DDSD_WIDTH) ||
		!(pddsd->dwFlags & DDSD_PIXELFORMAT))
		return DDENUMRET_OK;

	// reject non-RGB / non-8-bit-paletted modes
	if(!((pddsd->ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) ||
		(pddsd->ddpfPixelFormat.dwFlags & DDPF_RGB)))
		return DDENUMRET_OK;

	// remember what truecolor modes have been covered
	if(pddsd->ddpfPixelFormat.dwRGBBitCount == 24 ||
		pddsd->ddpfPixelFormat.dwRGBBitCount == 32) {
		i=0;
		found=0;
		while(i<mldata->num_truecolor) {
			if(mldata->truecolor_modes[i].width == pddsd->dwWidth &&
				mldata->truecolor_modes[i].height == pddsd->dwHeight) {
				found = 1;
				break;
			}
			i++;
		}
		if(!found) {
			mldata->num_truecolor++;
			mldata->truecolor_modes[i].width = pddsd->dwWidth;
			mldata->truecolor_modes[i].height = pddsd->dwHeight;
			mldata->truecolor_modes[i].is24 = 0;
			mldata->truecolor_modes[i].is32 = 0;
		}

		if(pddsd->ddpfPixelFormat.dwRGBBitCount == 32)
			mldata->truecolor_modes[i].is32 = 1;
		if(pddsd->ddpfPixelFormat.dwRGBBitCount == 24)
			mldata->truecolor_modes[i].is24 = 1;
	}

	// save mode info
	mldata->data->s[0] = (USHORT)pddsd->ddpfPixelFormat.dwRGBBitCount;
	mldata->data->s[1] = (USHORT)pddsd->dwWidth;
	mldata->data->s[2] = (USHORT)pddsd->dwHeight;
	mldata->data->s[3] = 0;
	mldata->data->i[0] = 0;
	mldata->data->i[1] = 0;
	mldata->data->i[2] = 0;
	mldata->data->i[3] = 0;
	mldata->data->i[4] = 0;
	mldata->data->i[5] = 0;
	mldata->data->i[6] = 0;
	mldata->data->i[7] = 0;

	// call DOS callback
/*	LogMessage("W/H/D/E = %d/%d/%d/%d", (unsigned int)mldata->data->s[1],
		(unsigned int)mldata->data->s[2],
		(unsigned int)mldata->data->s[0],
		(unsigned int)mldata->data->s[3]);*/
	setCS(mldata->cb_seg);
	setIP(mldata->cb_off);
	VDDSimulate16();

	return DDENUMRET_OK;
}

int DDraw_SetMode(int width, int height, int depth)
{
	HRESULT hr;
	DDSURFACEDESC ddsd;
	IDirectDrawSurface *pSurface;

/*	LogMessage("SetMode parameters: w=%i, h=%i, d=%i",
		width, height, depth);*/
	if(*WindowedMode) {
		hr = IDirectDraw3_SetCooperativeLevel(pDD3, GetMyWindow(),
			DDSCL_NORMAL);
		if(hr != DD_OK)
			return -1;
	} else {
		hr = IDirectDraw3_SetCooperativeLevel(pDD3, GetMyWindow(),
			DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
		if(hr != DD_OK)
			return -1;

		hr = IDirectDraw3_SetDisplayMode(pDD3, width, height, depth, 0, 0);
		if(hr != DD_OK) {
			if(depth == 24 || depth == 32) {
				hr = IDirectDraw3_SetDisplayMode(pDD3, width, height, depth==24?32:24, 0, 0);
				if(hr != DD_OK)
					return -1;
			} else {
				return -1;
			}
		}
	}

	SetRect(&screenRect, 0, 0, width, height);

	// Create the primary screen surface
	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_CAPS;
	ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

	hr = IDirectDraw3_CreateSurface(pDD3, &ddsd, &pSurface, NULL);
	if(hr != DD_OK)
		return -1;

	hr = IDirectDrawSurface_QueryInterface(pSurface,
		&IID_IDirectDrawSurface3, (LPVOID *)&pDDSPrimary);
	if(hr != DD_OK)
		return -1;

	IDirectDrawSurface_Release(pSurface);

	hr = IDirectDrawSurface3_GetSurfaceDesc(pDDSPrimary, &ddsd);
	if (hr != DD_OK)
		return -1;
	samePixelFormat = ddsd.ddpfPixelFormat.dwFlags == DDPF_RGB &&
		ddsd.ddpfPixelFormat.dwRGBBitCount == (unsigned int)depth;

	if (*WindowedMode) {
		LPDIRECTDRAWCLIPPER pClipper;

// Create a clipper to ensure that our drawing stays inside our window
		hr = IDirectDraw3_CreateClipper(pDD3, 0, &pClipper, NULL);
		if(hr != DD_OK)
		{
			IDirectDrawSurface3_Release(pDDSPrimary);
			return -1;
		}

		// setting it to our hwnd gives the clipper the coordinates from our window
		hr = IDirectDrawClipper_SetHWnd(pClipper, 0, GetMyWindow());
		if(hr != DD_OK)
		{
			IDirectDrawClipper_Release(pClipper);
			IDirectDrawSurface3_Release(pDDSPrimary);
			return -1;
		}

		// attach the clipper to the primary surface
		hr = IDirectDrawSurface3_SetClipper(pDDSPrimary, pClipper);
		if(hr != DD_OK)
		{
			IDirectDrawClipper_Release(pClipper);
			IDirectDrawSurface3_Release(pDDSPrimary);
			return -1;
		}
	}

	// Create the backbuffer surface
	if (samePixelFormat) {
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
		ddsd.dwWidth = width;
		ddsd.dwHeight = height;

		hr = IDirectDraw3_CreateSurface(pDD3, &ddsd, &pSurface, NULL);
		if(hr != DD_OK)
			return -1;

		hr = IDirectDrawSurface_QueryInterface(pSurface,
			&IID_IDirectDrawSurface3, (LPVOID *)&pDDSBack);
		if(hr != DD_OK)
			return -1;

		IDirectDrawSurface_Release(pSurface);
	} else {
		if(depth == 32) {
			BackBuf24 = malloc(width*height*3+16);
			BackBuf = BackBuf24;
		} else {
			BackBuf32 = malloc(width*height*4+16);
			BackBuf = BackBuf32;
		}
		
		if (!BackBuf) {
			LogMessage("Error allocating space for depth emulation buffer");
			return -1;
		}

		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
		ddsd.dwWidth = width;
		ddsd.dwHeight = height;

		hr = IDirectDraw3_CreateSurface(pDD3, &ddsd, &pSurface, NULL);
		if(hr != DD_OK)
			return -1;

		hr = IDirectDrawSurface_QueryInterface(pSurface,
			&IID_IDirectDrawSurface3, (LPVOID *)&pDDSBack);
		if(hr != DD_OK)
			return -1;

		IDirectDrawSurface_Release(pSurface);
	}

	return 0;
}

WORD DDraw_SetMode_Old(int width, int height, PVOID backbufmem)
{
	HRESULT hr;
	DDSURFACEDESC ddsd;
	IDirectDrawSurface *pSurface;

	BackBufWidth = width;
	BackBufHeight = height;

	if(DDraw_SetMode(width, height, 32))
		return 0xFFFF;

	// Create the backbuffer surface
	if (samePixelFormat) {
		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_LPSURFACE |
			DDSD_PIXELFORMAT;
		ddsd.dwWidth = width;
		ddsd.dwHeight = height;
		ddsd.lPitch = width*4;
		ddsd.lpSurface = backbufmem;
		ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
		ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
		ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
		ddsd.ddpfPixelFormat.dwGBitMask = 0x00FF00;
		ddsd.ddpfPixelFormat.dwBBitMask = 0x0000FF;
//		LogMessage("Setting up back buffer surface");
		hr = IDirectDrawSurface3_SetSurfaceDesc(pDDSBack, &ddsd, 0);
		if(hr != DD_OK) {
			switch(hr) {
			case DDERR_INVALIDPARAMS: LogMessage("InvalidParams"); break;
			case DDERR_INVALIDOBJECT: LogMessage("InvalidObject"); break;
			case DDERR_SURFACELOST: LogMessage("SurfaceLost"); break;
			case DDERR_SURFACEBUSY: LogMessage("SurfaceBusy"); break;
			case DDERR_INVALIDSURFACETYPE: LogMessage("InvalidSurfaceType");
				break;
			case DDERR_INVALIDPIXELFORMAT: LogMessage("InvalidPixelFormat");
				break;
			case DDERR_INVALIDCAPS: LogMessage("InvalidCaps"); break;
			case DDERR_UNSUPPORTED: LogMessage("Unsupported"); break;
			case DDERR_GENERIC: LogMessage("Generic"); break;
			}
			return 0xFFFF;
		}

	} else {
		BackBuf32 = backbufmem;

		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_LPSURFACE |
			DDSD_PIXELFORMAT;
		ddsd.dwWidth = width;
		ddsd.dwHeight = height;
		ddsd.lPitch = width*3;
		ddsd.lpSurface = BackBuf24;
		ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
		ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
		ddsd.ddpfPixelFormat.dwRGBBitCount = 24;
		ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
		ddsd.ddpfPixelFormat.dwGBitMask = 0x00FF00;
		ddsd.ddpfPixelFormat.dwBBitMask = 0x0000FF;
//		LogMessage("Setting up back buffer surface");
		hr = IDirectDrawSurface3_SetSurfaceDesc(pDDSBack, &ddsd, 0);
		if(hr != DD_OK) {
			switch(hr) {
			case DDERR_INVALIDPARAMS: LogMessage("InvalidParams"); break;
			case DDERR_INVALIDOBJECT: LogMessage("InvalidObject"); break;
			case DDERR_SURFACELOST: LogMessage("SurfaceLost"); break;
			case DDERR_SURFACEBUSY: LogMessage("SurfaceBusy"); break;
			case DDERR_INVALIDSURFACETYPE: LogMessage("InvalidSurfaceType");
				break;
			case DDERR_INVALIDPIXELFORMAT: LogMessage("InvalidPixelFormat");
				break;
			case DDERR_INVALIDCAPS: LogMessage("InvalidCaps"); break;
			case DDERR_UNSUPPORTED: LogMessage("Unsupported"); break;
			case DDERR_GENERIC: LogMessage("Generic"); break;
			}
			return 0xFFFF;
		}

	}

	return 0;
}

WORD DDraw_UnSetMode(VOID)
{
	HRESULT hr;

	if(pDDSBack) {
		IDirectDrawSurface3_Release(pDDSBack);
		pDDSBack = NULL;
	}
	if(pDDSPrimary) {
		IDirectDrawSurface3_Release(pDDSPrimary);
		pDDSPrimary = NULL;
	}

	if (BackBuf24) {
		free(BackBuf24);
		BackBuf24 = NULL;
	}

	if(!*WindowedMode) {
		hr = IDirectDraw3_RestoreDisplayMode(pDD3);
		if(hr != DD_OK)
			return 0xFFFF;

		hr = IDirectDraw3_SetCooperativeLevel(pDD3, GetMyWindow(), DDSCL_NORMAL);
		if(hr != DD_OK)
			return 0xFFFF;
	}

	return 0;
}

VOID DDraw_RefreshScreen(VOID)
{
	HRESULT hr;
	DDSURFACEDESC ddsd;

	if(usingVBEAF)
		return;

	if(!pDDSPrimary || !pDDSBack)
		return;
//LogMessage("Start Refresh");	
	if(!samePixelFormat) {
//		LogMessage("Starting 32 to 24 conversion");
		Copy32To24(BackBuf32, BackBuf24, BackBufWidth, BackBufHeight);
//		LogMessage("Finished 32 to 24 conversion");
	}

	if(*WindowedMode) {
		RECT windowRect;
		POINT p;

		p.x = 0; p.y = 0;
	        ClientToScreen(GetMyWindow(), &p);
		windowRect = screenRect;
        	OffsetRect(&windowRect, p.x, p.y);

		hr = IDirectDrawSurface3_Blt(pDDSPrimary, &windowRect, pDDSBack,
			&screenRect, DDBLT_WAIT, NULL);
	} else {
		hr = IDirectDrawSurface3_Blt(pDDSPrimary, &screenRect, pDDSBack,
			&screenRect, DDBLT_WAIT, NULL);
	}
	if(hr != DD_OK) {
		switch(hr) {
		case DDERR_GENERIC: LogMessage("Generic"); break;
		case DDERR_INVALIDCLIPLIST: LogMessage("InvalidClipList"); break;
		case DDERR_INVALIDOBJECT: LogMessage("InvalidObject"); break;
		case DDERR_INVALIDPARAMS: LogMessage("InvalidParams"); break;
		case DDERR_INVALIDRECT: LogMessage("InvalidRect"); break;
		case DDERR_NOBLTHW: LogMessage("NoBltHW"); break;
		case DDERR_UNSUPPORTED: LogMessage("Unsupported"); break;
		default: LogMessage("Other"); break;
		}
	}
//LogMessage("End Refresh");
}

int DDraw_BitBltSys(PVOID source, DISPATCH_DATA *data)
{
	HRESULT hr;
	DDSURFACEDESC ddsd;
	RECT srcRect, destRect;

	if(!usingVBEAF)
		return -1;

	if(!pDDSPrimary || !pDDSBack)
		return -1;

	memset(&ddsd, 0, sizeof(ddsd));
	ddsd.dwSize = sizeof(ddsd);
	ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_LPSURFACE |
		DDSD_PIXELFORMAT;
	ddsd.dwHeight = data->i[4];
	ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
	ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
	ddsd.ddpfPixelFormat.dwGBitMask = 0x00FF00;
	ddsd.ddpfPixelFormat.dwBBitMask = 0x0000FF;

	if (samePixelFormat) {
		srcRect.left = data->i[1];
		srcRect.top = data->i[2];
		ddsd.dwWidth = data->i[0]/(VBEAF_depth/8);
		ddsd.lPitch = data->i[0];
		ddsd.lpSurface = source;
		ddsd.ddpfPixelFormat.dwRGBBitCount = VBEAF_depth;
	} else {
		srcRect.left = 0;
		srcRect.top = 0;
		ddsd.dwWidth = data->i[3];
		ddsd.lpSurface = BackBuf;
		if(VBEAF_depth==24) {
			ddsd.lPitch = data->i[3]*4;
			ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
			Copy24To32_pitched(source, BackBuf, data->i[0],
				data->i[3], data->i[4], data->i[1], data->i[2]);
		} else {
			ddsd.lPitch = data->i[3]*3;
			ddsd.ddpfPixelFormat.dwRGBBitCount = 24;
			Copy32To24_pitched(source, BackBuf, data->i[0],
				data->i[3], data->i[4], data->i[1], data->i[2]);
		}
	}

	srcRect.right = srcRect.left + data->i[3];
	srcRect.bottom = srcRect.top + data->i[4];

	// calculate destination rectangle
	destRect.left = data->i[5];
	destRect.top = data->i[6];
	destRect.right = data->i[5] + data->i[3];
	destRect.bottom = data->i[6] + data->i[4];

	// modify surface parameters
	hr = IDirectDrawSurface3_SetSurfaceDesc(pDDSBack, &ddsd, 0);
	if(hr != DD_OK) {
		switch(hr) {
		case DDERR_INVALIDPARAMS: LogMessage("InvalidParams"); break;
		case DDERR_INVALIDOBJECT: LogMessage("InvalidObject"); break;
		case DDERR_SURFACELOST: LogMessage("SurfaceLost"); break;
		case DDERR_SURFACEBUSY: LogMessage("SurfaceBusy"); break;
		case DDERR_INVALIDSURFACETYPE: LogMessage("InvalidSurfaceType");
			break;
		case DDERR_INVALIDPIXELFORMAT: LogMessage("InvalidPixelFormat");
			break;
		case DDERR_INVALIDCAPS: LogMessage("InvalidCaps"); break;
		case DDERR_UNSUPPORTED: LogMessage("Unsupported"); break;
		case DDERR_GENERIC: LogMessage("Generic"); break;
		}
		return -1;
	}

	if(*WindowedMode) {
		RECT windowRect;
		POINT p;

		p.x = 0; p.y = 0;
	        ClientToScreen(GetMyWindow(), &p);
		windowRect = destRect;
        	OffsetRect(&windowRect, p.x, p.y);

		hr = IDirectDrawSurface3_Blt(pDDSPrimary, &windowRect, pDDSBack,
			&srcRect, DDBLT_WAIT, NULL);
	} else {
		hr = IDirectDrawSurface3_Blt(pDDSPrimary, &destRect, pDDSBack,
			&srcRect, DDBLT_WAIT, NULL);
	}
	if(hr != DD_OK) {
		switch(hr) {
		case DDERR_GENERIC: LogMessage("Generic"); break;
		case DDERR_INVALIDCLIPLIST: LogMessage("InvalidClipList"); break;
		case DDERR_INVALIDOBJECT: LogMessage("InvalidObject"); break;
		case DDERR_INVALIDPARAMS: LogMessage("InvalidParams"); break;
		case DDERR_INVALIDRECT: LogMessage("InvalidRect"); break;
		case DDERR_NOBLTHW: LogMessage("NoBltHW"); break;
		case DDERR_UNSUPPORTED: LogMessage("Unsupported"); break;
		default: LogMessage("Other"); break;
		}
		return -1;
	}

	return 0;
}

