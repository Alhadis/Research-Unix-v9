#include "copyright.h"

/* $Header: XGrServer.c,v 11.5 87/09/11 08:04:37 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"
XGrabServer (dpy)
register Display *dpy;
{
	register xReq *req;
	LockDisplay(dpy);
        GetEmptyReq(GrabServer, req);
	UnlockDisplay(dpy);
	SyncHandle();
}

