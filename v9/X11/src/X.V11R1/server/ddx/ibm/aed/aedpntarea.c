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
/* $Header: aedpntarea.c,v 1.1 87/09/13 03:35:07 erik Exp $ */
#include "X.h"

#include "windowstr.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"

#include "mfb.h"
#include "maskbits.h"
#include "rtutils.h"
#include "xaed.h"

void aedBigStippleFillArea();

/* 
   the solid fillers are called for rectangles and window backgrounds.
   the boxes are already translated.
   maybe this should always take a pixmap instead of a drawable?

   NOTE:
   iy = ++iy < tileHeight ? iy : 0
is equivalent to iy%= tileheight, and saves a division.
*/


void
aedSolidFillArea(pDraw, nbox, pbox, merge, nop)
    DrawablePtr pDraw;
    int nbox;
    BoxPtr pbox;
    int merge;
    PixmapPtr nop;
{
    int i;
    TRACE(("aedSolidFillArea(pDraw= 0x%x, nbox= %d, pbox=0x%x, merge=%d, nop = 0x%x)\n", pDraw, nbox, pbox, merge, nop));

/*
    miprintRects(pRegion);
*/
    if ( nbox == 0 )
	return;

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



/* stipple a list of boxes

you can use the reduced rasterop for stipples.  if rrop is
black, AND the destination with (not stipple pattern).  if rrop is
white OR the destination with the stipple pattern.  if rrop is invert,
XOR the destination with the stipple pattern.

	MFBSTIPPLEFILLAREA	OPEQ
	mfbStippleFillWhite	|=
	mfbStippleFillBlack	&=~
	mfbStippleFillInvert	^=
*/

void
aedStippleFillArea(pDraw, nbox, pbox, merge, pstipple)
    DrawablePtr pDraw;
    int nbox;
    BoxPtr pbox;
    int merge;
    PixmapPtr pstipple;
{
    int i;
    int tilelen, tileHeight, tileWidth, skip, j, n;
    unsigned short *bits;

    TRACE(("aedStippleFillArea(pDraw= 0x%x, nbox= %d, pbox=0x%x, merge=%d, pstipple = 0x%x)\n", pDraw, nbox, pbox, merge, pstipple));
    if(nbox == 0)
	{
	return;
	}
/*
    miprintRects(pRegion);
*/


    bits = (unsigned short *)(pstipple->devPrivate);

    tileWidth = pstipple->width;
    tileHeight = pstipple->height;
    tilelen = ( tileWidth + 15 ) / 16;
    TRACE(("tileWidth = %d, tileHeight = %d\n", tileWidth, tileHeight));

    if ((tileHeight * tilelen ) > 2000) 
    {
	aedBigStippleFillArea(pDraw, nbox, pbox, merge, pstipple);
	return;
    }

    vforce();

    clear(5+(tilelen * tileHeight));

    skip = (((tileWidth-1) & 0x1f) < 16);

    vikint[ORMERGE] = merge;
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

void
aedBigStippleFillArea(pDraw, nbox, pbox, merge, pstipple)
    DrawablePtr pDraw;
    int nbox;
    BoxPtr pbox;
    int merge;
    PixmapPtr pstipple;
{
    int i;
    int tilelen, tileHeight, tileWidth, skip, j, n;
    unsigned short *bits;
    int chunkHeight, copyHeight, tileYPos, fillHeight, tilewords;
    int tilePassedHeight;
    int chunkYPos;
    int k, h, y;

    vforce();
    clear(2);

    bits = (unsigned short *)(pstipple->devPrivate);

    tileWidth = pstipple->width;
    tileHeight = pstipple->height;
    tilelen = ( tileWidth + 15 ) / 16;
    tilewords = ( tileWidth + 31 ) / 32;

    chunkHeight = 2000/tilelen;


    for( i = 0 ; i < nbox; i++, pbox++ )
    {
	y = pbox->y1;
	h = pbox->y2 - pbox->y1;

	while ( h > 0 )
	{
	    vikint[ORMERGE] = merge;
	    tileYPos = y%tileHeight;
	    chunkYPos = (tileYPos/chunkHeight)*chunkHeight;
	    bits = (unsigned short *)(pstipple->devPrivate) + chunkYPos*tilewords*2;
	    if ( ( chunkYPos + chunkHeight ) > tileHeight )
		copyHeight = tileHeight - chunkYPos;
	    else
		copyHeight = chunkHeight;

	    if ( ( y % chunkHeight ) != 0 )
		{
		if ( ( fillHeight = chunkHeight - ( y % chunkHeight ) ) > copyHeight )
		    fillHeight = copyHeight;
		}
	    else
		fillHeight = copyHeight;

	    if( ( y + fillHeight ) > pbox->y2 )
		fillHeight = pbox->y2 - y;

    	    vikint[ORXPOSN] = pbox->x1;
    	    vikint[ORYPOSN] = y;
	    vikint[vikoff++] = 10;	  		/* tile order */
	    vikint[vikoff++] = pbox->x2 - pbox->x1;	/* rectangle width */
	    vikint[vikoff++] = fillHeight;  		/* rectangle height */
	    vikint[vikoff++] = tileWidth;		/* tile width */
	    vikint[vikoff++] = chunkHeight;	 	/* tile height */

	    if ( ( tilewords * 2 ) == tilelen )
	    {
		bcopy(bits, &vikint[vikoff], copyHeight*tilelen*2);
		vikoff+=(tilelen*copyHeight);
	    }
	    else
	    {
		for (j = 0 ; j < copyHeight; j++)
		{
		    for ( k = 0; k < tilelen; k++ )
			vikint[vikoff++] = *bits++;
		    bits++;
		}
	    }
	    vforce();
	    clear(2);
	    h = h - fillHeight;
	    y = y + fillHeight;
	}
    }
}
	    




