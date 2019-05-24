#include "copyright.h"

/* $Header: XChCmap.c,v 11.5 87/09/11 08:01:29 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XSetWindowColormap(dpy, w, colormap)
    register Display *dpy;
    Window w;
    Colormap colormap;
{
    register xChangeWindowAttributesReq *req;

    LockDisplay(dpy);
    GetReqExtra (ChangeWindowAttributes, 4, req);
    req->window = w;
    req->valueMask = CWColormap;
    * (unsigned long *) (req + 1) = colormap;
    UnlockDisplay(dpy);
    SyncHandle();
}

