#include "copyright.h"

/* $Header: XCopyArea.c,v 11.6 87/09/11 08:02:14 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XCopyArea(dpy, src_drawable, dst_drawable, gc,
	  src_x, src_y, width, height,
	  dst_x, dst_y)
     register Display *dpy;
     Drawable src_drawable, dst_drawable;
     GC gc;
     int src_x, src_y;
     unsigned int width, height;
     int dst_x, dst_y;

{
    register xCopyAreaReq *req;

    LockDisplay(dpy);
    FlushGC(dpy, gc);
    GetReq(CopyArea, req);
    req->srcDrawable = src_drawable;
    req->dstDrawable = dst_drawable;
    req->gc = gc->gid;
    req->srcX = src_x;
    req->srcY = src_y;
    req->dstX = dst_x;
    req->dstY = dst_y;
    req->width = width;
    req->height = height;
    UnlockDisplay(dpy);
    SyncHandle();
}

