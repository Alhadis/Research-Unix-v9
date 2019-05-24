#include "copyright.h"

/* $Header: XLiHosts.c,v 11.12 87/08/06 16:45:02 newman Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/
/* This can really be considered an os dependent routine */

#define NEED_REPLIES
#include "Xlibint.h"
/*
 * can be freed using XFree.
 */

XHostAddress *XListHosts (dpy, nhosts, enabled)
    register Display *dpy;
    int *nhosts;
    Bool *enabled;
    {
    register XHostAddress *outbuf, *op;
    xListHostsReply reply;
    long nbytes;
    unsigned char *buf, *bp;
    register int i;
    register xListHostsReq *req;

    LockDisplay(dpy);
    GetReq (ListHosts, req);
    
    if (!_XReply (dpy, (xReply *) &reply, 0, xFalse)) {
       UnlockDisplay(dpy);
       SyncHandle();
       return (NULL);
	}

    *enabled = reply.enabled;
    *nhosts = reply.nHosts;
    if (*nhosts == 0) {
	UnlockDisplay(dpy);
        SyncHandle();
	return (NULL);
	}

    nbytes = reply.length << 2;	/* compute number of bytes in reply */
    op = outbuf = 
	(XHostAddress *) Xmalloc (nbytes + *nhosts * sizeof (XHostAddress));
    bp = buf = ((unsigned char  *)outbuf) + *nhosts * sizeof (XHostAddress);

    _XRead (dpy, buf, nbytes);

    for (i = 0; i < *nhosts; i++) {
	op->family = ((xHostEntry *) bp)->family;
	op->length =((xHostEntry *) bp)->length; 
	op->address = (char *) (((xHostEntry *) bp) + 1);
	bp += sizeof(xHostEntry) + (((op->length + 3) >> 2) << 2);
	op++;
	}
	

    UnlockDisplay(dpy);
    SyncHandle();

    return (outbuf);
    }

    


