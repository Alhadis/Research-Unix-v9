#include "copyright.h"

/* $Header: XUngrabPtr.c,v 11.7 87/09/11 08:08:05 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XUngrabPointer(dpy, time)
register Display *dpy;
Time time;
{
    register xResourceReq *req;

    LockDisplay(dpy);
    GetResReq(UngrabPointer, time, req);
    UnlockDisplay(dpy);
    SyncHandle();
}
