#include "copyright.h"

/* $Header: XSelInput.c,v 11.5 87/09/11 08:06:37 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XSelectInput (dpy, w, mask)
    register Display *dpy;
    Window w;
    unsigned long mask;
{
    register xChangeWindowAttributesReq *req;

    LockDisplay(dpy);
    GetReqExtra (ChangeWindowAttributes, 4, req);
    req->window = w;
    req->valueMask = CWEventMask;
    * (unsigned long *) (req + 1) = mask;
    UnlockDisplay(dpy);
    SyncHandle();
}

