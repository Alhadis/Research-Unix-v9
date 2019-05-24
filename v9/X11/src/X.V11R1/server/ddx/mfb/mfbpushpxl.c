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
/* $Header: mfbpushpxl.c,v 1.16 87/09/11 07:48:33 toddb Exp $ */

#include "X.h"
#include "gcstruct.h"
#include "screenint.h"
#include "pixmapstr.h"
#include "miscstruct.h"
#include "maskbits.h"

#define NPT 128

/* mfbPushPixels -- squeegees the forground color of pGC through pBitMap
 * into pDrawable.  pBitMap is a stencil (dx by dy of it is used, it may
 * be bigger) which is placed on the drawable at xOrg, yOrg.  Where a 1 bit
 * is set in the bitmap, the fill style is put onto the drawable using
 * the GC's logical function. The drawable is not changed where the bitmap
 * has a zero bit or outside the area covered by the stencil.
 */
void
mfbPushPixels(pGC, pBitMap, pDrawable, dx, dy, xOrg, yOrg)
    GCPtr	pGC;
    PixmapPtr	pBitMap;
    DrawablePtr pDrawable;
    int		dx, dy, xOrg, yOrg;
{
    int		h, dxDiv32, ibEnd;
    unsigned int *pwLineStart;
    register unsigned int	*pw, *pwEnd;
    register unsigned int mask;
    register int ib, w;
    register int ipt;		/* index into above arrays */
    Bool 	fInBox;
    DDXPointRec	pt[NPT];
    int		width[NPT];

    /* Now scan convert the pixmap and use the result to call fillspans in
     * in the drawable with the original GC */
    ipt = 0;
    dxDiv32 = dx/32;
    for(h = 0; h < dy; h++)
    {

	pw = (unsigned int *)
	     (((char *)(pBitMap->devPrivate))+(h * pBitMap->devKind));
	pwLineStart = pw;
	/* Process all words which are fully in the pixmap */
	
	fInBox = FALSE;
	pwEnd = pwLineStart + dxDiv32;
	while(pw  < pwEnd)
	{
	    w = *pw;
	    mask = endtab[1];
	    for(ib = 0; ib < 32; ib++)
	    {
		if(w & mask)
		{
		    if(!fInBox)
		    {
			pt[ipt].x = ((pw - pwLineStart) << 5) + ib + xOrg;
			pt[ipt].y = h + yOrg;
			/* start new box */
			fInBox = TRUE;
		    }
		}
		else
		{
		    if(fInBox)
		    {
			width[ipt] = ((pw - pwLineStart) << 5) + 
				     ib + xOrg - pt[ipt].x;
			if (++ipt >= NPT)
			{
			    (*pGC->FillSpans)(pDrawable, pGC, NPT, pt,
			                      width, TRUE);
			    ipt = 0;
			}
			/* end box */
			fInBox = FALSE;
		    }
		}
		mask = SCRRIGHT(mask, 1);
	    }
	    pw++;
	}
	ibEnd = dx & 0x1F;
	if(ibEnd)
	{
	    /* Process final partial word on line */
	    w = *pw;
	    mask = endtab[1];
	    for(ib = 0; ib < ibEnd; ib++)
	    {
		if(w & mask)
		{
		    if(!fInBox)
		    {
			/* start new box */
			pt[ipt].x = ((pw - pwLineStart) << 5) + ib + xOrg;
			pt[ipt].y = h + yOrg;
			fInBox = TRUE;
		    }
		}
		else
		{
		    if(fInBox)
		    {
			/* end box */
			width[ipt] = ((pw - pwLineStart) << 5) + 
				     ib + xOrg - pt[ipt].x;
			if (++ipt >= NPT)
			{
			    (*pGC->FillSpans)(pDrawable, pGC, NPT, pt,
			                      width, TRUE);
			    ipt = 0;
			}
			fInBox = FALSE;
		    }
		}
		mask = SCRRIGHT(mask, 1);
	    }
	}
	/* If scanline ended with last bit set, end the box */
	if(fInBox)
	{
	    width[ipt] = ((pw - pwLineStart) << 5) + ib + xOrg - pt[ipt].x;
	    if (++ipt >= NPT)
	    {
		(*pGC->FillSpans)(pDrawable, pGC, NPT, pt, width, TRUE);
		ipt = 0;
	    }
	}
    }
    /* Flush any remaining spans */
    if (ipt)
    {
	(*pGC->FillSpans)(pDrawable, pGC, ipt, pt, width, TRUE);
    }
}
