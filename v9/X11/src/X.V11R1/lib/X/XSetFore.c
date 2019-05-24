#include "copyright.h"

/* $Header: XSetFore.c,v 11.7 87/09/11 08:06:56 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XSetForeground (dpy, gc, foreground)
register Display *dpy;
GC gc;
unsigned long foreground; /* CARD32 */
{
    LockDisplay(dpy);
    if (gc->values.foreground != foreground) {
	gc->values.foreground = foreground;
	gc->dirty |= GCForeground;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
