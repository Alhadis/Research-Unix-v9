#include "copyright.h"

/* $Header: XWinEvent.c,v 11.12 87/09/11 08:08:27 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1985	*/

#define NEED_EVENTS
#include "Xlibint.h"

extern _XQEvent *_qfree;
extern long _event_to_mask[];

/* 
 * Flush output and (wait for and) return the next event in the queue
 * for the given window matching one of the events in the mask.
 * Events earlier in the queue are not discarded.
 */

XWindowEvent (dpy, w, mask, event)
        register Display *dpy;
	Window w;		/* Selected window. */
	long mask;		/* Selected event mask. */
	register XEvent *event;	/* XEvent to be filled in. */
{
	register _XQEvent *prev, *qelt;

        LockDisplay(dpy);
	_XFlush (dpy);
	for (prev = NULL, qelt = dpy->head;
	     qelt && (qelt->event.type != KeymapNotify)
		  && (qelt->event.xany.window != w ||
		      !(_event_to_mask [qelt->event.type] & mask));
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
	    _XRead (dpy, (char *)&pe, (long) sizeof(xEvent));
	    /* go call through display to find proper event reformatter */
	    (*dpy->event_vec[pe.u.u.type & 0177])(dpy, event, &pe);
	    if (event->type == X_Error)
		_XError(dpy, (xError *) &pe);
	    else if ((event->xany.window == w) 
		&& (_event_to_mask[event->type] & mask)) {
		UnlockDisplay(dpy);
		return;
	    } else
		_XEnq(dpy, &pe);
	}
}
