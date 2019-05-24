#include "copyright.h"

/* $Header: XSetPntMap.c,v 11.3 87/09/11 08:10:00 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_REPLIES

#include "Xlibint.h"
/* returns either  DeviceMappingSuccess or DeviceMappingBusy  */

int XSetPointerMapping (dpy, map, nmaps)
    register Display *dpy;
    unsigned char map[];
    int nmaps;
    {
    register xSetPointerMappingReq *req;
    xSetPointerMappingReply rep;

    LockDisplay(dpy);
    GetReq (SetPointerMapping, req);
    req->nElts = nmaps;
    req->length += (nmaps + 3)>>2;
    Data (dpy, (char *)map, (long) nmaps);
    if (_XReply (dpy, (xReply *)&rep, 0, xFalse) == 0) 
	rep.success = MappingSuccess;
    UnlockDisplay(dpy);
    SyncHandle();
    return ((int) rep.success);
    }

XChangeKeyboardMapping (dpy, first_keycode, keysyms_per_keycode, 
		     keysyms, nkeycodes)
    register Display *dpy;
    int first_keycode;
    int keysyms_per_keycode;
    KeySym *keysyms;
    int nkeycodes;
    {
    register int nbytes;
    register xChangeKeyboardMappingReq *req;

    LockDisplay(dpy);
    GetReq (ChangeKeyboardMapping, req);
    req->firstKeyCode = first_keycode;
    req->keyCodes = nkeycodes;
    req->keySymsPerKeyCode = keysyms_per_keycode;
    req->firstKeyCode = first_keycode;
    req->length += nkeycodes * keysyms_per_keycode;
    nbytes = keysyms_per_keycode * nkeycodes * sizeof (CARD32);
    Data (dpy, (char *)keysyms, nbytes);
    UnlockDisplay(dpy);
    SyncHandle();
    return;
    }
    
