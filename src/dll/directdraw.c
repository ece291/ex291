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
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#include "ex291srv.h"
#include <ddrawex.h>

static IDirectDraw3 *pDD3 = NULL;
static IDirectDrawFactory *pDDF = NULL;
static IDirectDrawSurface3 *pDDSPrimary = NULL;
static IDirectDrawSurface3 *pDDSBack = NULL;
static RECT screenRect;
static BOOL samePixelFormat = FALSE;
static PVOID BackBuf32 = NULL;	// these two pointers used when
static PVOID BackBuf24 = NULL;	// samePixelFormat = FALSE
int BackBufWidth = 0;
int BackBufHeight = 0;

extern PBYTE WindowedMode;

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

WORD DDraw_GetFreeMemory(VOID)
{
	DDSCAPS dispcaps;
	DWORD memamt = 0;
	int retval;

	if(!pDD3)
		return 0xFFFF;

	dispcaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
	retval = IDirectDraw3_GetAvailableVidMem(pDD3, &dispcaps, NULL,
		&memamt);
	if (retval != DD_OK) {
		MessageBox(NULL, "Error in GetFreeMemory",
			"Extra BIOS Services for ECE 291",
			MB_OK | MB_ICONERROR);
		return 0xFFFE;
	}

	return (WORD)(memamt/65536);
}

WORD DDraw_SetMode(int width, int height, PVOID backbufmem)
{
	HRESULT hr;
	DDSURFACEDESC ddsd;
	DDSCAPS ddscaps;
	IDirectDrawSurface *pSurface;

	BackBufWidth = width;
	BackBufHeight = height;

/*	LogMessage("SetMode parameters: w=%i, h=%i, d=%i, bbmem=0x%x",
		width, height, 32, (unsigned int)backbufmem);*/
	if(*WindowedMode) {
		hr = IDirectDraw3_SetCooperativeLevel(pDD3, GetMyWindow(),
			DDSCL_NORMAL);
		if(hr != DD_OK)
			return 0xFFFF;
	} else {
		hr = IDirectDraw3_SetCooperativeLevel(pDD3, GetMyWindow(),
			DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN);
		if(hr != DD_OK)
			return 0xFFFF;

		hr = IDirectDraw3_SetDisplayMode(pDD3, width, height, 32, 0, 0);
		if(hr != DD_OK) {
			hr = IDirectDraw3_SetDisplayMode(pDD3, width, height, 24, 0, 0);
			if(hr != DD_OK)
				return 0xFFFF;
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
		return 0xFFFF;

	hr = IDirectDrawSurface_QueryInterface(pSurface,
		&IID_IDirectDrawSurface3, (LPVOID *)&pDDSPrimary);
	if(hr != DD_OK)
		return 0xFFFF;

	IDirectDrawSurface_Release(pSurface);

	hr = IDirectDrawSurface3_GetSurfaceDesc(pDDSPrimary, &ddsd);
	if (hr != DD_OK)
		return 0xFFFF;
	samePixelFormat = (ddsd.ddpfPixelFormat.dwFlags == DDPF_RGB) &&
		(ddsd.ddpfPixelFormat.dwRGBBitCount == 32);

	if (*WindowedMode) {
		LPDIRECTDRAWCLIPPER pClipper;

// Create a clipper to ensure that our drawing stays inside our window
		hr = IDirectDraw3_CreateClipper(pDD3, 0, &pClipper, NULL);
		if(hr != DD_OK)
		{
			IDirectDrawSurface3_Release(pDDSPrimary);
			return 0xFFFF;
		}

		// setting it to our hwnd gives the clipper the coordinates from our window
		hr = IDirectDrawClipper_SetHWnd(pClipper, 0, GetMyWindow());
		if(hr != DD_OK)
		{
			IDirectDrawClipper_Release(pClipper);
			IDirectDrawSurface3_Release(pDDSPrimary);
			return 0xFFFF;
		}

		// attach the clipper to the primary surface
		hr = IDirectDrawSurface3_SetClipper(pDDSPrimary, pClipper);
		if(hr != DD_OK)
		{
			IDirectDrawClipper_Release(pClipper);
			IDirectDrawSurface3_Release(pDDSPrimary);
			return 0xFFFF;
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
			return 0xFFFF;

		hr = IDirectDrawSurface_QueryInterface(pSurface,
			&IID_IDirectDrawSurface3, (LPVOID *)&pDDSBack);
		if(hr != DD_OK)
			return 0xFFFF;

		IDirectDrawSurface_Release(pSurface);

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
//		LogMessage("Starting 24-bit compatibility");
		BackBuf32 = backbufmem;
		BackBuf24 = malloc(width*height*3+16);
		
		if (!BackBuf24) {
			LogMessage("Error allocating space for 24-bit buffer");
			return 0xFFFF;
		}
//		LogMessage("Creating 24-bit surface");

		memset(&ddsd, 0, sizeof(ddsd));
		ddsd.dwSize = sizeof(ddsd);
		ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
		ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
		ddsd.dwWidth = width;
		ddsd.dwHeight = height;

		hr = IDirectDraw3_CreateSurface(pDD3, &ddsd, &pSurface, NULL);
		if(hr != DD_OK)
			return 0xFFFF;

		hr = IDirectDrawSurface_QueryInterface(pSurface,
			&IID_IDirectDrawSurface3, (LPVOID *)&pDDSBack);
		if(hr != DD_OK)
			return 0xFFFF;

		IDirectDrawSurface_Release(pSurface);

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

//	LogMessage("Finished surface setup");

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

	hr = IDirectDraw3_RestoreDisplayMode(pDD3);
	if(hr != DD_OK)
		return 0xFFFF;

	hr = IDirectDraw3_SetCooperativeLevel(pDD3, GetMyWindow(), DDSCL_NORMAL);
	if(hr != DD_OK)
		return 0xFFFF;

	return 0;
}

VOID DDraw_RefreshScreen(VOID)
{
	HRESULT hr;
	DDSURFACEDESC ddsd;

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

