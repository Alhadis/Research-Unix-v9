#include "copyright.h"

/* $Header: XSetPMask.c,v 11.7 87/09/11 08:07:09 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XSetPlaneMask (dpy, gc, planemask)
register Display *dpy;
GC gc;
unsigned long planemask; /* CARD32 */
{
    LockDisplay(dpy);
    if (gc->values.plane_mask != planemask) {
	gc->values.plane_mask = planemask;
	gc->dirty |= GCPlaneMask;
    }
    UnlockDisplay(dpy);	
    SyncHandle();
}
