#include "copyright.h"

/* $Header: XGrButton.c,v 11.7 87/09/11 08:04:30 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XGrabButton(dpy, button, modifiers, grab_window, owner_events, event_mask,
	    pointer_mode, keyboard_mode, confine_to, curs)
register Display *dpy;
unsigned int modifiers; /* CARD16 */
unsigned int button; /* CARD8 */
Window grab_window;
Bool owner_events;
unsigned int event_mask; /* CARD16 */
int pointer_mode, keyboard_mode;
Window confine_to;
Cursor curs;
{
    register xGrabButtonReq *req;
    LockDisplay(dpy);
    GetReq(GrabButton, req);
    req->modifiers = modifiers;
    req->button = button;
    req->grabWindow = grab_window;
    req->ownerEvents = owner_events;
    req->eventMask = event_mask;
    req->pointerMode = pointer_mode;
    req->keyboardMode = keyboard_mode;
    req->confineTo = confine_to;
    req->cursor = curs;
    UnlockDisplay(dpy);
    SyncHandle();
}
