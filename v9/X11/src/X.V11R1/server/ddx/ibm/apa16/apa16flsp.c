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
#include "Xmd.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "mfb.h"
#include "maskbits.h"

#include "rtutils.h"

#include "apa16hdwr.h"
#include "apa16decls.h"

/* scanline filling for monochrome frame buffer
   written by drewry, oct 1986
   modified for apa16 by erik, may 1987

   these routines all clip.  they assume that anything that has called
them has already translated the points (i.e. pGC->miTranslate is
non-zero, which is howit gets set in mfbCreateGC().)

   the number of new scnalines created by clipping ==
MaxRectsPerBand * nSpans.

    FillSolid is overloaded to be used for OpaqueStipple as well,
if fgPixel == bgPixel.  
Note that for solids, PrivGC.rop == PrivGC.ropOpStip


    FillTiled is overloaded to be used for OpaqueStipple, if
fgPixel != bgPixel.  based on the fill style, it uses
{RotatedTile, gc.alu} or {RotatedStipple, PrivGC.ropOpStip}
*/


void apa16SolidFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int		nInit;			/* number of spans to fill */
    DDXPointPtr pptInit;		/* pointer to list of start points */
    int		*pwidthInit;		/* pointer to list of n widths */
    int 	fSorted;
{
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    register DDXPointPtr ppt;	/* pointer to list of start points */
    register int *pwidth;	/* pointer to list of n widths */
    int *addrlBase;		/* pointer to start of bitmap */
    int nlwidth;		/* width in longwords of bitmap */
    register int *addrl;	/* pointer to current longword in bitmap */
    register int startmask;
    register int endmask;
    register int nlmiddle;
    int rop;			/* reduced rasterop */
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;

    TRACE(("apa16SolidFS(pDrawable= 0x%x, pGC= 0x%x, nInit= %d, pptInit= 0x%x, pwidthInit= 0x%x, fSorted= %d)\n",
			pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted));

    if (!(pGC->planemask & 1))
	return;

    rop = ((mfbPrivGC *)(pGC->devPriv))->rop;
    if (rop==RROP_NOP)	return;

    if (pDrawable->type!=DRAWABLE_WINDOW) {
	if (rop==RROP_WHITE)
	    mfbWhiteSolidFS(pDrawable,pGC,nInit,pptInit,pwidthInit,fSorted);
	else if (rop==RROP_BLACK)
	    mfbBlackSolidFS(pDrawable,pGC,nInit,pptInit,pwidthInit,fSorted);
	else if (rop==RROP_INVERT)
	    mfbInvertSolidFS(pDrawable,pGC,nInit,pptInit,pwidthInit,fSorted);
	return;
    }

    n = nInit * miFindMaxBand(((mfbPrivGC *)(pGC->devPriv))->pCompositeClip);
    pwidth = (int *)ALLOCATE_LOCAL(n * sizeof(int));
    ppt = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
    if(!ppt || !pwidth)
    {
	DEALLOCATE_LOCAL(ppt);
	DEALLOCATE_LOCAL(pwidth);
	return;
    }
    pwidthFree = pwidth;
    pptFree = ppt;
    n = miClipSpans(((mfbPrivGC *)(pGC->devPriv))->pCompositeClip,
		     pptInit, pwidthInit, nInit,
		     ppt, pwidth, fSorted);

    addrlBase = (int *)
	(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate);
    nlwidth = (int)
	(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;

    if		(rop == RROP_BLACK)	SET_MERGE_MODE(MERGE_BLACK);
    else if	(rop == RROP_WHITE)	SET_MERGE_MODE(MERGE_WHITE);
    else if	(rop == RROP_INVERT)	SET_MERGE_MODE(MERGE_INVERT);
    else if	(rop == RROP_NOP)	return;
    else {
	ErrorF("Unexpected rrop %d in apa16SolidFS\n",rop);
    }

    while (n--)
    {
        addrl = addrlBase + (ppt->y * nlwidth) + (ppt->x >> 5);

	if (*pwidth)
	{
	    if ( ((ppt->x & 0x1f) + *pwidth) < 32)
	    {
		/* all bits inside same longword */
		maskpartialbits(ppt->x, *pwidth, startmask);
		*addrl = startmask;
	    }
	    else
	    {
		maskbits(ppt->x, *pwidth, startmask, endmask, nlmiddle);
		if (startmask)
		    *addrl++ = startmask;
		while (nlmiddle--)
		    *addrl++ = 0xffffffff;
		if (endmask)
		    *addrl = endmask;
	    }
	}
	pwidth++;
	ppt++;
    }
    SET_MERGE_MODE(MERGE_COPY);
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}


void apa16StippleFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
DrawablePtr pDrawable;
GC *pGC;
int nInit;			/* number of spans to fill */
DDXPointPtr pptInit;		/* pointer to list of start points */
int *pwidthInit;		/* pointer to list of n widths */
int fSorted;
{
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    register DDXPointPtr ppt;	/* pointer to list of start points */
    register int *pwidth;	/* pointer to list of n widths */
    int *addrlBase;		/* pointer to start of bitmap */
    int nlwidth;		/* width in longwords of bitmap */
    register int *addrl;	/* pointer to current longword in bitmap */
    register int startmask;
    register int endmask;
    register int nlmiddle;
    int rop;			/* reduced rasterop */
    PixmapPtr pStipple;
    int *psrc;
    int src;
    int tileHeight;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;

    if (!(pGC->planemask & 1))
	return;

    rop = ((mfbPrivGC *)(pGC->devPriv))->rop;
    if (rop==RROP_NOP)	return;

    if (pDrawable->type!=DRAWABLE_WINDOW) {
	if (rop==RROP_WHITE)
	    mfbWhiteStippleFS(pDrawable,pGC,nInit,pptInit,pwidthInit,fSorted);
	else if (rop==RROP_BLACK)
	    mfbWhiteStippleFS(pDrawable,pGC,nInit,pptInit,pwidthInit,fSorted);
	else if (rop==RROP_INVERT)
	    mfbWhiteStippleFS(pDrawable,pGC,nInit,pptInit,pwidthInit,fSorted);
	return;
    }

    n = nInit * miFindMaxBand(((mfbPrivGC *)(pGC->devPriv))->pCompositeClip);
    pwidth = (int *)ALLOCATE_LOCAL(n * sizeof(int));
    ppt = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
    if(!ppt || !pwidth)
    {
	DEALLOCATE_LOCAL(ppt);
	DEALLOCATE_LOCAL(pwidth);
	return;
    }
    pwidthFree = pwidth;
    pptFree = ppt;
    n = miClipSpans(((mfbPrivGC *)(pGC->devPriv))->pCompositeClip,
		     pptInit, pwidthInit, nInit, 
		     ppt, pwidth, fSorted);

    addrlBase = (int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate);
    nlwidth = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;

    pStipple = ((mfbPrivGC *)(pGC->devPriv))->pRotatedStipple;
    tileHeight = pStipple->height;
    psrc = (int *)(pStipple->devPrivate);

    if		(rop == RROP_BLACK)	SET_MERGE_MODE(MERGE_BLACK);
    else if	(rop == RROP_WHITE)	SET_MERGE_MODE(MERGE_WHITE);
    else if	(rop == RROP_INVERT)	SET_MERGE_MODE(MERGE_INVERT);
    else if	(rop == RROP_NOP)	return;
    else {
	ErrorF("Unexpected rrop %d in apa16StippleFS\n",rop);
    }

    while (n--)
    {
        addrl = addrlBase + (ppt->y * nlwidth) + (ppt->x >> 5);
	src = psrc[ppt->y % tileHeight];

        /* all bits inside same longword */
        if ( ((ppt->x & 0x1f) + *pwidth) < 32)
        {
	    maskpartialbits(ppt->x, *pwidth, startmask);
	    *addrl = (src & startmask);
        }
        else
        {
	    maskbits(ppt->x, *pwidth, startmask, endmask, nlmiddle);
	    if (startmask)
		*addrl++ = (src & startmask);
	    while (nlmiddle--)
		*addrl++ = src;
	    if (endmask)
		*addrl = (src & endmask);
        }
	pwidth++;
	ppt++;
    }
    SET_MERGE_MODE(MERGE_COPY);
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}

