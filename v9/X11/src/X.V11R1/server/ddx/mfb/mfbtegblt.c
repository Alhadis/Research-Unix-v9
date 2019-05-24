/* $Header: mfbtegblt.c,v 1.1 87/09/11 07:48:45 toddb Exp $ */
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
#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"
#include	"mfb.h"
#include	"maskbits.h"

/*
    this works for fonts with glyphs <= 32 bits wide.

    This should be called only with a terminal-emulator font;
this means that the FIXED_METRICS flag is set, and that
glyphbounds == charbounds.

    in theory, this goes faster; even if it doesn't, it reduces the
flicker caused by writing a string over itself with image text (since
the background gets repainted per character instead of per string.)
this seems to be important for some converted X10 applications.

    Image text looks at the bits in the glyph and the fg and bg in the
GC.  it paints a rectangle, as defined in the protocol dcoument,
and the paints the characters.

   to avoid source proliferation, this file is compiled
two times:
	MFBTEGLYPHBLT		OP
	mfbTEGlyphBltWhite		(white text, black bg )
	mfbTEGlyphBltBlack	~	(black text, white bg )

*/

void
MFBTEGLYPHBLT(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    unsigned char *pglyphBase;	/* start of array of glyphs */
{
    CharInfoPtr pci;
    FontInfoPtr pfi = pGC->font->pFI;
    int xorg, yorg;
    int widthDst;
    unsigned int *pdstBase;	/* pointer to longword with top row 
				   of current glyph */

    int w;			/* width of glyph and char */
    int h;			/* height of glyph and char */
    register int xpos;		/* current x%32  */
    int ypos;			/* current y%32 */
    register unsigned char *pglyph;
    int widthGlyph;

    register unsigned int *pdst;/* pointer to current longword in dst */
    int hTmp;			/* counter for height */
    register int startmask, endmask;
    int nfirst;			/* used if glyphs spans a longword boundary */
    register unsigned int tmpSrc;
    BoxRec bbox;		/* for clipping */

    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	xorg = ((WindowPtr)pDrawable)->absCorner.x;
	yorg = ((WindowPtr)pDrawable)->absCorner.y;
	pdstBase = (unsigned int *)
		(((PixmapPtr)(pDrawable->pScreen->devPrivate))->devPrivate);
	widthDst = (int)
		  (((PixmapPtr)(pDrawable->pScreen->devPrivate))->devKind) >> 2;
    }
    else
    {
	xorg = 0;
	yorg = 0;
	pdstBase = (unsigned int *)(((PixmapPtr)pDrawable)->devPrivate);
	widthDst = (int)(((PixmapPtr)pDrawable)->devKind) >> 2;
    }

    xpos = x + xorg;
    ypos = y + yorg;

    pci = &pfi->maxbounds;
    w = pci->metrics.characterWidth;
    h = pfi->fontAscent + pfi->fontDescent;
    widthGlyph = GLYPHWIDTHBYTESPADDED(pci);

    xpos += pci->metrics.leftSideBearing;
    ypos -= pfi->fontAscent;

    bbox.x1 = xpos;
    bbox.x2 = xpos + (w * nglyph);
    bbox.y1 = ypos;
    bbox.y2 = ypos + h;

    switch ((*pGC->pScreen->RectIn)(
                ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip, &bbox))
    {
      case rgnOUT:
	break;
      case rgnPART:
	/* this is the WRONG thing to do, but it works.
	   calling the non-terminal text is easy, but slow, given
	   what we know about the font.

	   the right thing to do is something like:
	    for each clip rectangle
		compute at which row the glyph starts to be in it,
		   and at which row the glyph ceases to be in it
		compute which is the first glyph inside the left
		    edge, and the last one inside the right edge
		draw a fractional first glyph, using only
		    the rows we know are in
		draw all the whole glyphs, using the appropriate rows
		draw any pieces of the last glyph, using the right rows

	   this way, the code would take advantage of knowing that
	   all glyphs are the same height and don't overlap.

	   one day...
	*/
	CLIPTETEXT(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);
	break;
      case rgnIN:
        pdstBase += ((widthDst * ypos) + (xpos >> 5));
        xpos &= 0x1f;
        while(nglyph--)
        {
	    pglyph = pglyphBase + (*ppci++)->byteOffset;
	    hTmp = h;
	    pdst = pdstBase;

	    if ( (xpos+w) < 32)
	    {
	        maskpartialbits(xpos, w, startmask);
	        while(hTmp--)
	        {
		    getleftbits(pglyph, w, tmpSrc);
		    *pdst = (*pdst & ~startmask) |
			    (OP(SCRRIGHT(tmpSrc, xpos)) & startmask);
		    pdst += widthDst;
		    pglyph += widthGlyph;
	        }
	        xpos += w;
	    }
	    else
	    {
	        mask32bits(xpos, w, startmask, endmask);
	        nfirst = 32 - xpos;
	        while(hTmp--)
	        {
		    getleftbits(pglyph, w, tmpSrc);
		    *pdst = (*pdst & ~startmask) |
			    (OP(SCRRIGHT(tmpSrc, xpos)) & startmask);
		    *(pdst+1) = (*(pdst+1) & ~endmask) |
			    (OP(SCRLEFT(tmpSrc, nfirst)) & endmask);
		    pdst += widthDst;
		    pglyph += widthGlyph;
	        }
	        xpos += w;
	        xpos &= 0x1f;
	        pdstBase++;
	    }
        }
	break;
    }
}

