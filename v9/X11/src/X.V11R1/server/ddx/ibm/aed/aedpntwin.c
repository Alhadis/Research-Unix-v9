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

#include "windowstr.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "mfb.h"
#include "maskbits.h"

#include "xaed.h"
#include "rtutils.h"

void
aedPaintWindowSolid(pWin, pRegion, what)
    WindowPtr pWin;
    RegionPtr pRegion;
    int what;		
{
    int nbox;
    BoxPtr pbox;
    int i;
    short merge;

    TRACE(("aedPaintWindowSolid(pWin= 0x%x, pRegion= 0x%x, what= %d)\n", pWin, pRegion, what));
/*
    miprintRects(pRegion);
*/
    nbox = pRegion->numRects;
    pbox = pRegion->rects;
    if ( nbox == 0 )
	return;
    if (what == PW_BACKGROUND)
    {
	if ( pWin->backgroundPixel )
	    merge = mergexlate[GXset];
        else
	    merge = mergexlate[GXclear];
    } 
    else
    {
	if ( pWin->borderPixel )
	    merge = mergexlate[GXset];
        else
	    merge = mergexlate[GXclear];
    }
    vforce();
    clear(2);
    vikint[ORMERGE] = merge;
    vikint[ORCLIPLX] = pbox->x1;
    vikint[ORCLIPLY] = pbox->y1;
    vikint[ORCLIPHX] = pbox->x2-1;
    vikint[ORCLIPHY] = pbox->y2-1;
    pbox++;
    nbox--;
    vikint[vikoff++] = 10;	/* tile order */
    vikint[vikoff++] = 1024;	/* rectangle width */
    vikint[vikoff++] = 800;	/* rectangle height */
    vikint[vikoff++] = 1;	/* tile height */
    vikint[vikoff++] = 1;	/* tile height */
    vikint[vikoff++] = -1;	/* tile (all ones) */

    vforce();

    vikint[VIKCMD] = 2;	/* reprocess order */
    for(i = 0; i < nbox; i++, pbox++)
    {
	vikint[ORCLIPLX] = pbox->x1;
	vikint[ORCLIPLY] = pbox->y1;
	vikint[ORCLIPHX] = pbox->x2-1;
	vikint[ORCLIPHY] = pbox->y2-1;
	command(ORDATA);
    }
    /* reset clipping window */
    clear(2);
}

void
aedPaintWindowTile(pWin, pRegion, what)
    WindowPtr pWin;
    RegionPtr pRegion;
    int what;		
{
    int nbox;
    BoxPtr pbox;
    int i;
    short merge;
    PixmapPtr pTile;
    int tilelen, tileHeight, tileWidth, skip, j, n;
    unsigned short *bits;
    mfbPrivWin *pPrivWin;

    TRACE(("aedPaintWindowTile(pWin= 0x%x, pRegion= 0x%x, what= %d)\n", pWin, pRegion, what));
    nbox = pRegion->numRects;
    pbox = pRegion->rects;
    if(nbox == 0)
	{
	return;
	}
/*
    miprintRects(pRegion);
*/

    pPrivWin = (mfbPrivWin *)(pWin->devPrivate);

    if (what == PW_BACKGROUND)
    {
	pTile = pPrivWin->pRotatedBackground;
	bits = (unsigned short *)(pPrivWin->pRotatedBackground->devPrivate);
    } 
    else
    {
	pTile = pPrivWin->pRotatedBorder;
	bits = (unsigned short *)(pPrivWin->pRotatedBorder->devPrivate);
    }

    tileWidth = pTile->width;
    tileHeight = pTile->height;
    tilelen = ( tileWidth + 15 ) / 16;

    if ((tileHeight * tilelen ) > 2000) 
    {
	aedBigStippleFillArea(pWin, nbox, pbox, mergexlate[GXcopy], pTile);
	return;
    }

    TRACE(("tileWidth = %d, tileHeight = %d\n", tileWidth, tileHeight));
    vforce();

    clear(5+(tilelen * tileHeight));

    skip = (((tileWidth-1) & 0x1f) < 16);

    vikint[ORMERGE] = mergexlate[GXcopy];
    vikint[ORCLIPLX] = pbox->x1;
    vikint[ORCLIPLY] = pbox->y1;
    vikint[ORCLIPHX] = pbox->x2-1;
    vikint[ORCLIPHY] = pbox->y2-1;
    pbox++;
    nbox--;

    vikint[vikoff++] = 10;	  		/* tile order */
    vikint[vikoff++] = 1024;		/* rectangle width */
    vikint[vikoff++] = 800;	  		/* rectangle height */
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

    vikint[VIKCMD] 	= 2; 		/* reprocess orders */ 
    for(i=0; i<nbox; i++, pbox++)
    {
	vikint[ORCLIPLX] = pbox->x1;
	vikint[ORCLIPLY] = pbox->y1;
	vikint[ORCLIPHX] = pbox->x2-1;
	vikint[ORCLIPHY] = pbox->y2-1;
	command(ORDATA);
    }
    clear(2);

}
