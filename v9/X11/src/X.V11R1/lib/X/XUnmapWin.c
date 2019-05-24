#include "copyright.h"

/* $Header: XUnmapWin.c,v 11.5 87/09/11 08:08:20 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"
XUnmapWindow (dpy, w)
        register Display *dpy;
	Window w;
{
        register xResourceReq *req;

        LockDisplay(dpy);
        GetResReq(UnmapWindow, w, req);
	UnlockDisplay(dpy);
	SyncHandle();
}

