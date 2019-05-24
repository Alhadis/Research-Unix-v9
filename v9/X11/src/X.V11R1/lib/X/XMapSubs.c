#include "copyright.h"

/* $Header: XMapSubs.c,v 11.4 87/09/11 08:05:05 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XMapSubwindows(dpy, win)
    register Display *dpy;
    Window win;
{
    register xResourceReq *req;
    LockDisplay(dpy);
    GetResReq(MapSubwindows, win, req);
    UnlockDisplay(dpy);
    SyncHandle();
}
