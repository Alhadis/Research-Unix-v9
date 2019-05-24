#include "copyright.h"

/* $Header: XLiICmaps.c,v 11.13 87/09/11 08:04:49 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#define NEED_REPLIES
#include "Xlibint.h"

Colormap *XListInstalledColormaps(dpy, win, n)
register Display *dpy;
Window win;
int *n;  /* RETURN */
{
    long nbytes;
    Colormap *cmaps;
    xListInstalledColormapsReply rep;
    register xResourceReq *req;

    LockDisplay(dpy);
    GetResReq(ListInstalledColormaps, win, req);

    if(_XReply(dpy, (xReply *) &rep, 0, xFalse) == 0) {
	    UnlockDisplay(dpy);
	    SyncHandle();
	    *n = 0;
	    return((Colormap *)None);
	}
	

    *n = rep.nColormaps;
    cmaps = (Colormap *) Xmalloc(
	(unsigned) (nbytes = ((long)rep.nColormaps * sizeof(Colormap))));
    _XRead (dpy, (char *) cmaps, nbytes);

    UnlockDisplay(dpy);
    SyncHandle();
    return(cmaps);
}

