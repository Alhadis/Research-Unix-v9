#include "copyright.h"

/* $Header: XPending.c,v 11.10 87/09/11 08:05:32 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_EVENTS
#define NEED_REPLIES
#include "Xlibint.h"

/* Read in any pending events and return the number of queued events. */

int XPending (dpy)
    register Display *dpy;
{
	register int len;
	int pend;
	char buf[BUFSIZE];
	register xReply *rep;
	
	LockDisplay(dpy);
	_XFlush (dpy);
	if (BytesReadable(dpy->fd, (char *) &pend) < 0)
	    (*_XIOErrorFunction)(dpy);
	if ((len = pend) < sizeof(xReply)) {
	    UnlockDisplay(dpy);
	    return(dpy->qlen);
	    }
	else if (len > BUFSIZE)
	    len = BUFSIZE;
	len /= sizeof(xReply);
	pend = len * sizeof(xReply);
	_XRead (dpy, buf, (long) pend);
	for (rep = (xReply *) buf; len > 0; rep++, len--) {
	    if (rep->generic.type == X_Error)
		_XError(dpy, (xError *)rep);
	    else   /* must be an event packet */
		_XEnq(dpy, (xEvent *) rep);
	}
	UnlockDisplay(dpy);
	return(dpy->qlen);
}

