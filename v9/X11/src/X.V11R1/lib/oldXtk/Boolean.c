/* $Header: Boolean.c,v 1.2 87/09/12 12:41:35 swick Exp $ */
#ifndef lint
static char *sccsid = "@(#)Boolean.c	1.8	2/24/87";
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
#include "Boolean.h"
#include "Atoms.h"

/* Private Definitions */

typedef void (*NotifyProc)();

typedef struct _WidgetDataRec {
    Display	*dpy;		/* widget display connection */
    Window	window;		/* widget window */
    Position	x, y;		/* location of widget */
    Dimension	borderWidth;	/* border width in pixels */
    Dimension	 width, height;	/* width/height in pixels */
    int		ibw, ibh;	/* internal border width/height in pixels */
    char	*text;		/* button text */
    int		fgpixel;	/* color index for text */
    int		bgpixel;	/* color index for background */
    int		brpixel;	/* color for border */
    Boolean	highlighted;	/* are we highlighted? */
    Boolean	value;		/* pointer to value */
    XFontStruct	*fontstruct;	/* font for text */
    Dimension	twidth, theight;/* text width/height */
    XtJustify  justify;		/* text justification */
    NotifyProc	proc;		/* procedure to invoke on value change */
    caddr_t	tag;		/* widget client data */
    XtEventsPtr eventTable;	/* Table for Translation Manager */
    caddr_t	state;		/* state for Translation Manager */
    int		eventlevels;	/* recursion levels of event handling */
    GC		gc;		/* current gc */
    GC		onGC;		/* GC to use when on */
    GC		offGC;		/* GC to use when off */
} WidgetDataRec, *WidgetData;

extern void Dummy(); 
static WidgetDataRec globaldata;
static WidgetDataRec globalinit = {

    NULL,		/* Display dpy; */
    NULL,		/* Window window; */
    0,0,		/* x,y */
    1,			/* int borderWidth; */
    0, 0,		/* int width, height; */
    4, 2,		/* int ibw, ibh; */
    NULL,		/* char *text; */
    NULL,		/* int fgpixel; */ /* init proc */
    NULL,		/* int bgpixel; */ /* init proc */
    NULL,		/* Pixmap brpixmap; */ /* init proc */
    FALSE,		/* Boolean highlighted; */
    FALSE, 	/* Boolean value; */
    NULL,		/* XFontStruct *fontstruct; */ /* init proc */
    0, 0,		/* int twidth, theight; */
    XtjustifyCenter,	/* justify */
    Dummy,		/* void (*proc) (); */
    NULL,		/* caddr_t tag; */
    NULL,		/* event table pointer */
    NULL,		/* state for Translation Manager */
    NULL,		/* event levels */
    NULL,		/* gc */
    NULL,		/* onGC */
    NULL,		/* offGC */
};

/* Private Data */

static int defaultBorderWidth 		= 1;
static int defaultInternalBorderWidth 	= 4;
static int defaultInternalBorderHeight 	= 2;
static XtJustify defaultJustify 	= XtjustifyCenter;
static NotifyProc defaultFunction	= Dummy;

static Resource resources[] = {
    {XtNvalue,		XtCValue,	XrmRBoolean,	sizeof(Boolean),
        (caddr_t)&globaldata.value,	(caddr_t)NULL},
    {XtNwindow,		XtCWindow,	XrmRWindow,	sizeof(Window), 
	(caddr_t)&globaldata.window,	(caddr_t)NULL},
    {XtNborderWidth,	XtCBorderWidth,	XrmRInt,		sizeof(int), 
	(caddr_t)&globaldata.borderWidth, (caddr_t)&defaultBorderWidth},
    {XtNwidth,		XtCWidth,	XrmRInt,		sizeof(int), 
	(caddr_t)&globaldata.width,	(caddr_t)NULL},
    {XtNheight,		XtCHeight,	XrmRInt,		sizeof(int), 
	(caddr_t)&globaldata.height,	(caddr_t)NULL},
    {XtNinternalWidth,	XtCWidth,	XrmRInt,		sizeof(int), 
	(caddr_t)&globaldata.ibw,	(caddr_t)&defaultInternalBorderWidth},
    {XtNinternalHeight,	XtCHeight,	XrmRInt,		sizeof(int), 
	(caddr_t)&globaldata.ibh,	(caddr_t)&defaultInternalBorderHeight},
    {XtNlabel,		XtCLabel,	XrmRString,	sizeof(char *), 
	(caddr_t)&globaldata.text,	(caddr_t)NULL},
    {XtNforeground,	XtCColor,	XrmRPixel,	sizeof(int), 
	(caddr_t)&globaldata.fgpixel,	(caddr_t)&XtDefaultFGPixel},
    {XtNbackground,	XtCColor,	XrmRPixel,	sizeof(int), 
	(caddr_t)&globaldata.bgpixel,	(caddr_t)&XtDefaultBGPixel},
    {XtNborder,		XtCColor,	XrmRPixel,	sizeof(int),
	(caddr_t)&globaldata.brpixel,	(caddr_t)&XtDefaultFGPixel},
    {XtNfont,		XtCFont,	XrmRFontStruct,	sizeof(XFontStruct *),
	(caddr_t)&globaldata.fontstruct,(caddr_t)NULL},
    {XtNjustify,	XtCJustify,	XtRJustify,	sizeof(XtJustify), 
        (caddr_t)&globaldata.justify,	(caddr_t)&defaultJustify},
    {XtNfunction,	XtCFunction,	XtRFunction,	sizeof(NotifyProc), 
	(caddr_t)&globaldata.proc,	(caddr_t)&defaultFunction},
    {XtNparameter,	XtCParameter,	XrmRPointer,	sizeof(caddr_t), 
	(caddr_t)&globaldata.tag,	(caddr_t)NULL},
    {XtNeventBindings,	XtCEventBindings,XtREventBindings, sizeof(caddr_t), 
	(caddr_t)&globaldata.eventTable,NULL },
};

static char *defaultEventBindings[] = {
	"<EnterWindow>:		highlight\n",
	"<LeaveWindow>:		unhighlight\n",
	"<ButtonPress>left:	toggle\n",
	"<ButtonRelease>left:	notify\n",
	NULL
};

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

static Boolean initialized = FALSE; /* !!! STATIC !!! */

static XContext widgetContext;

static void BooleanInitialize (dpy)
 Display *dpy;
{
    if (initialized)
    	return;
    initialized = TRUE;

    widgetContext = XUniqueContext();

    globalinit.fontstruct = XLoadQueryFont(dpy,"fixed");
    globalinit.justify = XtjustifyCenter;
    globalinit.eventTable = XtParseEventBindings(defaultEventBindings);
}

static void SetTextWidthAndHeight(data)
WidgetData data;
{
	data->theight = data->fontstruct->max_bounds.ascent +
		data->fontstruct->max_bounds.descent;
	data->twidth = XTextWidth(data->fontstruct, data->text,
				  strlen(data->text));
}

/*ARGSUSED*/
static void Dummy(p)
caddr_t p;
{
    (void) printf("dummy notify for Boolean\n");
}

static void Ignore ()
{
   (void) printf("you are being ignored\n");
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
 * Repaint the widget window
 *
 */

static void Redisplay (data)
WidgetData	data;
{
    int     textx, texty;
    Window  w = data->window;


    /*
     * Calculate Text x,y given window width and text width
     * to the specified justification
     */

    if (data->justify == XtjustifyLeft) 
	textx = 2;
    else if (data->justify == XtjustifyRight)
	textx = data->width - data->twidth;
    else
        textx = ((data->width - data->twidth) / 2);
    if (textx < 0) textx = 0;
    texty = (data->height - data->theight) / 2 +
	data->fontstruct->max_bounds.ascent;

    XFillRectangle(data->dpy, w,
	(data->highlighted ? data->onGC : data->offGC),
	0, 0, data->width, data->height);

    XDrawImageString(
	data->dpy, w, (((data->value)) ? data->offGC : data->onGC),
	textx, texty, data->text, strlen(data->text));

}

extern void Destroy();

/*
 *
 * Generic widget event handler
 *
 */

static XtEventReturnCode EventHandler(event, eventdata)
XEvent *event;
caddr_t eventdata;
{
    WidgetData		data = (WidgetData) eventdata;
    XtActionTokenPtr	actionList;
    NotifyProc		proc;

  data->eventlevels++;

    switch (event->type) {
	case ConfigureNotify:
	    data->width = event->xconfigure.width;
	    data->height = event->xconfigure.height;
	    data->borderWidth = event->xconfigure.border_width;
	    break;

        case DestroyNotify: Destroy(data); break;

        case Expose:
	    if (event->xexpose.count == 0)
		Redisplay(data);
	    break;

	default:
	    actionList = (XtActionTokenPtr)XtTranslateEvent(
		event, (TranslationPtr) data->state);
	    for (; actionList != NULL; actionList = actionList->next) {
 		if (actionList->type == XttokenAction) {
		    proc = (NotifyProc)XtInterpretAction(
		         data->dpy, 
			(TranslationPtr) data->state,
			actionList->value.action);
		    (*proc) (data);
		}
	    }
	    break;
    }

    data->eventlevels--;
    return (XteventHandled);
}

/*
 *
 * Widget hilight event handler
 *
 */

/*ARGSUSED*/
static XtEventReturnCode Highlight(data)
    WidgetData data;
{
    data->highlighted = TRUE;
    Redisplay(data);
    return (XteventHandled);
}

/*
 *
 * Widget un-hilight event handler
 *
 */

/*ARGSUSED*/
static XtEventReturnCode Unhighlight(data)
    WidgetData data;
{
    data->highlighted = FALSE;
    Redisplay(data);
    return (XteventHandled);
}

static void SetValue(data, newval)
    WidgetData data;
    Boolean    newval;
{
    if ((data->value) == newval) return;

    (data->value) = newval;
    data->gc = (newval ? data->onGC : data->offGC);
/*
    XSetWindowBackground(data->dpy, data->window, data->gc->value.background);
*/
    Redisplay(data);
}

/*
 *
 * Widget set value event handler
 *
 */

/*ARGSUSED*/
static XtEventReturnCode On(event, eventdata)
XEvent *event;
caddr_t eventdata;
{
    WidgetData	data = (WidgetData) eventdata;

    SetValue(data, TRUE);
    return (XteventHandled);
}

/*
 *
 * Widget un-set value event handler
 *
 */

/*ARGSUSED*/
static XtEventReturnCode Off(event, eventdata)
XEvent *event;
caddr_t eventdata;
{
    WidgetData	data = (WidgetData) eventdata;

    SetValue(data, FALSE);
    return (XteventHandled);
}

/*
 *
 * Widget toggle value event handler
 *
 */

/*ARGSUSED*/
static XtEventReturnCode Toggle(data)
    WidgetData data;
{
    SetValue(data, ! (data->value));
    return (XteventHandled);
}

/*
 * Widget handler to invoke application routine */

/*ARGSUSED*/
static XtEventReturnCode Notify(data)
    WidgetData data;
{
	Redisplay(data);
	XFlush(data->dpy);
	data->proc(data->tag); /* invoke application proc */
	
	return(XteventHandled);
}


/*
 *
 * Destroy the widget
 *
 */

static void Destroy(data)
WidgetData	data;
{
    (void) XDeleteContext(data->dpy, data->window, widgetContext);
    XtClearEventHandlers(data->dpy, data->window);
    XtFree ((char*)data->text);
    XtFree ((char *) data);
}

/****************************************************************
 *
 * Public Procedures
 *
 ****************************************************************/

Window XtBooleanCreate(dpy, parent, args, argCount)
    Display  *dpy;
    Window   parent;
    ArgList  args;
    int      argCount;
{
    WidgetData	data;
    XrmNameList	names;
    XrmClassList classes;
    Position x, y;
    unsigned int depth;
    Drawable	root;
    unsigned long valuemask;
    XSetWindowAttributes wvals;
    XGCValues values;

    static XtActionsRec actionsTable[] = {
	{"toggle",	(caddr_t)Toggle},
	{"highlight",	(caddr_t)Highlight},
	{"unhighlight",	(caddr_t)Unhighlight},
	{"notify",	(caddr_t)Notify},
	{"on", 		(caddr_t)On},
	{"off",		(caddr_t)Off},
	{NULL, NULL}
    };


   if (!initialized)
   	BooleanInitialize(dpy);

   data = (WidgetData) XtMalloc (sizeof(WidgetDataRec));

    /* Set Default Values */
    globaldata = globalinit;
    globaldata.dpy = dpy;
    XtGetResources(dpy, resources, XtNumber(resources), args, argCount, parent,
        "boolean", "Boolean", &names, &classes);
    *data = globaldata;

    data->state = XtSetActionBindings(
        data->dpy,
	data->eventTable, actionsTable, (caddr_t)Ignore);

    if (data->text == NULL)
	data->text = XrmNameToAtom(names[XrmNameListLength(names) - 1]);
    data->text = strcpy( XtMalloc ((unsigned) strlen(data->text) + 1),
                        data->text);

    /* obtain text dimensions and calculate the window size */
    SetTextWidthAndHeight(data);
    if (data->width == 0) data->width = data->twidth + 2*data->ibw;
    if (data->height == 0) data->height = data->theight + 2*data->ibh;

    wvals.background_pixel = data->bgpixel;
    wvals.border_pixel = data->brpixel;
    wvals.bit_gravity = CenterGravity;
    
    valuemask = CWBackPixel | CWBorderPixel | CWBitGravity;
    
    if (data->window != NULL) {
	/* set global data from window parameters */
	if (
	    XGetGeometry(
	        data->dpy, data->window, &root,
		&x, &y, &(data->width), &(data->height),
		&(data->borderWidth), &depth)
           ) {
	    XReparentWindow(data->dpy, data->window, parent, data->x,data->y);
	    XChangeWindowAttributes(data->dpy, data->window, valuemask, &wvals);
	} else
	    data->window = NULL;
    }
    if (data->window == NULL)
	data->window = XCreateWindow(data->dpy, parent, data->x, data->y,
			 data->width, data->height, data->borderWidth,
			 0, (unsigned) InputOutput, (Visual *) CopyFromParent,
			 valuemask, &wvals);

    values.foreground = data->fgpixel;
    values.background = data->bgpixel;
    values.font = data->fontstruct->fid;
    valuemask = GCForeground | GCBackground | GCFont;
    data->onGC = XtGetGC(data->dpy, widgetContext, data->window,
			 valuemask, &values);
    values.foreground = data->bgpixel;
    values.background = data->fgpixel;
    data->offGC = XtGetGC(data->dpy, widgetContext, data->window,
			  valuemask, &values);

    XtSetNameAndClass(data->dpy, data->window, names, classes);
    XrmFreeNameList(names);
    XrmFreeClassList(classes);

    (void)XSaveContext(data->dpy, data->window, widgetContext, (caddr_t)data);

    /* set handler for expose, resize, and message events */
    XtSetEventHandler (data->dpy, data->window, (XtEventHandler) EventHandler,
     StructureNotifyMask | ExposureMask | ButtonPressMask | ButtonReleaseMask
     | EnterWindowMask | LeaveWindowMask, (caddr_t)data);

    return (data->window);
}

/*
 * Set specified arguments in widget
 */

void XtBooleanSetValues (dpy, window, args, argCount)
    Display	*dpy;
    Window	window;
    ArgList	args;
    int		argCount;
{
    WidgetData	data;
    data = DataFromWindow(dpy, window);
    if (data == NULL) return;

    globaldata = *data;
    XtSetValues(resources, XtNumber(resources), args, argCount);

    (globaldata.value) = ((globaldata.value) ? TRUE : FALSE);

    if (strcmp (data->text, globaldata.text)
	  || data->fontstruct != globaldata.fontstruct) {
	XtGeometryReturnCode reply;
	WindowBox reqbox, replybox;

	globaldata.text = strcpy(
	    XtMalloc ((unsigned) strlen(globaldata.text) + 1),
	    globaldata.text);
        XtFree ((char *) data->text);

	/* obtain text dimensions and calculate the window size */
	SetTextWidthAndHeight(&globaldata);
	reqbox.width = (int) globaldata.twidth + 2*globaldata.ibw;
	reqbox.height = (int) globaldata.theight + 2*globaldata.ibh;
	reply = XtMakeGeometryRequest(
		globaldata.dpy, globaldata.window,
		XtgeometryResize, &reqbox, &replybox);
	if (reply == XtgeometryAlmost) {
	    reqbox = replybox;
	    (void) XtMakeGeometryRequest(
		globaldata.dpy, globaldata.window,
		XtgeometryResize, &reqbox, &replybox);
	}
    }

    *data = globaldata;
    Redisplay (data);
}

/*
 * Get specified arguments from widget
 */

void XtBooleanGetValues (dpy, window, args, argCount)
    Display	*dpy;
    Window	window;
    ArgList	args;
    int		argCount;
{
    WidgetData	data;
    data = DataFromWindow(dpy, window);
    if (data == NULL) return;

    globaldata = *data;
    XtGetValues(resources, XtNumber(resources), args, argCount);
}

