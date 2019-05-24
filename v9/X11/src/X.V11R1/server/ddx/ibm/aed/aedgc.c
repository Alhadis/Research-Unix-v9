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
/* $Header: aedgc.c,v 1.1 87/09/13 03:34:31 erik Exp $ */
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
#include "rtutils.h"
#include "xaed.h"
extern aedXRotatePixmap();
extern miPolyPoint();

static PixmapPtr BogusPixmap = (PixmapPtr)1;

Bool
aedCreateGC(pGC)
    register GCPtr pGC;
{
    aedPrivGC 	*pPriv;
    GCInterestPtr	pQ;

    TRACE(("aedCreateGC(pGC= 0x%x)\n", pGC));
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

    pPriv = (aedPrivGC *)Xalloc(sizeof(aedPrivGC));
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
	pPriv->lastDrawableType = -1;
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
    pQ->ValidateGC = aedValidateGC;
    pQ->ChangeInterestMask = 0; /* interested in nothing at change time */
    pQ->ChangeGC = (int (*) () ) NULL;
    pQ->CopyGCSource = (void (*) () ) NULL;
    pQ->CopyGCDest = (void (*) () ) NULL;
    pQ->DestroyGC = aedDestroyGC;
    return TRUE;
}

void
aedDestroyGC(pGC, pQ)
    GC 			*pGC;
    GCInterestPtr	pQ;

{
    aedPrivGC *pPriv;

    TRACE(("aedDestroyGC(pGC= 0x%x, pQ= 0x%x)\n", pGC, pQ));

    /* Most GCInterest pointers would free pQ->devPriv.  This one is privileged
     * and allowed to allocate its private data directly in the GC (this
     * saves an indirection).  We must also unlink and free the pQ.
     */
    pQ->pLastGCInterest->pNextGCInterest = pQ->pNextGCInterest;
    pQ->pNextGCInterest->pLastGCInterest = pQ->pLastGCInterest;

    pPriv = (aedPrivGC *)(pGC->devPriv);
    if (pPriv->pRotatedTile)
	mfbDestroyPixmap(pPriv->pRotatedTile);
    if (pPriv->pRotatedStipple)
	mfbDestroyPixmap(pPriv->pRotatedStipple);
    if (pPriv->freeCompClip == FREE_CC && pPriv->pCompositeClip)
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
aedValidateGC(pGC, pQ, changes, pDrawable)
    register GCPtr 	pGC;
    GCInterestPtr	pQ;
    Mask 		changes;
    DrawablePtr 	pDrawable;
{
    register aedPrivGCPtr	devPriv;
    WindowPtr pWin;
    int mask;			/* stateChanges */
    int index;			/* used for stepping through bitfields */
    int	xrot, yrot;		/* rotations for tile and stipple pattern */
    Bool fRotate = FALSE;	/* True if rotated pixmaps are needed */
    int rrop;			/* reduced rasterop */
				/* flags for changing the proc vector */
    int new_rrop,  new_line, new_text, new_fill;

    TRACE(("aedValidateGC(pGC= 0x%x, pQ= 0x%x, changes=0x%x, pDrawable= 0x%x)\n", pGC, pQ, changes, pDrawable));

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	pWin = (WindowPtr)pDrawable;
    }
    else
    {
	pWin = (WindowPtr)NULL;
    }

    devPriv = ((aedPrivGCPtr) (pGC->devPriv));

    if (pDrawable->type != devPriv->lastDrawableType )
    {
        devPriv->lastDrawableType = pDrawable->type;
	if (pDrawable->type == DRAWABLE_WINDOW)
	{
	    pGC->SetSpans = aedSetSpans;
	    pGC->PutImage = miPutImage;
	    pGC->CopyArea = aedCopyArea;
	    pGC->CopyPlane = miCopyPlane;
	    pGC->PolyPoint = miPolyPoint;
	    pGC->PolySegment = aedPolySegment;

	    pGC->PushPixels = mfbPushPixels;
	    changes = ~0;
	}
	else
	{
	    pGC->SetSpans = mfbSetSpans;
	    pGC->PutImage = mfbPutImage;
	    pGC->CopyArea = mfbCopyArea;
	    pGC->CopyPlane = mfbCopyPlane;
	    pGC->PolyPoint = mfbPolyPoint;
	    pGC->PolySegment = miPolySegment;

	    pGC->PushPixels = mfbPushPixels;
	    changes = ~0;
	}
    }

    if (pDrawable->type != DRAWABLE_WINDOW)
    {
	mfbValidateGC(pGC, pQ, changes, pDrawable);
	return;
    }



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

    if (pWin)
    {

	/* rotate tile patterns so that pattern can be combined in word by
	 * word, but the pattern seems to begin aligned with the window */
	xrot = pWin->absCorner.x;
	yrot = pWin->absCorner.y;
    }
    else
    {
        yrot = 0;
	xrot = 0;
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
/*
	    mfbPadPixmap(pGC->tile);
*/
	    fRotate = TRUE;
	    new_fill = TRUE;
	    break;

	  case GCStipple:
	    if(pGC->stipple == (PixmapPtr)NULL)
		break;
/*
	    mfbPadPixmap(pGC->stipple);
*/
	    fRotate = TRUE;
	    new_fill = TRUE;
	    break;

	  case GCTileStipXOrigin:
	    fRotate = TRUE;
	    break;

	  case GCTileStipYOrigin:
	    fRotate = TRUE;
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
		if( pGC->fillStyle == FillSolid )
		    pGC->Polylines = aedSolidLine;
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
	        pGC->Polylines = aedDashLine;
/*
	        pGC->Polylines = miZeroLine;
*/
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
	    pGC->PolyGlyphBlt = miPolyGlyphBlt;
	    pGC->ImageGlyphBlt = aedImageGlyphBlt;
	    pGC->PolyGlyphBlt = miPolyGlyphBlt;
    }

    if (new_fill)
    {
	/* install a suitable fillspans */
	if ((pGC->fillStyle == FillSolid) ||
	   (pGC->fillStyle == FillOpaqueStippled && pGC->fgPixel==pGC->bgPixel)
	   )
	{
	    pGC->PushPixels = aedPushPixSolid;
	    pGC->FillSpans = aedSolidFS;
	}
	else /* overload tiles to do parti-colored opaque stipples */
	{
	    pGC->PushPixels = mfbPushPixels;
	    pGC->FillSpans = aedTileFS;
	}

	pGC->PolyFillRect = aedPolyFillRect;

	if ((pGC->fillStyle == FillSolid) ||
		(pGC->fillStyle == FillOpaqueStippled &&
		 pGC->fgPixel == pGC->bgPixel)
	       )
	{
	    devPriv->ppPixmap = &BogusPixmap;
	    if( devPriv->rop == RROP_NOP )
		devPriv->FillArea = NoopDDA;
	    else
		devPriv->FillArea = aedSolidFillArea;
	}
	else if (pGC->fillStyle == FillStippled)
	{
	    devPriv->ppPixmap = &devPriv->pRotatedStipple;
	    if( devPriv->rop == RROP_NOP )
		devPriv->FillArea = NoopDDA;
	    else
		devPriv->FillArea = aedStippleFillArea;
	}
	else /* deal with tiles */
	{
	    if (pGC->fillStyle == FillTiled)
		devPriv->ppPixmap = &devPriv->pRotatedTile;
	    else
		devPriv->ppPixmap = &devPriv->pRotatedStipple;
	    devPriv->FillArea = aedStippleFillArea;
	} /* end of natural rectangles */

    } /* end of new_fill */


    if(fRotate)
    {
        xrot += pGC->patOrg.x;
        yrot += pGC->patOrg.y;
    }
    if(xrot || yrot || fRotate)
    {

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
        if(pGC->tile &&
	   (devPriv->pRotatedTile = mfbCopyPixmap(pGC->tile)) ==
	       (PixmapPtr)NULL)
	    return ;           /* shouldn't happen, internal error */
        if(pGC->stipple &&
	   (devPriv->pRotatedStipple = mfbCopyPixmap(pGC->stipple)) ==
	       (PixmapPtr)NULL)
	    return ;          /* shouldn't happen, internal error */
    }
    if(xrot)
    {
	if (pGC->tile && devPriv->pRotatedTile)
	    aedXRotatePixmap(devPriv->pRotatedTile, xrot); 
	if (pGC->stipple && devPriv->pRotatedStipple)
	    aedXRotatePixmap(devPriv->pRotatedStipple, xrot); 
    }
    if(yrot)
    {
	if (pGC->tile && devPriv->pRotatedTile)
	    mfbYRotatePixmap(devPriv->pRotatedTile, yrot); 
	if (pGC->tile && devPriv->pRotatedStipple)
	    mfbYRotatePixmap(devPriv->pRotatedStipple, yrot); 
    }

    return ;
}

