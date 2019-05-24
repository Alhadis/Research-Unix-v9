#include "copyright.h"

/* $Header: XGetWAttrs.c,v 11.18 87/09/11 08:09:17 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_REPLIES
#include "Xlibint.h"

Status XGetWindowAttributes(dpy, w, att)
     register Display *dpy;
     Window w;
     XWindowAttributes *att;

{       
    xGetWindowAttributesReply rep;
    xGetGeometryReply rep2;
    register xResourceReq *req1;
    register xResourceReq *req2;
    register int i;
    register Screen *sp;
 
    LockDisplay(dpy);
    GetResReq(GetWindowAttributes, w, req1);
    if (!_XReply (dpy, (xReply *)&rep,
       (sizeof(xGetWindowAttributesReply) - sizeof(xReply)) >> 2, xTrue)) {
		UnlockDisplay(dpy);
		SyncHandle();
      		return (0);
	}
    att->class = rep.class;
    att->bit_gravity = rep.bitGravity;
    att->win_gravity = rep.winGravity;
    att->backing_store = rep.backingStore;
    att->backing_planes = rep.backingBitPlanes;
    att->backing_pixel = rep.backingPixel;
    att->save_under = rep.saveUnder;
    att->colormap = rep.colormap;
    att->map_installed = rep.mapInstalled;
    att->map_state = rep.mapState;
    att->all_event_masks = rep.allEventMasks;
    att->your_event_mask = rep.yourEventMask;
    att->do_not_propagate_mask = rep.doNotPropagateMask;
    att->override_redirect = rep.override;
    att->visual = _XVIDtoVisual (dpy, rep.visualID);
    
    GetResReq(GetGeometry, w, req2);

    if (!_XReply (dpy, (xReply *)&rep2, 0, xTrue)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return (0);
	}
    att->x = rep2.x;
    att->y = rep2.y;
    att->width = rep2.width;
    att->height = rep2.height;
    att->border_width = rep2.borderWidth;
    att->depth = rep2.depth;
    att->root = rep2.root;
    /* find correct screen so that applications find it easier.... */
    for (i = 0; i < dpy->nscreens; i++) {
	sp = &dpy->screens[i];
	if (sp->root == att->root) {
	    att->screen = sp;
	    break;
	}
    }
    UnlockDisplay(dpy);
    SyncHandle();
    return(1);
}

