#include "copyright.h"

/* $Header: XSetStip.c,v 11.11 87/09/11 08:07:22 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XSetStipple (dpy, gc, stipple)
register Display *dpy;
GC gc;
Pixmap stipple;
{
    LockDisplay(dpy);

    if (gc->values.stipple != stipple) {
	gc->values.stipple = stipple;
	gc->dirty |= GCStipple;
	_XFlushGCCache(dpy, gc);
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
