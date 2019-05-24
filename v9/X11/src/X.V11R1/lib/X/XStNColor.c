#include "copyright.h"

/* $Header: XStNColor.c,v 11.8 87/09/11 08:07:36 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XStoreNamedColor(dpy, cmap, name, pixel, flags)
register Display *dpy;
Colormap cmap;
unsigned long pixel; /* CARD32 */
char *name; /* STRING8 */
int flags;  /* DoRed, DoGreen, DoBlue */
{
    unsigned int nbytes;
    register xStoreNamedColorReq *req;

    LockDisplay(dpy);
    GetReq(StoreNamedColor, req);

    req->cmap = cmap;
    req->flags = flags;
    req->pixel = pixel;
    req->nbytes = nbytes = strlen(name);
    req->length += (nbytes + 3) >> 2; /* round up to multiple of 4 */
    Data(dpy, name, (long)nbytes);
    UnlockDisplay(dpy);
    SyncHandle();
}


