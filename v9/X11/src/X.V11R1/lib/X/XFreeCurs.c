#include "copyright.h"

/* $Header: XFreeCurs.c,v 11.5 87/09/11 08:03:35 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XFreeCursor(dpy, cursor)
    register Display *dpy;
    Cursor cursor;
{
    register xResourceReq *req;
    LockDisplay(dpy);
    GetResReq(FreeCursor, cursor, req);
    UnlockDisplay(dpy);
    SyncHandle();
}

