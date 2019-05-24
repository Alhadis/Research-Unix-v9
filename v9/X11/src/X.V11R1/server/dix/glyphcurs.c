/************************************************************************
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

************************************************************************/

/* $Header: glyphcurs.c,v 1.8 87/09/07 18:54:49 rws Exp $ */

#include "X.h"
#include "Xmd.h"
#include "Xproto.h"
#include "dixfontstr.h"
#include "fontstruct.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "resource.h"
#include "dix.h"
#include "cursorstr.h"
#include "misc.h"
#include "opaque.h"
#include "servermd.h"


/*
    get the bits out of the font in a portable way.  to avoid
dealing with padding and such-like, we draw the glyph into
a bitmap, then read the bits out with GetImage, which
uses server-natural format.
    since all screens return the same bitmap format, we'll just use
the first one we find.
    if the font isn't there, it's probably been sent from
the mask part of CreateGlyphCursor, so we return a bitmap
filled with 1s.
    the character origin lines up with the hotspot in the
cursor metrics.
*/

int
ServerBitsFromGlyph(fontID, pfont, ch, cm, ppbits)
    XID		fontID;
    FontPtr	pfont;
    unsigned short ch;
    register CursorMetricPtr cm;
    unsigned char **ppbits;
{
    register ScreenPtr pScreen;
    register GCPtr pGC;
    xRectangle rect;
    PixmapPtr ppix;
    int nby;
    unsigned char *pbits;
    int gcval[3];
    unsigned char char2b[2];

    /* this takes in a short, which needs to be made into a char2b 
       the msb of the short is byte1, the lsb is byte2
       this is the way a short is on the 68000 anyway.  not a whole lot
    of time here, so just do it always to cut down on extra flags in the
    compilation.  the glyph access code might turn this all back into
    a short; this routine takes a short for the beneffit of application
    writers using a linear cursor font, so they don't have to think
    about 2-byte characters.
       sigh.
    */
    char2b[0] = (unsigned char)(ch >> 8);
    char2b[1] = (unsigned char)(ch & 0xff);

    pScreen = &screenInfo.screen[0];
    nby = PixmapBytePad(cm->width, 1) * cm->height;
    pbits = (unsigned char *)Xalloc(nby);
    if (!pbits)
	return BadAlloc;

    ppix = (PixmapPtr)(*pScreen->CreatePixmap)(pScreen, cm->width,
					       cm->height, 1);
    if (!ppix)
    {
	Xfree(pbits);
	return BadAlloc;
    }
    pGC = GetScratchGC(1, pScreen);

    rect.x = 0;
    rect.y = 0;
    rect.width = cm->width;
    rect.height = cm->height;

    if (!pfont)
    {
	/* fill the pixmap with 1 */
	gcval[0] = GXcopy;
	gcval[1] = 1;
	DoChangeGC(pGC, GCFunction | GCForeground, gcval, 0);
	ValidateGC(ppix, pGC);
	(*pGC->PolyFillRect)(ppix, pGC, 1, &rect);
    }
    else
    {
	/* fill the pixmap with 0 */
	gcval[0] = GXcopy;
	gcval[1] = 0;
	gcval[2] = fontID;
	DoChangeGC(pGC, GCFunction | GCForeground | GCFont, gcval, 0);
	ValidateGC(ppix, pGC);
	(*pGC->PolyFillRect)(ppix, pGC, 1, &rect);

	/* draw the glyph */
	gcval[0] = 1;
	DoChangeGC(pGC, GCForeground, gcval, 0);
	ValidateGC(ppix, pGC);
	(*pGC->PolyText16)(ppix, pGC, cm->xhot, cm->yhot, 1, char2b);
    }
    (*pScreen->GetImage)(ppix, 0, 0, cm->width, cm->height,
			 ZPixmap, 0xffffffff, pbits);
    *ppbits = pbits;
    FreeScratchGC(pGC);
    (*pScreen->DestroyPixmap)(ppix);
    return Success;
}


Bool
CursorMetricsFromGlyph( pfont, ch, cm)
    register FontPtr 	pfont;
    int		ch;
    register CursorMetricPtr cm;
{
    register CharInfoPtr 	pci = ADDRXTHISCHARINFO(pfont, ch);

    if (   ch < pfont->pFI->chFirst
	|| ch >= pfont->pFI->chFirst + n1dChars(pfont->pFI))
    {
	cm->width = 0;
	cm->height = 0;
	cm->xhot = 0;
	cm->yhot = 0;
	return FALSE;
    }
    cm->xhot = - pci->metrics.leftSideBearing;
    cm->yhot =   pci->metrics.ascent;
    cm->width = pci->metrics.rightSideBearing + cm->xhot;
    cm->height = pci->metrics.descent + cm->yhot;

    return TRUE;
}



