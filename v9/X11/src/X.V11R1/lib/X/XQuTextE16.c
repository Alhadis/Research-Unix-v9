#include "copyright.h"

/* $Header: XQuTextE16.c,v 11.7 87/09/11 08:06:10 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986, 1987	*/

#define NEED_REPLIES
#include "Xlibint.h"

XQueryTextExtents16 (dpy, fid, string, nchars, dir, font_ascent, font_descent,
                     overall)
    register Display *dpy;
    Font fid;
    register short *string;
    register int nchars;
    int *dir;
    int *font_ascent, *font_descent;
    register XCharStruct *overall;
{
    int indian;
    register long i;
    register CARD16 *buf;
    xQueryTextExtentsReply rep;
    long nbytes;
    register xQueryTextExtentsReq *req;

    LockDisplay(dpy);
    GetReq(QueryTextExtents, req);
    req->fid = fid;
    nbytes = nchars << 1;
    req->length += (nbytes + 3)>>2;
    req->oddLength = nchars & 1;
    buf = (CARD16 *) _XAllocScratch (dpy, 
		(unsigned long)nchars * sizeof(CARD16));
    /*
     * this call does not have to be all that swift, as it is doing round
     * trip; have to expand to 16 bit anyway.
     */
    for (i = 0; i < nchars; i++) {
	 buf[i] = (unsigned)*string++;
	}
    /*
     * always send big indian....
     */
    indian = 1;
    if (*(char *) & indian) _swapshort ((char *)buf, nbytes);
    Data (dpy, (char *) buf, nbytes);
    if (!_XReply (dpy, (xReply *)&rep, 0, xTrue))
	return (0);
    *dir = rep.drawDirection;
    *font_ascent = rep.fontAscent;
    *font_descent = rep.fontDescent;
    overall->ascent = rep.overallAscent;
    overall->descent = rep.overallDescent;
    overall->width  = rep.overallWidth;
    overall->lbearing = rep.overallLeft;
    overall->rbearing = rep.overallRight;
    UnlockDisplay(dpy);
    SyncHandle();
    return (1);
}

