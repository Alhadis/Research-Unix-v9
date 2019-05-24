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
/* $Header: mfbgc.c,v 1.115 87/09/03 09:04:25 toddb Exp $ */
#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "dixfontstr.h"
#include "fontstruct.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "region.h"

#include "mfb.h"
#include "mistruct.h"

#include "maskbits.h"

static PixmapPtr BogusPixmap = (PixmapPtr)1;

Bool
mfbCreateGC(pGC)
    register GCPtr pGC;
{
    mfbPrivGC 	*pPriv;
    GCInterestPtr	pQ;

    pGC->clientClip = NULL;
    pGC->clientClipType = CT_NONE;
    
    /* some of the output primitives aren't really necessary, since
       they will be filled in ValidateGC because of dix/CreateGC()
       setting all the change bits.  Others are necessary because although
       they depend on being a monochrome frame buffer, they don't change 
    */

    pGC->FillSpans = mfbWhiteSolidFS;
    pGC->SetSpans = mfbSetSpans;
    pGC->PutImage = mfbPutImage;
    pGC->CopyArea = mfbCopyArea;
    pGC->CopyPlane = mfbCopyPlane;
    pGC->PolyPoint = mfbPolyPoint;

    pGC->Polylines = mfbLineSS;
    pGC->PolySegment = miPolySegment;
    pGC->PolyRectangle = miPolyRectangle;
    pGC->PolyArc = miPolyArc;
    pGC->FillPolygon = miFillPolygon;
    pGC->PolyFillRect = mfbPolyFillRect;
    pGC->PolyFillArc = miPolyFillArc;
    pGC->PolyText8 = miPolyText8;
    pGC->ImageText8 = miImageText8;
    pGC->PolyText16 = miPolyText16;
    pGC->ImageText16 = miImageText16;
    pGC->ImageGlyphBlt = mfbImageGlyphBltWhite;
    pGC->PolyGlyphBlt = mfbPolyGlyphBltInvert;
    pGC->PushPixels = mfbPushPixels;
    pGC->LineHelper = miMiter;
    pGC->ChangeClip = mfbChangeClip;
    pGC->DestroyClip = mfbDestroyClip;
    pGC->CopyClip = mfbCopyClip;

    /* mfb wants to translate before scan convesion */
    pGC->miTranslate = 1;

    pPriv = (mfbPrivGC *)Xalloc(sizeof(mfbPrivGC));
    if (!pPriv)
	return FALSE;
    else
    {
	pPriv->rop = ReduceRop(pGC->alu, pGC->fgPixel);
	pPriv->fExpose = TRUE;
	pGC->devPriv = (pointer)pPriv;
	pPriv->pRotatedTile = NullPixmap;
	pPriv->pRotatedStipple = NullPixmap;
	pPriv->pAbsClientRegion =(* pGC->pScreen->RegionCreate)(NULL, 1); 

	/* since freeCompClip isn't FREE_CC, we don't need to create
	   a null region -- no one will try to free the field.
	*/
	pPriv->freeCompClip = REPLACE_CC;
	pPriv->ppPixmap = &BogusPixmap;
	pPriv->FillArea = mfbSolidInvertArea;
    }
    pQ = (GCInterestPtr) Xalloc(sizeof(GCInterestRec));
    if(!pQ)
    {
	Xfree(pPriv);
	return FALSE;
    }
     
    /* Now link this device into the GCque */
    pGC->pNextGCInterest = pQ;
    pGC->pLastGCInterest = pQ;
    pQ->pNextGCInterest = (GCInterestPtr) &pGC->pNextGCInterest;
    pQ->pLastGCInterest = (GCInterestPtr) &pGC->pNextGCInterest;
    pQ->length = sizeof(GCInterestRec);
    pQ->owner = 0;		/* server owns this */
    pQ->ValInterestMask = ~0;	/* interested in everything at validate time */
    pQ->ValidateGC = mfbValidateGC;
    pQ->ChangeInterestMask = 0; /* interested in nothing at change time */
    pQ->ChangeGC = (int (*) () ) NULL;
    pQ->CopyGCSource = (void (*) () ) NULL;
    pQ->CopyGCDest = (void (*) () ) NULL;
    pQ->DestroyGC = mfbDestroyGC;
    return TRUE;
}

void
mfbDestroyGC(pGC, pQ)
    GC 			*pGC;
    GCInterestPtr	pQ;

{
    mfbPrivGC *pPriv;

    /* Most GCInterest pointers would free pQ->devPriv.  This one is privileged
     * and allowed to allocate its private data directly in the GC (this
     * saves an indirection).  We must also unlink and free the pQ.
     */
    pQ->pLastGCInterest->pNextGCInterest = pQ->pNextGCInterest;
    pQ->pNextGCInterest->pLastGCInterest = pQ->pLastGCInterest;

    pPriv = (mfbPrivGC *)(pGC->devPriv);
    if (pPriv->pRotatedTile)
	mfbDestroyPixmap(pPriv->pRotatedTile);
    if (pPriv->pRotatedStipple)
	mfbDestroyPixmap(pPriv->pRotatedStipple);
    if (pPriv->freeCompClip == FREE_CC)
	(*pGC->pScreen->RegionDestroy)(pPriv->pCompositeClip);
    if(pPriv->pAbsClientRegion)
	(*pGC->pScreen->RegionDestroy)(pPriv->pAbsClientRegion);
    Xfree(pGC->devPriv);
    Xfree(pQ);
}

#define WINMOVED(pWin, pGC) \
((pWin->absCorner.x != pGC->lastWinOrg.x) || \
 (pWin->absCorner.y != pGC->lastWinOrg.y))

/* Clipping conventions
	if the drawable is a window
	    CT_REGION ==> pCompositeClip really is the composite
	    CT_other ==> pCompositeClip is the window clip region
	if the drawable is a pixmap
	    CT_REGION ==> pCompositeClip is the translated client region
		clipped to the pixmap boundary
	    CT_other ==> pCompositeClip is the pixmap bounding box
*/

void
mfbValidateGC(pGC, pQ, changes, pDrawable)
    register GCPtr 	pGC;
    GCInterestPtr	pQ;
    Mask 		changes;
    DrawablePtr 	pDrawable;
{
    register mfbPrivGCPtr	devPriv;
    WindowPtr pWin;
    int mask;			/* stateChanges */
    int index;			/* used for stepping through bitfields */
    int	xrot, yrot;		/* rotations for tile and stipple pattern */
    int rrop;			/* reduced rasterop */
				/* flags for changing the proc vector 
				   and updating things in devPriv
				*/
    int new_rotate, new_rrop,  new_line, new_text, new_fill;
    DDXPointRec	oldOrg;		/* origin of thing GC was last used with */

    if (pDrawable->type == DRAWABLE_WINDOW)
	pWin = (WindowPtr)pDrawable;
    else
	pWin = (WindowPtr)NULL;

    devPriv = ((mfbPrivGCPtr) (pGC->devPriv));
    oldOrg = pGC->lastWinOrg;
    /*
	if the client clip is different or moved OR
	the subwindowMode has changed OR
	the window's clip has changed since the last validation
	we need to recompute the composite clip
    */

    if ((changes & (GCClipXOrigin|GCClipYOrigin|GCClipMask)) ||
	(changes & GCSubwindowMode) ||
	(pDrawable->serialNumber != (pGC->serialNumber & DRAWABLE_SERIAL_BITS))
       )
    {

        /* if there is a client clip (always a region, for us) AND
	        it has moved or is different OR
	        the window has moved
           we need to (re)translate it.
        */
	if ((pGC->clientClipType == CT_REGION) &&
	    ((changes & (GCClipXOrigin|GCClipYOrigin|GCClipMask)) ||
	     (pWin && WINMOVED(pWin, pGC))
	    )
	   )
	{
	    /* retranslate client clip */
	    (* pGC->pScreen->RegionCopy)( devPriv->pAbsClientRegion, 
			                  pGC->clientClip);

	    if (pWin)
	    {
		pGC->lastWinOrg.x = pWin->absCorner.x;
		pGC->lastWinOrg.y = pWin->absCorner.y;
		(* pGC->pScreen->TranslateRegion)(
	                       devPriv->pAbsClientRegion, 
			       pGC->lastWinOrg.x + pGC->clipOrg.x,
			       pGC->lastWinOrg.y + pGC->clipOrg.y);
	    }
	    else
	    {
		pGC->lastWinOrg.x = 0;
		pGC->lastWinOrg.y = 0;
		(* pGC->pScreen->TranslateRegion)(
	            devPriv->pAbsClientRegion, pGC->clipOrg.x, pGC->clipOrg.y);
	    }
	}

	if (pWin)
	{
	    int freeTmpClip, freeCompClip;
	    RegionPtr pregWin;		/* clip for this window, without
					   client clip */

	    if (pGC->subWindowMode == IncludeInferiors)
	    {
	        pregWin = NotClippedByChildren(pWin);
		freeTmpClip = FREE_CC;
	    }
	    else
	    {
	        pregWin = pWin->clipList;
		freeTmpClip = REPLACE_CC;
	    }
	    freeCompClip = devPriv->freeCompClip;

	    /* if there is no client clip, we can get by with
	       just keeping the pointer we got, and remembering
	       whether or not should destroy (or maybe re-use)
	       it later.  this way, we avoid unnecessary copying
	       of regions.  (this wins especially if many clients clip
	       by children and have no client clip.)
	    */
	    if (pGC->clientClipType == CT_NONE)
	    {
	        if(freeCompClip == FREE_CC) 
		{
		    (* pGC->pScreen->RegionDestroy) (devPriv->pCompositeClip);
		}
	        devPriv->pCompositeClip = pregWin;
	        devPriv->freeCompClip = freeTmpClip;
	    }
	    else
	    {
		/* we need one 'real' region to put into the composite
		   clip.
			if pregWin and the current composite clip 
		   are real, we can get rid of one.
			if the current composite clip is real and
		   pregWin isn't, intersect the client clip and
		   pregWin into the existing composite clip.
			if pregWin is real and the current composite
		   clip isn't, intersect pregWin with the client clip
		   and replace the composite clip with it.
			if neither is real, create a new region and
		   do the intersection into it.
		*/

		if ((freeTmpClip == FREE_CC) && (freeCompClip == FREE_CC))
		{
		    (* pGC->pScreen->Intersect)(
		        devPriv->pCompositeClip,
			pregWin,
			devPriv->pAbsClientRegion);
		    (* pGC->pScreen->RegionDestroy)(pregWin);
		}
		else if ((freeTmpClip == REPLACE_CC) && 
		        (freeCompClip == FREE_CC))
		{
		    (* pGC->pScreen->Intersect)(
			devPriv->pCompositeClip,
		        pregWin,
			devPriv->pAbsClientRegion);
		}
		else if ((freeTmpClip == FREE_CC) &&
		         (freeCompClip == REPLACE_CC))
		{
		    (* pGC->pScreen->Intersect)( 
		       pregWin,
		       pregWin,
		       devPriv->pAbsClientRegion);
		    devPriv->pCompositeClip = pregWin;
		}
		else if ((freeTmpClip == REPLACE_CC) &&
		         (freeCompClip == REPLACE_CC))
		{
		    devPriv->pCompositeClip = 
			(* pGC->pScreen->RegionCreate)(NULL, 1);
		    (* pGC->pScreen->Intersect)(
			devPriv->pCompositeClip,
		        pregWin,
			devPriv->pAbsClientRegion);
		}
		devPriv->freeCompClip = FREE_CC;
	    }
	} /* end of composite clip for a window */
	else
	{
	    BoxRec pixbounds;

	    pixbounds.x1 = 0;
	    pixbounds.y1 = 0;
	    pixbounds.x2 = ((PixmapPtr)pDrawable)->width;
	    pixbounds.y2 = ((PixmapPtr)pDrawable)->height;

	    if (devPriv->freeCompClip == FREE_CC)
	        (* pGC->pScreen->RegionReset)(
		    devPriv->pCompositeClip, &pixbounds);
	    else
	    {
		devPriv->freeCompClip = FREE_CC;
		devPriv->pCompositeClip = 
			(* pGC->pScreen->RegionCreate)(&pixbounds, 1);
	    }

	    if (pGC->clientClipType == CT_REGION)
		(* pGC->pScreen->Intersect)(
		   devPriv->pCompositeClip, 
		   devPriv->pCompositeClip,
		   devPriv->pAbsClientRegion);
	} /* end of composite clip for pixmap */
    }

    /* we need to re-rotate the tile if the previous window/pixmap
       origin (oldOrg) differs from the new window/pixmap origin
       (pGC->lastWinOrg)
    */
    if ((oldOrg.x != pGC->lastWinOrg.x) ||
	(oldOrg.y != pGC->lastWinOrg.y))
    {
	new_rotate = TRUE;
    }
    else
    {
	new_rotate = FALSE;
    }

    new_rrop = FALSE;
    new_line = FALSE;
    new_text = FALSE;
    new_fill = FALSE;

    mask = changes;
    while (mask)
    {
	index = ffs(mask) - 1;
	mask &= ~(index = (1 << index));

	/* this switch acculmulates a list of which procedures
	   might have to change due to changes in the GC.  in
	   some cases (e.g. changing one 16 bit tile for another)
	   we might not really need a change, but the code is
	   being paranoid.
	   this sort of batching wins if, for example, the alu
	   and the font have been changed, or any other pair
	   of items that both change the same thing.
	*/
	switch (index)
	{
	  case GCFunction:
	  case GCForeground:
	    new_rrop = TRUE;
	    break;
	  case GCPlaneMask:
	    break;
	  case GCBackground:
	    new_rrop = TRUE;	/* for opaque stipples */
	    break;
	  case GCLineStyle:
	    new_line = TRUE;
	    break;
	  case GCLineWidth:
	  case GCCapStyle:
	  case GCJoinStyle:
	    new_line = TRUE;
	    break;
	  case GCFillStyle:
	    new_fill = TRUE;
	    break;
	  case GCFillRule:
	    break;
	  case GCTile:
	    if(pGC->tile == (PixmapPtr)NULL)
		break;
	    mfbPadPixmap(pGC->tile);
	    new_rotate = TRUE;
	    new_fill = TRUE;
	    break;

	  case GCStipple:
	    if(pGC->stipple == (PixmapPtr)NULL)
		break;
	    mfbPadPixmap(pGC->stipple);
	    new_rotate = TRUE;
	    new_fill = TRUE;
	    break;

	  case GCTileStipXOrigin:
	    new_rotate = TRUE;
	    break;

	  case GCTileStipYOrigin:
	    new_rotate = TRUE;
	    break;

	  case GCFont:
	    new_text = TRUE;
	    break;
	  case GCSubwindowMode:
	    break;
	  case GCGraphicsExposures:
	    break;
	  case GCClipXOrigin:
	    break;
	  case GCClipYOrigin:
	    break;
	  case GCClipMask:
	    break;
	  case GCDashOffset:
	    break;
	  case GCDashList:
	    break;
	  case GCArcMode:
	    break;
	  default:
	    break;
	}
    }

    /* deal with the changes we've collected .
       new_rrop must be done first because subsequent things
       depend on it.
    */
    if (new_rrop || new_fill)
    {
	rrop = ReduceRop(pGC->alu, pGC->fgPixel);
	devPriv->rop = rrop;
	new_fill = TRUE;
	/* FillArea raster op is GC's for tile filling,
	   and the reduced rop for solid and stipple
	*/
	if (pGC->fillStyle == FillTiled)
	    devPriv->ropFillArea = pGC->alu;
	else
	    devPriv->ropFillArea = rrop;

	/* opaque stipples:
	   fg	bg	ropOpStip	fill style
	   1	0	alu		tile
	   0	1	inverseAlu	tile
	   1	1	rrop(fg, alu)	solid
	   0	0	rrop(fg, alu)	solid
	Note that rrop(fg, alu) == mfbPrivGC.rop, so we don't really need to
	compute it.
	*/
        if (pGC->fillStyle == FillOpaqueStippled)
        {
	    if (pGC->fgPixel != pGC->bgPixel)
	    {
	        if (pGC->fgPixel)
		    devPriv->ropOpStip = pGC->alu;
	        else
		    devPriv->ropOpStip = InverseAlu[pGC->alu];
	    }
	    else
	        devPriv->ropOpStip = rrop;
	    devPriv->ropFillArea = devPriv->ropOpStip;
        }
    }
    else
	rrop = devPriv->rop;

    if (new_line || new_fill)
    {
	if (pGC->lineStyle == LineSolid)
	{
	    if(pGC->lineWidth == 0)
	    {
	        if (pGC->fillStyle == FillSolid)
		    pGC->Polylines = mfbLineSS;
	        else
		    pGC->Polylines = miZeroLine;
	    }
	    else
	    {
		pGC->Polylines = miWideLine;
	    }
	}
	else
	    if(pGC->lineWidth == 0)
	        pGC->Polylines = mfbDashLine;
	    else
	        pGC->Polylines = miWideDash;

	switch(pGC->joinStyle)
	{
	  case JoinMiter:
	    pGC->LineHelper = miMiter;
	    break;
	  case JoinRound:
	  case JoinBevel:
	    pGC->LineHelper = miNotMiter;
	    break;
	}
    }

    if (new_text || new_fill)
    {
	if ((pGC->font) &&
	    (pGC->font->pFI->maxbounds.metrics.rightSideBearing -
	     pGC->font->pFI->maxbounds.metrics.leftSideBearing) > 32)
	{
	    pGC->PolyGlyphBlt = miPolyGlyphBlt;
	    pGC->ImageGlyphBlt = miImageGlyphBlt;
	}
	else
	{
	    /* special case ImageGlyphBlt for terminal emulator fonts */
	    if ((pGC->font) &&
		(pGC->font->pFI->terminalFont) &&
		(pGC->fgPixel != pGC->bgPixel))
	    {
		/* pcc bug makes this not compile...
		pGC->ImageGlyphBlt = (pGC->fgPixel) ? mfbTEGlyphBltWhite :
						      mfbTEGlyphBltBlack;
		*/
		if (pGC->fgPixel)
		    pGC->ImageGlyphBlt = mfbTEGlyphBltWhite;
		else
		    pGC->ImageGlyphBlt = mfbTEGlyphBltBlack;
	    }
	    else
	    {
	        if (pGC->fgPixel == 0)
		    pGC->ImageGlyphBlt = mfbImageGlyphBltBlack;
	        else
		    pGC->ImageGlyphBlt = mfbImageGlyphBltWhite;
	    }

	    /* now do PolyGlyphBlt */
	    if (pGC->fillStyle == FillSolid ||
		(pGC->fillStyle == FillOpaqueStippled &&
		 pGC->fgPixel == pGC->bgPixel
		)
	       )
	    {
		if (rrop == RROP_WHITE)
		    pGC->PolyGlyphBlt = mfbPolyGlyphBltWhite;
		else if (rrop == RROP_BLACK)
		    pGC->PolyGlyphBlt = mfbPolyGlyphBltBlack;
		else if (rrop == RROP_INVERT)
		    pGC->PolyGlyphBlt = mfbPolyGlyphBltInvert;
		else
		    pGC->PolyGlyphBlt = NoopDDA;
	    }
	    else
	    {
		pGC->PolyGlyphBlt = miPolyGlyphBlt;
	    }
	}
    }

    if (new_fill)
    {
	/* install a suitable fillspans */
	if ((pGC->fillStyle == FillSolid) ||
	    (pGC->fillStyle == FillOpaqueStippled && pGC->fgPixel==pGC->bgPixel)
	   )
	{
	    switch(devPriv->rop)
	    {
	      case RROP_WHITE:
		pGC->FillSpans = mfbWhiteSolidFS;
		break;
	      case RROP_BLACK:
		pGC->FillSpans = mfbBlackSolidFS;
		break;
	      case RROP_INVERT:
		pGC->FillSpans = mfbInvertSolidFS;
		break;
	      case RROP_NOP:
		pGC->FillSpans = NoopDDA;
		break;
	    }
	}
	/* beyond this point, opaqueStippled ==> fg != bg */
	else if ((pGC->fillStyle==FillTiled && pGC->tile->width!=32) ||
		 (pGC->fillStyle==FillOpaqueStippled && pGC->stipple->width!=32)
		)
	{
	    pGC->FillSpans = mfbUnnaturalTileFS;
	}
	else if (pGC->fillStyle == FillStippled && pGC->stipple->width != 32)
	{
	    pGC->FillSpans = mfbUnnaturalStippleFS;
	}
	else if (pGC->fillStyle == FillStippled)
	{
	    switch(devPriv->rop)
	    {
	      case RROP_WHITE:
		pGC->FillSpans = mfbWhiteStippleFS;
		break;
	      case RROP_BLACK:
		pGC->FillSpans = mfbBlackStippleFS;
		break;
	      case RROP_INVERT:
		pGC->FillSpans = mfbInvertStippleFS;
		break;
	      case RROP_NOP:
		pGC->FillSpans = NoopDDA;
		break;
	    }
	}
	else /* overload tiles to do parti-colored opaque stipples */
	{
	    pGC->FillSpans = mfbTileFS;
	}

	/* the rectangle code doesn't deal with opaque stipples that
	   are two colors -- we can fool it for fg==bg, though
	 */
	if (((pGC->fillStyle == FillTiled) && (pGC->tile->width!=32)) ||
	    ((pGC->fillStyle == FillStippled) && (pGC->stipple->width!=32)) ||
	    ((pGC->fillStyle == FillOpaqueStippled) &&
	     (pGC->fgPixel != pGC->bgPixel))
	   )
	{
	    pGC->PolyFillRect = miPolyFillRect;
	    devPriv->ppPixmap = &BogusPixmap;
	}
	else /* deal with solids and natural stipples and tiles */
	{
	    pGC->PolyFillRect = mfbPolyFillRect;

	    if ((pGC->fillStyle == FillSolid) ||
		(pGC->fillStyle == FillOpaqueStippled &&
		 pGC->fgPixel == pGC->bgPixel)
	       )
	    {
	        devPriv->ppPixmap = &BogusPixmap;
		switch(devPriv->rop)
		{
		  case RROP_WHITE:
		    devPriv->FillArea = mfbSolidWhiteArea;
		    break;
		  case RROP_BLACK:
		    devPriv->FillArea = mfbSolidBlackArea;
		    break;
		  case RROP_INVERT:
		    devPriv->FillArea = mfbSolidInvertArea;
		    break;
		  case RROP_NOP:
		    devPriv->FillArea = NoopDDA;
		    break;
		}
	    }
	    else if (pGC->fillStyle == FillStippled)
	    {
		devPriv->ppPixmap = &devPriv->pRotatedStipple;
		switch(devPriv->rop)
		{
		  case RROP_WHITE:
		    devPriv->FillArea = mfbStippleWhiteArea;
		    break;
		  case RROP_BLACK:
		    devPriv->FillArea = mfbStippleBlackArea;
		    break;
		  case RROP_INVERT:
		    devPriv->FillArea = mfbStippleInvertArea;
		    break;
		  case RROP_NOP:
		    devPriv->FillArea = NoopDDA;
		    break;
		}
	    }
	    else /* deal with tiles */
	    {
		if (pGC->fillStyle == FillTiled)
		    devPriv->ppPixmap = &devPriv->pRotatedTile;
		else
		    devPriv->ppPixmap = &devPriv->pRotatedStipple;
		devPriv->FillArea = mfbTileArea32;
	    }
	} /* end of natural rectangles */
    } /* end of new_fill */


    if(new_rotate)
    {
	/* figure out how much to rotate */
        xrot = pGC->patOrg.x;
        yrot = pGC->patOrg.y;
	if (pWin)
	{
	    xrot += pWin->absCorner.x;
	    yrot += pWin->absCorner.y;
	}

	/* destroy any previously rotated tile or stipple */
	if(devPriv->pRotatedTile)
	{
	    mfbDestroyPixmap(devPriv->pRotatedTile);
	    devPriv->pRotatedTile = (PixmapPtr)NULL;
	}
	if(devPriv->pRotatedStipple)
	{
	    mfbDestroyPixmap(devPriv->pRotatedStipple);
	    devPriv->pRotatedStipple = (PixmapPtr)NULL;
	}

	/* copy current tile and stipple */
        if(pGC->tile &&
	   (pGC->tile->width == 32) &&
	   (devPriv->pRotatedTile = mfbCopyPixmap(pGC->tile)) ==
	       (PixmapPtr)NULL)
	    return ;           /* shouldn't happen, internal error */
        if(pGC->stipple &&
	   (pGC->stipple->width == 32) &&
	   (devPriv->pRotatedStipple = mfbCopyPixmap(pGC->stipple)) ==
	       (PixmapPtr)NULL)
	    return ;          /* shouldn't happen, internal error */

        if(xrot)
        {
	    if (pGC->tile && pGC->tile->width == 32 && 
	        devPriv->pRotatedTile)
	        mfbXRotatePixmap(devPriv->pRotatedTile, xrot); 
	    if (pGC->stipple && pGC->stipple->width == 32 && 
	        devPriv->pRotatedStipple)
	        mfbXRotatePixmap(devPriv->pRotatedStipple, xrot); 
        }
        if(yrot)
        {
	    if (pGC->tile && pGC->tile->width == 32 && 
	        devPriv->pRotatedTile)
	        mfbYRotatePixmap(devPriv->pRotatedTile, yrot); 
	    if (pGC->tile && pGC->tile->width == 32 && 
	        devPriv->pRotatedStipple)
	        mfbYRotatePixmap(devPriv->pRotatedStipple, yrot); 
        }
    }

    return ;
}

/* table to map alu(src, dst) to alu(~src, dst) */
int InverseAlu[16] = {
	GXclear,
	GXandInverted,
	GXnor,
	GXcopyInverted,
	GXand,
	GXnoop,
	GXequiv,
	GXorInverted,
	GXandReverse,
	GXxor,
	GXinvert,
	GXnand,
	GXcopy,
	GXor,
	GXorReverse,
	GXset
	};

ReduceRop(alu, src)
    register int alu;
    register int src;
{
    int rop;
    if (src == 0)	/* src is black */
    {
	switch(alu)
	{
	  case GXclear:
	    rop = RROP_BLACK;
	    break;
	  case GXand:
	    rop = RROP_BLACK;
	    break;
	  case GXandReverse:
	    rop = RROP_BLACK;
	    break;
	  case GXcopy:
	    rop = RROP_BLACK;
	    break;
	  case GXandInverted:
	    rop = RROP_NOP;
	    break;
	  case GXnoop:
	    rop = RROP_NOP;
	    break;
	  case GXxor:
	    rop = RROP_NOP;
	    break;
	  case GXor:
	    rop = RROP_NOP;
	    break;
	  case GXnor:
	    rop = RROP_INVERT;
	    break;
	  case GXequiv:
	    rop = RROP_INVERT;
	    break;
	  case GXinvert:
	    rop = RROP_INVERT;
	    break;
	  case GXorReverse:
	    rop = RROP_INVERT;
	    break;
	  case GXcopyInverted:
	    rop = RROP_WHITE;
	    break;
	  case GXorInverted:
	    rop = RROP_WHITE;
	    break;
	  case GXnand:
	    rop = RROP_WHITE;
	    break;
	  case GXset:
	    rop = RROP_WHITE;
	    break;
	}
    }
    else /* src is white */
    {
	switch(alu)
	{
	  case GXclear:
	    rop = RROP_BLACK;
	    break;
	  case GXand:
	    rop = RROP_NOP;
	    break;
	  case GXandReverse:
	    rop = RROP_INVERT;
	    break;
	  case GXcopy:
	    rop = RROP_WHITE;
	    break;
	  case GXandInverted:
	    rop = RROP_BLACK;
	    break;
	  case GXnoop:
	    rop = RROP_NOP;
	    break;
	  case GXxor:
	    rop = RROP_INVERT;
	    break;
	  case GXor:
	    rop = RROP_WHITE;
	    break;
	  case GXnor:
	    rop = RROP_BLACK;
	    break;
	  case GXequiv:
	    rop = RROP_NOP;
	    break;
	  case GXinvert:
	    rop = RROP_INVERT;
	    break;
	  case GXorReverse:
	    rop = RROP_WHITE;
	    break;
	  case GXcopyInverted:
	    rop = RROP_BLACK;
	    break;
	  case GXorInverted:
	    rop = RROP_NOP;
	    break;
	  case GXnand:
	    rop = RROP_INVERT;
	    break;
	  case GXset:
	    rop = RROP_WHITE;
	    break;
	}
    }
    return rop;
}

void
mfbDestroyClip(pGC)
    GCPtr	pGC;
{
    if(pGC->clientClipType == CT_NONE)
	return;
    else if (pGC->clientClipType == CT_PIXMAP)
    {
	mfbDestroyPixmap((PixmapPtr)(pGC->clientClip));
    }
    else
    {
	/* we know we'll never have a list of rectangles, since
	   ChangeClip immediately turns them into a region 
	*/
        (*pGC->pScreen->RegionDestroy)(pGC->clientClip);
    }
    pGC->clientClip = NULL;
    pGC->clientClipType = CT_NONE;
}

void
mfbChangeClip(pGC, type, pvalue, nrects)
    GCPtr	pGC;
    int		type;
    pointer	pvalue;
    int		nrects;
{
    /*
       remember to bump the pixmap's refcnt before destroying the old
       one, since the two may be the same.
       regions that have been fed to the clip code are assumed to have
       disappeared, so we'll never see the same one twice.
    */

    if (type == CT_PIXMAP)
    {
	/* so we can call destroy later, and leave this as
	   an example
	*/
	((PixmapPtr)(pvalue))->refcnt++;
    }

    mfbDestroyClip(pGC);
    if(type == CT_PIXMAP)
    {
	/* convert the pixmap to a region */
	pGC->clientClip = (pointer) mfbPixmapToRegion(pvalue);
	/* you wouldn't do this if you were leaving the pixmap in
	   rather than converting it.
	*/
	(*pGC->pScreen->DestroyPixmap)(pvalue);
    }
    else if (type == CT_REGION)
    {
	/* stuff the region in the GC */
	pGC->clientClip = pvalue;
    }
    else if (type != CT_NONE)
    {
	pGC->clientClip = (pointer) miRectsToRegion(pGC, nrects, pvalue, type);
	Xfree(pvalue);
    }
    pGC->clientClipType = (type != CT_NONE && pGC->clientClip) ? CT_REGION :
        CT_NONE;
    pGC->stateChanges |= GCClipMask;
}

void
mfbCopyClip (pgcDst, pgcSrc)
    GCPtr pgcDst, pgcSrc;
{
    RegionPtr prgnNew;

    switch(pgcSrc->clientClipType)
    {
      case CT_NONE:
      case CT_PIXMAP:
	mfbChangeClip(pgcDst, pgcSrc->clientClipType, pgcSrc->clientClip, 0);
	break;
      case CT_REGION:
	prgnNew = (*pgcSrc->pScreen->RegionCreate)(NULL, 1);
	(*pgcSrc->pScreen->RegionCopy)(prgnNew,
				       (RegionPtr)(pgcSrc->clientClip));
	mfbChangeClip(pgcDst, CT_REGION, prgnNew, 0);
	break;
    }
}

void
mfbCopyGCDest (pGC, pQ, changes, pGCSrc)
    GCPtr	pGC;
    GCInterestPtr	pQ;
    Mask 		changes;
    GCPtr		pGCSrc;
{
    RegionPtr		pClip;

    if(changes & GCClipMask)
    {
	if(pGC->clientClipType == CT_PIXMAP)
	{
	    ((PixmapPtr)pGC->clientClip)->refcnt++;
	}
	else if(pGC->clientClipType == CT_REGION)
	{
	    BoxRec pixbounds;

	    pixbounds.x1 = 0;
	    pixbounds.y1 = 0;
	    pixbounds.x2 = 0;
	    pixbounds.y2 = 0;

	    pClip = (RegionPtr) pGC->clientClip;
	    pGC->clientClip =
	        (pointer)(* pGC->pScreen->RegionCreate)(&pixbounds, 1);
	    (* pGC->pScreen->RegionCopy)(pGC->clientClip, pClip);
	}
    }
}
