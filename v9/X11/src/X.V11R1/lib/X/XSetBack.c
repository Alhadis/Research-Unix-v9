#include "copyright.h"

/* $Header: XSetBack.c,v 11.7 87/09/11 08:06:43 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XSetBackground (dpy, gc, background)
register Display *dpy;
GC gc;
unsigned long background; /* CARD32 */
{
    LockDisplay(dpy);
    if (gc->values.background != background) {
	gc->values.background = background;
	gc->dirty |= GCBackground;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
