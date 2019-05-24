#include "copyright.h"

/* $Header: XFreeGC.c,v 11.8 87/09/11 08:03:43 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XFreeGC (dpy, gc)
    register Display *dpy;
    GC gc;
    {
    register xResourceReq *req;
    register _XExtension *ext;
    LockDisplay(dpy);
    GetResReq (FreeGC, gc->gid, req);
    _XFreeExtData(gc->ext_data);
    Xfree ((char *) gc);
    ext = dpy->ext_procs;
    while (ext) {		/* call out to any extensions interested */
	if (ext->free_GC != NULL) (*ext->free_GC)(dpy, gc, &ext->codes);
	ext = ext->next;
	}    
    UnlockDisplay(dpy);
    SyncHandle();
    }
    
