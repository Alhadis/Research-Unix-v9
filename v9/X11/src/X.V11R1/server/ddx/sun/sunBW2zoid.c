/* $Header: sunBW2zoid.c,v 4.1 87/09/10 18:25:11 sun Exp $ */
/*-
 * sunBW2zoids.c --
 *	Functions for handling the sun BWTWO board for the zoids extension.
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
#include    "resource.h"


/*
 * ZOIDS should only ever be defined if SUN_WINDOWS is also defined.
 */
#ifdef ZOIDS
/*
 *  For tiled & stippled trapezoids,
 *  make a mem_point(), pointing at the tile or stipple
 *  image.  Tile the scratch_pr with the mpr, the size of pDraw.
 *  Use the scratch_pr as the source
 *  in calls to pr_polygon_2().  Tiles use PIX_SRC; stipples use
 *  PIX_SRC|PIX_DST.
 */

#define X_ALIGN	0
#define Y_ALIGN	1

static Pixrect *
getMpr(pPixmap)
    PixmapPtr	pPixmap;
{
    register Pixrect	*pMpr;
    
    pMpr = mem_point(pPixmap->width, pPixmap->height,
		     1, pPixmap->devPrivate);
    return pMpr;
}

static Pixrect *
getSubregion(pDraw, pScratch)
    DrawablePtr  pDraw;
    Pixrect	*pScratch;
{
    register Pixrect	*pSubpr;
    register int	 w, h;

    if (pDraw->type == DRAWABLE_WINDOW) {
	w = ((WindowPtr) pDraw)->clientWinSize.width;
	h = ((WindowPtr) pDraw)->clientWinSize.height;
    } else {
	w = ((PixmapPtr) pDraw)->width;
	h = ((PixmapPtr) pDraw)->height;
    }
    pSubpr = pr_region(pScratch, 0, 0, w, h);
    return (pSubpr);
}

static void
tileRegion(pScratch, pTile)
    Pixrect	*pScratch, *pTile;
{
    pr_replrop(pScratch, 0, 0, pScratch->pr_size.x, pScratch->pr_size.y,
	       PIX_SRC, pTile, 0, 0);
}

static int
orderXzoid(pZoid, pPtList)
    xXTraps		*pZoid;
    struct pr_pos	*pPtList;
{
    int			 npts = 2;
    
    if (pZoid->x2 == pZoid->x1)
	return (0);
    pPtList[0].x = pZoid->x1;
    pPtList[0].y = pZoid->y1;
    pPtList[1].x = pZoid->x2;
    pPtList[1].y = pZoid->y3;
    if (pZoid->y4 != pPtList[1].y) {
	pPtList[2].x = pZoid->x2;
	pPtList[2].y = pZoid->y4;
	npts++;
    }
    if (pZoid->y2 != pPtList[0].y) {
	pPtList[npts].x = pZoid->x1;
	pPtList[npts].y = pZoid->y2;
	npts++;
    }
    if (npts == 2)
	npts = 0;

    return (npts);
}

static int
orderYzoid(pZoid, pPtList)
    xYTraps		*pZoid;
    struct pr_pos	*pPtList;
{
    int			 npts = 2;
    
    if (pZoid->y2 == pZoid->y1)
	return (0);
    pPtList[0].x = pZoid->x1;
    pPtList[0].y = pZoid->y1;
    pPtList[1].x = pZoid->x3;
    pPtList[1].y = pZoid->y2;
    if (pZoid->x4 != pPtList[1].x) {
	pPtList[2].x = pZoid->x4;
	pPtList[2].y = pZoid->y2;
	npts++;
    }
    if (pZoid->x2 != pPtList[0].x) {
	pPtList[npts].x = pZoid->x2;
	pPtList[npts].y = pZoid->y1;
	npts++;
    }
    if (npts == 2)
	npts = 0;

    return (npts);
}

static void
solidXzoid(pPixscreen, pZoid, pPixsrc, sx, sy)
    Pixrect	*pPixscreen,
    		*pPixsrc;	/* unused */
    xXTraps	*pZoid;
    int		 sx, sy;	/* unused */
{
    int			 npts;
    struct pr_pos	 ptlist[4];
    
    if ((npts = orderXzoid(pZoid, ptlist)) > 0)
	pr_polygon_2(pPixscreen, 0, 0, 1, &npts, ptlist,
		     PIX_SET, NULL, 0, 0);
}

static void
solidYzoid(pPixscreen, pZoid, pPixsrc, sx, sy)
    Pixrect	*pPixscreen,
    		*pPixsrc;	/* unused */
    xYTraps	*pZoid;
    int		 sx, sy;	/* unused */
{
    int			 npts;
    struct pr_pos	 ptlist[4];
    
    if ((npts = orderYzoid(pZoid, ptlist)) > 0)
	pr_polygon_2(pPixscreen, 0, 0, 1, &npts, ptlist,
		     PIX_SET, NULL, 0, 0);
}

static void
tiledXzoid(pPixscreen, pZoid, pPixsrc, sx, sy)
    Pixrect	*pPixscreen,
    		*pPixsrc;
    xXTraps	*pZoid;
    int		 sx, sy;
{
    int			 npts;
    struct pr_pos	 ptlist[4];
    
    if ((npts = orderXzoid(pZoid, ptlist)) > 0)
	pr_polygon_2(pPixscreen, 0, 0, 1, &npts, ptlist,
		     PIX_SRC, pPixsrc, sx, sy);
}

static void
tiledYzoid(pPixscreen, pZoid, pPixsrc, sx, sy)
    Pixrect	*pPixscreen,
    		*pPixsrc;
    xYTraps	*pZoid;
    int		 sx, sy;
{
    int			 npts;
    struct pr_pos	 ptlist[4];
    
    if ((npts = orderYzoid(pZoid, ptlist)) > 0)
	pr_polygon_2(pPixscreen, 0, 0, 1, &npts, ptlist,
		     PIX_SRC, pPixsrc, sx, sy);
}

static void
stippledXzoid(pPixscreen, pZoid, pPixsrc, sx, sy)
    Pixrect	*pPixscreen,
    		*pPixsrc;
    xXTraps	*pZoid;
    int		 sx, sy;
{
    int			 npts;
    struct pr_pos	 ptlist[4];
    
    if ((npts = orderXzoid(pZoid, ptlist)) > 0)
	pr_polygon_2(pPixscreen, 0, 0, 1, &npts, ptlist,
		     PIX_SRC|PIX_DST, pPixsrc, sx, sy);
}

static void
stippledYzoid(pPixscreen, pZoid, pPixsrc, sx, sy)
    Pixrect	*pPixscreen,
    		*pPixsrc;
    xYTraps	*pZoid;
    int		 sx, sy;
{
    int			 npts;
    struct pr_pos	 ptlist[4];
    
    if ((npts = orderYzoid(pZoid, ptlist)) > 0)
	pr_polygon_2(pPixscreen, 0, 0, 1, &npts, ptlist,
		     PIX_SRC|PIX_DST, pPixsrc, sx, sy);
}

static void
XzoidBbox(pzoid, pbox)
    xXTraps *pzoid;
    BoxRec *pbox;
{
    pbox->x1 = min (pzoid->x1, pzoid->x2);
    pbox->y1 = min (min (pzoid->y1, pzoid->y2), min (pzoid->y3, pzoid->y4));
    pbox->x2 = max (pzoid->x1, pzoid->x2);
    pbox->y2 = max (max (pzoid->y1, pzoid->y2), max (pzoid->y3, pzoid->y4));
}

static void
YzoidBbox(pzoid, pbox)
    xYTraps *pzoid;
    BoxRec *pbox;
{
    pbox->x1 = min (min (pzoid->x1, pzoid->x2), min (pzoid->x3, pzoid->x4));
    pbox->y1 = min (pzoid->y1, pzoid->y2);
    pbox->x2 = max (max (pzoid->x1, pzoid->x2), max (pzoid->x3, pzoid->x4));
    pbox->y2 = max (pzoid->y1, pzoid->y2);
}

/*
 *  X aligned
 *
 *              [x2,y3]
 *                 +
 *              A /|
 *       [x1,y1] / |
 *              +  |
 *              |  |
 *              +  |
 *       [x1,y2] \ |
 *              B \|
 *                 +
 *              [x2,y4]
 *
 *  A:      y1-y3           y1-y3
 *      y = ----- x + (y1 - ----- x1)
 *          x1-x2           x1-x2
 *
 *  B:      y2-y4           y2-y4
 *      y = ----- x + (y2 - ----- x1)
 *          x1-x2           x1-x2
 */
#define Ax2y(_x) (int) (((((long)pz->y1 - (long)pz->y3) * (long)(_x))/ \
			  ((long)pz->x1 - (long)pz->x2)) + (long)pz->y1 - \
			((((long)pz->y1 - (long)pz->y3)*(long)pz->x1)/ \
			  ((long)pz->x1 - (long)pz->x2)))
#define Bx2y(_x) (int) (((((long)pz->y2 - (long)pz->y4) * (long)(_x))/ \
			  ((long)pz->x1 - (long)pz->x2)) + (long)pz->y2 - \
			((((long)pz->y2 - (long)pz->y4)*(long)pz->x1)/ \
			  ((long)pz->x1 - (long)pz->x2)))
#define Ay2x(_y) (int) (((((long)pz->x1 - (long)pz->x2) * (long)(_y))/ \
			  ((long)pz->y1 - (long)pz->y3)) + (long)pz->x1 - \
			((((long)pz->x1 - (long)pz->x2)*(long)pz->y1)/ \
			  ((long)pz->y1 - (long)pz->y3)))
#define By2x(_y) (int) (((((long)pz->x1 - (long)pz->x2) * (long)(_y))/ \
			  ((long)pz->y2 - (long)pz->y4)) + (long)pz->x1 - \
			((((long)pz->x1 - (long)pz->x2)*(long)pz->y2)/ \
			  ((long)pz->y2 - (long)pz->y4)))
static int
intersectXzoid(pbox, pzoid, subzoid)
    BoxRec *pbox;
    xXTraps *pzoid;
    xXTraps *subzoid;
{
    int nzoids = 1;
    register int tx, ty, i;
    xXTraps *pz;
    
    pz = subzoid;
    *pz = *pzoid;
    if (pzoid->x1 < pbox->x1) {
	pz->y1 = Ax2y (pbox->x1);
	pz->y2 = Bx2y (pbox->x1);
	pz->x1 = pbox->x1;
    }
    if (pzoid->x2 > pbox->x2) {
	pz->y3 = Ax2y (pbox->x2);
	pz->y4 = Bx2y (pbox->x2);
	pz->x2 = pbox->x2;
    }
    /* did we just clip out the zoid? */
    if (pz->y1 > pbox->y2 && pz->y3 > pbox->y2)
        return (0);
    if (pz->y2 < pbox->y1 && pz->y4 < pbox->y1)
        return (0);
    /* does zoid side A cross box side A? */
    if ((pz->y1 < pbox->y1 && pz->y3 > pbox->y1)
    ||  (pz->y1 > pbox->y1 && pz->y3 < pbox->y1)) {
	tx = Ay2x(pbox->y1);	/* side A crosses at [tx,pbox->y1] */
	ty = Bx2y(tx);
	*(pz+1) = *pz;
	nzoids++;
	/* does either subside of side B cross the box? */
	if ((pz->y2 < pbox->y2 && ty > pbox->y2)
	||  (pz->y2 > pbox->y2 && ty < pbox->y2)) {
	    /* left subside crosses; keep right side in pz */
	    pz->x1 = tx;
	    pz->y1 = pbox->y1;
	    pz->y2 = ty;
	    pz++;
	    pz->x2 = tx;
	    pz->y3 = pbox->y1;
	    pz->y4 = ty;
	} else {
	    /* keep left side in pz */
	    pz->x2 = tx;
	    pz->y3 = pbox->y1;
	    pz->y4 = ty;
	    pz++;
	    pz->x1 = tx;
	    pz->y1 = pbox->y1;
	    pz->y2 = ty;
	}
    } else
    /* does zoid side A cross box side B? */
    if ((pz->y1 < pbox->y2 && pz->y3 > pbox->y2)
    ||  (pz->y1 > pbox->y2 && pz->y3 < pbox->y2)) {
	tx = Ay2x(pbox->y2);	/* side A crosses at [tx,pbox->y2] */
	ty = Bx2y(tx);
	/* pbox contains only a triangle. */
	if (pz->y1 < pbox->y2) { /* left  part of side A is inside */
	    pz->x2 = tx;
	    pz->y3 = pbox->y2;
	    pz->y4 = ty;
	} else {		 /* right part of side A is inside */
	    pz->x1 = tx;
	    pz->y1 = pbox->y2;
	    pz->y2 = ty;
	}
    }
    /* does zoid side B cross box side B? */
    if ((pz->y2 < pbox->y2 && pz->y4 > pbox->y2)
    ||  (pz->y2 > pbox->y2 && pz->y4 < pbox->y2)) {
	tx = By2x(pbox->y2);	/* side B crosses at [tx,pbox->y2] */
	ty = Ax2y(tx);
	*(pz+1) = *pz;
	nzoids++;
	/* side A doesn't cross; keep left side in pz */
	pz->x2 = tx;
	pz->y3 = ty;
	pz->y4 = pbox->y2;
	pz++;
	pz->x1 = tx;
	pz->y1 = ty;
	pz->y2 = pbox->y2;
    } else
    /* does zoid side B cross box side A? */
    if ((pz->y2 < pbox->y1 && pz->y4 > pbox->y1)
    ||  (pz->y2 > pbox->y1 && pz->y4 < pbox->y1)) {
	tx = By2x(pbox->y1);	/* side B crosses at [tx,pbox->y1] */
	ty = Ax2y(tx);
	/* pbox contains only a triangle. */
	if (pz->y2 > pbox->y1) { /* left  part of side B is inside */
	    pz->x2 = tx;
	    pz->y3 = ty;
	    pz->y4 = pbox->y1;
	} else {		 /* right part of side B is inside */
	    pz->x1 = tx;
	    pz->y1 = ty;
	    pz->y2 = pbox->y1;
	}
    }
    /* finish the clipping, now that nothing crosses */
    for (i = 0, pz = subzoid; i < nzoids; i++, pz++) {
	if (pz->y1 < pbox->y1 || pz->y3 < pbox->y1) {
	    pz->y1 = pz->y3 = pbox->y1;
	}
	if (pz->y2 > pbox->y2 || pz->y4 > pbox->y2) {
	    pz->y2 = pz->y4 = pbox->y2;
	}
    }

    return (nzoids);
}

/*
 *  Y aligned
 *
 *         [x1,y1]     [x2,y1]
 *            +-----------+
 *        A  /             \  B
 *          +---------------+
 *       [x3,y2]         [x4,y2]
 *
 *  A:      y1-y2           y1-y2
 *      y = ----- x + (y1 - ----- x1)
 *          x1-x3           x1-x3
 *
 *  B:      y1-y2           y1-y2
 *      y = ----- x + (y1 - ----- x2)
 *          x2-x4           x2-x4
 */
#undef Ax2y
#undef Bx2y
#undef Ay2x
#undef By2x
#define Ax2y(_x) (int) (((((long)pz->y1 - (long)pz->y2) * (long)(_x))/ \
			  ((long)pz->x1 - (long)pz->x3)) + (long)pz->y1 - \
			((((long)pz->y1 - (long)pz->y2)*(long)pz->x1)/ \
			  ((long)pz->x1 - (long)pz->x3)))
#define Bx2y(_x) (int) (((((long)pz->y1 - (long)pz->y2) * (long)(_x))/ \
			  ((long)pz->x2 - (long)pz->x4)) + (long)pz->y1 - \
			((((long)pz->y1 - (long)pz->y2)*(long)pz->x2)/ \
			  ((long)pz->x2 - (long)pz->x4)))
#define Ay2x(_y) (int) (((((long)pz->x1 - (long)pz->x3) * (long)(_y))/ \
			  ((long)pz->y1 - (long)pz->y2)) + (long)pz->x1 - \
			((((long)pz->x1 - (long)pz->x3)*(long)pz->y1)/ \
			  ((long)pz->y1 - (long)pz->y2)))
#define By2x(_y) (int) (((((long)pz->x2 - (long)pz->x4) * (long)(_y))/ \
			  ((long)pz->y1 - (long)pz->y2)) + (long)pz->x2 - \
			((((long)pz->x2 - (long)pz->x4)*(long)pz->y1)/ \
			  ((long)pz->y1 - (long)pz->y2)))
static int
intersectYzoid(pbox, pzoid, subzoid)
    BoxRec *pbox;
    xYTraps *pzoid;
    xYTraps *subzoid;
{
    int nzoids = 1;
    register int tx, ty, i;
    xYTraps *pz;
    
    pz = subzoid;
    *pz = *pzoid;
    if (pzoid->y1 < pbox->y1) {
	pz->x1 = Ay2x (pbox->y1);
	pz->x2 = By2x (pbox->y1);
	pz->y1 = pbox->y1;
    }
    if (pzoid->y2 > pbox->y2) {
	pz->x3 = Ay2x (pbox->y2);
	pz->x4 = By2x (pbox->y2);
	pz->y2 = pbox->y2;
    }
    /* did we just clip out the zoid? */
    if (pz->x1 > pbox->x2 && pz->x3 > pbox->x2)
        return (0);
    if (pz->x2 < pbox->x1 && pz->x4 < pbox->x1)
        return (0);
    /* does zoid side A cross box side A? */
    if ((pz->x1 < pbox->x1 && pz->x3 > pbox->x1)
    ||  (pz->x1 > pbox->x1 && pz->x3 < pbox->x1)) {
	ty = Ax2y(pbox->x1);	/* side A crosses at [pbox->x1,ty] */
	tx = By2x(ty);
	*(pz+1) = *pz;
	nzoids++;
	/* does either subside of side B cross the box? */
	if ((pz->x2 < pbox->x2 && tx > pbox->x2)
	||  (pz->x2 > pbox->x2 && tx < pbox->x2)) {
	    /* top subside crosses; keep bottom side in pz */
	    pz->y1 = ty;
	    pz->x1 = pbox->x1;
	    pz->x2 = tx;
	    pz++;
	    pz->y2 = ty;
	    pz->x3 = pbox->x1;
	    pz->x4 = tx;
	} else {
	    /* keep top side in pz */
	    pz->y2 = ty;
	    pz->x3 = pbox->x1;
	    pz->x4 = tx;
	    pz++;
	    pz->y1 = ty;
	    pz->x1 = pbox->x1;
	    pz->x2 = tx;
	}
    } else
    /* does zoid side A cross box side B? */
    if ((pz->x1 < pbox->x2 && pz->x3 > pbox->x2)
    ||  (pz->x1 > pbox->x2 && pz->x3 < pbox->x2)) {
	ty = Ax2y(pbox->x2);	/* side A crosses at [pbox->x2,ty] */
	tx = By2x(ty);
	/* pbox contains only a triangle. */
	if (pz->x1 < pbox->x2) { /* top    part of side A is inside */
	    pz->y2 = ty;
	    pz->x3 = pbox->x2;
	    pz->x4 = tx;
	} else {		 /* bottom part of side A is inside */
	    pz->y1 = ty;
	    pz->x1 = pbox->x2;
	    pz->x2 = tx;
	}
    }
    /* does zoid side B cross box side B? */
    if ((pz->x2 < pbox->x2 && pz->x4 > pbox->x2)
    ||  (pz->x2 > pbox->x2 && pz->x4 < pbox->x2)) {
	ty = Bx2y(pbox->x2);	/* side B crosses at [pbox->x2,ty] */
	tx = Ay2x(ty);
	*(pz+1) = *pz;
	nzoids++;
	/* side A doesn't cross; keep top side in pz */
	pz->y2 = ty;
	pz->x3 = tx;
	pz->x4 = pbox->x2;
	pz++;
	pz->y1 = ty;
	pz->x1 = tx;
	pz->x2 = pbox->x2;
    } else
    /* does zoid side B cross box side A? */
    if ((pz->x2 < pbox->x1 && pz->x4 > pbox->x1)
    ||  (pz->x2 > pbox->x1 && pz->x4 < pbox->x1)) {
	ty = Bx2y(pbox->x1);	/* side B crosses at [pbox->x1,ty] */
	tx = Ay2x(ty);
	/* pbox contains only a triangle. */
	if (pz->x2 > pbox->x1) { /* top    part of side B is inside */
	    pz->y2 = ty;
	    pz->x3 = tx;
	    pz->x4 = pbox->x1;
	} else {		 /* bottom part of side B is inside */
	    pz->y1 = ty;
	    pz->x1 = tx;
	    pz->x2 = pbox->x1;
	}
    }
    /* finish the clipping, now that nothing crosses */
    for (i = 0, pz = subzoid; i < nzoids; i++, pz++) {
	if (pz->x1 < pbox->x1 || pz->x3 < pbox->x1) {
	    pz->x1 = pz->x3 = pbox->x1;
	}
	if (pz->x2 > pbox->x2 || pz->x4 > pbox->x2) {
	    pz->x2 = pz->x4 = pbox->x2;
	}
    }

    return (nzoids);
}

static void
fillZoids(pDrawable, pGC, ntraps, traplist, alignment, pPaint)
    DrawablePtr pDrawable;
    GCPtr pGC;
    int ntraps;
    xXYTraps *traplist;
    int alignment;
    void (* pPaint) ();
{
    /* rip off from mfbPolyFillRect() */
    register BoxPtr pbox;	/* pointer to current box in clip region */
    int i;			/* loop index */

    register xXYTraps *pzoid;	/* pointer to current zoid to be filled */
    int xorg, yorg;		/* origin of window */
    int	nrectClip;		/* number of rectangles in clip region */
    short alu;			/* gc filling function */
    Pixrect *pTile;		/* temporary for replropping scratch_pr */
    Pixrect *pScratch;		/* temporary for replropping scratch_pr */
    mfbPrivGC *privGC;		/* for reducing pointer dereferencing */


    if (!(pGC->planemask & 1))
	return;

    privGC = (mfbPrivGC *)((GCPtr)pGC->devPriv)->devPriv;
    nrectClip = privGC->pCompositeClip->numRects;

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	xorg = ((WindowPtr)pDrawable)->absCorner.x;
	yorg = ((WindowPtr)pDrawable)->absCorner.y;
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate);
	/* translate the rectangle list */
	for (i = 0, pzoid = traplist; i < ntraps; i++, pzoid++)
	{
	    if (alignment == X_ALIGN) {
		pzoid->Xt.x1 += xorg;
		pzoid->Xt.x2 += xorg;
		pzoid->Xt.y1 += yorg;
		pzoid->Xt.y2 += yorg;
		pzoid->Xt.y3 += yorg;
		pzoid->Xt.y4 += yorg;
	    } else {
		pzoid->Yt.y1 += yorg;
		pzoid->Yt.y2 += yorg;
		pzoid->Yt.x1 += xorg;
		pzoid->Yt.x2 += xorg;
		pzoid->Yt.x3 += xorg;
		pzoid->Yt.x4 += xorg;
	    }
	}
    }
    else
    {
	xorg = 0;
	yorg = 0;
    }


    if((alu = pGC->alu) == GXnoop)
	return;			/* hey, this is easy! */

    switch (pGC->fillStyle)
    {
      case FillSolid:
        alu = privGC->rop; /* used reduced ROP */
        pScratch = (Pixrect *)NULL;
	break;

      case FillStippled:
        alu = privGC->rop; /* used reduced ROP */
	pTile = getMpr(privGC->pRotatedStipple);
	pScratch = getSubregion(pDrawable,
			sunFbData[pDrawable->pScreen->myNum].scratch_pr);
	tileRegion(pScratch, pTile);
	pr_destroy(pTile);
	break;

      case FillTiled:
	pTile = getMpr(privGC->pRotatedTile);
	pScratch = getSubregion(pDrawable,
			sunFbData[pDrawable->pScreen->myNum].scratch_pr);
	tileRegion(pScratch, pTile);
	pr_destroy(pTile);
	break;
    }

    for (pzoid = traplist, --ntraps; ntraps >= 0; ++pzoid, --ntraps) {
	BoxRec	boxS, boxD;

	if (alignment == X_ALIGN)
	    XzoidBbox(pzoid, &boxS);
	else
	    YzoidBbox(pzoid, &boxS);

	/* if lower right is out, the whole box is */
	if ((boxS.x2 <= 0) || (boxS.y2 <= 0))
	    continue;

	/* clip left and top to drawable's origin */
	if (boxS.x1 < 0)
	    boxS.x1 = 0;

	if (boxS.y1 < 0)
	    boxS.y1 = 0;

	/* If the box is completely out of this clip rectangle, ignore it.
	 * if it is completely within, draw it,
	 * otherwise, draw the part that is within this region 
	 */
	switch ((*pGC->pScreen->RectIn)
		      (privGC->pCompositeClip, &boxS))
	{
	  case rgnOUT:
		break;
	  case rgnIN:
	    {
		/* Draw entire zoid */
		(*pPaint)(sunFbData[pDrawable->pScreen->myNum].pr,
			  pzoid, pScratch, -xorg, -yorg);
	        break;
	    }
	  case rgnPART:
	    {
		pbox = privGC->pCompositeClip->rects;
		for (i = nrectClip; i > 0; --i, ++pbox) {
		    /* Clip box to each rect in turn 
		       and draw the clipped box */

		    boxD.x1 = max(boxS.x1, pbox->x1);
		    boxD.y1 = max(boxS.y1, pbox->y1);
		    boxD.x2 = min(boxS.x2, pbox->x2);
		    boxD.y2 = min(boxS.y2, pbox->y2);

		    if(boxD.x1 < boxD.x2 && boxD.y1 < boxD.y2) {
			/* Construct subzoid and draw it.
			 * Note: sometimes 3 subzoids are needed.
			 */
			xXYTraps	subzoid[3], *pz;
			int		nzoids;
			
			if (alignment == X_ALIGN) {
			    nzoids = intersectXzoid(&boxD, pzoid, subzoid);
			} else {
			    nzoids = intersectYzoid(&boxD, pzoid, subzoid);
			}
			for (pz=subzoid; nzoids > 0; pz++, --nzoids) {
			    (*pPaint)(
			      sunFbData[pDrawable->pScreen->myNum].pr,
			      pz, pScratch, -xorg, -yorg);
			}
		    }
		}
		break;
	    }
	}   /* switch RectIn */
    }   /* for each pzoid */
    if (pScratch)
	pr_destroy(pScratch);
}

void
sunBW2SolidXZoids(pDraw, pGC, ntraps, traplist)
    DrawablePtr pDraw;
    GCPtr pGC;
    int ntraps;
    xXYTraps *traplist;
{
    fillZoids(pDraw, pGC, ntraps, traplist, X_ALIGN, solidXzoid);
}

void
sunBW2SolidYZoids(pDraw, pGC, ntraps, traplist)
    DrawablePtr pDraw;
    GCPtr pGC;
    int ntraps;
    xXYTraps *traplist;
{
    fillZoids(pDraw, pGC, ntraps, traplist, Y_ALIGN, solidYzoid);
}

void
sunBW2TiledXZoids(pDraw, pGC, ntraps, traplist)
    DrawablePtr pDraw;
    GCPtr pGC;
    int ntraps;
    xXYTraps *traplist;
{
    fillZoids(pDraw, pGC, ntraps, traplist, X_ALIGN, tiledXzoid);
}

void
sunBW2TiledYZoids(pDraw, pGC, ntraps, traplist)
    DrawablePtr pDraw;
    GCPtr pGC;
    int ntraps;
    xXYTraps *traplist;
{
    fillZoids(pDraw, pGC, ntraps, traplist, Y_ALIGN, tiledYzoid);
}

void
sunBW2StipXZoids(pDraw, pGC, ntraps, traplist)
    DrawablePtr pDraw;
    GCPtr pGC;
    int ntraps;
    xXYTraps *traplist;
{
    fillZoids(pDraw, pGC, ntraps, traplist, X_ALIGN, stippledXzoid);
}

void
sunBW2StipYZoids(pDraw, pGC, ntraps, traplist)
    DrawablePtr pDraw;
    GCPtr pGC;
    int ntraps;
    xXYTraps *traplist;
{
    fillZoids(pDraw, pGC, ntraps, traplist, Y_ALIGN, stippledYzoid);
}
#endif ZOIDS
