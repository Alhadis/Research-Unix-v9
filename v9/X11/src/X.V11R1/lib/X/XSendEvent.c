#include "copyright.h"

/* $Header: XSendEvent.c,v 11.6 87/09/11 08:06:40 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_EVENTS
#include "Xlibint.h"

extern _XEventToWire();
/*
 * In order to avoid all images requiring _XEventToWire, we install the
 * event converter here if it has never been installed.
 */
XSendEvent(dpy, w, propagate, event_mask, event)
    register Display *dpy;
    Window w;
    Bool propagate;
    unsigned long event_mask; /* CARD32 */
    XEvent *event;
{
    register xSendEventReq *req;
    xEvent ev;
    register (**fp)();

    LockDisplay (dpy);
    GetReq(SendEvent, req);
    req->destination = w;
    req->propagate = propagate;
    req->eventMask = event_mask;

    /* call through display to find proper conversion routine */

    fp = &dpy->wire_vec[event->type & 0177];
    if (*fp == NULL) *fp = _XEventToWire;
    (**fp)(dpy, event, &ev);
    req->event = ev;

    UnlockDisplay(dpy);
    SyncHandle();
}
