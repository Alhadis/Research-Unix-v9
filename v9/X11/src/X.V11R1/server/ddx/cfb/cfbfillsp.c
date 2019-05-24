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
#include "Xmd.h"
#include "servermd.h"
#include "gcstruct.h"
#include "window.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "cfb.h"
#include "cfbmskbits.h"

/* scanline filling for color frame buffer
   written by drewry, oct 1986 modified by smarks
   changes for compatibility with Little-endian systems Jul 1987; MIT:yba.

   these routines all clip.  they assume that anything that has called
them has already translated the points (i.e. pGC->miTranslate is
non-zero, which is howit gets set in cfbCreateGC().)

   the number of new scnalines created by clipping ==
MaxRectsPerBand * nSpans.

    FillSolid is overloaded to be used for OpaqueStipple as well,
if fgPixel == bgPixel.  
Note that for solids, PrivGC.rop == PrivGC.ropOpStip


    FillTiled is overloaded to be used for OpaqueStipple, if
fgPixel != bgPixel.  based on the fill style, it uses
{RotatedTile, gc.alu} or {RotatedStipple, PrivGC.ropOpStip}
*/

#ifdef	notdef
#include	<stdio.h>
static
dumpspans(n, ppt, pwidth)
    int	n;
    DDXPointPtr ppt;
    int *pwidth;
{
    fprintf(stderr,"%d spans\n", n);
    while (n--) {
	fprintf(stderr, "[%d,%d] %d\n", ppt->x, ppt->y, *pwidth);
	ppt++;
	pwidth++;
    }
    fprintf(stderr, "\n");
}
#endif

void
cfbSolidFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
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
    int fill, rrop;

    switch (pDrawable->depth) {
	case 1:
	    rrop = ReduceRop( pGC->alu, pGC->fgPixel );
	    switch ( rrop ) {
		case RROP_BLACK:
		    mfbBlackSolidFS(pDrawable, pGC, nInit, pptInit, 
			pwidthInit, fSorted);
		    break;
		case RROP_WHITE:
		    mfbWhiteSolidFS(pDrawable, pGC, nInit, pptInit, 
			pwidthInit, fSorted);
		    break;
		case RROP_NOP:
		    return;
		case RROP_INVERT:
		    mfbInvertSolidFS(pDrawable, pGC, nInit, pptInit, 
			pwidthInit, fSorted);
		    break;
	    }
	    return;
	case 8:
	    break;
	default:
	    FatalError("cfbSolidFS: invalid depth\n");
    }

    if (!(pGC->planemask))
	return;

    n = nInit * miFindMaxBand(((cfbPrivGC *)(pGC->devPriv))->pCompositeClip);
    pwidth = (int *)ALLOCATE_LOCAL(n * sizeof(int));
    ppt = (DDXPointRec *)ALLOCATE_LOCAL(n * sizeof(DDXPointRec));
    if(!ppt || !pwidth)
    {
	DEALLOCATE_LOCAL(ppt);
	DEALLOCATE_LOCAL(pwidth);
	return;
    }
#ifdef	notdef
    dumpspans(n, pptInit, pwidthInit);
#endif
    pwidthFree = pwidth;
    pptFree = ppt;
    n = miClipSpans(((cfbPrivGC *)(pGC->devPriv))->pCompositeClip,
		     pptInit, pwidthInit, nInit,
		     ppt, pwidth, fSorted);

#ifdef	notdef
    dumpspans(n, ppt, pwidth);
#endif
    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	addrlBase = (int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate);
	nlwidth = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	addrlBase = (int *)(((PixmapPtr)pDrawable)->devPrivate);
	nlwidth = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    rop = pGC->alu;
    fill = PFILL(pGC->fgPixel);

    while (n--)
    {
        addrl = addrlBase + (ppt->y * nlwidth) + (ppt->x >> PWSH);

	if (*pwidth)
	{
	    if ( ((ppt->x & PIM) + *pwidth) <= PPW)
	    {
		/* all bits inside same longword */
		putbitsrop( fill, ppt->x & PIM, *pwidth,
		    addrl, pGC->planemask, rop );
	    }
	    else
	    {
		maskbits(ppt->x, *pwidth, startmask, endmask, nlmiddle);
		if ( startmask ) {
		    putbitsrop( fill, ppt->x & PIM,
			PPW-(ppt->x&PIM), addrl, pGC->planemask, rop);
		    ++addrl;
		}
		while ( nlmiddle-- ) {
		    putbitsrop( fill, 0, PPW,
			addrl, pGC->planemask, rop );
		    ++addrl;
		}
		if ( endmask ) {
		    putbitsrop( fill, 0, 
			((ppt->x + *pwidth)&PIM), addrl, pGC->planemask, rop );
		}
	    }
	}
	pwidth++;
	ppt++;
    }
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}


/* Fill spans with tiles that aren't 32 bits wide */
void
cfbUnnaturalTileFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
DrawablePtr pDrawable;
GC		*pGC;
int		nInit;		/* number of spans to fill */
DDXPointPtr pptInit;		/* pointer to list of start points */
int *pwidthInit;		/* pointer to list of n widths */
int fSorted;
{
    int		iline;		/* first line of tile to use */
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    register DDXPointPtr ppt;	/* pointer to list of start points */
    register int *pwidth;	/* pointer to list of n widths */
    int		*addrlBase;	/* pointer to start of bitmap */
    int		 nlwidth;	/* width in longwords of bitmap */
    register int *pdst;		/* pointer to current word in bitmap */
    register int *psrc;		/* pointer to current word in tile */
    register int startmask;
    register int nlMiddle;
    PixmapPtr	pTile;		/* pointer to tile we want to fill with */
    int		w, width, x, tmpSrc, srcStartOver, nstart, nend;
    int 	endmask, tlwidth, rem, tileWidth, *psrcT, rop;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;

    switch (pDrawable->depth) {
	case 1:
	    mfbUnnaturalTileFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);
	    return;
	case 8:
	    break;
	default:
	    FatalError("cfbUnnaturalTileFS: invalid depth\n");
    }

    if (!(pGC->planemask))
	return;

    n = nInit * miFindMaxBand(((cfbPrivGC *)(pGC->devPriv))->pCompositeClip);
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
    n = miClipSpans(((cfbPrivGC *)(pGC->devPriv))->pCompositeClip,
		     pptInit, pwidthInit, nInit, 
		     ppt, pwidth, fSorted);

    if (pGC->fillStyle == FillTiled)
    {
	pTile = ((cfbPrivGC *)(pGC->devPriv))->pRotatedTile;
	tlwidth = pTile->devKind >> 2;
	rop = pGC->alu;
    }
    else
    {
	pTile = ((cfbPrivGC *)(pGC->devPriv))->pRotatedStipple;
	tlwidth = pTile->devKind >> 2;
	rop = pGC->alu;
    }

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	addrlBase = (int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate);
	nlwidth = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	addrlBase = (int *)(((PixmapPtr)pDrawable)->devPrivate);
	nlwidth = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    tileWidth = pTile->width;
    while (n--)
    {
	iline = ppt->y % pTile->height;
        pdst = addrlBase + (ppt->y * nlwidth) + (ppt->x >> PWSH);
        psrcT = (int *) pTile->devPrivate + (iline * tlwidth);
	x = ppt->x;

	if (*pwidth)
	{
	    width = *pwidth;
	    while(width > 0)
	    {
		rem = x % tileWidth;
		psrc = psrcT + rem / PPW;
	        w = min(tileWidth, width);
		w = min(w,tileWidth-rem);
#ifdef notdef
		if((rem = x % tileWidth) != 0)
		{
		    w = min(min(tileWidth - rem, width), PPW);
		    /* we want to grab from the end of the tile.  Figure
		     * out where that is.  In general, the start of the last
		     * word of data on this scanline is tlwidth -1 words 
		     * away. But if we want to grab more bits than we'll
		     * find on that line, we have to back up 1 word further.
		     * On the other hand, if the whole tile fits in 1 word,
		     * let's skip the work */ 
		    endinc = tlwidth - 1 - (tileWidth-rem) / PPW;

		    if(endinc)
		    {
			if((rem & PIM) + w > tileWidth % PPW)
			    endinc--;
		    }

		    getbits(psrc + endinc, rem & PIM, w, tmpSrc);
		    putbitsrop(tmpSrc, (x & PIM), w, pdst, 
			pGC->planemask, rop);
		    if((x & PIM) + w >= PPW)
			pdst++;
		}
		else
#endif notdef
		if(((rem & PIM) + w) <= PPW)
		{
		    getbits(psrc, (rem & PIM), w, tmpSrc);
		    putbitsrop(tmpSrc, x & PIM, w, pdst, 
			pGC->planemask, rop);
		    ++pdst;
		}
		else
		{
		    maskbits(x, w, startmask, endmask, nlMiddle);

	            if (startmask)
		        nstart = PPW - (x & PIM);
	            else
		        nstart = 0;
	            if (endmask)
	                nend = (x + w)  & PIM;
	            else
		        nend = 0;

	            srcStartOver = nstart > PLST;

		    if(startmask)
		    {
			getbits(psrc, rem & PIM, nstart, tmpSrc);
			putbitsrop(tmpSrc, x & PIM, nstart, pdst, 
			    pGC->planemask, rop);
			pdst++;
			psrc++;
		    }
		     
		    while(nlMiddle--)
		    {
			    getbits(psrc, 0, PPW, tmpSrc);
			    putbitsrop( tmpSrc, 0, PPW,
				pdst, pGC->planemask, rop );
			    pdst++;
			    psrc++;
		    }
		    if(endmask)
		    {
			getbits(psrc, 0, nend, tmpSrc);
			putbitsrop(tmpSrc, 0, nend, pdst, 
			    pGC->planemask, rop);
		    }
		 }
		 x += w;
		 width -= w;
	    }
	}
	ppt++;
	pwidth++;
    }
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}


/*
 * QuartetBitsTable contains four masks whose binary values are masks in the 
 * low order quartet that contain the number of bits specified in the 
 * index.  This table is used by getstipplepixels.
 */
static unsigned int QuartetBitsTable[5] = {
#if (BITMAP_BIT_ORDER == MSBFirst)
    0x00000000,				/* 0 - 0000 */
    0x00000008,				/* 1 - 1000 */
    0x0000000C,				/* 2 - 1100 */
    0x0000000E,				/* 3 - 1110 */
    0x0000000F				/* 4 - 1111 */
#else /* (BITMAP_BIT_ORDER == LSBFirst */
    0x00000000,				/* 0 - 0000 */
    0x00000001,				/* 1 - 0001 */
    0x00000003,				/* 2 - 0011 */
    0x00000007,				/* 3 - 0111 */
    0x0000000F				/* 4 - 1111 */
#endif (BITMAP_BIT_ORDER == MSBFirst)
};

/*
 * QuartetPixelMaskTable is used by getstipplepixels to get a pixel mask 
 * corresponding to a quartet of bits.
 */
static unsigned int QuartetPixelMaskTable[16] = {
    0x00000000,
    0x000000FF,
    0x0000FF00,
    0x0000FFFF,
    0x00FF0000,
    0x00FF00FF,
    0x00FFFF00,
    0x00FFFFFF,
    0xFF000000,
    0xFF0000FF,
    0xFF00FF00,
    0xFF00FFFF,
    0xFFFF0000,
    0xFFFF00FF,
    0xFFFFFF00,
    0xFFFFFFFF
};


/*
 * getstipplepixels( psrcstip, x, w, ones, psrcpix, destpix )
 * 
 * Converts bits to pixels in a reasonable way.  Takes w (1 <= w <= 4)
 * bits from *psrcstip, starting at bit x; call this a quartet of bits.
 * Then, takes the pixels from *psrcpix corresponding to the one-bits (if
 * ones is TRUE) or the zero-bits (if ones is FALSE) of the quartet
 * and puts these pixels into destpix.
 *
 * Example:
 * 
 *	getstipplepixels( &(0x08192A3B), 17, 4, 1, &(0x4C5D6E7F), dest )
 *
 * 0x08192A3B = 0000 1000 0001 1001 0010 1010 0011 1011
 *
 * This will take 4 bits starting at bit 17, so the quartet is 0x5 = 0101.
 * It will take pixels from 0x4C5D6E7F corresponding to the one-bits in this 
 * quartet, so dest = 0x005D007F.
 *
 * XXX This should be turned into a macro after it is debugged.
 * XXX Has byte order dependencies.
 * XXX This works for all values of x and w within a doubleword, depending
 *     on the compiler to generate proper code for negative shifts.
 */

void getstipplepixels( psrcstip, x, w, ones, psrcpix, destpix )
unsigned int *psrcstip, *psrcpix, *destpix;
int x, w, ones;
{
    unsigned int q;

#if (BITMAP_BIT_ORDER == MSBFirst)
    q = ((*psrcstip) >> (28-x)) & 0x0F;
    if ( x+w > 32 )
	q |= *(psrcstip+1) >> (64-x-w); /* & 0xF ? ****XXX*/
#else
    q = (*psrcstip) >> x;
    if ( x+w > 32 )
	q |= *(psrcstip+1) << (32-x);
    q &= 0xf;
#endif
    q = QuartetBitsTable[w] & (ones ? q : ~q);
    *destpix = (*(psrcpix)) & QuartetPixelMaskTable[q];
}
    

/* Fill spans with stipples that aren't 32 bits wide */
void
cfbUnnaturalStippleFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
DrawablePtr pDrawable;
GC		*pGC;
int		nInit;		/* number of spans to fill */
DDXPointPtr pptInit;		/* pointer to list of start points */
int *pwidthInit;		/* pointer to list of n widths */
int fSorted;
{
				/* next three parameters are post-clip */
    int n;			/* number of spans to fill */
    int xrem; 
    register DDXPointPtr ppt;	/* pointer to list of start points */
    register int *pwidth;	/* pointer to list of n widths */
    int		iline;		/* first line of tile to use */
    int		*addrlBase;	/* pointer to start of bitmap */
    int		 nlwidth;	/* width in longwords of bitmap */
    register int *pdst;		/* pointer to current word in bitmap */
    PixmapPtr	pStipple;	/* pointer to stipple we want to fill with */
    int		w, width,  x;
    unsigned int tmpSrc, tmpDst1, tmpDst2;
    int 	stwidth, stippleWidth, *psrcS, rop;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;
    unsigned int fgfill, bgfill;

    switch (pDrawable->depth) {
	case 1:
	    mfbUnnaturalStippleFS(pDrawable, pGC, nInit, pptInit, 
		pwidthInit, fSorted);
	    return;
	case 8:
	    break;
	default:
	    FatalError("cfbUnnaturalStippleFS: invalid depth\n");
    }

    if (!(pGC->planemask))
	return;

    n = nInit * miFindMaxBand(((cfbPrivGC *)(pGC->devPriv))->pCompositeClip);
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
    n = miClipSpans(((cfbPrivGC *)(pGC->devPriv))->pCompositeClip,
		     pptInit, pwidthInit, nInit, 
		     ppt, pwidth, fSorted);
    rop = ((cfbPrivGC *)(pGC->devPriv))->rop;
    fgfill = PFILL(pGC->fgPixel);
    bgfill = PFILL(pGC->bgPixel);

    /*
     *  OK,  so what's going on here?  We have two Drawables:
     *
     *  The Stipple:
     *		Depth = 1
     *		Width = stippleWidth
     *		Words per scanline = stwidth
     *		Pointer to pixels = pStipple->devPrivate
     */
    pStipple = ((cfbPrivGC *)(pGC->devPriv))->pRotatedStipple;

    if (pStipple->drawable.depth != 1) {
	FatalError( "Stipple depth not equal to 1!\n" );
    }

    stwidth = pStipple->devKind >> 2;
    stippleWidth = pStipple->width;

    /*
     *	The Target:
     *		Depth = PSZ
     *		Width = determined from *pwidth
     *		Words per scanline = nlwidth
     *		Pointer to pixels = addrlBase
     */
    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	addrlBase = (int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate);
	nlwidth = (int)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	addrlBase = (int *)(((PixmapPtr)pDrawable)->devPrivate);
	nlwidth = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    while (n--)
    {
	iline = ppt->y % pStipple->height;
	x = ppt->x;
	pdst = addrlBase + (ppt->y * nlwidth);
        psrcS = (int *) pStipple->devPrivate + (iline * stwidth);

	if (*pwidth)
	{
	    width = *pwidth;
	    while(width > 0)
	    {
		/*
		 *  Do a stripe through the stipple & destination w pixels
		 *  wide.  w is not more than:
		 *	-	the width of the destination
		 *	-	the width of the stipple
		 *	-	the distance between x and the next word 
		 *		boundary in the destination
		 *	-	the distance between x and the next word
		 *		boundary in the stipple
		 */

		/* width of dest/stipple */
                xrem = x % stippleWidth;
	        w = min((stippleWidth - (x % stippleWidth)), width);
		/* dist to word bound in dest */
		w = min(w, PPW - (x & PIM));
		/* dist to word bound in stip */
		w = min(w, 32 - (x & 0x1f));

		/* Fill tmpSrc with the source pixels */
		tmpSrc = *(pdst + (x >> PWSH));
		switch ( pGC->fillStyle ) {
		    case FillOpaqueStippled:
			getstipplepixels(psrcS + (xrem>>5), (x&0x1f), w, 0, 
			    &bgfill, &tmpDst1);
			getstipplepixels(psrcS + (xrem>>5), (x&0x1f), w, 1,
			    &fgfill, &tmpDst2);
			break;
		    case FillStippled:
			getstipplepixels(psrcS + (xrem>>5), (x&0x1f), w, 0,
			    &tmpSrc, &tmpDst1);
			getstipplepixels(psrcS + (xrem>>5), (x&0x1f), w, 1,
			    &fgfill, &tmpDst2);
			break;
		}
#ifdef notdef
		putbitsrop(tmpDst1 | tmpDst2, x & PIM, w, pdst + (x>>PWSH), 
		    pGC->planemask, rop);
#else
		{
		    /*
		     * Ultrix cc hasn't enough expression stack to compile
		     * these values in the putbitsrop call.  (MIT:yba)
		     */
		    int tmpDst = tmpDst1 | tmpDst2, tmpx = x & PIM;
		    int * pdsttmp = pdst + (x>>PWSH);
		    putbitsrop(tmpDst, tmpx, w, pdsttmp, pGC->planemask, rop);
		}
#endif notdef
		x += w;
		width -= w;
	    }
	}
#ifdef	notdef
	if (*pwidth)
	{
	    width = *pwidth;
	    while(width > 0)
	    {
		psrc = psrcS;
	        w = min(min(stippleWidth, width), PPW);
		if((rem = x % stippleWidth) != 0)
		{
		    w = min(min(stippleWidth - rem, width), PPW);
		    /* we want to grab from the end of the tile.  Figure
		     * out where that is.  In general, the start of the last
		     * word of data on this scanline is stwidth -1 words 
		     * away. But if we want to grab more bits than we'll
		     * find on that line, we have to back up 1 word further.
		     * On the other hand, if the whole tile fits in 1 word,
		     * let's skip the work */ 
		    endinc = stwidth - 1 - w / PPW;

		    if(endinc)
		    {
			if((rem & 0x1f) + w > stippleWidth % PPW)
			    endinc--;
		    }

		    getbits(psrc + endinc, rem & PIM, w, tmpSrc);
		    putbitsrop(tmpSrc, (x & PIM), w, pdst, 
			pGC->planemask, rop);
		    if((x & PIM) + w >= PPW)
			pdst++;
		}

		else if(((x & PIM) + w) < PPW)
		{
		    getbits(psrc, 0, w, tmpSrc);
		    putbitsrop(tmpSrc, x & PIM, w, pdst, 
			pGC->planemask, rop);
		}
		else
		{
		    maskbits(x, w, startmask, endmask, nlMiddle);

	            if (startmask)
		        nstart = PPW - (x & PIM);
	            else
		        nstart = 0;
	            if (endmask)
	                nend = (x + w)  & PIM;
	            else
		        nend = 0;

	            srcStartOver = nstart > PLST;

		    if(startmask)
		    {
			getbits(psrc, 0, nstart, tmpSrc);
			putbitsrop(tmpSrc, (x & PIM), nstart, pdst, 
			    pGC->planemask, rop);
			pdst++;
			if(srcStartOver)
			    psrc++;
		    }
		     
		    while(nlMiddle--)
		    {
			/*
			    getbits(psrc, nstart, PPW, tmpSrc);
			    putbitsrop( tmpSrc, 0, PPW,
				pdst, pGC->planemask, rop );
			*/
			switch ( pGC->fillStyle ) {
			    case FillStippled:
				getstipplepixels( psrc, j, 4, 1, pdst, tmp1 );
				break;
			    case FillOpaqueStippled:
				getstipplepixels( psrc, j, 4, 1, pdst, tmp1 );
				break;
			}
			pdst++;
			psrc++;
		    }
		    if(endmask)
		    {
			getbits(psrc, nstart, nend, tmpSrc);
			putbitsrop(tmpSrc, 0, nend, pdst, 
			    pGC->planemask, rop);
		    }
		 }
		 x += w;
		 width -= w;
	    }
	}
#endif
	ppt++;
	pwidth++;
    }
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}
