#include "copyright.h"

/* $Header: XChkWinEv.c,v 11.9 87/09/08 14:30:29 newman Exp $ */
/* Copyright    Massachusetts Institute of Technology    1985, 1987	*/
#define NEED_EVENTS
#include "Xlibint.h"

extern _XQEvent *_qfree;
extern long _event_to_mask[];
/* 
 * Flush output and return the next event in the queue
 * for the given window matching one of the events in the mask if there
 * is one.  Return whether such an event was found.
 * Events earlier in the queue are not discarded.
 */

Bool XCheckWindowEvent (dpy, w, mask, event)
        register Display *dpy;
	Window w;		/* Selected window. */
	int mask;		/* Selected event mask. */
	register XEvent *event;	/* XEvent to be filled in. */
{
 	register _XQEvent *prev, *qelt;
	int ret_value;

	(void) XPending(dpy);
        LockDisplay(dpy);
	_XFlush (dpy);
	for (prev = NULL, qelt = dpy->head;
	     qelt && (qelt->event.xany.window != w ||
		      !(_event_to_mask[qelt->event.type] & mask));
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
