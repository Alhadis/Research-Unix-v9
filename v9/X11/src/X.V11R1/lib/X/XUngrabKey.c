#include "copyright.h"

/* $Header: XUngrabKey.c,v 11.5 87/09/11 08:08:01 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XUngrabKey(dpy, key, modifiers, grab_window)
     register Display *dpy;
     int key;
     unsigned int modifiers;
     Window grab_window;

{
    register xUngrabKeyReq *req;

    LockDisplay(dpy);
    GetReq(UngrabKey, req);
    req->grabWindow = grab_window;
    req->modifiers = modifiers;
    req->key = key;
    UnlockDisplay(dpy);
    SyncHandle();
}



