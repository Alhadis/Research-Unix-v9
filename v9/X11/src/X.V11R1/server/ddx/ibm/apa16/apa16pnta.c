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
/* $Header: apa16pnta.c,v 1.2 87/09/13 03:20:17 erik Exp $ */
#include "X.h"

#include "windowstr.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "gcstruct.h"

#include "mfb.h"
#include "maskbits.h"

#include "rtutils.h"

#include "apa16hdwr.h"

/* 
   the solid fillers are called for rectangles and window backgrounds.
   the boxes are already translated.
   8/4/87 -- modified for RT/apa16 by erik

   NOTE:
   iy = ++iy < tileHeight ? iy : 0
is equivalent to iy%= tileheight, and saves a division.
*/

void
apa16SolidFillArea(pDraw, nbox, pbox, rrop, nop)
    DrawablePtr pDraw;
    int nbox;
    BoxPtr pbox;
    int rrop;
    PixmapPtr nop;
{
    unsigned cmd;

    if (pDraw->type != DRAWABLE_WINDOW)
    {
	if (rrop==RROP_BLACK)	
	    mfbSolidBlackArea(pDraw,nbox,pbox,rrop,nop);
	else if (rrop==RROP_WHITE)
	    mfbSolidWhiteArea(pDraw,nbox,pbox,rrop,nop);
	else if (rrop==RROP_INVERT)
	    mfbSolidInvertArea(pDraw,nbox,pbox,rrop,nop);
	return;
    }
    QUEUE_RESET();
    APA16_GET_CMD(ROP_RECT_FILL,rrop,cmd);

    while (nbox--)
    {
	register int w,h;

	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	if (w==0)	w=1;
	if (h==0)	h=1;

	FILL_RECT(cmd,pbox->x2,pbox->y2,w,h);
    }
    APA16_GO();
}



/* stipple a list of boxes

you can use the reduced rasterop for stipples.  if rrop is
black, AND the destination with (not stipple pattern).  if rrop is
white OR the destination with the stipple pattern.  if rrop is invert,
XOR the destination with the stipple pattern.

*/

void
apa16StippleFillArea(pDraw, nbox, pbox, rrop, pstipple)
    DrawablePtr pDraw;
    int nbox;
    BoxPtr pbox;
    int rrop;
    PixmapPtr pstipple;
{
    register unsigned int *psrc;
			/* pointer to bits in tile, if needed */
    int tileHeight;	/* height of the tile */
    register unsigned int srcpix;	

    int nlwidth;	/* width in longwords of the drawable */
    int w;		/* width of current box */
    register int h;	/* height of current box */
    int startmask;
    int endmask;	/* masks for reggedy bits at either end of line */
    int nlwMiddle;	/* number of longwords between sides of boxes */
    register int nlwExtra;	
		        /* to get from right of box to left of next span */
    
    register int nlw;	/* loop version of nlwMiddle */
    register unsigned int *p;	/* pointer to bits we're writing */
    int iy;		/* index of current scanline in tile */


    unsigned int *pbits;	/* pointer to start of drawable */

    if (pDraw->type == DRAWABLE_WINDOW)
    {
	pbits = (unsigned int *)
		(((PixmapPtr)(pDraw->pScreen->devPrivate))->devPrivate);
	nlwidth = (int)
		(((PixmapPtr)(pDraw->pScreen->devPrivate))->devKind) >> 2;
    }
    else {
	if (rrop==RROP_WHITE)
		mfbStippleWhiteArea(pDraw, nbox, pbox, rrop, pstipple);
	else if (rrop==RROP_BLACK)
		mfbStippleBlackArea(pDraw, nbox, pbox, rrop, pstipple);
	else if (rrop==RROP_INVERT)
		mfbStippleInvertArea(pDraw, nbox, pbox, rrop, pstipple);
	return;
    }

    if		(rrop == RROP_BLACK)	SET_MERGE_MODE(MERGE_BLACK);
    else if	(rrop == RROP_WHITE)	SET_MERGE_MODE(MERGE_WHITE);
    else if	(rrop == RROP_INVERT)	SET_MERGE_MODE(MERGE_INVERT);
    else if	(rrop == RROP_NOP)	return;
    else {
	ErrorF("Unexpected rrop %d in apa16StippleFillArea\n",rrop);
    }

    tileHeight = pstipple->height;
    psrc = (unsigned int *)(pstipple->devPrivate);

    while (nbox--)
    {
	w = pbox->x2 - pbox->x1;
	h = pbox->y2 - pbox->y1;
	iy = pbox->y1 % tileHeight;
	p = pbits + (pbox->y1 * nlwidth) + (pbox->x1 >> 5);

	if ( ((pbox->x1 & 0x1f) + w) < 32)
	{
	    maskpartialbits(pbox->x1, w, startmask);
	    nlwExtra = nlwidth;
	    while (h--)
	    {
		srcpix = psrc[iy];
		iy = ++iy < tileHeight ? iy : 0;
		*p = (srcpix & startmask);
		p += nlwExtra;
	    }
	}
	else
	{
	    maskbits(pbox->x1, w, startmask, endmask, nlwMiddle);
	    nlwExtra = nlwidth - nlwMiddle;

	    if (startmask && endmask)
	    {
		nlwExtra -= 1;
		while (h--)
		{
		    srcpix = psrc[iy];
		    iy = ++iy < tileHeight ? iy : 0;
		    nlw = nlwMiddle;
		    *p = (srcpix & startmask);
		    p++;
		    while (nlw--)
		    {
			*p = srcpix;
			p++;
		    }
		    *p = (srcpix & endmask);
		    p += nlwExtra;
		}
	    }
	    else if (startmask && !endmask)
	    {
		nlwExtra -= 1;
		while (h--)
		{
		    srcpix = psrc[iy];
		    iy = ++iy < tileHeight ? iy : 0;
		    nlw = nlwMiddle;
		    *p = (srcpix & startmask);
		    p++;
		    while (nlw--)
		    {
			*p = srcpix;
			p++;
		    }
		    p += nlwExtra;
		}
	    }
	    else if (!startmask && endmask)
	    {
		while (h--)
		{
		    srcpix = psrc[iy];
		    iy = ++iy < tileHeight ? iy : 0;
		    nlw = nlwMiddle;
		    while (nlw--)
		    {
			*p = srcpix;
			p++;
		    }
		    *p = (srcpix & endmask);
		    p += nlwExtra;
		}
	    }
	    else /* no ragged bits at either end */
	    {
		while (h--)
		{
		    srcpix = psrc[iy];
		    iy = ++iy < tileHeight ? iy : 0;
		    nlw = nlwMiddle;
		    while (nlw--)
		    {
			*p = srcpix;
			p++;
		    }
		    p += nlwExtra;
		}
	    }
	}
        pbox++;
    }
    SET_MERGE_MODE(MERGE_COPY);
}


