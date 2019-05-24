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
#include "X.h"
#include "pixmap.h"
#include "pixmapstr.h"
#include "mfb.h"
#include "xaed.h"

void
printPixmap(pPixmap)
    PixmapPtr pPixmap;
{
    int i, j;
    int wordLen;
    int *bits;

    if (pPixmap == NullPixmap)
    {
	ErrorF("Null Pixmap\n");
	return;
    }
    ErrorF("    width = %d\n", pPixmap->width); 
    ErrorF("    height = %d\n", pPixmap->height); 
    ErrorF("    refcnt = %d\n", pPixmap->refcnt); 
    ErrorF("    devKind = %d\n", pPixmap->devKind); 
    ErrorF("    bitmap:\n"); 
    wordLen = ( pPixmap->width + 31 ) / 32;
    bits = (int *)pPixmap->devPrivate;
    for ( i = 0 ; i < pPixmap->height ; i++ )
    {
	ErrorF("\t");
	for ( j = 0 ; j < wordLen ; j++ )
	    printBinaryWord(*bits++);
	ErrorF("\n");
    }
    ErrorF("\n");
}

printBinaryWord(word)
    int word;
{
    int i;

    for ( i = 0 ; i < sizeof(int)*8 ; i++ )
    {
	if ( word & ( 1 << (sizeof(int)*8 - 1)))
	    ErrorF("1");
	else
	    ErrorF("0");
	word = word << 1;
    }
}
