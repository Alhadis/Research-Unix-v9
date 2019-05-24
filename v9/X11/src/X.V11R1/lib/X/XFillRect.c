#include "copyright.h"

/* $Header: XFillRect.c,v 11.9 87/09/11 08:03:09 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

/* precompute the maximum size of batching request allowed */

static int size = sizeof(xPolyFillRectangleReq) + EPERBATCH * sizeof(xRectangle);

XFillRectangle(dpy, d, gc, x, y, width, height)
    register Display *dpy;
    Drawable d;
    GC gc;
    int x, y; /* INT16 */
    unsigned int width, height; /* CARD16 */
{
    xRectangle *rect;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    {
    register xPolyFillRectangleReq *req 
		= (xPolyFillRectangleReq *) dpy->last_req;
    /* if same as previous request, with same drawable, batch requests */
    if (
          (req->reqType == X_PolyFillRectangle)
       && (req->drawable == d)
       && (req->gc == gc->gid)
       && ((dpy->bufptr + sizeof (xRectangle)) <= dpy->bufmax)
       && (((char *)dpy->bufptr - (char *)req) < size) ) {
         rect = (xRectangle *) dpy->bufptr;
	 req->length += sizeof (xRectangle) >> 2;
	 dpy->bufptr += sizeof (xRectangle);
	 }

    else {
	GetReqExtra(PolyFillRectangle, sizeof(xRectangle), req);
	req->drawable = d;
	req->gc = gc->gid;
	rect = (xRectangle *) (req + 1);
	}
    rect->x = x;
    rect->y = y;
    rect->width = width;
    rect->height = height;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
