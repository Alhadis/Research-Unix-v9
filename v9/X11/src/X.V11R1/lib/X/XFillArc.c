#include "copyright.h"

/* $Header: XFillArc.c,v 11.9 87/09/11 08:03:00 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

/* precompute the maximum size of batching request allowed */

static int size = sizeof(xPolyFillArcReq) + EPERBATCH * sizeof(xArc);

XFillArc(dpy, d, gc, x, y, width, height, angle1, angle2)
    register Display *dpy;
    Drawable d;
    GC gc;
    int x, y; /* INT16 */
    unsigned int width, height; /* CARD16 */
    int angle1, angle2; /* INT16 */
{
    xArc *arc;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    {
    register xPolyFillArcReq *req = (xPolyFillArcReq *) dpy->last_req;
    /* if same as previous request, with same drawable, batch requests */
    if (
          (req->reqType == X_PolyFillArc)
       && (req->drawable == d)
       && (req->gc == gc->gid)
       && ((dpy->bufptr + sizeof (xArc)) <= dpy->bufmax)
       && (((char *)dpy->bufptr - (char *)req) < size) ) {
         arc = (xArc *) dpy->bufptr;
	 req->length += sizeof (xArc) >> 2;
	 dpy->bufptr += sizeof (xArc);
	 }

    else {
	GetReqExtra(PolyFillArc, sizeof(xArc), req);

	req->drawable = d;
	req->gc = gc->gid;
	arc = (xArc *) (req + 1);
	}
    arc->x = x;
    arc->y = y;
    arc->width = width;
    arc->height = height;
    arc->angle1 = angle1;
    arc->angle2 = angle2;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
