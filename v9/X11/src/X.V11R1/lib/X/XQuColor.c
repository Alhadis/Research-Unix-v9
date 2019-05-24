#include "copyright.h"

/* $Header: XQuColor.c,v 11.13 87/09/11 08:05:47 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_REPLIES
#include "Xlibint.h"

XQueryColor(dpy, cmap, def)
    register Display *dpy;
    Colormap cmap;
    XColor *def;	/* RETURN */
{
    xrgb color;
    xQueryColorsReply rep;
    register xQueryColorsReq *req;

    LockDisplay(dpy);
    GetReqExtra(QueryColors, 4, req); /* a pixel (CARD32) is 4 bytes */
    req->cmap = cmap;

    * (unsigned long *) (req + 1) = def->pixel;

    if (_XReply(dpy, (xReply *) &rep, 0, xFalse) != 0) {

	    _XRead(dpy, (char *)&color, (long) sizeof(xrgb));

	    def->red = color.red;
	    def->blue = color.blue;
	    def->green = color.green;
	    def->flags = DoRed | DoGreen | DoBlue;
        }
    UnlockDisplay(dpy);
    SyncHandle();
}
