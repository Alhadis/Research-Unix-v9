#include "copyright.h"

/* $Header: XDelProp.c,v 11.4 87/09/11 08:02:43 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XDeleteProperty(dpy, window, property)
register Display *dpy;
Window window;
Atom property;
{
    register xDeletePropertyReq *req;

    LockDisplay(dpy);
    GetReq(DeleteProperty, req);
    req->window = window;
    req->property = property;
    UnlockDisplay(dpy);
    SyncHandle();
}
