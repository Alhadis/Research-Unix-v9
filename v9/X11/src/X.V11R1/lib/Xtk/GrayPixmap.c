#ifndef lint
static char rcsid[] = "$Header: GrayPixmap.c,v 1.5 87/09/12 16:37:19 swick Exp $";
#endif lint

/*
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 * 
 *                         All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of Digital Equipment
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.  
 * 
 * 
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include "Intrinsic.h"
#include <stdio.h>

#define XtMalloc(s) malloc(s)

typedef struct _PixmapCache {
    Screen *screen;
    Pixmap pixmap;
    struct _PixmapCache *next;
  } CacheEntry;

static CacheEntry *pixmapCache = NULL;

Pixmap XtGrayPixmap( screen )
    Screen *screen;
/*
 *	Creates a gray pixmap of depth DefaultDepth(screen)
 *	caches these so that multiple requests share the pixmap
 */
{
    register Display *display = DisplayOfScreen(screen);
    XImage image;
    CacheEntry *cachePtr;
    Pixmap gray_pixmap;
    GC gc;
    XGCValues gcValues;
    static unsigned short pixmap_bits[] = {
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555
    };

#define pixmap_width 32
#define pixmap_height 32

    /* see if we already have a pixmap suitable for this screen */
    for (cachePtr = pixmapCache; cachePtr; cachePtr = cachePtr->next) {
	if (cachePtr->screen == screen)
	    return( cachePtr->pixmap );
    }

    /* nope, we'll have to construct one now */
    image.height = pixmap_height;
    image.width = pixmap_width;
    image.xoffset = 0;
    image.format = XYBitmap;
    image.data = (char*) pixmap_bits;
    image.byte_order = ImageByteOrder(display);
    image.bitmap_pad = BitmapPad(display);
    image.bitmap_bit_order = BitmapBitOrder(display);
    image.bitmap_unit = BitmapUnit(display);
    image.depth = 1;
    image.bytes_per_line = pixmap_width/8;
    image.obdata = NULL;

    gray_pixmap = XCreatePixmap( display, RootWindowOfScreen(screen), 
				 image.width, image.height,
				 (unsigned) DefaultDepthOfScreen(screen) );

    /* and insert it at the head of the cache */
    cachePtr = (CacheEntry *)XtMalloc( sizeof(CacheEntry) );
    cachePtr->screen = screen;
    cachePtr->pixmap = gray_pixmap;
    cachePtr->next = pixmapCache;
    pixmapCache = cachePtr;

    /* now store the image into it */
    gcValues.foreground = BlackPixelOfScreen(screen);
    gcValues.background = WhitePixelOfScreen(screen);
    gc = XCreateGC( display, RootWindowOfScreen(screen),
		    GCForeground | GCBackground, &gcValues );

    XPutImage( display, gray_pixmap, gc, &image, 0, 0, 0, 0,
	       image.width, image.height);

    XFreeGC( display, gc );

    return( gray_pixmap );
}
