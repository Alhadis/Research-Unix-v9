#include "copyright.h"

/* $Header: XFreeCols.c,v 11.6 87/09/11 08:03:32 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XFreeColors(dpy, cmap, pixels, npixels, planes)
register Display *dpy;
Colormap cmap;
unsigned long *pixels; /* LISTofCARD32 */
int npixels;
unsigned long planes; /* CARD32 */
{
    register xFreeColorsReq *req;
    register long nbytes;

    LockDisplay(dpy);
    GetReq(FreeColors, req);
    req->cmap = cmap;
    req->planeMask = planes;

    /* on the VAX, each pixel is a 32-bit (unsigned) integer */
    req->length += npixels;

    /* divide by 4 once; Data may be a macro and thus do it
       multiple times if we pass it as a parameter */

    nbytes = npixels << 2;

    Data(dpy, (char *) pixels, nbytes);
    UnlockDisplay(dpy);
    SyncHandle();
}

