#include "copyright.h"

/* $Header: XDrLine.c,v 11.10 87/09/11 08:09:53 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

/* precompute the maximum size of batching request allowed */

static int size = sizeof(xPolySegmentReq) + EPERBATCH * sizeof(xSegment);

XDrawLine (dpy, d, gc, x1, y1, x2, y2)
    register Display *dpy;
    Drawable d;
    GC gc;
    int x1, y1, x2, y2;
{
    register xSegment *segment;
    LockDisplay(dpy);
    FlushGC(dpy, gc);
    {
    register xPolySegmentReq *req = (xPolySegmentReq *) dpy->last_req;
    /* if same as previous request, with same drawable, batch requests */
    if (
          (req->reqType == X_PolySegment)
       && (req->drawable == d)
       && (req->gc == gc->gid)
       && ((dpy->bufptr + sizeof (xSegment)) <= dpy->bufmax)
       && (((char *)dpy->bufptr - (char *)req) < size) ) {
         segment = (xSegment *) dpy->bufptr;
	 req->length += sizeof (xSegment) >> 2;
	 dpy->bufptr += sizeof (xSegment);
	 }

    else {
	GetReqExtra (PolySegment, sizeof(xSegment), req);
	req->drawable = d;
	req->gc = gc->gid;
	segment = (xSegment *) (req + 1);
	}

    segment->x1 = x1;
    segment->y1 = y1;
    segment->x2 = x2;
    segment->y2 = y2;
    UnlockDisplay(dpy);
    SyncHandle();
    }
}

