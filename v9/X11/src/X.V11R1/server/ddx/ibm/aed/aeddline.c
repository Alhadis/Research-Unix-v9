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
/* $Header: aeddline.c,v 1.1 87/09/13 03:34:10 erik Exp $ */
#include "X.h"

#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "mistruct.h"
#include "Xprotostr.h"

#include "mfb.h"
#include "maskbits.h"


/*
    this code does not pretend to be efficient, but it does recycle a
lot of the line code and use the miDashLine() code too.  a better
implementation is to use the solid line code to clip and
translate, and then call mfbBresD(), to do the dashes as the
line is drawn.  a Bres() procedure entry in the devPrivate
part of the GC would make this easy to do, as well as possibly speeding
up solid lines to (by avoiding the test of rrop for each segment.)

    to do double dashes we concoct a rop for the (alu, bg) pair.

    the error term at the start of each dash is computed for us by
miDashLine.  if the segment we draw is not clipped, we can use this
error term; if the first point of the dash is clipped, we have to
calculate a new error term based on e at the first point of the line.
*/

void
aedDashLine( pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr pDrawable;
    GCPtr pGC;
    int mode;		/* Origin or Previous */
    int npt;		/* number of points */
    DDXPointPtr pptInit;
{
    int nseg;			/* number of dashed segments */
    int tnseg;			/* number of dashed segments */
    miDashPtr pdash;		/* list of dashes */
    miDashPtr pdashInit;

    int nptTmp;
    DDXPointPtr ppt;		/* pointer to list of translated points */


    int xorg, yorg;		/* origin of window */



    xSegment *pSegs, *initSegs;
    int nSegs;
    int oldfg;
    int oldstyle, newstyle;


    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	xorg = ((WindowPtr)pDrawable)->absCorner.x;
	yorg = ((WindowPtr)pDrawable)->absCorner.y;
    }
    else
    {
	xorg = 0;
	yorg = 0;
    }
    oldstyle = pGC->lineStyle;
    newstyle = LineSolid;
    ChangeGC(pGC, GCLineStyle, &newstyle);
    ValidateGC(pDrawable, pGC); 

    /* translate the point list */
    ppt = pptInit;
    nptTmp = npt;
    if (mode == CoordModeOrigin)
    {
/*
	while(nptTmp--)
	{
	    ppt->x += xorg;
	    ppt->y += yorg;
	    ppt++;
	}
*/
    }
    else
    {
/*
	ppt->x += xorg;
	ppt->y += yorg;
*/
	nptTmp--;
	while(nptTmp--)
	{
	    ppt++;
	    ppt->x += (ppt-1)->x;
	    ppt->y += (ppt-1)->y;
	}
    }


    pdash = miDashLine(npt, pptInit, 
		       pGC->numInDashList, pGC->dash, pGC->dashOffset,
		       &nseg);

    initSegs = (xSegment *)Xalloc( sizeof(xSegment)*nseg );
    pSegs = initSegs;
    nSegs = 0;
    pdashInit = pdash;


    tnseg = nseg;
    while(tnseg--)
    {
	while ((tnseg) && (pdash->which == ODD_DASH))
	{
	    tnseg--;
	    pdash++;
	}
	/* ??? is this right ??? */
	if (!tnseg)
	    break;
	if ((pdash+1)->newLine == 0)
	{
	    pSegs->x1 = pdash->pt.x;
	    pSegs->y1 = pdash->pt.y;
	    pSegs->x2 = (pdash+1)->pt.x;
	    pSegs->y2 = (pdash+1)->pt.y;
	    if ( abs(pSegs->x2 - pSegs->x1) > abs(pSegs->y2 - pSegs->y1))
		if (pSegs->x2 > pSegs->x1)
		    pSegs->x2--;
		else
		    pSegs->x1--;
	    else
		if (pSegs->y2 > pSegs->y1)
		    pSegs->y2--;
		else
		    pSegs->y1--;
	    pSegs++;
	    nSegs++;
	}
	pdash++;
    } /* while --tnseg */
    miPolySegment(pDrawable, pGC, nSegs, initSegs);

    if( pGC->lineStyle == LineDoubleDash )
    {
	nSegs = 0;
	pSegs = initSegs;
	tnseg = nseg;
	pdash = pdashInit;

	oldfg = pGC->fgPixel;
	ChangeGC(pGC, GCForeground, &pGC->bgPixel);
	ValidateGC(pDrawable, pGC); 

	while(tnseg--)
	{
	    while ((tnseg) && (pdash->which == EVEN_DASH))
	    {
		tnseg--;
		pdash++;
	    }
	    /* ??? is this right ??? */
	    if (!tnseg)
		break;
	    if ((pdash+1)->newLine == 0)
	    {
		pSegs->x1 = pdash->pt.x;
		pSegs->y1 = pdash->pt.y;
		pSegs->x2 = (pdash+1)->pt.x;
		pSegs->y2 = (pdash+1)->pt.y;
		if ( abs(pSegs->x2 - pSegs->x1) > abs(pSegs->y2 - pSegs->y1))
		    if (pSegs->x2 > pSegs->x1)
			pSegs->x2--;
		    else
			pSegs->x1--;
		else
		    if (pSegs->y2 > pSegs->y1)
			pSegs->y2--;
		    else
			pSegs->y1--;
		pSegs++;
		nSegs++;
	    }
	    pdash++;
	} /* while --tnseg */
	miPolySegment(pDrawable, pGC, nSegs, initSegs);

	ChangeGC(pGC, GCForeground, &oldfg);
    }
    ChangeGC(pGC, GCLineStyle, &oldstyle);
    ValidateGC(pDrawable, pGC); 

    Xfree(pdashInit);
    Xfree(initSegs);
}
