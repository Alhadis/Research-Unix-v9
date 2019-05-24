#ifndef lint
static char rcsid[] = "$Header: Scroll.c,v 1.7 87/09/14 00:43:28 swick Exp $";
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
/* converted by swick, Thu Aug 27 1987 */

#include "Xlib.h"
#include "Xresource.h"
/*#include "Conversion.h"*/
#include "Intrinsic.h"
#include "Scroll.h"
#include "ScrollP.h"
#include "Atoms.h"
#include "cursorfont.h"

/* Private definitions. */

static char *defaultTranslationTable[] = {
	"<Btn1Down>:		StartPageBack\n", /* should be generic... */
	"<Btn2Down>:		StartScroll\n",
	"<Btn3Down>:		StartPageForward\n", /* ...but needs TM args */
	"<Btn1Up>:		DoPageBack\n",
	"<Btn2Up>:		DoScroll\n",
	"<Btn3Up>:		DoPageForward\n",
	"<Motion>2:		MoveThumb\n", /* bug? in TM forces button spec here */
	NULL
};

/* grodyness needed because Xrm wants pointer to thing, not thing... */
static caddr_t defaultTranslations = (caddr_t)defaultTranslationTable;

static XtResource resources[] = {
  {XtNorientation, XtCOrientation, XtROrientation, sizeof(XtOrientation),
     XtOffset(ScrollbarWidget, scrollbar.orientation), XtRString, "vertical"},
  {XtNscrollProc, XtCScrollProc, XtRFunction, sizeof(XtCallbackProc),
     XtOffset(ScrollbarWidget, scrollbar.scrollProc), XtRFunction, NULL},
  {XtNthumbProc, XtCScrollProc, XtRFunction, sizeof(XtCallbackProc),
     XtOffset(ScrollbarWidget, scrollbar.thumbProc), XtRFunction, NULL},
  {XtNparameter, XtCParameter, XtRPointer, sizeof(caddr_t),
     XtOffset(ScrollbarWidget, scrollbar.closure), XtRPointer, NULL},
  {XtNthumb, XtCThumb, XtRPixmap, sizeof(Pixmap),
     XtOffset(ScrollbarWidget, scrollbar.thumb), XtRPixmap, NULL},
  {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
     XtOffset(ScrollbarWidget, scrollbar.foreground), XtRString, "black"},
  {XtNscrollVCursor, XtCScrollVCursor, XtRCursor, sizeof(Cursor),
     XtOffset(ScrollbarWidget, scrollbar.verCursor), XtRString, "sb_v_double_arrow"},
  {XtNscrollHCursor, XtCScrollHCursor, XtRCursor, sizeof(Cursor),
     XtOffset(ScrollbarWidget, scrollbar.horCursor), XtRString, "sb_h_double_arrow"},
  {XtNscrollUCursor, XtCScrollUCursor, XtRCursor, sizeof(Cursor),
     XtOffset(ScrollbarWidget, scrollbar.upCursor), XtRString, "sb_up_arrow"},
  {XtNscrollDCursor, XtCScrollDCursor, XtRCursor, sizeof(Cursor),
     XtOffset(ScrollbarWidget, scrollbar.downCursor), XtRString, "sb_down_arrow"},
  {XtNscrollLCursor, XtCScrollLCursor, XtRCursor, sizeof(Cursor),
     XtOffset(ScrollbarWidget, scrollbar.leftCursor), XtRString, "sb_left_arrow"},
  {XtNscrollRCursor, XtCScrollRCursor, XtRCursor, sizeof(Cursor),
     XtOffset(ScrollbarWidget, scrollbar.rightCursor), XtRString, "sb_right_arrow"},
  {XtNeventBindings, XtCEventBindings, XtRStringTable, sizeof(_XtTranslations),
     XtOffset(ScrollbarWidget, core.translations), XtRStringTable, (caddr_t)&defaultTranslations},
};

static void ClassInitialize();
static void Initialize();
static void Realize();
static void Resize();
static void Redisplay();
static Boolean SetValues();

static void StartPageBack();
static void StartSmoothScroll();
static void StartPageForward();
static void DoPageBack();
static void DoSmoothScroll();
static void DoPageForward();
static void MoveThumb();

static XtActionsRec actions[] = {
	{"StartPageBack",	(caddr_t)StartPageBack},
	{"StartScroll",		(caddr_t)StartSmoothScroll},
	{"StartPageForward",	(caddr_t)StartPageForward},
	{"DoPageBack",		(caddr_t)DoPageBack},
	{"DoScroll",		(caddr_t)DoSmoothScroll},
	{"DoPageForward",	(caddr_t)DoPageForward},
	{"MoveThumb",		(caddr_t)MoveThumb},
	{NULL,NULL}
};


static ScrollbarClassRec scrollbarClassRec = {
/* core fields */
    /* superclass       */      (WidgetClass) &widgetClassRec,
    /* class_name       */      "Scroll",
    /* size             */      sizeof(ScrollbarRec),
    /* class_initialize	*/	ClassInitialize,
    /* class_inited	*/	FALSE,
    /* initialize       */      Initialize,
    /* realize          */      Realize,
    /* actions          */      actions,
    /* num_actions	*/	XtNumber(actions),
    /* resources        */      resources,
    /* num_resources    */      XtNumber(resources),
    /* xrm_class        */      NULLQUARK,
    /* compress_motion	*/	FALSE,
    /* compress_exposure*/	TRUE,
    /* visible_interest */      FALSE,
    /* destroy          */      NULL,
    /* resize           */      Resize,
    /* expose           */      Redisplay,
    /* set_values       */      SetValues,
    /* accept_focus     */      NULL,
};

WidgetClass scrollbarWidgetClass = (WidgetClass)&scrollbarClassRec;

#define MINBARHEIGHT	7     /* How many pixels of scrollbar to always show */
#define NoButton -1
#define PICKLENGTH(widget, x, y) \
    ((widget->scrollbar.orientation == XtorientHorizontal) ? x : y)
#define PICKTHICKNESS(widget, x, y) \
    ((widget->scrollbar.orientation == XtorientHorizontal) ? y : x)
#define MIN(x,y)	((x) < (y) ? (x) : (y))
#define MAX(x,y)	((x) > (y) ? (x) : (y))


/* Orientation enumeration constants */

static	XrmQuark  XtQEhorizontal;
static	XrmQuark  XtQEvertical;

/*ARGSUSED*/
#define	done(address, type) \
	{ (*toVal).size = sizeof(type); (*toVal).addr = (caddr_t) address; }

extern void _XLowerCase();

static void CvtStringToOrientation(dpy, fromVal, toVal)
    Display	*dpy;
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


static void ClassInitialize()
{
    XtQEhorizontal = XrmAtomToQuark(XtEhorizontal);
    XtQEvertical   = XrmAtomToQuark(XtEvertical);
    XrmRegisterTypeConverter(XrmRString, XtROrientation, CvtStringToOrientation);
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


/* Fill the area specified by top and bottom with the given pattern. */
static float FractionLoc(w, x, y)
  ScrollbarWidget w;
  int x, y;
{
    float   result;

    result = PICKLENGTH(w, (float) x/w->core.width,
			(float) y/w->core.height);
    return FloatInRange(result, 0.0, 1.0);
}


static FillArea(w, top, bottom, thumb)
  ScrollbarWidget w;
  Position top, bottom;
  int thumb;
{
    Dimension length = bottom-top;

    switch(thumb) {
	/* Fill the new Thumb location */
      case 1:
	if (w->scrollbar.orientation == XtorientHorizontal) 
	    XFillRectangle(XtDisplay(w), XtWindow(w),
			   w->scrollbar.gc, top, 1, length,
			   w->core.height-2);
	
	else XFillRectangle(XtDisplay(w), XtWindow(w), w->scrollbar.gc,
			    1, top, w->core.width-2, length);

	break;
	/* Clear the old Thumb location */
      case 0:
	if (w->scrollbar.orientation == XtorientHorizontal) 
	    XClearArea(XtDisplay(w), XtWindow(w), top, 1,
		       length, w->core.height-2, FALSE);
	
	else XClearArea(XtDisplay(w), XtWindow(w), 1,
			top, w->core.width-2, length, FALSE);

    }  
}


/* Paint the thumb in the area specified by w->top and
   w->shown.  The old area is erased.  The painting and
   erasing is done cleverly so that no flickering will occur. */

static void PaintThumb( w )
  ScrollbarWidget w;
{
    int length, oldtop, oldbot, newtop, newbot;
    length = PICKLENGTH(w, w->core.width, w->core.height);
    oldtop = w->scrollbar.topLoc;
    oldbot = oldtop + w->scrollbar.shownLength;
    newtop = length * w->scrollbar.top;
    newbot = newtop + length * (w->scrollbar.shown);
    if (newbot < newtop + MINBARHEIGHT) newbot = newtop + MINBARHEIGHT;
    if (newtop < oldtop)
	FillArea(w, newtop, MIN(newbot, oldtop), 1);
    if (newtop > oldtop)
	FillArea(w, oldtop, MIN(newtop, oldbot), 0);
    if (newbot < oldbot)
	FillArea(w, MAX(newbot, oldtop), oldbot, 0);
    if (newbot > oldbot)
	FillArea(w, MAX(newtop, oldbot), newbot, 1);
    w->scrollbar.topLoc = newtop;
    w->scrollbar.shownLength = newbot - newtop;
}


static void Initialize( request, new )
   Widget request;		/* what the client asked for */
   Widget new;			/* what we're going to give him */
{
    ScrollbarWidget w = (ScrollbarWidget) new;
    XGCValues gcValues;

    if (w->scrollbar.thumb == NULL) {
        w->scrollbar.thumb = XtGrayPixmap( XtScreen(w) );
    }

    gcValues.fill_style = FillTiled;
    gcValues.tile = w->scrollbar.thumb;
    w->scrollbar.gc = XtGetGC( w, GCFillStyle | GCTile, &gcValues);

    if (w->core.width == 0)  w->core.width = 1;
    if (w->core.height == 0) w->core.height = 1;

}

static void Realize( gw, valueMask, attributes )
   Widget gw;
   Mask valueMask;
   XSetWindowAttributes *attributes;
{
    ScrollbarWidget w = (ScrollbarWidget) gw;
    XSetWindowAttributes winAttr;
    Mask attrMask;

    w->scrollbar.inactiveCursor =
      (w->scrollbar.orientation == XtorientVertical)
	? w->scrollbar.verCursor
	: w->scrollbar.horCursor;

    winAttr = *attributes;
    winAttr.cursor = w->scrollbar.inactiveCursor;
    attrMask = valueMask | CWCursor;
    
    w->core.window =
      XCreateWindow(
		     XtDisplay( w ), XtWindow(w->core.parent),
		     w->core.x, w->core.y,
		     w->core.width, w->core.height,
		     w->core.border_width, w->core.depth,
		     InputOutput, (Visual *)CopyFromParent,
		     attrMask, &winAttr );
}


static Boolean SetValues( current, request, desired )
   Widget current,		/* what I am */
          request,		/* what he wants me to be */
          desired;		/* what I will become */
{
    ScrollbarWidget w = (ScrollbarWidget) current;
    ScrollbarWidget rw = (ScrollbarWidget) request;
    ScrollbarWidget dw = (ScrollbarWidget) desired;
    short thumbmoved, redraw;

    thumbmoved = redraw = FALSE;

    /* Core make take care of the following... we'll have to see */
    if (w->core.border_pixel != dw->core.border_pixel) {
	if (w->core.border_width != 0)
	    XSetWindowBorder( XtDisplay(w), XtWindow(w), w->core.border_pixel );
    }

    if (w->scrollbar.foreground != rw->scrollbar.foreground ||
	w->core.background_pixel != rw->core.background_pixel)
        redraw = TRUE;

    if (w->scrollbar.top != dw->scrollbar.top ||
        w->scrollbar.shown != dw->scrollbar.shown)
	thumbmoved = TRUE;

    if (redraw)
	w->scrollbar.topLoc = -1000;

    return( redraw || thumbmoved );
}

/* ARGSUSED */
static void Resize( gw, geometry )
   Widget gw;
   XtWidgetGeometry geometry;
{
    ScrollbarWidget w = (ScrollbarWidget) gw;

    FillArea( w, w->scrollbar.topLoc, 
	      w->scrollbar.topLoc + w->scrollbar.shownLength, 0 );

    w->scrollbar.topLoc = -1000; /* Forces entire thumb to be painted. */
    PaintThumb( w );
    
}


/* ARGSUSED */
static void Redisplay( gw, event )
   Widget gw;
   XEvent *event;
{
    ScrollbarWidget w = (ScrollbarWidget) gw;

    w->scrollbar.topLoc = -1000; /* Forces entire thumb to be painted. */
    PaintThumb( w ); 
}


static void StartScroll(gw, event, direction)
  Widget gw;
  XEvent *event;
  char direction;		/* Back|Forward|Smooth */
{
    ScrollbarWidget w = (ScrollbarWidget) gw;
    Cursor cursor;

    if (w->scrollbar.direction != 0) return;
    w->scrollbar.direction = direction;

    switch( direction ) {
	case 'B':
	case 'b':	cursor = (w->scrollbar.orientation == XtorientVertical)
				   ? w->scrollbar.upCursor
				   : w->scrollbar.leftCursor; break;

	case 'F':
	case 'f':	cursor = (w->scrollbar.orientation == XtorientVertical)
				   ? w->scrollbar.downCursor
				   : w->scrollbar.rightCursor; break;

	case 'S':
	case 's':	cursor = (w->scrollbar.orientation == XtorientVertical)
				   ? w->scrollbar.rightCursor
				   : w->scrollbar.upCursor; break;

	default:	return; /* invalid invocation */
    }

    XDefineCursor(XtDisplay(w), XtWindow(w), cursor);

    XFlush(XtDisplay(w));

    if (direction == 'S' || direction == 's') MoveThumb(gw, event);

}


/* The following are only needed until the TM implements args */

static void StartPageBack( gw, event )
   Widget gw;
   XEvent *event;
{
    StartScroll( gw, event, 'B' );
};

static void StartPageForward( gw, event )
   Widget gw;
   XEvent *event;
{
    StartScroll( gw, event, 'F' );
};

static void StartSmoothScroll( gw, event )
   Widget gw;
   XEvent *event;
{
    StartScroll( gw, event, 'S' );
};


static void DoScroll( gw, event, direction )
   Widget gw;
   XEvent *event;
   char direction;
{
    ScrollbarWidget w = (ScrollbarWidget) gw;

    if (w->scrollbar.direction == 0) return;

    XDefineCursor(XtDisplay(w), XtWindow(w), w->scrollbar.inactiveCursor);
    XFlush(XtDisplay(w));

    switch( direction ) {
        case 'B':
	case 'b':
        case 'F':
	case 'f':    if (w->scrollbar.scrollProc)
			(*(w->scrollbar.scrollProc))
			  ( w, w->scrollbar.closure,
			    InRange( PICKLENGTH( w,
						 event->xmotion.x,
						 event->xmotion.y),
				     0,
				     (int)PICKLENGTH( w,
						      w->core.width,
						      w->core.height)));
	             break;

        case 'S':
	case 's':    /* MoveThumb has already called the thumbProc(s) */
		     break;
    }

    w->scrollbar.direction = 0;

}

/* The following are only needed until the TM implements args */

static void DoPageBack( gw, event )
   Widget gw;
   XEvent *event;
{
    DoScroll( gw, event, 'B' );
}

static void DoPageForward( gw, event )
   Widget gw;
   XEvent *event;
{
    DoScroll( gw, event, 'F' );
};

static void DoSmoothScroll( gw, event )
   Widget gw;
   XEvent *event;
{
    DoScroll( gw, event, 'S' );
};



static void MoveThumb( gw, event )
   Widget gw;
   XEvent *event;
{
    ScrollbarWidget w = (ScrollbarWidget) gw;

    if (w->scrollbar.direction == 0) return;

    w->scrollbar.top = FractionLoc(w, event->xmotion.x, event->xmotion.y);
    PaintThumb(w);

/*    XFlush(XtDisplay(w)); */

    if (w->scrollbar.thumbProc)
       (*(w->scrollbar.thumbProc)) (w, w->scrollbar.closure, w->scrollbar.top);

    w->scrollbar.direction = 'S';
}



/* Public routines. */

/* Set the scroll bar to the given location. */
extern void XtScrollBarSetThumb( w, top, shown )
  ScrollbarWidget w;
  float top, shown;
{
    w->scrollbar.top = top;
    w->scrollbar.shown = shown;
    PaintThumb( w );
}
