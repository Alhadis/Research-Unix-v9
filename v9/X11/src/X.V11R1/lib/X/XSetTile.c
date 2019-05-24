#include "copyright.h"

/* $Header: XSetTile.c,v 11.11 87/09/11 08:07:25 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XSetTile (dpy, gc, tile)
register Display *dpy;
GC gc;
Pixmap tile;
{
    LockDisplay(dpy);
    if (gc->values.tile != tile) {
	gc->values.tile = tile;
	gc->dirty |= GCTile;
	_XFlushGCCache(dpy, gc);
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
