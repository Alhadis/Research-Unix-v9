#include "copyright.h"

/* $Header: XCopyPlane.c,v 11.7 87/09/11 08:02:23 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XCopyPlane(dpy, src_drawable, dst_drawable, gc,
	  src_x, src_y, width, height,
	  dst_x, dst_y, bit_plane)
     register Display *dpy;
     Drawable src_drawable, dst_drawable;
     GC gc;
     int src_x, src_y;
     unsigned int width, height;
     int dst_x, dst_y;
     unsigned long bit_plane;

{       
    register xCopyPlaneReq *req;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReq(CopyPlane, req);
    req->srcDrawable = src_drawable;
    req->dstDrawable = dst_drawable;
    req->gc = gc->gid;
    req->srcX = src_x;
    req->srcY = src_y;
    req->dstX = dst_x;
    req->dstY = dst_y;
    req->width = width;
    req->height = height;
    req->bitPlane = bit_plane;
    UnlockDisplay(dpy);
    SyncHandle();
}

