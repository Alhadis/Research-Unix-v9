#include "copyright.h"

/* $Header: XLookupCol.c,v 11.6 87/09/11 08:10:04 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1985	*/

#define NEED_REPLIES
#include "Xlibint.h"

Status XLookupColor (dpy, cmap, spec, def, scr)
	register Display *dpy;
        Colormap cmap;
	register char *spec;
	XColor *def, *scr;
{
	register int n;
	xLookupColorReply reply;
	register xLookupColorReq *req;

	n = strlen (spec);
	LockDisplay(dpy);
	GetReq (LookupColor, req);
	req->cmap = cmap;
	req->nbytes = n;
	req->length += (n + 3) >> 2;
	Data (dpy, spec, (long)n);
	if (!_XReply (dpy, (xReply *) &reply, 0, xTrue)) {
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return (0);
	    }
	def->red   = reply.exactRed;
	def->green = reply.exactGreen;
	def->blue  = reply.exactBlue;

	scr->red   = reply.screenRed;
	scr->green = reply.screenGreen;
	scr->blue  = reply.screenBlue;

	UnlockDisplay(dpy);
	SyncHandle();
	return (1);
}
