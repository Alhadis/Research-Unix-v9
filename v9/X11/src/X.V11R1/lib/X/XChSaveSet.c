#include "copyright.h"

/* $Header: XChSaveSet.c,v 11.4 87/09/11 08:01:43 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XChangeSaveSet(dpy, win, mode)
register Display *dpy;
Window win;
int mode;
{
    register xChangeSaveSetReq *req;

    LockDisplay(dpy);
    GetReq(ChangeSaveSet, req);
    req->window = win;
    req->mode = mode;
    UnlockDisplay(dpy);
    SyncHandle();
}

XAddToSaveSet(dpy, win)
    register Display *dpy;
    Window win;
{
    XChangeSaveSet(dpy,win,SetModeInsert);
}

XRemoveFromSaveSet (dpy, win)
    register Display *dpy;
    Window win;
{
    XChangeSaveSet(dpy,win,SetModeDelete);
}
