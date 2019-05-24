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
/* $Header: milines.c,v 1.45 87/09/11 07:20:09 toddb Exp $ */

/*
 *
 *  Written by Todd Newman 
 *  loosely based on long ago code and recent advice
 *  from Brian Kelleher, John Danskin
 *
 *  Draw fat lines using a convex polygon routine.
 */

#include <stdio.h>
extern double hypot();
#include "X.h"
#include "windowstr.h"
#include "Xprotostr.h"
#include "gcstruct.h"
#include "scrnintstr.h"
#include "miscstruct.h"
#include "pixmapstr.h"
#include "mifpoly.h"
#include "mi.h"

#define GCVALSALU	0
#define GCVALSFORE	1
#define GCVALSBACK	2
#define GCVALSWIDTH	3
#define GCVALSCAPSTYLE	4
#define GCVALSJOINSTYLE	5
#define GCVALSARCMODE	6
static int gcvals[] = {GXcopy, 1, 0, 0, 0, 0, ArcChord};

#define SAMESIGN(a, b)  ((((a) >= 0) && ((b) >= 0)) ||\
		     (((a) <= 0) && ((b) <= 0)))

SppPointRec IntersectLines();

/* MIWIDELINE - Public entry for PolyLine call
 * handles 1 segment wide lines specially.  Then it sets up the GC/Drawable
 * based on the raster op.  If the raster op is one that doesn't look at
 * the destination, then we can draw directly onto the drawable.  If the
 * raster op does look at the destination (e.g., Xor), then we will draw
 * with GXcopy onto a side bitmap and then squeegee the appropriate pattern
 * (foreground, or tile, or stipple) onto the real drawable.
 * We call one of two helpers (miMiter or miNonMiter) to actually draw
 * the line segments. Which helper we call is determined by the device
 * specific GC validator (e.g., mfbValidateGC).
 */
void
miWideLine(pDrawable, pGC, mode, npt, pPts)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		mode;
    int		npt;
    DDXPointPtr pPts;
{
    int		width = pGC->lineWidth;
    SppPointPtr pSppPts, pSppT;
    int		gcflags, i, xOrg, yOrg, fTricky, arcmode;
    int		xMin, xMax, yMin, yMax, dx, dy;
    DrawablePtr	pDrawTo;
    GCPtr	pGCTo;


    /* make everything absolute */
    if (mode == CoordModePrevious)
    {
	DDXPointPtr pptTmp;
	int nptTmp;

	pptTmp = pPts + 1;
	nptTmp = npt - 1;
	while (nptTmp--)
	{
	    pptTmp->x += (pptTmp-1)->x;
	    pptTmp->y += (pptTmp-1)->y;
	    pptTmp++;
	}
    }
    switch(pGC->alu)
    {
      case GXclear:		/* 0 */
      case GXcopy:		/* src */
      case GXcopyInverted:	/* NOT src */
      case GXset:		/* 1 */
	fTricky = FALSE;
        xOrg = yOrg = 0;
	pDrawTo = pDrawable;
	pGCTo = pGC;
	arcmode = pGCTo->arcMode;
	if(arcmode != ArcChord)
	{
	    DoChangeGC(pGCTo, GCArcMode, &gcvals[GCVALSARCMODE], 0);
	    ValidateGC(pDrawTo, pGCTo);
	}

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
	yMin = yMax = pPts[0].y;
	xMin = xMax = pPts[0].x;

	for (i = 1; i < npt; i++)
	{
	    xMin = min(xMin, pPts[i].x);
	    xMax = max(xMax, pPts[i].x);
	    yMin = min(yMin, pPts[i].y);
	    yMax = max(yMax, pPts[i].y);
	}
	xOrg = xMin - (width + 1)/2;
	yOrg = yMin - (width + 1)/2;
	dx = xMax - xMin + width + 1;
	dy = yMax - yMin + width + 1;
	pDrawTo = (DrawablePtr) (*pDrawable->pScreen->CreatePixmap)
	  (pDrawable->pScreen, dx, dy, 1, XYBitmap);
	pGCTo = GetScratchGC(1, pDrawable->pScreen);
	gcvals[GCVALSWIDTH] = width;
	gcvals[GCVALSCAPSTYLE] = pGC->capStyle;
	gcvals[GCVALSJOINSTYLE] = pGC->joinStyle;
	gcflags = GCFunction|GCForeground|GCBackground|GCLineWidth |
		  GCCapStyle | GCJoinStyle;
	if(pGCTo->arcMode != ArcChord)
	    gcflags |= GCArcMode;
	arcmode = ArcChord;
	DoChangeGC(pGCTo, gcflags, gcvals, 0);
	ValidateGC(pDrawTo, pGCTo);
	miClearDrawable(pDrawTo, pGCTo);

    }
    pSppPts = (SppPointPtr) Xalloc((npt + 1)  * sizeof(SppPointRec));
    pSppT = pSppPts;
    /* convert points to DblPts */
    for(i = 0; i < npt; i++)
    {
	pSppT->x = (double) pPts->x;
	pSppT++->y = (double) pPts++->y;
	
    }

    /* Draw the lines with the appropriate join style.
     * We always wants caps drawn. */
    (*pGCTo->LineHelper) (pDrawTo, pGCTo, TRUE, npt, pSppPts, xOrg, yOrg);

    if(arcmode != ArcChord)
    {
	DoChangeGC(pGCTo, GCArcMode, &arcmode, 0);
	ValidateGC(pDrawTo, pGCTo);
    }
    if(fTricky)
    {
	if (pGC->miTranslate && (pDrawable->type == DRAWABLE_WINDOW) )
	{
	    xOrg += ((WindowPtr)pDrawable)->absCorner.x;
	    yOrg += ((WindowPtr)pDrawable)->absCorner.y;
	}

	(*pGC->PushPixels)(pGC, pDrawTo, pDrawable, dx, dy, xOrg, yOrg);
	(*pDrawable->pScreen->DestroyPixmap)(pDrawTo);
	FreeScratchGC(pGCTo);
    }
}

/* MIONESEGWIDE -- private helper for wide line code
 * Draw wide line which only has 1 segment.  This is so easy that it's worth
 * special casing. Regardless of the raster op, this can't self-intersect.
 * Also handles fluke cases where we're called with only 1 point 
 */
static
void
miOneSegWide(pDrawable, pGC, npt, pPts, caps, xOrg, yOrg)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		npt;
    SppPointPtr pPts;
    int		caps, xOrg, yOrg;
{
    SppPointRec	PolyPoints[4];
    int		width = pGC->lineWidth;

    if(npt == 1)
    {
	Xrealloc(pPts, 2 * sizeof(SppPointRec));
        pPts[1] = pPts[0];
    }
    if(caps && pGC->capStyle == CapProjecting)
    {
	if(PtEqual(pPts[0], pPts[1]))
	{
	    pPts[0].y -= width/2.0;
	    pPts[1].y += width/2.0;
	}
	else
	{
	    pPts[0] = miExtendSegment(pPts[0], pPts[1], width/2);
	    pPts[1] = miExtendSegment(pPts[1], pPts[0], width/2);
	}
    }
    miGetPts(pPts[0], pPts[1], &PolyPoints[0], 
	   &PolyPoints[1], &PolyPoints[2], &PolyPoints[3], width);
    miFillSppPoly(pDrawable, pGC, 4, PolyPoints, -xOrg, -yOrg);
    if(caps && pGC->capStyle == CapRound)
    {
	miRoundCap(pDrawable, pGC, pPts[0], pPts[1], PolyPoints[0],
	  PolyPoints[3], FirstEnd, xOrg, yOrg);
	miRoundCap(pDrawable, pGC, pPts[1], pPts[0], PolyPoints[2],
	  PolyPoints[1], SecondEnd, xOrg, yOrg);
    }
}

/* MIMITER -- helper for wide line code, stuffed into GC.lineHelper by
 * validation routine
 * draw wide polylines by finding the polygon that outlines
 * each wide line segment, and drawing that. Uses GC's fill polygon routine.
 *
 * internal vertexes are joined with a mitre as in PostScript. 
 * vertices at the beginning and end of the polyline
 * are given the appropriate end unless they meet at the same point in which
 * case they are mitred together.  */

void
miMiter (pDraw, pGC, caps, npt, pPts, xOrg, yOrg)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		caps;
    int		npt;
    SppPointPtr pPts;
    int		xOrg, yOrg;
{
    int		width = pGC->lineWidth;
    SppPointRec	PolyPoints[4], FirstEdge[2];
    SppPointRec	p1, p2, p3, p4, p5, p6, p7, p8;
    int		edges_match, i,
		capStyle = pGC->capStyle;

    /*
     * special case length 1 polyline
     */
    if(npt <= 2)
    {
        miOneSegWide(pDraw, pGC, npt, pPts, caps, xOrg, yOrg);
	return;
    }
    miGetPts(pPts[0], pPts[1], &p1, &p2, &p3, &p4, width);

    /*
     * start on first edge of first point
     */
    if (PtEqual(pPts[0], pPts[npt-1]))
    {
	/*
	 * edges match so mitre them
	 */
	edges_match = 1;
	miGetPts(pPts[npt-2], pPts[npt-1], &p5, &p6, &p7, &p8, width);

	PolyPoints[3] = IntersectLines(p1, p2, p5, p6);
	PolyPoints[2] = IntersectLines(p3, p4, p7, p8);

	FirstEdge[0] = PolyPoints[3];
	FirstEdge[1] = PolyPoints[2];
    }
    else
    {
	edges_match = 0;
	PolyPoints[3] = p1;
	PolyPoints[2] = p4;
	if (caps && capStyle == CapProjecting)
	{
	    pPts[0] = miExtendSegment(pPts[0], pPts[1], width/2);
	    miGetPts(pPts[0], pPts[1], &p1, &p2, &p3, &p4, width);
	    pPts[npt-1] = miExtendSegment(pPts[npt-1], pPts[npt-2], width/2);
	}
	else if (caps && capStyle == CapRound)
	{
	    miRoundCap(pDraw, pGC, pPts[0], pPts[1], p1, p4, FirstEnd,
	             xOrg, yOrg);
	    miGetPts(pPts[npt-1], pPts[npt-2], &p5, &p6, &p7, &p8, width);
	    miRoundCap(pDraw, pGC, pPts[npt-1], pPts[npt-2], p5, p8, FirstEnd,
	             xOrg, yOrg);
	}
    }

    for (i = 1; i < (npt - 1); i++)
    {
	/*
	 * find the next edge
	 * build a polygon out of the next edge and the last one
	 * send the polygon
	 * bump to next
	 */
	miGetPts(pPts[i], pPts[i+1], &p5, &p6, &p7, &p8, width);

	PolyPoints[0] = PolyPoints[3];
	PolyPoints[1] = PolyPoints[2];
	PolyPoints[2] = IntersectLines(p4, p3, p7, p8);
	PolyPoints[3] = IntersectLines(p1, p2, p5, p6);

	miFillSppPoly(pDraw, pGC, 4, PolyPoints, -xOrg, -yOrg);

	p1 = p5;  p2 = p6;  p3 = p7;  p4 = p8;

    }

    PolyPoints[0] = PolyPoints[3];
    PolyPoints[1] = PolyPoints[2];

    if (edges_match)
    {
	PolyPoints[2] = FirstEdge[1];
	PolyPoints[3] = FirstEdge[0];
    }
    else
    {
	miGetPts(pPts[npt-2], pPts[npt-1], &p5, &p6, &p7, &p8, width);
	PolyPoints[2] = p7;
	PolyPoints[3] = p6;
    }
	
    miFillSppPoly(pDraw, pGC, 4, PolyPoints, -xOrg, -yOrg);
}

/* MINOTMITER -- helper for wide line code, stuffed into GC.lineHelper by
 * validation routine
 *
 * internal vertexes are joined with a bevel or rounded joint as in PostScript. 
 * vertices at the beginning and end of the polyline
 * are given the appropriate end unless they meet at the same point in which
 * case they are drawn with the appropriate join.  */

typedef struct {
    int		c;
    SppPointRec	v0, v1, v2, v3, v4;
    SppPointRec	p1, p2, p3, p4;
} POLYLINEINFO;

void
miNotMiter (pDraw, pGC, caps, npt, pPts, xOrg, yOrg)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		caps;
    int		npt;
    SppPointPtr pPts;
    int		xOrg, yOrg;
{
    int		i, c,
		width = pGC->lineWidth,
    		capStyle = pGC->capStyle,
    		joinStyle = pGC->joinStyle;
    Bool	edges_match;
    SppPointRec	FirstEdge[4],	/* 2 extra for special npt == 2 */
    		wedge[3],
    		p, p1, p2, p3, p4, p5, p6, p7, p8, center;
    POLYLINEINFO *plinfo;

    /* special case length 1 polyline */
    if(npt <= 2)
    {
        miOneSegWide(pDraw, pGC, npt, pPts, caps, xOrg, yOrg);
	return;
    }
    if(!(plinfo = (POLYLINEINFO *) ALLOCATE_LOCAL(npt * sizeof(POLYLINEINFO))))
	return;
    miGetPts(pPts[0], pPts[1], &p1, &p2, &p3, &p4, width);
    /* save the points, in case this is hard */
    plinfo[0].p1 = p1;
    plinfo[0].p2 = p2;
    plinfo[0].p3 = p3;
    plinfo[0].p4 = p4;

    /*
     * If ends meet, calculate angle of final intersection and points
     * of intersection for it.
     */
    if(PtEqual(pPts[0], pPts[npt-1]))
    {
	edges_match = TRUE;
	miGetPts(pPts[npt-2], pPts[npt-1], &p5, &p6, &p7, &p8, width);

	c = (pPts[0].x - pPts[npt-2].x)*(pPts[1].y - pPts[0].y) -
	    (pPts[1].x - pPts[0].x)*(pPts[0].y - pPts[npt-2].y);
	plinfo[0].c = c;

	if(c > 0) 	/* it's a clockwise turn */
	{
	    p = IntersectLines(p1, p2, p5, p6);
	    plinfo[0].v0 = p4;
	    plinfo[0].v1 = p7;
	    plinfo[0].v2 = p;
	    FirstEdge[0] = p;
	    FirstEdge[1] = p7;
	    if(joinStyle == JoinRound)
	    {
		center.x = (p4.x + p7.x)/2;
		center.y = (p4.y + p7.y)/2;
		miRoundCap(pDraw, pGC, center, p, p4, p7, NotEnd, xOrg, yOrg);
	    }
	}
	else if (c < 0)	/* counterclockwise */
	{
	    p = IntersectLines(p3, p4, p7, p8);
	    plinfo[0].v0 = p;
	    plinfo[0].v1 = p6;
	    plinfo[0].v2 = p1;
	    FirstEdge[0] = p6;
	    FirstEdge[1] = p;
	    if(joinStyle == JoinRound)
	    {
		center.x = (p6.x + p1.x)/2;
		center.y = (p6.y + p1.y)/2;
		miRoundCap(pDraw, pGC, center, p, p1, p6, NotEnd, xOrg, yOrg);
	    }
	}
	else /* straight line intersection */
	{
	    plinfo[0].v1 = p4;
	    plinfo[0].v2 = p1;
	    FirstEdge[0] = p6;
	    FirstEdge[1] = p7;

	}
    }
    else	/* Ends don't meet */
    {
	if(caps)
	{
	    if(capStyle == CapProjecting)
	    {
		pPts[0] = miExtendSegment(pPts[0], pPts[1], width/2);
		miGetPts(pPts[0], pPts[1], &p1, &p2, &p3, &p4, width);
		plinfo[0].p1 = p1;
		plinfo[0].p2 = p2;
		plinfo[0].p3 = p3;
		plinfo[0].p4 = p4;
		pPts[npt-1] =
		    miExtendSegment(pPts[npt-1], pPts[npt-2], width/2);
	    } else if (capStyle == CapRound)
	    {
		miRoundCap(pDraw, pGC, pPts[0], pPts[1], p1, p4, FirstEnd,
			 xOrg, yOrg);
		miGetPts(pPts[npt-1], pPts[npt-2], &p5, &p6, &p7, &p8, width);
		miRoundCap(pDraw, pGC, pPts[npt-1], pPts[npt-2], p5, p8,
			 FirstEnd, xOrg, yOrg);
	    }
	}
	edges_match = FALSE;
	plinfo[0].c = 0;	/* treat no connection as straight connection */
	plinfo[0].v1 = p4;
	plinfo[0].v2 = p1;

    }

    for (i = 1; i < (npt - 1); i++)
    {
	/* Find the point of intersection with the previous segment.
	 * Make sure it's on this segment */
	if(plinfo[i-1].c > 0)
	{
	    p = plinfo[i-1].v2;
	}
	else if (plinfo[i-1].c < 0)
	{
	    p = plinfo[i-1].v0;
	}
	miGetPts(pPts[i], pPts[i+1], &p5, &p6, &p7, &p8, width);
	
	plinfo[i].p1 = p5;
	plinfo[i].p2 = p6;
	plinfo[i].p3 = p7;
	plinfo[i].p4 = p8;

	/* Figure out whether this angle turns clockwise or not by taking
	 * the total cross product of the three vectors formed by drawing
	 * vectors between the start of the previous segment and the start
	 * and end of this one */
	if(i == npt - 1)
	    c = 0;
	else
	    c = (pPts[i].x - pPts[i-1].x)*(pPts[i + 1].y - pPts[i].y) -
		 (pPts[i + 1].x - pPts[i].x)*(pPts[i].y - pPts[i-1].y);
	plinfo[i].c = c;


	if(c > 0) 	/* counter-clockwise */		
	{
	    p = IntersectLines(p1, p2, p5, p6);
	    plinfo[i].v0 = p8;
	    plinfo[i].v1 = p3;
	    plinfo[i].v2 = p;

	    plinfo[i-1].v3 = p;
	    plinfo[i-1].v4 = p3;
	    if(joinStyle == JoinRound)
	    {
		center.x = (p8.x + p3.x)/2;
		center.y = (p8.y + p3.y)/2;
		miRoundCap(pDraw, pGC, center, p, p8, p3, NotEnd, xOrg, yOrg);
	    }
	}
	else if (c < 0)	
	{
	    p = IntersectLines(p3, p4, p7, p8);
	    plinfo[i].v0 = p;
	    plinfo[i].v1 = p2;
	    plinfo[i].v2 = p5;

	    plinfo[i-1].v3 = p2;
	    plinfo[i-1].v4 = p;
	    if(joinStyle == JoinRound)
	    {
		center.x = (p2.x + p5.x)/2;
		center.y = (p2.y + p5.y)/2;
		miRoundCap(pDraw, pGC, center, p, p2, p5, NotEnd, xOrg, yOrg);
	    }
	}
	else 			/* straight line intersection */
	{
	    plinfo[i].v1 = p2;
	    plinfo[i].v2 = p3;
	    plinfo[i-1].v3 = p2;
	    plinfo[i-1].v4 = p3;

	}

	p1 = p5;  p2 = p6;  p3 = p7;  p4 = p8;
    }

    /* Special case for final segment --
    */
    if(plinfo[i-1].c > 0)
    {
	p = plinfo[i-1].v2;
    }
    else if (plinfo[i-1].c < 0)
    {
	p = plinfo[i-1].v0;
    }

    if (edges_match)
    {
	plinfo[i-1].v3 = FirstEdge[0];
	plinfo[i-1].v4 = FirstEdge[1];
	if(plinfo[i-1].c > 0)
	{
	    p = plinfo[i-1].v2;
	}
	else if (plinfo[i-1].c < 0)
	{
	    p = plinfo[i-1].v0;
	}
    }
    else
    {
	miGetPts(pPts[npt-2], pPts[npt-1], &p5, &p6, &p7, &p8, width);
	plinfo[i-1].v3 = p6;
	plinfo[i-1].v4 = p7;
    }
    /*
     * plinfo[i].c is 0 if there is no notch, either because the lines meet
     * at 180 degrees, or the lines don't meet.  
     * Since we've set the alu to GCCopy, we don't worry about drawing
     * points twice.
     */
    for(i = 0; i < (npt - 1); i++)
    {
	if(plinfo[i].c < 0)
	{
	    wedge[1] = plinfo[i].p1;
	    wedge[2] = i ? plinfo[i - 1].p2 : plinfo[npt-2].p2;
	}
	else if (plinfo[i].c > 0)
	{
	    wedge[1] = plinfo[i].p4;
	    wedge[2] = i ? plinfo[i-1].p3 : plinfo[npt-2].p3;
	}

	if(plinfo[i].c)
	{
	    wedge[0] = pPts[i];
	    /* Check that points aren't colinear */
	    if(!(wedge[0].x == wedge[1].x && wedge[1].x == wedge[2].x) &&
	       !(wedge[0].y == wedge[1].y && wedge[1].y == wedge[2].y))
	    {
		miFillSppPoly(pDraw, pGC, 3, wedge, -xOrg, -yOrg);
	    }
	}
	miFillSppPoly(pDraw, pGC, 4, &plinfo[i].p1, -xOrg, -yOrg);
    }
    DEALLOCATE_LOCAL(plinfo);	
}

/* MIGETPTS -- helper function
 *  Get the corners of the polygon formed for a line with the endpoints
 *  p1In, p2In and the given width stuff the corners into *p1Out, *p2Out ...
 *  p1Out and p4Out are always on the same end as p1In.
 */
void
miGetPts(p1In, p2In, p1Out, p2Out, p3Out, p4Out, width)
    SppPointRec p1In, p2In;
    SppPointPtr p1Out, p2Out, p3Out, p4Out;
    int width;
{
    double hyp;         /* true distance of line (hypotenuse) */
    double dx, dy;
    double tmpdx, tmpdy;

    tmpdx = (double)(p1In.x - p2In.x);
    tmpdy = (double)(p1In.y - p2In.y);

    hyp = hypot(tmpdx, tmpdy);
    if (!ISZERO(hyp))
    {
    /* NOTE: the division by 2 must be done in floating point */
	dy = (tmpdx * width/2.0) / hyp;
	dx = (tmpdy * width/2.0) / hyp;

	p1Out->x = (p1In.x + dx);
	p1Out->y = (p1In.y - dy);

	p2Out->x = (p2In.x + dx);
	p2Out->y = (p2In.y - dy);

	p3Out->x = (p2In.x - dx);
	p3Out->y = (p2In.y + dy);

	p4Out->x = (p1In.x - dx);
	p4Out->y = (p1In.y + dy);
    }
    else
    {    
	/* Bizarre answer, but it's what the spec calls for */
	p1Out->x = p2Out->x = p1In.x - width/2.0;
	p3Out->x = p4Out->x = p1In.x + width/2.0;
	p1Out->y = p2Out->y = p3Out->y = p4Out->y = p1In.y;
    }
}

/* INTERSECTLINES -- private helper for line code 
 *  Return the intersection of the line from p1 to p2 with
 *  the line from p3 to p4.
 *
 *  First, reduce the lines to:   y = m1x + b,  and y = m2x + c.
 *  Then do the standard math.
 */
static
SppPointRec
IntersectLines(p1, p2, p3, p4)
    SppPointRec p1, p2, p3, p4;
{
    SppPointRec i;
    double m1, m2;
    double b1, b2;
    double dx1, dx2;
    double tx;

    dx1 = p2.x - p1.x;
    dx2 = p4.x - p3.x;

    if (ISZERO(dx1))
    {
	if (! ISZERO(dx2))
	{
	    i.y = p3.y + ((p4.y - p3.y) * (p1.x - p3.x)) / (p4.x - p3.x);
	    i.x = p1.x;
	}
	else
	    i = p2;
    }
    else if (ISZERO(dx2))
    {
	i.y = p1.y + ((p2.y - p1.y) * (p3.x - p1.x)) / (p2.x - p1.x);
	i.x = p3.x;
    }
    else
    {
	m1 = (p2.y - p1.y) / dx1;
	m2 = (p4.y - p3.y) / dx2;

	b1 = p2.y - (m1 * p2.x);
	b2 = p4.y - (m2 * p4.x);

	if (! ISEQUAL(m1, m2))
	    i.x = (tx = ((b2-b1) / (m1-m2)));
	else
	    i.x = (tx = p2.x);

	i.y = m1 * tx + b1;
    }
    return(i);
}


/* MIEXTENDSEGMENT -- a private helper function 
 * Calcuate point p3 made by extending the segment backward (away from p2)
 * by a length d */

SppPointRec
miExtendSegment(p1, p2, d)
    SppPointRec	p1, p2;
    int		d;
{
    SppPointRec	p3;
    double	l, dx, dy, hypot();

    dx = p2.x - p1.x;
    dy = p2.y - p1.y;
    l = hypot(dx, dy);
    if(!ISZERO(l))
    {
	p3.x = (p2.x - (l + d)/l * dx);
	p3.y = (p2.y - (l + d)/l * dy);
    }
    else
    {
	ErrorF("ExtendSegment called with coincident points.\n");
	p3 = p2;
    }
    return(p3);
}

/* MIROUNDCAP -- a private helper function
 * Put Rounded cap on end. pCenter is the center of this end of the line
 * pEnd is the center of the other end of the line. pCorner is one of the
 * two corners at this end of the line.  It doesn't matter which of the two
 * corners you give it. pOtherCorner is the other one.
 */
void
miRoundCap(pDraw, pGC, pCenter, pEnd, pCorner, pOtherCorner, fLineEnd,
     xOrg, yOrg)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    SppPointRec	pCenter, pEnd;
    SppPointRec	pCorner, pOtherCorner;
    int		fLineEnd, xOrg, yOrg;
{
    int		c, i, signc, cpt;
    double	width;
    double	dx, dy, hypot();
    xArc	arc;
    DDXPointPtr	pArcPts;
    SppPointPtr	pPts;
    int		arcmode = pGC->arcMode;

    if(arcmode != ArcChord)
    {
	DoChangeGC(pGC, GCArcMode, &gcvals[GCVALSARCMODE], 0);
	ValidateGC(pDraw, pGC);
    }
    if (fLineEnd == NotEnd)
    {
	dx = pCenter.x - pCorner.x;
	dy = pCenter.y - pCorner.y;
	width = hypot(dx, dy);
    }
    else
	width = pGC->lineWidth;
    if(PtEqual(pCenter, pEnd))
    {
	signc = 1;
    }
    else
    {
	c = (pCenter.x - pEnd.x) * (pCorner.y - pCenter.y) -
	     (pCorner.x - pCenter.x) * (pCenter.y - pEnd.y);
	signc = -sign(c);
    }

    /* These come back scaled by 64 and all ready to use */
    arc.x = ROUNDTOINT(pCenter.x - width/2);
    arc.y = ROUNDTOINT(pCenter.y - width/2);
    arc.width = ROUNDTOINT(width);
    arc.height = arc.width;
    arc.angle1 = -PtToAngle(pCenter, pCorner);
    arc.angle2 = -signc * 180 * 64;
    pArcPts = (DDXPointPtr)NULL;
    if( cpt = miGetArcPts(&arc, 0, &pArcPts))
    {
	/* by drawing with miFillSppPoly and setting the endpoints of the arc
	 * to be the corners, we assure that the cap will meet up with the
	 * rest of the line */
	pPts = (SppPointPtr) Xalloc(cpt * sizeof(SppPointRec));
	for(i = 1; i < cpt -1; i++)
	{
	    pPts[i].x = (double) pArcPts[i].x;
	    pPts[i].y = (double) pArcPts[i].y;
	}
	pPts[0] = pCorner;
	pPts[cpt - 1] = pOtherCorner;
	miFillSppPoly(pDraw, pGC, cpt, pPts, -xOrg, -yOrg);
	Xfree((char *)pArcPts);
	Xfree((char *)pPts);
	if(arcmode != ArcChord)
	{
	    DoChangeGC(pGC, GCArcMode, &arcmode, 0);
	    ValidateGC(pDraw, pGC);
	}
    }
}

/* these are from <math.h>, but I'm told some systems don't have math.h Psi! */
extern double sqrt(), cos(), sin(), atan();
#define M_PI	3.14159265358979323846
#define M_PI_2	1.57079632679489661923

/* PTTOANGLE -- a private helper function
 * Given two points, compute the slope of the line segment
 * Result is in radians * 64
 */
static
int
PtToAngle(c, p)
    SppPointRec	c, p;
{
    double	dx, dy, theta;

    dx = p.x - c.x;
    dy  = p.y - c.y;
    if(dx == 0)
    {
	theta = (dy > 0) ? M_PI_2 : -M_PI_2;
    }
    else
        theta =(atan(dy/dx));
    if(dx < 0)
	theta += M_PI;
    theta = (theta * 64 * 180/M_PI);
    return(ROUNDTOINT(theta));
}

