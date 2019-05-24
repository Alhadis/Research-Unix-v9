#include "copyright.h"

/* $Header: XMapWindow.c,v 11.5 87/09/11 08:05:08 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"
XMapWindow (dpy, w)
	Window w;
	register Display *dpy;
{
	register xResourceReq *req;
	LockDisplay (dpy);
        GetResReq(MapWindow, w, req);
	UnlockDisplay (dpy);
	SyncHandle();
}

