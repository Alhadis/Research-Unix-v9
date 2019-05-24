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
/* $Header: mfbbitblt.c,v 1.42 87/08/09 13:58:52 ham Exp $ */
#include "X.h"
#include "Xprotostr.h"

#include "miscstruct.h"
#include "regionstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "mfb.h"
#include "maskbits.h"


/* CopyArea and CopyPlane for a monchrome frame buffer


    clip the source rectangle to the source's available bits.  (this
avoids copying unnecessary pieces that will just get exposed anyway.)
this becomes the new shape of the destination.
    clip the destination region to the composite clip in the
GC.  this requires translating the destination region to (dstx, dsty).
    build a list of source points, one for each rectangle in the
destination.  this is a simple translation.
    go do the multiple rectangle copies
    do graphics exposures
*/

void 
mfbCopyArea(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty)
register DrawablePtr pSrcDrawable;
register DrawablePtr pDstDrawable;
GC *pGC;
int srcx, srcy;
int width, height;
int dstx, dsty;
{
    BoxRec srcBox;
    RegionPtr prgnSrcClip;	/* may be a new region, or just a copy */
    int realSrcClip = 0;	/* non-0 if we've created a src clip */

    RegionPtr prgnDst;
    DDXPointPtr pptSrc;
    register DDXPointPtr ppt;
    register BoxPtr pbox;
    int i;
    register int dx;
    register int dy;
    xRectangle origSource;
    DDXPointRec origDest;

    if (!(pGC->planemask & 1))
	return;

    origSource.x = srcx;
    origSource.y = srcy;
    origSource.width = width;
    origSource.height = height;
    origDest.x = dstx;
    origDest.y = dsty;

    /*
       clip the left and top edges of the source
    */
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

    /* clip the source */

    if (pSrcDrawable->type == DRAWABLE_PIXMAP)
    {
	BoxRec box;

	box.x1 = 0;
	box.y1 = 0;
	box.x2 = ((PixmapPtr)pSrcDrawable)->width;
	box.y2 = ((PixmapPtr)pSrcDrawable)->height;

	prgnSrcClip = miRegionCreate(&box, 1);
	realSrcClip = 1;
    }
    else
    {
	srcx += ((WindowPtr)pSrcDrawable)->absCorner.x;
	srcy += ((WindowPtr)pSrcDrawable)->absCorner.y;
	prgnSrcClip = ((WindowPtr)pSrcDrawable)->clipList;
    }

    srcBox.x1 = srcx;
    srcBox.y1 = srcy;
    srcBox.x2 = srcx + width;
    srcBox.y2 = srcy + height;

    prgnDst = miRegionCreate(&srcBox, 1);
    miIntersect(prgnDst, prgnDst, prgnSrcClip);

    if (pDstDrawable->type == DRAWABLE_WINDOW)
    {
	if (!((WindowPtr)pDstDrawable)->realized)
	{
	    miSendNoExpose(pGC);
	    miRegionDestroy(prgnDst);
	    if (realSrcClip)
		miRegionDestroy(prgnSrcClip);
	    return;
	}
	dstx += ((WindowPtr)pDstDrawable)->absCorner.x;
	dsty += ((WindowPtr)pDstDrawable)->absCorner.y;
    }

    dx = srcx - dstx;
    dy = srcy - dsty;

    /* clip the shape of the dst to the destination composite clip */
    miTranslateRegion(prgnDst, -dx, -dy);
    miIntersect(prgnDst,
		prgnDst,
		((mfbPrivGC *)(pGC->devPriv))->pCompositeClip);

    if (!prgnDst->numRects)
    {
        miSendNoExpose(pGC);
	miRegionDestroy(prgnDst);
	if (realSrcClip)
	    miRegionDestroy(prgnSrcClip);
	return;
    }
    if(!(pptSrc = (DDXPointPtr)ALLOCATE_LOCAL( prgnDst->numRects *
        sizeof(DDXPointRec))))
    {
	miRegionDestroy(prgnDst);
	if (realSrcClip)
	    miRegionDestroy(prgnSrcClip);
	return;
    }
    pbox = prgnDst->rects;
    ppt = pptSrc;
    for (i=0; i<prgnDst->numRects; i++, pbox++, ppt++)
    {
	ppt->x = pbox->x1 + dx;
	ppt->y = pbox->y1 + dy;
    }

    mfbDoBitblt(pSrcDrawable, pDstDrawable, pGC->alu, prgnDst, pptSrc);

    if (((mfbPrivGC *)(pGC->devPriv))->fExpose)
        miHandleExposures(pSrcDrawable, pDstDrawable, pGC,
		          origSource.x, origSource.y,
		          origSource.width, origSource.height,
		          origDest.x, origDest.y);
		
    DEALLOCATE_LOCAL(pptSrc);
    miRegionDestroy(prgnDst);
    if (realSrcClip)
	miRegionDestroy(prgnSrcClip);
}

/* DoBitblt() does multiple rectangle moves into the rectangles
   DISCLAIMER:
   this code can be made much faster; this implementation is
designed to be independent of byte/bit order, processor
instruction set, and the like.  it could probably be done
in a similarly device independent way using mask tables instead
of the getbits/putbits macros.  the narrow case (w<32) can be
subdivided into a case that crosses word boundaries and one that
doesn't.

   we have to cope with the dircetion on a per band basis,
rather than a per rectangle basis.  moving bottom to top
means we have to invert the order of the bands; moving right
to left requires reversing the order of the rectangles in
each band.

   if src or dst is a window, the points have already been
translated.
*/

mfbDoBitblt(pSrcDrawable, pDstDrawable, alu, prgnDst, pptSrc)
DrawablePtr pSrcDrawable;
DrawablePtr pDstDrawable;
int alu;
RegionPtr prgnDst;
DDXPointPtr pptSrc;
{
    unsigned int *psrcBase, *pdstBase;	
				/* start of src and dst bitmaps */
    int widthSrc, widthDst;	/* add to get to same position in next line */

    register BoxPtr pbox;
    int nbox;

    BoxPtr pboxTmp, pboxNext, pboxBase, pboxNew1, pboxNew2;
				/* temporaries for shuffling rectangles */
    DDXPointPtr pptTmp, pptNew1, pptNew2;
				/* shuffling boxes entails shuffling the
				   source points too */
    int w, h;
    int xdir;			/* 1 = left right, -1 = right left/ */
    int ydir;			/* 1 = top down, -1 = bottom up */

    unsigned int *psrcLine, *pdstLine;	
				/* pointers to line with current src and dst */
    register unsigned int *psrc;/* pointer to current src longword */
    register unsigned int *pdst;/* pointer to current dst longword */

				/* following used for looping through a line */
    int startmask, endmask;	/* masks for writing ends of dst */
    int nlMiddle;		/* whole longwords in dst */
    register int nl;		/* temp copy of nlMiddle */
    register unsigned int tmpSrc;
				/* place to store full source word */
    register int xoffSrc;	/* offset (>= 0, < 32) from which to
			           fetch whole longwords fetched 
				   in src */
    int nstart;			/* number of ragged bits at start of dst */
    int nend;			/* number of ragged bits at end of dst */
    int srcStartOver;		/* pulling nstart bits from src
				   overflows into the next word? */


    if (pSrcDrawable->type == DRAWABLE_WINDOW)
    {
	psrcBase = (unsigned int *)
		(((PixmapPtr)(pSrcDrawable->pScreen->devPrivate))->devPrivate);
	widthSrc = (int)
		   ((PixmapPtr)(pSrcDrawable->pScreen->devPrivate))->devKind
		    >> 2;
    }
    else
    {
	psrcBase = (unsigned int *)(((PixmapPtr)pSrcDrawable)->devPrivate);
	widthSrc = (int)(((PixmapPtr)pSrcDrawable)->devKind) >> 2;
    }

    if (pDstDrawable->type == DRAWABLE_WINDOW)
    {
	pdstBase = (unsigned int *)
		(((PixmapPtr)(pDstDrawable->pScreen->devPrivate))->devPrivate);
	widthDst = (int)
		   ((PixmapPtr)(pDstDrawable->pScreen->devPrivate))->devKind
		    >> 2;
    }
    else
    {
	pdstBase = (unsigned int *)(((PixmapPtr)pDstDrawable)->devPrivate);
	widthDst = (int)(((PixmapPtr)pDstDrawable)->devKind) >> 2;
    }

    pbox = prgnDst->rects;
    nbox = prgnDst->numRects;

    pboxNew1 = 0;
    pptNew1 = 0;
    pboxNew2 = 0;
    pptNew2 = 0;
    if (pptSrc->y < pbox->y1) 
    {
        /* walk source botttom to top */
	ydir = -1;
	widthSrc = -widthSrc;
	widthDst = -widthDst;

	if (nbox > 1)
	{
	    /* keep ordering in each band, reverse order of bands */
	    pboxNew1 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    pptNew1 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pboxNew1 || !pptNew1)
	    {
	        DEALLOCATE_LOCAL(pptNew1);
	        DEALLOCATE_LOCAL(pboxNew1);
	        return;
	    }
	    pboxBase = pboxNext = pbox+nbox-1;
	    while (pboxBase >= pbox)
	    {
	        while ((pboxNext >= pbox) && 
		       (pboxBase->y1 == pboxNext->y1))
		    pboxNext--;
	        pboxTmp = pboxNext+1;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp <= pboxBase)
	        {
		    *pboxNew1++ = *pboxTmp++;
		    *pptNew1++ = *pptTmp++;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew1 -= nbox;
	    pbox = pboxNew1;
	    pptNew1 -= nbox;
	    pptSrc = pptNew1;
        }
    }
    else
    {
	/* walk source top to bottom */
	ydir = 1;
    }

    if (pptSrc->x < pbox->x1)
    {
	/* walk source right to left */
        xdir = -1;

	if (nbox > 1)
	{
	    /* reverse order of rects in each band */
	    pboxNew2 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    pptNew2 = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    pboxBase = pboxNext = pbox;
	    if(!pboxNew2 || !pptNew2)
	    {
	        DEALLOCATE_LOCAL(pptNew2);
	        DEALLOCATE_LOCAL(pboxNew2);
	        return;
	    }
	    while (pboxBase < pbox+nbox)
	    {
	        while ((pboxNext < pbox+nbox) &&
		       (pboxNext->y1 == pboxBase->y1))
		    pboxNext++;
	        pboxTmp = pboxNext;
	        pptTmp = pptSrc + (pboxTmp - pbox);
	        while (pboxTmp != pboxBase)
	        {
		    *pboxNew2++ = *--pboxTmp;
		    *pptNew2++ = *--pptTmp;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew2 -= nbox;
	    pbox = pboxNew2;
	    pptNew2 -= nbox;
	    pptSrc = pptNew2;
	}
    }
    else
    {
	/* walk source left to right */
        xdir = 1;
    }


    /* special case copy */
    if (alu == GXcopy)
    {
        while (nbox--)
        {
	    w = pbox->x2 - pbox->x1;
	    h = pbox->y2 - pbox->y1;

	    if (ydir == -1) /* start at last scanline of rectangle */
	    {
	        psrcLine = psrcBase + ((pptSrc->y+h-1) * -widthSrc);
	        pdstLine = pdstBase + ((pbox->y2-1) * -widthDst);
	    }
	    else /* start at first scanline */
	    {
	        psrcLine = psrcBase + (pptSrc->y * widthSrc);
	        pdstLine = pdstBase + (pbox->y1 * widthDst);
	    }

	    /* x direction doesn't matter for < 1 longword */
	    if (w <= 32)
	    {
	        int srcBit, dstBit;	/* bit offset of src and dst */

	        pdstLine += (pbox->x1 >> 5);
	        psrcLine += (pptSrc->x >> 5);
	        psrc = psrcLine;
	        pdst = pdstLine;

	        srcBit = pptSrc->x & 0x1f;
	        dstBit = pbox->x1 & 0x1f;

	        while(h--)
	        {
		    getbits(psrc, srcBit, w, tmpSrc)
		    putbits(tmpSrc, dstBit, w, pdst)
		    pdst += widthDst;
		    psrc += widthSrc;
	        }
	    }
	    else
	    {
	        maskbits(pbox->x1, w, startmask, endmask, nlMiddle)
	        if (startmask)
		    nstart = 32 - (pbox->x1 & 0x1f);
	        else
		    nstart = 0;
	        if (endmask)
	            nend = pbox->x2 & 0x1f;
	        else
		    nend = 0;

	        xoffSrc = ((pptSrc->x & 0x1f) + nstart) & 0x1f;
	        srcStartOver = ((pptSrc->x & 0x1f) + nstart) > 31;

	        if (xdir == 1) /* move left to right */
	        {
	            pdstLine += (pbox->x1 >> 5);
	            psrcLine += (pptSrc->x >> 5);

		    while (h--)
		    {
		        psrc = psrcLine;
		        pdst = pdstLine;

		        if (startmask)
		        {
			    getbits(psrc, (pptSrc->x & 0x1f), nstart, tmpSrc)
			    putbits(tmpSrc, (pbox->x1 & 0x1f), nstart, pdst)
			    pdst++;
			    if (srcStartOver)
			        psrc++;
		        }

		        nl = nlMiddle;
		        while (nl--)
		        {
			    getbits(psrc, xoffSrc, 32, tmpSrc)
			    *pdst++ = tmpSrc;
			    psrc++;
		        }

		        if (endmask)
		        {
			    getbits(psrc, xoffSrc, nend, tmpSrc)
			    putbits(tmpSrc, 0, nend, pdst)
		        }

		        pdstLine += widthDst;
		        psrcLine += widthSrc;
		    }
	        }
	        else /* move right to left */
	        {
	            pdstLine += (pbox->x2 >> 5);
	            psrcLine += (pptSrc->x+w >> 5);
		    /* if fetch of last partial bits from source crosses
		       a longword boundary, start at the previous longword
		    */
		    if (xoffSrc + nend >= 32)
		        --psrcLine;

		    while (h--)
		    {
		        psrc = psrcLine;
		        pdst = pdstLine;

		        if (endmask)
		        {
			    getbits(psrc, xoffSrc, nend, tmpSrc)
			    putbits(tmpSrc, 0, nend, pdst)
		        }

		        nl = nlMiddle;
		        while (nl--)
		        {
			    --psrc;
			    getbits(psrc, xoffSrc, 32, tmpSrc)
			    *--pdst = tmpSrc;
		        }

		        if (startmask)
		        {
			    if (srcStartOver)
			        --psrc;
			    --pdst;
			    getbits(psrc, (pptSrc->x & 0x1f), nstart, tmpSrc)
			    putbits(tmpSrc, (pbox->x1 & 0x1f), nstart, pdst)
		        }

		        pdstLine += widthDst;
		        psrcLine += widthSrc;
		    }
	        } /* move right to left */
	    }
	    pbox++;
	    pptSrc++;
        } /* while (nbox--) */
    }
    else /* do some rop */
    {
        while (nbox--)
        {
	    w = pbox->x2 - pbox->x1;
	    h = pbox->y2 - pbox->y1;

	    if (ydir == -1) /* start at last scanline of rectangle */
	    {
	        psrcLine = psrcBase + ((pptSrc->y+h-1) * -widthSrc);
	        pdstLine = pdstBase + ((pbox->y2-1) * -widthDst);
	    }
	    else /* start at first scanline */
	    {
	        psrcLine = psrcBase + (pptSrc->y * widthSrc);
	        pdstLine = pdstBase + (pbox->y1 * widthDst);
	    }

	    /* x direction doesn't matter for < 1 longword */
	    if (w <= 32)
	    {
	        int srcBit, dstBit;	/* bit offset of src and dst */

	        pdstLine += (pbox->x1 >> 5);
	        psrcLine += (pptSrc->x >> 5);
	        psrc = psrcLine;
	        pdst = pdstLine;

	        srcBit = pptSrc->x & 0x1f;
	        dstBit = pbox->x1 & 0x1f;

	        while(h--)
	        {
		    getbits(psrc, srcBit, w, tmpSrc)
		    putbitsrop(tmpSrc, dstBit, w, pdst, alu)
		    pdst += widthDst;
		    psrc += widthSrc;
	        }
	    }
	    else
	    {
	        maskbits(pbox->x1, w, startmask, endmask, nlMiddle)
	        if (startmask)
		    nstart = 32 - (pbox->x1 & 0x1f);
	        else
		    nstart = 0;
	        if (endmask)
	            nend = pbox->x2 & 0x1f;
	        else
		    nend = 0;

	        xoffSrc = ((pptSrc->x & 0x1f) + nstart) & 0x1f;
	        srcStartOver = ((pptSrc->x & 0x1f) + nstart) > 31;

	        if (xdir == 1) /* move left to right */
	        {
	            pdstLine += (pbox->x1 >> 5);
	            psrcLine += (pptSrc->x >> 5);

		    while (h--)
		    {
		        psrc = psrcLine;
		        pdst = pdstLine;

		        if (startmask)
		        {
			    getbits(psrc, (pptSrc->x & 0x1f), nstart, tmpSrc)
			    putbitsrop(tmpSrc, (pbox->x1 & 0x1f), nstart, pdst,
				       alu)
			    pdst++;
			    if (srcStartOver)
			        psrc++;
		        }

		        nl = nlMiddle;
		        while (nl--)
		        {
			    getbits(psrc, xoffSrc, 32, tmpSrc)
			    *pdst = DoRop(alu, tmpSrc, *pdst);
			    pdst++;
			    psrc++;
		        }

		        if (endmask)
		        {
			    getbits(psrc, xoffSrc, nend, tmpSrc)
			    putbitsrop(tmpSrc, 0, nend, pdst, alu)
		        }

		        pdstLine += widthDst;
		        psrcLine += widthSrc;
		    }
	        }
	        else /* move right to left */
	        {
	            pdstLine += (pbox->x2 >> 5);
	            psrcLine += (pptSrc->x+w >> 5);
		    /* if fetch of last partial bits from source crosses
		       a longword boundary, start at the previous longword
		    */
		    if (xoffSrc + nend >= 32)
		        --psrcLine;

		    while (h--)
		    {
		        psrc = psrcLine;
		        pdst = pdstLine;

		        if (endmask)
		        {
			    getbits(psrc, xoffSrc, nend, tmpSrc)
			    putbitsrop(tmpSrc, 0, nend, pdst, alu)
		        }

		        nl = nlMiddle;
		        while (nl--)
		        {
			    --psrc;
			    --pdst;
			    getbits(psrc, xoffSrc, 32, tmpSrc)
			    *pdst = DoRop(alu, tmpSrc, *pdst);
		        }

		        if (startmask)
		        {
			    if (srcStartOver)
			        --psrc;
			    --pdst;
			    getbits(psrc, (pptSrc->x & 0x1f), nstart, tmpSrc)
			    putbitsrop(tmpSrc, (pbox->x1 & 0x1f), nstart, pdst,
				       alu)
		        }

		        pdstLine += widthDst;
		        psrcLine += widthSrc;
		    }
	        } /* move right to left */
	    }
	    pbox++;
	    pptSrc++;
        } /* while (nbox--) */
    }

    /* free up stuff */
    if (pptNew1)
    {
	DEALLOCATE_LOCAL(pptNew1);
    }
    if (pboxNew1)
    {
	DEALLOCATE_LOCAL(pboxNew1);
    }
    if (pptNew2)
    {
	DEALLOCATE_LOCAL(pptNew2);
    }
    if (pboxNew2)
    {
	DEALLOCATE_LOCAL(pboxNew2);
    }
}


/*
    if fg == 1 and bg ==0, we can do an ordinary CopyArea.
    if fg == bg, we can do a CopyArea with alu = ReduceRop(alu, fg)
    if fg == 0 and bg == 1, we use the same rasterop, with
	source operand inverted.

    CopyArea deals with all of the graphics exposure events.
    This code depends on knowing that we can change the
alu in the GC without having to call ValidateGC() before calling
CopyArea().

*/

void
mfbCopyPlane(pSrcDrawable, pDstDrawable,
	    pGC, srcx, srcy, width, height, dstx, dsty, plane)
DrawablePtr pSrcDrawable, pDstDrawable;
register GC *pGC;
int srcx, srcy;
int width, height;
int dstx, dsty;
unsigned int plane;
{
    int alu;

    if (!(pGC->planemask & 1))
	return;

    if (plane != 1)
	return;

    if ((pGC->fgPixel == 1) && (pGC->bgPixel == 0))
    {
	(*pGC->CopyArea)(pSrcDrawable, pDstDrawable,
			 pGC, srcx, srcy, width, height, dstx, dsty);
    }
    else if (pGC->fgPixel == pGC->bgPixel)
    {
	alu = pGC->alu;
	pGC->alu = ReduceRop(pGC->alu, pGC->fgPixel);
	(*pGC->CopyArea)(pSrcDrawable, pDstDrawable,
			 pGC, srcx, srcy, width, height, dstx, dsty);
	pGC->alu = alu;
    }
    else /* need to invert the src */
    {
	alu = pGC->alu;
	pGC->alu = InverseAlu[alu];
	(*pGC->CopyArea)(pSrcDrawable, pDstDrawable,
			 pGC, srcx, srcy, width, height, dstx, dsty);
	pGC->alu = alu;
    }
}

