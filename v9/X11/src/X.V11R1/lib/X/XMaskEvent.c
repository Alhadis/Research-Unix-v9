#include "copyright.h"

/* $Header: XMaskEvent.c,v 11.14 87/09/11 08:05:11 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_EVENTS
#include "Xlibint.h"

extern _XQEvent *_qfree;
extern long _event_to_mask[];

/* 
 * Flush output and (wait for and) return the next event in the queue
 * matching one of the events in the mask.
 * Events earlier in the queue are not discarded.
 */

XMaskEvent (dpy, mask, event)
	register Display *dpy;
	unsigned long mask;		/* Selected event mask. */
	register XEvent *event;	/* XEvent to be filled in. */
{
	register _XQEvent *prev, *qelt;

	LockDisplay(dpy);
	_XFlush (dpy);
	for (prev = NULL, qelt = dpy->head;
	     qelt &&  !(_event_to_mask[qelt->event.type] & mask);
	     qelt = (prev = qelt)->next) ;
	if (qelt) {
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
	    return;
	}
	while (1) {
	    xEvent pe;		/* wire protocol event */
	    _XRead (dpy, (char *) &pe, (long)sizeof(xEvent));
	    (*dpy->event_vec[pe.u.u.type & 0177])(dpy, event, &pe);
	    if (event->type == X_Error)
		_XError(dpy, (xError *) &pe);
	    else if (_event_to_mask[event->type] & mask) {
		UnlockDisplay (dpy);
		return;
		}
	    else
		_XEnq(dpy, &pe);
	}
}

