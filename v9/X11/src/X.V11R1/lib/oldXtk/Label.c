/* $Header: Label.c,v 1.2 87/09/12 12:43:01 swick Exp $ */
#ifndef lint
static char *sccsid = "@(#)Label.c	1.15	2/25/87";
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


#include <stdio.h>
#include <strings.h>
#include "Xlib.h"
#include "Intrinsic.h"
#include "Label.h"
#include "Atoms.h"

/* Private Definitions */


typedef struct {
    Display	*dpy;		/* widget display connection	      */
    Window	window;   	/* widget window		      */
    Dimension	borderWidth;	/* border width in pixels	      */
    Dimension	width;		/* window width in pixels	      */
    Dimension	height;	/* window height in pixels	      */
    Dimension	internalWidth;	/* internal border width in pixels    */
    Dimension	internalHeight;	/* internal border height in pixels   */
    String	label;		/* button text to display	      */
    int		labelLength;	/* number of non-NULL chars in label  */
    Pixel	foreground;
    Pixel	background;
    Pixel	border;
    XFontStruct	*fontStruct;	/* label font			      */
    Dimension	labelWidth;	/* label width in pixels	      */
    Dimension	labelHeight;	/* label height in pixels	      */
    XtJustify	justify;	/* left, right, or center	      */
    GC		gc;		/* pointer to GraphicsContext	      */
} WidgetDataRec, *WidgetData;

static WidgetDataRec globaldata;


/* Private Data */

static caddr_t		defNULL	  	  = (caddr_t) NULL;
static Dimension	defBorderWidth    = 1;
static Dimension	defInternalWidth  = 4;
static Dimension	defInternalHeight = 2;
static XFontStruct	*defFontStruct;
static XtJustify	defJustify	  = XtjustifyCenter;

static Resource resources[] = {
    {XtNwindow,		XtCWindow,	XrmRWindow,	sizeof(Window),
	(caddr_t) &globaldata.window,		(caddr_t) &defNULL},
    {XtNborderWidth,	XtCBorderWidth,	XrmRInt,		sizeof(Dimension),
	(caddr_t) &globaldata.borderWidth,	(caddr_t) &defBorderWidth},
    {XtNwidth,		XtCWidth,	XrmRInt,		sizeof(Dimension), 
	(caddr_t) &globaldata.width,		(caddr_t) &defNULL},
    {XtNheight,		XtCHeight,	XrmRInt,		sizeof(Dimension), 
	(caddr_t) &globaldata.height,		(caddr_t) &defNULL},
    {XtNinternalWidth,	XtCWidth,	XrmRInt,		sizeof(Dimension),
	(caddr_t) &globaldata.internalWidth,	(caddr_t) &defInternalWidth},
    {XtNinternalHeight,	XtCHeight,	XrmRInt,		sizeof(Dimension),
	(caddr_t) &globaldata.internalHeight,	(caddr_t) &defInternalHeight},
    {XtNlabel,		XtCLabel,	XrmRString,	sizeof(String),
	(caddr_t) &globaldata.label,		(caddr_t) &defNULL},
    {XtNforeground,	XtCColor,	XrmRPixel,	sizeof(Pixel), 
	(caddr_t) &globaldata.foreground, 	(caddr_t) &XtDefaultFGPixel},
    {XtNbackground,	XtCColor,	XrmRPixel,	sizeof(Pixel),
	(caddr_t) &globaldata.background, 	(caddr_t) &XtDefaultBGPixel},
    {XtNborder,		XtCColor,	XrmRPixel,	sizeof(Pixel),
	(caddr_t) &globaldata.border,		(caddr_t) &XtDefaultFGPixel},
    {XtNfont,		XtCFont,	XrmRFontStruct,	sizeof(XFontStruct *),
	(caddr_t) &globaldata.fontStruct,	(caddr_t) &defFontStruct},
    {XtNjustify,	XtCJustify,	XtRJustify,	sizeof(XtJustify),
 	(caddr_t) &globaldata.justify,		(caddr_t) &defJustify},
};


/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

static Boolean initialized = FALSE;

static XContext widgetContext;

static void LabelInitialize(dpy)
Display *dpy;
{
    if (initialized)
    	return;
    initialized = TRUE;

    widgetContext = XUniqueContext();
    defFontStruct = XLoadQueryFont(dpy,"fixed");
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
 * Calculate width and height of displayed text in pixels
 */

static void SetTextWidthAndHeight(data)
    WidgetData data;
{
    register XFontStruct	*fs = data->fontStruct;

    data->labelHeight = fs->max_bounds.ascent + fs->max_bounds.descent;
    data->labelWidth = XTextWidth(fs, data->label, data->labelLength);
}


/*
 * Repaint the widget window
 */

static void Redisplay(data)
    WidgetData data;
{
    Position x, y;

    /* Calculate text position within window given window width and height  */
    switch (data->justify) {
	case XtjustifyLeft   :
	    x = data->internalWidth;
	    break;
	case XtjustifyRight  :
	    x = data->width - data->labelWidth - data->internalWidth;
	    break;
	case XtjustifyCenter :
	    x = (data->width - data->labelWidth) / 2;
	    break;
    }
    if (x < 0) x = 0;
    y = (data->height - data->labelHeight) / 2
        + data->fontStruct->max_bounds.ascent;

   XDrawString(data->dpy, data->window, data->gc, x, y,
       data->label, data->labelLength);
}


/*
 * Fill in the gc field
 */

static void GetGC(data)
    WidgetData  data;
{
    XGCValues	values;

    values.foreground	= data->foreground;
    values.font		= data->fontStruct->fid;
    values.background	= data->background;
    data->gc = XtGetGC(data->dpy, (XContext) widgetContext, data->window,
    	GCForeground | GCFont | GCBackground, &values);
}


/*
 * Widget event handlers
 */

static XtEventReturnCode HandleExpose(event, eventdata)
    XEvent  *event;
    caddr_t eventdata;
{
    if (event->xexpose.count == 0)
	Redisplay((WidgetData) eventdata);
    return XteventHandled;
}


/*
 * ||| Kludge as long as X events are clumped together
 */

static XtEventReturnCode HandleStructureNotify(event, eventdata)
    XEvent  *event;
    caddr_t eventdata;
{
    register WidgetData data = (WidgetData) eventdata;

    switch (event->type) {
	case ConfigureNotify :
	    data->height = event->xconfigure.height;
	    data->width  = event->xconfigure.width;
	    return XteventHandled;

	case DestroyNotify   :
	    (void) XDeleteContext(data->dpy, data->window, widgetContext);
	    XtClearEventHandlers(data->dpy, data->window);
/* |||	    XtFreeGC(data->gc); */
	    XtFree(data->label);
	    XtFree ((char *) data);
	    return XteventHandled;
    }
    return XteventNotHandled;
}


/****************************************************************
 *
 * Public Procedures
 *
 ****************************************************************/

Window XtLabelCreate(dpy, parent, args, argCount)
    Display  *dpy;
    Window   parent;
    ArgList  args;
    int      argCount;
{
    register WidgetData	 data;
    	     XrmNameList  names;
    	     XrmClassList classes;

    if (!initialized)
    	LabelInitialize (dpy);

    data = (WidgetData ) XtMalloc (sizeof (WidgetDataRec));

    /* Set resource values */
    globaldata.dpy = dpy;
    XtGetResources(dpy, resources, XtNumber(resources), args, argCount, parent,
	"label", "Label", &names, &classes);
    *data = globaldata;    


    /* Set text label to widget name if needed */
    if (data->label == NULL)
	data->label = XrmNameToAtom(names[XrmNameListLength(names) - 1]);
    data->labelLength = strlen(data->label);
    data->label = strcpy( XtMalloc ((unsigned) data->labelLength + 1),
                        data->label);

    /* obtain text dimensions and calculate the window size */
    SetTextWidthAndHeight(data);
    if (data->width == 0)
        data->width = data->labelWidth + 2 * data->internalWidth;
    if (data->height == 0)
        data->height = data->labelHeight + 2 * data->internalHeight;

    if (data->window != NULL) {
	XWindowAttributes wi;
	/* set global data from window parameters */
	if (! XGetWindowAttributes(data->dpy, data->window, &wi)) {
	    data->window = NULL;
	} else {
	    data->borderWidth = wi.border_width;
	    data->width       = wi.width;
	    data->height      = wi.height;
	}
    }
    if (data->window == NULL) {
#ifdef notdef
	int gravity;
	switch(data->justify) {
	    case XtjustifyLeft   : gravity = WestGravity;	  break;
	    case XtjustifyRight  : gravity = EastGravity;   break;
	    case XtjustifyCenter : gravity = CenterGravity; break;
	}
#endif
	/* !!! put window gravity into window !!! */
	/* create the Label button window */
        data->window = XCreateSimpleWindow(data->dpy,parent, 0, 0, 
	    data->width, data->height,
	    data->borderWidth, data->border, data->background);
    }

    XtSetNameAndClass(data->dpy, data->window, names, classes);
    XrmFreeNameList(names);
    XrmFreeClassList(classes);

    /* Create a graphics context */
    GetGC(data);

    (void)XSaveContext(data->dpy, data->window, widgetContext, (caddr_t)data);

    /* Set handlers for expose, resize, destroy, and message events */
    XtSetEventHandler (data->dpy, data->window, (XtEventHandler)HandleStructureNotify,
        StructureNotifyMask, (caddr_t)data);
    XtSetEventHandler (data->dpy, data->window, (XtEventHandler)HandleExpose,
	ExposureMask, (caddr_t)data);

    return (data->window);
}


/*
 * Get specified arguments from widget
 */

void XtLabelGetValues(dpy, window, args, argCount)
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


/*
 * Set specified arguments into widget
 */

void XtLabelSetValues(dpy, window, args, argCount)
Display *dpy;
Window window;
ArgList	args;
int argCount;
{
    WidgetData	data;
    data = DataFromWindow(dpy, window);
    if (data == NULL) return;
    globaldata = *data;
    XtSetValues(resources, XtNumber(resources), args, argCount);

    if (strcmp(data->label, globaldata.label) != 0
	  || data->fontStruct != globaldata.fontStruct) {
	XtGeometryReturnCode reply;
	WindowBox reqbox, replybox;

	globaldata.labelLength = strlen(globaldata.label);
	globaldata.label = strcpy(
	    XtMalloc ((unsigned) globaldata.labelLength + 1),
	    globaldata.label);
        XtFree ((char *) data->label);

	/* obtain text dimensions and calculate the window size */
	SetTextWidthAndHeight(&globaldata);
	reqbox.width = globaldata.labelWidth + 2*globaldata.internalWidth;
	reqbox.height = globaldata.labelHeight + 2*globaldata.internalHeight;
	reply = XtMakeGeometryRequest(globaldata.dpy, globaldata.window, XtgeometryResize, 
				     &reqbox, &replybox);
	if (reply == XtgeometryAlmost) {
	    reqbox = replybox;
	    reply = XtMakeGeometryRequest(globaldata.dpy, globaldata.window, XtgeometryResize, 
				 &reqbox, &replybox);
	}
	if (reply == XtgeometryYes) {
	    globaldata.width = reqbox.width;
	    globaldata.height = reqbox.height;
	}
    }

    if (data->foreground != globaldata.foreground
    	|| data->background != globaldata.background
	|| data->fontStruct->fid != globaldata.fontStruct->fid) {
	/* Need new graphics context */
	/* ||| XtFreeGC(data->gc) */
	GetGC(&globaldata);
    }
    XClearWindow(data->dpy, data->window);
    *data = globaldata;
    Redisplay (data);
}
