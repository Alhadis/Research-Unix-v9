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
#include	"rtutils.h"
#include	"xaed.h"


/*
    we should eventually special-case fixed-width fonts for ImageText.

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

    Image text looks at the bits in the glyph and the fg and bg in the
GC.  it paints a rectangle, as defined in the protocol dcoument,
and the paints the characters.

   to avoid source proliferation, this file is compiled
three times:
	MFBIMAGEGLYPHBLT	OPEQ
	mfbImageGlyphBltWhite	|=
	mfbImageGlyphBltBlack	&=~

    the register allocations for startmask and endmask may not
be the right thing.  are there two other deserving candidates?
xoff, pdst, pglyph, and tmpSrc seem like the right things, though.
*/

void
aedImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    unsigned char *pglyphBase;	/* start of array of glyphs */
{
    ExtentInfoRec info;	/* used by QueryGlyphExtents() */
    BoxRec bbox;	/* string's bounding box */
    xRectangle backrect;/* backing rectangle to paint.
			   in the general case, NOT necessarily
			   the same as the string's bounding box
			*/

    CharInfoPtr pci;
    int xorg, yorg;	/* origin of drawable in bitmap */
    int widthDst;	/* width of dst in longwords */

			/* these keep track of the character origin */
    register unsigned char *pdst;
			/* pointer to current char in vikint */

    int w;		/* width of glyph in bits */
    int h;		/* height of glyph */
    int widthGlyph;	/* width of glyph, in bytes */
    register unsigned char *pglyph;
			/* pointer to current row of glyph */

			/* used for putting down glyph */    
    int nbox;
    BoxPtr pbox;
    int i, j;
    short merge;
    RegionPtr pRegion;
    int oddWidth;
    int widthShorts;

    TRACE(("aedImageGlyphBlt( pDrawable = 0x%x, pGC = 0x%x, x=%d, y=%d, nglyph=%d, ppci= 0x%x, pglyphBase= 0x%x)\n", pDrawable, pGC, x, y, nglyph, ppci, pglyphBase));
    if (!(pGC->planemask & 1))
	return;

    xorg = ((WindowPtr)pDrawable)->absCorner.x;
    yorg = ((WindowPtr)pDrawable)->absCorner.y;

    QueryGlyphExtents(pGC->font, ppci, nglyph, &info);

    x += xorg;
    y += yorg;

    backrect.x = x + info.overallLeft;
    backrect.y = y - pGC->font->pFI->fontAscent;
    backrect.width = info.overallRight - info.overallLeft;
    backrect.width = max( backrect.width, info.overallWidth );
    backrect.height = pGC->font->pFI->fontAscent + 
		      pGC->font->pFI->fontDescent;

    bbox.x1 = x + info.overallLeft;
    bbox.x2 = x + info.overallRight;
    bbox.y1 = y - info.overallAscent;
    bbox.y2 = y + info.overallDescent;

    if (pGC->bgPixel)
	merge = mergexlate[GXset];
    else
	merge = mergexlate[GXclear];

    pRegion = ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip;
    nbox = pRegion->numRects;
    pbox = pRegion->rects;
    if ( nbox == 0 )
	return;

    vforce();
    clear(2);
    vikint[ORMERGE] = merge;
    vikint[ORCLIPLX] = pbox->x1;
    vikint[ORCLIPLY] = pbox->y1;
    vikint[ORCLIPHX] = pbox->x2-1;
    vikint[ORCLIPHY] = pbox->y2-1;
    vikint[ORXPOSN] = backrect.x;
    vikint[ORYPOSN] = backrect.y;
    pbox++;
    nbox--;
    vikint[vikoff++] = 10;	/* tile order */
    vikint[vikoff++] = backrect.width;	/* rectangle width */
    vikint[vikoff++] = backrect.height;	/* rectangle height */
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

    if( pGC->fgPixel )
	merge = mergexlate[GXor];
    else
	merge = mergexlate[GXandInverted];

    switch ((*pGC->pScreen->RectIn)(
		  ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip, &bbox))
    {
      case rgnOUT:
	break;
      case rgnIN:

        while(nglyph--)
        {
	    pci = *ppci;
	    pglyph = pglyphBase + pci->byteOffset;
	    w = GLYPHWIDTHPIXELS(pci);
	    h = GLYPHHEIGHTPIXELS(pci);
	    widthGlyph = GLYPHWIDTHBYTESPADDED(pci);
	    widthShorts = widthGlyph >> 1;
	    oddWidth = widthGlyph != ( widthShorts << 1 );
	    vikint[ORXPOSN] = x + pci->metrics.leftSideBearing;
	    vikint[ORYPOSN] = y - pci->metrics.ascent; 
	    vikint[ORMERGE] = merge;
	    vikint[vikoff++] = 9;	/* draw image order */
	    vikint[vikoff++] = w;
	    vikint[vikoff++] = h;
	    pdst = &(vikint[vikoff]);
	    for( i = 0; i < h; i++ )
	    {
		for( j = 0; j < widthGlyph; j++ )
		    *pdst++ = *pglyph++;
		vikoff = vikoff + widthShorts;
		if( oddWidth )
		{
		    vikoff++;
		    *pdst++ = (char)0;
		}
	    }

	    /* update character origin */
	    x += pci->metrics.characterWidth;
	    ppci++;
	    vforce();
	    clear(2);
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
	RegionPtr prgnClip;

	BoxRec cbox;
	int glx, gly;

        while(nglyph--)
        {
	    pci = *ppci;
	    pglyph = pglyphBase + pci->byteOffset;
	    w = GLYPHWIDTHPIXELS(pci);
	    h = GLYPHHEIGHTPIXELS(pci);
	    cbox.x1 = glx = x + pci->metrics.leftSideBearing;
	    cbox.y1 = gly = y - pci->metrics.ascent;
	    cbox.x2 = cbox.x1 + w;
	    cbox.y2 = cbox.y1 + h;

	    switch ((*pGC->pScreen->RectIn)(
		  ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip, &cbox))

	    {
	      case rgnOUT:
		break;
	      case rgnIN:
		widthGlyph = GLYPHWIDTHBYTESPADDED(pci);
		widthShorts = widthGlyph >> 1;
		oddWidth = widthGlyph != ( widthShorts << 1 );
		vikint[ORXPOSN] = glx;
		vikint[ORYPOSN] = gly; 
		vikint[ORMERGE] = merge;
		vikint[vikoff++] = 9;	/* draw image order */
		vikint[vikoff++] = w;
		vikint[vikoff++] = h;
		pdst = &(vikint[vikoff]);
		for( i = 0; i < h; i++ )
		{
		    for( j = 0; j < widthGlyph; j++ )
			*pdst++ = *pglyph++;
		    vikoff = vikoff + widthShorts;
		    if( oddWidth )
		    {
			vikoff++;
			*pdst++ = (char)0;
		    }
		}
		break;
	      case rgnPART:
		prgnClip = (* ((WindowPtr)pDrawable)->drawable.pScreen->RegionCreate)(&cbox, 
		  ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip->numRects);
		(* ((WindowPtr)pDrawable)->drawable.pScreen->Intersect)
			(prgnClip, prgnClip, 
			((mfbPrivGC *)(pGC->devPriv))->pCompositeClip);
		pbox = prgnClip->rects;
		nbox = prgnClip->numRects;
		if ( nbox == 0 )
		    break;
		widthGlyph = GLYPHWIDTHBYTESPADDED(pci);
		widthShorts = widthGlyph >> 1;
		oddWidth = widthGlyph != ( widthShorts << 1 );
		vikint[ORXPOSN] = glx;
		vikint[ORYPOSN] = gly; 
		vikint[ORMERGE] = merge;
		vikint[vikoff++] = 9;	/* draw image order */
		vikint[vikoff++] = w;
		vikint[vikoff++] = h;
		pdst = &(vikint[vikoff]);
		for( i = 0; i < h; i++ )
		{
		    for( j = 0; j < widthGlyph; j++ )
			*pdst++ = *pglyph++;
		    vikoff = vikoff + widthShorts;
		    if( oddWidth )
		    {
			vikoff++;
			*pdst++ = (char)0;
		    }
		}
		vikint[ORCLIPLX] = pbox->x1;
		vikint[ORCLIPLY] = pbox->y1;
		vikint[ORCLIPHX] = pbox->x2-1;
		vikint[ORCLIPHY] = pbox->y2-1;
		pbox++;
		nbox--;
		vforce();
		vikint[VIKCMD] = 2; /* reprocess order */
		for(i = 0; i < nbox; i++, pbox++)
		{
		    vikint[ORCLIPLX] = pbox->x1;
		    vikint[ORCLIPLY] = pbox->y1;
		    vikint[ORCLIPHX] = pbox->x2-1;
		    vikint[ORCLIPHY] = pbox->y2-1;
		    command(ORDATA);
		}
		
		break;
	    }
	    /* update character origin */
	    x += pci->metrics.characterWidth;
	    ppci++;
	    vforce();
	    clear(2);
        } /* while nglyph-- */

      }
      default:
	break;
    }
}

