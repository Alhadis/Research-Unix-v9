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
/* $Header: mibitblt.c,v 1.52 87/09/13 20:56:01 toddb Exp $ */
/* Author: Todd Newman  (aided and abetted by Mr. Drewry) */

#include "X.h"
#include "Xprotostr.h"

#include "misc.h"
#include "gcstruct.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "mi.h"
#include "regionstr.h"
#include "Xmd.h"
#include "../mfb/maskbits.h"
#include "servermd.h"


/* MICOPYAREA -- public entry for the CopyArea request 
 * For each rectangle in the source region
 *     get the pixels with GetSpans
 *     set them in the destination with SetSpans
 * We let SetSpans worry about clipping to the destination.
 */
void
miCopyArea(pSrcDrawable, pDstDrawable,
	    pGC, xIn, yIn, widthSrc, heightSrc, xOut, yOut)
    register DrawablePtr 	pSrcDrawable;
    register DrawablePtr 	pDstDrawable;
    GCPtr 			pGC;
    int 			xIn, yIn;
    int 			widthSrc, heightSrc;
    int 			xOut, yOut;
{
    DDXPointPtr		ppt, pptFirst;
    unsigned int	*pwidthFirst, *pwidth, *pbits;
    BoxRec 		srcBox, *prect;
    			/* may be a new region, or just a copy */
    RegionPtr 		prgnSrcClip;
    			/* non-0 if we've created a src clip */
    int 		realSrcClip = 0;
    int			srcx, srcy, dstx, dsty, i, j, y, width, height,
    			xMin, xMax, yMin, yMax;
    unsigned int	*ordering;

    /* clip the left and top edges of the source */
    if (xIn < 0)
    {
        widthSrc += xIn;
        srcx = 0;
    }
    else
	srcx = xIn;
    if (yIn < 0)
    {
        heightSrc += yIn;
        srcy = 0;
    }
    else
	srcy = yIn;


    /* If the destination isn't realized, this is easy */
    if (pDstDrawable->type == DRAWABLE_WINDOW &&
	!((WindowPtr)pDstDrawable)->realized)
    {
	miSendNoExpose(pGC);
	return;
    }

    /* clip the source */
    if (pSrcDrawable->type == DRAWABLE_PIXMAP)
    {
	BoxRec box;

	box.x1 = 0;
	box.y1 = 0;
	box.x2 = ((PixmapPtr)pSrcDrawable)->width;
	box.y2 = ((PixmapPtr)pSrcDrawable)->height;

	prgnSrcClip = (*pGC->pScreen->RegionCreate)(&box, 1);
	realSrcClip = 1;
    }
    else
    {
	srcx += ((WindowPtr)pSrcDrawable)->absCorner.x;
	srcy += ((WindowPtr)pSrcDrawable)->absCorner.y;
	prgnSrcClip = ((WindowPtr)pSrcDrawable)->clipList;
    }

    /* If the src drawable is a window, we need to translate the srcBox so
     * that we can compare it with the window's clip region later on. */
    srcBox.x1 = srcx;
    srcBox.y1 = srcy;
    srcBox.x2 = srcx  + widthSrc;
    srcBox.y2 = srcy  + heightSrc;

    if (pGC->miTranslate && (pDstDrawable->type == DRAWABLE_WINDOW) )
    {
	dstx = xOut + ((WindowPtr)pDstDrawable)->absCorner.x;
	dsty = yOut + ((WindowPtr)pDstDrawable)->absCorner.y;
    }
    else
    {
	dstx = xOut;
	dsty = yOut;
    }



    pptFirst = ppt = (DDXPointPtr)
        ALLOCATE_LOCAL(heightSrc * sizeof(DDXPointRec));
    pwidthFirst = pwidth = (unsigned int *)
        ALLOCATE_LOCAL(heightSrc * sizeof(unsigned int));
    ordering = (unsigned int *)
        ALLOCATE_LOCAL(prgnSrcClip->numRects * sizeof(unsigned int));
    if(!pptFirst || !pwidthFirst || !ordering)
    {
       if (pptFirst)
           DEALLOCATE_LOCAL(pptFirst);
       if (pwidthFirst)
           DEALLOCATE_LOCAL(pwidthFirst);
       if (ordering)
	   DEALLOCATE_LOCAL(ordering);
       return;
    }

    /* If not the same drawable then order of move doesn't matter.
       Following assumes that prgnSrcClip->rects are sorted from top
       to bottom and left to right.
    */
    if (pSrcDrawable != pDstDrawable)
      for (i=0; i < prgnSrcClip->numRects; i++)
        ordering[i] = i;
    else { /* within same drawable, must sequence moves carefully! */
      if (dsty <= srcBox.y1) { /* Scroll up or stationary vertical.
                                  Vertical order OK */
        if (dstx <= srcBox.x1) /* Scroll left or stationary horizontal.
                                  Horizontal order OK as well */
          for (i=0; i < prgnSrcClip->numRects; i++)
            ordering[i] = i;
        else { /* scroll right. must reverse horizontal banding of rects. */
          for (i=0, j=1, xMax=0;
               i < prgnSrcClip->numRects;
               j=i+1, xMax=i) {
            /* find extent of current horizontal band */
            y=prgnSrcClip->rects[i].y1; /* band has this y coordinate */
            while ((j < prgnSrcClip->numRects) &&
                   (prgnSrcClip->rects[j].y1 == y))
              j++;
            /* reverse the horizontal band in the output ordering */
            for (j-- ; j >= xMax; j--, i++)
              ordering[i] = j;
          }
        }
      }
      else { /* Scroll down. Must reverse vertical banding. */
        if (dstx < srcBox.x1) { /* Scroll left. Horizontal order OK. */
          for (i=prgnSrcClip->numRects-1, j=i-1, yMin=i, yMax=0;
              i >= 0;
              j=i-1, yMin=i) {
            /* find extent of current horizontal band */
            y=prgnSrcClip->rects[i].y1; /* band has this y coordinate */
            while ((j >= 0) &&
                   (prgnSrcClip->rects[j].y1 == y))
              j--;
            /* reverse the horizontal band in the output ordering */
            for (j++ ; j <= yMin; j++, i--, yMax++)
              ordering[yMax] = j;
          }
        }
        else /* Scroll right or horizontal stationary.
                Reverse horizontal order as well (if stationary, horizontal
                order can be swapped without penalty and this is faster
                to compute). */
          for (i=0, j=prgnSrcClip->numRects-1;
               i < prgnSrcClip->numRects;
               i++, j--)
              ordering[i] = j;
      }
    }
 
     for(i = 0;
         i < prgnSrcClip->numRects;
         i++)
     {
        prect = &prgnSrcClip->rects[ordering[i]];
  	xMin = max(prect->x1, srcBox.x1);
  	xMax = min(prect->x2, srcBox.x2);
  	yMin = max(prect->y1, srcBox.y1);
	yMax = min(prect->y2, srcBox.y2);
	/* is there anything visible here? */
	if(xMax <= xMin || yMax <= yMin)
	    continue;

        ppt = pptFirst;
	pwidth = pwidthFirst;
	y = yMin;
	height = yMax - yMin;
	width = xMax - xMin;

	for(j = 0; j < height; j++)
	{
	    /* We must untranslate before calling GetSpans */
	    ppt->x = xMin;
	    ppt++->y = y++;
	    *pwidth++ = width;
	}
	pbits = (*pSrcDrawable->pScreen->GetSpans)(pSrcDrawable, width, 
					       pptFirst, pwidthFirst, height);
        ppt = pptFirst;
	pwidth = pwidthFirst;
	xMin -= (srcx - dstx);
	y = yMin - (srcy - dsty);
	for(j = 0; j < height; j++)
	{
	    ppt->x = xMin;
	    ppt++->y = y++;
	    *pwidth++ = width;
	}
	
        (*pGC->SetSpans)(pDstDrawable, pGC, pbits, pptFirst, pwidthFirst,
	                 height, TRUE);
        Xfree(pbits);
    }
    miHandleExposures(pSrcDrawable, pDstDrawable, pGC, xIn, yIn,
		      widthSrc, heightSrc, xOut, yOut);
    if(realSrcClip)
	(*pGC->pScreen->RegionDestroy)(prgnSrcClip);
		
    DEALLOCATE_LOCAL(pptFirst);
    DEALLOCATE_LOCAL(pwidthFirst);
    DEALLOCATE_LOCAL(ordering);
}

/* MIGETPLANE -- gets a bitmap representing one plane of pDraw
 * A helper used for CopyPlane and XY format GetImage 
 * No clever strategy here, we grab a scanline at a time, pull out the
 * bits and then stuff them in a 1 bit deep map.
 */
static
unsigned long	*
miGetPlane(pDraw, planeNum, sx, sy, w, h, result)
    DrawablePtr		pDraw;
    int			planeNum;	/* number of the bitPlane */
    int			sx, sy, w, h;
    unsigned long	*result;
{
    int			i, j, k, depth, width, bitsPerPixel, widthInBytes;
    DDXPointRec 	pt;
    unsigned int 	*pline;
    unsigned int	bit;
    unsigned char	*pCharsOut;
    CARD16		*pShortsOut;
    CARD32		*pLongsOut;

    depth = pDraw->depth;
    if(pDraw->type != DRAWABLE_PIXMAP)
    {
	sx += ((WindowPtr) pDraw)->absCorner.x;
	sy += ((WindowPtr) pDraw)->absCorner.y;
    }
    widthInBytes = PixmapBytePad(w, 1);
    if(!result)
        result = (unsigned long *)Xalloc(h * widthInBytes);
    bitsPerPixel = GetBitsPerPixel(depth);
    bzero(result, h * widthInBytes);
    if(BITMAP_SCANLINE_UNIT == 8)
	pCharsOut = (unsigned char *) result;
    else if(BITMAP_SCANLINE_UNIT == 16)
	pShortsOut = (CARD16 *) result;
    else if(BITMAP_SCANLINE_UNIT == 32)
	pLongsOut = (CARD32 *) result;
    if(bitsPerPixel == 1)
	pCharsOut = (unsigned char *) result;
    for(i = sy; i < sy + h; i++)
    {
	if(bitsPerPixel == 1)
	{
	    pt.x = sx;
	    pt.y = i;
	    width = w;
            pline = (*pDraw->pScreen->GetSpans)(pDraw, width, &pt, &width, 1);	
	    bcopy(pline, pCharsOut, (w + 7)/8);
	    pCharsOut += widthInBytes;
	    Xfree(pline);
	}
	else
	{
	    k = 0;
	    for(j = 0; j < w; j++)
	    {
		pt.x = sx + j;
		pt.y = i;
		width = 1;
		/* Fetch the next pixel */
		pline = (*pDraw->pScreen->GetSpans)(pDraw, width, &pt,
		                                   &width, 1);
		bit = (unsigned int) ((*pline >> planeNum) & 1);
		/* Now insert that bit into a bitmap in XY format */
	        if(BITMAP_BIT_ORDER == LSBFirst)
		    bit <<= k;
		else
		    bit <<= ((BITMAP_SCANLINE_UNIT - 1) - k);
		if(BITMAP_SCANLINE_UNIT == 8)
		{
		    *pCharsOut |= (unsigned char) bit;
		    k++;
		    if (k == 8)
		    {
		        pCharsOut++;
		        k = 0;
		    }
		}
		else if(BITMAP_SCANLINE_UNIT == 16)
		{
		    *pShortsOut |= (CARD16) bit;
		    k++;
		    if (k == 16)
		    {
		        pShortsOut++;
		        k = 0;
		    }
		}
		if(BITMAP_SCANLINE_UNIT == 32)
		{
		    *pLongsOut |= (CARD32) bit;
		    k++;
		    if (k == 32)
		    {
		        pLongsOut++;
		        k = 0;
		    }
		}
	        Xfree(pline);
	    }

	}
    }
    return(result);    

}

/* GETBITSPERPIXEL -- Find out how many bits per pixel are supported at
 * this depth -- another helper function 
 */
static
int
GetBitsPerPixel(depth)
    int		depth;
{
    int 	i;

    for(i = 0; i < screenInfo.numPixmapFormats; i++)
    {
        if(screenInfo.formats[i].depth == depth)
	{
	    return (screenInfo.formats[i].bitsPerPixel);
	}
    }
    return(1);
}


/* MIOPQSTIPDRAWABLE -- use pbits as an opaque stipple for pDraw.
 * Drawing through the clip mask we SetSpans() the bits into a 
 * bitmap and stipple those bits onto the destination drawable by doing a
 * PolyFillRect over the whole drawable, 
 * then we invert the bitmap by copying it onto itself with an alu of
 * GXinvert, invert the foreground/background colors of the gc, and draw
 * the background bits.
 * Note how the clipped out bits of the bitmap are always the background
 * color so that the stipple never causes FillRect to draw them.
 */
void
miOpqStipDrawable(pDraw, pGC, prgnSrc, pbits, srcx, w, h, dstx, dsty)
    DrawablePtr pDraw;
    GCPtr	pGC;
    RegionPtr	prgnSrc;
    unsigned long	*pbits;
    int		srcx, w, h, dstx, dsty;
{
    int		oldfill, i;
    unsigned long oldfg;
    int		*pwidth, *pwidthFirst;
    long	gcv[6];
    PixmapPtr	pStipple, pPixmap;
    DDXPointRec	oldOrg;
    GCPtr	pGCT;
    DDXPointPtr ppt, pptFirst;
    xRectangle rect;
    RegionPtr	prgnSrcClip;

    pPixmap = (PixmapPtr)(*pDraw->pScreen->CreatePixmap)
			   (pDraw->pScreen, w, h, 1);

    if (!pPixmap)
    {
	ErrorF( "miOpqStipDrawable can't make temp pixmap\n");
	return;
    }

    /* Put the image into a 1 bit deep pixmap */
    pGCT = GetScratchGC(1, pDraw->pScreen);
    /* First set the whole pixmap to 0 */
    gcv[0] = 0;
    DoChangeGC(pGCT, GCBackground, gcv, 0);
    ValidateGC(pPixmap, pGCT);
    miClearDrawable(pPixmap, pGCT);
    ppt = pptFirst = (DDXPointPtr)ALLOCATE_LOCAL(h * sizeof(DDXPointRec));
    pwidth = pwidthFirst = (int *)ALLOCATE_LOCAL(h * sizeof(int));
    if(!ppt || !pwidth)
    {
	DEALLOCATE_LOCAL(pwidthFirst);
	DEALLOCATE_LOCAL(pptFirst);
	FreeScratchGC(pGCT);
	return;
    }

    /* we need a temporary region because ChangeClip must be assumed
       to destroy what it's sent.  note that this means we don't
       have to free prgnSrcClip ourselves.
    */
    prgnSrcClip = (*pGCT->pScreen->RegionCreate)(NULL, 0);
    (*pGCT->pScreen->RegionCopy)(prgnSrcClip, prgnSrc);
    (*pGCT->ChangeClip)(pGCT, CT_REGION, prgnSrcClip, 0);
    ValidateGC(pPixmap, pGCT);

    /* Since we know pDraw is always a pixmap, we never need to think
     * about translation here */
    for(i = 0; i < h; i++)
    {
	ppt->x = srcx;
	ppt++->y = i;
	*pwidth++ = w;
    }

    (*pGCT->SetSpans)(pPixmap, pGCT, pbits, pptFirst, pwidthFirst, h, TRUE);
    DEALLOCATE_LOCAL(pwidthFirst);
    DEALLOCATE_LOCAL(pptFirst);


    /* Save current values from the client GC */
    oldfill = pGC->fillStyle;
    pStipple = pGC->stipple;
    if(pStipple)
        pStipple->refcnt++;
    oldOrg = pGC->patOrg;

    /* Set a new stipple in the drawable */
    gcv[0] = FillStippled;
    gcv[1] = (long) pPixmap;
    gcv[2] = dstx;
    gcv[3] = dsty;

    DoChangeGC(pGC,
             GCFillStyle | GCStipple | GCTileStipXOrigin | GCTileStipYOrigin,
	     gcv, 1);
    ValidateGC(pDraw, pGC);

    /* Fill the drawable with the stipple.  This will draw the
     * foreground color whereever 1 bits are set, leaving everything
     * with 0 bits untouched.  Note that the part outside the clip
     * region is all 0s.  */
    rect.x = dstx;
    rect.y = dsty;
    rect.width = w;
    rect.height = h;
    (*pGC->PolyFillRect)(pDraw, pGC, 1, &rect);

    /* Invert the tiling pixmap. This sets 0s for 1s and 1s for 0s, only
     * within the clipping region, the part outside is still all 0s */
    gcv[0] = GXinvert;
    DoChangeGC(pGCT, GCFunction, gcv, 0);
    ValidateGC(pPixmap, pGCT);
    (*pGCT->CopyArea)(pPixmap, pPixmap, pGCT, 0, 0, w, h, 0, 0);

    /* Swap foreground and background colors on the GC for the drawable.
     * Now when we fill the drawable, we will fill in the "Background"
     * values */
    oldfg = pGC->fgPixel;
    gcv[0] = (long) pGC->bgPixel;
    gcv[1] = (long) oldfg;
    gcv[2] = (long) pPixmap;
    DoChangeGC(pGC, GCForeground | GCBackground | GCStipple, gcv, 1); 
    ValidateGC(pDraw, pGC);
    /* PolyFillRect might have bashed the rectangle */
    rect.x = dstx;
    rect.y = dsty;
    rect.width = w;
    rect.height = h;
    (*pGC->PolyFillRect)(pDraw, pGC, 1, &rect);

    /* Now put things back */
    if(pStipple)
        pStipple->refcnt--;
    gcv[0] = (long) oldfg;
    gcv[1] = pGC->fgPixel;
    gcv[2] = oldfill;
    gcv[3] = (long) pStipple;
    gcv[4] = oldOrg.x;
    gcv[5] = oldOrg.y;
    DoChangeGC(pGC, 
        GCForeground | GCBackground | GCFillStyle | GCStipple | 
	GCTileStipXOrigin | GCTileStipYOrigin, gcv, 1);

    ValidateGC(pDraw, pGC);
    /* put what we hope is a smaller clip region back in the scratch gc */
    (*pGCT->ChangeClip)(pGCT, CT_NONE, 0, 0);
    FreeScratchGC(pGCT);
    (*pDraw->pScreen->DestroyPixmap)(pPixmap);

}

/* MICOPYPLANE -- public entry for the CopyPlane request.
 * strategy: 
 * First build up a bitmap out of the bits requested 
 * build a source clip
 * Use the bitmap we've built up as a Stipple for the destination 
 */
void
miCopyPlane(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty, bitPlane)
    DrawablePtr 	pSrcDrawable;
    DrawablePtr		pDstDrawable;
    GCPtr		pGC;
    int 		srcx, srcy;
    int 		width, height;
    int 		dstx, dsty;
    unsigned long	bitPlane;
{
    unsigned long	*ptile;
    BoxRec 		box;
    RegionPtr		prgnSrc;



    /* clip the left and top edges of the source */
    if (srcx < 0)
    {
        width += srcx;
        srcx = 0;
    }
    if (srcy < 0)
    {
        height += srcy;
        srcy = 0;
    }

    /* incorporate the source clip */

    if (pSrcDrawable->type != DRAWABLE_PIXMAP)
    {
        box.x1 = ((WindowPtr)pSrcDrawable)->absCorner.x;
        box.y1 = ((WindowPtr)pSrcDrawable)->absCorner.y;
        box.x2 = box.x1 + width;
        box.y2 = box.y1 + height;
        prgnSrc = (*pGC->pScreen->RegionCreate)(&box, 1);
	(*pGC->pScreen->Intersect)
	    (prgnSrc, prgnSrc, ((WindowPtr)pSrcDrawable)->clipList);
	(*pGC->pScreen->TranslateRegion)(prgnSrc,
			-((WindowPtr)pSrcDrawable)->absCorner.x,
			-((WindowPtr)pSrcDrawable)->absCorner.y);
    }
    else
    {
        box.x1 = 0;
        box.y1 = 0;
        box.x2 = ((PixmapPtr)pSrcDrawable)->width;
        box.y2 = ((PixmapPtr)pSrcDrawable)->height;
        prgnSrc = (*pGC->pScreen->RegionCreate)(&box, 1);
    }

    /* note that we convert the plane mask bitPlane into a plane number */
    ptile = miGetPlane(pSrcDrawable, ffs(bitPlane) - 1, srcx, srcy,
   		       width, height, (unsigned char *) NULL);
    miOpqStipDrawable(pDstDrawable, pGC, prgnSrc, ptile, 0,
                      width, height, dstx, dsty);
    miHandleExposures(pSrcDrawable, pDstDrawable, pGC, srcx, srcy,
		      width, height, dstx, dsty);
    Xfree(ptile);
    (*pGC->pScreen->RegionDestroy)(prgnSrc);
}

/* MIGETIMAGE -- public entry for the GetImage Request
 * We're getting the image into a memory buffer. While we have to use GetSpans
 * to read a line from the device (since we don't know what that looks like),
 * we can just write into the destination buffer
 *
 * two different strategies are used, depending on whether we're getting the
 * image in Z format or XY format
 * Z format:
 * Line at a time, GetSpans a line and bcopy it to the destination
 * buffer, except that if the planemask is not all ones, we create a
 * temporary pixmap and do a SetSpans into it (to get bits turned off)
 * and then another GetSpans to get stuff back (because pixmaps are
 * opaque, and we are passed in the memory to write into).  This is
 * completely ugly and slow but works, but the interfaces just aren't
 * designed for this case.  Life is hard.
 * XY format:
 * get the single plane specified in planemask
 */
void
miGetImage(pDraw, sx, sy, w, h, format, planeMask, pdstLine)
    DrawablePtr 	pDraw;
    int			sx, sy, w, h;
    unsigned int 	format;
    unsigned long 	planeMask;
    pointer             pdstLine;
{
    int			depth, i, linelength, width, srcx, srcy;
    DDXPointRec		pt;
    unsigned long	*pbits;
    long		gcv[2];
    PixmapPtr		pPixmap = (PixmapPtr)NULL;
    GCPtr		pGC;
    unsigned long *     pDst = (unsigned long *)pdstLine;

    depth = pDraw->depth;
    if(format == ZPixmap)
    {
	if ( (((1<<pDraw->depth)-1)&planeMask) != 
	     (1<<pDraw->depth)-1
	   )
	{
	    pGC = GetScratchGC(depth, pDraw->pScreen);
            pPixmap = (PixmapPtr)(*pDraw->pScreen->CreatePixmap)
			       (pDraw->pScreen, w, h, depth);
	    gcv[0] = GXcopy;
	    gcv[1] = planeMask;
	    DoChangeGC(pGC, GCPlaneMask | GCFunction, gcv, 0);
	    ValidateGC(pPixmap, pGC);
	}

        linelength = PixmapBytePad(w, depth);
	srcx = sx;
	srcy = sy;
	if(pDraw->type == DRAWABLE_WINDOW)
	{
	    srcx += ((WindowPtr)pDraw)->absCorner.x;
	    srcy += ((WindowPtr)pDraw)->absCorner.y;
	}
	for(i = 0; i < h; i++)
	{
	    pt.x = srcx;
	    pt.y = srcy + i;
	    width = w;
            pbits = (unsigned long *)
	        (*pDraw->pScreen->GetSpans)(pDraw, w, &pt, &width, 1);
	    if (pPixmap)
	    {
	       pt.x = 0;
	       pt.y = 0;
	       width = w;
	       (*pGC->SetSpans)(pPixmap, pGC, pbits, &pt, &width, 1, TRUE);
	       Xfree(pbits);
	       pbits = (unsigned long *)
		  (*pDraw->pScreen->GetSpans)(pPixmap, w, &pt, &width, 1);
	    }
	    bcopy(pbits, (char *)pDst, linelength);
	    pDst += linelength / sizeof(long);
	    Xfree(pbits);
	}
	if (pPixmap)
	{
	    (*pGC->pScreen->DestroyPixmap)(pPixmap);
	    FreeScratchGC(pGC);
	}
    }
    else
    {
	miGetPlane(pDraw, ffs(planeMask) - 1, sx, sy, w, h, pDst);
    }
}


/* MIPUTIMAGE -- public entry for the PutImage request
 * Here we benefit from knowing the format of the bits pointed to by pImage,
 * even if we don't know how pDraw represents them.  
 * Three different strategies are used depending on the format 
 * XYBitmap Format:
 * 	we just use the Opaque Stipple helper function to cover the destination
 * 	Note that this covers all the planes of the drawable with the 
 *	foreground color (masked with the GC planemask) where there are 1 bits
 *	and the background color (masked with the GC planemask) where there are
 *	0 bits
 * XYPixmap format:
 *	what we're called with is a series of XYBitmaps, but we only want 
 *	each XYPixmap to update 1 plane, instead of updating all of them.
 * 	we set the foreground color to be all 1s and the background to all 0s
 *	then for each plane, we set the plane mask to only effect that one
 *	plane and recursive call ourself with the format set to XYBitmap
 *	(This clever idea courtesy of RGD.)
 * ZPixmap format:
 *	This part is simple, just call SetSpans
 */
void
miPutImage(pDraw, pGC, depth, x, y, w, h, leftPad, format, pImage)
    DrawablePtr		pDraw;
    GCPtr		pGC;
    int 		depth, x, y, w, h, leftPad;
    unsigned int	format;
    unsigned char	*pImage;
{
    DDXPointPtr		pptFirst, ppt;
    int			*pwidthFirst, *pwidth, i;
    RegionPtr		prgnSrc;
    BoxRec		box;
    unsigned long	oldFg, oldBg, gcv[3];
    unsigned long	oldPlanemask;

    switch(format)
    {
      case XYBitmap:

	box.x1 = 0;
	box.y1 = 0;
	box.x2 = w;
	box.y2 = h;
	prgnSrc = (*pGC->pScreen->RegionCreate)(&box, 1);

        miOpqStipDrawable(pDraw, pGC, prgnSrc, pImage, leftPad, 
                          (w - leftPad), h, x, y);
	(*pGC->pScreen->RegionDestroy)(prgnSrc);
	break;

      case XYPixmap:
	depth = pGC->depth;
	oldPlanemask = pGC->planemask;
	oldFg = pGC->fgPixel;
	oldBg = pGC->bgPixel;
	gcv[0] = ~0;
	gcv[1] = 0;
	DoChangeGC(pGC, GCForeground | GCBackground, gcv, 0);

	for (i = 1 << (depth-1); i > 0; i >>= 1)
	{
	    if (i & oldPlanemask)
	    {
	        gcv[0] = i;
	        DoChangeGC(pGC, GCPlaneMask, gcv, 0);
	        ValidateGC(pDraw, pGC);
	        (*pGC->PutImage)(pDraw, pGC, 1, x, y, w, h, leftPad,
			         XYBitmap, pImage);
	        pImage += h * PixmapBytePad(w, 1);
	    }
	}
	gcv[0] = oldPlanemask;
	gcv[1] = oldFg;
	gcv[2] = oldBg;
	DoChangeGC(pGC, GCPlaneMask | GCForeground | GCBackground, gcv, 0);
	break;

      case ZPixmap:
    	ppt = pptFirst = (DDXPointPtr)ALLOCATE_LOCAL(h * sizeof(DDXPointRec));
    	pwidth = pwidthFirst = (int *)ALLOCATE_LOCAL(h * sizeof(int));
	if(!ppt || !pwidth)
        {
           if (ppt)
               DEALLOCATE_LOCAL(ppt);
           else if (pwidth)
               DEALLOCATE_LOCAL(pwidthFirst);
           return;
        }
	if ((pDraw->type == DRAWABLE_WINDOW) &&
	    (pGC->miTranslate))
	{
	    x += ((WindowPtr)(pDraw))->absCorner.x;
	    y += ((WindowPtr)(pDraw))->absCorner.y;
	}

	for(i = 0; i < h; i++)
	{
	    ppt->x = x;
	    ppt->y = y + i;
	    ppt++;
	    *pwidth++ = w;
	}

	(*pGC->SetSpans)(pDraw, pGC, pImage, pptFirst, pwidthFirst, h, TRUE);
	DEALLOCATE_LOCAL(pptFirst);
	DEALLOCATE_LOCAL(pwidthFirst);
	break;
    }
}

/* MICLEARDRAWABLE -- sets the entire drawable to the background color of
 * the GC.  Useful when we have a scratch drawable and need to initialize 
 * it. */
miClearDrawable(pDraw, pGC)
    DrawablePtr	pDraw;
    GCPtr	pGC;
{
    int     fg = pGC->fgPixel;
    int	    bg = pGC->bgPixel;
    xRectangle rect;

    rect.x = 0;
    rect.y = 0;
    if(pDraw->type == DRAWABLE_PIXMAP)
    {
	rect.width = ((PixmapPtr) pDraw)->width;
	rect.height = ((PixmapPtr) pDraw)->height;
    }
    else
    {
	rect.width = ((WindowPtr) pDraw)->clientWinSize.width;
	rect.height = ((WindowPtr) pDraw)->clientWinSize.height;
    }
    DoChangeGC(pGC, GCForeground, &bg, 0);
    ValidateGC(pDraw, pGC);
    (*pGC->PolyFillRect)(pDraw, pGC, 1, &rect);
    DoChangeGC(pGC, GCForeground, &fg, 0);
    ValidateGC(pDraw, pGC);
}
