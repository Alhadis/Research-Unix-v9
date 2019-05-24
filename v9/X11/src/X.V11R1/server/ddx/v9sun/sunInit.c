/*-
 * sunInit.c --
 *	Initialization functions for screen/keyboard/mouse, etc.
 *
 * Copyright (c) 1987 by the Regents of the University of California
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

#include    "sun.h"
#include    <servermd.h>
#include    "dixstruct.h"
#include    "dix.h"
#include    "opaque.h"

extern int sunMouseProc();
extern void sunKbdProc();
extern Bool sunBW2Probe();

extern void SetInputCheck();
extern GCPtr CreateScratchGC();

	/* What should this *really* be? */
#define MOTION_BUFFER_SIZE 0

sunFbDataRec sunFbData[] = {
    sunBW2Probe,  	"/dev/bwtwo0",	    neverProbed,
};

/*
 * NUMSCREENS is the number of supported frame buffers (i.e. the number of
 * structures in sunFbData which have an actual probeProc).
 */
#define NUMSCREENS (sizeof(sunFbData)/sizeof(sunFbData[0]))

fbFd	sunFbs[NUMSCREENS];  /* Space for descriptors of open frame buffers */

static PixmapFormatRec	formats[] = {
    1, 1, BITMAP_SCANLINE_PAD,	/* 1-bit deep */
};
#define NUMFORMATS	(sizeof formats)/(sizeof formats[0])

/*-
 *-----------------------------------------------------------------------
 * InitOutput --
 *	Initialize screenInfo for all actually accessible framebuffers.
 *	The
 *
 * Results:
 *	screenInfo init proc field set
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */

InitOutput(pScreenInfo, argc, argv)
    ScreenInfo 	  *pScreenInfo;
    int     	  argc;
    char    	  **argv;
{
    int     	  i, index, ac = argc;

    pScreenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    pScreenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    pScreenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    pScreenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;

    pScreenInfo->numPixmapFormats = NUMFORMATS;
    for (i=0; i< NUMFORMATS; i++)
    {
        pScreenInfo->formats[i] = formats[i];
    }

    for (i = 0, index = 0; i < NUMSCREENS; i++) {
	if ((* sunFbData[i].probeProc) (pScreenInfo, index, i, argc, argv)) {
	    /* This display exists OK */
	    index++;
	} else {
	    /* This display can't be opened */
	    ;
	}
    }
    if (index == 0)
	FatalError("Can't find any displays\n");

    pScreenInfo->numScreens = index;

    sunInitCursor();
}

/*-
 *-----------------------------------------------------------------------
 * InitInput --
 *	Initialize all supported input devices...what else is there
 *	besides pointer and keyboard?
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Two DeviceRec's are allocated and registered as the system pointer
 *	and keyboard devices.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
InitInput(argc, argv)
    int     	  argc;
    char    	  **argv;
{
    DevicePtr p, k;
    static int  zero = 0;
    
    p = AddInputDevice(sunMouseProc, TRUE);
    k = AddInputDevice(sunKbdProc, TRUE);

    RegisterPointerDevice(p, MOTION_BUFFER_SIZE);
    RegisterKeyboardDevice(k);

    SetInputCheck (&zero, &isItTimeToYield);
}

/*-
 *-----------------------------------------------------------------------
 * sunQueryBestSize --
 *	Supposed to hint about good sizes for things.
 *
 * Results:
 *	Perhaps change *pwidth (Height irrelevant)
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
sunQueryBestSize(class, pwidth, pheight)
int class;
short *pwidth;
short *pheight;
{
    unsigned width, test;

    switch(class)
    {
      case CursorShape:
      case TileShape:
      case StippleShape:
	  width = *pwidth;
	  if (width > 0) {
	      /* Return the closes power of two not less than what they gave me */
	      test = 0x80000000;
	      /* Find the highest 1 bit in the width given */
	      while(!(test & width))
		 test >>= 1;
	      /* If their number is greater than that, bump up to the next
	       *  power of two */
	      if((test - 1) & width)
		 test <<= 1;
	      *pwidth = test;
	  }
	  /* We don't care what height they use */
	  break;
    }
}

/*-
 *-----------------------------------------------------------------------
 * sunScreenInit --
 *	Things which must be done for all types of frame buffers...
 *	Should be called last of all.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The graphics context for the screen is created. The CreateGC,
 *	CreateWindow and ChangeWindowAttributes vectors are changed in
 *	the screen structure.
 *
 *	Both a BlockHandler and a WakeupHandler are installed for the
 *	first screen.  Together, these handlers implement autorepeat
 *	keystrokes on the Sun.
 *
 *-----------------------------------------------------------------------
 */
void
sunScreenInit (pScreen)
    ScreenPtr	  pScreen;
{
    fbFd    	  *fb;
    DrawablePtr	  pDrawable;
 
    fb = &sunFbs[pScreen->myNum];

    /*
     * Prepare the GC for cursor functions on this screen.
     * Do this before setting interceptions to avoid looping when
     * putting down the cursor...
     */
    pDrawable = (DrawablePtr)(pScreen->devPrivate);

    fb->pGC = CreateScratchGC (pDrawable->pScreen, pDrawable->depth);

    /*
     * By setting graphicsExposures false, we prevent any expose events
     * from being generated in the CopyArea requests used by the cursor
     * routines.
     */
    fb->pGC->graphicsExposures = FALSE;

    /*
     * Preserve the "regular" functions
     */
    fb->CreateGC =	    	    	pScreen->CreateGC;
    fb->CreateWindow = 	    	    	pScreen->CreateWindow;
    fb->ChangeWindowAttributes =    	pScreen->ChangeWindowAttributes;
    fb->GetImage =	    	    	pScreen->GetImage;
    fb->GetSpans =			pScreen->GetSpans;

    /*
     * Interceptions
     */
    pScreen->CreateGC =	    	    	sunCreateGC;
    pScreen->CreateWindow = 	    	sunCreateWindow;
    pScreen->ChangeWindowAttributes = 	sunChangeWindowAttributes;
    pScreen->QueryBestSize =		sunQueryBestSize;
    pScreen->GetImage =	    	    	sunGetImage;
    pScreen->GetSpans =			sunGetSpans;

    /*
     * Cursor functions
     */
    pScreen->RealizeCursor = 	    	sunRealizeCursor;
    pScreen->UnrealizeCursor =	    	sunUnrealizeCursor;
    pScreen->DisplayCursor = 	    	sunDisplayCursor;
    pScreen->SetCursorPosition =    	sunSetCursorPosition;
    pScreen->CursorLimits = 	    	sunCursorLimits;
    pScreen->PointerNonInterestBox = 	sunPointerNonInterestBox;
    pScreen->ConstrainCursor = 	    	sunConstrainCursor;
    pScreen->RecolorCursor = 	    	sunRecolorCursor;

}

/*-
 *-----------------------------------------------------------------------
 * sunOpenFrameBuffer --
 *	Open a frame buffer.
 *
 * Results:
 *	The fd of the framebuffer.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
int
sunOpenFrameBuffer(expect, pfbType, index, fbNum, argc, argv)
    int	    	  expect;   	/* The expected type of framebuffer */
    struct fbtype *pfbType; 	/* Place to store the fb info */
    int	    	  fbNum;    	/* Index into the sunFbData array */
    int	    	  index;    	/* Screen index */
    int	    	  argc;	    	/* Command-line arguments... */
    char	  **argv;   	/* ... */
{
    char       	  *name;
    int           fd = -1;	    	/* Descriptor to device */

	name = "/dev/fb";
	fd = open(name, 2);
        if (fd < 0) {
	    return (-1);
	} 
	if (ioctl(fd, FBIOGTYPE, pfbType) < 0) {
	    perror("sunOpenFrameBuffer");
	    (void) close(fd);
	    return (-1);
	}
	if (pfbType->fb_type != expect) {
	    (void) close(fd);
	    return (-1);
	}
	return (fd);
}
