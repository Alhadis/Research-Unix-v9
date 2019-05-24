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
/*
 * Push Pixels Routines for the AED
 * by Jeff Weinstein 8/21/87
 */

#include "X.h"
#include "Xmd.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "windowstr.h"
#include "gcstruct.h"
#include "mfb.h"
#include "xaed.h"
#include "rtutils.h"


/* aedPushPixels -- squeegees the forground color of pGC through pBitMap
 * into pDrawable.  pBitMap is a stencil (dx by dy of it is used, it may
 * be bigger) which is placed on the drawable at xOrg, yOrg.  Where a 1 bit
 * is set in the bitmap, the fill style is put onto the drawable using
 * the GC's logical function. The drawable is not changed where the bitmap
 * has a zero bit or outside the area covered by the stencil.
 */
void
aedPushPixSolid(pGC, pBitMap, pDrawable, dx, dy, xOrg, yOrg)
    GCPtr	pGC;
    PixmapPtr	pBitMap;
    DrawablePtr pDrawable;
    int		dx, dy, xOrg, yOrg;
{
    GCPtr tmpGC;
    char * origPix, workPix;
    mfbPrivGCPtr devPriv;
    WindowPtr pWin;
    int alu;

    if ( pDrawable->type != DRAWABLE_WINDOW )
	mfbPushPixels(pGC, pBitMap, pDrawable, dx, dy, xOrg, yOrg);
    TRACE(("aedPushPixSolid(pGC=0x%x, pBitMap=0x%x, pDrawable=0x%x, dx=%d, dy=%d, xOrg=%d, yOrg=%d)\n", pGC, pBitMap, pDrawable, dx, dy, xOrg, yOrg));

    pWin = (WindowPtr)pDrawable;

    devPriv = (mfbPrivGC *)(pGC->devPriv);
    switch ( devPriv->rop )
    {
	case RROP_BLACK:
	    alu = GXandInverted;
	    break;
	case RROP_WHITE:
	    alu = GXor;
	    break;
	case RROP_INVERT:
	    alu = GXxor;
	    break;
	case RROP_NOP:
	    return;
    }
/*
    ErrorF("gcalu = %d, rop = %d, alu = %d\n", pGC->alu, devPriv->rop, alu);
    if (pGC->fgPixel)
    {
	alu = InverseAlu[alu];
	ErrorF("Inverted alu = %d\n", alu);
    }
*/
    aedDrawImage(pBitMap, alu, devPriv->pCompositeClip, xOrg, yOrg, dx, dy);
}

void
aedDrawImage( pPix, alu, pReg, x, y, w, h )
    PixmapPtr pPix;
    int alu;
    RegionPtr pReg;
    int x, y, h, w;
{
    BoxPtr pbox;
    int nbox;
    short *pline;
    int linewidth;
    int imagewidth;
    int i;

    TRACE(("aedDrawImage( pPix=0x%x, alu=%d, pReg=0x%x, x=%d, y=%d, h=%d, w=%d )\n", pPix, alu, pReg, x, y, h, w ));

    pbox = pReg->rects;
    nbox = pReg->numRects;
    if( nbox == 0 )
	return;


    vforce();
    clear(2);
    vikint[ORMERGE] = mergexlate[alu];
    vikint[ORXPOSN] = x;
    vikint[ORYPOSN] = y;
    vikint[ORCLIPLX] = pbox->x1; 
    vikint[ORCLIPLY] = pbox->y1; 
    vikint[ORCLIPHX] = pbox->x2-1; 
    vikint[ORCLIPHY] = pbox->y2-1; 
    pbox++;
    nbox++;
    vikint[vikoff++] = 9;	/* draw image order */
    vikint[vikoff++] = w;	/* image width */
    vikint[vikoff++] = h;	/* image height */

    linewidth = pPix->devKind >> 1;
    pline = pPix->devPrivate;
    imagewidth = (w + 15)/16;
    for ( i = 0; i < h; i++ )
    {
	bcopy(pline, &vikint[vikoff], imagewidth*2);
	vikoff = vikoff + imagewidth;
	pline = pline + linewidth;
    }
    vforce();
    vikint[VIKCMD] = 2;	/* reprocess orders command */
    for( i = 0; i < nbox; i++, pbox++ )
    {
	vikint[ORCLIPLX] = pbox->x1; 
	vikint[ORCLIPLY] = pbox->y1; 
	vikint[ORCLIPHX] = pbox->x2-1; 
	vikint[ORCLIPHY] = pbox->y2-1; 
	command(ORDATA);
    }
    clear(2);
}
	
