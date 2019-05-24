#include "copyright.h"

/* $Header: XChkMaskEv.c,v 11.11 87/09/08 14:29:58 newman Exp $ */
/* Copyright    Massachusetts Institute of Technology    1985, 1987	*/
#define NEED_EVENTS
#include "Xlibint.h"

extern _XQEvent *_qfree;
extern long _event_to_mask[];

/* 
 * Flush output and return the next event in the queue
 * matching one of the events in the mask if there is one.
 * Return whether such an event was found.
 * Events earlier in the queue are not discarded.
 */

Bool XCheckMaskEvent (dpy, mask, event)
        register Display *dpy;
	unsigned long mask;		/* Selected event mask. */
	register XEvent *event;	/* XEvent to be filled in. */
{
	register _XQEvent *prev, *qelt;
        int ret_value;

	(void) XPending(dpy);
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
            ret_value = 1;
	} else ret_value = 0;

	UnlockDisplay(dpy);
	return ret_value;
}
