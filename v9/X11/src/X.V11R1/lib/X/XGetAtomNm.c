#include "copyright.h"

/* $Header: XGetAtomNm.c,v 11.13 87/09/11 08:03:51 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/
#define NEED_REPLIES
#include "Xlibint.h"

char *XGetAtomName(dpy, atom)
register Display *dpy;
Atom atom;
{
    xGetAtomNameReply rep;
    xResourceReq *req;
    char *storage;

    LockDisplay(dpy);
    GetResReq(GetAtomName, atom, req);

    if (_XReply(dpy, (xReply *)&rep, 0, xFalse) == 0) {
	UnlockDisplay(dpy);
	SyncHandle();
	return(NULL);
	}

    storage = (char *) Xmalloc(rep.nameLength+1);

    _XReadPad(dpy, storage, (long)rep.nameLength);
    storage[rep.nameLength] = '\0';

    UnlockDisplay(dpy);
    SyncHandle();
    return(storage);
}
