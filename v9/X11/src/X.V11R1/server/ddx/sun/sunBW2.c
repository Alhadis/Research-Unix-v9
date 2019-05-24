/*-
 * sunBW2.c --
 *	Functions for handling the sun BWTWO board.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 * Copyright (c) 1987 by Adam de Boor, UC Berkeley
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
 */

/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or MIT not be used in
advertising or publicity pertaining to distribution  of  the
software  without specific prior written permission. Sun and
M.I.T. make no representations about the suitability of this
software for any purpose. It is provided "as is" without any
express or implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/


#ifndef	lint
static char sccsid[] = "%W %G Copyright 1987 Sun Micro";
#endif

/*-
 * Copyright (c) 1987 by Sun Microsystems,  Inc.
 */

#include    "sun.h"
#include    "resource.h"

#include    <sys/mman.h>
#include    <sundev/bw2reg.h>

extern caddr_t mmap();

typedef struct bw2 {
    u_char	image[BW2_FBSIZE];          /* Pixel buffer */
} BW2, BW2Rec, *BW2Ptr;

typedef struct bw2hr {
    u_char	image[BW2_FBSIZE_HIRES];          /* Pixel buffer */
} BW2HR, BW2HRRec, *BW2HRPtr;


/*-
 *-----------------------------------------------------------------------
 * sunBW2SaveScreen --
 *	Disable the video on the frame buffer to save the screen.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Video enable state changes.
 *
 *-----------------------------------------------------------------------
 */
static Bool
sunBW2SaveScreen (pScreen, on)
    ScreenPtr	  pScreen;
    Bool    	  on;
{
    int         state = on;

    switch (on) {
    case SCREEN_SAVER_FORCER:
	SetTimeSinceLastInputEvent();
	screenSaved = FALSE;
	state = FBVIDEO_ON;
	break;
    case SCREEN_SAVER_OFF:
	screenSaved = FALSE;
	state = FBVIDEO_ON;
	break;
    case SCREEN_SAVER_ON:
    default:
	screenSaved = TRUE;
	state = FBVIDEO_OFF;
	break;
    }
    (void) ioctl(sunFbs[pScreen->myNum].fd, FBIOSVIDEO, &state);
    return TRUE;
}

/*-
 *-----------------------------------------------------------------------
 * sunBW2CloseScreen --
 *	called to ensure video is enabled when server exits.
 *
 * Results:
 *	Screen is unsaved.
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static Bool
sunBW2CloseScreen(i, pScreen)
    int		i;
    ScreenPtr	pScreen;
{
    return (pScreen->SaveScreen(pScreen, SCREEN_SAVER_OFF));
}

/*-
 *-----------------------------------------------------------------------
 * sunBW2ResolveColor --
 *	Resolve an RGB value into some sort of thing we can handle.
 *	Just looks to see if the intensity of the color is greater than
 *	1/2 and sets it to 'white' (all ones) if so and 'black' (all zeroes)
 *	if not.
 *
 * Results:
 *	*pred, *pgreen and *pblue are overwritten with the resolved color.
 *
 * Side Effects:
 *	see above.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
sunBW2ResolveColor(pred, pgreen, pblue, pVisual)
    unsigned short	*pred;
    unsigned short	*pgreen;
    unsigned short	*pblue;
    VisualPtr		pVisual;
{
    /* 
     * Gets intensity from RGB.  If intensity is >= half, pick white, else
     * pick black.  This may well be more trouble than it's worth.
     */
    *pred = *pgreen = *pblue = 
        (((39L * *pred +
           50L * *pgreen +
           11L * *pblue) >> 8) >= (((1<<8)-1)*50)) ? ~0 : 0;
}

/*-
 *-----------------------------------------------------------------------
 * sunBW2CreateColormap --
 *	create a bw colormap
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	allocate two pixels
 *
 *-----------------------------------------------------------------------
 */
void
sunBW2CreateColormap(pmap)
    ColormapPtr	pmap;
{
    int	red, green, blue, pix;

    /* this is a monochrome colormap, it only has two entries, just fill
     * them in by hand.  If it were a more complex static map, it would be
     * worth writing a for loop or three to initialize it */

    /* this will be pixel 0 */
    red = green = blue = ~0;
    AllocColor(pmap, &red, &green, &blue, &pix, 0);

    /* this will be pixel 1 */
    red = green = blue = 0;
    AllocColor(pmap, &red, &green, &blue, &pix, 0);

}

/*-
 *-----------------------------------------------------------------------
 * sunBW2DestroyColormap --
 *	destroy a bw colormap
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
sunBW2DestroyColormap(pmap)
    ColormapPtr	pmap;
{
}

/*-
 *-----------------------------------------------------------------------
 * sunBW2Init --
 *	Attempt to find and initialize a bw2 framebuffer
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Most of the elements of the ScreenRec are filled in.  The
 *	video is enabled for the frame buffer...
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static Bool
sunBW2Init (index, pScreen, argc, argv)
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    ScreenPtr	  pScreen;  	/* The Screen to initialize */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    ColormapPtr pColormap;

    if (!mfbScreenInit(index, pScreen,
			   sunFbs[index].fb,
			   sunFbs[index].info.fb_width,
			   sunFbs[index].info.fb_height, 90, 90))
	return (FALSE);

    pScreen->SaveScreen = sunBW2SaveScreen;
    pScreen->RealizeCursor = sunRealizeCursor;
    pScreen->UnrealizeCursor = sunUnrealizeCursor;
    pScreen->DisplayCursor = sunDisplayCursor;
    pScreen->SetCursorPosition = sunSetCursorPosition;
    pScreen->CursorLimits = sunCursorLimits;
    pScreen->PointerNonInterestBox = sunPointerNonInterestBox;
    pScreen->ConstrainCursor = sunConstrainCursor;
    pScreen->RecolorCursor = sunRecolorCursor;
    pScreen->ResolveColor = sunBW2ResolveColor;
    pScreen->CreateColormap = sunBW2CreateColormap;
    pScreen->DestroyColormap = sunBW2DestroyColormap;
    pScreen->RegionCreate = miRegionCreate;
    pScreen->RegionCopy = miRegionCopy;
    pScreen->RegionDestroy = miRegionDestroy;
    pScreen->Intersect = miIntersect;
    pScreen->Inverse = miInverse;
    pScreen->Union = miUnion;
    pScreen->Subtract = miSubtract;
    pScreen->RegionReset = miRegionReset;
    pScreen->TranslateRegion = miTranslateRegion;
    pScreen->RectIn = miRectIn;
    pScreen->PointInRegion = miPointInRegion;
    pScreen->whitePixel = 0;
    pScreen->blackPixel = 1;

/*
 * ZOIDS should only ever be defined if SUN_WINDOWS is also defined.
 */
#ifdef ZOIDS
    {
	GCPtr	pGC = CreateScratchGC(pScreen, 1);

	if (pGC) {
	    RegisterProc("PolySolidXAlignedTrapezoid", pGC,
			  sunBW2SolidXZoids);
	    RegisterProc("PolySolidYAlignedTrapezoid", pGC,
			  sunBW2SolidYZoids);
	    RegisterProc("PolyTiledXAlignedTrapezoid", pGC,
			  sunBW2TiledXZoids);
	    RegisterProc("PolyTiledYAlignedTrapezoid", pGC,
			  sunBW2TiledYZoids);
	    RegisterProc("PolyStipXAlignedTrapezoid", pGC,
			  sunBW2StipXZoids);
	    RegisterProc("PolyStipYAlignedTrapezoid", pGC,
			  sunBW2StipYZoids);
	    FreeScratchGC(pGC);
	}
    }
#endif ZOIDS

    if (CreateColormap(pScreen->defColormap, pScreen,
		   LookupID(pScreen->rootVisual, RT_VISUALID, RC_CORE),
		   &pColormap, AllocNone, 0) != Success
	|| pColormap == NULL)
	    FatalError("Can't create colormap in sunBW2Init()\n");
    mfbInstallColormap(pColormap);

    /*
     * Enable video output...? 
     */
    (void) sunBW2SaveScreen(pScreen, SCREEN_SAVER_FORCER);

    sunScreenInit(pScreen);
    return (TRUE);

}

/*-
 *-----------------------------------------------------------------------
 * sunBW2Probe --
 *	Attempt to find and initialize a bw2 framebuffer
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Memory is allocated for the frame buffer and the buffer is mapped. 
 *
 *-----------------------------------------------------------------------
 */

Bool
sunBW2Probe(pScreenInfo, index, fbNum, argc, argv)
    ScreenInfo	  *pScreenInfo;	/* The screenInfo struct */
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    int	    	  fbNum;    	/* Index into the sunFbData array */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    int         i, oldNumScreens;

    if (sunFbData[fbNum].probeStatus == probedAndFailed) {
	return FALSE;
    }

    if (sunFbData[fbNum].probeStatus == neverProbed) {
	int         fd;
	struct fbtype fbType;
	BW2Ptr      BW2fb = NULL;	/* Place to map the thing */
	BW2HRPtr    BW2HRfb = NULL;	/* Place to map the thing */
	int         isHiRes = 0;

	if ((fd = sunOpenFrameBuffer(FBTYPE_SUN2BW, &fbType, index, fbNum,
				     argc, argv)) < 0) {
	    sunFbData[fbNum].probeStatus = probedAndFailed;
	    return FALSE;
	}

	isHiRes = (fbType.fb_width > 1152);
#ifdef	_MAP_NEW
	if (isHiRes) {
	    BW2HRfb = (BW2HRPtr) mmap((caddr_t) 0, sizeof(BW2HRRec),
			   PROT_READ | PROT_WRITE,
			   MAP_SHARED | _MAP_NEW,
			   fd, (off_t) 0);
	    if ((int)BW2HRfb == -1) {
		Error("mapping BW2 (hires)");
		sunFbData[fbNum].probeStatus = probedAndFailed;
		(void) close(fd);
		return FALSE;
	    }
	}
	else {
	    BW2fb = (BW2Ptr) mmap((caddr_t) 0, sizeof(BW2Rec),
			 PROT_READ | PROT_WRITE,
			 MAP_SHARED | _MAP_NEW,
			 fd, (off_t) 0);
	    if ((int)BW2fb == -1) {
		Error("mapping BW2");
		sunFbData[fbNum].probeStatus = probedAndFailed;
		(void) close(fd);
		return FALSE;
	    }
	}
#else
	if (isHiRes) {
	    BW2HRfb = (BW2HRPtr) valloc(sizeof(BW2HRRec));
	}
	else {
	    BW2fb = (BW2Ptr) valloc(sizeof(BW2Rec));
	}
	if ((BW2fb == (BW2Ptr) NULL) && (BW2HRfb == (BW2HRPtr) NULL)) {
	    ErrorF("Could not allocate room for frame buffer.\n");
	    sunFbData[fbNum].probeStatus = probedAndFailed;
	    (void) close(fd);
	    return FALSE;
	}
	if (mmap((isHiRes ? (pointer) BW2HRfb : (pointer) BW2fb),
		 (isHiRes ? sizeof(BW2HRRec) : sizeof(BW2Rec)),
		 PROT_READ | PROT_WRITE, MAP_SHARED,
		 fd, (off_t) 0) < 0) {
	    ErrorF("Mapping bw2");
	    sunFbData[fbNum].probeStatus = probedAndFailed;
	    (void) close(fd);
	    return FALSE;
	}
#endif	_MAP_NEW

	/*
	 * ZOIDS should only ever be defined if SUN_WINDOWS is also
	 * defined.
	 */
#ifdef ZOIDS
	if ((sunFbData[fbNum].pr = pr_open(sunFbData[fbNum].devName)) == 0) {
	    ErrorF("Opening bw2 pixrect");
	    sunFbData[fbNum].probeStatus = probedAndFailed;
	    /* do we need to free BW2fb or BW2HRfb? */
	    (void) close(fd);
	    return FALSE;
	}

	if ((sunFbData[fbNum].scratch_pr = mem_create(
			   fbType.fb_width, fbType.fb_height, 1)) == 0) {
	    ErrorF("Opening bw2 scratch pixrect");
	    sunFbData[fbNum].probeStatus = probedAndFailed;
	    /* do we need to free BW2fb or BW2HRfb? */
	    pr_destroy(sunFbData[fbNum].pr);
	    (void) close(fd);
	    return FALSE;
	}
#endif ZOIDS
	sunFbs[index].fb = (isHiRes ? (pointer) BW2HRfb : (pointer) BW2fb);
	sunFbs[index].fd = fd;
	sunFbs[index].info = fbType;
        sunFbs[index].EnterLeave = NoopDDA;
	sunFbData[fbNum].probeStatus = probedAndSucceeded;

    }

    /*
     * If we've ever successfully probed this device, do the following.
     */
    oldNumScreens = pScreenInfo->numScreens;
    i = AddScreen(sunBW2Init, argc, argv);
    pScreenInfo->screen[index].CloseScreen = sunBW2CloseScreen;
    return (i > oldNumScreens);
}

