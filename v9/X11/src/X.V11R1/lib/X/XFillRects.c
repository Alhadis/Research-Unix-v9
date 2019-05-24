#include "copyright.h"

/* $Header: XFillRects.c,v 11.8 87/09/11 08:03:13 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XFillRectangles(dpy, d, gc, rectangles, n_rects)
register Display *dpy;
Drawable d;
GC gc;
XRectangle *rectangles;
int n_rects;
{
    register xPolyFillRectangleReq *req;
    register long nbytes;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReq(PolyFillRectangle, req);
    req->drawable = d;
    req->gc = gc->gid;

    /* sizeof(xRectangle) will be a multiple of 4 */
    req->length += n_rects * (sizeof(xRectangle) / 4);

    nbytes = n_rects * sizeof(xRectangle);

    PackData (dpy, (char *) rectangles, nbytes);
    UnlockDisplay(dpy);
    SyncHandle();
}
    
