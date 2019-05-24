#include "copyright.h"

/* $Header: XLoadFont.c,v 11.6 87/09/11 08:04:52 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

Font XLoadFont (dpy, name)
    register Display *dpy;
    char *name;
{
    register long nbytes;
    Font fid;
    register xOpenFontReq *req;
    LockDisplay(dpy);
    GetReq(OpenFont, req);
    nbytes = req->nbytes = strlen(name);
    req->fid = fid = XAllocID(dpy);
    req->length += (nbytes+3)>>2;
    Data (dpy, name, nbytes);
    UnlockDisplay(dpy);
    SyncHandle();
    return (fid); 
       /* can't return (req->fid) since request may have already been sent */
}

