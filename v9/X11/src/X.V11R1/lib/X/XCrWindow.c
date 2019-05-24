#include "copyright.h"

/* $Header: XCrWindow.c,v 11.9 87/09/11 08:02:37 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

Window XCreateSimpleWindow(dpy, parent, x, y, width, height, 
                      borderWidth, border, background)
    register Display *dpy;
    Window parent;
    int x, y;
    unsigned int width, height, borderWidth;
    unsigned long border;
    unsigned long background;
{
    Window wid;
    register unsigned long *valuePtr;
    register xCreateWindowReq *req;

    LockDisplay(dpy);
    GetReqExtra(CreateWindow, 8, req);
    req->parent = parent;
    req->x = x;
    req->y = y;
    req->width = width;
    req->height = height;
    req->borderWidth = borderWidth;
    req->depth = 0;
    req->class = CopyFromParent;
    req->visual = CopyFromParent;
    wid = req->wid = XAllocID(dpy);
    req->mask = CWBackPixel | CWBorderPixel;

    valuePtr = (unsigned long *) (req + 1);
    *valuePtr++ = background;
    *valuePtr = border;
    UnlockDisplay(dpy);
    SyncHandle();
    return (wid);
    }
