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
#include "Xprotostr.h"

#include "miscstruct.h"
#include "regionstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "cfb.h"
#include "cfbmskbits.h"


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

/* macro for bitblt to avoid a switch on the alu per scanline 
   comments are in the real code in cfbDoBitblt.
   we need tmpDst for things less than 1 word wide becuase
the destination may cross a word boundary, and we need to read
it all at once to do the rasterop.  (this perhaps argues for
sub-casing narrow things that don't cross a word boundary.)
*/
#define DOBITBLT(ALU) \
while (nbox--) \
{ \
    w = pbox->x2 - pbox->x1; \
    h = pbox->y2 - pbox->y1; \
    if (ydir == -1) \
    { \
        psrcLine = psrcBase + ((pptSrc->y+h-1) * -widthSrc); \
        pdstLine = pdstBase + ((pbox->y2-1) * -widthDst); \
    } \
    else \
    { \
        psrcLine = psrcBase + (pptSrc->y * widthSrc); \
        pdstLine = pdstBase + (pbox->y1 * widthDst); \
    } \
    if (w <= PPW) \
    { \
	int tmpDst; \
        int srcBit, dstBit; \
        pdstLine += (pbox->x1 >> PWSH); \
        psrcLine += (pptSrc->x >> PWSH); \
        psrc = psrcLine; \
        pdst = pdstLine; \
        srcBit = pptSrc->x & PIM; \
        dstBit = pbox->x1 & PIM; \
        while(h--) \
        { \
	    getbits(psrc, srcBit, w, tmpSrc) \
	    getbits(pdst, dstBit, w, tmpDst) \
	    tmpSrc = ALU(tmpSrc, tmpDst); \
/*XXX*/	    putbits(tmpSrc, dstBit, w, pdst, -1) \
	    pdst += widthDst; \
	    psrc += widthSrc; \
        } \
    } \
    else \
    { \
        register int xoffSrc; \
        int nstart; \
        int nend; \
        int srcStartOver; \
        maskbits(pbox->x1, w, startmask, endmask, nlMiddle) \
        if (startmask) \
	    nstart = PPW - (pbox->x1 & PIM); \
        else \
	    nstart = 0; \
        if (endmask) \
            nend = pbox->x2 & PIM; \
        else \
	    nend = 0; \
        xoffSrc = ((pptSrc->x & PIM) + nstart) & PIM; \
        srcStartOver = ((pptSrc->x & PIM) + nstart) > PLST; \
        if (xdir == 1) \
        { \
            pdstLine += (pbox->x1 >> PWSH); \
            psrcLine += (pptSrc->x >> PWSH); \
	    while (h--) \
	    { \
	        psrc = psrcLine; \
	        pdst = pdstLine; \
	        if (startmask) \
	        { \
		    getbits(psrc, (pptSrc->x & PIM), nstart, tmpSrc) \
		    tmpSrc = ALU(tmpSrc, *pdst); \
/*XXX*/		    putbits(tmpSrc, (pbox->x1 & PIM), nstart, pdst, -1) \
		    pdst++; \
		    if (srcStartOver) \
		        psrc++; \
	        } \
	        nl = nlMiddle; \
	        while (nl--) \
	        { \
		    getbits(psrc, xoffSrc, PPW, tmpSrc) \
		    *pdst = ALU(tmpSrc, *pdst); \
		    pdst++; \
		    psrc++; \
	        } \
	        if (endmask) \
	        { \
		    getbits(psrc, xoffSrc, nend, tmpSrc) \
		    tmpSrc = ALU(tmpSrc, *pdst); \
/*XXX*/		    putbits(tmpSrc, 0, nend, pdst, -1) \
	        } \
	        pdstLine += widthDst; \
	        psrcLine += widthSrc; \
	    } \
        } \
        else  \
        { \
            pdstLine += (pbox->x2 >> PWSH); \
            psrcLine += (pptSrc->x+w >> PWSH); \
	    if (xoffSrc + nend >= PPW) \
	        --psrcLine; \
	    while (h--) \
	    { \
	        psrc = psrcLine; \
	        pdst = pdstLine; \
	        if (endmask) \
	        { \
		    getbits(psrc, xoffSrc, nend, tmpSrc) \
		    tmpSrc = ALU(tmpSrc, *pdst); \
/*XXX*/		    putbits(tmpSrc, 0, nend, pdst, -1) \
	        } \
	        nl = nlMiddle; \
	        while (nl--) \
	        { \
		    --psrc; \
		    getbits(psrc, xoffSrc, PPW, tmpSrc) \
		    --pdst; \
		    *pdst = ALU(tmpSrc, *pdst); \
	        } \
	        if (startmask) \
	        { \
		    if (srcStartOver) \
		        --psrc; \
		    --pdst; \
		    getbits(psrc, (pptSrc->x & PIM), nstart, tmpSrc) \
		    tmpSrc = ALU(tmpSrc, *pdst); \
/*XXX*/		    putbits(tmpSrc, (pbox->x1 & PIM), nstart, pdst, -1) \
	        } \
	        pdstLine += widthDst; \
	        psrcLine += widthSrc; \
	    } \
        } \
    } \
    pbox++; \
    pptSrc++; \
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

cfbDoBitblt(pSrcDrawable, pDstDrawable, alu, prgnDst, pptSrc)
DrawablePtr pSrcDrawable;
DrawablePtr pDstDrawable;
int alu;
RegionPtr prgnDst;
DDXPointPtr pptSrc;
{
    int *psrcBase, *pdstBase;	/* start of src and dst bitmaps */
    int widthSrc, widthDst;	/* add to get to same position in next line */

    register BoxPtr pbox;
    int nbox;

    BoxPtr pboxTmp, pboxNext, pboxBase, pboxNew;
				/* temporaries for shuffling rectangles */
    DDXPointPtr pptTmp, pptNew;	/* shuffling boxes entails shuffling the
				   source points too */
    int w, h;
    int xdir;			/* 1 = left right, -1 = right left/ */
    int ydir;			/* 1 = top down, -1 = bottom up */

    int *psrcLine, *pdstLine;	/* pointers to line with current src and dst */
    register int *psrc;		/* pointer to current src longword */
    register int *pdst;		/* pointer to current dst longword */

				/* following used for looping through a line */
    int startmask, endmask;	/* masks for writing ends of dst */
    int nlMiddle;		/* whole longwords in dst */
    register int nl;		/* temp copy of nlMiddle */
    register int tmpSrc;	/* place to store full source word */

    if (pSrcDrawable->type == DRAWABLE_WINDOW)
    {
	psrcBase = (int *)
		(((PixmapPtr)(pSrcDrawable->pScreen->devPrivate))->devPrivate);
	widthSrc = (int)
		   ((PixmapPtr)(pSrcDrawable->pScreen->devPrivate))->devKind
		    >> 2;
    }
    else
    {
	psrcBase = (int *)(((PixmapPtr)pSrcDrawable)->devPrivate);
	widthSrc = (int)(((PixmapPtr)pSrcDrawable)->devKind) >> 2;
    }

    if (pDstDrawable->type == DRAWABLE_WINDOW)
    {
	pdstBase = (int *)
		(((PixmapPtr)(pDstDrawable->pScreen->devPrivate))->devPrivate);
	widthDst = (int)
		   ((PixmapPtr)(pDstDrawable->pScreen->devPrivate))->devKind
		    >> 2;
    }
    else
    {
	pdstBase = (int *)(((PixmapPtr)pDstDrawable)->devPrivate);
	widthDst = (int)(((PixmapPtr)pDstDrawable)->devKind) >> 2;
    }

    pbox = prgnDst->rects;
    nbox = prgnDst->numRects;

    pboxNew = 0;
    pptNew = 0;
    if (pptSrc->y < pbox->y1) 
    {
        /* walk source botttom to top */
	ydir = -1;
	widthSrc = -widthSrc;
	widthDst = -widthDst;

	if (nbox > 1)
	{
	    /* keep ordering in each band, reverse order of bands */
	    pboxNew = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    pptNew = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    if(!pboxNew || !pptNew)
	    {
	        DEALLOCATE_LOCAL(pptNew);
	        DEALLOCATE_LOCAL(pboxNew);
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
		    *pboxNew++ = *pboxTmp++;
		    *pptNew++ = *pptTmp++;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew -= nbox;
	    pbox = pboxNew;
	    pptNew -= nbox;
	    pptSrc = pptNew;
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
	    /* reverse order of rects ineach band */
	    pboxNew = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    pptNew = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * nbox);
	    pboxBase = pboxNext = pbox;
	    if(!pboxNew || !pptNew)
	    {
	        DEALLOCATE_LOCAL(pptNew);
	        DEALLOCATE_LOCAL(pboxNew);
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
		    *pboxNew++ = *--pboxTmp;
		    *pptNew++ = *--pptTmp;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew -= nbox;
	    pbox = pboxNew;
	    pptNew -= nbox;
	    pptSrc = pptNew;
	}
    }
    else
    {
	/* walk source left to right */
        xdir = 1;
    }


    /* special case copy, to avoid some redundant moves into temporaries */
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
	    if (w <= PPW)
	    {
	        int srcBit, dstBit;	/* bit offset of src and dst */

	        pdstLine += (pbox->x1 >> PWSH);
	        psrcLine += (pptSrc->x >> PWSH);
	        psrc = psrcLine;
	        pdst = pdstLine;

	        srcBit = pptSrc->x & PIM;
	        dstBit = pbox->x1 & PIM;

	        while(h--)
	        {
		    getbits(psrc, srcBit, w, tmpSrc)
/*XXX*/		    putbits(tmpSrc, dstBit, w, pdst, -1)
		    pdst += widthDst;
		    psrc += widthSrc;
	        }
	    }
	    else
	    {
    	        register int xoffSrc;	/* offset (>= 0, < 32) from which to
				           fetch whole longwords fetched 
					   in src */
	        int nstart;		/* number of ragged bits 
					   at start of dst */
	        int nend;		/* number of ragged bits at end 
					   of dst */
	        int srcStartOver;	/* pulling nstart bits from src
					   overflows into the next word? */

	        maskbits(pbox->x1, w, startmask, endmask, nlMiddle)
	        if (startmask)
		    nstart = PPW - (pbox->x1 & PIM);
	        else
		    nstart = 0;
	        if (endmask)
	            nend = pbox->x2 & PIM;
	        else
		    nend = 0;

	        xoffSrc = ((pptSrc->x & PIM) + nstart) & PIM;
	        srcStartOver = ((pptSrc->x & PIM) + nstart) > PLST;

	        if (xdir == 1) /* move left to right */
	        {
	            pdstLine += (pbox->x1 >> PWSH);
	            psrcLine += (pptSrc->x >> PWSH);

		    while (h--)
		    {
		        psrc = psrcLine;
		        pdst = pdstLine;

		        if (startmask)
		        {
			    getbits(psrc, (pptSrc->x & PIM), nstart, tmpSrc)
/*XXX*/			    putbits(tmpSrc, (pbox->x1 & PIM), nstart, pdst, 
				-1)
			    pdst++;
			    if (srcStartOver)
			        psrc++;
		        }

		        nl = nlMiddle;
		        while (nl--)
		        {
			    getbits(psrc, xoffSrc, PPW, tmpSrc)
			    *pdst++ = tmpSrc;
			    psrc++;
		        }

		        if (endmask)
		        {
			    getbits(psrc, xoffSrc, nend, tmpSrc)
/*XXX*/			    putbits(tmpSrc, 0, nend, pdst, -1)
		        }

		        pdstLine += widthDst;
		        psrcLine += widthSrc;
		    }
	        }
	        else /* move right to left */
	        {
	            pdstLine += (pbox->x2 >> PWSH);
	            psrcLine += (pptSrc->x+w >> PWSH);
		    /* if fetch of last partial bits from source crosses
		       a longword boundary, start at the previous longword
		    */
		    if (xoffSrc + nend >= PPW)
		        --psrcLine;

		    while (h--)
		    {
		        psrc = psrcLine;
		        pdst = pdstLine;

		        if (endmask)
		        {
			    getbits(psrc, xoffSrc, nend, tmpSrc)
/*XXX*/			    putbits(tmpSrc, 0, nend, pdst, -1)
		        }

		        nl = nlMiddle;
		        while (nl--)
		        {
			    --psrc;
			    getbits(psrc, xoffSrc, PPW, tmpSrc)
			    *--pdst = tmpSrc;
		        }

		        if (startmask)
		        {
			    if (srcStartOver)
			        --psrc;
			    --pdst;
			    getbits(psrc, (pptSrc->x & PIM), nstart, tmpSrc)
/*XXX*/			    putbits(tmpSrc, (pbox->x1 & PIM), nstart, pdst, 
			    -1)
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
    else
    {
          if (alu == GXclear)
	    DOBITBLT(fnCLEAR)
          else if (alu ==GXand)
	    DOBITBLT(fnAND)
          else if (alu == GXandReverse)
	    DOBITBLT(fnANDREVERSE)
          else if (alu == GXcopy)
	    DOBITBLT(fnCOPY)
          else if (alu == GXandInverted)
	    DOBITBLT(fnANDINVERTED)
/*
          else if (alu == GXnoop)
*/
          else if (alu == GXxor)
	    DOBITBLT(fnXOR)
          else if (alu == GXor)
	    DOBITBLT(fnOR)
          else if (alu == GXnor)
	    DOBITBLT(fnNOR)
          else if (alu == GXequiv)
	    DOBITBLT(fnEQUIV)
          else if (alu == GXinvert)
	    DOBITBLT(fnINVERT)
          else if (alu == GXorReverse)
	    DOBITBLT(fnORREVERSE)
          else if (alu == GXcopyInverted)
	    DOBITBLT(fnCOPYINVERTED)
          else if (alu == GXorInverted)
	    DOBITBLT(fnORINVERTED)
          else if (alu == GXnand)
	    DOBITBLT(fnNAND)
          else if (alu == GXset)
	    DOBITBLT(fnSET)
    }

    if (pptNew)
    {
	DEALLOCATE_LOCAL(pptNew);
    }
    if (pboxNew)
    {
	DEALLOCATE_LOCAL(pboxNew);
    }
}
