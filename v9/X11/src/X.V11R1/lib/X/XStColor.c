#include "copyright.h"

/* $Header: XStColor.c,v 11.5 87/09/11 08:07:28 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XStoreColor(dpy, cmap, def)
register Display *dpy;
Colormap cmap;
XColor *def;
{
    xColorItem *citem;
    register xStoreColorsReq *req;

    LockDisplay(dpy);
    GetReqExtra(StoreColors, sizeof(xColorItem), req); /* assume size is 4*n */

    req->cmap = cmap;

    citem = (xColorItem *) (req + 1);

    citem->pixel = def->pixel;
    citem->red = def->red;
    citem->green = def->green;
    citem->blue = def->blue;
    citem->flags = def->flags; /* do_red, do_green, do_blue */
    UnlockDisplay(dpy);
    SyncHandle();
}
