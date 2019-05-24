#include "copyright.h"

/* $Header: XNextEvent.c,v 11.13 87/09/11 08:05:18 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_EVENTS
#include "Xlibint.h"

extern _XQEvent *_qfree;

/* 
 *Flush output and (wait for and) return the next event in the queue.
 */

XNextEvent (dpy, event)
	register Display *dpy;
	register XEvent *event;
{
	register _XQEvent *qelt;
	
	LockDisplay(dpy);
	_XFlush (dpy);
	
	/* if the queue is empty, read as many events as possible
	   and enqueue them */
	while (dpy->head == NULL) {
	    char buf[BUFSIZE];
	    long pend_not_register; /* because can't "&" a register variable */
	    register long pend;
	    register xEvent *ev;

	    /* find out how much data can be read */
	    if (BytesReadable(dpy->fd, (char *) &pend_not_register) < 0)
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
		else  /* it's an event packet; enqueue it */
		    _XEnq (dpy, ev);
	    }

	*event = (qelt = dpy->head)->event;

	/* move the head of the queue to the free list */
	if ((dpy->head = qelt->next) == NULL)
	    dpy->tail = NULL;
	qelt->next = _qfree;
	_qfree = qelt;
	dpy->qlen--;
	UnlockDisplay(dpy);
	return;
}

