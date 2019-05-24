#include "copyright.h"

/* $Header: XUnldFont.c,v 11.5 87/09/11 08:08:14 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XUnloadFont(dpy, font)
     register Display *dpy;
     Font font;

{       
    register xResourceReq *req;

    LockDisplay(dpy);
    GetResReq(CloseFont, font, req);
    UnlockDisplay(dpy);
    SyncHandle();
}

