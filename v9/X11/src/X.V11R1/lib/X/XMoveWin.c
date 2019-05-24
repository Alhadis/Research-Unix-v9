#include "copyright.h"

/* $Header: XMoveWin.c,v 11.6 87/09/11 08:05:15 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XMoveWindow (dpy, w, x, y)
    register Display *dpy;
    Window w;
    int x, y;
{
    unsigned long *valuePtr;
    register xConfigureWindowReq *req;
    LockDisplay(dpy);
    GetReqExtra(ConfigureWindow, 8, req);
    req->window = w;
    req->mask = CWX | CWY;
    valuePtr = (unsigned long *) (req + 1);
    *valuePtr++ = x;
    *valuePtr = y;
    UnlockDisplay(dpy);
    SyncHandle();
}

