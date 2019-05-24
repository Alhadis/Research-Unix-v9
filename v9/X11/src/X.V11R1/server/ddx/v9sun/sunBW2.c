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


/*-
 * Copyright (c) 1987 by Sun Microsystems,  Inc.
 */

#include    "sun.h"
#include    "resource.h"

#include    <sys/mman.h>

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
    switch (on) {
    case SCREEN_SAVER_FORCER:
	SetTimeSinceLastInputEvent();
    case SCREEN_SAVER_OFF:
	screenSaved = FALSE;
	(void) ioctl(sunFbs[pScreen->myNum].fd, FBIOVIDON, 0);
	break;
    case SCREEN_SAVER_ON:
    default:
	screenSaved = TRUE;
	(void) ioctl(sunFbs[pScreen->myNum].fd, FBIOVIDOFF, 0);
	break;
    }
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
	char *bitmap = (char *)NULL;
	char *valloc();

	if ((fd = sunOpenFrameBuffer(FBTYPE_SUN2BW, &fbType, index, fbNum,
				     argc, argv)) < 0) {
	    sunFbData[fbNum].probeStatus = probedAndFailed;
	    return FALSE;
	}

        bitmap = valloc(fbType.fb_size);
	if (bitmap == (char *) NULL) {
	    ErrorF("Could not allocate room for frame buffer.\n");
	    sunFbData[fbNum].probeStatus = probedAndFailed;
	    (void) close(fd);
	    return FALSE;
	}
	if (mmap(bitmap, fbType.fb_size, PROT_READ|PROT_WRITE, fd, 0) < 0) {
	    ErrorF("Mapping bw2");
	    sunFbData[fbNum].probeStatus = probedAndFailed;
	    (void) close(fd);
	    return FALSE;
	}

	sunFbs[index].fb = (pointer) bitmap;
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

