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
/* $Header: miwidedash.c,v 1.13 87/08/31 17:01:06 toddb Exp $ */
/* Author: Todd "Mr. Wide Line" Newman */

#include "X.h"
#include "Xprotostr.h"
#include "miscstruct.h"
#include "mistruct.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "mifpoly.h"

#define GCVALSALU	0
#define GCVALSFORE	1
#define GCVALSBACK	2
#define GCVALSWIDTH	3
#define GCVALSCAPSTYLE	4
#define GCVALSARCMODE	5
static int gcvals[] = {GXcopy, 1, 0, 0, 0, ArcChord};

/* Neither Fish nor Fowl, it's a Wide Dashed Line. */
/* Actually, wide, dashed lines Are pretty foul. (You knew that was coming,
 * didn't you.) */

/* MIWIDEDASH -- Public entry for PolyLine Requests when the GC speicifies
 * that we must be dashing (All Hail Errol Flynn)
 *
 * We must use the raster op to decide how whether we will draw directly into
 * the Drawable or squeegee bits through a scratch pixmap to avoid the dash
 * interfering with itself.
 * 
 * miDashLine will convert the poly line we were called with into the
 * appropriate set of line segments. 
 * Based on the dash style we then draw the segments. For OnOff dashes we
 * draw every other segment and cap each segment.  For DoubleDashes, we
 * draw every other segment starting with the first in the foreground color,
 * then draw every other segment starting with the second in the background
 * color.  Then we cap the first and last segments "by hand."
 */
void
miWideDash(pDraw, pGC, mode, npt, pPtsIn)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		mode;
    int		npt;
    DDXPointPtr pPtsIn;
{

    SppPointPtr	pPts, ppt;
    int		nseg, which, whichPrev, i, j, xOrg, yOrg, width,
    		fTricky, arcmode, xMin, xMax, yMin, yMax,
		dxi, dyi, gcflags;
    unsigned long oldfore, newfore;
    miDashPtr	dashes;
    double	dy, dx, m;
    Bool	IsDoubleDash = (pGC->lineStyle == LineDoubleDash),
		fXmajor;
    SppPointRec pt, PointStash[4], PolyPoints[4];
    DDXPointPtr pPtIn;
    DrawablePtr	pDrawTo;
    GCPtr	pGCTo;


    m = EPSILON;
    if (mode == CoordModePrevious)
    {
	DDXPointPtr pptT;
	int nptTmp;

	pptT = pPtsIn + 1;
	nptTmp = npt - 1;
	while (nptTmp--)
	{
	    pptT->x += (pptT-1)->x;
	    pptT->y += (pptT-1)->y;
	    pptT++;
	}
    }

    width = pGC->lineWidth;
    switch(pGC->alu)
    {
      case GXclear:		/* 0 */
      case GXcopy:		/* src */
      case GXcopyInverted:	/* NOT src */
      case GXset:		/* 1 */
	fTricky = FALSE;
        xOrg = yOrg = 0;
	pDrawTo = pDraw;
	pGCTo = pGC;
	if(pGCTo->arcMode != ArcChord)
	{
	    arcmode = pGCTo->arcMode;
	    DoChangeGC(pGCTo, GCArcMode, &gcvals[GCVALSARCMODE], 0);
	    ValidateGC(pDrawTo, pGCTo);
	}
	else
	    arcmode = ArcChord;

	break;
      case GXand:		/* src AND dst */
      case GXandReverse:	/* src AND NOT dst */
      case GXandInverted:	/* NOT src AND dst */
      case GXnoop:		/* dst */
      case GXxor:		/* src XOR dst */
      case GXor	:		/* src OR dst */
      case GXnor:		/* NOT src AND NOT dst */
      case GXequiv:		/* NOT src XOR dst */
      case GXinvert:		/* NOT dst */
      case GXorReverse:		/* src OR NOT dst */
      case GXorInverted:	/* NOT src OR dst */
      case GXnand:		/* NOT src OR NOT dst */
	fTricky = TRUE;
	yMin = yMax = pPtsIn[0].y;
	xMin = xMax = pPtsIn[0].x;

	for (i = 1; i < npt; i++)
	{
	    xMin = min(xMin, pPtsIn[i].x);
	    xMax = max(xMax, pPtsIn[i].x);
	    yMin = min(yMin, pPtsIn[i].y);
	    yMax = max(yMax, pPtsIn[i].y);
	}
	xOrg = xMin - (width + 1)/2;
	yOrg = yMin - (width + 1)/2;
	dxi = xMax - xMin + width;
	dyi = yMax - yMin + width;
	pDrawTo = (DrawablePtr) (*pDraw->pScreen->CreatePixmap)
	  (pDraw->pScreen, dxi, dyi, 1, XYBitmap);
	pGCTo =  GetScratchGC(1, pDraw->pScreen);
	gcvals[GCVALSWIDTH] = width;
	gcvals[GCVALSCAPSTYLE] = pGC->capStyle;
	gcflags = GCFunction | GCForeground | GCBackground | GCLineWidth |
		  GCCapStyle;
	if(pGCTo->arcMode != ArcChord)
	    gcflags |= GCArcMode;
	DoChangeGC(pGCTo, gcflags, gcvals, 0);
	ValidateGC(pDrawTo, pGCTo);
	miClearDrawable(pDrawTo, pGCTo);

    }

    dashes = miDashLine(npt, pPtsIn,
               pGC->numInDashList, pGC->dash, pGC->dashOffset, &nseg);
    if(!(pPts = (SppPointPtr) Xalloc((nseg + 1) * sizeof(SppPointRec))))
    {
	Xfree(dashes);
	if(fTricky)
	    (*pDraw->pScreen->DestroyPixmap)(pDrawTo);
	return;
    }
    ppt = pPts;
    pPtIn = pPtsIn;
    whichPrev = EVEN_DASH;

    j = 0;
    for(i = 0; i < nseg + 1; i++)
    {
	if(dashes[i].newLine)
	{
	    /* calculate slope of the line */
	    dx = (double) ((pPtsIn + 1)->x - pPtIn->x);
	    dy = (double) ((pPtsIn + 1)->y - pPtIn->y);
	    pPtIn++;
	    /* use slope of line to figure out how to use error term */
	    fXmajor = (fabs(dx) > fabs(dy));
	    if(fXmajor)
		m = !ISZERO(dx) ? (dy/dx) : EPSILON;
	    else
		m = !ISZERO(dy) ? (dx/dy) : EPSILON;
	}
	/* Add this point to our list, adjusting the error term as needed */
	ppt->x = (double) dashes[i].pt.x;
	ppt->y = (double) dashes[i].pt.y;

	if(i < 2 || i > nseg - 2)
	{
	    PointStash[j++] = *ppt;
	}
	ppt++;
	which = dashes[i].which;
	if(which != whichPrev)
	{
	    if(which == ODD_DASH)
	    {
		/* Display the collect line */
		(*pGC->LineHelper)(pDrawTo, pGCTo, !IsDoubleDash,
				   ppt - pPts, pPts, xOrg, yOrg);
	    }
	    /* Reset the line  and start a new dash */
	    pPts[0] = ppt[-1];
	    ppt = &pPts[1];
	    whichPrev = which;
	}

    }
    if(IsDoubleDash)
    {
        ppt = pPts;
        pPtIn = pPtsIn;
	whichPrev = EVEN_DASH;

	/* cap the first (and maybe the last) line segment(s)  appropriately */
	if(pGC->capStyle == CapProjecting)
	{
	    pt = miExtendSegment(PointStash[0], PointStash[1], width/2);
	    miGetPts(pt, PointStash[0],
	        &PolyPoints[0], &PolyPoints[1], &PolyPoints[2], &PolyPoints[3],
	        width);
	    miFillSppPoly(pDrawTo, pGCTo, 4, PolyPoints, xOrg, yOrg);
	    if(dashes[nseg].which == EVEN_DASH)
	    {
	        pt = miExtendSegment(PointStash[3], PointStash[2], width/2);
	        miGetPts(pt, PointStash[3],
	            &PolyPoints[0], &PolyPoints[1], &PolyPoints[2],
		    &PolyPoints[3], width);
	        miFillSppPoly(pDrawTo, pGCTo, 4, PolyPoints, xOrg, yOrg);
	    }
	 
	}
	else if (pGC->capStyle == CapRound)
	{
	    miGetPts(PointStash[0], PointStash[1],
	        &PolyPoints[0], &PolyPoints[1], &PolyPoints[2], &PolyPoints[3],
	        width);
	    miRoundCap(pDrawTo, pGCTo, PointStash[0], PointStash[1],
	             PolyPoints[0], PolyPoints[3], FirstEnd, xOrg, yOrg);
	    if(dashes[nseg].which == EVEN_DASH)
	    {
	        miGetPts(PointStash[3], PointStash[2],
	            &PolyPoints[0], &PolyPoints[1], &PolyPoints[2],
		    &PolyPoints[3], width);
	        miRoundCap(pDrawTo, pGCTo, PointStash[3], PointStash[2],
	                 PolyPoints[3], PolyPoints[0], SecondEnd, xOrg, yOrg);
	    }
	}
	oldfore = pGC->fgPixel;
	newfore = pGC->bgPixel;
	DoChangeGC(pGCTo, GCForeground, &newfore, 0);
	ValidateGC(pDrawTo, pGCTo);

	for(i = 0; i < nseg + 1; i++)
	{
	    if(dashes[i].newLine)
	    {
		/* calculate slope of the line */
		dx = (double) ((pPtIn + 1)->x - pPtIn->x);
		dy = (double) ((pPtIn + 1)->y - pPtIn->y);
		/* use slope of line to figure out how to use error term */
		fXmajor = (fabs(dx) > fabs(dy));
		if(fXmajor)
		    m = ISZERO(dx) ? (dy/dx) : EPSILON;
		else
		    m = ISZERO(dy) ? (dx/dy) : EPSILON;
		pPtIn++;
	    }
	    /* Add this point to our list */
	    ppt->x = (double) dashes[i].pt.x +
	    		(fXmajor ? 0.0 : dashes[i].e*m);
	    ppt->y = (double) dashes[i].pt.y +
	    		(fXmajor ? dashes[i].e*m : 0.0);
	    ppt++;
	    which = dashes[i].which;
	    if(which != whichPrev)
	    {
		if(which == EVEN_DASH)
		{
		    /* Display the collected line */
		    (*pGC->LineHelper)(pDrawTo, pGCTo, FALSE,
				       ppt - pPts, pPts, xOrg, yOrg);
		}
		/* Reset the line  and start a new dash */
		pPts[0] = ppt[-1];
		ppt = &pPts[1];
		whichPrev = which;
	    }

	}

	/* cap the last line segments appropriately */
	if(dashes[nseg].which == ODD_DASH)
	{
	    if(pGC->capStyle == CapProjecting)
	    {
		pt = miExtendSegment(PointStash[3], PointStash[2], width/2);
		miGetPts(pt, PointStash[3],
		    &PolyPoints[0], &PolyPoints[1], &PolyPoints[2],
		    &PolyPoints[3], width);
		miFillSppPoly(pDrawTo, pGCTo, 4, PolyPoints, xOrg, yOrg);
	     
	    }
	    else if (pGC->capStyle == CapRound)
	    {
		miGetPts(PointStash[3], PointStash[2],
		    &PolyPoints[0], &PolyPoints[1], &PolyPoints[2],
		    &PolyPoints[3], width);
		miRoundCap(pDrawTo, pGCTo, PointStash[3], PointStash[2],
			 PolyPoints[3], PolyPoints[0], SecondEnd, xOrg, yOrg);
	    }
	}
	DoChangeGC(pGCTo, GCForeground, &oldfore, 0);
    }
    if(arcmode != ArcChord)
    {
	DoChangeGC(pGCTo, GCArcMode, &gcvals[GCVALSARCMODE], 0);
    }
    ValidateGC(pDrawTo, pGCTo);
    if(fTricky)
    {
	if (pGC->miTranslate && (pDraw->type == DRAWABLE_WINDOW) )
	{
	    xOrg += ((WindowPtr)pDraw)->absCorner.x;
	    yOrg += ((WindowPtr)pDraw)->absCorner.y;
	}

	(*pGC->PushPixels)(pGC, pDrawTo, pDraw, dxi, dyi, xOrg, yOrg);
	(*pDraw->pScreen->DestroyPixmap)(pDrawTo);
        FreeScratchGC(pGCTo);
    }
    Xfree(dashes);
    Xfree(pPts);
}
