#include "copyright.h"

/* $Header: XMapRaised.c,v 1.6 87/09/11 08:05:02 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XMapRaised (dpy, w)
    Window w;
    register Display *dpy;
{
    register xConfigureWindowReq *req;
    register xResourceReq *req2;
    LockDisplay(dpy);
    GetReqExtra(ConfigureWindow, 4, req);
    req->window = w;
    req->mask = CWStackMode;
    * (unsigned long *) (req + 1) = Above;
    GetResReq (MapWindow, w, req2);
    UnlockDisplay(dpy);
    SyncHandle();
}

