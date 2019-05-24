#include "copyright.h"

/* $Header: XWindow.c,v 11.11 87/09/11 08:08:31 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

Window XCreateWindow(dpy, parent, x, y, width, height, 
                borderWidth, depth, class, visual, valuemask, attributes)
    register Display *dpy;
    Window parent;
    int x, y;
    unsigned int width, height, borderWidth;
    int depth;
    unsigned int class;
    Visual *visual;
    unsigned long valuemask;
    XSetWindowAttributes *attributes;
{
    Window wid;
    register xCreateWindowReq *req;

    LockDisplay(dpy);
    GetReq(CreateWindow, req);
    req->parent = parent;
    req->x = x;
    req->y = y;
    req->width = width;
    req->height = height;
    req->borderWidth = borderWidth;
    req->depth = depth;
    req->class = class;
    if (visual == CopyFromParent)
	req->visual = CopyFromParent;
    else
	req->visual = visual->visualid;
    wid = req->wid = XAllocID(dpy);
    if (req->mask = valuemask) 
        _XProcessWindowAttributes (dpy, (xChangeWindowAttributesReq *)req, 
			valuemask, attributes);
    UnlockDisplay(dpy);
    SyncHandle();
    return (wid);
    }

_XProcessWindowAttributes (dpy, req, valuemask, attributes)
    register Display *dpy;
    xChangeWindowAttributesReq *req;
    register unsigned long valuemask;
    register XSetWindowAttributes *attributes;
    {

    /* Warning!  This code assumes that "unsigned long" is 32-bits wide */

    unsigned long values[32];
    register unsigned long *value = values;
    unsigned int nvalues;

    if (valuemask & CWBackPixmap)
	*value++ = attributes->background_pixmap;
	
    if (valuemask & CWBackPixel)
    	*value++ = attributes->background_pixel;

    if (valuemask & CWBorderPixmap)
    	*value++ = attributes->border_pixmap;

    if (valuemask & CWBorderPixel)
    	*value++ = attributes->border_pixel;

    if (valuemask & CWBitGravity)
    	*value++ = attributes->bit_gravity;

    if (valuemask & CWWinGravity)
	*value++ = attributes->win_gravity;

    if (valuemask & CWBackingStore)
        *value++ = attributes->backing_store;
    
    if (valuemask & CWBackingPlanes)
	*value++ = attributes->backing_planes;

    if (valuemask & CWBackingPixel)
    	*value++ = attributes->backing_pixel;

    if (valuemask & CWOverrideRedirect)
    	*value++ = attributes->override_redirect;

    if (valuemask & CWSaveUnder)
    	*value++ = attributes->save_under;

    if (valuemask & CWEventMask)
	*value++ = attributes->event_mask;

    if (valuemask & CWDontPropagate)
	*value++ = attributes->do_not_propagate_mask;

    if (valuemask & CWColormap)
	*value++ = attributes->colormap;

    if (valuemask & CWCursor)
	*value++ = attributes->cursor;

    req->length += (nvalues = value - values);

    /* note: Data is a macro that uses its arguments multiple
       times, so "nvalues" is changed in a separate assignment
       statement */

    nvalues <<= 2;
    Data (dpy, (char *) values, (long)nvalues);

    }
