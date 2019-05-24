#include "copyright.h"

/* $Header: XInsCmap.c,v 11.5 87/09/11 08:04:42 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XInstallColormap(dpy, cmap)
register Display *dpy;
Colormap cmap;
{
    register xResourceReq *req;
    LockDisplay(dpy);
    GetResReq(InstallColormap, cmap, req);
    UnlockDisplay(dpy);
    SyncHandle();
}

