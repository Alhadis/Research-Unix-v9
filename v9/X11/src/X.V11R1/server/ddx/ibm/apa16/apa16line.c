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

/* $Header: apa16line.c,v 1.3 87/09/13 03:19:06 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/apa16/RCS/apa16line.c,v $ */

#ifndef lint
static char *rcsid = "$Header: apa16line.c,v 1.3 87/09/13 03:19:06 erik Exp $";
#endif

#include "X.h"
#include "Xmd.h"

#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "scrnintstr.h"

#include "rtutils.h"

#include "mfb.h"
#include "maskbits.h"

#include "apa16hdwr.h"

/* single-pixel lines on a monochrome frame buffer
   written by drewry, with debts to kelleher, mullis, carver, sanborn

   we loop through the clip rectangles on the outside and the line in
the inner loop because we expect there to be few clip rectangles.
*/

/* cohen-sutherland clipping codes
   right and bottom edges of clip rectangles are not
included
*/ 

#define OUTCODES(result, x, y, pbox) \
    if (x < pbox->x1) \
	result |= OUT_LEFT; \
    if (y < pbox->y1) \
	result |= OUT_ABOVE; \
    if (pbox->x2 <= x) \
	result |= OUT_RIGHT; \
    if (pbox->y2 <= y) \
	result |= OUT_BELOW; \

/*
    clipping lines presents a few difficulties, most of them
caused by numerical error or boundary conditions.
    when clipping the endpoints, we want to use the floor
operator after division rather than truncating towards 0; truncating
towards 0 gives different values for the clipped endpoint, based
on the direction the line is drawn, because of the signs of
the operands.
    ??? Brian will explain why all the swapping and so on ???
    since we don't want to draw the points on the lower and right
edges of the clip regions, we adjust them in by one if the point
being clipped is the first point in the line.  (if it's the last
point, it won't get drawn anyway.)
    another way would be to sort the points, and always do
everything in the same direction, but then we'd have to keep
track of which was the original first point of the segment, for
the specil-case last endpoint calculations.
    another other way to solve this is to run the bresenham until
the line enters the clipping rectangle.  this is simple, but means
you might run the full line once for each clip rectangle.
    (See ACM Transactions on Graphics, April 1983, article by 
Rob Pike for perhaps a better solution (although there are several
details to fill in.

    the macro mod() is defined because pcc generates atrocious code
for the C % operator; the macro can be turned off and the inline
utility can generate something better.
*/

#define mod(a, b) ((a)%(b))

/* there are NO parentheses here; the user is thus encoureaged to
think very carefully about how this is called.
*/
#define floordiv(dividend, divisor, quotient) \
quotient = dividend/divisor; \
if ( (quotient < 0) && (mod(dividend, divisor)) ) \
    quotient--;


/* single-pixel solid lines */
void apa16LineSS(pDrawable, pGC, mode, npt, pptInit)
DrawablePtr pDrawable;
GC *pGC;
int mode;		/* Origin or Previous */
int npt;		/* number of points */
DDXPointPtr pptInit;
{
    int nrect;
    register BoxPtr pbox;
    int nline;			/* npt - 1 */
    int nptTmp;
    register DDXPointPtr ppt;	/* pointer to list of translated points */

    DDXPointRec pt1;
    DDXPointRec pt2;
    DDXPointRec pt1Orig;		/* spare copy of points */

    register int spos;		/* inness/outness of starting point */
    register int fpos;		/* inness/outness of final point */

    int *addrl;			/* address of longword with first point */
    int nlwidth;		/* width in longwords of destination bitmap */
    int xorg, yorg;		/* origin of window */

    int dx, dy;

    int clipDone;		/* flag for clipping loop */
    int swapped;		/* swapping of endpoints when clipping */

    register int cliptmp;	/* for endpoint clipping */
    register int clipquot;	/* quotient from floordiv() */
    int	start_apa16= FALSE;
    int	lastx= -1,lasty= -1;
    unsigned cmd;

    TRACE(("apa16LineSS(pDrawable= 0x%x, pGC= 0x%x, mode= 0x%x, npt= %d, pptInit= 0x%x)\n",pDrawable,pGC,mode,npt,pptInit));

    if (pDrawable->type==DRAWABLE_PIXMAP) {
	mfbLineSS(pDrawable,pGC,mode,npt,pptInit);
	return;
    }

    if ((((mfbPrivGC *)pGC->devPriv)->rop)==RROP_NOP) 
	return;

    QUEUE_RESET();
    APA16_GET_CMD(ROP_NULL_VECTOR,(((mfbPrivGC *)pGC->devPriv)->rop),cmd);
    pbox = ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip->rects;
    nrect = ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip->numRects;

    xorg = ((WindowPtr)pDrawable)->absCorner.x;
    yorg = ((WindowPtr)pDrawable)->absCorner.y;
    addrl = (int *)(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate);
    nlwidth= (int)(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;

    /* translate the point list */
    ppt = pptInit;
    nptTmp = npt;
    if (mode == CoordModeOrigin)
    {
	while(nptTmp--)
	{
	    ppt->x += xorg;
	    ppt->y += yorg;
	    ppt++;
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

    while(nrect--)
    {
	nline = npt;
	ppt = pptInit;

	while(--nline)
	{
	    pt1 = *ppt++;
	    pt2 = *ppt;
	    pt1Orig = pt1;

	    dx = pt2.x - pt1.x;
	    dy = pt2.y - pt1.y;

	    /* calculate clipping */
	    swapped = 0;
	    clipDone = 0;
	    do
	    {
		DDXPointRec ptTemp;
		int temp;

	        spos = 0;
	        fpos = 0;

	        OUTCODES(spos, pt1.x, pt1.y, pbox)
	        OUTCODES(fpos, pt2.x, pt2.y, pbox)
        
	        if (spos & fpos)	/* line entirely out */
		    clipDone = -1;
	        else if ((spos | fpos) == 0) /* line entirely in */
	        {
		    if (swapped)
		    {
		        ptTemp = pt1;
		        pt1 = pt2;
		        pt2 = ptTemp;
		    }
		    clipDone = 1;
	        }
	        else	/* line is clipped */
	        {
		    /* clip start point */
		    if (spos == 0)
		    {
		        ptTemp = pt1;
		        pt1 = pt2;
		        pt2 = ptTemp;

		        temp = spos;
		        spos = fpos;
		        fpos = temp;
			swapped = !swapped;
		    }

		    if (spos & OUT_BELOW)
		    {
			cliptmp = ((pbox->y2-1) - pt1Orig.y) * dx;
			floordiv(cliptmp, dy, clipquot);
		        pt1.x = pt1Orig.x + clipquot;
		        pt1.y = pbox->y2-1;
		    }
		    else if (spos & OUT_ABOVE)
		    {
			cliptmp = (pbox->y1 - pt1Orig.y) * dx;
			floordiv(cliptmp, dy, clipquot);
		        pt1.x = pt1Orig.x + clipquot;
		        pt1.y = pbox->y1;
		    }
		    else if (spos & OUT_RIGHT)
		    {
			cliptmp = ((pbox->x2-1) - pt1Orig.x) * dy;
			floordiv(cliptmp, dx, clipquot);
		        pt1.y = pt1Orig.y + clipquot;
		        pt1.x = pbox->x2-1;
		    }
		    else if (spos & OUT_LEFT)
		    {
			cliptmp = (pbox->x1 - pt1Orig.x) * dy;
			floordiv(cliptmp, dx, clipquot);
		        pt1.y = pt1Orig.y + clipquot;
		        pt1.x = pbox->x1;
		    }
	        }
	    } while (!clipDone);

	    if (clipDone == -1)
		continue;

	    if ((pt1.x==pt2.x)&&(pt1.y==pt2.y)) /* length 0 */
		continue;
	    if ((pt1.x==lastx)&&(pt1.y==lasty)) {
		POLY_VECTOR(cmd,pt2.x,pt2.y);
	    }
	    else {
		DRAW_VECTOR(cmd,pt1.x,pt1.y,pt2.x,pt2.y);
	    }
	    lastx= pt2.x;
	    lasty= pt2.y;
	    start_apa16= TRUE;
	} /* while (nline--) */
	pbox++;
    } /* while (nbox--) */

    /* paint the last point if it's not the same as the first
       and hasn't been clipped
       after the main loop is done, ppt points to the last point in
       the list
    */

    if ((ppt->x != pptInit->x) ||
	(ppt->y != pptInit->y))
    {
        pbox = ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip->rects;
        nrect = ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip->numRects;
	pt1 = *ppt;

	while (nrect--)
	{
	    if ((pt1.x >= pbox->x1) &&
		(pt1.y >= pbox->y1) &&
		(pt1.x <  pbox->x2) &&
		(pt1.y <  pbox->y2))
	    {
		addrl += (pt1.y * nlwidth) + (pt1.x >> 5);
		switch( ((mfbPrivGC *)(pGC->devPriv))->rop)
		{
		    case RROP_BLACK:
		        *addrl &= rmask[pt1.x & 0x1f];
			break;
		    case RROP_WHITE:
		        *addrl |= mask[pt1.x & 0x1f];
			break;
		    case RROP_INVERT:
		        *addrl ^= mask[pt1.x & 0x1f];
			break;
		}
		break;
	    }
	    else
		pbox++;
	}
    }
    if (start_apa16) APA16_GO();
}

