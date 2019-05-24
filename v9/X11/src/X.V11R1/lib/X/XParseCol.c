#include "copyright.h"

/* $Header: XParseCol.c,v 11.13 87/09/11 08:05:21 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1985	*/

#define NEED_REPLIES
#include "Xlibint.h"

Status XParseColor (dpy, cmap, spec, def)
	register Display *dpy;
        Colormap cmap;
	register char *spec;
	XColor *def;
{
	register int n, i;
	int r, g, b;
	char c;

	n = strlen (spec);
	if (*spec != '#') {
	    xLookupColorReply reply;
	    register xLookupColorReq *req;
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
	    def->red = reply.exactRed;
	    def->green = reply.exactGreen;
	    def->blue = reply.exactBlue;
	    UnlockDisplay(dpy);
	    SyncHandle();
	    return (1);
	}
	spec++;
	n--;
	if (n != 3 && n != 6 && n != 9 && n != 12)
	    return (0);
	n /= 3;
	r = g = b = 0;
	do {
	    r = g;
	    g = b;
	    b = 0;
	    for (i = n; --i >= 0; ) {
		c = *spec++;
		b <<= 4;
		if (c >= '0' && c <= '9')
		    b |= c - '0';
		else if (c >= 'A' && c <= 'F')
		    b |= c - ('A' - 10);
		else if (c >= 'a' && c <= 'f')
		    b |= c - ('a' - 10);
		else return (0);
	    }
	} while (*spec != '\0');
	n <<= 2;
	n = 16 - n;
	def->red = r << n;
	def->green = g << n;
	def->blue = b << n;
	return (1);
}
