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

/* $Header: miexpose.c,v 1.25 87/09/11 07:19:01 toddb Exp $ */

#include "X.h"
#define NEED_EVENTS
#include "Xproto.h"
#include "Xprotostr.h"

#include "misc.h"
#include "regionstr.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmap.h"

#include "dixstruct.h"
#include "mi.h"
#include "Xmd.h"

extern WindowRec WindowTable[];

/*
    machine-independent graphics exposure code.  any device that uses
the region package can call this.
*/


/* miHandleExposures 
    generate exposures for areas that were copied from obscured or
non-existent areas to non-obscured areas of the destination.

NOTE:
    this code sends the exposure events, then paints the background.
this will NOT work unless this routine is allowed to complete before
the dispatcher looks at this client again.

NOTE:
    this does not deal with backing store.
*/

void
miHandleExposures(pSrcDrawable, pDstDrawable,
		  pGC, srcx, srcy, width, height, dstx, dsty)
    register DrawablePtr	pSrcDrawable;
    register DrawablePtr	pDstDrawable;
    GCPtr 			pGC;
    int 			srcx, srcy;
    int 			width, height;
    int 			dstx, dsty;
{
    register ScreenPtr pscr = pGC->pScreen;
    RegionPtr prgnSrcClip;	/* drawable-relative source clip */
    RegionPtr prgnDstClip;	/* drawable-relative dest clip */
    BoxRec srcBox;		/* unclipped source */
    RegionPtr prgnSrc;		/* clipped source */
    RegionPtr prgnExposed;	/* exposed region, calculated source-
				   relative, made dst relative to
				   intersect with visible parts of
				   dest and send events to client, 
				   and then screen relative to paint 
				   the window background
				*/

    if (pSrcDrawable->type == DRAWABLE_WINDOW)
    {
	prgnSrcClip = (*pscr->RegionCreate)(NullBox, 1);
	(*pscr->RegionCopy)(prgnSrcClip,
			    ((WindowPtr)pSrcDrawable)->clipList);
	(*pscr->TranslateRegion)(prgnSrcClip,
				 -((WindowPtr)pSrcDrawable)->absCorner.x,
				 -((WindowPtr)pSrcDrawable)->absCorner.y);
    }
    else
    {
	BoxRec	box;

	box.x1 = 0;
	box.y1 = 0;
	box.x2 = ((PixmapPtr)pSrcDrawable)->width;
	box.y2 = ((PixmapPtr)pSrcDrawable)->height;
	prgnSrcClip = (*pscr->RegionCreate)(&box, 1);
    }

    if (pDstDrawable->type == DRAWABLE_WINDOW)
    {
	prgnDstClip = (*pscr->RegionCreate)(NullBox, 1);
	(*pscr->RegionCopy)(prgnDstClip,
			    ((WindowPtr)pDstDrawable)->clipList);
	(*pscr->TranslateRegion)(prgnDstClip,
				 -((WindowPtr)pDstDrawable)->absCorner.x,
				 -((WindowPtr)pDstDrawable)->absCorner.y);
    }
    else
    {
	BoxRec	box;

	box.x1 = 0;
	box.y1 = 0;
	box.x2 = ((PixmapPtr)pDstDrawable)->width;
	box.y2 = ((PixmapPtr)pDstDrawable)->height;
	prgnDstClip = (*pscr->RegionCreate)(&box, 1);
    }

    /* drawable-relative source region */
    srcBox.x1 = srcx;
    srcBox.y1 = srcy;
    srcBox.x2 = srcx+width;
    srcBox.y2 = srcy+height;
    prgnSrc = (*pscr->RegionCreate)(&srcBox, 1);

    /* get the visible parts of the source box */
    (*pscr->Intersect)(prgnSrc, prgnSrc, prgnSrcClip);

    /* now get the hidden parts */
    prgnExposed = (*pscr->RegionCreate)(NullBox, 1);
    (*pscr->Inverse)(prgnExposed, prgnSrc, &srcBox);

    /* move them over the destination */
    (*pscr->TranslateRegion)(prgnExposed, dstx-srcx, dsty-srcy);

    /* intersect with visible areas of dest */
    (*pscr->Intersect)(prgnExposed, prgnExposed, prgnDstClip);

    /* send them to the client */
    if (pGC->graphicsExposures)
    {
        if (REGION_NOT_EMPTY(prgnExposed))
        {
            xEvent *pEvent;
    	    register xEvent *pe;
    	    register BoxPtr pBox = prgnExposed->rects;
    	    register int i;

	    if(!(pEvent = (xEvent *)ALLOCATE_LOCAL(prgnExposed->numRects * 
					 sizeof(xEvent))))
		return;
	    pe = pEvent;

	    for (i=1; i<=prgnExposed->numRects; i++, pe++, pBox++)
	    {
	        pe->u.u.type = GraphicsExpose;
	        pe->u.graphicsExposure.drawable = 
				requestingClient->lastDrawableID;
	        pe->u.graphicsExposure.x = pBox->x1;
	        pe->u.graphicsExposure.y = pBox->y1;
	        pe->u.graphicsExposure.width = pBox->x2 - pBox->x1;
	        pe->u.graphicsExposure.height = pBox->y2 - pBox->y1;
	        pe->u.graphicsExposure.count = prgnExposed->numRects - i;
					
	    }
	    TryClientEvents(requestingClient, pEvent, prgnExposed->numRects,
			    0, NoEventMask, 0);
	    DEALLOCATE_LOCAL(pEvent);
        }
        else
        {
            xEvent event;
	    event.u.u.type = NoExpose;
	    event.u.noExposure.drawable = requestingClient->lastDrawableID;
	    TryClientEvents(requestingClient, &event, 1,
	        0, NoEventMask, 0);
        }
    }


    if ((pDstDrawable->type == DRAWABLE_WINDOW) &&
	(((WindowPtr)pDstDrawable)->backgroundTile != None))
    {
	WindowPtr pWin = (WindowPtr)pDstDrawable;

	/* make the exposed area screen-relative */
	(*pscr->TranslateRegion)(prgnExposed, 
				 pWin->absCorner.x, pWin->absCorner.y);

	(*pWin->PaintWindowBackground)(pDstDrawable, prgnExposed, 
				       PW_BACKGROUND);
    }
    (*pscr->RegionDestroy)(prgnDstClip);
    (*pscr->RegionDestroy)(prgnSrcClip);
    (*pscr->RegionDestroy)(prgnSrc);
    (*pscr->RegionDestroy)(prgnExposed);
}

void
miSendNoExpose(pGC)
    GCPtr pGC;
{
    if (pGC->graphicsExposures)
    {
        xEvent event;
	event.u.u.type = NoExpose;
	event.u.noExposure.drawable = 
		    requestingClient->lastDrawableID;
        TryClientEvents(requestingClient, &event, 1,
	        0, NoEventMask, 0);
    }
}


void 
miWindowExposures(pWin)
    WindowPtr pWin;
{
    register RegionPtr prgn;

    prgn = pWin->exposed;
    if (prgn->numRects)
    {
	xEvent *pEvent;
	register xEvent *pe;
	register BoxPtr pBox;
	register int i;

        (*pWin->PaintWindowBackground)(pWin, prgn, PW_BACKGROUND);
	(* pWin->drawable.pScreen->TranslateRegion)(prgn,
			-pWin->absCorner.x, -pWin->absCorner.y);
 	if (pWin->backingStore != NotUseful && pWin->backStorage)
	{
		/* modifies pWin->exposed */
	    (*pWin->backStorage->RestoreAreas)(pWin);
	}
	pBox = prgn->rects;
        
	if(!(pEvent = (xEvent *)
	    ALLOCATE_LOCAL(prgn->numRects * sizeof(xEvent))))
	    return;
	pe = pEvent;
        
	for (i=1; i<=prgn->numRects; i++, pe++, pBox++)
	{
	    pe->u.u.type = Expose;
	    pe->u.expose.window = pWin->wid;
	    pe->u.expose.x = pBox->x1;
	    pe->u.expose.y = pBox->y1;
	    pe->u.expose.width = pBox->x2 - pBox->x1;
	    pe->u.expose.height = pBox->y2 - pBox->y1;
	    pe->u.expose.count = (prgn->numRects - i);
	}
	DeliverEvents(pWin, pEvent, prgn->numRects, NullWindow);
	prgn->numRects = 0;
	DEALLOCATE_LOCAL(pEvent);
    }
}


/*
    this code is highly unlikely.  it is not haile selassie.

    there is some hair here.  we can't just use the window's
clip region as it is, because if we are painting the border,
the border is not in the client area and so we will be excluded
when we validate the GC, and if we are painting a parent-relative
background, the area we want to paint is in some other window.
since we trust the code calling us to tell us to paint only areas
that are really ours, we will temporarily give the window a
clipList the size of the whole screen and an origin at (0,0).
this more or less assumes that ddX code will do translation
based on the window's absCorner, and that ValidateGC will
look at clipList, and that no other fields from the
window will be used.  it's not possible to just draw
in the root because it may be a different depth.

to get the tile to align correctly we set the GC's tile origin to
be the (x,y) of the window's upper left corner, after which we
get the right bits when drawing into the root.

in order to call ChangeGC, we need to get an id for the pixmap, and
enter it in the resource table.
*/
void
miPaintWindow(pWin, prgn, what)
WindowPtr pWin;
RegionPtr prgn;
int what;
{
    int gcval[6];
    int gcmask;
    RegionPtr prgnWin;
    DDXPointRec oldCorner;
    BoxRec box;
    GCPtr pGC;
    int pid = 0;
    register int i;
    register BoxPtr pbox;
    register ScreenPtr pScreen = pWin->drawable.pScreen;
    register xRectangle *prect;

    gcval[0] = GXcopy;
    gcval[3] = pWin->absCorner.x;
    gcval[4] = pWin->absCorner.y;
    gcval[5] = None;
    gcmask = GCFunction | GCFillStyle |
	     GCTileStipXOrigin | GCTileStipYOrigin | GCClipMask;
    if (what == PW_BACKGROUND)
    {
	if (pWin->backgroundTile == None)
	    return;
	else if ((int)pWin->backgroundTile == ParentRelative)
	{
	    (*pWin->parent->PaintWindowBorder)(pWin->parent, prgn, what);
	    return;
	}
	else if ((int)pWin->backgroundTile == USE_BACKGROUND_PIXEL)
	{
	    gcval[1] = pWin->backgroundPixel;
	    gcval[2] = FillSolid;
	    gcmask |= GCForeground;
	}
	else
	{
	    gcval[1] = FillTiled;
	    pid = FakeClientID(0);
	    AddResource(pid, RT_PIXMAP, pWin->backgroundTile,
			NoopDDA, RC_CORE);
	    gcval[2] = pid;
	    gcmask |= GCTile;
	}
    }
    else
    {
	if (pWin->borderTile == None)
	    return;
	else if ((int)pWin->borderTile == USE_BORDER_PIXEL)
	{
	    gcval[1] = pWin->borderPixel;
	    gcval[2] = FillSolid;
	    gcmask |= GCForeground;
	}
	else
	{
	    gcval[1] = FillTiled;
	    pid = FakeClientID(0);
	    AddResource(pid, RT_PIXMAP, pWin->borderTile,
			NoopDDA, RC_CORE);
	    gcval[2] = pid;
	    gcmask |= GCTile;
	}
    }
    pGC = GetScratchGC(pWin->drawable.depth, pWin->drawable.pScreen);
    DoChangeGC(pGC, gcmask, gcval, 0);

    box.x1 = 0;
    box.y1 = 0;
    box.x2 = pScreen->width;
    box.y2 = pScreen->height;
    
    prgnWin = pWin->clipList;
    oldCorner = pWin->absCorner;
    pWin->absCorner.x = pWin->absCorner.y = 0;
    pWin->clipList = (*pScreen->RegionCreate)(&box, 1);
    pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    ValidateGC(pWin, pGC);

    prect = (xRectangle *)ALLOCATE_LOCAL(prgn->numRects * sizeof(xRectangle));
    pbox = prgn->rects;
    for (i= 0; i < prgn->numRects; i++, pbox++, prect++)
    {
	prect->x = pbox->x1;
	prect->y = pbox->y1;
	prect->width = pbox->x2 - pbox->x1;
	prect->height = pbox->y2 - pbox->y1;
    }
    prect -= prgn->numRects;
    (*pGC->PolyFillRect)(pWin, pGC, prgn->numRects, prect);
    DEALLOCATE_LOCAL(prect);

    if (pid)
	FreeResource(pid, RC_CORE);
    (*pScreen->RegionDestroy)(pWin->clipList);
    pWin->clipList = prgnWin;
    pWin->absCorner = oldCorner;
    pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    FreeScratchGC(pGC);
}

