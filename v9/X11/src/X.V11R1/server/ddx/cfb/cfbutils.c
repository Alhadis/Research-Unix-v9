/*
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

*/

#include "cfbmskbits.h"

/* cfbQuickBlt -- a quick and dirty bitblit routine
 * copies from psrcBase(xSrc, ySrc) to pdstBase(xDst, yDst) an array of bits
 * w wide by h high.  It is assumed that psrcBase and pdstBase point at 
 * things reasonable to bitblt between. It is assumed that all clipping has
 * been done.  It is also assumed that the rectangle fits on the destination
 * (that is, that (pdstBase + (yDst * h) + w) is a bit in the destination).
 * This routine does no error-checking of any type, CAVEAT HACKER 
 */
cfbQuickBlt(psrcBase, pdstBase, xSrc, ySrc, xDst, yDst, w, h, wSrc, wDst)
    int  *psrcBase, *pdstBase,	/* first bits in pixmaps */
         xSrc, ySrc,		/* origin of source */
	 xDst, yDst,		/* origin for destination */
	 w,			/* width to blt  */
	 h,			/* height to blt */
	 wSrc, wDst;		/* width of pixmaps */
{

    int *psrcLine, *pdstLine;
    int xdir, ydir;
    int nl, startmask, endmask, nlMiddle, *psrc, *pdst, tmpSrc;

    if(psrcBase + wSrc * h < pdstBase ||
       pdstBase + wDst * h < psrcBase)
    {
	/* the areas don't overlap  right and down is fine */
	xdir = ydir = 1;
    }
    else if(ySrc < yDst) /* move right and up */
    {
	xdir = 1;
	ydir = -1;
    }
    else if(ySrc > yDst) /* move right and down */
    {
	xdir = 1;
	ydir = 1;
    }
    else if(xSrc < xDst) /* move left and down */
    {
	xdir = -1;
	ydir = 1;
    }
    else /* if xSrc <= xDst */ /* move right and down */
    {
	xdir = 1;
	ydir = 1;
    }

    if (ydir == -1) /* start at last scanline of rectangle */
    {
	psrcLine = psrcBase + (((ySrc + h) -1) * wSrc);
	pdstLine = pdstBase + (((yDst + h) -1) * wDst);
	wSrc = -wSrc;
	wDst = -wDst;
    }
    else /* start at first scanline */
    {
	psrcLine = psrcBase + (ySrc * wSrc);
	pdstLine = pdstBase + (yDst * wDst);
    }

    /* x direction doesn't matter for < 1 longword */
    if (w <= PPW)
    {
	int srcBit, dstBit;	/* bit offset of src and dst */

	pdstLine += (xDst >> PWSH);
	psrcLine += (xSrc >> PWSH);
	psrc = psrcLine;
	pdst = pdstLine;

	srcBit = xSrc & PIM;
	dstBit = xDst & PIM;

	while(h--)
	{
	    getbits(psrc, srcBit, w, tmpSrc);
/*XXX*/	    putbits(tmpSrc, dstBit, w, pdst, -1);
	    psrc += wSrc;
	    pdst += wDst;
	}
    }
    else
    {
	register int xoffSrc;	/* offset (>= 0, < 32) from which to
				   fetch whole longwords fetched 
				   in src */
	int nstart;		/* number of ragged bits 
				   at start of dst */
	int nend;		/* number of ragged bits at end 
				   of dst */
	int srcStartOver;	/* pulling nstart bits from src
				   overflows into the next word? */

	maskbits(xDst, w, startmask, endmask, nlMiddle);
	if (startmask)
	    nstart = PPW - (xDst & PIM);
	else
	    nstart = 0;
	if (endmask)
	    nend = (xDst + w)  & PIM;
	else
	    nend = 0;

	xoffSrc = ((xSrc & PIM) + nstart) & PIM;
	srcStartOver = ((xSrc & PIM) + nstart) > PLST;

	if (xdir == 1) /* move left to right */
	{
	    pdstLine += (xDst >> PWSH);
	    psrcLine += (xSrc >> PWSH);

	    while (h--)
	    {
		psrc = psrcLine;
		pdst = pdstLine;

		if (startmask)
		{
		    getbits(psrc, (xSrc & PIM), nstart, tmpSrc);
/*XXX*/		    putbits(tmpSrc, (xDst & PIM), nstart, pdst, -1);
		    pdst++;
		    if (srcStartOver)
			psrc++;
		}

		nl = nlMiddle;
		while (nl--)
		{
		    getbits(psrc, xoffSrc, PPW, tmpSrc);
		    *pdst++ = tmpSrc;
		    psrc++;
		}

		if (endmask)
		{
		    getbits(psrc, xoffSrc, nend, tmpSrc);
/*XXX*/		    putbits(tmpSrc, 0, nend, pdst, -1);
		}

		psrcLine += wSrc;
		pdstLine += wDst;
	    }
	}
	else /* move right to left */
	{
	    pdstLine += ((xDst + w) >> PWSH);
	    psrcLine += (xSrc + w >> PWSH);
	    /* if fetch of last partial bits from source crosses
	       a longword boundary, start at the previous longword
	    */
	    if (xoffSrc + nend >= PPW)
		--psrcLine;

	    while (h--)
	    {
		psrc = psrcLine;
		pdst = pdstLine;

		if (endmask)
		{
		    getbits(psrc, xoffSrc, nend, tmpSrc)
/*XXX*/		    putbits(tmpSrc, 0, nend, pdst, -1)
		}

		nl = nlMiddle;
		while (nl--)
		{
		    --psrc;
		    getbits(psrc, xoffSrc, PPW, tmpSrc)
		    *--pdst = tmpSrc;
		}

		if (startmask)
		{
		    if (srcStartOver)
			--psrc;
		    --pdst;
		    getbits(psrc, (xSrc & PIM), nstart, tmpSrc)
/*XXX*/		    putbits(tmpSrc, (xDst & PIM), nstart, pdst, -1)
		}

		pdstLine += wDst;
		psrcLine += wSrc;
	    }
	} /* move right to left */
    }
}
