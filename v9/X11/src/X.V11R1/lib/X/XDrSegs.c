#include "copyright.h"

/* $Header: XDrSegs.c,v 11.8 87/09/11 08:15:31 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XDrawSegments (dpy, d, gc, segments, nsegments)
    register Display *dpy;
    Drawable d;
    GC gc;
    XSegment *segments;
    int nsegments;
{
    register xPolySegmentReq *req;
    long nbytes;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReq (PolySegment, req);
    req->drawable = d;
    req->gc = gc->gid;
    req->length += nsegments<<1;
       /* each segment is 4 16-bit integers, i.e. 2*32 bits */
    nbytes = nsegments << 3; 
       /* do this here, not in arguments to PackData, since PackData
          may be a macro which uses its arguments more than once */
    PackData (dpy, (char *) segments, nbytes);
    UnlockDisplay(dpy);
    SyncHandle();
}

