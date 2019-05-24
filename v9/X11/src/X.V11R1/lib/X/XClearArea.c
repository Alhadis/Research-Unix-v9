#include "copyright.h"

/* $Header: XClearArea.c,v 11.7 87/09/11 08:02:04 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XClearArea (dpy, w, x, y, width, height, exposures)
    register Display *dpy;
    Window w;
    int x, y;
    unsigned int width, height;
    Bool exposures;
{
    register xClearAreaReq *req;

    LockDisplay(dpy);
    GetReq(ClearArea, req);
    req->window = w;
    req->x = x;
    req->y = y;
    req->width = width;
    req->height = height;
    req->exposures = exposures;
    UnlockDisplay(dpy);
    SyncHandle();
}

