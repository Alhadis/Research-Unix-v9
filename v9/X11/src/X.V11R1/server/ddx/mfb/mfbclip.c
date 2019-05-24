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
/* $Header: mfbclip.c,v 1.14 87/09/11 07:21:11 toddb Exp $ */
#include "X.h"
#include "miscstruct.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "gc.h"
#include "maskbits.h"

/* Convert bitmap clip mask into clipping region. 
 * First, goes through each line and makes boxes by noting the transitions
 * from 0 to 1 and 1 to 0.
 * Then it coalesces the current line with the previous if they have boxes
 * at the same X coordinates.  We have to keep indices to rectangles rather 
 * than pointers because everything may move if we Xrealloc when adding a
 * rectangle.
 */
RegionPtr
mfbPixmapToRegion(pPix)
    PixmapPtr	pPix;
{
    RegionPtr	pReg;
    register unsigned	*pw, w;
    register int	ib;
    int			width, h, base, rx1, crects;
    unsigned int	*pwLineStart;
    int			irectPrevStart, irectLineStart;
    register BoxPtr	prectO, prectN;
    BoxPtr		FirstRect, rects, prectLineStart;
    Bool		fInBox, fSame;


    if((pReg = (*pPix->drawable.pScreen->RegionCreate)(NULL, 1)) == NullRegion)
	return NullRegion;
    rects = (BoxPtr) Xalloc(sizeof(BoxRec));
    FirstRect = rects;
    width = pPix->width;
    pw = (unsigned int  *)pPix->devPrivate;
    irectPrevStart = -1;
    for(h = 0; h < pPix->height; h++)
    {

	irectLineStart = rects - FirstRect;
	pwLineStart = pw;
	/* If the Screen left most bit of the word is set, we're starting in
	 * a box */
	if(*pw & mask[0])
	{
	    fInBox = TRUE;
	    rx1 = 0;
	}
	else
	    fInBox = FALSE;
	/* Process all words which are fully in the pixmap */
	while(pw  < pwLineStart + width/32)
	{
	    base = (pw - pwLineStart) * 32;
	    w = *pw++;
	    for(ib = 0; ib < 32; ib++)
	    {
	        /* If the Screen left most bit of the word is set, we're
		 * starting a box */
		if(w & mask[0])
		{
		    if(!fInBox)
		    {
			rx1 = base + ib;
			/* start new box */
			fInBox = TRUE;
		    }
		}
		else
		{
		    if(fInBox)
		    {
			/* end box */
			MEMCHECK(pReg, rects, FirstRect);
			ADDRECT(pReg, rects, rx1, h, base + ib, h + 1);
			fInBox = FALSE;
		    }
		}
		/* Shift the word VISUALLY left one. */
		w = SCRLEFT(w, 1);
	    }
	}
	if(width & 0x1F)
	{
	    /* Process final partial word on line */
	    base = (pw - pwLineStart) * 32;
	    w = *pw++;
	    for(ib = 0; ib < width &0x1F; ib++)
	    {
	        /* If the Screen left most bit of the word is set, we're
		 * starting a box */
		if(w & mask[0])
		{
		    if(!fInBox)
		    {
			rx1 = base + ib;
			/* start new box */
			fInBox = TRUE;
		    }
		}
		else
		{
		    if(fInBox)
		    {
			/* end box */
			MEMCHECK(pReg, rects, FirstRect);
			ADDRECT(pReg, rects, rx1, h, base + ib, h + 1);
			fInBox = FALSE;
		    }
		}
		/* Shift the word VISUALLY left one. */
		w = SCRLEFT(w, 1);
	    }
	}
	/* If scanline ended with last bit set, end the box */
	if(fInBox)
	{
	    MEMCHECK(pReg, rects, FirstRect);
	    ADDRECT(pReg, rects, rx1, h, base + ib, h + 1);
	}
	/* if all rectangles on this line have the same x-coords as
	 * those on the previous line, then add 1 to all the previous  y2s and 
	 * throw away all the rectangles from this line 
	 */
	fSame = FALSE;
	if(irectPrevStart != -1)
	{
	    crects = irectLineStart - irectPrevStart;
	    if(crects == ((rects - FirstRect) - irectLineStart))
	    {
	        prectO = FirstRect + irectPrevStart;
	        prectN = prectLineStart = FirstRect + irectLineStart;
		fSame = TRUE;
	        while(prectO < prectLineStart)
		{
		    if((prectO->x1 != prectN->x1) || (prectO->x2 != prectN->x2))
		    {
			  fSame = FALSE;
			  break;
		    }
		    prectO++;
		    prectN++;
		}
		if (fSame)
		{
		    prectO = FirstRect + irectPrevStart;
		    while(prectO < prectLineStart)
		    {
			prectO->y2 += 1;
			prectO++;
		    }
		    rects -= crects;
		    pReg->numRects -= crects;
		}
	    }
	}
	if(!fSame)
	    irectPrevStart = irectLineStart;
    }
    if(pReg->numRects)
    {
	Xfree((char *)pReg->rects);
	pReg->size = pReg->numRects;
	pReg->rects = FirstRect;
    }
    else
    {
	Xfree((char *) FirstRect);
	Xfree((char *) pReg);
	pReg = NullRegion;
    }

    return(pReg);
}

