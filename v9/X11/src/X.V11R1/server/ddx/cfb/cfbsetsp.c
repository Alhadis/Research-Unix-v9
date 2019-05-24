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

#include "X.h"
#include "Xmd.h"
#include "servermd.h"

#include "misc.h"
#include "regionstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "cfb.h"
#include "cfbmskbits.h"


/* cfbSetScanline -- copies the bits from psrc to the drawable starting at
 * (xStart, y) and continuing to (xEnd, y).  xOrigin tells us where psrc 
 * starts on the scanline. (I.e., if this scanline passes through multiple
 * boxes, we may not want to start grabbing bits at psrc but at some offset
 * further on.) 
 */
cfbSetScanline(y, xOrigin, xStart, xEnd, psrc, alu, pdstBase, widthDst, planemask)
    int			y;
    int			xOrigin;	/* where this scanline starts */
    int			xStart;		/* first bit to use from scanline */
    int			xEnd;		/* last bit to use from scanline + 1 */
    register int	*psrc;
    register int	alu;		/* raster op */
    int			*pdstBase;	/* start of the drawable */
    int			widthDst;	/* width of drawable in words */
    long		planemask;
{
    int			w;		/* width of scanline in bits */
    register int	*pdst;		/* where to put the bits */
    register int	tmpSrc;		/* scratch buffer to collect bits in */
    int			dstBit;		/* offset in bits from beginning of 
					 * word */
    register int	nstart; 	/* number of bits from first partial */
    register int	nend; 		/* " " last partial word */
    int			offSrc;
    int		startmask, endmask, nlMiddle, nl;

    pdst = pdstBase + (y * widthDst) + (xStart >> PWSH); 
    psrc += (xStart - xOrigin) >> PWSH;
    offSrc = (xStart - xOrigin) & PIM;
    w = xEnd - xStart;
    dstBit = xStart & PIM;

    if (dstBit + w <= PPW) 
    { 
	getbits(psrc, offSrc, w, tmpSrc);
	putbitsrop(tmpSrc, dstBit, w, pdst, planemask, alu); 
    } 
    else 
    { 

	maskbits(xStart, w, startmask, endmask, nlMiddle);
	if (startmask) 
	    nstart = PPW - dstBit; 
	else 
	    nstart = 0; 
	if (endmask) 
	    nend = xEnd & PIM; 
	else 
	    nend = 0; 
	if (startmask) 
	{ 
	    getbits(psrc, offSrc, nstart, tmpSrc);
	    putbitsrop(tmpSrc, dstBit, nstart, pdst, planemask, alu);
	    pdst++; 
	    offSrc += nstart;
	    if (offSrc > PLST)
	    {
		psrc++;
		offSrc -= PPW;
	    }
	} 
	nl = nlMiddle; 
	while (nl--) 
	{ 
	    getbits(psrc, offSrc, PPW, tmpSrc);
	    putbitsrop(tmpSrc, 0, PPW, pdst, planemask, alu );
	    pdst++; 
	    psrc++; 
	} 
	if (endmask) 
	{ 
	    getbits(psrc, offSrc, nend, tmpSrc);
	    putbitsrop(tmpSrc, 0, nend, pdst, planemask, alu);
	} 
	 
    } 
}



/* SetSpans -- for each span copy pwidth[i] bits from psrc to pDrawable at
 * ppt[i] using the raster op from the GC.  If fSorted is TRUE, the scanlines
 * are in increasing Y order.
 * Source bit lines are server scanline padded so that they always begin
 * on a word boundary.
 */ 
void
cfbSetSpans(pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted)
    DrawablePtr		pDrawable;
    GCPtr		pGC;
    int			*psrc;
    register DDXPointPtr ppt;
    int			*pwidth;
    int			nspans;
    int			fSorted;
{
    int 		*pdstBase;	/* start of dst bitmap */
    int 		widthDst;	/* width of bitmap in words */
    register BoxPtr 	pbox, pboxLast, pboxTest;
    register DDXPointPtr pptLast;
    int 		alu;
    RegionPtr 		prgnDst;
    int			xStart, xEnd;
    int			yMax;

    switch (pDrawable->depth) {
	case 1:
	    mfbSetSpans(pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted);
	    return;
	case 8:
	    break;
	default:
	    FatalError("cfbSetSpans: invalid depth\n");
    }

    alu = pGC->alu;
    prgnDst = ((cfbPrivGC *)(pGC->devPriv))->pCompositeClip;

    pptLast = ppt + nspans;

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	pdstBase = (int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate);
	widthDst = (int)
		   ((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind
		    >> 2;
	yMax = (int)((WindowPtr)pDrawable)->clientWinSize.height +
		    ((WindowPtr)pDrawable)->absCorner.y;

/* translation should be done by caller of this routine
        pptT = ppt;
        while(pptT < pptLast)
	{
	    pptT->x += ((WindowPtr)pDrawable)->absCorner.x;
	    pptT->y += ((WindowPtr)pDrawable)->absCorner.y;
	    pptT++;
	}
*/
    }
    else
    {
	pdstBase = (int *)(((PixmapPtr)pDrawable)->devPrivate);
	widthDst = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
	yMax = ((PixmapPtr)pDrawable)->height;
    }

    pbox =  prgnDst->rects;
    pboxLast = pbox + prgnDst->numRects;

    if(fSorted)
    {
    /* scan lines sorted in ascending order. Because they are sorted, we
     * don't have to check each scanline against each clip box.  We can be
     * sure that this scanline only has to be clipped to boxes at or after the
     * beginning of this y-band 
     */
	pboxTest = pbox;
	while(ppt < pptLast)
	{
	    pbox = pboxTest;
	    if(ppt->y >= yMax)
		break;
	    while(pbox < pboxLast)
	    {
		if(pbox->y1 > ppt->y)
		{
		    /* scanline is before clip box */
		    break;
		}
		else if(pbox->y2 <= ppt->y)
		{
		    /* clip box is before scanline */
		    pboxTest = ++pbox;
		    continue;
		}
		else if(pbox->x1 > ppt->x + *pwidth) 
		{
		    /* clip box is to right of scanline */
		    break;
		}
		else if(pbox->x2 <= ppt->x)
		{
		    /* scanline is to right of clip box */
		    pbox++;
		    continue;
		}

		/* at least some of the scanline is in the current clip box */
		xStart = max(pbox->x1, ppt->x);
		xEnd = min(ppt->x + *pwidth, pbox->x2);
		cfbSetScanline(ppt->y, ppt->x, xStart, xEnd, psrc, alu,
		    pdstBase, widthDst, pGC->planemask);
		if(ppt->x + *pwidth <= pbox->x2)
		{
		    /* End of the line, as it were */
		    break;
		}
		else
		    pbox++;
	    }
	    /* We've tried this line against every box; it must be outside them
	     * all.  move on to the next point */
	    ppt++;
	    psrc += PixmapWidthInPadUnits(*pwidth, PSZ);
	    pwidth++;
	}
    }
    else
    {
    /* scan lines not sorted. We must clip each line against all the boxes */
	while(ppt < pptLast)
	{
	    if(ppt->y >= 0 && ppt->y < yMax)
	    {
		
		for(pbox = prgnDst->rects; pbox< pboxLast; pbox++)
		{
		    if(pbox->y1 > ppt->y)
		    {
			/* rest of clip region is above this scanline,
			 * skip it */
			break;
		    }
		    if(pbox->y2 <= ppt->y)
		    {
			/* clip box is below scanline */
			pbox++;
			break;
		    }
		    if(pbox->x1 <= ppt->x + *pwidth &&
		       pbox->x2 > ppt->x)
		    {
			xStart = max(pbox->x1, ppt->x);
			xEnd = min(pbox->x2, ppt->x + *pwidth);
			cfbSetScanline(ppt->y, ppt->x, xStart, xEnd, psrc, alu,
			    pdstBase, widthDst, pGC->planemask);
		    }

		}
	    }
	psrc += PixmapWidthInPadUnits(*pwidth, PSZ);
	ppt++;
	pwidth++;
	}
    }
}

