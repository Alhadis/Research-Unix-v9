#include "copyright.h"

/* $Header: XCirWin.c,v 11.5 87/09/11 08:01:51 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XCirculateSubwindows(dpy, w, direction)
    register Display *dpy;
    Window w;
    int direction;
{
    register xCirculateWindowReq *req;

    LockDisplay(dpy);
    GetReq(CirculateWindow, req);
    req->window = w;
    req->direction = direction;
    UnlockDisplay(dpy);
    SyncHandle();
}

