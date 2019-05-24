#include "copyright.h"

/* $Header: XFreeCmap.c,v 11.5 87/09/11 08:03:29 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XFreeColormap(dpy, cmap)
register Display *dpy;
Colormap cmap;
{
    register xResourceReq *req;

    LockDisplay(dpy);
    GetResReq(FreeColormap, cmap, req);
    UnlockDisplay(dpy);
    SyncHandle();
}
