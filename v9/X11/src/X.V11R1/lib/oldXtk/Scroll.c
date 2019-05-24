#ifndef lint
static char *sccsid = "@(#)ScrollBar.c	1.8	2/25/87";
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


/* ScrollBar.c */
/* created by weissman, Mon Jul  7 13:20:03 1986 */

#include "X.h"
#include "Xlib.h"
#include "Intrinsic.h"
#include "Scroll.h"
#include "Atoms.h"
#include "cursorfont.h"

/* Private definitions. */

typedef void (*NotifyProc)();

typedef struct _WidgetDataRec
{
    Display	*dpy;		/* Display connection */
    Window	scrollbarWindow;/* Window containing the scrollbar */
    XtOrientation orientation;	/* Horizontal or vertical? */
    int		*value;		/* client data to pass in call-back procs */
    Position	x, y;		/* Location of the scroll bar */
    Dimension width, height, borderwidth;
				/* Dimensions of the scroll bar */
    NotifyProc	ScrollProc;     /* Call to scroll content */
    NotifyProc	ThumbProc;	/* Call if user thumbs scrollbar */
    float	top;		/* What percent is above the win's top */
    float	shown;		/* What percent is shown in the win */
    int		topLoc;		/* Pixel that corresponds to top */
    int		shownLength;	/* Num pixels corresponding to shown */
    int		foreground;	/* thumb color */
    Pixmap	customthumb;	/* thumb pixmap */
    Pixel	background;	/* window background color. */
    Pixel	border;		/* What to use for painting the border. */
    XtEventsPtr	eventTable;	/* Table for Translation Manager */
    caddr_t	state;		/* state for Translation Manager */
    int		eventlevels;	/* recursion levels of event handling */
    Cursor	NormalCursor;	/* The normal cursor for scrollbar window */
    Cursor	ButtonCursors[4];/* The cursors to use with each button */
    short	curbutton;	/* Which button is currently held down. */
    GC	gc;			/*this will go away if thumb becomes a window*/
} WidgetDataRec, *WidgetData;

static int dummyvalue;
extern void Dummy ();
static WidgetDataRec globaldata;
static WidgetDataRec globalinit = {
	NULL,		/* Display dpy; */
	NULL,		/* scrollBar window */
	XtorientVertical,/*orientation */
	&dummyvalue,	/* client data */
	0,0,		/* x,y */
	10,20,		/* width,height */
	1,		/* borderwidth */
	Dummy,		/* void (*proc)(); */
	Dummy,		/* void (*proc)(); */
	0.0,		/* fraction above top */
	0.0,		/* shown fraction */
	NULL,		/* top percent pixel */
	NULL,		/* shown length */
	NULL,		/* foreground */
        NULL,		/* customthumb */
	0,		/* background pixel*/
	0,		/* border pixel */
	NULL,		/* table for TM */
	NULL,		/* state for TM */
	NULL,		/* eventlevels */
	NULL,		/* normal cursor */
	{NULL,NULL,NULL,NULL}, /*ButtonCursors */
	NULL,		/*curbotton */
	NULL		/*gc*/
};


static Resource resources[] = {
    {XtNwindow, XtCWindow, XrmRWindow, sizeof(Window),
	 (caddr_t)&globaldata.scrollbarWindow, NULL},
    {XtNorientation, XtCOrientation, XtROrientation, sizeof(XtOrientation),
	 (caddr_t)&globaldata.orientation, NULL},
    {XtNvalue, XtCValue, XrmRPointer, sizeof(caddr_t),
	(caddr_t)&globaldata.value,(caddr_t)NULL},
    {XtNx, XtCX, XrmRInt,
	sizeof(int),(caddr_t)&globaldata.x, (caddr_t)NULL },
    {XtNy, XtCY, XrmRInt,
	sizeof(int),(caddr_t)&globaldata.y, (caddr_t)NULL },
    {XtNwidth, XtCWidth, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.width, NULL},
    {XtNheight, XtCHeight, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.height, NULL},
    {XtNborderWidth, XtCBorderWidth, XrmRInt, sizeof(int),
	 (caddr_t)&globaldata.borderwidth, NULL},
    {XtNborder, XtCColor, XrmRPixel, sizeof(int),
	 (caddr_t)&globaldata.border, (caddr_t)&XtDefaultFGPixel},
    {XtNscrollUpDownProc, XtCFunction, XtRFunction,
	 sizeof(globaldata.ScrollProc),
	 (caddr_t)&globaldata.ScrollProc, NULL},
    {XtNthumbProc, XtCFunction, XtRFunction, sizeof(globaldata.ThumbProc),
	 (caddr_t)&globaldata.ThumbProc, NULL},
    {XtNforeground, XtCColor, XrmRPixel, sizeof(int),
	 (caddr_t)&globaldata.foreground, (caddr_t)&XtDefaultFGPixel},
    {XtNcustomthumb, XtCPixmap, XrmRPixmap, sizeof(Pixmap),
	 (caddr_t)&globaldata.customthumb,NULL},
    {XtNbackground, XtCColor, XrmRPixel, sizeof(int),
	 (caddr_t)&globaldata.background, (caddr_t)&XtDefaultBGPixel},
    {XtNtop, XtCFraction, XrmRFloat, sizeof(float),
	 (caddr_t)&globaldata.top, NULL},
    {XtNshown, XtCFraction, XrmRFloat, sizeof(float),
	 (caddr_t)&globaldata.shown, NULL},
    {XtNeventBindings,XtCEventBindings,XtREventBindings,
	 sizeof(caddr_t), (caddr_t)&globaldata.eventTable, NULL}
};

static XContext widgetContext;



#define MINBARHEIGHT	7     /* How many pixels of scrollbar to always show */
#define NoButton -1
#define PICKLENGTH(data,x,y) \
    ((data->orientation == XtorientHorizontal) ? x : y)
#define PICKTHICKNESS(data,x,y) \
    ((data->orientation == XtorientHorizontal) ? y : x)
#define MIN(x,y)	((x) < (y) ? (x) : (y))
#define MAX(x,y)	((x) > (y) ? (x) : (y))


/* Orientation enumeration constants */

static	XrmQuark  XtQEhorizontal;
static	XrmQuark  XtQEvertical;

/*ARGSUSED*/
#define	done(address, type) \
	{ (*toVal).size = sizeof(type); (*toVal).addr = (caddr_t) address; }

extern void _XLowerCase();

static void CvtStringToOrientation(fromVal, toVal)
    XrmValue	fromVal;
    XrmValue	*toVal;
{
    static XtOrientation orient;
    XrmQuark	q;
    char	lowerName[1000];

/* ||| where to put LowerCase */
    _XLowerCase((char *) fromVal.addr, lowerName);
    q = XrmAtomToQuark(lowerName);
    if (q == XtQEhorizontal) {
    	orient = XtorientHorizontal;
	done(&orient, XtOrientation);
	return;
    }
    if (q == XtQEvertical) {
    	orient = XtorientVertical;
	done(&orient, XtOrientation);
	return;
    }
};


static char *defaultEventBindings[] = {
	"<ButtonPress>left:		startup\n",
	"<ButtonPress>middle:		startscroll\n",
	"<ButtonPress>right:		startdown\n",
	"<ButtonRelease>left:		doup\n",
	"<ButtonRelease>middle:		doscroll\n",
	"<ButtonRelease>right:		dodown\n",
	"<MotionNotify>middle:		movebar\n", /*bug in TM forces button spec here */
	NULL
};

static void Ignore()
{
	(void)printf("you are being ignored\n");
}

static Boolean initialized = FALSE;
static void ScrollBarInitialize ()
{
    widgetContext = XUniqueContext();
    globalinit.eventTable = XtParseEventBindings(defaultEventBindings);
    XtQEhorizontal = XrmAtomToQuark(XtEhorizontal);
    XtQEvertical   = XrmAtomToQuark(XtEvertical);
    XrmRegisterTypeConverter(XrmRString, XtROrientation, CvtStringToOrientation);
    initialized = TRUE;
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

/* ARGSUSED */
static void Dummy(p)	/* default call back proc */
caddr_t p;
{
	(void) printf("dummy call back for ScrollBar\n");
}

/*
 * Make sure the first number is within the range specified by the other
 * two numbers.
 */

static int InRange(num, small, big)
int num, small, big;
{
    return (num < small) ? small : ((num > big) ? big : num);
}

/*
 * Same as above, but for floating numbers. 
 */

static float FloatInRange(num, small, big)
float num, small, big;
{
    return (num < small) ? small : ((num > big) ? big : num);
}

/* Given the scrollbar window, return the SbarInfo record for it. */
static WidgetData SbarInfoFromSbarWindow(dpy, window)
  Display *dpy;
  Window window;	/* Scrollbar window */
{
    WidgetData result;
    if (XFindContext(dpy, window, widgetContext, (caddr_t *)&result))
	return NULL;
    return result;
}




/* Given a WidgetData record and a point in the scrollbarWindow, determine */
/* what percentage that point indicates.				 */





/* Fill the area specified by top and bottom with the given pattern. */
 float FractionLoc(data,x,y)
  WidgetData data;
  int x,y;
{
    float   result;
    result = PICKLENGTH(data, (float) x/data->width, (float) y/data->height);
    return FloatInRange(result, 0.0, 1.0);
}


static FillArea(data, top, bottom, thumb)
  WidgetData data;
  Position top, bottom;
  int thumb;
{
    Dimension length = bottom-top;
    switch(thumb) {
	/* Fill the new Thumb location */
      case 1:
	if (data->orientation == XtorientHorizontal) 
	    XFillRectangle(data->dpy, data->scrollbarWindow, data->gc,
			   top, 1, length, data->height-2);
	
	else XFillRectangle(data->dpy, data->scrollbarWindow, data->gc,
			    1, top, data->width-2, length);

	break;
	/* Clear the old Thumb location */
      case 0:
	if (data->orientation == XtorientHorizontal) 
	    XClearArea(data->dpy, data->scrollbarWindow, top, 1,
		       length, data->height-2, FALSE);
	
	else XClearArea(data->dpy, data->scrollbarWindow, 1, top,
			data->width-2, length, FALSE);

    }  
}


/* Paint the thumb in the area specified by data->top and
   data->shown.  The old area is erased.  The painting and
   erasing is done cleverly so that no flickering will occur. */

static PaintThumb(data)
  WidgetData data;
{
    int length, oldtop, oldbot, newtop, newbot;
    length = PICKLENGTH(data, data->width, data->height);
    oldtop = data->topLoc;
    oldbot = oldtop + data->shownLength;
    newtop = length * data->top;
    newbot = newtop + length * (data->shown);
    if (newbot < newtop + MINBARHEIGHT) newbot = newtop + MINBARHEIGHT;
    if (newtop < oldtop)
	FillArea(data, newtop, MIN(newbot, oldtop),1);
    if (newtop > oldtop)
	FillArea(data, oldtop, MIN(newtop, oldbot), 0);
    if (newbot < oldbot)
	FillArea(data, MAX(newbot, oldtop), oldbot,0);
    if (newbot > oldbot)
	FillArea(data, MAX(newtop, oldbot), newbot, 1);
    data->topLoc = newtop;
    data->shownLength = newbot - newtop;
}


/* Move the thumb to the value corresponding to the given point. */

static MoveThumb(data,x,y)
  WidgetData data;
  int x,y;
{
    data->top = FractionLoc(data, x, y);
    PaintThumb(data);
}





StartScroll(data,button)
  WidgetData data;
  unsigned int button;
{
    if (data->curbutton != 0) return;
    data->curbutton = button;

    XDefineCursor(data->dpy, data->scrollbarWindow,
		  data->ButtonCursors[data->curbutton]);
    XFlush(data->dpy);

}



ScrollElevator(data,button,x,y)
   WidgetData data;
   Position x,y;
   unsigned int button;
{
    if (data->curbutton != 0) return;
    data->curbutton = button;
    XDefineCursor(data->dpy, data->scrollbarWindow,
		  data->ButtonCursors[data->curbutton]);
    XFlush(data->dpy);

    MoveThumb(data,x,y);
}   



/* The user has thumbed to the location he wants; tell the other module where
   the thumb now is. */

static FinalThumbLoc(data,x,y)
  WidgetData data;
  Position x,y;
{
    MoveThumb(data, x, y);
    (*(data->ThumbProc)) (data->dpy, data->scrollbarWindow,
	    data->value,
	    data->top);
}


static NotifyScroll(data,x,y)
   WidgetData data;
   Position x,y;
{
    if (data->curbutton == 0) return; 
    XDefineCursor(data->dpy,data->scrollbarWindow, data->NormalCursor);
    XFlush(data->dpy);
    (*(data->ScrollProc)) (data->dpy, data->scrollbarWindow,
			   data->value,
			   InRange(PICKLENGTH(data, x,y),
				   0,
				   (int)PICKLENGTH(data, data->width, data->height)));
    data->curbutton = AnyButton; /*means NoButton */


}

static NotifyScroll1(data,x,y)
   WidgetData data;
   Position x,y;
{
    if (data->curbutton == 0) return;
    XDefineCursor(data->dpy,data->scrollbarWindow, data->NormalCursor);
    XFlush(data->dpy);
    (*(data->ScrollProc)) (data->dpy, data->scrollbarWindow,
			   data->value,
			   -InRange(PICKLENGTH(data, x,y),
				    0,
				    (int)PICKLENGTH(data, data->width, data->height)));
    data->curbutton = AnyButton; /*means NoButton */


}
static NotifyThumb(data,x,y)
  WidgetData data;
  Position x,y;
{
    if (data->curbutton == 0) return;
    XDefineCursor(data->dpy,data->scrollbarWindow, data->NormalCursor);
    XFlush(data->dpy);

	    FinalThumbLoc(data, x, y);
	    data->curbutton = AnyButton; /*means NoButton */

}




static MoveElevator(data, x,y)
WidgetData data;
Position x,y;
{
	if (data->curbutton == 0) return;
	MoveThumb(data, x, y);
}



/* Handle an event in a scrollBar window.  */

static XtEventReturnCode HandleScrollBarEvent(event, eventdata)
XEvent *event;		/* The event itself. */
caddr_t eventdata;
{
    WidgetData data = (WidgetData) eventdata;
    XtActionTokenPtr actionList;
    NotifyProc proc;

    data->eventlevels++;
    switch (event->type) {
	    case Expose:
		if (event->xexpose.count == 0){
		    data->topLoc = -1000;	/* Forces entire thumb to be painted. */
		    PaintThumb(data); 
		}
		    return XteventHandled;
	    case DestroyNotify:
		XtScrollBarDestroy(event->xany.display, event->xany.window);
		return XteventHandled;
	    case ConfigureNotify:
		data->width = event->xconfigure.width;
		data->height = event->xconfigure.height;
		FillArea(data, data->topLoc, 
	  	 data->topLoc + data->shownLength, 0);

		    data->topLoc = -1000;	/* Forces entire thumb to be painted. */
		    PaintThumb(data);
		    return (XteventHandled);

	default:
		actionList = (XtActionTokenPtr) XtTranslateEvent(
			event, (TranslationPtr) data->state);
		for (; actionList != NULL; actionList = actionList ->next) {
 		 if (actionList ->type ==XttokenAction) {
		 proc = (NotifyProc)XtInterpretAction(data->dpy, (TranslationPtr) data->state, actionList->value.action);

		/* HACK until TM passes data */
	if (proc == (NotifyProc)StartScroll) StartScroll(data,event->xbutton.button);
 	 else if (proc == (NotifyProc)ScrollElevator) ScrollElevator(data,event->xbutton.button,event->xbutton.x,event->xbutton.y);
	 else if (proc == (NotifyProc)NotifyScroll) NotifyScroll(data, event->xbutton.x, event->xbutton.y);
 	 else if (proc == (NotifyProc)NotifyThumb) NotifyThumb(data, event->xbutton.x,event->xbutton.y);
	 else if (proc == (NotifyProc)NotifyScroll1) NotifyScroll1(data,event->xbutton.x,event->xbutton.y);
	 else if (proc == (NotifyProc)MoveElevator) MoveElevator(data, event->xmotion.x, event->xmotion.y);

/*		 (*proc) (data); */
		 }
	        }
		break;

}
		data->eventlevels--;
		return (XteventHandled);
}


/* Public routines. */

/* Set the scroll bar to the given location. */

void XtScrollBarSetThumb(dpy, scrollbarWindow,top,shown)
  Display *dpy;
  Window scrollbarWindow;
  float top, shown;
{
    WidgetData data;
    data = SbarInfoFromSbarWindow(dpy, scrollbarWindow);
    data->top = top;
    data->shown = shown;
    PaintThumb(data);
}


/* scroll bar already destroyed, remove its associated data. */
void XtScrollBarDestroy(dpy, scrollbarWindow)
  Display *dpy;
  Window scrollbarWindow;
{
    XtFree((char *)SbarInfoFromSbarWindow(dpy, scrollbarWindow));
    XtClearEventHandlers(dpy, scrollbarWindow);
    (void) XDeleteContext(dpy, scrollbarWindow,widgetContext);
}



/* Given a window definition, make it a scroll bar. */
Window XtScrollBarCreate(dpy, parent, args, argCount)
	Display *dpy;
	Window parent;
	ArgList args;
	int	argCount;
{
    WidgetData data;	/* Record for info about this scrollBar */
    XrmNameList names;
    XrmClassList classes;
    XGCValues values; /* goes away if thumb becomes a window */
    unsigned long valuemask;
    XImage image;

static short pixmap_bits[] = {
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

#define default_width 32
#define default_height 32

static XtActionsRec actionsTable[] = {
	{"startup",	(caddr_t)StartScroll},
	{"startscroll",	(caddr_t)ScrollElevator},
	{"startdown",	(caddr_t)StartScroll},
	{"doup",	(caddr_t)NotifyScroll},
	{"doscroll",	(caddr_t)NotifyThumb},
	{"dodown",	(caddr_t)NotifyScroll1},
	{"movebar",	(caddr_t)MoveElevator},
	{NULL,NULL}
  };

    if (!initialized) ScrollBarInitialize();


    globaldata = globalinit;
    globaldata.dpy = dpy;
    XtGetResources(dpy, 
	resources, XtNumber(resources), args, argCount, parent,
	"scrollBar","ScrollBar",&names, &classes);

    data = (WidgetData) XtMalloc(sizeof(WidgetDataRec));
    *data = globaldata;

    data->state = XtSetActionBindings(data->dpy, data->eventTable, actionsTable, (caddr_t) Ignore);

    if (data->scrollbarWindow == NULL) {
	if (data->width == 0) data->width = 1;
	if (data->height == 0) data->height = 1;
	data->scrollbarWindow = XCreateSimpleWindow(
		data->dpy, parent, data->x, data->y,
		data->width, data->height, 1,
		data->border, data->background);
    } else {
	XWindowChanges wc;
	unsigned int valuemask;
	valuemask = CWX | CWY | CWWidth | CWHeight | CWBorderWidth;
	wc.x = data->x; wc.y = data->y; wc.width = data->width;
	wc.height=data->height; wc.border_width = data->borderwidth;
	XConfigureWindow(data->dpy,data->scrollbarWindow,valuemask,&wc);
	XReparentWindow(data->dpy,data->scrollbarWindow,parent,data->x,data->y);
    }

    if (data->orientation == XtorientHorizontal) {
	if (data->NormalCursor == NULL)
	 data->NormalCursor = XtGetCursor(dpy, XC_sb_h_double_arrow);
	if (data -> ButtonCursors[Button1] == NULL )
  	 data->ButtonCursors[Button1] = XtGetCursor(dpy, XC_sb_left_arrow);
	if (data -> ButtonCursors[Button2] == NULL)
	 data->ButtonCursors[Button2] = XtGetCursor(dpy, XC_sb_up_arrow);
	if (data -> ButtonCursors[Button3] == NULL)
	data->ButtonCursors[Button3] = XtGetCursor(dpy, XC_sb_right_arrow);
    } else { if (data->orientation == XtorientVertical) {
	if (data->NormalCursor == NULL)
	 data->NormalCursor = XtGetCursor(dpy, XC_sb_v_double_arrow);
	if (data -> ButtonCursors[Button1] == NULL )
  	 data->ButtonCursors[Button1] = XtGetCursor(dpy, XC_sb_up_arrow);
	if (data -> ButtonCursors[Button2] == NULL)
	 data->ButtonCursors[Button2] = XtGetCursor(dpy, XC_sb_left_arrow);
	if (data -> ButtonCursors[Button3] == NULL)
	data->ButtonCursors[Button3] = XtGetCursor(dpy, XC_sb_down_arrow);
       }
    }
  if (data->foreground == NULL)
	data->foreground = BlackPixel(data->dpy,DefaultScreen(dpy));


  
  if (data->customthumb == NULL) {
   image.height = default_height;
   image.width = default_width;
   image.xoffset = 0;
   image.format = XYBitmap;
   image.data = (char*) pixmap_bits;
   image.byte_order = ImageByteOrder(data->dpy);
   image.bitmap_pad = BitmapPad(data->dpy);
   image.bitmap_bit_order = BitmapBitOrder(data->dpy);
   image.bitmap_unit = BitmapUnit(data->dpy);
   image.depth = 1;
   image.bytes_per_line = default_width/8;
   image.obdata = NULL;

   data->customthumb = XCreatePixmap(data->dpy, data->scrollbarWindow,
				     (Dimension)image.width,
				     (Dimension)image.height, 
				     DefaultDepth(
				         data->dpy, DefaultScreen(data->dpy)));

    values.foreground = data->foreground;
    values.background = data->background;
    values.function = GXcopy;	    /* need to be explicit about defaults */
    values.plane_mask = AllPlanes;  /* until XtGetGC is re-done		  */
    valuemask = GCFunction | GCForeground | GCBackground | GCPlaneMask;
    data->gc = XtGetGC( data->dpy, widgetContext, data->scrollbarWindow,
		        valuemask, &values);
    XPutImage(data->dpy, data->customthumb, data->gc, &image, 0, 0, 0, 0,
	      (Dimension) image.width, (Dimension) image.height);

    values.fill_style = FillTiled;
    values.tile = data->customthumb;
    valuemask = GCFillStyle | GCTile;
    data->gc = XtGetGC(data->dpy,widgetContext,data->scrollbarWindow,valuemask,&values);
   }

else {

    values.foreground = data->foreground;
    values.fill_style = FillTiled;
    values.tile = data->customthumb;
    valuemask = GCForeground | GCFillStyle | GCTile;
    data->gc = XtGetGC(data->dpy,widgetContext,data->scrollbarWindow,valuemask,&values);
}
    XDefineCursor(data->dpy,data->scrollbarWindow, data->NormalCursor);

    XtSetNameAndClass(data->dpy, data->scrollbarWindow, names, classes);

    (void) XSaveContext(data->dpy, data->scrollbarWindow, widgetContext, (caddr_t)data);
    XtSetEventHandler(data->dpy, data->scrollbarWindow,
			(XtEventHandler)HandleScrollBarEvent,
			ButtonPressMask | ExposureMask | ButtonReleaseMask |
			    Button2MotionMask | StructureNotifyMask,
			(caddr_t) data);
    return data->scrollbarWindow;
}

void XtScrollBarGetValues(dpy, window, args, argCount)
Display *dpy;
Window window;
ArgList args;
int argCount;
{
    WidgetData	data;
    data = DataFromWindow(dpy, window);
    if (data == NULL) return;
    globaldata = *data;
    XtGetValues(resources, XtNumber(resources), args, argCount);
}


void XtScrollBarSetValues(dpy, window, args, argCount)
Display *dpy;
Window window;
ArgList args;
int argCount;
{
    WidgetData	data;
    short thumbmoved, redraw;

    thumbmoved = redraw = FALSE;
    data = DataFromWindow(dpy, window);
    if (data == NULL) return;
    globaldata = *data;
    XtSetValues(resources, XtNumber(resources), args, argCount);
    if (globaldata.border != data->border) {
	data->border = globaldata.border;
	if (data->borderwidth != 0)
	    XSetWindowBorder(data->dpy,data->scrollbarWindow, data->border);
    }
    data->ScrollProc = globaldata.ScrollProc;
    data->ThumbProc = globaldata.ThumbProc;
    if (globaldata.foreground != data->foreground
     || globaldata.background != data->background) {

    }
    if (globaldata.foreground != data->foreground) {
	data->foreground = globaldata.foreground;
    }
    if (globaldata.background != data->background) {
    	data->background = globaldata.background;

    }

   if (globaldata.customthumb != data->customthumb) 
	data->customthumb = globaldata.customthumb;

    if (globaldata.top != data->top) {
	data->top = globaldata.top;
	thumbmoved = TRUE;
    }
    if (globaldata.shown != data->shown) {
	data->shown = globaldata.shown;
	thumbmoved = TRUE;
    }
    if (redraw) {
	XClearWindow(data->dpy, data->scrollbarWindow);
	data->topLoc = -1000;
	PaintThumb(data);
    } else if (thumbmoved)
	PaintThumb(data);
}

