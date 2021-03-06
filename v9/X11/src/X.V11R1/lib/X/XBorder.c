#include "copyright.h"

/* $Header: XBorder.c,v 11.5 87/09/11 08:01:10 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XSetWindowBorder(dpy, w, pixel)
    register Display *dpy;
    Window w;
    unsigned long pixel;
{
    register xChangeWindowAttributesReq *req;

    LockDisplay(dpy);
    GetReqExtra (ChangeWindowAttributes, 4, req);
    req->window = w;
    req->valueMask = CWBorderPixel;
    * (unsigned long *) (req + 1) = pixel;
    UnlockDisplay(dpy);
    SyncHandle();
}

