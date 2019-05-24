/* $Header: ButtonBox.c,v 1.1 87/09/11 07:57:18 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)ButtonBox.c	1.14	2/26/87";
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


/* 
 * ButtonBox.c - Button box composite widget
 * 
 * Author:	haynes
 * 		Digital Equipment Corporation
 * 		Western Research Laboratory
 * Date:	Sat Jan 24 1987
 */

#include	"Xlib.h"
#include	"Intrinsic.h"
#include	"ButtonBox.h"
#include	"Atoms.h"

#define MAXHEIGHT	((1 << 31)-1)
#define MAXWIDTH	((1 << 31)-1)

#define max(x, y)	(((x) > (y)) ? (x) : (y))
#define min(x, y)	(((x) < (y)) ? (x) : (y))
#define assignmax(x, y)	if ((y) > (x)) x = (y)
#define assignmin(x, y)	if ((y) < (x)) x = (y)

/****************************************************************
 *
 * Private Types
 *
 ****************************************************************/

typedef	struct _WidgetDataRec {
    Display		*dpy;		/* widget display connection */
    Window		w;		/* widget window */
    Position		x, y;		/* widget window location */
    Dimension		width, height;	/* widget window width and height */
    Dimension		borderWidth;	/* widget window border width */
    Pixel		borderpixel;	/* widget window border color */
    Pixel		bgpixel;	/* widget window background color */
    Dimension		hspace, vspace;	/* spacing between buttons */
    int			numbuttons;	/* number of managed buttons */
    WindowLugPtr	buttons;	/* list of managed buttons */
    Boolean             allowresize;    /* reconfigure on notify */
} WidgetDataRec, *WidgetData;


/****************************************************************
 *
 * Private Data
 *
 ****************************************************************/

static int	 index; /* index into button list to add/delete window */

static WidgetDataRec globaldata;
static WidgetDataRec globalinit = {
    NULL,		/* Display dpy; */
    NULL,		/* Window w; */
    0, 0,		/* Position x, y; */
    0, 0,		/* Dimension width, height; */
    1,			/* Dimension borderWidth; */
    NULL,		/* Pixmap borderpixel; */
    NULL,		/* Pixmap bgpixel; */
    4, 4,		/* int hspace, vspace; */
    0,			/* int numbuttons; */
    NULL,		/* WindowLugPtr buttons; */
    TRUE		/* resizable */
};

static Resource resources[] = {
    {XtNwindow, XtCWindow, XrmRWindow, sizeof(Window),
	 (caddr_t)&globaldata.w, (caddr_t)NULL},
    {XtNx, XtCX, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.x, (caddr_t)NULL},
    {XtNy, XtCY, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.y, (caddr_t)NULL},
    {XtNwidth, XtCWidth, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.width, (caddr_t)NULL},
    {XtNheight, XtCHeight, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.height, (caddr_t)NULL},
    {XtNborderWidth, XtCBorderWidth, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.borderWidth, (caddr_t)NULL},
    {XtNborder, XtCColor, XrmRPixel, sizeof(int),
	 (caddr_t)&globaldata.borderpixel, (caddr_t)&XtDefaultFGPixel},
    {XtNbackground, XtCColor, XrmRPixel, sizeof(int),
	 (caddr_t)&globaldata.bgpixel, (caddr_t)&XtDefaultBGPixel},
    {XtNhSpace, XtCHSpace, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.hspace, (caddr_t)NULL},
    {XtNvSpace, XtCVSpace, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.vspace, (caddr_t)NULL},
    {XtNresizable, XtCBoolean, XrmRBoolean, sizeof(Boolean),
	 (caddr_t)&globaldata.allowresize, (caddr_t) NULL}
};


static int	indexinit = -1;
static Resource parmResources[] = {
    {XtNindex, XtCIndex, XrmRInt,
        sizeof(int), (caddr_t)&index, (caddr_t)&indexinit},
};

/****************************************************************
 *
 * Private Routines
 *
 ****************************************************************/

static Boolean initialized = FALSE;

static XContext widgetContext;

static void ButtonBoxInitialize()
{
    if (initialized)
    	return;
    initialized = TRUE;

    widgetContext = XUniqueContext();
}

/*
 * Given a display and window, get the widget data.
 */

static WidgetData DataFromWindow(dpy, window)
Display *dpy;
Window window;
{
    WidgetData result;
    if (XFindContext(dpy, window, widgetContext, (caddr_t *)&result))
        return NULL;
    return result;
}

/*
 *
 * Do a layout, either actually assigning positions, or just calculating size.
 * Returns 1 on success; 0 if it couldn't make things fit.
 *
 */

static int DoLayout(data, width, height, box, position)
WidgetData	data;
Dimension	width, height;
WindowBox	*box;		/* RETURN */
Boolean		position;	/* actually reposition the windows? */
{
    int	i;
    int	w, h, lw, lh, count;

    w = data->hspace;
    if ((w > width) && !position) return (0);
    h = data->vspace;
    if ((h > height) && !position) return (0);
   
    for (i = 0; i < data->numbuttons; ) {
	count = 0;
	lh = 0;
	lw = data->hspace;
	/* compute one line worth */
	for ( ; (i < data->numbuttons); i++) {
	    int tw, th;
	    tw = lw
	        + data->buttons[i].wb.width
		+ 2*data->buttons[i].wb.borderWidth
		+ data->hspace;
	    if (tw > width) {
	      if (!position) break;
	      if (count > 0) break;
	    }
	    if (position &&
		(lw != data->buttons[i].wb.x || h != data->buttons[i].wb.y)) {
		XMoveWindow(data->dpy, data->buttons[i].w, lw, h);
		data->buttons[i].wb.x = lw;
		data->buttons[i].wb.y = h;
		(void) XtSendConfigureNotify(data->dpy, data->buttons[i].w,
					     &(data->buttons[i].wb));
	    }
	    lw = tw;
	    th = data->buttons[i].wb.height + 2*data->buttons[i].wb.borderWidth;
	    assignmax(lh, th);
	    count++;
	}
	if (count == 0) return (0);
	assignmax(w, lw);
	h += lh+data->vspace;
	if ((h > height) && !position) return (0);
    }

    if (box != NULL) {
	box->width = w;
	box->height = h;
    }
    return (1);
}

/*
 *
 * Calculate preferred size, given constraining box
 *
 */

static int PreferredSize(data, width, height, box)
WidgetData	data;
Dimension	width, height;
WindowBox	*box;		/* RETURN */
{
    return DoLayout(data, width, height, box, FALSE);
}

/*
 *
 * Compute the layout of the button box
 *
 */

static void Layout(data)
WidgetData	data;
{
    (void) DoLayout(data, data->width, data->height, (WindowBox *)NULL, TRUE);
}

/*
 *
 * Main buttonbox event handler
 *
 */

static XtEventReturnCode EventHandler(event, eventdata)
XEvent *event;
caddr_t eventdata;
{
    WidgetData	data = (WidgetData) eventdata;
    void Destroy();

    switch (event->type) {
	case ConfigureNotify: {
	    data->x = event->xconfigure.x;
	    data->y = event->xconfigure.y;
	    data->borderWidth = event->xconfigure.border_width;
            if (data->allowresize) {
	        if (data->height != event->xconfigure.height ||
		      data->width != event->xconfigure.width) {
		    data->height = event->xconfigure.height;
		    data->width = event->xconfigure.width;
		    (void) TryLayout(data, data->width, MAXHEIGHT);
		    Layout(data);
	        }
	    }
	    break;
	}

        case DestroyNotify: Destroy(data); break;
    }

    return (XteventHandled);
}


/*
 *
 * Handle events on subwidgets
 *
 */

static XtEventReturnCode SubEventHandler(event, eventdata)
XEvent *event;
caddr_t eventdata;
{
    WidgetData data = (WidgetData) eventdata;
    int i;
    if (event->type == DestroyNotify) {
	for (i=0 ; i<data->numbuttons; i++)
	    if (data->buttons[i].w == event->xany.window) {
		DeleteButton(data, i);
		(void) TryNewLayout(data);
		Layout(data);
		break;
	    }
    }
}

/*
 *
 * Destroy the buttonbox
 *
 */

static void Destroy(data)
WidgetData	data;
{
    int	i;
    /* send destroy messages to all my subwindows */
    for (i=0; i < data->numbuttons; i++)
	(void) XtSendDestroyNotify(data->dpy, data->buttons[i].w);
    XtFree((char *) data->buttons);

    XtClearEventHandlers(data->dpy, data->w);
    (void) XDeleteContext(data->dpy, data->w, widgetContext);
    XtFree ((char *) data);
}

/*
 *
 * Find Button
 *
 */

static WindowLugPtr FindButton(data, w)
    WidgetData	data;
    Window	w;
{
    int	i;

    for (i=0; i<data->numbuttons; i++)
	if (data->buttons[i].w == w) return &(data->buttons[i]);

    return NULL;
}

/*
 *
 * Try to do a new layout within a particular width and height
 *
 */

static int TryLayout(data, width, height)
WidgetData	data;
Dimension	width, height;
{
    WindowBox	box, rbox;

    if (!PreferredSize(data, width, height, &box)) return (0);

    /* let's see if our parent will go for it. */
    switch (XtMakeGeometryRequest(
	data->dpy, data->w, XtgeometryResize, &box, &rbox)) {

	case XtgeometryNoManager:
	    XResizeWindow(data->dpy, data->w, box.width, box.height);
	    /* fall through to "yes" */

	case XtgeometryYes:
	    data->width = box.width;
	    data->height = box.height;
	    return (1);


	case XtgeometryNo:
	    return (0);


	case XtgeometryAlmost:
	    if (! PreferredSize(data, rbox.width, rbox.height,
				(WindowBox *) NULL))
	        return (0);
	    box = rbox;
	    (void) XtMakeGeometryRequest(data->dpy, data->w, XtgeometryResize, &box, &rbox);
	    data->width = box.width;
	    data->height = box.height;
	    return (1);

    }
    return (0);
}

/*
 *
 * Try to do a new layout
 *
 */

static int TryNewLayout(data)
WidgetData	data;
{
    if (TryLayout(data, data->width, data->height)) return (1);
    if (TryLayout(data, data->width, MAXHEIGHT)) return (1);
    if (TryLayout(data, MAXWIDTH, MAXHEIGHT)) return(1);
    return (0);
}

/*
 *
 * Button Resize Request
 *
 */

/*ARGSUSED*/
static XtGeometryReturnCode ResizeButtonRequest(data, w, reqBox, replBox)
WidgetData	data;
Window		w;
WindowBox	*reqBox;
WindowBox	*replBox;	/* RETURN */

{
    WindowLugPtr	b;
    WindowLug		oldb;

    b = FindButton(data, w);
    if (b == NULL) return (XtgeometryNo);
    oldb = *b;
    b->wb = *reqBox;
    b->wb.borderWidth = oldb.wb.borderWidth;
 /* HACK -- maybe we need a "change borderWidth" command? */

    if ((reqBox->width <= oldb.wb.width && reqBox->height <= oldb.wb.height) ||
				/* making the button smaller always works */
	(PreferredSize(data, data->width, data->height, (WindowBox *) NULL)) ||
				/* will it fit inside old dims? */
	(TryNewLayout(data)))
				/* can we make it fit at all? */
    {
	XResizeWindow(data->dpy, b->w, b->wb.width, b->wb.height);
	(void) XtSendConfigureNotify(data->dpy, b->w, &(b->wb));
	Layout(data);
	return XtgeometryYes;
    }
    *b = oldb;
    return (XtgeometryNo);
}

/*
 *
 * Button Box Geometry Manager
 *
 */

static XtGeometryReturnCode ButtonBoxGeometryManager(
	dpy, w, req, reqBox, replBox)
    Display		*dpy;
    Window		w;
    XtGeometryRequest	req;
    WindowBox		*reqBox;
    WindowBox		*replBox;	/* RETURN */
{
    WidgetData	data;

    if (XFindContext(dpy, w, widgetContext, (caddr_t *)&data) == XCNOENT)
	return (XtgeometryYes);
    /* requests: move, resize, top, bottom */
    switch (req) {
    case XtgeometryTop    : return (XtgeometryYes);
    case XtgeometryBottom : return (XtgeometryYes);
    case XtgeometryMove   : return (XtgeometryNo);
    case XtgeometryResize :
        return (ResizeButtonRequest(data, w, reqBox, replBox));
    }
    return (XtgeometryNo);
}

static XtStatus AddButton(data, w, index)
WidgetData data;
Window	w;
int	index;
{
    int		i;
    WindowLug	b;

    b.w = w;
    if (XtGetWindowSize(data->dpy, w, &b.wb.width, &b.wb.height,
			&b.wb.borderWidth))
	return (0);
    
    if (FindButton(data, b.w) != NULL) return (0);

    data->numbuttons++;
    if (data->numbuttons == 1) {
	data->buttons = (WindowLugPtr) XtCalloc(1, sizeof(WindowLug));
    } else {
	data->buttons = (WindowLugPtr) XtRealloc(
	    (char *)data->buttons,
	    (unsigned) data->numbuttons*sizeof(WindowLug));
    }

    for (i=data->numbuttons-1; i > index; i--)
	data->buttons[i] = data->buttons[i-1];

    b.wb.x = b.wb.y = -99;
    data->buttons[index] = b;
    (void) XSaveContext(data->dpy, b.w, widgetContext, (caddr_t)data);
    (void) XtSetGeometryHandler(
	data->dpy, b.w, (XtGeometryHandler) ButtonBoxGeometryManager);
    XtSetEventHandler(data->dpy, b.w, SubEventHandler, StructureNotifyMask,
		      (caddr_t)data);

    return(1);
}

static DeleteButton(data, index)
    WidgetData	data;
    int		index;
{
    (void) XDeleteContext(data->dpy, data->buttons[index].w, widgetContext);
    (void) XtClearGeometryHandler(data->dpy, data->buttons[index].w);
    XtDeleteEventHandler(data->dpy, data->buttons[index].w, SubEventHandler);

    for (index++; index<data->numbuttons; index++)
	data->buttons[index-1] = data->buttons[index];

    data->numbuttons--;
    data->buttons = (WindowLugPtr) XtRealloc(
        (char *)data->buttons,
	(unsigned) data->numbuttons*sizeof(WindowLug));
}

/****************************************************************
 *
 * Public Routines
 *
 ****************************************************************/

Window XtButtonBoxCreate(dpy, parent, args, argCount)
    Display  *dpy;
    Window   parent;
    ArgList  args;
    int      argCount;
{
    WidgetData	data;
    XrmNameList	names;
    XrmClassList classes;
    unsigned long valuemask;
    XSetWindowAttributes wvals;
    Boolean found;

    if (!initialized) ButtonBoxInitialize();

    data = (WidgetData) XtMalloc(sizeof(WidgetDataRec));

    globaldata = globalinit;
    globaldata.dpy = dpy;
    XtGetResources(dpy, resources, XtNumber(resources), args, argCount, parent,
    	"buttonBox", "ButtonBox", &names, &classes);
    *data = globaldata;
    if (data->width == 0)
        data->width = ((data->hspace != 0) ? data->hspace : 10);
    if (data->height == 0)
	data->height = ((data->vspace != 0) ? data->vspace : 10);

    wvals.background_pixel = data->bgpixel;
    wvals.border_pixel = data->borderpixel;
    wvals.bit_gravity = NorthWestGravity;
    valuemask = CWBackPixel | CWBorderPixel | CWBitGravity;
    
    if (data->w != NULL) {
	Drawable root;
	Position x, y;
	unsigned int depth;

        /* set global data from window parameters */
        if (!XGetGeometry(data->dpy, data->w, &root, &x, &y,
			  &(data->width), &(data->height),
			  &(data->borderWidth), &depth)) {
            data->w = NULL;
        } else {
            /* set window according to args */
	    XChangeWindowAttributes(data->dpy, data->w, valuemask, &wvals);
	}
    }
    if (data->w == NULL) {
	data->w = XCreateWindow(data->dpy, parent, data->x, data->y,
				data->width, data->height, data->borderWidth,
				0, InputOutput, (Visual *)CopyFromParent,
				valuemask, &wvals);
    }

    XtSetNameAndClass(data->dpy, data->w, names, classes);
    XrmFreeNameList(names);
    XrmFreeClassList(classes);

    /* set handler for message and destroy events */
    XtSetEventHandler(dpy, 
       data->w, (XtEventHandler)EventHandler, StructureNotifyMask,
       (caddr_t) data);

    (void) XSaveContext(data->dpy, data->w, widgetContext, (caddr_t)data);

    /* batch add initial buttons */
    if (argCount) {
	found = FALSE;
	for ( ; --argCount >= 0; args++) {
	    if (XrmAtomsEqual(args->name, XtNbutton)) {
		(void) AddButton(
			data, (Window)args->value, data->numbuttons);
		found = TRUE;
	    }
	}
	if (found) {
	    (void) TryNewLayout(data);
	    Layout(data);
	    XMapSubwindows(data->dpy, data->w);
	}
    }

    return (data->w);
}

XtStatus XtButtonBoxAddButton(dpy, parent, args, argCount)
    Display  *dpy;
    Window   parent;
    ArgList  args;
    int      argCount;
{
    WidgetData	data;

    (void) XFindContext(dpy, parent, widgetContext, (caddr_t *)&data);
    index = -1;
    XtSetValues(parmResources, XtNumber(parmResources), args, argCount);
    if ((index < -1) || (index > data->numbuttons)) return (0);

    /* batch add buttons */
    if (argCount) {
	for ( ; --argCount >= 0; args++) {
	    if (XrmAtomsEqual(args->name, XtNwindow)) {
		(void) AddButton(
			data,
			(Window)args->value,
			((index < 0) ? data->numbuttons : index));
		if (index >= 0) index++;
	    }
	}
	(void) TryNewLayout(data);
	Layout(data);
	XMapSubwindows(data->dpy, data->w);
    }
/*
    if (!AddButton(data, parms.w, index)) return(0);

    if (!TryNewLayout(data)) return(0);
    Layout(data);
*/
    return (1);
}

XtStatus XtButtonBoxDeleteButton(dpy, parent, args, argCount)
    Display  *dpy;
    Window   parent;
    ArgList  args;
    int      argCount;
{
    WidgetData	data;
    Boolean	foundOne = FALSE;
    int		i;

    (void) XFindContext(dpy, parent, widgetContext, (caddr_t *) &data);

    index = -1;
    XtSetValues(parmResources, XtNumber(parmResources), args, argCount);
    if ((index < -1) || (index >= data->numbuttons)) return(0);

    if (index >= 0) {
	DeleteButton(data, index);
	foundOne = TRUE;
    } else if (argCount) {
	for ( ; --argCount >= 0; args++) {
	    if (XrmAtomsEqual(args->name, XtNwindow)) {
		for (i=0; i<data->numbuttons; i++) {
		    if (data->buttons[i].w == (Window)args->value) {
			DeleteButton(data, i);
			foundOne = TRUE;
			break;
		    }
		}
	    }
	}
    }

    if (! foundOne) return(0);

    (void) TryNewLayout(data);	/* We may want to SHRINK things! */
    Layout(data);
    return (1);
}

/*
 *
 * Get Attributes
 *
 */

void XtButtonBoxGetValues (dpy, window, args, argCount)
Display *dpy;
Window window;
ArgList args;
int argCount;
{
    WidgetData  data;
    data = DataFromWindow(dpy, window);
    if (data == NULL) return;
    globaldata = *data;
    XtGetValues(resources, XtNumber(resources), args, argCount);
}

/*
 *
 * Set Attributes
 *
 */

void XtButtonBoxSetValues (dpy, window, args, argCount)
Display *dpy;
Window window;
ArgList args;
int argCount;
{
    WidgetData  data;
    data = DataFromWindow(dpy, window);
    if (data == NULL) return;
    globaldata = *data;
    XtSetValues(resources, XtNumber(resources), args, argCount);
    *data = globaldata;
}

