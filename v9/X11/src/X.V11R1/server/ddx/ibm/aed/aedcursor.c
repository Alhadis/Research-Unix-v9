/***********************************************************
		Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $Header: aedcursor.c,v 1.1 87/09/13 03:34:06 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/aed/RCS/aedcursor.c,v $ */

#ifndef lint
static char *rcsid = "$Header: aedcursor.c,v 1.1 87/09/13 03:34:06 erik Exp $";
#endif

#include <stdio.h>

#include "X.h"
#include "Xmd.h"
#include "miscstruct.h"
#include "scrnintstr.h"
#include "cursorstr.h"

#include <machinecons/xio.h>

#include "mfb.h"
#include "maskbits.h"

#include "rtio.h"
#include "rtcursor.h"
#include "rtutils.h"

#include "xaed.h"

/***============================================================***/

	/*
	 * NOTES ON THE RT/PC X11 SERVER CURSOR SUPPORT (../librt/rtcursor.c)
	 * The X emulator in the kernel normally tries to track the cursor.
	 * Unfortunately, the kernel aed locator code uses the aed library
	 * code to copy and clear cursor images.  If the X server is
	 * trying to talk to the aed microcode at the same time, it can get
	 * pretty messy.  This means that we have to track the cursor in the
	 * server.   The rt cursor support (in librt) tries to allow for
	 * the server to track the cursor in a fairly generic way.  If the
	 * variable rtSoftCursor is non-zero, every mouse event that gets
	 * reported is also used to update the cursor position.  If the
	 * motion event occurs INSIDE of the BOX rtCursorBounds, *rtCursorX and
	 * *rtCursorY (both are short *) are set to the coordinates of the
	 * event offset by a hotspot (rtCursorHotX and rtCursorHotY).
	 * If the event occurs outside of rtCursorBounds, the function
	 * rtCursorShow() is called with the coordinates of the event.
	 */


/***============================================================***/





/***============================================================***/

	/*
	 * Initialize the apa16cursor package.  Sets up data structures
	 * and such so the RT/PC specific code will track the cursor
	 * correctly.  Called by apa16ScreenInit().
	 *
	 * I had trouble convinving the kernel *not* to track the cursor
	 * automatically.  The magic sequence of ioctls below seems to
	 * do the trick.
	 */

aedCursorInit()
{
int  i,col;
XCursor	xloc,old;

    TRACE(("aedCursorInit()\n"));

    /* Have kernel hide the cursor */
    old= rtXaddr->mouse;
    xloc.x= 2000;
    xloc.y= 800;
    ioctl(rtScreenFD, QIOCSMSTATE, &xloc);
    ioctl(rtScreenFD, QIOCHIDECUR);
    ioctl(rtScreenFD, QIOCSMSTATE, &old);

    /* Convince rtio.c to track the cursor for us */
    rtSoftCursor= TRUE;
    rtCursorX=	(short *)0;
    rtCursorY=	(short *)0;
    rtCursorBounds.x1=	AED_SCREEN_WIDTH+65;
    rtCursorBounds.y1=	AED_SCREEN_HEIGHT+65;
    rtCursorBounds.x2=	AED_SCREEN_WIDTH+65;
    rtCursorBounds.y2=	AED_SCREEN_HEIGHT+65;
    rtCursorShow=	aedShowCursor;

    vforce();
    vikint[1] = 8;	/* enable cursor command */
    command(1);
    clear(2);
    return TRUE;
}


/***============================================================***/

	/* 
	 * realize cursor for aed.
	 */

Bool
aedRealizeCursor( pScr, pCurs )
    ScreenPtr	pScr;
    CursorPtr	pCurs;
{
    int *aedCursor;
    unsigned endbits;
    int srcWidth;
    int srcHeight;
    int srcRealWidth;
    int *pImage, *pMask;
    int *psrcImage, *psrcMask;
    int i;

    TRACE(("aedRealizeCursor( pScr= 0x%x, pCurs= 0x%x)\n",pScr,pCurs));
    if( (pCurs->devPriv[ pScr->myNum ] = (pointer)Xalloc(1024)) == NULL )
	{
	ErrorF("aedRealizeCursor: can't malloc\n");
	return FALSE;
	}
    pMask = aedCursor = (int *)pCurs->devPriv[pScr->myNum];
    pImage = pMask + 128;
    bzero((char *)aedCursor, 1024);
    psrcImage = pCurs->source;
    psrcMask = pCurs->mask;
    srcRealWidth = srcWidth = (pCurs->width + 31 ) / 32;
    endbits = endtab[pCurs->width & 0x1f];
    if ( endbits == 0 )
	endbits = 0xffffffff;
    if( srcWidth > 2 )
	{
	srcWidth = 2;
	endbits = ~0;
	}
    if ( ( srcHeight = pCurs->height ) > 64 )
	srcHeight = 64;
    for( i = 0; i < srcHeight; i++ )
    {
	if( srcWidth = 1 )
	{
	    *pImage = (*psrcImage)&endbits;
	    *pMask = (*psrcMask)&endbits;
	}
	else
	{
	    *pImage = *psrcImage;
	    *pMask = *psrcMask;
	    *(pImage+1) = (*(psrcImage+1))&endbits;
	    *(pMask+1) = (*(psrcMask+1))&endbits;
	}
	pImage = pImage+2;
	pMask = pMask+2;
	psrcImage = psrcImage + srcRealWidth;
	psrcMask = psrcMask + srcRealWidth;
    }
    TRACE(("exiting aedRealizeCursor\n"));
}


/***============================================================***/

	/*
	 * Free up the space reserved for 'pCurs'
	 */

Bool
aedUnrealizeCursor( pScr, pCurs)
    ScreenPtr 	pScr;
    CursorPtr 	pCurs;
{

    TRACE(("aedUnrealizeCursor( pScr= 0x%x, pCurs= 0x%x )\n",pScr,pCurs));

    Xfree( pCurs->devPriv[ pScr->myNum ]);
    return TRUE;
}

/***============================================================***/

	/*
	 *  Display (and track) the cursor described by "pCurs"
	 *  Copies the cursor image into the hardware active cursor
	 *  area.
	 *
	 *  If the cursor image has not already been copied into the
	 *  adapted off-screen memory (cursor is not "realized"), try
	 *  to realize it.  If the area reserved for cursor images is
	 *  full, print an error message and bail out.
	 *
	 *  After copying the cursor image, adjust rtCursorHotX and 
	 *  rtCursorHotY so that the cursor is displayed with it's
	 *  hot spot at the coordinates of mouse motion events.
	 *  Bearing in mind that we are tracking the cursor hot spot
	 *  (NOT the edges of the cursor), we have to adjust the
	 *  acceptable bounds (rtCursorBounds) of the cursor so we bump 
	 *  the cursor image around at the right times).
	 */

int
aedDisplayCursor( pScr, pCurs )
    ScreenPtr 	pScr;
    CursorPtr 	pCurs;
{
    int *aedCurs;

    TRACE(("aedDisplayCursor( pScr= 0x%x, pCurs= 0x%x )\n",pScr,pCurs));

    aedCurs = (int *)pCurs->devPriv[ pScr->myNum];

    rtCursorHotX= 0;
    rtCursorHotY= 0;

    rtCursorBounds.x1=	AED_SCREEN_WIDTH;
    rtCursorBounds.y1=	AED_SCREEN_HEIGHT;
    rtCursorBounds.x2=	AED_SCREEN_WIDTH;
    rtCursorBounds.y2=	AED_SCREEN_HEIGHT;

    vforce();
    vikint[1] = 7;	/* define cursor */
    vikint[2] = (short) pCurs->xhot;
    vikint[3] = (short) pCurs->yhot;
    bcopy((char *)aedCurs,(char *)&vikint[4], 1024);
    command(515);
    clear(2);
    return TRUE;
}

aedShowCursor(x,y)
    short *x, *y;
{
    TRACE(("aedShowCursor( x= 0x%x, y= 0x%x)( *x = %d, *y = %d )\n", x, y, (int)*x,(int)*y));
    vforce();
    vikint[1] = 10;	/* position cursor */
    vikint[2] = *x;
    vikint[3] = *y;
    command(3);
    clear(2);
}


