/* $Header: Debug.c,v 1.1 87/09/11 07:57:29 toddb Exp $ */
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

#include <stdio.h>
#include "Xlib.h"
#include "Intrinsic.h"

char *DBEventTypeToString(type)
int type;
{
    static char *types[] = {
	"Event 0",
	"Event 1 (Toolkit message event)",
	"KeyPress",
	"KeyRelease",
	"ButtonPress",
	"ButtonRelease",
	"MotionNotify",
	"EnterNotify",
	"LeaveNotify",
	"FocusIn",
	"FocusOut",
	"KeymapNotify",
	"Expose",
	"GraphicsExpose",
	"NoExpose",
	"VisibilityNotify",
	"CreateNotify",
	"DestroyNotify",
	"UnmapNotify",
	"MapNotify",
	"MapRequest",
	"ReparentNotify",
	"ConfigureNotify",
	"ConfigureRequest",
	"GravityNotify",
	"ResizeRequest",
	"CirculateNotify",
	"CirculateRequest",
	"PropertyNotify",
	"SelectionClear",
	"SelectionRequest",
	"SelectionNotify",
	"ColormapNotify",
	"ClientMessage",
	"LASTEvent"
    };
    return types[type];
}

DBPrintEvent(event)
XEvent *event;
{
    (void) fprintf(stderr, "Event: %8x %s\t",
	    event->xany.window, DBEventTypeToString(event->type));
    switch (event->type) {
	case ConfigureNotify:
	    (void) fprintf(stderr, "(x, y, w, h) = (%d, %d, %d, %d)",
		    event->xconfigure.x, event->xconfigure.y,
		    event->xconfigure.width, event->xconfigure.height);
	    break;
	case Expose:
	    (void) fprintf(stderr, "(x, y, w, h) = (%d, %d, %d, %d)",
		    event->xexpose.x, event->xexpose.y,
		    event->xexpose.width, event->xexpose.height);
	    if (event->xexpose.count > 0)
		(void) fprintf(stderr, "(%d more)", event->xexpose.count);
	    break;
    }
    (void) fprintf(stderr, "\n");
}

DBSyncOn(dpy)
    Display	*dpy;
{
    (void) XSynchronize(dpy, TRUE);
}


DBSyncOff(dpy)
    Display	*dpy;
{
    (void) XSynchronize(dpy, FALSE);
}
