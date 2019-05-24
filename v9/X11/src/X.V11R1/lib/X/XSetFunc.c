#include "copyright.h"

/* $Header: XSetFunc.c,v 11.7 87/09/11 08:06:59 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XSetFunction (dpy, gc, function)
register Display *dpy;
GC gc;
int function;
{
    LockDisplay(dpy);
    if (gc->values.function != function) {
	gc->values.function = function;
	gc->dirty |= GCFunction;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
