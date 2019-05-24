#include "copyright.h"

/* $Header: XRestackWs.c,v 1.6 87/09/11 08:09:32 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XRestackWindows (dpy, windows, n)
    register Display *dpy;
    register Window *windows;
    int n;
    {
    int i = 0;
    LockDisplay(dpy);
    while (windows++, ++i < n) {
	register unsigned long *values;
	register xConfigureWindowReq *req;

    	GetReqExtra (ConfigureWindow, 8, req);
	req->window = *windows;
	req->mask = CWSibling | CWStackMode;
        values = (unsigned long *) (req + 1);
	*values++ = *(windows-1);
	*values   = Below;
	}
    UnlockDisplay(dpy);
    SyncHandle();
    }

    

    
