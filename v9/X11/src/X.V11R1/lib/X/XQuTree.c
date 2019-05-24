#include "copyright.h"

/* $Header: XQuTree.c,v 11.15 87/09/11 08:06:20 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_REPLIES
#include "Xlibint.h"

Status XQueryTree (dpy, w, root, parent, children, nchildren)
    register Display *dpy;
    Window w;
    Window *root;
    Window *parent;  /* RETURN */
    Window **children; /* RETURN */
    unsigned int *nchildren;  /* RETURN */
{
    long nbytes;
    xQueryTreeReply rep;
    register xResourceReq *req;

    LockDisplay(dpy);
    GetResReq(QueryTree, w, req);
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	UnlockDisplay(dpy);
	SyncHandle();
	return (0);
	}
    *parent = rep.parent;
    *root = rep.root;
    *nchildren = rep.nChildren;
    *children = (Window *) NULL;
    if (rep.nChildren != 0) {
      *children = (Window *) Xmalloc (
	    (unsigned)(nbytes = rep.nChildren * sizeof(Window)));
      _XRead (dpy, (char *) *children, nbytes);
    }
       /* Note: won't work if sizeof(Window) is not 32 bits! */
    UnlockDisplay(dpy);
    SyncHandle();
    return (1);
}

