#include "copyright.h"

/* $Header: XGetMoEv.c,v 11.13 87/09/11 08:04:16 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_REPLIES
#include "Xlibint.h"

XTimeCoord *XGetMotionEvents(dpy, w, start, stop, nEvents)
    register Display *dpy;
    Time start, stop;
    Window w;
    int *nEvents;  /* RETURN */
{       
    xGetMotionEventsReply rep;
    register xGetMotionEventsReq *req;
    XTimeCoord *tc;
    long nbytes;
    LockDisplay(dpy);
    GetReq(GetMotionEvents, req);
    req->window = w;
/* XXX is this right for all machines? */
    req->start = start;
    req->stop  = stop;
    if (!_XReply (dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
        SyncHandle();
	*nEvents = 0;
	return (NULL);
	}
    
    *nEvents = rep.nEvents;
    tc = (XTimeCoord *) Xmalloc (
		(unsigned)(nbytes = (long)rep.nEvents * sizeof (XTimeCoord)));
    if (!tc) {
	/* XXX this is wrong!!  we need to read and throw away the data
           somehow.  Probably we should try to malloc less space and repeatedly
           read the events into the smaller space.... */
	*nEvents = 0;
	UnlockDisplay(dpy);
        SyncHandle();
	return (NULL);
	}
    _XRead (dpy, (char *) tc, nbytes);
    /* XXX need to do something different if short isn't 16-bits, or long
       isn't 32-bits, since in that case XTimeCoord won't be the same as
       protocol structure */

    UnlockDisplay(dpy);
    SyncHandle();
    return (tc);
}

