#include "copyright.h"

/* $Header: XDrPoints.c,v 1.8 87/09/11 08:15:24 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XDrawPoints(dpy, d, gc, points, n_points, mode)
    register Display *dpy;
    Drawable d;
    GC gc;
    XPoint *points;
    int n_points;
    int mode; /* CoordMode */
{
    register xPolyPointReq *req;
    register long nbytes;
    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReq(PolyPoint, req);
    req->drawable = d;
    req->gc = gc->gid;
    req->coordMode = mode;

    /* on the VAX, each point is 2 16-bit integers */
    req->length += n_points;

    /* do this explicitly once; PackData may be a macro and thus do it
       multiple times if we pass it as a parameter */
    nbytes = (long)n_points << 2;

    PackData(dpy, (char *) points, nbytes);
    UnlockDisplay(dpy);
    SyncHandle();
}
