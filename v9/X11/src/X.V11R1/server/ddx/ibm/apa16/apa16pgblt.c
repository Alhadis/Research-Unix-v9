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

/* $Header: apa16pgblt.c,v 5.6 87/09/13 03:19:35 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/apa16/RCS/apa16pgblt.c,v $ */

#ifndef lint
static char *rcsid = "$Header: apa16pgblt.c,v 5.6 87/09/13 03:19:35 erik Exp $";
#endif

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "scrnintstr.h"
#include "regionstr.h"
#include "mfb.h"
#include "maskbits.h"
#include "miscstruct.h"

#include "rtutils.h"

#include "apa16hdwr.h"

/*
    we should eventually special-case fixed-width fonts, although
its more important for ImageText, which is meant for terminal
emulators.

    this works for fonts with glyphs <= 32 bits wide.

    the clipping calculations are done for worst-case fonts.
we make no assumptions about the heights, widths, or bearings
of the glyphs.  if we knew that the glyphs are all the same height,
we could clip the tops and bottoms per clipping box, rather
than per character per clipping box.  if we knew that the glyphs'
left and right bearings were wlle-behaved, we could clip a single
character at the start, output until the last unclipped
character, and then clip the last one.  this is all straightforward
to determine based on max-bounds and min-bounds from the font.
    there is some inefficiency introduced in the per-character
clipping to make what's going on clearer.

    (it is possible, for example, for a font to be defined in which the
next-to-last character in a font would be clipped out, but the last
one wouldn't.  the code below deals with this.)

    PolyText looks at the fg color and the rasterop; mfbValidateGC
swaps in the right routine after looking at the reduced ratserop
in the private field of the GC.  

*/

void
apa16PolyGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GCPtr	pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    char 	*pglyphBase;	/* start of array of glyphs */
{
    ExtentInfoRec info;	/* used by QueryGlyphExtents() */
    BoxRec bbox;		/* string's bounding box */

    CharInfoPtr pci;
    int xorg, yorg;	/* origin of drawable in bitmap */
    int widthDst;	/* width of dst in longwords */

			/* these keep track of the character origin */
    unsigned int *pdstBase;
			/* points to longword with character origin */
    int xchar;		/* xorigin of char (mod 32) */

			/* these are used for placing the glyph */
    int xoff;		/* x offset of left edge of glyph (mod 32) */
    unsigned int *pdst;	/* pointer to current longword in dst */

    int w;		/* width of glyph in bits */
    int h;		/* height of glyph */
    int widthGlyph;	/* width of glyph, in bytes */
    char *pglyph;	/* pointer to current row of glyph */

    int startmask;	/* used for putting down glyph */
    int endmask;
    int nFirst;		/* bits of glyph in current longword */
    int tmpSrc;		/* for getting bits from glyph */
    int	rrop;

    TRACE(("apa16PolyGlyphBlt(pDrawable= 0x%x, pGC= 0x%x, x= %d, y= %d, nglyph= %d, ppci= %d, pglyphBase= 0x%x)\n",pDrawable,pGC,x,y,nglyph,ppci,pglyphBase));

    if (!(pGC->planemask & 1))
	return;

    rrop= ((mfbPrivGC *)(pGC->devPriv))->rop;
    if (rrop==RROP_NOP)
	return;

    if (pDrawable->type != DRAWABLE_WINDOW)
    {
	if (rrop==RROP_WHITE) 
	    mfbPolyGlyphBltWhite(pDrawable,pGC,x,y,nglyph,ppci,pglyphBase);
	else if (rrop==RROP_BLACK)
	    mfbPolyGlyphBltBlack(pDrawable,pGC,x,y,nglyph,ppci,pglyphBase);
	else if (rrop==RROP_INVERT)
	    mfbPolyGlyphBltInvert(pDrawable,pGC,x,y,nglyph,ppci,pglyphBase);
	else  {
	    ErrorF("unexpected rrop %d in apa16PolyGlyphBlt\n",rrop);
	    ErrorF("calling miPolyGlyphBlt\n");
	    miPolyGlyphBlt(pDrawable,pGC,x,y,nglyph,ppci,pglyphBase);
	}
	return;
    }

    if (rrop==RROP_WHITE)	SET_MERGE_MODE(MERGE_WHITE);
    else if (rrop==RROP_BLACK)	SET_MERGE_MODE(MERGE_BLACK);
    else if (rrop==RROP_INVERT)	SET_MERGE_MODE(MERGE_INVERT);
    else if (rrop==RROP_NOP)	return;
    else {
	ErrorF("Unexpected rrop %d in apa16PolyGlyphBlt\n",rrop);
	ErrorF("calling miPolyGlyphBlt\n");
	miPolyGlyphBlt(pDrawable,pGC,x,y,nglyph,ppci,pglyphBase);
    }
    xorg = ((WindowPtr)pDrawable)->absCorner.x;
    yorg = ((WindowPtr)pDrawable)->absCorner.y;
    pdstBase= (unsigned int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate);
    widthDst = (int)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind)>>2;

    x += xorg;
    y += yorg;

    QueryGlyphExtents(pGC->font, ppci, nglyph, &info);
    bbox.x1 = x + info.overallLeft;
    bbox.x2 = x + info.overallRight;
    bbox.y1 = y - info.overallAscent;
    bbox.y2 = y + info.overallDescent;

    switch ((*pGC->pScreen->RectIn)(
			((mfbPrivGC *)(pGC->devPriv))->pCompositeClip, &bbox))
    {
      case rgnOUT:
	break;
      case rgnIN:
        pdstBase = pdstBase + (widthDst * y) + (x >> 5);
        xchar = x & 0x1f;

        while(nglyph--)
        {
	    pci = *ppci;
	    pglyph = pglyphBase + pci->byteOffset;
	    w = GLYPHWIDTHPIXELS(pci);
	    h = GLYPHHEIGHTPIXELS(pci);
	    widthGlyph = GLYPHWIDTHBYTESPADDED(pci);

	    /* start at top scanline of glyph */
	    pdst = pdstBase - (pci->metrics.ascent * widthDst);

	    /* find correct word in scanline and x offset within it
	       for left edge of glyph
	    */
	    xoff = xchar + pci->metrics.leftSideBearing;
	    while (xoff > 31)
	    {
	        pdst++;
	        xoff -= 32;
	    }
	    while (xoff < 0)
	    {
	        xoff += 32;
	        pdst--;
	    }

	    if ((xoff + w) <= 32)
	    {
	        /* glyph all in one longword */
	        maskpartialbits(xoff, w, startmask);
	        while (h--)
	        {
		    getleftbits((int *)pglyph, w, tmpSrc);
		    *pdst = (SCRRIGHT(tmpSrc, xoff) & startmask);
		    pglyph += widthGlyph;
		    pdst += widthDst;
	        }
	    }
	    else if (w<=32)
	    {
	        /* glyph crosses longword boundary */
	        mask32bits(xoff, w, startmask, endmask);
	        nFirst = 32 - xoff;
	        while (h--)
	        {
		    getleftbits((int *)pglyph, w, tmpSrc);
		    *pdst = (SCRRIGHT(tmpSrc, xoff) & startmask);
		    *(pdst+1) = (SCRLEFT(tmpSrc, nFirst) & endmask);
		    pglyph += widthGlyph;
		    pdst += widthDst;
	        }
	    } else {
		int	nlw,row,left,right;
		unsigned *tmpGlyph,tmpBits;

		maskbits(xoff,w,startmask,endmask,nlw);
		mask32bits(xoff,32,right,left);
	        nFirst = 32 - xoff;
	        while (h--)
	        {
		    getleftbits((tmpGlyph=(unsigned *)pglyph), 32, tmpSrc);
		    *pdst = (SCRRIGHT(tmpSrc, xoff) & startmask);
		    for (row=1;row<=nlw;row++) {
			tmpBits= (SCRLEFT(tmpSrc,nFirst)&left);
			tmpGlyph++;
			getleftbits(tmpGlyph,32,tmpSrc);
			tmpBits|= SCRRIGHT(tmpSrc,xoff)&right;
			*(pdst+row)= tmpBits;
		    }
		    getleftbits((tmpGlyph+1),32,tmpBits);
		    tmpSrc= (SCRLEFT(tmpSrc,nFirst)&left)|
						(SCRRIGHT(tmpBits,xoff)&right);
		    *(pdst+nlw+1) = (tmpSrc & endmask);
		    pglyph += widthGlyph;
		    pdst += widthDst;
	        }
	    }

	    /* update character origin */
	    x += pci->metrics.characterWidth;
	    xchar += pci->metrics.characterWidth;
	    while (xchar > 31)
	    {
	        xchar -= 32;
	        pdstBase++;
	    }
	    while (xchar < 0)
	    {
	        xchar += 32;
	        pdstBase--;
	    }
	    ppci++;
        } /* while nglyph-- */
	break;
      case rgnPART:
      {
	TEXTPOS *ppos;
	int nbox;
	BoxPtr pbox;
	int xpos;		/* x position of char origin */
	int i;
	BoxRec clip;
	int leftEdge, rightEdge;
	int topEdge, bottomEdge;
	int glyphRow;		/* first row of glyph not wholly
				   clipped out */
	int glyphCol;		/* leftmost visible column of glyph */

	if(!(ppos = (TEXTPOS *)ALLOCATE_LOCAL(nglyph * sizeof(TEXTPOS))))
	    return;

        pdstBase = pdstBase + (widthDst * y) + (x >> 5);
        xpos = x;
	xchar = xpos & 0x1f;

	for (i=0; i<nglyph; i++)
	{
	    pci = ppci[i];

	    ppos[i].xpos = xpos;
	    ppos[i].xchar = xchar;
	    ppos[i].leftEdge = xpos + pci->metrics.leftSideBearing;
	    ppos[i].rightEdge = xpos + pci->metrics.rightSideBearing;
	    ppos[i].topEdge = y - pci->metrics.ascent;
	    ppos[i].bottomEdge = y + pci->metrics.descent;
	    ppos[i].pdstBase = pdstBase;
	    ppos[i].widthGlyph = GLYPHWIDTHBYTESPADDED(pci);

	    xpos += pci->metrics.characterWidth;
	    xchar += pci->metrics.characterWidth;
	    while (xchar > 31)
	    {
		xchar -= 32;
		pdstBase++;
	    }
	    while (xchar < 0)
	    {
		xchar += 32;
		pdstBase--;
	    }
	}

	pbox = ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip->rects;
	nbox = ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip->numRects;

	/* HACK ALERT
	   since we continue out of the loop below so often, it
	   is easier to increment pbox at the  top than at the end.
	   don't try this at home.
	*/
	pbox--;
	while(nbox--)
	{
	    pbox++;
	    clip.x1 = max(bbox.x1, pbox->x1);
	    clip.y1 = max(bbox.y1, pbox->y1);
	    clip.x2 = min(bbox.x2, pbox->x2);
	    clip.y2 = min(bbox.y2, pbox->y2);
	    if ((clip.x2<=clip.x1) || (clip.y2<=clip.y1))
		continue;

	    for(i=0; i<nglyph; i++)
	    {
		pci = ppci[i];
		xchar = ppos[i].xchar;

		/* clip the left and right edges */
		if (ppos[i].leftEdge < clip.x1)
		    leftEdge = clip.x1;
		else
		    leftEdge = ppos[i].leftEdge;

		if (ppos[i].rightEdge > clip.x2)
		    rightEdge = clip.x2;
		else
		    rightEdge = ppos[i].rightEdge;

		w = rightEdge - leftEdge;
		if (w <= 0)
		    continue;

		/* clip the top and bottom edges */
		if (ppos[i].topEdge < clip.y1)
		    topEdge = clip.y1;
		else
		    topEdge = ppos[i].topEdge;

		if (ppos[i].bottomEdge > clip.y2)
		    bottomEdge = clip.y2;
		else
		    bottomEdge = ppos[i].bottomEdge;

		h = bottomEdge - topEdge;
		if (h <= 0)
		    continue;

		glyphRow = (topEdge - y) + pci->metrics.ascent;
		widthGlyph = ppos[i].widthGlyph;
		pglyph = pglyphBase + pci->byteOffset;
		pglyph += (glyphRow * widthGlyph);

		pdst = ppos[i].pdstBase - ((y-topEdge) * widthDst);

		glyphCol = (leftEdge - ppos[i].xpos) -
			   (pci->metrics.leftSideBearing);
		xoff = xchar + (leftEdge - ppos[i].xpos);
		while (xoff > 31)
		{
		    xoff -= 32;
		    pdst++;
		}
		while  (xoff < 0)
		{
		    xoff += 32;
		    pdst--;
		}

		if ((xoff + w) <= 32)
		{
		    maskpartialbits(xoff, w, startmask);
		    while (h--)
		    {
			getshiftedleftbits((int *)pglyph, glyphCol, w, tmpSrc);
			*pdst = (SCRRIGHT(tmpSrc, xoff) & startmask);
			pglyph += widthGlyph;
			pdst += widthDst;
		    }
		}
		else if (w<=32)
		{
		    mask32bits(xoff, w, startmask, endmask);
		    nFirst = 32 - xoff;
		    while (h--)
		    {
			getshiftedleftbits((int *)pglyph, glyphCol, w, tmpSrc);
			*pdst = (SCRRIGHT(tmpSrc, xoff) & startmask);
			*(pdst+1) = (SCRLEFT(tmpSrc, nFirst) & endmask);
			pglyph += widthGlyph;
			pdst += widthDst;
		    }
		} else {
		    int	nlw,row,left,right;
		    unsigned *tmpGlyph,tmpBits;

		    maskbits(xoff,w,startmask,endmask,nlw);
		    mask32bits(xoff,32,right,left);
		    nFirst = 32 - xoff;
		    while (h--)
		    {
			getleftbits((tmpGlyph=(unsigned *)pglyph), 32, tmpSrc);
			*pdst = (SCRRIGHT(tmpSrc, xoff) & startmask);
			for (row=1;row<=nlw;row++) {
			    tmpBits= (SCRLEFT(tmpSrc,nFirst)&left);
			    tmpGlyph++;
			    getleftbits(tmpGlyph,32,tmpSrc);
			    tmpBits|= SCRRIGHT(tmpSrc,xoff)&right;
			    *(pdst+row)= tmpBits;
			}
			getleftbits((tmpGlyph+1),32,tmpBits);
			tmpSrc= (SCRLEFT(tmpSrc,nFirst)&left)|
						(SCRRIGHT(tmpBits,xoff)&right);
			*(pdst+nlw+1) = (tmpSrc & endmask);
			pglyph += widthGlyph;
			pdst += widthDst;
		    }
		} 
	    } /* for each glyph */
	} /* while nbox-- */
	DEALLOCATE_LOCAL(ppos);
	break;
      }
      default:
	break;
    }
    SET_MERGE_MODE(MERGE_COPY);
}


