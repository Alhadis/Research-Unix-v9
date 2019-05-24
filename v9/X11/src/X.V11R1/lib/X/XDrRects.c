#include "copyright.h"

/* $Header: XDrRects.c,v 11.8 87/09/11 08:15:27 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XDrawRectangles(dpy, d, gc, rects, n_rects)
register Display *dpy;
Drawable d;
GC gc;
XRectangle *rects;
int n_rects;
{
    register xPolyRectangleReq *req;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReq(PolyRectangle, req);
    req->drawable = d;
    req->gc = gc->gid;

    /* sizeof(xRectangle) will be a multiple of 4 */
    req->length += n_rects * (sizeof(xRectangle) / 4);

    n_rects *= sizeof(xRectangle);

    PackData (dpy, (char *) rects, (long)n_rects);
    UnlockDisplay(dpy);
    SyncHandle();
}
