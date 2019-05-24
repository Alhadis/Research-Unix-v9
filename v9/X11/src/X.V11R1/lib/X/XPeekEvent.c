#include "copyright.h"

/* $Header: XPeekEvent.c,v 11.11 87/09/11 08:05:29 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_EVENTS
#include "Xlibint.h"

extern _XQEvent *_qfree;

/* 
 * Flush output and (wait for and) return the next event in the queue,
 * BUT do not remove it from the queue.
 */

XPeekEvent (dpy, event)
	register Display *dpy;
	register XEvent *event;
{
	register _XQEvent *qelt;
	xEvent ev;

	LockDisplay(dpy);
	_XFlush (dpy);
	if (qelt = (_XQEvent *)dpy->head) {
	    *event = qelt->event;
	    UnlockDisplay(dpy);
	    return;
	    }

	while (1) {
	    _XRead (dpy, (char *)&ev, (long) sizeof(xEvent));
	    if (ev.u.u.type == X_Error)
	    	_XError (dpy, (xError *) event);
	    else {  /* it's an event packet */
	    	_XEnq (dpy, &ev);
		*event = dpy->head->event;
		UnlockDisplay(dpy);
		return;
		}
	}
}

