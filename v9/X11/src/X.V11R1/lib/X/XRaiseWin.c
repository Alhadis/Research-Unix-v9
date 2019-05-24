#include "copyright.h"

/* $Header: XRaiseWin.c,v 11.6 87/09/11 08:06:23 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XRaiseWindow (dpy, w)
    register Display *dpy;
    Window w;
{
    register xConfigureWindowReq *req;

    LockDisplay(dpy);
    GetReqExtra(ConfigureWindow, 4, req);
    req->window = w;
    req->mask = CWStackMode;
    * (unsigned long *) (req + 1) = Above;
    UnlockDisplay(dpy);
    SyncHandle();
}

