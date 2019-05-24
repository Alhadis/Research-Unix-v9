#include "copyright.h"

/* Copyright 	Massachusetts Institute of Technology  1986 */
/* $Header: XPutBEvent.c,v 11.8 87/09/11 08:05:41 toddb Exp $ */
/* XPutBackEvent puts an event back at the head of the queue. */
#define NEED_EVENTS
#include "Xlibint.h"

extern _XQEvent *_qfree;

XPutBackEvent (dpy, event)
	register Display *dpy;
	register XEvent *event;
	{
	register _XQEvent *qelt;

	LockDisplay(dpy);
	if (!_qfree) {
    	    _qfree = (_XQEvent *) Xmalloc (sizeof (_XQEvent));
	    _qfree->next = NULL;
	    }
	qelt = _qfree;
	_qfree = qelt->next;
	qelt->next = dpy->head;
	qelt->event = *event;
	dpy->head = qelt;
	if (dpy->tail == NULL)
	    dpy->tail = qelt;
	dpy->qlen++;
	UnlockDisplay(dpy);
	}
