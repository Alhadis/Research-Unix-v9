/* $Header: Form.c,v 1.1 87/09/11 07:58:23 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)Form.c	1.17	5/18/87";
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
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#include "Xlib.h"
#include "Intrinsic.h"
#include "Form.h"
#include "Atoms.h"


/* Private Definitions */


typedef struct SubWidgetDataRec {
    Window w;			/* Window id for this subwidget. */
    struct WidgetDataRec *data;	/* Pointer to form widget data. */
    XtEdgeType top, bottom, left, right;
				/* Where to drag things on resize. */
    int dx;			/* Desired initial horiz offset. */
    Window fromhoriz;		/* Other subwidget to base dx off of.
				   (NULL if form edge.) */
    int dy;			/* Desired initial vertical offset. */
    Window fromvert;		/* Other subwidget to base dy off of.
				   (NULL if form edge.) */
    Boolean allowresize;	/* TRUE if we allow the subwidget to resize
				   itself. */
    WindowBox wb;		/* Current coordinates.  */
} SubWidgetDataRec, *SubWidgetData;


static SubWidgetDataRec subglob;
static SubWidgetDataRec subinit = {0};

typedef struct WidgetDataRec {
    Display *dpy;		/* Display connection for the form widget */
    Window mywin;		/* Window ID of form widget. */
    WindowBox wb;		/* Coordinates of form widget. */
    SubWidgetData *sub;		/* Array of subwidgets. */
    int numsub;			/* Number of subwidgets. */
    Pixel background;		/* Background color. */
    Pixel border;		/* Border color. */
    int defaultdistance;	/* Default distance between widgets. */
    int dontrefigure;		/* Nonzero if we don't want to recalc. */
    Boolean needsrefigure;	/* TRUE if we need to recalc. */
} WidgetDataRec, *WidgetData;

/* !!! STATIC !!! */
static WidgetDataRec globaldata;
static WidgetDataRec globalinit = {0};

/* Private Data */

static Resource resources[] = {
    {XtNwindow, XtCWindow, XrmRWindow, sizeof(Window),
	 (caddr_t)&globaldata.mywin, (caddr_t)NULL},
    {XtNborderWidth, XtCBorderWidth, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.wb.borderWidth, (caddr_t)NULL},
    {XtNx, XtCX, XrmRInt, sizeof(int),
  (caddr_t)&globaldata.wb.x, (caddr_t)NULL},
    {XtNy, XtCY, XrmRInt, sizeof(int),
  (caddr_t)&globaldata.wb.y, (caddr_t)NULL},
    {XtNwidth, XtCWidth, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.wb.width, (caddr_t)NULL},
    {XtNheight, XtCHeight, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.wb.height, (caddr_t)NULL},
    {XtNbackground, XtCColor, XrmRPixel, sizeof(Pixel),
	 (caddr_t)&globaldata.background, (caddr_t)&XtDefaultBGPixel},
    {XtNborder, XtCColor, XrmRPixel, sizeof(Pixel),
	 (caddr_t)&globaldata.border,(caddr_t)&XtDefaultFGPixel},
    {XtNdefaultDistance, XtCThickness, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.defaultdistance, (caddr_t)NULL},
};

static XtEdgeType defEdge = XtRubber;

static Resource subresources[] = {
    {XtNtop, XtCEdge, XtRJustify, sizeof(XtEdgeType),
	 (caddr_t)&subglob.top, (caddr_t)&defEdge},
    {XtNbottom, XtCEdge, XtRJustify, sizeof(XtEdgeType),
	 (caddr_t)&subglob.bottom, (caddr_t)&defEdge},
    {XtNleft, XtCEdge, XtRJustify, sizeof(XtEdgeType),
	 (caddr_t)&subglob.left, (caddr_t)&defEdge},
    {XtNright, XtCEdge, XtRJustify, sizeof(XtEdgeType),
	 (caddr_t)&subglob.right, (caddr_t)&defEdge},
    {XtNfromHoriz, XtCWindow, XrmRWindow, sizeof(Window),
	 (caddr_t)&subglob.fromhoriz, (caddr_t)NULL},
    {XtNfromVert, XtCWindow, XrmRWindow, sizeof(Window),
	 (caddr_t)&subglob.fromvert, (caddr_t)NULL},
    {XtNhorizDistance, XtCThickness, XrmRInt, sizeof(int),
	 (caddr_t)&subglob.dx, (caddr_t)NULL},
    {XtNvertDistance, XtCThickness, XrmRInt, sizeof(int),
	 (caddr_t)&subglob.dy, (caddr_t)NULL},
    {XtNresizable, XtCBoolean, XrmRBoolean, sizeof(Boolean),
	 (caddr_t)&subglob.allowresize, (caddr_t) NULL}
};


static XContext formContext;
static XContext subContext;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

static Boolean initialized = FALSE;

static void Initialize () 
{
    if (initialized)
    	return;
    initialized = TRUE;

    subinit.top = subinit.bottom = subinit.left = subinit.right = XtRubber;
    subinit.wb.x = subinit.wb.y = -1;

    globalinit.wb.width = globalinit.wb.height = 10;
    globalinit.wb.x = globalinit.wb.y = 0;
    globalinit.defaultdistance = 4;
    globalinit.wb.borderWidth = 1;

    formContext = XUniqueContext();
    subContext = XUniqueContext();
}


static WidgetData DataFromWindow(dpy, w)
Display *dpy;
Window w;
{
    WidgetData result;
    if (XFindContext(dpy, w, formContext, (caddr_t *)&result))
	return NULL;
    return result;
}

static SubWidgetData SubFromWindow(dpy, w)
Display *dpy;
Window w;
{
    SubWidgetData result;
    if (XFindContext(dpy, w, subContext, (caddr_t *)&result))
	return NULL;
    return result;
}

static void RefigureLocations(data)
WidgetData data;
{
    int i;
    Position x, y, maxx, maxy;
    SubWidgetData sub, sub2;
    WindowBox reqBox, replBox;
    XtGeometryReturnCode result;
    if (data->dontrefigure) {
	data->needsrefigure = TRUE;
	return;
    }
    maxx = maxy = 0;
    for (i=0 ; i<data->numsub ; i++) {
	sub = data->sub[i];
	x = sub->dx;
	y = sub->dy;
	if (sub->fromhoriz && (sub2 = SubFromWindow(data->dpy, sub->fromhoriz)))
	    x += sub2->wb.x + sub2->wb.width;
	if (sub->fromvert && (sub2 = SubFromWindow(data->dpy, sub->fromvert)))
	    y += sub2->wb.y + sub2->wb.height;
	if (x != sub->wb.x || y != sub->wb.y) {
	    XMoveWindow(data->dpy, sub->w, x, y);
	    sub->wb.x = x;
	    sub->wb.y = y;
	}
	if (maxx < x + sub->wb.width) maxx = x + sub->wb.width;
	if (maxy < y + sub->wb.height) maxy = y + sub->wb.height;
    }
    maxx += data->defaultdistance;
    maxy += data->defaultdistance;
    if (maxx != data->wb.width || maxy != data->wb.height) {
	reqBox.width = maxx;
	reqBox.height = maxy;
	result = XtMakeGeometryRequest(data->dpy, data->mywin, XtgeometryResize,
				       &reqBox, &replBox);
	if (result == XtgeometryAlmost) {
	    replBox = reqBox;
	    result = XtMakeGeometryRequest(data->dpy, data->mywin, XtgeometryResize,
					   &reqBox, &replBox);
	}
	if (result == XtgeometryYes) {
	    data->wb.width = reqBox.width;
	    data->wb.height = reqBox.height;
	}
    }
    data->needsrefigure = FALSE;
}


static int MoveEdge(loc, old, new, type)
int loc, old, new;
XtEdgeType type;
{
    if (type == XtRubber)
	return loc * new / old;
    if (type == XtChainTop || type == XtChainLeft)
	return loc;
    return loc + new - old;
}

static void ChangeSize(data, newwidth, newheight)
WidgetData data;
int newwidth, newheight;
{
    int i;
    SubWidgetData sub;
    WindowBox wb;
    for (i=0 ; i<data->numsub ; i++) {
	sub = data->sub[i];
	wb.borderWidth = sub->wb.borderWidth;
	wb.x = MoveEdge(sub->wb.x, (int)data->wb.width, newwidth, sub->left);
	wb.y = MoveEdge(sub->wb.y, (int)data->wb.height, newheight, sub->top);
	wb.width =
	    MoveEdge((int)(sub->wb.x + sub->wb.width),
		       (int)data->wb.width, newwidth, sub->right) - wb.x;
	wb.height = MoveEdge((int)(sub->wb.y + sub->wb.height),
			    (int)data->wb.height, newheight, sub->bottom)
			- wb.y;
	if (wb.width < 1) wb.width = 1;
	if (wb.height < 1) wb.height = 1;
	if (wb.x != sub->wb.x || wb.y != sub->wb.y ||
	        wb.width != sub->wb.width || wb.height != sub->wb.height) {
	    XMoveResizeWindow(data->dpy, sub->w, wb.x, wb.y,
			      wb.width, wb.height);
	    (void) XtSendConfigureNotify(data->dpy, sub->w, &wb);
	    sub->wb = wb;
	}
    }
    data->wb.width = newwidth;
    data->wb.height = newheight;
}


/* ARGSUSED */
static XtGeometryReturnCode FormGeometryHandler(dpy, w, req, reqBox, replBox)
Display *dpy;
Window w;
XtGeometryRequest req;
WindowBox *reqBox;
WindowBox *replBox;	/* RETURN */
{
    SubWidgetData sub;
    sub = SubFromWindow(dpy, w);
    if (sub && req == XtgeometryResize && sub->allowresize) {
	XResizeWindow(sub->data->dpy, sub->w, reqBox->width, reqBox->height);
	*replBox = *reqBox;
	sub->wb.width = reqBox->width;
	sub->wb.height = reqBox->height;
	(void) XtSendConfigureNotify(sub->data->dpy, w, &(sub->wb));
	RefigureLocations(sub->data);
	return XtgeometryYes;
    }
    return XtgeometryNo;
}


/*
 *
 * Destroy the widget
 *
 */

static void Destroy(data)
WidgetData data;
{
    int i;
    for (i=0 ; i<data->numsub; i++) {
	(void) XDeleteContext(data->dpy, data->sub[i]->w, subContext);
	(void) XtSendDestroyNotify(data->dpy, data->sub[i]->w);
    }
    XtClearEventHandlers(data->dpy, data->mywin);
    (void) XDeleteContext(data->dpy, data->mywin, formContext);
    XtFree((char *) data->sub);
    XtFree((char *) data);
}


/*
 *
 * Generic widget event handler
 *
 */

static XtEventReturnCode EventHandler(event, eventdata)
XEvent *event;
caddr_t eventdata;
{
    WidgetData	data = (WidgetData ) eventdata;
    int newwidth, newheight;

    switch (event->type) {
	case ConfigureNotify:
	    newwidth = event->xconfigure.width;
	    newheight = event->xconfigure.height;
	    if (newwidth != data->wb.width || newheight != data->wb.height)
		ChangeSize(data, newwidth, newheight);
	    break;

	case DestroyNotify:
	    Destroy(data);
	    break;
	
    }
    return (XteventHandled);
}

/*
 * Handle events in subwidgets. 
 */

static XtEventReturnCode SubEventHandler(event, sub)
XEvent *event;
SubWidgetData sub;
{
    if (event->type == DestroyNotify) {
	XtFormRemoveWidget(sub->data->dpy, sub->data->mywin, sub->w);
	return XteventHandled;
    }
    return XteventNotHandled;
}

/****************************************************************
 *
 * Public Procedures
 *
 ****************************************************************/

Window XtFormCreate(dpy, parent, args, argCount)
Display  *dpy;
Window   parent;
ArgList  args;
int      argCount;
{
    WidgetData data;
    XrmNameList  names;
    XrmClassList classes;
    if (!initialized)
    	Initialize ();

    data = (WidgetData) XtMalloc(sizeof(WidgetDataRec));

    /* Set Default Values */
    globaldata = globalinit;
    globaldata.dpy = dpy;
    XtGetResources(dpy, resources, XtNumber(resources), args, argCount, parent,
		   "form", "Form", &names, &classes);
    *data = globaldata;    

    data->numsub = 0;
    data->sub = (SubWidgetData *)XtMalloc(sizeof(SubWidgetData));
    if (data->mywin != NULL) {
	XWindowAttributes wi;
	/* set global data from window parameters */
	if (! XGetWindowAttributes(data->dpy, data->mywin, &wi)) {
	    data->mywin = NULL;
	} else {
	    data->wb.borderWidth = wi.border_width;
	    data->wb.width = wi.width;
	    data->wb.height = wi.height;
            data->wb.x = wi.x;
            data->wb.y = wi.y;
	}
    }
    if (data->mywin == NULL) {
	/* create the Form window */
        data->mywin = XCreateSimpleWindow(data->dpy, parent, data->wb.x,
					  data->wb.y, data->wb.width, data->wb.height,
					  data->wb.borderWidth,
					  data->border, data->background);
    }

    XtSetNameAndClass(data->dpy, data->mywin, names, classes);
    XrmFreeNameList(names);
    XrmFreeClassList(classes);
    (void) XSaveContext(data->dpy, data->mywin, formContext, (caddr_t)data);

    /* set handler for resize, and message events */

    XtSetEventHandler (data->dpy, data->mywin, (XtEventHandler)EventHandler,
		       StructureNotifyMask, (caddr_t)data);

    return (data->mywin);
}

/*
 *
 * Get Attributes
 *
 */

void XtFormGetValues (dpy, w, args, argCount)
Display *dpy;
Window w;
ArgList args;
int argCount;
{
    WidgetData data;

    if ((data = DataFromWindow(dpy, w)) == NULL)
	return;
    globaldata = *data;
    XtGetValues(resources, XtNumber(resources), args, argCount);
}

/*
 *
 * Set Values
 *
 */

void XtFormSetValues (dpy, w, args, argCount)
Display *dpy;
Window w;
ArgList args;
int argCount;
{
    WidgetData data;

    if ((data = DataFromWindow(dpy, w)) == NULL)
	return;
    globaldata = *data;
    XtSetValues(resources, XtNumber(resources), args, argCount);
    *data = globaldata;
}


/*
 * Add a new widget to the form.
 */

void XtFormAddWidget(dpy, mywin, w, args, argCount)
Display *dpy;
Window mywin, w;
ArgList args;
int argCount;
{
    WidgetData data;
    SubWidgetData sub;
    XrmNameList names;
    XrmClassList classes;
    XWindowAttributes info;
    if ((data = DataFromWindow(dpy, mywin)) == NULL) return;
    if ((sub = SubFromWindow(dpy, w)) == NULL) {
	data->sub = (SubWidgetData *)
	    XtRealloc((char *) data->sub,
		      (unsigned) sizeof(SubWidgetData) * ++data->numsub);
	sub = data->sub[data->numsub - 1] =
	    (SubWidgetData) XtMalloc(sizeof(SubWidgetDataRec));
	(void)XSaveContext(dpy, w, subContext, (caddr_t)sub);
	*sub = subinit;
	sub->data = data;
	sub->w = w;
	sub->dx = sub->dy = data->defaultdistance;
	(void) XGetWindowAttributes(data->dpy, w, &info); /* %%% Icky-poo. */
	sub->wb.width = info.width;
	sub->wb.height = info.height;
	sub->wb.borderWidth = info.border_width;
	(void) XtSetGeometryHandler(dpy, w, (XtGeometryHandler) FormGeometryHandler);
	(void) XtSetEventHandler(dpy, w, (XtEventHandler)SubEventHandler,
				 StructureNotifyMask, (caddr_t)sub);
    }
    subglob = *sub;
    XtGetResources(dpy, subresources, XtNumber(subresources), args, argCount,
		   w, "ignore", "Ignore", &names, &classes);
    *sub = subglob;
    XrmFreeNameList(names);
    XrmFreeClassList(classes);
    RefigureLocations(data);
    XMapWindow(data->dpy, w);
}


/*
 * Remove a subwidget from a form.  Doesn't delete the subwidget. 
 */

void XtFormRemoveWidget(dpy, mywin, w)
Display *dpy;
Window mywin, w;
{
    WidgetData data;
    SubWidgetData sub;
    Boolean found;
    int i;
    if ((data = DataFromWindow(dpy, mywin)) == NULL ||
	(sub = SubFromWindow(dpy, w)) == NULL) return;
    found = FALSE;
    for (i=0 ; i<data->numsub ; i++) {
	if (data->sub[i]->fromhoriz == w)
	    data->sub[i]->fromhoriz = sub->fromhoriz;
	if (data->sub[i]->fromvert == w)
	    data->sub[i]->fromvert = sub->fromvert;
	if (!found) found = (data->sub[i] == sub);
	if (found) data->sub[i] = data->sub[i+1];
    }
    if (found) (data->numsub)--;
    XtSetEventHandler(dpy, w, (XtEventHandler) SubEventHandler,
		      (unsigned long) 0, (caddr_t) NULL);
    (void) XtClearGeometryHandler(dpy, w);
    (void) XDeleteContext(dpy, w, subContext);
    XtFree((char *)sub);
}
    


/* 
 * Set or reset figuring
 */

void XtFormDoLayout(dpy, mywin, doit)
Display *dpy;
Window mywin;
Boolean doit;
{
    WidgetData data;
    if ((data = DataFromWindow(dpy, mywin)) == NULL)
	return;
    if (doit && data->dontrefigure > 0) data->dontrefigure--;
    else data->dontrefigure++;
    if (data->needsrefigure) RefigureLocations(data);
}
