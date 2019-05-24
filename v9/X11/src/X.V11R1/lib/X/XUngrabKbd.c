#include "copyright.h"

/* $Header: XUngrabKbd.c,v 11.8 87/09/11 08:07:58 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"
XUngrabKeyboard (dpy, time)
        register Display *dpy;
	Time time;
{
        register xResourceReq *req;

	LockDisplay(dpy);
        GetResReq(UngrabKeyboard, time, req);
	UnlockDisplay(dpy);
	SyncHandle();
}

