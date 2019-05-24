#include "copyright.h"

/* $Header: XStColors.c,v 11.9 87/09/11 08:07:31 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XStoreColors(dpy, cmap, defs, ncolors)
register Display *dpy;
Colormap cmap;
XColor defs[];
int ncolors;
{
    register int i;
    xColorItem citem;
    register xStoreColorsReq *req;

    LockDisplay(dpy);    
    GetReq(StoreColors, req);

    req->cmap = cmap;

    req->length += (ncolors * sizeof(xColorItem)) >> 2; /* assume size is 4*n */

    for (i = 0; i < ncolors; i++) {
	citem.pixel = defs[i].pixel;
	citem.red = defs[i].red;
	citem.green = defs[i].green;
	citem.blue = defs[i].blue;
	citem.flags = defs[i].flags;

	/* note that xColorItem doesn't contain all 16-bit quantities, so
	   we can't use PackData */
	Data(dpy, (char *)&citem, (long) sizeof(xColorItem)); 
			/* assume size is 4*n */
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
