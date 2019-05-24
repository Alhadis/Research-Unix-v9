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

/* $Header: miglblt.c,v 1.16 87/09/08 10:10:10 toddb Exp $ */

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmap.h"
#include	"servermd.h"


/*
    machine-independent glyph blt.
    assumes that glyph bits in snf are written in bytes,
have same bit order as the server's bitmap format,
and are byte padded.  this corresponds to the snf distributed
with the sample server.

    get a scratch GC.
    in the scratch GC set alu = GXcopy, fg = 1, bg = 0
    allocate a bitmap big enough to hold the largest glyph in the font
    validate the scratch gc with the bitmap
    for each glyph
	carefully put the bits of the glyph in a buffer,
	    padded to the server pixmap scanline padding rules
	fake a call to PutImage from the buffer into the bitmap
	use the bitmap in a call to PushPixels
*/

void
miPolyGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    unsigned char *pglyphBase;	/* start of array of glyphs */
{
    int width, height;
    PixmapPtr pPixmap;
    int nbyLine;			/* bytes per line of padded pixmap */
    FontRec *pfont;
    GCPtr pGCtmp;
    register int i;
    register int j;
    unsigned char *pbits;		/* buffer for PutImage */
    register unsigned char *pb;		/* temp pointer into buffer */
    register CharInfoPtr pci;		/* currect char info */
    register unsigned char *pglyph;	/* pointer bits in glyph */
    int gWidth, gHeight;		/* width and height of glyph */
    register int nbyGlyphWidth;		/* bytes per scanline of glyph */
    int nbyPadGlyph;			/* server padded line of glyph */

    int gcvals[3];

    if ((pDrawable->type == DRAWABLE_WINDOW) &&
	(pGC->miTranslate))
    {
	x += ((WindowPtr)pDrawable)->absCorner.x;
	y += ((WindowPtr)pDrawable)->absCorner.y;
    }

    pfont = pGC->font;
    width = pfont->pFI->maxbounds.metrics.rightSideBearing - 
	pfont->pFI->minbounds.metrics.leftSideBearing;
    height = pfont->pFI->maxbounds.metrics.ascent + 
	pfont->pFI->maxbounds.metrics.descent;

    pPixmap = (PixmapPtr)(*pDrawable->pScreen->CreatePixmap)
      (pDrawable->pScreen, width, height, 1, XYBitmap);

    pGCtmp = GetScratchGC(1, pDrawable->pScreen);

    gcvals[0] = GXcopy;
    gcvals[1] = 1;
    gcvals[2] = 0;

    DoChangeGC(pGCtmp, GCFunction|GCForeground|GCBackground, gcvals, 0);

    nbyLine = PixmapBytePad(width, 1);
    pbits = (unsigned char *)ALLOCATE_LOCAL(height*nbyLine);
    if (!pbits)
        return ;

    while(nglyph--)
    {
	pci = *ppci++;
	pglyph = pglyphBase + pci->byteOffset;
	gWidth = GLYPHWIDTHPIXELS(pci);
	gHeight = GLYPHHEIGHTPIXELS(pci);
	nbyGlyphWidth = GLYPHWIDTHBYTESPADDED(pci);
	nbyPadGlyph = PixmapBytePad(gWidth, 1);

	for (i=0, pb = pbits; i<gHeight; i++, pb = pbits+(i*nbyPadGlyph))
	    for (j = 0; j < nbyGlyphWidth; j++)
		*pb++ = *pglyph++;


	if ((pGCtmp->serialNumber) != (pPixmap->drawable.serialNumber))
	    ValidateGC(pPixmap, pGCtmp);
	(*pGCtmp->PutImage)(pPixmap, pGCtmp, pPixmap->drawable.depth,
			    0, 0, gWidth, gHeight, 
			    0, XYBitmap, pbits);

	if ((pGC->serialNumber) != (pDrawable->serialNumber))
	    ValidateGC(pDrawable, pGC);
	(*pGC->PushPixels)(pGC, pPixmap, pDrawable,
			   gWidth, gHeight,
			   x + pci->metrics.leftSideBearing,
			   y - pci->metrics.ascent);
	x += pci->metrics.characterWidth;
    }
    (*pDrawable->pScreen->DestroyPixmap)(pPixmap);
    DEALLOCATE_LOCAL(pbits);
    FreeScratchGC(pGCtmp);
}


void
miImageGlyphBlt(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase)
    DrawablePtr pDrawable;
    GC 		*pGC;
    int 	x, y;
    unsigned int nglyph;
    CharInfoPtr *ppci;		/* array of character info */
    unsigned char *pglyphBase;	/* start of array of glyphs */
{
    ExtentInfoRec info;		/* used by QueryGlyphExtents() */
    long gcvals[3];
    int oldAlu, oldFS;
    unsigned long	oldFG;
    xRectangle backrect;

    QueryGlyphExtents(pGC->font, ppci, nglyph, &info);

    backrect.x = x;
    backrect.y = y - pGC->font->pFI->fontAscent;
    backrect.width = info.overallWidth;
    backrect.height = pGC->font->pFI->fontAscent + 
		      pGC->font->pFI->fontDescent;

    oldAlu = pGC->alu;
    oldFG = pGC->fgPixel;
    oldFS = pGC->fillStyle;

    /* fill in the background */
    gcvals[0] = (long) GXcopy;
    gcvals[1] = (long) pGC->bgPixel;
    gcvals[2] = (long) FillSolid;
    DoChangeGC(pGC, GCFunction|GCForeground|GCFillStyle, gcvals, 0);
    ValidateGC(pDrawable, pGC);
    (*pGC->PolyFillRect)(pDrawable, pGC, 1, &backrect);

    /* put down the glyphs */
    gcvals[0] = (long) oldFG;
    DoChangeGC(pGC, GCForeground, gcvals, 0);
    ValidateGC(pDrawable, pGC);
    (*pGC->PolyGlyphBlt)(pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);

    /* put all the toys away when done playing */
    gcvals[0] = (long) oldAlu;
    gcvals[1] = (long) oldFG;
    gcvals[2] = (long) oldFS;
    DoChangeGC(pGC, GCFunction|GCForeground|GCFillStyle, gcvals, 0);
/*
   ???
   should we cal ValidateGC now, to leave everything exactly as we
found it
   ???
*/

}

