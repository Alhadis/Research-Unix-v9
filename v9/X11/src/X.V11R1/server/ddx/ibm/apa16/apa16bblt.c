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

#include "rtutils.h"

#include "apa16hdwr.h"
#include "apa16decls.h"

/* CopyArea for the apa16


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

void apa16CopyArea(pSrcDrawable, pDstDrawable,
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

    TRACE(("apa16CopyArea(pSrcDrawable= 0x%x, pDstDrawable= 0x%x, pGC= 0x%x, srcx= %d, srcy= %d, width= %d, height= %d, dstx= %d, dsty= %d)\n",pSrcDrawable,pDstDrawable,pGC,srcx,srcy,width,height,dstx,dsty));
    if (!(pGC->planemask & 1))
	return;

    if ((pSrcDrawable->type!=DRAWABLE_WINDOW)||
	(pDstDrawable->type!=DRAWABLE_WINDOW)) {
	mfbCopyArea(pSrcDrawable,pDstDrawable,pGC,srcx,srcy,width,height,
								dstx,dsty);
	return;
    }

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

    srcx += ((WindowPtr)pSrcDrawable)->absCorner.x;
    srcy += ((WindowPtr)pSrcDrawable)->absCorner.y;
    prgnSrcClip = ((WindowPtr)pSrcDrawable)->clipList;

    srcBox.x1 = srcx;
    srcBox.y1 = srcy;
    srcBox.x2 = srcx + width;
    srcBox.y2 = srcy + height;

    prgnDst = miRegionCreate(&srcBox, 1);
    miIntersect(prgnDst, prgnDst, prgnSrcClip);

    if (!((WindowPtr)pDstDrawable)->realized) {
	miSendNoExpose(pGC);
	return;
    }
    dstx += ((WindowPtr)pDstDrawable)->absCorner.x;
    dsty += ((WindowPtr)pDstDrawable)->absCorner.y;

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
	return;
    }
    if(!(pptSrc = (DDXPointPtr)ALLOCATE_LOCAL( prgnDst->numRects *
        sizeof(DDXPointRec))))
	return;
    pbox = prgnDst->rects;
    ppt = pptSrc;
    for (i=0; i<prgnDst->numRects; i++, pbox++, ppt++)
    {
	ppt->x = pbox->x1 + dx;
	ppt->y = pbox->y1 + dy;
    }

    apa16DoBitblt(pSrcDrawable, pDstDrawable, pGC->alu, prgnDst, pptSrc);

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

/*
 * Macro to help make apa16DoBitblt comprehensible.
 */

#define DO_RECTS(func)\
	while (nbox--) {\
	    register int w,h;\
	    w= pbox->x2-pbox->x1;\
	    h= pbox->y2-pbox->y1;\
	    pptSrc->x+= w;\
	    pptSrc->y+= h;\
	    (func);\
	    APA16_GO();\
	    pbox++;\
	    pptSrc++;\
    	}
	
/* 
 *DoBitblt() does multiple rectangle moves into the rectangles
 */

apa16DoBitblt(pSrcDrawable, pDstDrawable, alu, prgnDst, pptSrc)
DrawablePtr pSrcDrawable;
DrawablePtr pDstDrawable;
int alu;
RegionPtr 	 prgnDst;
register DDXPointPtr pptSrc;
{
    int widthSrc, widthDst;	/* add to get to same position in next line */

    register BoxPtr pbox;
    int nbox;

    BoxPtr pboxTmp, pboxNext, pboxBase, pboxNew1, pboxNew2;
				/* temporaries for shuffling rectangles */
    DDXPointPtr pptTmp, pptNew1,pptNew2;
				/* shuffling boxes entails shuffling the
				   source points too */

    register unsigned 	cmd;	/* apa16 rasterop command word */

    TRACE(("apa16DoBitblt(pSrcDrawable= 0x%x, pDstDrawable= 0%x, alu= 0x%x, prgnDst= 0x%x, pptSrc= 0x%x)\n",pSrcDrawable,pDstDrawable,alu,prgnDst,pptSrc));

    if ((pSrcDrawable->type != DRAWABLE_WINDOW)||
				(pDstDrawable->type != DRAWABLE_WINDOW)) {
	mfbDoBitblt(pSrcDrawable,pDstDrawable,alu,prgnDst,pptSrc);
	return;
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

    if (pptSrc->x < pbox->x1)
    {
	/* walk source right to left */
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

    QUEUE_RESET();
    APA16_GET_CMD(ROP_RECT_COPY,alu,cmd);
    while (QUEUE_CNTR);
    if (cmd&STYPE_MASK) { /* hardware can handle copy */
	DO_RECTS(COPY_RECT(cmd,pbox->x2,pbox->y2,pptSrc->x,pptSrc->y,w,h));
    }
    else { /* special case, lend a hand */
	switch (alu) {
	    case GXcopyInverted: /* copy and invert. simple, no? */
		{ register unsigned cmd2;
		APA16_GET_CMD(ROP_RECT_COPY,GXcopy,cmd);
		APA16_GET_CMD(ROP_RECT_FILL,GXinvert,cmd2);
		DO_RECTS((COPY_RECT(cmd,pbox->x2,pbox->y2,pptSrc->x,pptSrc->y,
									w,h),
			  FILL_RECT(cmd2,pbox->x2,pbox->y2,w,h)));
		break;
		}
			/* really rectangle fill! */
	    case GXclear:
	    case GXinvert:
	    case GXset:	
		APA16_GET_CMD(ROP_RECT_FILL,alu,cmd);
		DO_RECTS(FILL_RECT(cmd,pbox->x2,pbox->y2,w,h));
		break;
	    case GXnoop:/* do nothing */
		return;
	    case GXequiv:
	    case GXorReverse: /* messy, let mfb do it */
		mfbDoBitblt(pSrcDrawable,pDstDrawable,alu,prgnDst,pptSrc);
		return;
       }
    }
    while (QUEUE_CNTR);
}

