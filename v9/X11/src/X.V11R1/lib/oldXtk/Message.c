/* $Header: Message.c,v 1.1 87/09/11 07:58:19 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)Message.c	1.4	2/25/87";
#endif lint

/*
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 * 
 *                         All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of Digital Equipment
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.  
 * 
 * 
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */


/* Message.c -- send messages and fake events to a widget. */

#include "Xlib.h"
#include "Intrinsic.h"
#include "Atoms.h"

/* 
 * Send an event of the given type to the widget.
 */

XtEventReturnCode XtSendEvent(dpy, w, type)
    Display *dpy;
    Window w;
    unsigned long type;
{
    XEvent event;
    event.type = type;
    event.xany.display = dpy;
    event.xany.window = w;
    return XtDispatchEvent(&event);
}


/*
 * Tell a widget that its window has been destroyed (or will be very soon).
 */

XtEventReturnCode XtSendDestroyNotify(dpy, w)
    Display *dpy;
    Window w;
{
    return XtSendEvent(dpy, w, (unsigned long)DestroyNotify);
}


XtEventReturnCode XtSendConfigureNotify(dpy, w, box)
Display *dpy;
Window w;
WindowBox *box;
{
    XConfigureEvent event;
    event.type 	        = ConfigureNotify;
    event.display 	= dpy;
    event.event 	= event.window = w;
    event.x 		= box->x;
    event.y 		= box->y;
    event.width 	= box->width;
    event.height 	= box->height;
    event.border_width 	= box->borderWidth;
    return XtDispatchEvent((XEvent *) &event);
}

XtEventReturnCode XtSendExpose(dpy, w)
    Display *dpy;
    Window w;
{
    XEvent event;
    event.type = Expose;
    event.xexpose.display = dpy;
    event.xexpose.window = w;
    event.xexpose.count = 0;
    event.xexpose.x = 0;
    event.xexpose.y = 0;
    event.xexpose.width = 9999;
    event.xexpose.height = 9999;
    return XtDispatchEvent(&event);
}


#ifdef MESSAGES

/* 
 * Send a message to a widget.
 */

XtEventReturnCode XtSendMessage(dpy, w, message, args, argCount)
    Display	*dpy;
    Window	w;
    XtMessage	message;
    ArgList	args;
    int		argCount;
{
    XtEvent event;
    event.type = MessageEvent;
    event.display = dpy;
    event.window = w;
    event.message = message;
    event.args = args;
    event.argCount = argCount;
    event.subwindow = NULL;
    return XtDispatchEvent((XEvent *)&event);
}

/*
 * Get the dimensions of the given window.  If it's not yet a widget (or the
 * widget doesn't handle messages), ask the server.  Returns nonzero if
 * the window doesn't exist.
 */

XtStatus XtGetWindowSize(dpy, window, width, height, borderWidth)
    Display *dpy;
    Window window;
    int *width, *height, *borderWidth;
{
    static Arg args[] = {
	{XtNwidth, 	 (XtArgVal) -1},
	{XtNheight, 	 (XtArgVal) -1},
	{XtNborderWidth, (XtArgVal) -1},
    };
    Drawable root;
    Dimension x, y;
    unsigned int depth;

    args[0].value = args[1].value = args[2].value = (XtArgVal) -1;
    (void) XtSendMessage(dpy, window, messageGETVALUES, args, XtNumber(args));
    if ((int)args[0].value < 0 || (int)args[1].value < 0 ||
	    (int)args[2].value < 0) {
	if (!XGetGeometry(dpy, window, &root, &x, &y,
			  width, height, borderWidth, &depth))
	    return XtNOTWINDOW;
    } else {
	*width = (int)args[0].value;
	*height = (int)args[1].value;
	*borderWidth = (int)args[2].value;
    }
    return XtSUCCESS;
}

#endif

/*
 * Get the dimensions of the given window.  First ask its geometry manager;
 * if it doesn't have one or the geometry manager is uncooperative, ask
 * the server.  Returns nonzero if the window doesn't exist.
 */

XtGetWindowSize(dpy, window, width, height, borderWidth)
Display *dpy;
Window window;
Dimension *width, *height, *borderWidth;
{
    WindowBox box;
    Drawable root;
    Position x, y;
    unsigned int depth;

    
    if (XtMakeGeometryRequest(dpy, window, XtgeometryGetWindowBox, &box, &box)
	    == XtgeometryYes) {
	*width = box.width;
	*height = box.height;
	*borderWidth = box.borderWidth;
    } else {	
	if (!XGetGeometry(dpy, window, &root, &x, &y,
			  width, height, borderWidth, &depth))
	    return XtNOTWINDOW;
    }
    return XtSUCCESS;
}
