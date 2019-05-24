#include "copyright.h"

/* $Header: XChkIfEv.c,v 11.5 87/09/08 14:29:43 newman Exp $ */
/* Copyright    Massachusetts Institute of Technology    1985, 1987	*/
#define NEED_EVENTS
#include "Xlibint.h"

extern _XQEvent *_qfree;

/* 
 * Flush output and return the next event in the queue matching the
 * predicate.  Return whether such an event was found.
 * Events earlier in the queue are not discarded.
 */

Bool XCheckIfEvent (dpy, event, predicate, arg)
        register Display *dpy;
	Bool (*predicate)();		/* function to call */
	register XEvent *event;		/* XEvent to be filled in. */
	char *arg;
{
 	register _XQEvent *prev, *qelt;

        LockDisplay(dpy);
	_XFlush (dpy);
	prev = NULL;
	qelt = dpy->head;
	while (qelt) {
	    if ((*predicate)(dpy, &qelt->event, arg)) {
		*event = qelt->event;
	        if (prev) {
	            if ((prev->next = qelt->next) == NULL)
	                dpy->tail = prev;
	        } else {
	            if ((dpy->head = qelt->next) == NULL)
                    dpy->tail = NULL;
                }
	        qelt->next = _qfree;
	        _qfree = qelt;
	        dpy->qlen--;
		UnlockDisplay(dpy);
		return 1;
		}
		qelt = (prev = qelt)->next;
	}

	UnlockDisplay(dpy);
	return 0;
}
