#include "copyright.h"

/* $Header: XChAccCon.c,v 11.7 87/09/11 08:01:23 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XEnableAccessControl(dpy) 
    register Display *dpy;

{
    XSetAccessControl(dpy, EnableAccess);
}

XDisableAccessControl(dpy) 
    register Display *dpy;

{
    XSetAccessControl(dpy, DisableAccess);
}

XSetAccessControl(dpy, mode)
    register Display *dpy; 
    int mode;

{
    register xSetAccessControlReq *req;

    LockDisplay(dpy);
    GetReq(SetAccessControl, req);
    req->mode = mode;
    UnlockDisplay(dpy);
    SyncHandle();
}

