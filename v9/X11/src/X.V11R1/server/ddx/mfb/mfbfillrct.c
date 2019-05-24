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
/* $Header: mfbfillrct.c,v 1.32 87/09/08 15:32:46 drewry Locked $ */
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

#define MODEQ(a, b) ((a) %= (b))
void mfbPaintOddSize();

/* 
    filled rectangles.
    translate the rectangles, clip them, and call the
helper function in the GC.
*/

void
mfbPolyFillRect(pDrawable, pGC, nrectFill, prectInit)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nrectFill; 	/* number of rectangles to fill */
    xRectangle	*prectInit;  	/* Pointer to first rectangle to fill */
{
    int xorg, yorg;
    register int n;		/* spare counter */
    xRectangle *prect;		/* temporary */
    RegionPtr prgnClip;
    register BoxPtr pbox;	/* used to clip with */
    register BoxPtr pboxClipped;
    BoxPtr pboxClippedBase;

    int alu;
    void (* pfn) ();
    PixmapPtr ppix;

    alu = ((mfbPrivGC *)(pGC->devPriv))->ropFillArea;
    pfn = ((mfbPrivGC *)(pGC->devPriv))->FillArea;
    ppix = *( ((mfbPrivGC *)(pGC->devPriv))->ppPixmap);
    prgnClip = ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip;

    pboxClippedBase = (BoxPtr)ALLOCATE_LOCAL(prgnClip->numRects * 
					     sizeof(BoxRec));

    if (!pboxClippedBase)
	return;

    prect = prectInit;
    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	xorg = ((WindowPtr)pDrawable)->absCorner.x;
	yorg = ((WindowPtr)pDrawable)->absCorner.y;
        prect = prectInit;
	n = nrectFill;
        while(n--)
        {
	    prect->x += xorg;
	    prect->y += yorg;
	    prect++;
        }
    }

    prect = prectInit;
    while (nrectFill--)
    {
	BoxRec box;

	if (prect->width <= 0 || prect->height <= 0)
	{
	    prect++;
	    continue;
	}
	box.x1 = prect->x;
	box.y1 = prect->y;
	box.x2 = box.x1 + prect->width;
	box.y2 = box.y1 + prect->height;
	prect++;

	switch((*pGC->pScreen->RectIn)(prgnClip, &box))
	{
	  case rgnOUT:
	    break;
	  case rgnIN:
	    (*pfn)(pDrawable, 1, &box, alu, ppix);
	    break;
	  case rgnPART:
	    pboxClipped = pboxClippedBase;
	    pbox = prgnClip->rects;
	    n = prgnClip->numRects;

	    /* clip the rectangle to each box in the clip region
	       this is logically equivalent to calling Intersect()
	    */
	    while(n--)
	    {
	        pboxClipped->x1 = max(box.x1, pbox->x1);
	        pboxClipped->y1 = max(box.y1, pbox->y1);
	        pboxClipped->x2 = min(box.x2, pbox->x2);
	        pboxClipped->y2 = min(box.y2, pbox->y2);
		pbox++;

	        /* see if clipping left anything */
	        if(pboxClipped->x1 < pboxClipped->x2 && 
	           pboxClipped->y1 < pboxClipped->y2)
	        {
		    pboxClipped++;
	        }
	    }
	    (*pfn)(pDrawable, pboxClipped-pboxClippedBase, 
		   pboxClippedBase, alu, ppix);
	    break;
	}
    }
    DEALLOCATE_LOCAL(pboxClippedBase);
}
