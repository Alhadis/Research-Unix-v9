/* $Header: ScrollMgr.c,v 1.1 87/09/11 07:59:35 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)ScrollBarMgr.c	1.4	2/25/87";
#endif lint
/*
 *			  COPYRIGHT 1987
 *		   DIGITAL EQUIPMENT CORPORATION
 *		       MAYNARD, MASSACHUSETTS
 *			ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR
 * ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT RIGHTS,
 * APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN ADDITION TO THAT
 * SET FORTH ABOVE.
 *
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting 
 * documentation, and that the name of Digital Equipment Corporation not be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 */


/* ScrollBarMgr.c */
/* created by weissman:	24 Jul 86 16:26 */

#include "Xlib.h"
#include "Intrinsic.h"
#include "Scroll.h"
#include "Atoms.h"

/* Private definitions. */

#define MAXBARS	4		/* How many scrollbars there can be, tops. */
#define DEFAULTTHICKNESS 15	/* Default thickness of scrollbars. */

typedef struct {
    Display *dpy;		/* the scrollbar window display */
    Window sbar;		/* The scrollbar window itself. */
    XtOrientation orientation;	/* Orientation of the scrollbar. */
    Boolean lowerRight;		/* Should this one go on the bottom or right */
    Position x, y;		/* Location of this scroll bar. */
    Dimension width, height;	/* Size of this scroll bar. */
} ScrollBarData, *ScrollBarDataPtr;

typedef struct {
    Display *dpy;		/* display connection for all this */
    Window outer;		/* All encompassing window. */
    Window frame;		/* Window that actually contains the data. */
    Position x, y;		/* Location of outer window. */
    Dimension pwidth, pheight;	/* Size of outer window. */
    Dimension cwidth, cheight;	/* Size of frame. */
    Position cx, cy;		/* Location of frame inside outer window. */
    Pixel border;		/* Pixmap for borders. */
    Dimension borderWidth;	/* Size of the borders. */
    int thickness;		/* Thickness of scroll bars. */
    int numbars;		/* How many scrollbars we have. */
    ScrollBarData bar[MAXBARS];	/* Data on each scrollbar. */
} WidgetDataRec, *WidgetData;

static WidgetDataRec globaldata;
static WidgetDataRec globalinit = {
    NULL,		/* display connection */
    NULL,		/* All encompassing window. */
    NULL,		/* Window that actually contains the data. */
    200,200,		/* Location of outer window. */
    100,100,		/* Size of outer window. */
    50,50,		/* Size of frame. */
    -99, -99,		/* Location of frame. */
    0,			/* Pixel for borders. */
    1,			/* Size of the borders. */
    DEFAULTTHICKNESS,	/* Thickness of scroll bars. */
    0,			/* How many scrollbars we have. */
    {(Display *) NULL, (Window)NULL, XtorientVertical, (Boolean)0, 0,0, 0,0}

};
static Resource resources[] = {
    {XtNwindow, XtCWindow, XrmRWindow, sizeof(Window),
	 (caddr_t)&globaldata.outer, NULL},
    {XtNchildWindow, XtCWindow, XrmRWindow, sizeof(Window),
	 (caddr_t)&globaldata.frame, NULL},
    {XtNborderWidth, XtCBorderWidth, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.borderWidth, NULL},
    {XtNborder, XtCColor, XrmRPixel, sizeof(int),
	 (caddr_t)&globaldata.border, (caddr_t)NULL},
    {XtNthickness, XtCThickness, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.thickness, NULL},
    {XtNwidth, XtCWidth, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.pwidth, NULL},
    {XtNheight, XtCHeight, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.pheight, NULL},
    {XtNx, XtCX, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.x, NULL},
    {XtNy, XtCY, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.y, NULL},
};


static Boolean lowerRight;
static XtOrientation orientation;

static Resource addResources[] = {
    {XtNlowerRight, XtCBoolean, XrmRBoolean, sizeof(Boolean),
	 (caddr_t)&lowerRight, NULL},
    {XtNorientation, XtCOrientation, XtROrientation, sizeof(XtOrientation),
	 (caddr_t)&orientation, NULL},
};

static XContext scrollBarMgrContext;

static Boolean initialized = FALSE;

static void ScrollBarMgrInitialize ()
{
    if (initialized)
    	return;
    initialized = TRUE;

    scrollBarMgrContext = XUniqueContext ();
}


static WidgetData GetSBMgrInfoFromWindow(dpy, w)
Display *dpy;
Window w;
{
    WidgetData data;
    if (!XFindContext(dpy, w, scrollBarMgrContext, (caddr_t *) &data))
	return data;
    return 0;
}



static ResizeEverything(data)
WidgetData data;
{
    int     top = 0,
            bottom = 0,
            left = 0,
            right = 0;
    Dimension thickness = data->thickness;
    Position borderWidth = data->borderWidth;
    Dimension pwidth = data->pwidth;
    Dimension pheight = data->pheight;
    Position x, y;
    Dimension cwidth, cheight, width, height;
    ScrollBarDataPtr bar;
    for (bar = data->bar;
	    bar - data->bar < data->numbars;
	    bar++) {
	if (bar->orientation == XtorientVertical) {
	    if (bar->lowerRight)
		right = thickness;
	    else
		left = thickness;
	}
	else {
	    if (bar->lowerRight)
		bottom = thickness;
	    else
		top = thickness;
	}
    }
    cwidth = pwidth - left - right;
    cheight = pheight - top - bottom;
    if (cwidth < 1 || cheight < 1)
	return;			/* Everything is too small still. */
    if (left != data->cx || top != data->cy ||
	   cwidth != data->cwidth || cheight != data->cheight) {
	WindowBox box;
	XMoveResizeWindow(data->dpy,data->frame,
			 box.x = data->cx = left,
			 box.y = data->cy = top,
			 box.width = data->cwidth = cwidth,
			 box.height = data->cheight = cheight);
	(void) XtSendConfigureNotify(data->dpy, data->frame, &box);
    }
    for (bar = data->bar;
	    bar - data->bar < data->numbars;
	    bar++) {
	if (bar->orientation == XtorientVertical) {
	    x = bar->lowerRight ? pwidth - thickness : -borderWidth;
	    y = top - borderWidth;
	    width = thickness - borderWidth;
	    height = pheight - top - bottom;
	}
	else {
	    x = left - borderWidth;
	    y = bar->lowerRight ? pheight - thickness : -borderWidth;
	    width = pwidth - left - right;
	    height = thickness - borderWidth;
	}
	if (width != bar->width || height != bar->height ||
		x != bar->x || y != bar->y) {
	    WindowBox box;
	    XMoveResizeWindow(data->dpy, bar->sbar,
		    box.x = bar->x = x,
		    box.y = bar->y = y,
		    box.width = bar->width = width,
		    box.height = bar->height = height);
	    (void) XtSendConfigureNotify(data->dpy, bar->sbar, &box);
	}
    }
}




static XtEventReturnCode HandleEvents(event)
    XEvent *event;
{
    WidgetData data;
    int i;

    data = GetSBMgrInfoFromWindow(event->xany.display, event->xany.window);
    if (!data) return XteventNotHandled;

    switch(event->type){
    	case ConfigureNotify:
	    if (data->pwidth != event->xconfigure.width
	     || data->pheight != event->xconfigure.height) {
	    	data->pwidth = event->xconfigure.width;
	    	data->pheight = event->xconfigure.height;
	    	ResizeEverything(data);
	    }
      	    return XteventHandled;

    	case DestroyNotify:
	    XtClearEventHandlers(data->dpy, data->outer);
	    for (i=0; i<MAXBARS; i++) {
		if (data->bar[i].sbar != NULL) 
		    (void) XtSendDestroyNotify(data->dpy,data->bar[i].sbar);
	    }
	    (void) XDeleteContext(data->dpy, data->outer,scrollBarMgrContext);
	    (void) XDeleteContext(data->dpy, data->frame,scrollBarMgrContext);
	    (void) XtSendDestroyNotify(data->dpy, data->frame);
	    XtClearEventHandlers(data->dpy, data->frame);
	    XtFree((char *)data);
	    return XteventHandled;
    }

    return XteventNotHandled;
}

static XtGeometryReturnCode 
ScrollBarGeometryRequest(dpy, myWindow, request, requestBox, replyBox)
  Display *dpy;
  Window myWindow;
  XtGeometryRequest request;
  WindowBox *requestBox, *replyBox;
{
    WidgetData data;
    WindowBox myRequest, myReply;
    XtGeometryReturnCode result;
    data = GetSBMgrInfoFromWindow(dpy, myWindow);
    if (!data || request != XtgeometryResize)
	return XtgeometryNo;
    myRequest.x = myRequest.y = 0; 
    myRequest.width = requestBox->width + data->pwidth - data->cwidth; 
    myRequest.height = requestBox->height + data->pheight - data->cheight;
    result = XtMakeGeometryRequest(dpy, data->outer, request, &myRequest, &myReply);
    if (result != XtgeometryNo) {
	replyBox->width = myReply.width + data->cwidth - data->pwidth;
	replyBox->width = myReply.height + data->cheight - data->pheight;
    }
    return result;
}



/* Public routines. */

/* Register this window as one that might require scrollbars within it.
   Returns the window created within that will actually be used to store
   the data. */

/*
 * Creates a new master window which will in turn contain a window that
 * will be layed out some number of scrollbars.  Returns the master window.
 */

Window XtScrollMgrCreate(dpy, parent, args, argCount)
    Display  *dpy;
    Window   parent;
    ArgList  args;
    int      argCount;
{
    WidgetData	data;
    XrmNameList	names;
    XrmClassList classes;

    if (!initialized) ScrollBarMgrInitialize ();

    data = (WidgetData) XtMalloc(sizeof(WidgetDataRec));
    globaldata = globalinit;
    globaldata.dpy = dpy;
    XtGetResources(dpy, resources, XtNumber(resources), args, argCount, parent,
	"scrollBarMgr", "ScrollBarMgr",	&names, &classes);
    *data = globaldata;
    if (data->outer != NULL) {
	XWindowChanges wc;
	unsigned int valuemask;
	valuemask = CWX | CWY | CWWidth | CWHeight | CWBorderWidth;
	wc.x = data->x; wc.y = data->y; wc.width = data->pwidth;
	wc.height = data->pheight; wc.border_width = data->borderWidth;
	XConfigureWindow(data->dpy,data->outer,valuemask, &wc);
	XReparentWindow(data->dpy,data->outer,parent,data->x,data->y);

    } else {
	data->outer = XtCreateWindow(data->dpy, parent, data->x, data->y,
				     data->pwidth, data->pheight,
				     data->borderWidth,
				     data->border, (Pixel)NULL,
				     NorthWestGravity);
    }
    XtSetNameAndClass(data->dpy, data->outer, names, classes);
    XrmFreeNameList(names);
    XrmFreeClassList(classes);

    if (data->frame == NULL)
	data->frame = XtCreateWindow(data->dpy, data->outer, 0, 0,
				     data->pwidth, data->pheight, 0,
				     (Pixel) 0, (Pixel) 1,
				     NorthWestGravity);
    XMapWindow(data->dpy, data->frame);
    (void) XSaveContext(data->dpy, data->outer, scrollBarMgrContext, (caddr_t) data);
    (void) XSaveContext(data->dpy, data->frame, scrollBarMgrContext, (caddr_t) data);
    data->numbars = 0;
    (void) XtSetGeometryHandler(data->dpy, data->frame, (XtGeometryHandler) ScrollBarGeometryRequest);
    (void) XtSetEventHandler(data->dpy, data->outer, (XtEventHandler) HandleEvents,
     StructureNotifyMask, (caddr_t) NULL);
    return data->outer;
}


Window XtScrollMgrGetChild(dpy, parent)
    Display *dpy;
    Window parent;
{
    WidgetData data;
    data = GetSBMgrInfoFromWindow(dpy, parent);
    if (data) return data->frame;
    else return NULL;
}


Window XtScrollMgrSetChild(dpy, parent, frame)
    Display *dpy;
    Window parent, frame;
{
    WidgetData data;
    data = GetSBMgrInfoFromWindow(dpy, parent);
    if (data) {
	XUnmapWindow(data->dpy,data->frame);
	(void) XDeleteContext(data->dpy, data->frame, scrollBarMgrContext);
	data->frame = frame;
	(void) XSaveContext(data->dpy, data->frame, scrollBarMgrContext, (caddr_t) data);
	(void) XtSetGeometryHandler(data->dpy, data->frame, ScrollBarGeometryRequest);
	ResizeEverything(data);
	XMapWindow(data->dpy,data->frame);
    }
}


/* Stop this window from being managed as scrollbars.  All windows are
   destroyed, except for the outer one. */

int XtScrollMgrDestroy(dpy, w)
    Display *dpy;
    Window w;
{
    WidgetData data;
    int i;
    data = GetSBMgrInfoFromWindow(dpy, w);
    if (!data) return;
    for (i=data->numbars - 1 ; i>=0 ; i--)
	XtDeleteScrollBar(dpy, w, data->bar[i].sbar);
    (void) XDeleteContext(dpy, w, scrollBarMgrContext);
    XDestroyWindow(data->dpy,data->frame);
    XtFree((char *) data);
}



/* Set the thickness to be used to draw scroll bars in the given window.  Any
   already existing scrollbars will be resized to match this. */

XtScrollMgrSetThickness(dpy, w, thickness)
Display *dpy;
Window w;
int thickness;
{
    WidgetData data;
    data = GetSBMgrInfoFromWindow(dpy,w);
    if (data->thickness != thickness) {
	data->thickness = thickness;
	ResizeEverything(data);
    }
}


/* Find out the thickness of the scrollbars in the given window. */

int XtScrollMgrGetThickness(dpy, w)
Display *dpy;
Window w;
{
    WidgetData data;
    data = GetSBMgrInfoFromWindow(dpy, w);
    return data->thickness;
}


/* Add a scroll bar. */

Window XtScrollMgrAddBar(dpy, parent, args, argCount)
    Display  *dpy;
    Window   parent;
    ArgList  args;
    int      argCount;
{
    WidgetData data;
    int i;
    XrmNameList names;
    XrmClassList classes;

    data = GetSBMgrInfoFromWindow(dpy, parent);
    if (data->numbars < MAXBARS) {
	orientation = XtorientVertical;
	lowerRight = FALSE;
	XtGetResources(data->dpy, addResources, XtNumber(addResources), args, argCount,
	    parent, "scrollBarMgr", "ScrollBarMgr", &names, &classes);
	XrmFreeNameList(names);
	XrmFreeClassList(classes);

	i = (data->numbars)++;
	data->bar[i].sbar = XtScrollBarCreate(data->dpy, data->outer, args, argCount);
	data->bar[i].orientation = orientation;
	data->bar[i].lowerRight = lowerRight;
	data->bar[i].width = 1;
	data->bar[i].height = 1;
	ResizeEverything(data);
	XMapWindow(data->dpy,data->bar[i].sbar);
	return data->bar[i].sbar;
    }
	else return 0;
}



/* Delete a scroll bar. */

XtDeleteScrollBar(dpy, parent, scrollbar)
Display *dpy;
Window parent, scrollbar;
{
    WidgetData data;
    int     i;
    data = GetSBMgrInfoFromWindow(dpy, parent);
    for (i = 0; i < data->numbars; i++) {
	if (data->bar[i].sbar == scrollbar) {
	    data->bar[i] = data->bar[--(data->numbars)];
	    XDestroyWindow(dpy, scrollbar);
	    XtSendDestroyNotify(dpy, scrollbar);
	    ResizeEverything(data);
	    break;
	}
    }
}
