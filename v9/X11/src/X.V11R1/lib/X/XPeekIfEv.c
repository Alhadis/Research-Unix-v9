#include "copyright.h"

/* $Header: XPeekIfEv.c,v 11.9 87/09/11 08:09:07 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_EVENTS
#include "Xlibint.h"

extern _XQEvent *_qfree;

/*
 * Flush output and (wait for and) return the next event in the queue
 * that satisfies the predicate.
 * BUT do not remove it from the queue.
 */

XPeekIfEvent (dpy, event, predicate, arg)
	register Display *dpy;
	register XEvent *event;
	Bool (*predicate)();
	char *arg;
{
	register _XQEvent *qelt;
	xEvent ev;

	LockDisplay(dpy);
	_XFlush (dpy);

	qelt = dpy->head;
	while (qelt) {
	    if ((*predicate)(dpy, &qelt->event, arg)) {
		*event = qelt->event;
		UnlockDisplay(dpy);
		return;
		}
		qelt = qelt->next;
	}

	/* 
	 * if no match in queue, then block waiting for event, enqueing
	 * other events all the while.
	 */
	while (1) {
	    _XRead (dpy, (char *)&ev, (long)sizeof(xEvent));
	    if (ev.u.u.type == X_Error)
	    	_XError (dpy, (xError *) &ev);
	    else {  /* it's an event packet */
	    	_XEnq (dpy, &ev);
		if ((*predicate)(dpy, &dpy->tail->event, arg)) {
			*event = dpy->tail->event;
			UnlockDisplay(dpy);
			return;
			}
		}
	}
}

