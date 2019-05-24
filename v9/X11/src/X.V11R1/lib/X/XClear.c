#include "copyright.h"

/* $Header: XClear.c,v 11.6 87/09/11 08:02:00 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XClearWindow(dpy, w)
    register Display *dpy;
    Window w;
{
    register xClearAreaReq *req;

    LockDisplay(dpy);
    GetReq(ClearArea, req);
    req->window = w;
    req->x = req->y = req->width = req->height = 0;
        /* these values mean "clear the entire window" */
    req->exposures = xFalse;
    UnlockDisplay(dpy);
    SyncHandle();
}

