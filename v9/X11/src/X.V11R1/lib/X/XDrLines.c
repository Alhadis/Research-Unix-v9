#include "copyright.h"

/* $Header: XDrLines.c,v 11.9 87/09/11 08:15:20 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XDrawLines (dpy, d, gc, points, npoints, mode)
    register Display *dpy;
    Drawable d;
    GC gc;
    XPoint *points;
    int npoints;
    int mode;
{
    register xPolyLineReq *req;
    register long length;
    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReq (PolyLine, req);
    req->drawable = d;
    req->gc = gc->gid;
    req->coordMode = mode;
    req->length += npoints;
       /* each point is 2 16-bit integers */
    length = npoints << 2; 
       /* do this here, not in arguments to PackData, since PackData
          may be a macro which uses its arguments more than once */
    PackData (dpy, (char *) points, length);
    UnlockDisplay(dpy);
    SyncHandle();
}

