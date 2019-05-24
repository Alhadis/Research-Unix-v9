/* $Header: Command.c,v 1.3 87/09/12 12:42:17 swick Exp $ */
#ifndef lint
static char *sccsid = "@(#)Command.c	1.13	2/25/87";
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
#include "Command.h"
#include "Atoms.h"

/* Private Definitions */

typedef void  (*CommandProc)();
        
typedef struct _WidgetDataRec {
    Display	*dpy;		/* widget display connection */
    Window	mywin;          /* widget window */
    Position	x, y;		/* Location of widget. */
    Dimension	borderWidth;	/* border width in pixels */
    Dimension	width, height;	/* width/height in pixels */
    Dimension	ibw, ibh;	/* internal border width/height in pixels */
    char	*text;		/* button text */
    Pixel	fgpixel;	/* color index for text */
    Pixel	bgpixel;	/* color index for background */
    Pixel	brpixel;	/* color for border */
    Boolean	sensitive;	/* user actions processed */
    Boolean	highlighted;	/* are we border highlighted? */
    Boolean	set;		/* are we text highlighted */
    Boolean	notifying;	/* are we in the notify proc? */
    Boolean	needsfreeing;	/* were we destroyed by the notify proc? */
    int		eventlevels;	/* recursion levels of event processing */
    XFontStruct	*fontstruct;	/* font for text */
    Dimension	twidth, theight;/* text width/height */
    XtJustify	justify;	/* text justification */
    CommandProc	proc;		/* command procedure to invoke on notify */
    caddr_t	tag;		/* widget client data */
    GC		normgc;		/* gc for normal display */
    GC		invgc;		/* gc for inverted display */
    GC		graygc;		/* gc for grayed-out text */
    XtEventsPtr eventTable;     /* Table for translation manager. */
    TranslationPtr state;	/* Translation manager state handle. */
} WidgetDataRec, *WidgetData;

static WidgetDataRec globaldata;
extern void Dummy();
static WidgetDataRec globalinit = {
    NULL,		/* Display dpy; */
    NULL,		/* Window mywin; */
    0, 0,		/* Position x, y; */
    1,			/* Dimension borderWidth; */
    0, 0,		/* Dimension width, height; */
    4, 2,		/* Dimension ibw, ibh; */
    NULL,		/* char *text; */
    NULL,		/* Pixel fgpixel; */ /* init proc */
    NULL,		/* Pixel bgpixel; */ /* init proc */
    NULL,		/* Pixel brpixel; */ /* init proc */
    TRUE,		/* int sensitive; */
    FALSE,		/* int highlighted; */
    FALSE,		/* int set; */
    FALSE,		/* int notifying; */
    FALSE,		/* int needsfreeing; */
    0,			/* int eventlevels; */
    NULL,		/* XFontStruct *fontstruct; */ /* init proc */
    0, 0,		/* int twidth, theight; */
    XtjustifyCenter,      /* text justification */
    Dummy,		/* void (*Proc) (); */
    NULL,		/* caddr_t tag; */
    NULL,		/* GC normgc; */
    NULL,		/* GC invgc; */
    NULL,		/* GC graygc; */
    NULL,               /* XtEventsPtr eventTable; */ /* init proc */
    NULL,               /* caddr_t state; */
};

/* Private Data */

static Resource resources[] = {
    {XtNwindow, XtCWindow, XrmRWindow,
	sizeof(Window), (caddr_t)&globaldata.mywin, (caddr_t)NULL},
    {XtNx, XtCX, XrmRInt,
	 sizeof(int), (caddr_t)&globaldata.x, (caddr_t)NULL},
    {XtNy, XtCY, XrmRInt,
	 sizeof(int), (caddr_t)&globaldata.y, (caddr_t)NULL},
    {XtNborderWidth, XtCBorderWidth, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.borderWidth, (caddr_t)NULL},
    {XtNwidth, XtCWidth, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.width, (caddr_t)NULL},
    {XtNheight, XtCHeight, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.height, (caddr_t)NULL},
    {XtNinternalWidth, XtCWidth, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.ibw, (caddr_t)NULL},
    {XtNinternalHeight, XtCHeight, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.ibh, (caddr_t)NULL},
    {XtNlabel, XtCLabel, XrmRString,
	sizeof(char *), (caddr_t)&globaldata.text, (caddr_t)NULL},
    {XtNforeground, XtCColor, XrmRPixel,
	sizeof(int), (caddr_t)&globaldata.fgpixel, (caddr_t)&XtDefaultFGPixel},
    {XtNbackground, XtCColor, XrmRPixel,
	sizeof(int), (caddr_t)&globaldata.bgpixel, (caddr_t)&XtDefaultBGPixel},
    {XtNborder, XtCColor, XrmRPixel, sizeof(int),
	(caddr_t)&globaldata.brpixel, (caddr_t)&XtDefaultFGPixel},
    {XtNfont, XtCFont, XrmRFontStruct,
	sizeof(XFontStruct *), (caddr_t)&globaldata.fontstruct, (caddr_t)NULL},
    {XtNjustify, XtCJustify, XtRJustify,
        sizeof(XtJustify), (caddr_t)&globaldata.justify, (caddr_t)NULL},
    {XtNfunction, XtCFunction, XtRFunction,
	sizeof(CommandProc), (caddr_t)&globaldata.proc, (caddr_t)NULL},
    {XtNparameter, XtCParameter, XrmRPointer,
	sizeof(caddr_t), (caddr_t)&globaldata.tag, (caddr_t)NULL},
    {XtNsensitive, XtCBoolean, XrmRBoolean, sizeof(Boolean),
	 (caddr_t)&globaldata.sensitive, NULL},
    {XtNeventBindings, XtCEventBindings, XtREventBindings, sizeof(caddr_t),
        (caddr_t)&globaldata.eventTable, NULL},
};
static char *defaultEventBindings[] = {
    "<ButtonPress>left:       set\n",
    "<ButtonRelease>left:      notify unset\n",
    "<EnterWindow>:             highlight\n",
    "<LeaveWindow>:             unhighlight unset\n",
    NULL
};

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

/* !!! STATIC !!! */
static Boolean initialized = FALSE;       /* initialization flag */

static XContext widgetContext;

static void CommandInitialize () 
{
    if (initialized)
    	return;
    initialized = TRUE;
    widgetContext = XUniqueContext();

    globalinit.eventTable = XtParseEventBindings(defaultEventBindings);
}

/*ARGSUSED*/
static void Dummy(p)                  /* default call back proc */
caddr_t p;
{
    (void) printf("dummy call back for Command\n");
}

static void Ignore()
{}


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
 * Calculate how wide and high the text will be when displayed.
 */

static void SetTextWidthAndHeight(data)
WidgetData data;
{
    data->theight = data->fontstruct->max_bounds.ascent +
	data->fontstruct->max_bounds.descent;
    data->twidth = XTextWidth(data->fontstruct, data->text,
			      strlen(data->text));
}


/*
 * Build the gc's for the widget.
 */

#define gray_width	32
#define gray_height	32

static BuildGcs(data)
WidgetData data;
{
    unsigned long valuemask;
    XGCValues values;
    Pixmap gray;
    static short gray_bits[] = {
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555,
	0xaaaa, 0xaaaa, 0x5555, 0x5555
    };
    if (XFindContext(data->dpy, (Window) NULL, widgetContext,
		      (caddr_t *)&gray)) {
	XImage ximage;
	GC pgc;
	XGCValues gcv;

	gcv.foreground = 1;
	gcv.background = 0;
  	gray = XCreatePixmap(data->dpy, data->mywin,
			     gray_width, gray_height, 
			     DefaultDepth(data->dpy, DefaultScreen(data->dpy)));

/*
    Instead of calling the GCManager, since it doesn't handle depths,
    and stipples are always XYBitmap, depth == 1, use XCreateGC
    directly.
*/
        pgc = XCreateGC(data->dpy, gray, GCForeground | GCBackground, &gcv);

/*
	pgc = XtGetGC(data->dpy, widgetContext, gray,
		      GCForeground | GCBackground, &gcv);
*/
	ximage.height = gray_height;
	ximage.width = gray_width;
	ximage.xoffset = 0;
	ximage.format = XYBitmap;
	ximage.data = (char *)gray_bits;
	ximage.byte_order = ImageByteOrder(data->dpy);
	ximage.bitmap_unit = BitmapUnit(data->dpy); 
	ximage.bitmap_bit_order = BitmapBitOrder(data->dpy);
	ximage.bitmap_pad = BitmapPad(data->dpy);
	ximage.bytes_per_line = gray_width / 8;
	ximage.depth = 1;

	XPutImage(data->dpy, gray, pgc, &ximage, 0, 0, 0, 0,
		  gray_width, gray_height);
	(void)XSaveContext(data->dpy, (Window) NULL, widgetContext,
			    (caddr_t)gray);
        XFreeGC(data->dpy, pgc);    /* if the GCManager used, don't do this */
    }

    valuemask = GCForeground | GCBackground | GCFont | GCFillStyle;
    values.font = data->fontstruct->fid;
    values.foreground = data->bgpixel;
    values.background = data->fgpixel;
    values.fill_style = FillSolid;
    data->invgc = XtGetGC(data->dpy, widgetContext, data->mywin,
			  valuemask, &values);
    values.foreground = data->fgpixel;
    values.background = data->bgpixel;
    data->normgc = XtGetGC(data->dpy, widgetContext, data->mywin,
			   valuemask, &values);
    valuemask |= GCStipple;
    values.fill_style = FillStippled;
    values.stipple = gray;
    data->graygc = XtGetGC(data->dpy, widgetContext, data->mywin,
			   valuemask, &values);
}


/*
 *
 * Repaint the widget window
 *
 */

static void Redisplay(data)
WidgetData	data;
{
    int     textx, texty;
    Window  w = data->mywin;


    /*
     * Calculate Text x,y given window width and text width
     * to the specified justification
     */

    if (data->needsfreeing) return;

    if (data->justify == XtjustifyLeft) 
	textx = 2;
    else if (data->justify == XtjustifyRight)
	textx = data->width - data->twidth - 2;
    else
        textx = ((data->width - data->twidth) / 2);
    if (textx < 0) textx = 0;
    texty = (data->height - data->theight) / 2 +
	data->fontstruct->max_bounds.ascent;

    XFillRectangle(data->dpy, w,
		   ((data->highlighted || data->set)
		    ? data->normgc : data->invgc),
		   0, 0, data->width, data->height);

    if (data->highlighted || data->set)
	XFillRectangle(data->dpy, w,
		       data->set ? data->normgc : data->invgc,
		       textx, (int) ((data->height - data->theight) / 2),
		       data->twidth, data->theight);

    XDrawString(data->dpy, w,
		data->set ? data->invgc : (data->sensitive ? data->normgc : data->graygc),
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
    WidgetData	data = (WidgetData) eventdata;
    XtActionTokenPtr    actionList;
    CommandProc         proc;

    data->eventlevels++;
    switch (event->type) {
	case ConfigureNotify:
	    data->x = event->xconfigure.x;
	    data->y = event->xconfigure.y;
	    data->width = event->xconfigure.width;
	    data->height = event->xconfigure.height;
	    data->borderWidth = event->xconfigure.border_width;
/*	    Redisplay(data);*/
	    break;

        case DestroyNotify: Destroy(data); break;

        case Expose:
	    if (event->xexpose.count == 0)
		Redisplay(data);
	    break;
        default:
	    if (!data->sensitive) break;
            actionList =
	     (XtActionTokenPtr) XtTranslateEvent(event, data->state);
            for ( ; actionList != NULL; actionList = actionList->next) {
                if (actionList->type == XttokenAction) {
                    proc = (CommandProc) XtInterpretAction(
			data->dpy, data->state,
			actionList->value.action);
                    (*(proc))(data);
                }
            }
            break;
    }
    data->eventlevels--;
    if (data->needsfreeing && data->eventlevels == 0) {
	XtClearEventHandlers(data->dpy, data->mywin);
	(void) XDeleteContext(data->dpy, data->mywin, widgetContext);
        XtFree((char*)data->text);
        XtFree((char *) data);
    }

    return (XteventHandled);
}

/*
 *
 * Widget hilight event handler
 *
 */

static void Highlight(data)
WidgetData data;
{
    data->highlighted = TRUE;
    Redisplay(data);
}

/*
 *
 * Widget un-hilight event handler
 *
 */

static void UnHighlight(data)
WidgetData data;
{
    data->highlighted = FALSE;
    Redisplay(data);
}

/*
 *
 * Widget set event handler
 *
 */

static void Set(data)
WidgetData data;
{
    data->set = TRUE;
    Redisplay(data);
}

/*
 *
 * Widget un-set event handler
 *
 */

static void UnSet(data)
WidgetData data;
{
    data->set = FALSE;
    Redisplay(data);
}

/*
 *
 * Widget notify event handler
 *
 */

static void Notify(data)
WidgetData data;
{
    data->notifying = TRUE;
    XFlush(data->dpy);
    data->proc(data->tag);
    data->notifying = FALSE;
}

/*
 *
 * Destroy the widget; the window's been destroyed already.
 *
 */

static void Destroy(data)
    WidgetData	data;
{
    data->needsfreeing = TRUE;
}

/****************************************************************
 *
 * Public Procedures
 *
 ****************************************************************/

Window XtCommandCreate(dpy, parent, args, argCount)
    Display  *dpy;
    Window   parent;
    ArgList  args;
    int      argCount;
{
    WidgetData	data;
    XrmNameList  names;
    XrmClassList	classes;
    Drawable 	root;
    Position x, y;
    unsigned int depth;
    unsigned long valuemask;
    XSetWindowAttributes wvals;

    static XtActionsRec actionsTable[] = {
        {"set",		(caddr_t)Set},
        {"unset",	(caddr_t)UnSet},
        {"highlight",	(caddr_t)Highlight},
        {"unhighlight",	(caddr_t)UnHighlight},
        {"notify",	(caddr_t)Notify},
        {NULL, NULL}
    };

    if (!initialized) CommandInitialize ();

    data = (WidgetData) XtMalloc (sizeof (WidgetDataRec));

    /* Set Default Values */
    globaldata = globalinit;
    globaldata.dpy = dpy;
    XtGetResources(dpy, resources, XtNumber(resources), args, argCount, parent,
	"command", "Command", &names, &classes);
    *data = globaldata;

    data->state = (TranslationPtr) XtSetActionBindings(
	data->dpy,
	data->eventTable, actionsTable, (caddr_t) Ignore);

    if (data->fontstruct == NULL)
	data->fontstruct = globalinit.fontstruct =
	    XLoadQueryFont(data->dpy, "variable");

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
    
    if (data->mywin != NULL) {
	/* set global data from window parameters */
	if (XGetGeometry(data->dpy, data->mywin, &root, &x, &y,
			  &(data->width), &(data->height), &(data->borderWidth),
			  &depth)) {
	    XReparentWindow(data->dpy, data->mywin, parent, data->x,data->y);
	    XChangeWindowAttributes(data->dpy, data->mywin,
				    valuemask, &wvals);
	} else
	    data->mywin = NULL;
    }
    if (data->mywin == NULL)
	data->mywin = XCreateWindow(data->dpy, parent, data->x, data->y,
			 data->width, data->height, data->borderWidth,
			 0, InputOutput, (Visual *)CopyFromParent,
			 valuemask, &wvals);

    BuildGcs(data);

    XtSetNameAndClass(data->dpy, data->mywin, names, classes);
    XrmFreeNameList(names);
    XrmFreeClassList(classes);

    (void)XSaveContext(data->dpy, data->mywin, widgetContext, (caddr_t) data);

    /* set handler for expose, resize, and message events */
    /* HACK -- translation mgr should somehow do the event selection, not us! */
     XtSetEventHandler (data->dpy, data->mywin, (XtEventHandler)EventHandler,
			  StructureNotifyMask | ExposureMask | ButtonPressMask
			  | ButtonReleaseMask | EnterWindowMask
			  | LeaveWindowMask | KeyPressMask,
			  (caddr_t)data);

    return (data->mywin);
}


/*
 * Get Attributes
 */

void XtCommandGetValues(dpy, window, args, argCount)
Display *dpy;
Window window;
ArgList args;
int argCount;
{
    WidgetData data;
    data = DataFromWindow(dpy, window);
    if (data) {
	globaldata = *data;
	XtGetValues(resources, XtNumber(resources), args, argCount);
    }
}



/*
 * Set Attributes
 */

void XtCommandSetValues(dpy, window, args, argCount)
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
	reqbox.width = globaldata.twidth + 2*globaldata.ibw;
	reqbox.height = globaldata.theight + 2*globaldata.ibh;
	reply = XtMakeGeometryRequest(globaldata.dpy, globaldata.mywin, XtgeometryResize, 
				      &reqbox, &replybox);
	if (reply == XtgeometryAlmost) {
	    reqbox = replybox;
	    (void) XtMakeGeometryRequest(globaldata.dpy, globaldata.mywin, XtgeometryResize, 
					 &reqbox, &replybox);
	}
    }

    if (data->fgpixel != globaldata.fgpixel ||
	data->bgpixel != globaldata.bgpixel ||
	data->fontstruct != globaldata.fontstruct) BuildGcs(&globaldata);
    *data = globaldata;
    if (!data->sensitive) data->set = data->highlighted = FALSE;
    Redisplay (data);
}

