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


/* $Header: cursor.c,v 1.27 87/09/11 07:18:41 toddb Exp $ */

#include "X.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "dixfont.h"	/* for CreateRootCursor */
#include "resource.h"	/* for CreateRootCursor */

#include "Xmd.h"
#include "dix.h"	/* for CreateRootCursor  */
#include "opaque.h"	/* for CloseFont  */

/*
 * To be called indirectly by DeleteResource; must use exactly two args
 */
void
FreeCursor( pCurs, cid)
    CursorPtr 	pCurs;
    int 	cid;	
{
    int		nscr;

    ScreenPtr	pscr;

    if ( --pCurs->refcnt > 0)
	return;

    for ( nscr=0, pscr=screenInfo.screen;
	  nscr<screenInfo.numScreens;
	  nscr++, pscr++)
    {
        if ( pscr->UnrealizeCursor)
	    ( *pscr->UnrealizeCursor)( pscr, pCurs);
    }
    Xfree( pCurs->source);
    Xfree( pCurs->mask);
    Xfree( pCurs);
}

/*
 * does nothing about the resource table, just creates the data structure.
 * allocates no storage.
 */
CursorPtr 
AllocCursor( psrcbits, pmaskbits, cm,
	    foreRed, foreGreen, foreBlue, backRed, backGreen, backBlue)
    unsigned char *	psrcbits;		/* server-defined padding */
    unsigned char *	pmaskbits;		/* server-defined padding */
    CursorMetricPtr	cm;
    unsigned	int foreRed, foreGreen, foreBlue;
    unsigned	int backRed, backGreen, backBlue;
{
    CursorPtr 	pCurs;
    int		nscr;
    ScreenPtr 	pscr;

    pCurs = (CursorPtr )Xalloc( sizeof(CursorRec)); 

    pCurs->source = psrcbits;
    pCurs->mask = pmaskbits;

    pCurs->width = cm->width;
    pCurs->height = cm->height;

    pCurs->refcnt = 1;		
    pCurs->xhot = cm->xhot;
    pCurs->yhot = cm->yhot;

    pCurs->foreRed = foreRed;
    pCurs->foreGreen = foreGreen;
    pCurs->foreBlue = foreBlue;

    pCurs->backRed = backRed;
    pCurs->backGreen = backGreen;
    pCurs->backBlue = backBlue;

    /*
     * realize the cursor for every screen
     */
    for ( nscr=0, pscr=screenInfo.screen;
	  nscr<screenInfo.numScreens;
	  nscr++, pscr++)
    {
        if ( pscr->RealizeCursor)
	    ( *pscr->RealizeCursor)( pscr, pCurs);
    }
    return pCurs;
}


/***********************************************************
 * CreateRootCursor
 *
 * look up the name of a font
 * open the font
 * add the font to the resource table
 * make bitmaps from glyphs "glyph" and "glyph + 1" of the font
 * make a cursor from the bitmaps
 * add the cursor to the resource table
 *************************************************************/

CursorPtr 
CreateRootCursor(pfilename, glyph)
    char *	pfilename;
    int		glyph;
{
    CursorPtr 	curs;
    FontPtr 	cursorfont;
    char *	psrcbits;
    char *	pmskbits;
    CursorMetricRec cm;
    XID		fontID;

    fontID = FakeClientID(0);
    if (cursorfont = OpenFont( strlen( pfilename), pfilename))
	AddResource(
	   fontID, RT_FONT, cursorfont, CloseFont, RC_CORE);
    else
	return NullCursor;

    if (!CursorMetricsFromGlyph(cursorfont, glyph+1, &cm))
	return NullCursor;

    if (ServerBitsFromGlyph(fontID, cursorfont, glyph, &cm, &psrcbits))
	return NullCursor;
    if (ServerBitsFromGlyph(fontID, cursorfont, glyph+1, &cm, &pmskbits))
	return NullCursor;

    curs = AllocCursor( psrcbits, pmskbits, &cm, 0, 0, 0, ~0, ~0, ~0);

    AddResource(FakeClientID(0), RT_CURSOR, curs, FreeCursor, RC_CORE);
    return curs;
}


