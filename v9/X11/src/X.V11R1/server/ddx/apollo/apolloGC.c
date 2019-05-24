/*-
 * apolloGC.c --
 *	Functions to support the meddling with GC's we do to preserve
 *	the software cursor...
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

#include "apollo.h"
#include    "../mi/mifpoly.h"		/* for SppPointPtr */

apProcPtrs apProcs;

/*
 * Overlap BoxPtr and Box elements
 */
#define BOX_OVERLAP(pCbox,X1,Y1,X2,Y2) \
 	(((pCbox)->x1 <= (X2)) && ((X1) <= (pCbox)->x2) && \
	 ((pCbox)->y1 <= (Y2)) && ((Y1) <= (pCbox)->y2))

/*
 * Overlap BoxPtr, origins, and rectangle
 */
#define ORG_OVERLAP(pCbox,xorg,yorg,x,y,w,h) \
    BOX_OVERLAP((pCbox),(x)+(xorg),(y)+(yorg),(x)+(xorg)+(w),(y)+(yorg)+(h))

/*
 * Overlap BoxPtr, origins and RectPtr
 */
#define ORGRECT_OVERLAP(pCbox,xorg,yorg,pRect) \
    ORG_OVERLAP((pCbox),(xorg),(yorg),(pRect)->x,(pRect)->y,(pRect)->width, \
		(pRect)->height)
/*
 * Overlap BoxPtr and horizontal span
 */
#define SPN_OVERLAP(pCbox,y,x,w) BOX_OVERLAP((pCbox),(x),(y),(x)+(w),(y))


#ifdef	notdef
#define	COMPARE_GCS(g1, g2)	compare_gcs(g1, g2)

static
compare_gcs(g1, g2)
GCPtr g1, g2;
{
    if (g1->pScreen != g2->pScreen) FatalError("GC and Shadow mis-match - pScreen\n");
    if (g1->depth != g2->depth) FatalError("GC and Shadow mis-match - depth\n");
#ifdef	notdef
    if (g1->serialNumber != g2->serialNumber) FatalError("GC and Shadow mis-match - serialNumber\n");
#endif
    if (g1->alu != g2->alu) FatalError("GC and Shadow mis-match - alu\n");
    if (g1->planemask != g2->planemask) FatalError("GC and Shadow mis-match - planemask\n");
    if (g1->fgPixel != g2->fgPixel) FatalError("GC and Shadow mis-match - fgPixel\n");
    if (g1->bgPixel != g2->bgPixel) FatalError("GC and Shadow mis-match - bgPixel\n");
    if (g1->lineWidth != g2->lineWidth) FatalError("GC and Shadow mis-match - lineWidth\n");
    if (g1->lineStyle != g2->lineStyle) FatalError("GC and Shadow mis-match - lineStyle\n");
    if (g1->capStyle != g2->capStyle) FatalError("GC and Shadow mis-match - capStyle\n");
    if (g1->joinStyle != g2->joinStyle) FatalError("GC and Shadow mis-match - joinStyle\n");
    if (g1->fillStyle != g2->fillStyle) FatalError("GC and Shadow mis-match - fillStyle\n");
    if (g1->fillRule != g2->fillRule) FatalError("GC and Shadow mis-match - fillRule\n");
    if (g1->arcMode != g2->arcMode) FatalError("GC and Shadow mis-match - arcMode\n");
    if (g1->tile != g2->tile) FatalError("GC and Shadow mis-match - tile\n");
    if (g1->stipple != g2->stipple) FatalError("GC and Shadow mis-match - stipple\n");
    if (g1->patOrg.x != g2->patOrg.x) FatalError("GC and Shadow mis-match - patOrg.x\n");
    if (g1->patOrg.y != g2->patOrg.y) FatalError("GC and Shadow mis-match - patOrg.y\n");
    if (g1->font != g2->font) FatalError("GC and Shadow mis-match - font\n");
    if (g1->subWindowMode != g2->subWindowMode) FatalError("GC and Shadow mis-match - subWindowMode\n");
    if (g1->graphicsExposures != g2->graphicsExposures) FatalError("GC and Shadow mis-match - graphicsExposures\n");
    if (g1->clipOrg.x != g2->clipOrg.x) FatalError("GC and Shadow mis-match - clipOrg.x\n");
    if (g1->clipOrg.y != g2->clipOrg.y) FatalError("GC and Shadow mis-match - clipOrg.y\n");
#ifdef	notdef
    if (g1->clientClip != g2->clientClip) FatalError("GC and Shadow mis-match - clientClip\n");
#endif
    if (g1->clientClipType != g2->clientClipType) FatalError("GC and Shadow mis-match - clientClipType\n");
    if (g1->dashOffset != g2->dashOffset) FatalError("GC and Shadow mis-match - dashOffset\n");
    if (g1->numInDashList != g2->numInDashList) FatalError("GC and Shadow mis-match - numInDashList\n");
#ifdef	notdef
    if (g1->dash != g2->dash) FatalError("GC and Shadow mis-match - dash\n");
    if (g1->stateChanges != g2->stateChanges) FatalError("GC and Shadow mis-match - stateChanges\n");
#endif
    if (g1->lastWinOrg.x != g2->lastWinOrg.x) FatalError("GC and Shadow mis-match - lastWinOrg.x\n");
    if (g1->lastWinOrg.y != g2->lastWinOrg.y) FatalError("GC and Shadow mis-match - lastWinOrg.y\n");
    if (g1->miTranslate != g2->miTranslate) FatalError("GC and Shadow mis-match - miTranslate\n");

}
#else
#define	COMPARE_GCS(g1, g2)
#endif


/*-
 *-----------------------------------------------------------------------
 * apSaveCursorBox --
 *	Given an array of points, figure out the bounding box for the
 *	series and remove the cursor if it overlaps that box.
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apSaveCursorBox (xorg, yorg, mode, pPts, nPts, pCursorBox)
    register int  	    xorg;   	    /* X-Origin for points */
    register int  	    yorg;   	    /* Y-Origin for points */
    int	    	  	    mode;   	    /* CoordModeOrigin or
					     * CoordModePrevious */
    register DDXPointPtr    pPts;   	    /* Array of points */
    int	    	  	    nPts;   	    /* Number of points */
    register BoxPtr 	    pCursorBox;	    /* Bounding box for cursor */
{
    register int  	    minx,
			    miny,
			    maxx,
			    maxy;

    minx = maxx = pPts->x + xorg;
    miny = maxy = pPts->y + yorg;

    pPts++;
    nPts--;

    if (mode == CoordModeOrigin) {
	while (nPts--) {
	    minx = min(minx, pPts->x + xorg);
	    maxx = max(maxx, pPts->x + xorg);
	    miny = min(miny, pPts->y + yorg);
	    maxy = max(maxy, pPts->y + yorg);
	    pPts++;
	}
    } else {
	xorg = minx;
	yorg = miny;
	while (nPts--) {
	    minx = min(minx, pPts->x + xorg);
	    maxx = max(maxx, pPts->x + xorg);
	    miny = min(miny, pPts->y + yorg);
	    maxy = max(maxy, pPts->y + yorg);
	    xorg += pPts->x;
	    yorg += pPts->y;
	    pPts++;
	}
    }
    if (BOX_OVERLAP(pCursorBox,minx,miny,maxx,maxy)) {
	apRemoveCursor(TRUE);
    }
}
		       
/*-
 *-----------------------------------------------------------------------
 * apFillSpans --
 *	Remove the cursor if any of the spans overlaps the area covered
 *	by the cursor. This assumes the points have been translated
 *	already, though perhaps it shouldn't...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apFillSpans(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nInit;			/* number of spans to fill */
    DDXPointPtr pptInit;		/* pointer to list of start points */
    int		*pwidthInit;		/* pointer to list of n widths */
    int 	fSorted;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;

    if (pDrawable->type == DRAWABLE_WINDOW) {
	BoxRec	  cursorBox;

	if (apCursorLoc (pDrawable->pScreen, &cursorBox)) {
	    register DDXPointPtr    pts;
	    register int    	    *widths;
	    register int    	    nPts;

	    for (pts = pptInit, widths = pwidthInit, nPts = nInit;
		 nPts--;
		 pts++, widths++) {
		     if (SPN_OVERLAP(&cursorBox,pts->y,pts->x,*widths)) {
			 apRemoveCursor(TRUE);
			 break;
		     }
	    }
	}
    }

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->FillSpans)(pDrawable, pShadowGC, nInit, pptInit,
			     pwidthInit, fSorted);
}

/*-
 *-----------------------------------------------------------------------
 * apSetSpans --
 *	Remove the cursor if any of the horizontal segments overlaps
 *	the area covered by the cursor. This also assumes the spans
 *	have been translated from the window's coordinates to the
 *	screen's.
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apSetSpans(pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted)
    DrawablePtr		pDrawable;
    GCPtr		pGC;
    int			*psrc;
    register DDXPointPtr ppt;
    int			*pwidth;
    int			nspans;
    int			fSorted;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;

    if (pDrawable->type == DRAWABLE_WINDOW) {
	BoxRec	  cursorBox;

	if (apCursorLoc (pDrawable->pScreen, &cursorBox)) {
	    register DDXPointPtr    pts;
	    register int    	    *widths;
	    register int    	    nPts;

	    for (pts = ppt, widths = pwidth, nPts = nspans;
		 nPts--;
		 pts++, widths++) {
		     if (SPN_OVERLAP(&cursorBox,pts->y,pts->x,*widths)) {
			 apRemoveCursor(TRUE);
			 break;
		     }
	    }
	}
    }

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->SetSpans) (pDrawable, pShadowGC, psrc, ppt, pwidth,
			     nspans, fSorted);
}

/*-
 *-----------------------------------------------------------------------
 * apGetSpans --
 *	Remove the cursor if any of the desired spans overlaps the cursor.
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
unsigned int *
apGetSpans(pDrawable, wMax, ppt, pwidth, nspans)
    DrawablePtr		pDrawable;	/* drawable from which to get bits */
    int			wMax;		/* largest value of all *pwidths */
    register DDXPointPtr ppt;		/* points to start copying from */
    int			*pwidth;	/* list of number of bits to copy */
    int			nspans;		/* number of scanlines to copy */
{
    if (pDrawable->type == DRAWABLE_WINDOW) {
	BoxRec	  cursorBox;

	if (apCursorLoc (pDrawable->pScreen, &cursorBox)) {
	    register DDXPointPtr    pts;
	    register int    	    *widths;
	    register int    	    nPts;
	    register int    	    xorg,
				    yorg;

	    xorg = ((WindowPtr)pDrawable)->absCorner.x;
	    yorg = ((WindowPtr)pDrawable)->absCorner.y;

	    for (pts = ppt, widths = pwidth, nPts = nspans;
		 nPts--;
		 pts++, widths++) {
		     if (SPN_OVERLAP(&cursorBox,pts->y+yorg,
				     pts->x+xorg,*widths)) {
					 apRemoveCursor(TRUE);
					 break;
		     }
	    }
	}
    }

    /*
     * XXX: Because we have no way to get at the GC used to call us,
     * we must rely on the GetSpans vector never changing and stick it
     * in the fbFd structure. Gross.
     */
    return (* apProcs.GetSpans) (pDrawable, wMax, ppt, pwidth, nspans);
}

/*-
 *-----------------------------------------------------------------------
 * apPutImage --
 *	Remove the cursor if it is in the way of the image to be
 *	put down...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apPutImage(pDst, pGC, depth, x, y, w, h, leftPad, format, pBits)
    DrawablePtr	  pDst;
    GCPtr   	  pGC;
    int		  depth;
    int	    	  x;
    int	    	  y;
    int	    	  w;
    int	    	  h;
    int	    	  format;
    char    	  *pBits;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;

    if (pDst->type == DRAWABLE_WINDOW) {
	BoxRec	  cursorBox;
	if (apCursorLoc (pDst->pScreen, &cursorBox)) {
	    register WindowPtr pWin = (WindowPtr)pDst;

	    if (ORG_OVERLAP(&cursorBox,pWin->absCorner.x,pWin->absCorner.y,
			    x,y,w,h)) {
				apRemoveCursor(TRUE);
	    }
	}
    }

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->PutImage) (pDst, pShadowGC, depth, x, y, w, h,
			     leftPad, format, pBits);
}

/*-
 *-----------------------------------------------------------------------
 * apGetImage --
 *	Remove the cursor if it overlaps the image to be gotten.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apGetImage (pSrc, x, y, w, h, format, planeMask, pBits)
    DrawablePtr	  pSrc;
    int	    	  x;
    int	    	  y;
    int	    	  w;
    int	    	  h;
    unsigned int  format;
    unsigned int  planeMask;
    int	    	  *pBits;
{
    if (pSrc->type == DRAWABLE_WINDOW) {
	BoxRec	  cursorBox;

	if (apCursorLoc(pSrc->pScreen, &cursorBox)) {
	    register WindowPtr	pWin = (WindowPtr)pSrc;

	    if (ORG_OVERLAP(&cursorBox,pWin->absCorner.x,pWin->absCorner.y,
			    x,y,w,h)) {
				apRemoveCursor(TRUE);
	    }
	}
    }

	(* apProcs.GetImage) (pSrc, x, y, w, h, format, planeMask, pBits);
}

/*-
 *-----------------------------------------------------------------------
 * apCopyArea --
 *	Remove the cursor if it overlaps either the source or destination
 *	drawables, then call the screen-specific CopyArea routine.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apCopyArea (pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty)
    DrawablePtr	  pSrc;
    DrawablePtr	  pDst;
    GCPtr   	  pGC;
    int	    	  srcx;
    int	    	  srcy;
    int	    	  w;
    int	    	  h;
    int	    	  dstx;
    int	    	  dsty;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec	  cursorBox;
    register WindowPtr	pWin;
    int	    	  out = FALSE;

    if (pSrc->type == DRAWABLE_WINDOW &&
	apCursorLoc(pSrc->pScreen, &cursorBox)) {
	    pWin = (WindowPtr)pSrc;

	    if (ORG_OVERLAP(&cursorBox,pWin->absCorner.x,pWin->absCorner.y,
			    srcx, srcy, w, h)) {
				apRemoveCursor(TRUE);
				out = TRUE;
	    }
    }

    if (!out && pDst->type == DRAWABLE_WINDOW &&
	apCursorLoc(pDst->pScreen, &cursorBox)) {
	    pWin = (WindowPtr)pDst;
	    
	    if (ORG_OVERLAP(&cursorBox,pWin->absCorner.x,pWin->absCorner.y,
			    dstx, dsty, w, h)) {
				apRemoveCursor(TRUE);
	    }
    }

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->CopyArea) (pSrc, pDst, pShadowGC, srcx, srcy,
			     w, h, dstx, dsty);
}

/*-
 *-----------------------------------------------------------------------
 * apCopyPlane --
 *	Remove the cursor as necessary and call the screen-specific
 *	CopyPlane function.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apCopyPlane (pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty, plane)
    DrawablePtr	  pSrc;
    DrawablePtr	  pDst;
    register GC   *pGC;
    int     	  srcx,
		  srcy;
    int     	  w,
		  h;
    int     	  dstx,
		  dsty;
    unsigned int  plane;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec	  cursorBox;
    register WindowPtr	pWin;
    int	    	  out = FALSE;

    if (pSrc->type == DRAWABLE_WINDOW &&
	apCursorLoc(pSrc->pScreen, &cursorBox)) {
	    pWin = (WindowPtr)pSrc;

	    if (ORG_OVERLAP(&cursorBox,pWin->absCorner.x,pWin->absCorner.y,
			    srcx, srcy, w, h)) {
				apRemoveCursor(TRUE);
				out = TRUE;
	    }
    }

    if (!out && pDst->type == DRAWABLE_WINDOW &&
	apCursorLoc(pDst->pScreen, &cursorBox)) {
	    pWin = (WindowPtr)pDst;
	    
	    if (ORG_OVERLAP(&cursorBox,pWin->absCorner.x,pWin->absCorner.y,
			    dstx, dsty, w, h)) {
				apRemoveCursor(TRUE);
	    }
    }

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->CopyPlane) (pSrc, pDst, pShadowGC, srcx, srcy, w, h,
			      dstx, dsty, plane);
}

/*-
 *-----------------------------------------------------------------------
 * apPolyPoint --
 *	See if any of the points lies within the area covered by the
 *	cursor and remove the cursor if one does. Then put the points
 *	down.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apPolyPoint (pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		mode;		/* Origin or Previous */
    int		npt;
    xPoint 	*pptInit;
{
    register GCPtr 	pShadowGC = (GCPtr) pGC->devPriv;
    register xPoint 	*pts;
    register int  	nPts;
    register int  	xorg;
    register int  	yorg;
    BoxRec  	  	cursorBox;

    if (pDrawable->type == DRAWABLE_WINDOW &&
	apCursorLoc (pDrawable->pScreen, &cursorBox)) {
	    xorg = ((WindowPtr)pDrawable)->absCorner.x;
	    yorg = ((WindowPtr)pDrawable)->absCorner.y;

	    if (mode == CoordModeOrigin) {
		for (pts = pptInit, nPts = npt; nPts--; pts++) {
		    if (ORG_OVERLAP(&cursorBox,xorg,yorg,pts->x,pts->y,0,0)){
			apRemoveCursor(TRUE);
			break;
		    }
		}
	    } else {
		for (pts = pptInit, nPts = npt; nPts--; pts++) {
		    if (ORG_OVERLAP(&cursorBox,xorg,yorg,pts->x,pts->y,0,0)){
			apRemoveCursor(TRUE);
			break;
		    } else {
			xorg += pts->x;
			yorg += pts->y;
		    }
		}
	    }
    }

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->PolyPoint) (pDrawable, pShadowGC, mode, npt, pptInit);
}

/*-
 *-----------------------------------------------------------------------
 * apPolylines --
 *	Find the bounding box of the lines and remove the cursor if
 *	the box overlaps the area covered by the cursor. Then call
 *	the screen's Polylines function to draw the lines themselves.
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apPolylines (pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr	  pDrawable;
    GCPtr   	  pGC;
    int	    	  mode;
    int	    	  npt;
    DDXPointPtr	  pptInit;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec  	  cursorBox;

    if (pDrawable->type == DRAWABLE_WINDOW &&
	apCursorLoc (pDrawable->pScreen, &cursorBox)) {
	    apSaveCursorBox(((WindowPtr)pDrawable)->absCorner.x,
			     ((WindowPtr)pDrawable)->absCorner.y,
			     mode,
			     pptInit,
			     npt,
			     &cursorBox);
    }
    COMPARE_GCS(pGC,pShadowGC);
    (*pShadowGC->Polylines) (pDrawable, pShadowGC, mode, npt, pptInit);
}

/*-
 *-----------------------------------------------------------------------
 * apPolySegment --
 *	Treat each segment as a box and remove the cursor if any box
 *	overlaps the cursor's area. Then draw the segments. Note that
 *	the endpoints of the segments are in no way guaranteed to be
 *	in the right order, so we find the bounding box of the segment
 *	in two comparisons and use that to figure things out.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apPolySegment(pDraw, pGC, nseg, pSegs)
    DrawablePtr pDraw;
    GCPtr 	pGC;
    int		nseg;
    xSegment	*pSegs;
{
    register GCPtr 	pShadowGC = (GCPtr) pGC->devPriv;
    register xSegment	*pSeg;
    register int  	nSeg;
    register int  	xorg,
			yorg;
    BoxRec  	  	cursorBox;
    Bool    	  	nuke = FALSE;

    if (pDraw->type == DRAWABLE_WINDOW &&
	apCursorLoc (pDraw->pScreen, &cursorBox)) {
	    xorg = ((WindowPtr)pDraw)->absCorner.x;
	    yorg = ((WindowPtr)pDraw)->absCorner.y;

	    for (nSeg = nseg, pSeg = pSegs; nSeg--; pSeg++) {
		if (pSeg->x1 < pSeg->x2) {
		    if (pSeg->y1 < pSeg->y2) {
			nuke = BOX_OVERLAP(&cursorBox,
					   pSeg->x1+xorg,pSeg->y1+yorg,
					   pSeg->x2+xorg,pSeg->y2+yorg);
		    } else {
			nuke = BOX_OVERLAP(&cursorBox,
					   pSeg->x1+xorg,pSeg->y2+yorg,
					   pSeg->x2+xorg,pSeg->y1+yorg);
		    }
		} else if (pSeg->y1 < pSeg->y2) {
		    nuke = BOX_OVERLAP(&cursorBox,
				       pSeg->x2+xorg,pSeg->y1+yorg,
				       pSeg->x1+xorg,pSeg->y2+yorg);
		} else {
		    nuke = BOX_OVERLAP(&cursorBox,
				       pSeg->x2+xorg,pSeg->y2+yorg,
				       pSeg->x1+xorg,pSeg->y1+yorg);
		}
		if (nuke) {
		    apRemoveCursor(TRUE);
		    break;
		}
	    }
    }

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->PolySegment) (pDraw, pShadowGC, nseg, pSegs);
}

/*-
 *-----------------------------------------------------------------------
 * apPolyRectangle --
 *	Remove the cursor if it overlaps any of the rectangles to be
 *	drawn, then draw them.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apPolyRectangle(pDraw, pGC, nrects, pRects)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		nrects;
    xRectangle	*pRects;
{
    register GCPtr 	pShadowGC = (GCPtr) pGC->devPriv;
    register xRectangle	*pRect;
    register int  	nRect;
    register int  	xorg,
			yorg;
    BoxRec  	  	cursorBox;

    if (pDraw->type == DRAWABLE_WINDOW &&
	apCursorLoc (pDraw->pScreen, &cursorBox)) {
	    xorg = ((WindowPtr)pDraw)->absCorner.x;
	    yorg = ((WindowPtr)pDraw)->absCorner.y;

	    for (nRect = nrects, pRect = pRects; nRect--; pRect++) {
		if (ORGRECT_OVERLAP(&cursorBox,xorg,yorg,pRect)){
		    apRemoveCursor(TRUE);
		    break;
		}
	    }
    }

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->PolyRectangle) (pDraw, pShadowGC, nrects, pRects);
}

/*-
 *-----------------------------------------------------------------------
 * apPolyArc --
 *	Using the bounding rectangle of each arc, remove the cursor
 *	if it overlaps any arc, then draw all the arcs.
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apPolyArc(pDraw, pGC, narcs, parcs)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
    register GCPtr 	pShadowGC = (GCPtr) pGC->devPriv;
    register xArc	*pArc;
    register int  	nArc;
    register int  	xorg,
			yorg;
    BoxRec  	  	cursorBox;

    if (pDraw->type == DRAWABLE_WINDOW &&
	apCursorLoc (pDraw->pScreen, &cursorBox)) {
	    xorg = ((WindowPtr)pDraw)->absCorner.x;
	    yorg = ((WindowPtr)pDraw)->absCorner.y;

	    for (nArc = narcs, pArc = parcs; nArc--; pArc++) {
		if (ORGRECT_OVERLAP(&cursorBox,xorg,yorg,pArc)){
		    apRemoveCursor(TRUE);
		    break;
		}
	    }
    }

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->PolyArc) (pDraw, pShadowGC, narcs, parcs);
}

/*-
 *-----------------------------------------------------------------------
 * apFillPolygon --
 *	Find the bounding box of the polygon to fill and remove the
 *	cursor if it overlaps this box...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apFillPolygon(pDraw, pGC, shape, mode, count, pPts)
    DrawablePtr		pDraw;
    register GCPtr	pGC;
    int			shape, mode;
    register int	count;
    DDXPointPtr		pPts;
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec  	  	cursorBox;

    if (pDraw->type == DRAWABLE_WINDOW &&
	apCursorLoc (pDraw->pScreen, &cursorBox)) {
	    apSaveCursorBox(((WindowPtr)pDraw)->absCorner.x,
			     ((WindowPtr)pDraw)->absCorner.y,
			     mode,
			     pPts,
			     count,
			     &cursorBox);
    }

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->FillPolygon) (pDraw, pShadowGC, shape, mode, count, pPts);
}

/*-
 *-----------------------------------------------------------------------
 * apPolyFillRect --
 *	Remove the cursor if it overlaps any of the filled rectangles
 *	to be drawn by the output routines.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apPolyFillRect(pDrawable, pGC, nrectFill, prectInit)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nrectFill; 	/* number of rectangles to fill */
    xRectangle	*prectInit;  	/* Pointer to first rectangle to fill */
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;
    register xRectangle	*pRect;
    register int  	nRect;
    register int  	xorg,
			yorg;
    BoxRec  	  	cursorBox;

    if (pDrawable->type == DRAWABLE_WINDOW &&
	apCursorLoc (pDrawable->pScreen, &cursorBox)) {
	    xorg = ((WindowPtr)pDrawable)->absCorner.x;
	    yorg = ((WindowPtr)pDrawable)->absCorner.y;

	    for (nRect = nrectFill, pRect = prectInit; nRect--; pRect++) {
		if (ORGRECT_OVERLAP(&cursorBox,xorg,yorg,pRect)){
		    apRemoveCursor(TRUE);
		    break;
		}
	    }
    }

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->PolyFillRect) (pDrawable, pShadowGC, nrectFill, prectInit);
}

/*-
 *-----------------------------------------------------------------------
 * apPolyFillArc --
 *	See if the cursor overlaps any of the bounding boxes for the
 *	filled arc and remove it if it does.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apPolyFillArc(pDraw, pGC, narcs, parcs)
    DrawablePtr	pDraw;
    GCPtr	pGC;
    int		narcs;
    xArc	*parcs;
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;
    register xArc	*pArc;
    register int  	nArc;
    register int  	xorg,
			yorg;
    BoxRec  	  	cursorBox;

    if (pDraw->type == DRAWABLE_WINDOW &&
	apCursorLoc (pDraw->pScreen, &cursorBox)) {
	    xorg = ((WindowPtr)pDraw)->absCorner.x;
	    yorg = ((WindowPtr)pDraw)->absCorner.y;

	    for (nArc = narcs, pArc = parcs; nArc--; pArc++) {
		if (ORGRECT_OVERLAP(&cursorBox,xorg,yorg,pArc)){
		    apRemoveCursor(TRUE);
		    break;
		}
	    }
    }

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->PolyFillArc) (pDraw, pShadowGC, narcs, parcs);
}

/*-
 *-----------------------------------------------------------------------
 * apText --
 *	Find the extent of a text operation and remove the cursor if they
 *	overlap. pDraw is assumed to be a window.
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
int
apText(pDraw, pGC, x, y, count, chars, fontEncoding, drawFunc)
    DrawablePtr   pDraw;
    GCPtr	  pGC;
    int		  x,
		  y;
    int		  count;
    char 	  *chars;
    FontEncoding  fontEncoding;
    void    	  (*drawFunc)();
{
    CharInfoPtr   *charinfo;
    unsigned int  n,
		  w;
    register int  xorg,
		  yorg;
    ExtentInfoRec extents;
    BoxRec  	  cursorBox;

    charinfo = (CharInfoPtr *)ALLOCATE_LOCAL (count * sizeof(CharInfoPtr));
    if (charinfo == (CharInfoPtr *)NULL) {
	return x;
    }

    GetGlyphs(pGC->font, count, chars, fontEncoding, &n, charinfo);

    if (apCursorLoc (pDraw->pScreen, &cursorBox)) {
	QueryGlyphExtents(pGC->font, charinfo, count, &extents);
	w = extents.overallWidth;
	xorg = ((WindowPtr)pDraw)->absCorner.x;
	yorg = ((WindowPtr)pDraw)->absCorner.y;
    
	if (BOX_OVERLAP(&cursorBox,
			x + xorg + extents.overallLeft,
			y + yorg - extents.overallAscent,
			x + xorg + extents.overallRight,
			y + yorg + extents.overallDescent)) {
			    apRemoveCursor(TRUE);
	}
    } else {
	w = 0;
    }
    
    if (n != 0) {
        (* drawFunc)(pDraw, pGC, x, y, n, charinfo, pGC->font->pGlyphs);
    }

    DEALLOCATE_LOCAL(charinfo);
    return x+w;
}

/*-
 *-----------------------------------------------------------------------
 * apPolyText8 --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
int
apPolyText8(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int 	count;
    char	*chars;
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;

    COMPARE_GCS(pGC,pShadowGC);
    if (pDraw->type == DRAWABLE_WINDOW) {
	return apText (pDraw, pShadowGC, x, y, count, chars, Linear8Bit,
			pShadowGC->PolyGlyphBlt);
    } else {
	return (* pShadowGC->PolyText8)(pDraw, pShadowGC, x, y, count, chars);
    }
}

/*-
 *-----------------------------------------------------------------------
 * apPolyText16 --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
int
apPolyText16(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    unsigned short *chars;
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;

    COMPARE_GCS(pGC,pShadowGC);
    if (pDraw->type == DRAWABLE_WINDOW) {
	return apText (pDraw, pShadowGC, x, y, count, chars,
			(pShadowGC->font->pFI->lastRow == 0 ?
			 Linear16Bit : TwoD16Bit),
			pShadowGC->PolyGlyphBlt);
    } else {
	return (* pShadowGC->PolyText16) (pDraw, pShadowGC, x, y,
					  count, chars);
    }
}

/*-
 *-----------------------------------------------------------------------
 * apImageText8 --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apImageText8(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    char	*chars;
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;

    COMPARE_GCS(pGC,pShadowGC);
    if (pDraw->type == DRAWABLE_WINDOW) {
	(void) apText (pDraw, pShadowGC, x, y, count, chars,
			Linear8Bit, pShadowGC->ImageGlyphBlt);
    } else {
	(* pShadowGC->ImageText8) (pDraw, pShadowGC, x, y, count, chars);
    }
}

/*-
 *-----------------------------------------------------------------------
 * apImageText16 --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apImageText16(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    unsigned short *chars;
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;

    COMPARE_GCS(pGC,pShadowGC);
    if (pDraw->type == DRAWABLE_WINDOW) {
	(void) apText (pDraw, pShadowGC, x, y, count, chars,
			(pShadowGC->font->pFI->lastRow == 0 ?
			 Linear16Bit : TwoD16Bit),
			pShadowGC->ImageGlyphBlt);
    } else {
	(* pShadowGC->ImageText16) (pDraw, pShadowGC, x, y, count, chars);
    }
}

/*-
 *-----------------------------------------------------------------------
 * apImageGlyphBlt --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    pointer 	pglyphBase;	/* start of array of glyphs */
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec  	  	cursorBox;
    ExtentInfoRec 	extents;
    register int  	xorg,
			yorg;

    if (pDrawable->type == DRAWABLE_WINDOW &&
	apCursorLoc (pDrawable->pScreen, &cursorBox)) {
	    QueryGlyphExtents (pGC->font, ppci, nglyph, &extents);
	    xorg = ((WindowPtr)pDrawable)->absCorner.x + x;
	    yorg = ((WindowPtr)pDrawable)->absCorner.y + y;
	    if (BOX_OVERLAP(&cursorBox,xorg+extents.overallLeft,
			    yorg+extents.overallAscent,
			    xorg+extents.overallRight,
			    yorg+extents.overallDescent)) {
				apRemoveCursor(TRUE);
	    }
    }

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->ImageGlyphBlt) (pDrawable, pShadowGC, x, y, nglyph,
				  ppci, pglyphBase);
}

/*-
 *-----------------------------------------------------------------------
 * apPolyGlyphBlt --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apPolyGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    char 	*pglyphBase;	/* start of array of glyphs */
{
    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec  	  	cursorBox;
    ExtentInfoRec 	extents;
    register int  	xorg,
			yorg;

    if (pDrawable->type == DRAWABLE_WINDOW &&
	apCursorLoc (pDrawable->pScreen, &cursorBox)) {
	    QueryGlyphExtents (pGC->font, ppci, nglyph, &extents);
	    xorg = ((WindowPtr)pDrawable)->absCorner.x + x;
	    yorg = ((WindowPtr)pDrawable)->absCorner.y + y;
	    if (BOX_OVERLAP(&cursorBox,xorg+extents.overallLeft,
			    yorg+extents.overallAscent,
			    xorg+extents.overallRight,
			    yorg+extents.overallDescent)){
				apRemoveCursor(TRUE);
	    }
    }

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->PolyGlyphBlt) (pDrawable, pShadowGC, x, y,
				nglyph, ppci, pglyphBase);
}

/*-
 *-----------------------------------------------------------------------
 * apPushPixels --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apPushPixels(pGC, pBitMap, pDst, w, h, x, y)
    GCPtr	pGC;
    PixmapPtr	pBitMap;
    DrawablePtr pDst;
    int		w, h, x, y;
{

    register GCPtr	pShadowGC = (GCPtr) pGC->devPriv;
    BoxRec  	  	cursorBox;
    register int  	xorg,
			yorg;

    if (pDst->type == DRAWABLE_WINDOW &&
	apCursorLoc (pDst->pScreen, &cursorBox)) {
	    xorg = ((WindowPtr)pDst)->absCorner.x + x;
	    yorg = ((WindowPtr)pDst)->absCorner.y + y;

	    if (BOX_OVERLAP(&cursorBox,xorg,yorg,xorg+w,yorg+h)){
		apRemoveCursor(TRUE);
	    }
    }

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->PushPixels) (pShadowGC, pBitMap, pDst, w, h, x, y);
}

/*-
 *-----------------------------------------------------------------------
 * apLineHelper --
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
void
apLineHelper (pDraw, pGC, caps, npt, pPts, xOrg, yOrg)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		caps;
    int		npt;
    SppPointPtr pPts;
    int		xOrg, yOrg;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->LineHelper) (pDraw, pShadowGC, caps, npt, pPts, xOrg, yOrg);
}

/*-
 *-----------------------------------------------------------------------
 * apChangeClip --
 *	Front end for changing the clip in the GC. Just passes the command
 *	on through the shadow GC.
 *
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	???
 *
 *-----------------------------------------------------------------------
 */
void
apChangeClip (pGC, type, pValue, numRects)
    GCPtr   	  pGC;
    int	    	  type;
    pointer 	  pValue;
    int	    	  numRects;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->ChangeClip) (pShadowGC, type, pValue, numRects);
}

/*-
 *-----------------------------------------------------------------------
 * apDestroyClip --
 *	Ditto for destroying the clipping region of the GC.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	???
 *
 *-----------------------------------------------------------------------
 */
void
apDestroyClip (pGC)
    GCPtr   pGC;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;

    COMPARE_GCS(pGC,pShadowGC);
    (* pShadowGC->DestroyClip) (pShadowGC);
}

/*-
 *-----------------------------------------------------------------------
 * apDestroyGC --
 *	Function called when a GC is being freed. Simply unlinks and frees
 *	the GCInterest structure.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The GCInterest structure is removed from the chain but its own
 *	links are untouched (so FreeGC has something to follow...)
 *
 *-----------------------------------------------------------------------
 */
void
apDestroyGC (pGC, pGCI)
    GCPtr	   pGC;	/* GC pGCI is attached to */
    GCInterestPtr  pGCI;	/* GCInterest being destroyed */
{
    if (pGC->devPriv)
	FreeGC ((GCPtr)pGC->devPriv);
    Xfree (pGCI);
}

/*-
 *-----------------------------------------------------------------------
 * apValidateGC --
 *	Called when a GC is about to be used for drawing. Copies all
 *	changes from the GC to its shadow and validates the shadow.
 *
 * Results:
 *	TRUE, for no readily apparent reason.
 *
 * Side Effects:
 *	Vectors in the shadow GC will likely be changed.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
apValidateGC (pGC, pGCI, changes, pDrawable)
    GCPtr	  pGC;
    GCInterestPtr pGCI;
    Mask	  changes;
    DrawablePtr	  pDrawable;
{
    register GCPtr pShadowGC = (GCPtr) pGC->devPriv;

    if ( pGC->depth != pDrawable->depth )
	FatalError( "apValidateGC: depth mismatch.\n" );

    CopyGC (pGC, pShadowGC, changes);
    pShadowGC->serialNumber = pGC->serialNumber;
    COMPARE_GCS(pGC,pShadowGC);
    ValidateGC (pDrawable, pShadowGC);
}
	
/*-
 *-----------------------------------------------------------------------
 * apCopyGC --
 *	Called when a GC with its shadow is the destination of a copy.
 *	Calls CopyGC to transfer the changes to the shadow GC as well.
 *	Should not be used for the CopyGCSource since we like to copy from
 *	the real GC to the shadow using CopyGC...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Any changes in the real GC are copied to the shadow.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
void
apCopyGC (pGCDst, pGCI, changes, pGCSrc)
    GCPtr   	  pGCDst;
    GCInterestPtr pGCI;
    int    	  changes;
    GCPtr   	  pGCSrc;
{
    CopyGC (pGCSrc, (GCPtr) pGCDst->devPriv, changes);
    COMPARE_GCS(pGCSrc,(GCPtr) pGCDst->devPriv);
}

/*
 * Array of functions to replace the functions in the GC.
 * Caveat: Depends on the ordering of functions in the GC structure.
 */
static void (* apGCFuncs[]) () = {
    apFillSpans,
    apSetSpans,

    apPutImage,
    apCopyArea,
    apCopyPlane,
    apPolyPoint,
    apPolylines,
    apPolySegment,
    apPolyRectangle,
    apPolyArc,
    apFillPolygon,
    apPolyFillRect,
    apPolyFillArc,
    (void(*)())apPolyText8,
    (void(*)())apPolyText16,
    apImageText8,
    apImageText16,
    apImageGlyphBlt,
    apPolyGlyphBlt,
    apPushPixels,
    apLineHelper,
    apChangeClip,
    apDestroyClip
};

/*-
 *-----------------------------------------------------------------------
 * apCreateGC --
 *	This function is used to get our own validation hooks into each
 *	GC to preserve the cursor. It calls the regular creation routine
 *	for the screen and then, if that was successful, tacks another
 *	GCInterest structure onto the GC *after* the one placed on by
 *	the screen-specific CreateGC...
 *
 * Results:
 *	TRUE if created ok. FALSE otherwise.
 *
 * Side Effects:
 *	A GCInterest structure is stuck on the end of the GC's list.
 *
 *-----------------------------------------------------------------------
 */
Bool
apCreateGC (pGC)
    GCPtr	pGC;	/* The GC to play with */
{
    GCInterestPtr	pGCI;
    register GCPtr	pShadowGC;
    int 		i;
    
    if ((*apProcs.CreateGC) (pGC)) {

	if (pGC->depth != pGC->pScreen->rootDepth) {
	    /* This GC will never be used for painting the screen,  so no shadow needed */
	    return TRUE;
	}

	pShadowGC = (GCPtr) Xalloc (sizeof (GC));
	if (pShadowGC == (GCPtr)NULL) {
	    return FALSE;
	}
	
	*pShadowGC = *pGC;
	pGC->devPriv = (pointer)pShadowGC;
	bcopy (apGCFuncs, &pGC->FillSpans, sizeof (apGCFuncs));
	
	pGCI = (GCInterestPtr) Xalloc (sizeof (GCInterestRec));
	if (!pGCI) {
	    return FALSE;
	}

	/*
	 * Any structure being shared between these two GCs must have its
	 * reference count incremented. This includes:
	 *  font, tile, stipple.
	 * Anything which doesn't have a reference count must be duplicated:
	 *  pCompositeClip, pAbsClientRegion.
	 * 
	 */
	if (pGC->font) {
	    pGC->font->refcnt++;
	}
	if (pGC->tile) {
	    pGC->tile->refcnt++;
	}
	if (pGC->stipple) {
	    pGC->stipple->refcnt++;
	}
	pShadowGC->dash = (unsigned char *)
		Xalloc(2 * sizeof(unsigned char));
	for (i=0; i<pGC->numInDashList; i++)
		pShadowGC->dash[i] = pGC->dash[i];

#ifdef	notdef
	if (pGC->pCompositeClip) {
	    pShadowGC->pCompositeClip =
		(* pGC->pScreen->RegionCreate) (NULL, 1);
	    (* pGC->pScreen->RegionCopy) (pShadowGC->pCompositeClip,
					  pGC->pCompositeClip);
	}
	if (pGC->pAbsClientRegion) {
	    pShadowGC->pAbsClientRegion=
		(* pGC->pScreen->RegionCreate) (NULL, 1);
	    (* pGC->pScreen->RegionCopy) (pShadowGC->pAbsClientRegion,
					  pGC->pAbsClientRegion);
	}
#endif	notdef
	
	pGC->pNextGCInterest = pGCI;
	pGC->pLastGCInterest = pGCI;
	pGCI->pNextGCInterest = (GCInterestPtr) &pGC->pNextGCInterest;
	pGCI->pLastGCInterest = (GCInterestPtr) &pGC->pNextGCInterest;
	pGCI->length = sizeof(GCInterestRec);
	pGCI->owner = 0;	    	    /* server owns this */
	pGCI->ValInterestMask = ~0; 	    /* interested in everything */
	pGCI->ValidateGC = apValidateGC;
	pGCI->ChangeInterestMask = 0; 	    /* interested in nothing */
	pGCI->ChangeGC = (int (*)()) NULL;
	pGCI->CopyGCSource = (void (*)())NULL;
	pGCI->CopyGCDest = apCopyGC;
	pGCI->DestroyGC = apDestroyGC;
	

	/*
	 * Because of this weird way of handling the GCInterest lists,
	 * we need to modify the output library's GCInterest structure to
	 * point to the pNextGCInterest field of the shadow GC...
	 */
	pGCI = pShadowGC->pNextGCInterest;
	pGCI->pLastGCInterest = pGCI->pNextGCInterest =
	    (GCInterestPtr) &pShadowGC->pNextGCInterest;

	return TRUE;
    } else {
	return FALSE;
    }
}

/*-
 * apPaintWindowBackground --
 *	Paint the window's background while preserving the cursor
 */
#define apPaintWindowBorder apPaintWindowBackground
void
apPaintWindowBackground (pWin, pRegion, what)
    WindowPtr	pWin;
    RegionPtr	pRegion;
    int		what;
{
    BoxRec	cursorBox;
    WinPrivPtr	pPriv;
    ScreenPtr	pScreen;

    pScreen = pWin->drawable.pScreen;

    if (apCursorLoc (pScreen, &cursorBox)) {
	/*
	 * If the cursor is on the same screen as the window, check the
	 * region to paint for the cursor and remove it as necessary
	 */
	if ((* pScreen->RectIn) (pRegion, &cursorBox) != rgnOUT) {
	    apRemoveCursor(TRUE);
	}
    }

    pPriv = (WinPrivPtr) LookupID (pWin->wid, RT_WINDOW, wPrivClass);
    if (what == PW_BACKGROUND) {
	(* pPriv->PaintWindowBackground) (pWin, pRegion, what);
    } else {
	(* pPriv->PaintWindowBorder) (pWin, pRegion, what);
    }
}

/*-
 *-----------------------------------------------------------------------
 * apCopyWindow --
 *	Protect the cursor from window copies..
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The cursor may be removed.
 *
 *-----------------------------------------------------------------------
 */
void
apCopyWindow (pWin, ptOldOrg, prgnSrc)
    WindowPtr	  pWin;
    DDXPointRec	  ptOldOrg;
    RegionPtr	  prgnSrc;
{
    BoxRec	cursorBox;
    WinPrivPtr	pPriv;
    ScreenPtr	pScreen;

    pScreen = pWin->drawable.pScreen;

    if (apCursorLoc (pScreen, &cursorBox)) {
	/*
	 * If the cursor is on the same screen, compare the box for the
	 * cursor against the original window clip region (prgnSrc) and
	 * the current window clip region (pWin->borderClip) and if it
	 * overlaps either one, remove the cursor. (Should it really be
	 * borderClip?)
	 */
	switch ((* pScreen->RectIn) (prgnSrc, &cursorBox)) {
	    case rgnOUT:
		if ((* pScreen->RectIn) (pWin->borderClip, &cursorBox) ==
		    rgnOUT) {
			break;
		}
	    case rgnIN:
	    case rgnPART:
		apRemoveCursor(TRUE);
	}
    }

    pPriv = (WinPrivPtr) LookupID (pWin->wid, RT_WINDOW, wPrivClass);
    (* pPriv->CopyWindow) (pWin, ptOldOrg, prgnSrc);
}

/*-
 * apCreateWindow --
 *	Allow the output library to do its thing and then make sure
 *	we intercept calls to PaintWindow{Border,Background} and
 *	ClearToBackground
 */
Bool
apCreateWindow(pWin)
    WindowPtr	pWin;
{
    WinPrivPtr	pPriv;

    (*apProcs.CreateWindow) (pWin);

    pPriv = (WinPrivPtr) Xalloc (sizeof (WinPrivRec));
    pPriv->PaintWindowBackground = pWin->PaintWindowBackground;
    pPriv->PaintWindowBorder = pWin->PaintWindowBorder;
    pPriv->CopyWindow = pWin->CopyWindow;

    AddResource (pWin->wid, RT_WINDOW, (pointer)pPriv, Xfree, 
		 wPrivClass);

    pWin->PaintWindowBackground = apPaintWindowBackground;
    pWin->PaintWindowBorder = apPaintWindowBorder;
    pWin->CopyWindow = apCopyWindow;

}

/*-
 * apChangeWindowAttributes --
 *	Catch the changing of the background/border functions
 */
Bool
apChangeWindowAttributes(pWin, mask)
    WindowPtr	pWin;
    Mask	mask;
{
    WinPrivPtr	pPriv;

    (*apProcs.ChangeWindowAttributes) (pWin, mask);

    pPriv = (WinPrivPtr) LookupID (pWin->wid, RT_WINDOW, wPrivClass);
    if (pPriv == (WinPrivPtr)0) {
	FatalError("apChangeWindowAttributes got null pPriv\n");
    }

    if ((char *)pWin->PaintWindowBackground !=(char *)apPaintWindowBackground){
	pPriv->PaintWindowBackground = pWin->PaintWindowBackground;
	pWin->PaintWindowBackground = apPaintWindowBackground;
    }
    if ((char *)pWin->PaintWindowBorder != (char *)apPaintWindowBorder) {
	pPriv->PaintWindowBorder = pWin->PaintWindowBorder;
	pWin->PaintWindowBorder = apPaintWindowBorder;
    }
    if ((char *)pWin->CopyWindow != (char *)apCopyWindow) {
	pPriv->CopyWindow = pWin->CopyWindow;
	pWin->CopyWindow = apCopyWindow;
    }

    return (TRUE);
}

