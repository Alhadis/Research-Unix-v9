#include "copyright.h"

/* $Header: XCrPixmap.c,v 11.5 87/09/11 08:02:35 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

Pixmap XCreatePixmap (dpy, d, width, height, depth)
    register Display *dpy;
    Drawable d;
    unsigned int width, height, depth;
{
    Pixmap pid;
    register xCreatePixmapReq *req;

    LockDisplay(dpy);
    GetReq(CreatePixmap, req);
    req->drawable = d;
    req->width = width;
    req->height = height;
    req->depth = depth;
    pid = req->pid = XAllocID(dpy);
    UnlockDisplay(dpy);
    SyncHandle();
    return (pid);
}

