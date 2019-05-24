#include "copyright.h"

/* $Header: XLiProps.c,v 11.16 87/07/10 00:39:09 newman Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_REPLIES
#include "Xlibint.h"

Atom *XListProperties(dpy, window, n_props)
register Display *dpy;
Window window;
int *n_props;  /* RETURN */
{
    long nbytes;
    xListPropertiesReply rep;
    Atom *properties;
    register xResourceReq *req;

    LockDisplay(dpy);
    GetResReq(ListProperties, window, req);
    if (!_XReply(dpy, (xReply *)&rep, 0, xFalse)) {
	*n_props = 0;
	UnlockDisplay(dpy);
        SyncHandle();
	return (NULL);
	}

    *n_props = rep.nProperties;
    properties = (Atom *) Xmalloc (
	(unsigned)(nbytes = ((long)rep.nProperties * sizeof(Atom))));
    _XRead (dpy, (char *) properties, nbytes);

    UnlockDisplay(dpy);
    SyncHandle();
    return (properties);
}
