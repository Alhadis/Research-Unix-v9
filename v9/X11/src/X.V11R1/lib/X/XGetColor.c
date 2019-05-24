#include "copyright.h"

/* $Header: XGetColor.c,v 11.12 87/09/11 08:03:55 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_REPLIES
#include "Xlibint.h"

Status XAllocNamedColor(dpy, cmap, colorname, hard_def, exact_def)
register Display *dpy;
Colormap cmap;
char *colorname; /* STRING8 */
XColor *hard_def; /* RETURN */
XColor *exact_def; /* RETURN */
{

    long nbytes;
    xAllocNamedColorReply rep;
    xAllocNamedColorReq *req;

    LockDisplay(dpy);
    GetReq(AllocNamedColor, req);

    req->cmap = cmap;
    nbytes = req->nbytes = strlen(colorname);
    req->length += (nbytes + 3) >> 2; /* round up to mult of 4 */

    _XSend(dpy, colorname, nbytes);
       /* _XSend is more efficient that Data, since _XReply follows */

    if (!_XReply (dpy, (xReply *) &rep, 0, xTrue)) {
	UnlockDisplay(dpy);
        SyncHandle();
        return (0);
    }

    exact_def->red = rep.exactRed;
    exact_def->green = rep.exactGreen;
    exact_def->blue = rep.exactBlue;

    hard_def->red = rep.screenRed;
    hard_def->green = rep.screenGreen;
    hard_def->blue = rep.screenBlue;

    exact_def->pixel = hard_def->pixel = rep.pixel;

    UnlockDisplay(dpy);
    SyncHandle();
    return (1);
}
