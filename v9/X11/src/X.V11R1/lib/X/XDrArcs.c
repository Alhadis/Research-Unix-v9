#include "copyright.h"

/* $Header: XDrArcs.c,v 11.8 87/09/11 08:10:11 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XDrawArcs(dpy, d, gc, arcs, n_arcs)
register Display *dpy;
Drawable d;
GC gc;
XArc *arcs;
int n_arcs;
{
    register xPolyArcReq *req;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReq(PolyArc,req);
    req->drawable = d;
    req->gc = gc->gid;

    /* sizeof(xArc) will be a multiple of 4 */
    req->length += n_arcs * (sizeof(xArc) / 4);
    
    n_arcs *= sizeof(xArc);

    PackData (dpy, (char *) arcs, (long)n_arcs);
    UnlockDisplay(dpy);
    SyncHandle();
}
