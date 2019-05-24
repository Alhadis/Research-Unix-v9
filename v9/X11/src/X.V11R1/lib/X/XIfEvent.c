#include "copyright.h"

/* $Header: XIfEvent.c,v 11.8 87/09/11 08:09:02 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_EVENTS
#include "Xlibint.h"

extern _XQEvent *_qfree;

/* 
 * Flush output and (wait for and) return the next event matching the
 * predicate in the queue.
 */

XIfEvent (dpy, event, predicate, arg)
	register Display *dpy;
	Bool (*predicate)();		/* function to call */
	register XEvent *event;
	char *arg;
{
	register _XQEvent *qelt, *prev;
	
	LockDisplay(dpy);
	_XFlush (dpy);
	/*
	 * first must search queue to find if any events match.
	 * If so, then remove from queue.
	 */
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
		return;
		}
		qelt = (prev = qelt)->next;
	}

	
	/* if the queue is empty, read as many events as possible
	   and enqueue them */
	while (1) {
	    char buf[BUFSIZE];
	    long pend_not_register; /* because can't "&" a register variable */
	    register long pend;
	    register xEvent *ev;
	    XEvent scratch;

	    /* find out how much data can be read */
	    if (BytesReadable(dpy->fd, (char *)&pend_not_register) < 0)
	    	(*_XIOErrorFunction)(dpy);
	    pend = pend_not_register;

	    /* must read at least one xEvent; if none is pending, then
	       we'll just block waiting for it */
	    if (pend < sizeof(xEvent))
	    	pend = sizeof (xEvent);
		
	    /* but we won't read more than the max buffer size */
	    if (pend > BUFSIZE)
	    	pend = BUFSIZE;

	    /* round down to an integral number of XReps */
	    pend = (pend / sizeof (xEvent)) * sizeof (xEvent);

	    _XRead (dpy, buf, pend);
	    for (ev = (xEvent *) buf; pend > 0; ev++, pend -= sizeof(xEvent))
		if (ev->u.u.type == X_Error)
		    _XError (dpy, (xError *) ev);
		else  { 
		    /* it's an event packet; enqueue it if not predicate */
		    (*dpy->event_vec[ev->u.u.type & 0177])(dpy, &scratch, ev);
		    if ((*predicate)(dpy, &scratch, arg)) {
			ev++;                   /* Enqueue remaining events */
			pend -= sizeof(xEvent);
			for (; pend > 0; ev++, pend -= sizeof(xEvent))
			    if (ev->u.u.type == X_Error)
				_XError (dpy, (xError *) ev);
			    else
				_XEnq (dpy, ev);
			*event = scratch;
			UnlockDisplay(dpy);
			return;
			}
		    _XEnq (dpy, ev);
	    }
	}
}

