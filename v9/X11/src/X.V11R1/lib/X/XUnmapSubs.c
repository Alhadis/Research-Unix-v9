#include "copyright.h"

/* $Header: XUnmapSubs.c,v 11.6 87/09/11 08:08:17 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XUnmapSubwindows(dpy, win)
register Display *dpy;
Window win;
{
    register xResourceReq *req;

    LockDisplay(dpy);
    GetResReq(UnmapSubwindows,win, req);
    UnlockDisplay(dpy);
    SyncHandle();
}
