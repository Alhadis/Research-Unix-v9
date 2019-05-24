#include "copyright.h"

/* $Header: XSync.c,v 11.12 87/09/11 08:07:42 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_REPLIES
#define NEED_EVENTS
#include "Xlibint.h"

extern _XQEvent *_qfree;

/* Synchronize with errors and events, optionally discarding pending events */

XSync (dpy, discard)
    register Display *dpy;
    int discard;
{
    xGetInputFocusReply rep;
    register xReq *req;

    LockDisplay(dpy);
    GetEmptyReq(GetInputFocus, req);
    (void) _XReply (dpy, (xReply *)&rep, 0, xTrue);

    if (discard && dpy->head) {
       ((_XQEvent *)dpy->tail)->next = _qfree;
       _qfree = (_XQEvent *)dpy->head;
       dpy->head = dpy->tail = NULL;
       dpy->qlen = 0;
    }
    UnlockDisplay(dpy);
}

