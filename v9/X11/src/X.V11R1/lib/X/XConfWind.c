#include "copyright.h"

/* $Header: XConfWind.c,v 11.6 87/09/11 08:02:07 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XMoveResizeWindow(dpy, w, x, y, width, height)
register Display *dpy;
Window w;
int x, y;
unsigned int width, height;
{
    unsigned long *valuePtr;
    register xConfigureWindowReq *req;

    LockDisplay(dpy);
    GetReqExtra(ConfigureWindow, 16, req);
    req->window = w;
    req->mask = CWX | CWY | CWWidth | CWHeight;
    valuePtr = (unsigned long *) (req + 1);
    *valuePtr++ = x;
    *valuePtr++ = y;
    *valuePtr++ = width;
    *valuePtr   = height;
    UnlockDisplay(dpy);
    SyncHandle();
}
