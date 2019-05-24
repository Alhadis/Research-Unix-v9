#include "copyright.h"

/* $Header: XReconfWin.c,v 11.9 87/09/11 08:06:30 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XConfigureWindow(dpy, w, mask, changes)
    register Display *dpy;
    Window w;
    unsigned int mask;
    XWindowChanges *changes;
    {
    unsigned long values[7];
    register unsigned long *value = values;
    long nvalues;
    register xConfigureWindowReq *req;

    LockDisplay(dpy);
    GetReq(ConfigureWindow, req);
    req->window = w;
    req->mask = mask;

    /* Warning!  This code assumes that "unsigned long" is 32-bits wide */

    if (mask & CWX)
	*value++ = changes->x;
	
    if (mask & CWY)
    	*value++ = changes->y;

    if (mask & CWWidth)
    	*value++ = changes->width;

    if (mask & CWHeight)
    	*value++ = changes->height;

    if (mask & CWBorderWidth)
    	*value++ = changes->border_width;

    if (mask & CWSibling)
	*value++ = changes->sibling;

    if (mask & CWStackMode)
        *value++ = changes->stack_mode;

    req->length += (nvalues = value - values);

    /* note: Data is a macro that uses its arguments multiple
       times, so "nvalues" is changed in a separate assignment
       statement */

    nvalues <<= 2;
    Data (dpy, (char *) values, nvalues);
    UnlockDisplay(dpy);
    SyncHandle();

    }
