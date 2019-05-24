#include "copyright.h"

/* $Header: XKillCl.c,v 11.7 87/09/11 08:04:45 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"
XKillClient(dpy, resource)
	register Display *dpy;
	XID resource;
{
	register xResourceReq *req;
	LockDisplay(dpy);
        GetResReq(KillClient, resource, req);
	UnlockDisplay(dpy);
	SyncHandle();
}

