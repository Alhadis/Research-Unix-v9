#include "copyright.h"

/* $Header: XRecolorC.c,v 11.6 87/09/11 08:06:26 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XRecolorCursor(dpy, cursor, foreground, background)
     register Display *dpy;
     Cursor cursor;
     XColor *foreground, *background;
{       
    register xRecolorCursorReq *req;

    LockDisplay(dpy);
    GetReq(RecolorCursor, req);
    req->cursor = cursor;
    req->foreRed = foreground->red;
    req->foreGreen = foreground->green;
    req->foreBlue = foreground->blue;
    req->backRed = background->red;
    req->backGreen = background->green;
    req->backBlue = background->blue;
    UnlockDisplay(dpy);
    SyncHandle();
}

