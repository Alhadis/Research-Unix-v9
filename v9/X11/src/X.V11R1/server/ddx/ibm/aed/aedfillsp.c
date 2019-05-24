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
#include "xaed.h"

/* scanline filling for monochrome frame buffer
   written by drewry, oct 1986

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


void aedSolidFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
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

    TRACE(("aedSolidFS(pDrawable= 0x%x, pGC= 0x%x, nInit= %d, ppt= %d, pwidth= 0x%x, fSorted= %d)\n", pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted));

    if (!(pGC->planemask & 1))
	return;

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

    rop = ((mfbPrivGC *)(pGC->devPriv))->rop;
/*************************************************************************/
    /* XXXXX
     * code to fill the spans.  At this point the following is true:
     *		rop will be either RROP_BLACK, RROP_WHITE,
     * 			or RROP_INVERT.  
     *		n = # of spans
     *		ppt points to list of starting points
     *		pwidth points to list of widths
     */

{

short i;
unsigned short x0;
vforce();
clear(9);
switch(rop)
	{
	case RROP_BLACK:   vikint[ORMERGE] = 0; break;
	case RROP_WHITE:   vikint[ORMERGE] =15; break;
	case RROP_INVERT:  vikint[ORMERGE] = 5; break;
	default: ErrorF("aedSolidFS: bad GXalu\n");
		 vikint[ORMERGE] = 15;
		 break;
	}
TRACE(("filling %d spans after clipping\n",n));
if( n <= 0 )
	return;

vikint[ORXPOSN] =  x0 = (unsigned short) ppt->x;	/* move to x0 */
vikint[ORYPOSN] =  (unsigned short) ppt->y;		/* move to y0 */
vikint[ORCLIPHX] = x0 + (unsigned short) *pwidth++ -1 ;
ppt++;

vikint[vikoff++] = 10;	  		/* tile order */
vikint[vikoff++] = 1024;		/* rectangle width */
vikint[vikoff++] = 1;	  		/* rectangle height */
vikint[vikoff++] = 1;	  		/* tile width */
vikint[vikoff++] = 1;	 	 	/* tile height */
vikint[vikoff++] = -1;	  		/* tile (all ones) */

vforce();

for(i=1; i<n; i++)
	{
	vikint[ORXPOSN] = x0 = (unsigned short) ppt->x;	/* move to x0 */
	vikint[ORYPOSN] = (unsigned short) ppt->y;	/* move to y0 */
	ppt++;
	vikint[ORCLIPHX] = x0 + *pwidth++ -1 ;
	vikint[VIKCMD] 	= 2; 		/* reprocess orders */ 
	command(ORDATA);
	}
clear(2);
}
/*************************************************************************/
    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}


/* Fill spans with tiles */
void
aedTileFS(pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted)
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
    PixmapPtr	pTile;		/* pointer to tile we want to fill with */
    int 	tlwidth, tileWidth, rop;
    int *pwidthFree;		/* copies of the pointers to free */
    DDXPointPtr pptFree;
    int tilelen,skip,tileHeight;
    short i,j;
    unsigned short x0;
    unsigned short *bits;

    TRACE(("aedTileFS(pDrawable= 0x%x, pGC= 0x%x, nInit= %d, ppt= %d, pwidth= 0x%x, fSorted= %d)\n", pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted));

    if (!(pGC->planemask & 1))
	return;

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
    if ( n <= 0 )
	return;

    if (pGC->fillStyle == FillTiled)
    {
	pTile = ((mfbPrivGC *)(pGC->devPriv))->pRotatedTile;
	tlwidth = pTile->devKind >> 2;
	rop = pGC->alu;
    }
    else
    {
	pTile = ((mfbPrivGC *)(pGC->devPriv))->pRotatedStipple;
	tlwidth = pTile->devKind >> 2;
	rop = ((mfbPrivGC *)(pGC->devPriv))->ropOpStip;
    }

    /* XXXXX
     * code to fill the spans.  At this point the following is true:
     *		n = # of spans
     *		ppt points to list of starting points
     *		pwidth points to list of widths
     *		pTile points to the tile bits (pre-rotated)
     *		tlwidth is the width of the (word padded) tile in words
     *		rop is the alu operation to perform between the tile 
     *			and the screen
     */

    tileWidth = pTile->width;
    tileHeight = pTile->height;
    TRACE(("tileWidth = %d, tileHeight = %d\n", tileWidth, tileHeight));
    vforce();

    vikint[ORMERGE] = mergexlate[rop];
    vikint[ORXPOSN] = x0 = (unsigned short) ppt->x;	/* move to x0 */
    vikint[ORYPOSN] = (unsigned short) ppt->y;	/* move to y0 */
    vikint[ORCLIPHX] = x0 + (unsigned short) *pwidth++ -1 ;
    ppt++;

    bits = (unsigned short *)pTile->devPrivate;

    tilelen = (pTile->width + 15 )/16;
    if ((tileHeight * tilelen) > 2000) 
    {
	ErrorF("aedTileFS: tile too large\n");
	tileWidth = 1;
	tileHeight = 1;
	tilelen = 1;
    }
    skip = (((tileWidth-1) & 0x1f) < 16);
    TRACE(("skip = %d\n", skip));
	
    clear(5+(tilelen * tileHeight));

    vikint[vikoff++] = 10;	  		/* tile order */
    vikint[vikoff++] = 1024;		/* rectangle width */
    vikint[vikoff++] = 1;	  		/* rectangle height */
    vikint[vikoff++] = tileWidth;		/* tile width */
    vikint[vikoff++] = tileHeight;	 	/* tile height */
    for(j=0; j<tileHeight; j++)
    {
	for (i=0; i<tilelen; i++) 
	    vikint[vikoff++] = *bits++;
	if (skip) 
	    bits++; /* skip the last 16 pad bits */
    }

    vforce();

    for(i=1; i<n; i++)
    {
	vikint[ORXPOSN] = x0 = (unsigned short) ppt->x;	/* move to x0 */
	vikint[ORYPOSN] = (unsigned short) ppt->y;	/* move to y0 */
	vikint[ORCLIPHX] = x0 + (unsigned short) *pwidth++ -1 ;
	vikint[VIKCMD] 	= 2; 		/* reprocess orders */ 
	ppt++;
	command(ORDATA);
    }
    clear(2);

    DEALLOCATE_LOCAL(pptFree);
    DEALLOCATE_LOCAL(pwidthFree);
}
