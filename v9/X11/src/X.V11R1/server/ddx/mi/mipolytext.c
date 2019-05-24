/*******************************************************************
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
/* $Header: mipolytext.c,v 1.11 87/09/11 07:20:25 toddb Exp $ */
/*
 * mipolytext.c - text routines
 *
 * Author:	haynes
 * 		Digital Equipment Corporation
 * 		Western Software Laboratory
 * Date:	Thu Feb  5 1987
 */

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
#include	"fontstruct.h"
#include	"dixfontstr.h"
#include	"gcstruct.h"

static unsigned int
miWidth(n, charinfo)
    CharInfoPtr charinfo[];
    unsigned int n;
{
    unsigned int i;
    unsigned int w = 0;

    for (i=0; i < n; i++) w += charinfo[i]->metrics.characterWidth;

    return w;
}


static int
miPolyText(pDraw, pGC, x, y, count, chars, fontEncoding)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    char 	*chars;
    FontEncoding fontEncoding;
{
    CharInfoPtr *charinfo;
    unsigned int n, w;

    if(!(charinfo = (CharInfoPtr *)ALLOCATE_LOCAL(count*sizeof(CharInfoPtr ))))
	return x ;
    GetGlyphs(pGC->font, count, chars, fontEncoding, &n, charinfo);
    w = miWidth(n, charinfo);
    if (n != 0)
        (*pGC->PolyGlyphBlt)(
	    pDraw, pGC, x, y, n, charinfo, pGC->font->pGlyphs);

    DEALLOCATE_LOCAL(charinfo);
    return x+w;
}


int
miPolyText8(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int 	count;
    char	*chars;
{
    return miPolyText(pDraw, pGC, x, y, count, chars, Linear8Bit);
}


int
miPolyText16(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    unsigned short *chars;
{
    if (pGC->font->pFI->lastRow == 0)
	return miPolyText(pDraw, pGC, x, y, count, (char *)chars, Linear16Bit);
    else
	return miPolyText(pDraw, pGC, x, y, count, (char *)chars, TwoD16Bit);
}


static int
miImageText(pDraw, pGC, x, y, count, chars, fontEncoding)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int 	x, y;
    int 	count;
    char 	*chars;
    FontEncoding fontEncoding;
{
    CharInfoPtr *charinfo;
    unsigned int n;
    FontPtr font = pGC->font;
    unsigned int w;

    if(!(charinfo = (CharInfoPtr *)ALLOCATE_LOCAL(count*sizeof(CharInfoPtr))))
	return x;
    GetGlyphs(font, count, chars, fontEncoding, &n, charinfo);
    w = miWidth(n, charinfo);
    if (n !=0 )
        (*pGC->ImageGlyphBlt)(pDraw, pGC, x, y, n, charinfo, font->pGlyphs);
    DEALLOCATE_LOCAL(charinfo);
    return x+w;
}


void
miImageText8(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    char	*chars;
{
    miImageText(pDraw, pGC, x, y, count, chars, Linear8Bit);
}


void
miImageText16(pDraw, pGC, x, y, count, chars)
    DrawablePtr pDraw;
    GCPtr	pGC;
    int		x, y;
    int		count;
    unsigned short *chars;
{
    if (pGC->font->pFI->lastRow == 0)
	miImageText(pDraw, pGC, x, y, count, (char *)chars, Linear16Bit);
    else
	miImageText(pDraw, pGC, x, y, count, (char *)chars, TwoD16Bit);
}
