#include "copyright.h"

/* $Header: XChGC.c,v 11.6 87/09/11 08:01:32 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XChangeGC (dpy, gc, valuemask, values)
    register Display *dpy;
    GC gc;
    unsigned long valuemask;
    XGCValues *values;
{
    LockDisplay(dpy);

    if (valuemask) _XUpdateGCCache (gc, valuemask, values);

    /* if any Resource ID changed, must flush */
    if (valuemask & (GCFont | GCTile | GCStipple)) _XFlushGCCache(dpy, gc);
    UnlockDisplay(dpy);
    SyncHandle();
}

