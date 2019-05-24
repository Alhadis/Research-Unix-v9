#include "copyright.h"

/* $Header: XQuKeybd.c,v 11.8 87/09/11 08:06:01 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_REPLIES
#include "Xlibint.h"

struct kmap {
  char map[32];
};

XQueryKeymap(dpy, keys)
    register Display *dpy;
    char keys[32];

{       
    xQueryKeymapReply rep;
    register xReq *req;

    LockDisplay(dpy);
    GetEmptyReq(QueryKeymap, req);
    (void) _XReply(dpy, (xReply *)&rep, 
       (sizeof (xQueryKeymapReply) - sizeof (xReply)) >> 2, xTrue);
    *(struct kmap *) keys = *(struct kmap *)rep.map;  /* faster than bcopy */
    UnlockDisplay(dpy);
    SyncHandle();
}

