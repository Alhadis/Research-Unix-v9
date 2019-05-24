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

#include "misc.h"
#include "regionstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "mfb.h"
#include "maskbits.h"
#include "rtutils.h"


/* SetSpans -- for each span copy pwidth[i] bits from psrc to pDrawable at
 * ppt[i] using the raster op from the GC.  If fSorted is TRUE, the scanlines
 * are in increasing Y order.
 * Source bit lines are server scanline padded so that they always begin
 * on a word boundary.
 */ 
void
aedSetSpans(pDrawable, pGC, psrc, pptInit, pwidthInit, nInit, fSorted)
    DrawablePtr		pDrawable;
    GCPtr		pGC;
    int			*psrc;
    register DDXPointPtr pptInit;
    int			*pwidthInit;
    int			nInit;
    int			fSorted;
{
    int 		*pdstBase;	/* start of dst bitmap */
    int 		widthDst;	/* width of bitmap in words */
    register BoxPtr 	pbox, pboxLast, pboxTest;
    register DDXPointPtr pptLast;
    DDXPointPtr		ppt, pptT;
    int 		alu;
    RegionPtr 		prgnDst;
    int			xStart, xEnd;
    int			yMax;
    int			n;		/* number of spans after clipping */
    DDXPointPtr	pptFree;
    int			*pwidthFree;
    int			*pwidth;
    short		*bits;

    
    TRACE(("aedSetSpans(pDrawable= 0x%x, pGC= 0x%x, psrc= 0x%x, pptInit= 0x%x, pwidthInit= 0x%x, nInit= %d, fSorted= %d)\n", pDrawable, pGC, psrc, pptInit, pwidthInit, nInit, fSorted));
/*
    pptT = pptInit;
    while(pptT < pptInit+nInit)
    {
	pptT->x += ((WindowPtr)pDrawable)->absCorner.x;
	pptT->y += ((WindowPtr)pDrawable)->absCorner.y;
	pptT++;
    }
*/
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

/******************************************************************/
    /* XXXXX
     * code to set the spans.  At this point the following is true:
     *		n = # of spans
     *		ppt points to list of starting points
     *		pwidth points to list of widths
     *		pGC->alu = the merge function
     *		psrc points to the source bits.  The spans are padded to
     *		32 bit boundaries, and are consecutive in memory
     *		NOTE: this does not need the stipple or tile
     */

{
#include "xaed.h"

int i,j, outlen,skip, merge;

vforce();
clear(2);
merge = mergexlate[pGC->alu];

bits = (short *) psrc;
for(i=0; i<n; i++)
	{
	outlen = ((*pwidth + 15)/ 16); /* one line at a time (!) */
/*
	skip =  ((((*pwidth)-1) & 0x1f) < 16);
*/
	skip = outlen & 1;

	clear(6+outlen);
	vikint[ORMERGE] = merge;
        vikint[vikoff++] = 5; 	/* move absolute */
	vikint[vikoff++] = ppt->x;	/* x0 */
	vikint[vikoff++] = ppt->y;	/* y0 */
	ppt++;
		
	/* build image order */
	vikint[vikoff++] = 9;		/* draw image */
	vikint[vikoff++] = *pwidth++;	/* image width */
	vikint[vikoff++] = 1;		/* image height */

	for (j=0; j<outlen; j++) vikint[vikoff++] = *bits++;
	if (skip) bits++; /* skip the last 16 pad bits */
	}
vforce();
clear(2);
}
    
/******************************************************************/

    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);

}

