/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $Header: mfbpolypnt.c,v 1.13 87/09/11 07:48:39 toddb Exp $ */

#include "X.h"
#include "Xprotostr.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "miscstruct.h"
#include "regionstr.h"
#include "scrnintstr.h"

#include "mfb.h"
#include "maskbits.h"

/* macro for checking a point in a box 
   note that this also tosses out negative coordinates
*/
#define PointInBox(pbox, x, y) \
    (((x) >= (pbox)->x1) && ((x) < (pbox)->x2) && \
     ((y) >= (pbox)->y1) && ((y) < (pbox)->y2)) \
    ? 1 : 0

void
mfbPolyPoint(pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		mode;		/* Origin or Previous */
    int		npt;
    xPoint 	*pptInit;
{

    BoxPtr pboxInit;
    int nboxInit;
    register BoxPtr pbox;
    register int nbox;
    int xorg;
    int yorg;

    int *addrlBase;
    register int *addrl;
    int nlwidth;

    int nptTmp;
    register xPoint *ppt;

    register int x;
    register int y;
    register int rop;

    if (!(pGC->planemask & 1))
	return;

    pboxInit = ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip->rects;
    nboxInit = ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip->numRects;
    rop = ((mfbPrivGC *)(pGC->devPriv))->rop;

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	xorg = ((WindowPtr)pDrawable)->absCorner.x;
	yorg = ((WindowPtr)pDrawable)->absCorner.y;
	addrlBase = (int *)
		   (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate);
	nlwidth = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	xorg = 0;
	yorg = 0;
	addrlBase = (int *)(((PixmapPtr)pDrawable)->devPrivate);
	nlwidth = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    /* translate the point list
       do this here rather than in the loop because there are
       two cases to deal with
    */
    ppt = pptInit;
    nptTmp = npt;
    if (mode == CoordModeOrigin)
    {
	while(nptTmp--)
	{
	    ppt->x += xorg;
	    ppt++->y += yorg;
	}
    }
    else
    {
	ppt->x += xorg;
	ppt->y += yorg;
	nptTmp--;
	while(nptTmp--)
	{
	    ppt++;
	    ppt->x += (ppt-1)->x;
	    ppt->y += (ppt-1)->y;
	}
    }

    ppt = pptInit;
/* NOTE
   the if(rop) could be moved outside of the loop, at
   the cost of having three times as much code.
*/
    while (npt--)
    {
 	nbox = nboxInit;
	pbox = pboxInit;
	x = ppt->x;
	y = ppt++->y;

	while(nbox--)
	{
	    if (PointInBox(pbox, x, y))
	    {
	        addrl = addrlBase +
		        (y * nlwidth) +
		        (x >> 5);

	        if (rop == RROP_BLACK)
		    *addrl &= rmask[x & 0x1f];
	        else if (rop == RROP_WHITE)
		    *addrl |= mask[x & 0x1f];
	        else if (rop == RROP_INVERT)
		    *addrl ^= mask[x & 0x1f];

	        break;
	    }
	    else
	        pbox++;
	}
    }
}

