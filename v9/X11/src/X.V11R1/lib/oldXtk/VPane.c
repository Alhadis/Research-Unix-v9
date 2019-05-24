/* $Header: VPane.c,v 1.1 87/09/11 07:59:07 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)VPane.c	1.10	2/25/87";
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


/* VPane.c */

#include "Xlib.h"
#include "cursorfont.h"
#include "Intrinsic.h"
#include "VPane.h"
#include "Atoms.h"

/* Private definitions. */


#define BORDERWIDTH		1 /* Size of borders between panes. */
				  /* %%% Should not be a constant! */
#define DEFAULTKNOBWIDTH	9
#define DEFAULTKNOBHEIGHT	9
#define DEFAULTKNOBINDENT	16

typedef struct {
    Display *dpy;		/* Display connection */
    Window w;			/* Subwindow */
    Position y;			/* Location in master window */
    Position dy;		/* Desired location. */
    Position olddy;		/* The last value of dy.  */
    Dimension min, max;		/* Minimum and maximum height. */
    short autochange;		/* Whether we're allowed to change this */
				/* subwindow's height without an */
				/* explicite command from user. */
    Dimension height;			/* Current height. */
    Dimension dheight;		/* Desired height. */
    Window knob;		/* The knob for this subwindow. */
    Boolean allowresize;	/* Whether we should honor geometry requests */
				/* from this window to change its height. */
} SubWindowInfo, *SubWindowPtr;

typedef struct {
    Display *dpy;		/* display connection for paned window */
    Window ourparent;		/* Parent of the paned window. */
    Window window;		/* Window containing everything. */
    Pixel foregroundpixel;	/* Foreground pixmap. */
    Pixel backgroundpixel;	/* Background pixmap. */
    Position x, y;		/* Location of master window. */
    Dimension width, height;	/* Dimension of master window. */
    Dimension borderWidth;	/* Borderwidth of master window. */
    Dimension borderpixel;	/* Color of border of master window. */
    Dimension knobwidth, knobheight; /* Dimension of knobs. */
    Position knobindent;	/* Location of knobs (offset from */
				/* right margin.) */
    Pixel knobpixel;		/* Color of knobs. */
    Dimension heightused;	/* Total height used by subwindows. */
    int numsubwindows;		/* How many windows within it. */
    SubWindowInfo *sub;		/* Array of info about the sub windows. */
    int whichtracking;		/* Which knob we are tracking, if any */
    Position starty;		/* Starting y value. */
    int whichdirection;		/* Which direction to refigure things in. */
    SubWindowPtr whichadd;	/* Which subwindow to add changes to. */
    SubWindowPtr whichsub;	/* Which subwindow to sub changes from. */
    Boolean refiguremode;	/* Whether to refigure things right now. */
    GC normgc;			/* GC to use when redrawing borders. */
    GC invgc;			/* GC to use when erasing borders. */
    GC flipgc;			/* GC to use when animating borders. */
} WidgetDataRec, *WidgetData;

static WidgetDataRec globaldata, globaldatainit;

static Resource resources[] = {
    {XtNwindow, XtCWindow, XrmRWindow, sizeof(Window),
	 (caddr_t)&globaldata.window, NULL},
    {XtNforeground, XtCColor, XrmRPixel, sizeof(int),
	 (caddr_t)&globaldata.foregroundpixel, (caddr_t)&XtDefaultFGPixel},
    {XtNbackground, XtCColor, XrmRPixel, sizeof(int),
	 (caddr_t)&globaldata.backgroundpixel, (caddr_t)&XtDefaultBGPixel},
    {XtNx, XtCX, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.x, NULL},
    {XtNy, XtCY, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.y, NULL},
    {XtNwidth, XtCWidth, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.width, NULL},
    {XtNheight, XtCHeight, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.height, NULL},
    {XtNborderWidth, XtCBorderWidth, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.borderWidth, NULL},
    {XtNborder, XtCColor, XrmRPixmap, sizeof(int),
	 (caddr_t)&globaldata.borderpixel, (caddr_t)&XtDefaultFGPixel},
    {XtNknobWidth, XtCWidth, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.knobwidth, NULL},
    {XtNknobHeight, XtCHeight, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.knobheight, NULL},
    {XtNknobIndent, XtCKnobIndent, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.knobindent, NULL},
    {XtNknobPixel, XtCKnobPixel, XrmRPixel, sizeof(int),
	 (caddr_t)&globaldata.knobpixel, (caddr_t)&globaldata.foregroundpixel},
};

static XContext widgetContext;
static int CursorNums[4] = {0,
			    XC_sb_up_arrow,
			    XC_sb_v_double_arrow,
			    XC_sb_down_arrow};

static WidgetData DataFromWindow(dpy, w)
  Display *dpy;
  Window w;
{
    WidgetData data;
    if (XFindContext(dpy, w, widgetContext, (caddr_t *) &data)) return 0;
    return data;
}

static void TryResize(data, newwidth, newheight)
  WidgetData data;
  Dimension newwidth, newheight;
{
    WindowBox requestBox, replyBox;
    XtGeometryReturnCode result;
    requestBox.x = 0;
    requestBox.y = 0;
    if (newwidth < 1) newwidth = 1;
    if (newheight < 1) newheight = 1;
    requestBox.width = newwidth;
    requestBox.height = newheight;
    result = XtMakeGeometryRequest(data->dpy, data->window, XtgeometryResize,
				   &requestBox, &replyBox);
    if (result == XtgeometryAlmost) {
	requestBox = replyBox;
	result = XtMakeGeometryRequest(data->dpy, data->window, XtgeometryResize,
				       &requestBox, &replyBox);
    }
    if (result == XtgeometryYes) {
	data->width = replyBox.width;
	data->height = replyBox.height;
    }
}


static RefigureLocations(data, position, dir)
  WidgetData data;
  int position;
  int dir;		/* -1 = up, 1 = down, 0 = this border only */
{
    SubWindowPtr sub, firstsub;
    int     i, old, y, cdir;
    if (data->numsubwindows == 0 || !data->refiguremode)
	return;
    do {
	data->heightused = 0;
	for (i = 0, sub = data->sub; i < data->numsubwindows; i++, sub++) {
	    if (sub->dheight < sub->min)
		sub->dheight = sub->min;
	    if (sub->dheight > sub->max)
		sub->dheight = sub->max;
	    data->heightused += sub->dheight;
	}
	data->heightused += BORDERWIDTH * (data->numsubwindows - 1);
	if (dir == 0 && data->heightused != data->height) {
	    for (i = 0, sub = data->sub; i < data->numsubwindows; i++, sub++)
		if (sub->dheight != sub->height)
		    sub->dheight += data->height - data->heightused;
	}
    } while (dir == 0 && data->heightused != data->height);

    firstsub = data->sub + position;
    sub = firstsub;
    cdir = dir;
    while (data->heightused != data->height) {
	if (sub->autochange || cdir != dir) {
	    old = sub->dheight;
	    sub->dheight = data->height - data->heightused + old;
	    if (sub->dheight < sub->min)
		sub->dheight = sub->min;
	    if (sub->dheight > sub->max)
		sub->dheight = sub->max;
	    data->heightused += (sub->dheight - old);
	}
	sub += cdir;
	while (sub < data->sub || sub - data->sub == data->numsubwindows) {
	    cdir = -cdir;
	    if (cdir == dir) goto doublebreak;
	    sub = firstsub + cdir;
	}
    }
doublebreak:
    y = -BORDERWIDTH;
    for (i = 0, sub = data->sub; i < data->numsubwindows; i++, sub++) {
	sub->dy = y;
	y += sub->dheight + BORDERWIDTH;
    }
}


static CommitNewLocations(data)
  WidgetData data;
{
    int i, kx, ky;
    SubWindowPtr sub;
    if (data->heightused != data->height)
	TryResize(data, data->width, data->height);
    for (i = 0, sub = data->sub; i < data->numsubwindows; i++, sub++) {
	if (sub->dy != sub->y || sub->dheight != sub->height) {
	    WindowBox box;
	    XMoveResizeWindow(data->dpy, sub->w,
			     box.x = -BORDERWIDTH,
			     box.y = sub->dy,
			     box.width = data->width,
			     box.height = sub->dheight);
	    sub->y = sub->dy;
	    sub->height = sub->dheight;
	    (void) XtSendConfigureNotify(data->dpy, sub->w, &box);
	    kx = data->width - data->knobindent;
	    ky = sub->y + sub->height - (data->knobheight/2)+1;
	    if (i == data->numsubwindows - 1)
		ky = -99;	/* Keep the last knob hidden. */
	    XMoveWindow(data->dpy, sub->knob, kx, ky);
	    XRaiseWindow(data->dpy,sub->knob);

	}
    }
/*    kx = data->width - data->knobindent;
    for (i = 0, sub = data->sub; i < data->numsubwindows; i++, sub++) {
	ky = sub->y + sub->height - (data->knobheight / 2) + 1;
	if (i == data->numsubwindows - 1)
	    ky = -99;
	XMoveWindow(data->dpy, sub->knob, kx, ky);
	XRaiseWindow(data->dpy, sub->knob);
    }*/
}


static RefigureLocationsAndCommit(data, position, dir)
  WidgetData data;
  int position, dir;
{
    RefigureLocations(data, position, dir);
    CommitNewLocations(data);
}


EraseInternalBorders(data)
  WidgetData data;
{
    int     i;
    for (i = 1; i < data->numsubwindows; i++)
	XFillRectangle(data->dpy, data->window, data->invgc,
		       0, data->sub[i].y, data->width, BORDERWIDTH);
}


DrawInternalBorders(data)
  WidgetData data;
{
    int     i;
    for (i = 1; i < data->numsubwindows; i++)
	XFillRectangle(data->dpy, data->window, data->normgc,
		       0, data->sub[i].y, data->width, BORDERWIDTH);
}


static DrawTrackLines(data)
  WidgetData data;
{
    int     i;
    SubWindowPtr sub;
    for (i = 1, sub = data->sub + 1; i < data->numsubwindows; i++, sub++) {
	if (sub->olddy != sub->dy) {
	    XDrawLine(data->dpy, data->window, data->flipgc,
		  0, sub->olddy, (Position) data->width, sub->olddy);
	    XDrawLine(data->dpy, data->window, data->flipgc,
		  0, sub->dy, (Position) data->width, sub->dy);
	    sub->olddy = sub->dy;
	}
    }
}

static EraseTrackLines(data)
  WidgetData data;
{
    int     i;
    SubWindowPtr sub;
    for (i = 1, sub = data->sub + 1; i < data->numsubwindows; i++, sub++)
	XDrawLine(data->dpy, data->window, data->flipgc,
	      0, sub->olddy, (Position) data->width, sub->olddy);
}


/* Semi-public routines. */

static XtGeometryReturnCode PanedWindowGeometryRequest(
				      dpy, w, request, reqBox, replBox)
Display *dpy;
Window w;
XtGeometryRequest request;
WindowBox *reqBox, *replBox;
{
    WidgetData data;
    int i;
    SubWindowPtr sub;

    data = DataFromWindow(dpy, w);
    if (!data) return XtgeometryNo;
    for (i=0 ; i<data->numsubwindows ; i++)
	if (data->sub[i].w == w) break;
    if (i >= data->numsubwindows) return XtgeometryNo;
    sub = data->sub + i;
    if (request == XtgeometryResize) {
	if (!sub->allowresize) return XtgeometryNo;
	if (data->width != reqBox->width) {
	    if (sub->height == reqBox->height)
		return XtgeometryNo;
	    *replBox = *reqBox;
	    replBox->width = data->width;
	    return XtgeometryAlmost;
	}
	if (sub->min == sub->height || sub->min > reqBox->height)
	    sub->min = reqBox->height;
	if (sub->max == sub->height || sub->max < reqBox->height)
	    sub->max = reqBox->height;
	sub->dheight = reqBox->height;
	RefigureLocationsAndCommit(data, i, 1);
	return XtgeometryYes;
    }
    if (request == XtgeometryGetWindowBox) {
	replBox->x = -BORDERWIDTH;
	replBox->y = sub->y;
	replBox->width = data->width;
	replBox->height = sub->height;
	replBox->borderWidth = BORDERWIDTH;
	return XtgeometryYes;
    }
    return XtgeometryNo;
}

static XtEventReturnCode HandleEvents(event)
  XEvent *event;
{
    WidgetData data;
    WindowBox box;
    int i;

    data = DataFromWindow(event->xany.display, event->xany.window);
    if (!data) return XteventNotHandled;
    switch (event->type) {
      case ConfigureNotify:
	if (data->width != event->xconfigure.width ||
	      data->height != event->xconfigure.height) {
	    if (data->width != event->xconfigure.width) {
		box.x = -BORDERWIDTH;
		box.width = event->xconfigure.width;
		box.borderWidth = BORDERWIDTH;
		for (i = 0; i < data->numsubwindows; i++) {
		    XResizeWindow(data->dpy, data->sub[i].w,
				  (Dimension) event->xconfigure.width,
				  (Dimension) data->sub[i].height);
		    box.y = data->sub[i].y;
		    box.height = data->sub[i].height;
		    (void) XtSendConfigureNotify(data->dpy, data->sub[i].w,
						 &box);
		}
	    }
	    data->width = event->xconfigure.width;
	    data->height = event->xconfigure.height;
	    RefigureLocationsAndCommit(data, data->numsubwindows - 1, -1);
	}
	data->x = event->xconfigure.x;
	data->y = event->xconfigure.y;
	data->borderWidth = event->xconfigure.border_width;
	return XteventHandled;
      case DestroyNotify:
	data->refiguremode = FALSE;
	for (i = data->numsubwindows-1 ; i >= 0 ; i--)
	    (void) XtSendDestroyNotify(data->dpy, data->sub[i].w);
	(void) XDeleteContext(data->dpy, data->window, widgetContext);
	XtClearEventHandlers(data->dpy, data->window);
	XtFree((char *)data->sub);
	XtFree((char *)data);
	return XteventHandled;
    }
    return XteventNotHandled;
}

static XtEventReturnCode HandleSubwindow(event)
  XEvent *event;
{
    WidgetData data;
    if (event->type == DestroyNotify) {
	data = DataFromWindow(event->xdestroywindow.display, event->xdestroywindow.window);
	if (data) {
	    XtVPanedWindowDeletePane(data->dpy, data->window,
				     event->xdestroywindow.window);
	    return XteventHandled;
	}
    }
    return XteventNotHandled;
}

static XtEventReturnCode HandleKnob(event)
  XEvent *event;
{
    WidgetData data;
    int     position, diff, y, i;

    data = DataFromWindow(event->xbutton.display, event->xbutton.window);
    if (!data) return XteventNotHandled;
    switch (event->type) {
	case ButtonPress: 
	    y = event->xbutton.y;
	    if (data->whichtracking != -1)
		return XteventNotHandled;
	    for (position = 0; position < data->numsubwindows; position++)
		if (data->sub[position].knob == event->xbutton.window)
		    break;
	    if (position >= data->numsubwindows)
		return XteventNotHandled;
	    
	    (void) XGrabPointer(data->dpy, event->xbutton.window, FALSE,
				(unsigned int)PointerMotionMask | ButtonReleaseMask,
				GrabModeAsync, GrabModeAsync, None,
				XtGetCursor(data->dpy,
					    CursorNums[event->xbutton.button]),
				CurrentTime);
	    data->whichadd = data->whichsub = NULL;
	    data->whichdirection = 2 - event->xbutton.button;/* Hack! */
	    data->starty = y;
	    if (data->whichdirection >= 0) {
		data->whichadd = data->sub + position;
		while (data->whichadd->max == data->whichadd->min &&
			data->whichadd > data->sub)
		    (data->whichadd)--;
	    }
	    if (data->whichdirection <= 0) {
		data->whichsub = data->sub + position + 1;
		while (data->whichsub->max == data->whichsub->min &&
			data->whichsub < data->sub + data->numsubwindows - 1)
		    (data->whichsub)++;
	    }
	    data->whichtracking = position;
	    if (data->whichdirection == 1)
		(data->whichtracking)++;
	    EraseInternalBorders(data);
	    for (i = 0; i < data->numsubwindows; i++)
		data->sub[i].olddy = -99;
	/* Fall through */

	case MotionNotify: 
	case ButtonRelease:
	    if (event->type == MotionNotify) y = event->xmotion.y;
	    else y = event->xbutton.y;
	    if (data->whichtracking == -1)
		return XteventNotHandled;
	    for (i = 0; i < data->numsubwindows; i++)
		data->sub[i].dheight = data->sub[i].height;
	    diff = y - data->starty;
	    if (data->whichadd)
		data->whichadd->dheight = data->whichadd->height + diff;
	    if (data->whichsub)
		data->whichsub->dheight = data->whichsub->height - diff;
	    RefigureLocations(data, data->whichtracking, data->whichdirection);

	    if (event->type != ButtonRelease) {
		DrawTrackLines(data);/* Draw new borders */
		return XteventHandled;
	    }
	    XUngrabPointer(data->dpy, CurrentTime);
	    EraseTrackLines(data);
	    CommitNewLocations(data);
	    DrawInternalBorders(data);
	    data->whichtracking = -1;
	    return XteventHandled;
    }
    return XteventNotHandled;
}
  

/* Public routines. */

static Boolean initialized = FALSE;

extern void VPanedInitialize()
{
    if (initialized)
    	return;
    initialized = TRUE;

    widgetContext = XUniqueContext();
    globaldatainit.window = NULL;
    globaldatainit.x = globaldatainit.y = 0;
    globaldatainit.width = globaldatainit.height = 10;
    globaldatainit.borderWidth = 1;
    globaldatainit.knobwidth = DEFAULTKNOBWIDTH;
    globaldatainit.knobheight = DEFAULTKNOBHEIGHT;
    globaldatainit.knobindent = DEFAULTKNOBINDENT;
}


Window XtVPanedWindowCreate(dpy, parent, args, argCount)
    Display  *dpy;
    Window   parent;
    ArgList  args;
    int      argCount;
{
    WidgetData data;
    XrmNameList	names;
    XrmClassList	classes;
    unsigned long valuemask;
    XSetWindowAttributes wvals;
    XGCValues values;

    if (!initialized) VPanedInitialize();

    data = (WidgetData) XtMalloc(sizeof(WidgetDataRec));
    globaldata = globaldatainit;
    globaldata.dpy = dpy;
    XtGetResources(dpy, resources, XtNumber(resources), args, argCount, parent,
	"vpane", "VPane", &names, &classes);
    *data = globaldata;

    wvals.background_pixel = data->backgroundpixel;
    wvals.border_pixel = data->borderpixel;
    wvals.bit_gravity = NorthWestGravity;
    valuemask = CWBackPixel | CWBorderPixel | CWBitGravity;

    if (data->window != NULL) {
	Drawable root;
	Position x, y;
	unsigned int depth;
	(void) XGetGeometry(data->dpy, data->window, &root, &x, &y,
			    &(data->width), &(data->height), &(data->borderWidth),
			    &depth);
	XReparentWindow(data->dpy, data->window, parent, data->x, data->y);
	XChangeWindowAttributes(data->dpy, data->window, valuemask, &wvals);
    } else {
	data->window = XCreateWindow(data->dpy, parent, data->x, data->y,
			      data->width, data->height, data->borderWidth,
			      (int) CopyFromParent, InputOutput,
			      (Visual *) CopyFromParent, valuemask, &wvals);
    }

    values.foreground = data->foregroundpixel;
    values.function = GXcopy;
    values.plane_mask = ~0;
    values.fill_style = FillSolid;
    values.fill_rule = EvenOddRule;
    values.subwindow_mode = IncludeInferiors;
    valuemask = GCForeground | GCFunction | GCPlaneMask | GCFillStyle
	| GCFillRule | GCSubwindowMode;
    data->normgc = XtGetGC(dpy, widgetContext, data->window, valuemask, &values);
    values.foreground = data->backgroundpixel;
    data->invgc = XtGetGC(dpy, widgetContext, data->window, valuemask, &values);
    values.function = GXinvert;
#if BORDERWIDTH == 1
    values.line_width = 0;	/* Take advantage of fast server lines. */
#else
    values.line_width = BORDERWIDTH;
#endif
    values.line_style = LineSolid;
    valuemask |= GCLineWidth | GCLineStyle;
    data->flipgc = XtGetGC(dpy, widgetContext, data->window, valuemask, &values);

    XtSetNameAndClass(data->dpy, data->window, names, classes);
    XrmFreeNameList(names);
    XrmFreeClassList(classes);
    data->heightused = 0;
    data->numsubwindows = 0;
    data->sub = (SubWindowInfo *) XtMalloc(1);
    data->whichtracking = -1;
    data->refiguremode = TRUE;
    (void) XSaveContext(data->dpy, data->window, widgetContext, (caddr_t) data);
    (void) XtSetEventHandler(data->dpy, data->window, HandleEvents, StructureNotifyMask,
     (caddr_t) NULL);
    return data->window;
}


void XtVPanedWindowDelete(dpy, w)
    Display *dpy;
    Window w;
{
    WidgetData data;
    int i;
    data = DataFromWindow(dpy, w);
    XUnmapWindow(data->dpy, w);
    for (i=data->numsubwindows-1 ; i>=0 ; i--) 
	XtVPanedWindowDeletePane(dpy, w, data->sub[i].w);
    (void) XDeleteContext(dpy, w, widgetContext);
    XtFree((char *) data->sub);
    XtFree((char *) data);
}



void XtVPanedWindowAddPane(
	dpy, parent, paneWindow, position, min, max, autochange)
    Display *dpy;
    Window parent, paneWindow;
    int position, min, max, autochange;
{
    WidgetData data;
    Drawable root;
    int i;
    Position x;
    Dimension width, borderWidth, needed;
    unsigned int depth;
    SubWindowPtr sub;
    data = DataFromWindow(dpy, parent);
    if (!data) return;
    if (position > 0 && position == data->numsubwindows)
	data->sub[position - 1].y = -99; /* HACK to make knob reappear. */
    data->numsubwindows++;
    data->sub = (SubWindowPtr)
	XtRealloc((char *) data->sub,
	       	  (unsigned) data->numsubwindows * sizeof(SubWindowInfo));
    for (i = data->numsubwindows - 1; i > position; i--)
	data->sub[i] = data->sub[i - 1];
    sub = &(data->sub[position]);
    sub->w = paneWindow;
    (void) XGetGeometry(data->dpy, paneWindow, &root, &x, &(sub->y),
			&width, &(sub->height), &borderWidth, &depth);
    XReparentWindow(data->dpy, paneWindow, data->window, -BORDERWIDTH,
		    sub->y);
    sub->dheight = sub->height;
    if (width != data->width || borderWidth != BORDERWIDTH) {
	XWindowChanges changes;
	changes.width = data->width;
	changes.border_width = BORDERWIDTH;
	XConfigureWindow(data->dpy, paneWindow,
			 (unsigned int) CWWidth | CWBorderWidth, &changes);
    }
    sub->min = min;
    sub->max = max;
    sub->autochange = autochange;
    needed = sub->height + BORDERWIDTH + data->heightused;
    if (needed > data->height)
	TryResize(data, data->width, needed);
    (void) XtSetGeometryHandler(data->dpy, paneWindow, PanedWindowGeometryRequest);
    sub->knob = XCreateSimpleWindow(data->dpy, data->window, -99, -99,
			      data->knobwidth, data->knobheight,
			      0, (Pixel) 0, data->knobpixel);
    (void) XSaveContext(dpy, sub->w, widgetContext, (caddr_t) data);
    (void) XSaveContext(dpy, sub->knob, widgetContext, (caddr_t) data);
    XDefineCursor(data->dpy, sub->knob, XtGetCursor(dpy, XC_double_arrow));
    XMapWindow(data->dpy, sub->w);
    XMapWindow(data->dpy, sub->knob);
    (void) XtSetEventHandler(dpy, sub->w, HandleSubwindow, StructureNotifyMask,
     (caddr_t) NULL);
    (void) XtSetEventHandler(dpy, sub->knob, HandleKnob, 
			ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
			(caddr_t) NULL);
    RefigureLocationsAndCommit(data, position, 1);
    for (i=0 ; i<data->numsubwindows; i++)
	XRaiseWindow(data->dpy, data->sub[i].knob);
}


/* Change the min and max size of the given sub window. */

void XtVPanedSetMinMax(dpy, w, paneWindow, min, max)
Display *dpy;
Window w, paneWindow;
int    min, max;
{
    WidgetData data;
    int i;
    data = DataFromWindow(dpy, w);
    if (!data)
	return;
    for (i = 0; i < data->numsubwindows; i++) {
	if (data->sub[i].w == paneWindow) {
	    data->sub[i].min = min;
	    data->sub[i].max = max;
	    RefigureLocationsAndCommit(data, i, 1);
	    return;
	}
    }
}


/* Get the min and max size of the given sub window. */

void XtVPanedGetMinMax(dpy, w, paneWindow, min, max)
Display *dpy;
Window w, paneWindow;
int    *min, *max;
{
    WidgetData data;
    int i;
    data = DataFromWindow(dpy, w);
    if (!data)
	return;
    for (i = 0; i < data->numsubwindows; i++) {
	if (data->sub[i].w == paneWindow) {
	    *min = data->sub[i].min;
	    *max = data->sub[i].max;
	    return;
	}
    }
}



/* Delete the given paneWindow from the given paned window.  Doesn't actually
   destroy the paneWindow. */

void XtVPanedWindowDeletePane(dpy, w, paneWindow)
    Display *dpy;
    Window w, paneWindow;
{
    WidgetData data;
    int     i;
    Boolean j;

    data = DataFromWindow(dpy, w);
    if (!data)
	return;
    j = FALSE;
    for (i = 0; i < data->numsubwindows; i++) {
	if (data->sub[i].w == paneWindow) {
	    j = TRUE;
	    (void) XDeleteContext(dpy, data->sub[i].w, widgetContext);
	    (void) XDeleteContext(dpy, data->sub[i].knob, widgetContext);
	    (void) XtSetEventHandler(dpy, data->sub[i].w, HandleSubwindow,
	     (unsigned long) NULL, (caddr_t) NULL);
	    XtClearEventHandlers(dpy, data->sub[i].knob);
	    XDestroyWindow(data->dpy, data->sub[i].knob);
	    TryResize(data, data->width,
		      data->height - data->sub[i].height - BORDERWIDTH);
	}
	if (j && i < data->numsubwindows - 1)
	    data->sub[i] = data->sub[i + 1];
    }
    if (!j)
	return;
    data->numsubwindows--;
    if (data->numsubwindows > 0) {
	data->sub = (SubWindowPtr)
	    XtRealloc((char *) data->sub,
	     (unsigned) data->numsubwindows * sizeof(SubWindowInfo));
    }
    RefigureLocationsAndCommit(data, data->numsubwindows - 1, -1);
}


void XtVPanedAllowResize(dpy, window, paneWindow, allowresize)
    Display *dpy;
    Window window, paneWindow;
    Boolean  allowresize;
{
    WidgetData data;
    int i;
    data = DataFromWindow(dpy, window);
    if (data)
	for (i=0 ; i<data->numsubwindows ; i++)
	    if (data->sub[i].w == paneWindow)
		data->sub[i].allowresize = allowresize;
}



Boolean XtVPanedGetResize(dpy, window, paneWindow)
    Display *dpy;
    Window window, paneWindow;
{
    WidgetData data;
    int i;
    data = DataFromWindow(dpy, window);
    if (data)
	for (i=0 ; i<data->numsubwindows ; i++)
	    if (data->sub[i].w == paneWindow)
		return(data->sub[i].allowresize);
    return (0);
}



int XtVPanedGetNumSub(dpy, window)
    Display *dpy;
    Window window;
{
    WidgetData data;
    int i;
    data = DataFromWindow(dpy, window);
    if (data)
        return(data->numsubwindows);
    return (0);
}




void XtVPanedRefigureMode(dpy, window, mode)
  Display *dpy;
  Window window;
  Boolean  mode;
{
    WidgetData data;
    data = DataFromWindow(dpy, window);
    if (data) {
	data->refiguremode = mode;
	if (mode)
	    RefigureLocationsAndCommit(data, data->numsubwindows - 1, -1);
    }
}

void XtVPaneGetValues(dpy, window, args, argCount)
Display *dpy;
Window window;
ArgList args;
int argCount;
{
    WidgetData data;
    data = DataFromWindow(dpy, window);
    if (data == NULL) return;

    globaldata = *data;
    XtGetValues(resources, XtNumber(resources), args, argCount);
}

void XtVPaneSetValues(dpy, window, args, argCount)
Display *dpy;
Window window;
ArgList args;
int argCount;
{
    WidgetData data;
    data = DataFromWindow(dpy, window);
    if (data == NULL) return;

    globaldata = *data;
    XtSetValues(resources, XtNumber(resources), args, argCount);
    *data = globaldata;
}
