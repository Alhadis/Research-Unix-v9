#include "copyright.h"

/* $Header: XLowerWin.c,v 11.5 87/09/11 08:04:56 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XLowerWindow(dpy, w)
Display *dpy;
Window w;
{
    register xConfigureWindowReq *req;
    LockDisplay(dpy);
    GetReqExtra(ConfigureWindow, 4, req);
    req->window = w;
    req->mask = CWStackMode;
    * (unsigned long *) (req + 1) = Below;
    UnlockDisplay(dpy);
    SyncHandle();
}

