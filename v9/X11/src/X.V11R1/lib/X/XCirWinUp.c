#include "copyright.h"

/* $Header: XCirWinUp.c,v 11.6 87/09/11 08:01:57 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XCirculateSubwindowsUp(dpy, w)
    register Display *dpy;
    Window w;
{
    register xCirculateWindowReq *req;

    LockDisplay(dpy);
    GetReq(CirculateWindow, req);
    req->window = w;
    req->direction = RaiseLowest;
    UnlockDisplay(dpy);
    SyncHandle();
}

