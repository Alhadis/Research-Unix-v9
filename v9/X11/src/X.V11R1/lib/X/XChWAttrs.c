#include "copyright.h"

/* $Header: XChWAttrs.c,v 11.6 87/09/11 08:01:40 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XChangeWindowAttributes (dpy, w, valuemask, attributes)
    register Display *dpy;
    Window w;
    unsigned long valuemask;
    XSetWindowAttributes *attributes;
{
    register xChangeWindowAttributesReq *req;

    LockDisplay(dpy);
    GetReq(ChangeWindowAttributes,req);
    req->window = w;
    if (req->valueMask = valuemask) 
        _XProcessWindowAttributes (dpy, req, valuemask, attributes);
    UnlockDisplay(dpy);
    SyncHandle();
}

