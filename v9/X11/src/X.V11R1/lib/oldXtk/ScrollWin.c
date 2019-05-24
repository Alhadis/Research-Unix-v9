/* $Header: ScrollWin.c,v 1.1 87/09/11 07:59:43 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)ScrolledWin.c	1.4	2/25/87";
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


/* File: ScrolledWin.c */

#include "Xlib.h"
#include "Intrinsic.h"
#include "Scroll.h"
#include "Atoms.h"

typedef struct _WidgetDataRec{
    Display *dpy;		/* Display connection for this widget */
    Window outer;		/* Encompassing window including scrollbars */
    Window frame;		/* Window containing the view of the data */
    Window inner;		/* Window with the data itself */
    Position x, y;		/* Location of outer within parent. */
    Dimension outerwidth, outerheight; /* Dimensions of encompassing window */
    WindowBox framebox;		/* Dimensions of framing (clipping) window */
    WindowBox innerbox;		/* Dimensions of the data */
    Dimension thickness;	/* Thickness of scroll bars */
    Boolean forcebars;		/* Whether we should always display */
				/* the selected bars. */
    Boolean allowhoriz;		/* Whether we allow horizontal scrollbars. */
    Boolean allowvert;		/* Whether we allow vertical scrollbars. */
    Boolean usebottom;		/* True iff horiz bars appear at bottom. */
    Boolean useright;		/* True iff vert bars appear at right. */
    Window horizinuse, vertinuse; /* What scrollbars we currently have */
} WidgetDataRec, *WidgetData;

static WidgetDataRec globaldata;
static WidgetDataRec globalinit = {
    NULL,		/* Display dpy; */
    NULL,		/* Encompassing window including scrollbars */
    NULL,		/* Window containing the view of the data */
    NULL,		/* Window with the data itself */
    0,0,		/* Location of outer within parent. */
    1,1,		/* Dimensions of encompassing window */
    {0,0,1,1,0},	/* framebox location, dimensions, border width */
    {0,0,1,1,0},	/* innerbox location, dimensions, border width */
    0,			/* Thickness of scroll bars */
    FALSE,		/* Whether we should always display */
			/* the selected bars. */
    FALSE,		/* Whether we allow horizontal scrollbars. */
    FALSE,		/* Whether we allow vertical scrollbars. */
    FALSE,		/* True iff horiz bars appear at bottom. */
    FALSE,		/* True iff vert bars appear at right. */
    (Window)NULL,(Window)NULL,	 /* What scrollbars we currently have */
};



extern int ScrollUpDownProc(), ThumbProc();

static Arg addBarArgs[] = {
    {XtNorientation, 	    (XtArgVal) NULL},
    {XtNlowerRight,	    (XtArgVal) NULL},
    {XtNvalue,		    (XtArgVal) NULL},
    {XtNscrollUpDownProc,   (XtArgVal) ScrollUpDownProc},
    {XtNthumbProc,	    (XtArgVal) ThumbProc},
};

static Resource resources[] = {
    {XtNinnerWidth, XtCWidth, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.innerbox.width, NULL},
    {XtNinnerHeight, XtCHeight, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.innerbox.height, NULL},
    {XtNinnerWindow, XtCWindow, XrmRWindow, sizeof(Window),
	 (caddr_t)&globaldata.inner, NULL},
    {XtNforceBars, XtCBoolean, XrmRBoolean, sizeof(Boolean),
	 (caddr_t)&globaldata.forcebars, NULL},
    {XtNallowHoriz, XtCBoolean, XrmRBoolean, sizeof(Boolean),
	 (caddr_t)&globaldata.allowhoriz, NULL},
    {XtNallowVert, XtCBoolean, XrmRBoolean, sizeof(Boolean),
	 (caddr_t)&globaldata.allowvert, NULL},
    {XtNuseBottom, XtCBoolean, XrmRBoolean, sizeof(Boolean),
	 (caddr_t)&globaldata.usebottom, NULL},
    {XtNuseRight, XtCBoolean, XrmRBoolean, sizeof(Boolean),
	 (caddr_t)&globaldata.useright, NULL},
};

static XContext scrolledWindowContext;

static Boolean initialized = FALSE;

static void ScrolledWindowInitialize ()
{
    if (initialized)
    	return;
    initialized = TRUE;

    scrolledWindowContext = XUniqueContext();
    globalinit.forcebars = globalinit.allowhoriz = globalinit.allowvert = FALSE;
    globalinit.usebottom = globalinit.useright = FALSE;
}

static WidgetData ScrolledWindowContextFromWindow(dpy, w)
  Display *dpy;
  Window w;
{
    WidgetData data;
    if (!XFindContext(dpy, w, scrolledWindowContext, (caddr_t *) &data)) 
	return data;
    return 0;
}
    
static SetBar(dpy, w, fx, wd, tw)
  Display *dpy;
  Window w;
  Position fx;
  Dimension wd, tw;
{
    float   from, width;
    from = (float) fx / tw;
    width = (float) wd / tw;
    XtScrollBarSetThumb(dpy, w, from, width);
}

static RedrawThumbs(data)
  WidgetData data;
{
    if (data->horizinuse)
	SetBar(data->dpy, data->horizinuse, -(data->innerbox.x), data->framebox.width,
		data->innerbox.width);
    if (data->vertinuse)
	SetBar(data->dpy, data->vertinuse, -(data->innerbox.y), data->framebox.height,
		data->innerbox.height);
}



static MoveInner(data, nx, ny)
  WidgetData data;
  int nx, ny;
{
    if (-nx + data->framebox.width > data->innerbox.width)
	nx = -(data->innerbox.width - data->framebox.width);
    if (-ny + data->framebox.height > data->innerbox.height)
	ny = -(data->innerbox.height - data->framebox.height);
    if (nx > 0) nx = 0;
    if (ny > 0) ny = 0;
    if (nx != data->innerbox.x || ny != data->innerbox.y) {
	XMoveWindow(data->dpy,data->inner, nx, ny);
	data->innerbox.x = nx;
	data->innerbox.y = ny;
	(void)XtSendConfigureNotify(data->dpy, data->inner, &(data->innerbox));
    }
    RedrawThumbs(data);
}

static ResizeEverything(data)
    WidgetData data;
{
    int   lw, lh;
    Boolean needshoriz, needsvert;
    int oldinnerwidth = data->innerbox.width;
    int oldinnerheight = data->innerbox.height;

    data->thickness = XtScrollMgrGetThickness(data->dpy, data->outer);
    if (data->forcebars) {
	needshoriz = data->allowhoriz;
	needsvert = data->allowvert;
	data->framebox.width =
	 data->outerwidth - (needsvert ? data->thickness : 0);
	data->framebox.height =
	 data->outerheight - (needshoriz ? data->thickness : 0);
	if (!needshoriz)
	    data->innerbox.width = data->framebox.width;
	if (!needsvert)
	    data->innerbox.height = data->framebox.height;
    }
    else {
	data->framebox.width = data->outerwidth;
	data->framebox.height = data->outerheight;
	do {
	    lw = data->framebox.width;
	    lh = data->framebox.height;
	    needshoriz = (Boolean)(data->innerbox.width > data->framebox.width);
	    needsvert =
	    	(Boolean)(data->innerbox.height > data->framebox.height);
	    if (!(data->allowhoriz)) {
		data->innerbox.width = data->framebox.width;
		needshoriz = FALSE;
	    }
	    if (!(data->allowvert)) {
		data->innerbox.height = data->framebox.height;
		needsvert = FALSE;
	    }
	    data->framebox.width = data->outerwidth -
		(needsvert ? data->thickness : 0);
	    data->framebox.height = data->outerheight -
		(needshoriz ? data->thickness : 0);
	} while (lw != data->framebox.width || lh != data->framebox.height);
    }
    if (oldinnerwidth != data->innerbox.width ||
	    oldinnerheight != data->innerbox.height) {
	XResizeWindow(data->dpy, data->inner,
		      data->innerbox.width, data->innerbox.height);
	(void)XtSendConfigureNotify(data->dpy, data->inner, &(data->innerbox));
    }
	
    if (needshoriz && data->horizinuse == 0) {
	addBarArgs[0].value = (XtArgVal)XtorientHorizontal;
	addBarArgs[1].value = (XtArgVal)(data->usebottom);
	addBarArgs[2].value = (XtArgVal)(data->outer);
	data->horizinuse =
	    XtScrollMgrAddBar(data->dpy, data->outer, addBarArgs, XtNumber(addBarArgs));
	XMapWindow(data->dpy,data->horizinuse);
    }
    else
	if (!needshoriz && data->horizinuse) {
	    XtDeleteScrollBar(data->dpy, data->outer, data->horizinuse);
	    data->horizinuse = 0;
	}
    if (needsvert && data->vertinuse == 0) {
	addBarArgs[0].value = (XtArgVal)XtorientVertical;
	addBarArgs[1].value = (XtArgVal)(data->useright);
	addBarArgs[2].value = (XtArgVal)(data->outer);
	data->vertinuse =
	    XtScrollMgrAddBar(data->dpy, data->outer, addBarArgs, XtNumber(addBarArgs));
	XMapWindow(data->dpy,data->vertinuse);
    }
    else
	if (!needsvert && data->vertinuse) {
	    XtDeleteScrollBar(data->dpy, data->outer, data->vertinuse);
	    data->vertinuse = 0;
	}
    data->framebox.width = data->outerwidth - (needsvert ? data->thickness : 0);
    data->framebox.height = data->outerheight - (needshoriz ? data->thickness : 0);
    RedrawThumbs(data);
}



/* Semi-public routines */


static ScrollUpDownProc(dpy, swin, outer, pix)
Display *dpy;
Window swin, outer;
int pix;
{
    WidgetData data;
    int     nx, ny;
    data = ScrolledWindowContextFromWindow(dpy, outer);
    if (!data) return;
    nx = data->innerbox.x - ((swin == data->horizinuse) ? pix : 0);
    ny = data->innerbox.y - ((swin == data->vertinuse) ? pix : 0);
    MoveInner(data, nx, ny);
}

static ThumbProc(dpy, swin, outer, percent)
  Display *dpy;
  Window swin, outer;
  float percent;
{
    WidgetData data;
    int     nx, ny;
    data = ScrolledWindowContextFromWindow(dpy, outer);
    if (!data) return;
    nx = data->innerbox.x;
    ny = data->innerbox.y;
    if (swin == data->horizinuse)
	nx = -(percent * data->innerbox.width);
    if (swin == data->vertinuse)
	ny = -(percent * data->innerbox.height);
    MoveInner(data, nx, ny);
}

static XtGeometryReturnCode
ScrolledWindowGeometryRequest(dpy, window, request, requestBox, replyBox)
Display *dpy;
Window window;
XtGeometryRequest request;
WindowBox *requestBox, *replyBox;
{
    WidgetData data;
    XtGeometryReturnCode    reply;
    WindowBox myrequest;
    data = ScrolledWindowContextFromWindow(dpy, window);
    if (!data || window != data->inner || request != XtgeometryResize)
	return XtgeometryNo;
    *replyBox = *requestBox;
    reply = XtgeometryYes;
    if ((!(data->allowhoriz) && requestBox->width != data->innerbox.width) ||
	    (!(data->allowvert) && requestBox->height != data->innerbox.height)) {
	myrequest = *requestBox;
	if (data->horizinuse) myrequest.height += data->thickness;
	if (data->vertinuse) myrequest.width += data->thickness;
	if (data->allowhoriz) myrequest.width = data->outerwidth;
	if (data->allowvert) myrequest.height = data->outerheight;
	reply = XtMakeGeometryRequest(data->dpy, data->outer, XtgeometryResize, &myrequest,
				      replyBox);
	if (reply != XtgeometryYes) {
	    replyBox->width = data->innerbox.width;
	    replyBox->height = data->innerbox.height;
	    reply = XtgeometryNo;
	    if (data->allowhoriz) replyBox->width = requestBox->width;
	    if (data->allowvert) replyBox->height = requestBox->height;
	    if (data->innerbox.width != replyBox->width ||
		    data->innerbox.height != replyBox->height) {
		reply = XtgeometryAlmost;
	    }
	}
    }
    if (reply == XtgeometryYes) {
	if (requestBox->width != data->innerbox.width ||
		requestBox->height != data->innerbox.height) {
	    XResizeWindow(dpy, window,
	    		  requestBox->width, requestBox->height);
	    data->innerbox.width = requestBox->width;
	    data->innerbox.height = requestBox->height;
	    (void) XtSendConfigureNotify(dpy, window, &(data->innerbox));
	    ResizeEverything(data);
	    MoveInner(data, data->innerbox.x, data->innerbox.y);
	    			/* Check inner location */
	}
    }
    return reply;
}


static XtEventReturnCode HandleEvents(event)
  XEvent *event;
{
    WidgetData data;
    data = ScrolledWindowContextFromWindow(event->xany.display, event->xany.window);
    if (!data) return XteventNotHandled;
    switch(event->type) {
	case ConfigureNotify:
	    if (data->outerwidth != event->xconfigure.width ||
		data->outerheight != event->xconfigure.height) {
		data->outerwidth = event->xconfigure.width;
		data->outerheight = event->xconfigure.height;
		ResizeEverything(data);
		MoveInner(data, data->innerbox.x, data->innerbox.y);
	    }
	    return XteventHandled;
	case DestroyNotify:
	    (void)XDeleteContext(data->dpy,data->outer,scrolledWindowContext);
	    (void)XDeleteContext(data->dpy,data->frame,scrolledWindowContext);
	    (void) XtSendDestroyNotify(data->dpy, data->inner);
	    /* Let the scrollbarmgr send messages to the scrollbars. */
	    XtFree((char*)data);
	    return XteventHandled;
    }
    return XteventNotHandled;
}



/* Public routines */

/* Create a scrolled window in the given one. */

Window XtScrolledWindowCreate(dpy, parent, args, argCount)
    Display *dpy;
    Window  parent;
    ArgList args;
    int	    argCount;
{
    WidgetData data;
    Dimension bw;
    unsigned int depth;
    Drawable root;
    XrmNameList names;
    XrmClassList classes;

    if (!initialized) ScrolledWindowInitialize();
    globaldata = globalinit;
    globaldata.dpy = dpy;
    XtGetResources(dpy, resources, XtNumber(resources), args, argCount, parent,
    	"scrolledWin", "ScrolledWin", &names, &classes);
    data = (WidgetData) XtMalloc(sizeof(WidgetDataRec));
    *data = globaldata;
    data->outer = XtScrollMgrCreate(dpy, parent, args, argCount);
    data->frame = XtScrollMgrGetChild(data->dpy, data->outer);

    XtSetNameAndClass(data->dpy, data->frame, names, classes);
    XrmFreeNameList(names);
    XrmFreeClassList(classes);

    if (data->inner != NULL) {
	XWindowChanges wc;
	unsigned int valuemask;
	valuemask = CWX | CWY | CWWidth | CWHeight | CWBorderWidth;
	wc.x = data->innerbox.x; wc.y = data ->innerbox.y; wc.width = data ->innerbox.width;
	wc.height=data->innerbox.height; wc.border_width = 1;
	XConfigureWindow(data->dpy,data->inner,valuemask,&wc);
	XReparentWindow(data->dpy,data->inner,parent,data->x,data->y);

    } else {
	data->inner = XtCreateWindow(data->dpy, data->frame, 0, 0,
				     data->innerbox.width,
				     data->innerbox.height,
				     0, (Pixel)0, (Pixel)0, NorthWestGravity);
    }
    (void) XSaveContext(data->dpy, data->outer, scrolledWindowContext, (caddr_t) data);
    (void) XSaveContext(data->dpy, data->inner, scrolledWindowContext, (caddr_t) data);
    (void) XtSetGeometryHandler(data->dpy, data->inner, ScrolledWindowGeometryRequest);
    data->innerbox.x = data->innerbox.y = 0;
    (void) XGetGeometry(data->dpy, data->outer, &root, &(data->x), &(data->y),
		       &(data->outerwidth), &(data->outerheight), &bw, &depth);
    data->horizinuse = data->vertinuse = (Window) NULL;
    ResizeEverything(data);
    XMapWindow(data->dpy,data->inner);
    XMapSubwindows(data->dpy,data->outer);
    XtSetEventHandler(data->dpy, data->outer, HandleEvents, StructureNotifyMask,
     (caddr_t) NULL);
    return data->outer;
}


Window XtScrolledWindowGetChild(dpy, outer)
Display *dpy;
Window outer;
{
    WidgetData data;
    data = ScrolledWindowContextFromWindow(dpy, outer);
    if (data) return data->inner;
    else return NULL;
}

void XtScrolledWindowSetChild(dpy, parent, child)
Display *dpy;
Window parent, child;
{
    WidgetData data;
    Dimension bw;
    unsigned int depth;
    Drawable root;
    data = ScrolledWindowContextFromWindow(dpy, parent);
    if (data) {
	XUnmapWindow(data->dpy,data->inner);
	(void) XDeleteContext(data->dpy, data->inner, scrolledWindowContext);
	data->inner = child;
	(void) XGetGeometry(data->dpy,data->inner,&root,&(data->innerbox.x),
	  &(data->innerbox.y), &(data->innerbox.width),
	  &(data->innerbox.height), &bw, &depth);
	(void) XSaveContext(data->dpy, data->inner, scrolledWindowContext, (caddr_t) data);
	(void) XtSetGeometryHandler(data->dpy, data->inner, ScrolledWindowGeometryRequest);
	ResizeEverything(data);
	XMapWindow(data->dpy, data->inner);
    }
}

Window XtScrolledWindowGetFrame(dpy, parent)
    Display *dpy;
    Window parent;
{
    WidgetData data;
    data = ScrolledWindowContextFromWindow(dpy, parent);
    if (data) return data->frame;
    else return NULL;
}

/* Unlink everything that was done to make this a scrolled window.  All the
   windows are actually destroyed, except for the parent one. */

Window XtUnmakeScrolledWindow(dpy, parent)
  Display *dpy;
  Window parent;
{
    WidgetData data;
    data = ScrolledWindowContextFromWindow(dpy, parent);
    if (!data) return;
    (void) XDeleteContext(data->dpy, data->inner, scrolledWindowContext);
    (void) XDeleteContext(data->dpy, parent, scrolledWindowContext);
    XDestroyWindow(data->dpy,data->inner);
    XtScrollMgrDestroy(data->dpy, parent);
    XtFree((caddr_t) data);
}
