#include "copyright.h"

/* $Header: XSetClOrig.c,v 11.11 87/09/11 08:06:46 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XSetClipOrigin (dpy, gc, xorig, yorig)
register Display *dpy;
GC gc;
int xorig, yorig;
{
    XGCValues *gv = &gc->values;

    LockDisplay(dpy);
    if (xorig != gv->clip_x_origin) {
        gv->clip_x_origin = xorig;
	gc->dirty |= GCClipXOrigin;
    }
    if (yorig != gv->clip_y_origin) {
        gv->clip_y_origin = yorig;
	gc->dirty |= GCClipYOrigin;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
