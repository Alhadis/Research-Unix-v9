#include "copyright.h"

/* $Header: XDrPoint.c,v 11.8 87/09/11 08:10:07 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

/* precompute the maximum size of batching request allowed */

static int size = sizeof(xPolyPointReq) + EPERBATCH * sizeof(xPoint);

XDrawPoint(dpy, d, gc, x, y)
    register Display *dpy;
    Drawable d;
    GC gc;
    int x, y; /* INT16 */
{
    xPoint *point;
    LockDisplay(dpy);
    FlushGC(dpy, gc);
    {
    register xPolyPointReq *req = (xPolyPointReq *) dpy->last_req;
    /* if same as previous request, with same drawable, batch requests */
    if (
          (req->reqType == X_PolyPoint)
       && (req->drawable == d)
       && (req->gc == gc->gid)
       && (req->coordMode == CoordModeOrigin)
       && ((dpy->bufptr + sizeof (xPoint)) <= dpy->bufmax)
       && (((char *)dpy->bufptr - (char *)req) < size) ) {
         point = (xPoint *) dpy->bufptr;
	 req->length += sizeof (xPoint) >> 2;
	 dpy->bufptr += sizeof (xPoint);
	 }

    else {
	GetReqExtra(PolyPoint, 4, req); /* 1 point = 4 bytes */
	req->drawable = d;
	req->gc = gc->gid;
	req->coordMode = CoordModeOrigin;
	point = (xPoint *) (req + 1);
	}
    point->x = x;
    point->y = y;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}

