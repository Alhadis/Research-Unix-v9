#include "copyright.h"

/* $Header: XUndefCurs.c,v 11.5 87/09/11 08:07:51 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XUndefineCursor (dpy,w)
    register Display *dpy;
    Window w;
{
    register xChangeWindowAttributesReq *req;

    LockDisplay(dpy);
    GetReqExtra (ChangeWindowAttributes, 4, req);
    req->window = w;
    req->valueMask = CWCursor;
    * (unsigned long *) (req + 1) = 0;
    UnlockDisplay(dpy);
    SyncHandle();
}

