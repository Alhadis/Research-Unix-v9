/* $Header: Clock.c,v 1.3 87/09/12 12:42:42 swick Exp $ */
#ifndef lint
static char *sccsid = "@(#)Clock.c	1.0	2/25/87";
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


#include <stdio.h>
#include <strings.h>
#include "Xlib.h"
#include "Xutil.h"
#include "Intrinsic.h"
#include "Clock.h"
#include "Atoms.h"
#include <sys/time.h>
#include <sys/file.h>
#include <sys/param.h>
#include <math.h>

extern long time();
	
/* Private Definitions */


#define VERTICES_IN_HANDS	6	/* to draw triangle */
#define PHI			(PI / 16.)	/* half angle of hand tip */
#define PI			3.14159265358979
#define PI4			(PI / 4.)
#define	SINPHI			0.19509032201613
#define TWOPI			(2. * PI)

#define SEG_BUFF_SIZE		128

#define SECOND_HAND_FRACT	90
#define MINUTE_HAND_FRACT	70
#define HOUR_HAND_FRACT		40
#define HAND_WIDTH_FRACT	7
#define SECOND_WIDTH_FRACT	5
#define SECOND_HAND_TIME	30

#define DEF_UPDATE		60

#define DEF_BORDER		2
#define DEF_VECTOR_HEIGHT	1
#define DEF_VECTOR_WIDTH	1

#define DEF_DIGITAL_PADDING	10
#define DEF_DIGITAL_FONT	"6x10"
#define DEF_ANALOG_PADDING	8
#define DEF_ANALOG_WIDTH	164
#define DEF_ANALOG_HEIGHT	164

#define UNINIT			-1
#define FAILURE			0

#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define abs(a) ((a) < 0 ? -(a) : (a))


typedef struct WidgetDataRec {
	Display *dpy;		/* widget display connection */
	 Window	mywin;          /* widget window */
	 Dimension	brwidth;	/* border width in pixels */
	 Dimension	width, height;	/* width/height in pixels */
	 Position	x, y;
	 Pixel	fgpixel;	/* color index for text */
	 Pixel	bgpixel;	/* color index for background */
	 Pixel	Hipixel;	/* color index for Highlighting */
	 Pixel	Hdpixel;	/* color index for hands */
	 Pixel	brpixel;	/* pixel for border */
	 XFontStruct	*fontstruct;	/* font for text */
	 GC	myGC;		/* pointer to GraphicsContext */
	 GC	EraseGC;	/* eraser GC */
	 GC	HandGC;		/* Hand GC */
	 GC	HighGC;		/* Highlighting GC */
/* start of graph stuff */
	 int	update;		/* update frequence */
	 Dimension radius;		/* radius factor */
	 int	chime;	
	 int	beeped;
	 int	analog;
	 int	show_second_hand;
	 Dimension second_hand_length;
	 Dimension minute_hand_length;
	 Dimension hour_hand_length;
	 Dimension hand_width;
	 Dimension second_hand_width;
	 Position centerX;
	 Position centerY;
	 int	numseg;
	 int	mapped;		/* is exposed */
	 int	padding;
	 XPoint	segbuff[SEG_BUFF_SIZE];
	 XPoint	*segbuffptr;
	 XPoint	*hour, *sec;
	 struct tm  otm ;
	 caddr_t cookie;
   } WidgetDataRec, *WidgetData;


/* Private Data */

static WidgetDataRec globaldata;

/* !!! STATIC !!! */
static int  initDone = 0;       /* initialization flag */

static XContext widgetContext;

/* bogus static's */
static int def_brwidth = DEF_BORDER;
static int def_width = 164;
static int def_height = 164;
static int def_x = 0;
static int def_y = 0;
static int def_update = DEF_UPDATE;
static int def_analog = 1;
static int def_chime = 0;
static int def_padding = DEF_ANALOG_PADDING;

static Resource resourcelist[] = {
    {XtNwindow, XtCWindow, XrmRWindow,
	sizeof(Window), (caddr_t)&globaldata.mywin, (caddr_t)NULL},
    {XtNborderWidth, XtCBorderWidth, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.brwidth, (caddr_t) &def_brwidth},
    {XtNwidth, XtCWidth, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.width, (caddr_t) &def_width},
    {XtNheight, XtCHeight, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.height, (caddr_t) &def_height },
    {XtNx, XtCX, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.x, (caddr_t) &def_x },
    {XtNy, XtCY, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.y, (caddr_t) &def_y },
    {XtNupdate, XtCInterval, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.update, (caddr_t) &def_update },
    {XtNchime, XtCChime, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.chime, (caddr_t) &def_chime },
    {XtNhand, XtCColor, XrmRPixel,
	sizeof(int), (caddr_t)&globaldata.Hdpixel, (caddr_t)&XtDefaultFGPixel},
    {XtNhigh, XtCColor, XrmRPixel,
	sizeof(int), (caddr_t)&globaldata.Hipixel, (caddr_t)&XtDefaultFGPixel},
    {XtNforeground, XtCColor, XrmRPixel,
	sizeof(int), (caddr_t)&globaldata.fgpixel, (caddr_t)&XtDefaultFGPixel},
    {XtNbackground, XtCColor, XrmRPixel,
	sizeof(int), (caddr_t)&globaldata.bgpixel, (caddr_t)&XtDefaultBGPixel},
    {XtNpadding, XtCPadding, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.padding, (caddr_t) &def_padding },
    {XtNborder, XtCColor, XrmRPixel,
	sizeof(int),(caddr_t)&globaldata.brpixel, (caddr_t)&XtDefaultFGPixel}
};

static Resource resourcelist1[] = {
    {XtNfont, XtCFont, XrmRFontStruct,
	sizeof(XFontStruct *), (caddr_t)&globaldata.fontstruct, (caddr_t)NULL},
    {XtNanalog, XtCAnalog, XrmRInt,
        sizeof(int), (caddr_t) &globaldata.analog, (caddr_t) &def_analog }
};

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

static void ClockInitialize (dpy)
    Display	*dpy;
{
    if (initDone) return;

    widgetContext = XUniqueContext();
    globaldata.fontstruct = XLoadQueryFont(dpy,"fixed");

    initDone = 1;
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

static void Destroy();

/*
 *
 * Generic widget event handler
 *
 */

static XtEventReturnCode EventHandler(event, eventdata)
XEvent *event;
caddr_t eventdata;
{
    WidgetData		data = (WidgetData ) eventdata;
    void clock_tic();

    switch (event->type) {
	case ConfigureNotify:
	    data->height = event->xconfigure.height;
	    data->width = event->xconfigure.width;
	    break;

	case DestroyNotify:
		Destroy(data);
	    break;
	
	case ClientMessage:
	    if (((XClientMessageEvent *)event)->message_type == 
		  (unsigned long) XtTimerExpired)
		  clock_tic(data);
	    break;
	
        case Expose:
	    data->mapped = 1;
	    if( ((XExposeEvent *)event)->count == 0)
	      redisplay_clock(data);
	    break;

	case NoExpose:
	    data->mapped = 0;
	    break;
    }

    return (XteventHandled);
}

static redisplay_clock(data) 
WidgetData	data;
{
    if(data->analog) {
      data->radius = (min(data->width, data->height)-(2 * data->padding)) / 2;
      data->second_hand_length = ((SECOND_HAND_FRACT * data->radius) / 100);
      data->minute_hand_length = ((MINUTE_HAND_FRACT * data->radius) / 100);
      data->hour_hand_length = ((HOUR_HAND_FRACT * data->radius) / 100);
      data->hand_width = ((HAND_WIDTH_FRACT * data->radius) / 100);
      data->second_hand_width = ((SECOND_WIDTH_FRACT * data->radius) / 100);
      data->centerX = data->width / 2;
      data->centerY = data->height / 2;
      DrawClockFace(data, data->second_hand_length, data->radius);
    }
    clock_tic(data);
}

/*
 *
 * Destroy the widget
 *
 */

static void Destroy(data)
WidgetData data;
{

    XtClearEventHandlers(data->dpy, data->mywin);
    (void) XtClearTimeOut(data->mywin, data->cookie);
    XtFree ((char *) data);
}

/****************************************************************
 *
 * Public Procedures
 *
 ****************************************************************/

Window XtCreateClock(dpy, pw, arglist, argCount)
Display *dpy;
Window pw;                      /* parent window */
ArgList arglist;
int argCount;
{
    WidgetData data;
    unsigned long eventmask;
    GCMask		valuemask;
    XrmNameList   names;
    XrmClassList	 classes;
    XGCValues	myXGCV;
    char *str;
    struct tm tm, *localtime();
    long time_value;

    if (!initDone)ClockInitialize (dpy);

    data = (WidgetData ) XtMalloc (sizeof (WidgetDataRec));
    globaldata.dpy = dpy;
 
    /* Set Some Default Values */
    XtGetResources(dpy, resourcelist1, XtNumber(resourcelist1), arglist,
		   argCount, pw, "clock", "Clock", &names, &classes);
    if(!globaldata.analog) {
      (void) time(&time_value);
      tm = *localtime(&time_value);
      str = asctime(&tm);
      def_width = XTextWidth( globaldata.fontstruct, str, strlen(str)) +
	2 * 10;
      def_height = globaldata.fontstruct->ascent +
	globaldata.fontstruct->descent + 2 * 10;
      def_padding = 10;
    }
    /* Set Default Values */
    XtGetResources(dpy, resourcelist, XtNumber(resourcelist), arglist,
		   argCount, pw, "clock", "Clock", &names, &classes);
    if(!globaldata.analog && globaldata.padding != def_padding) {
      def_width = XTextWidth( globaldata.fontstruct, str, strlen(str)) +
	2 * globaldata.padding;
      def_height = globaldata.fontstruct->ascent +
	globaldata.fontstruct->descent + 2 * globaldata.padding;
      XtGetResources(dpy, resourcelist, XtNumber(resourcelist), arglist,
		     argCount, pw, "clock", "Clock", &names, &classes);
    }
    *data = globaldata;
    valuemask = GCForeground | GCFont | GCBackground | GCLineWidth;
    myXGCV.foreground = (*data).fgpixel;
    myXGCV.font = (*data).fontstruct->fid;
    myXGCV.background = data->bgpixel;
    myXGCV.line_width = 0;
    (*data).myGC = XtGetGC(dpy, (XContext) XtNclock, pw, valuemask, &myXGCV);
    valuemask = GCForeground | GCLineWidth ;
    myXGCV.foreground = (*data).bgpixel;
    (*data).EraseGC = XtGetGC(dpy, (XContext) XtNclock, pw, valuemask, &myXGCV);
    myXGCV.foreground = (*data).Hdpixel;
    (*data).HandGC = XtGetGC(dpy, (XContext) XtNclock, pw, valuemask, &myXGCV);
    valuemask = GCForeground | GCLineWidth ;
    myXGCV.foreground = (*data).Hipixel;
    (*data).HighGC = XtGetGC(dpy, (XContext) XtNclock, pw, valuemask, &myXGCV);

    if (data->mywin != NULL) {
	XWindowAttributes wi;
	/* set global data from window parameters */
	if (! XGetWindowAttributes(data->dpy,data->mywin, &wi)) {
	    data->mywin = NULL;
	} else {
	    data->brwidth = wi.border_width;
	    data->width = wi.width;
	    data->height = wi.height;
	}
    }
    if (data->mywin == NULL) {
	/* create the Clock window */
	  data->mywin = XCreateSimpleWindow(data->dpy, pw, data->x, data->y,
			      data->width, data->height,
			      data->brwidth, data->brpixel, data->bgpixel);
    }

    XtSetNameAndClass(data->dpy, data->mywin, names, classes);
    XrmFreeNameList(names);
    XrmFreeClassList(classes);

    /* set handler for expose, resize, and message events */

    eventmask = ExposureMask+StructureNotifyMask;
    XtSetEventHandler (
	data->dpy, data->mywin,
	(XtEventHandler)EventHandler, eventmask, (caddr_t)data);

    XtSetTimeOut(data->mywin, XtNclock, data->update*1000);
    if (data->update <= SECOND_HAND_TIME)
      data->show_second_hand = TRUE;
    return (data->mywin);
}


static void clock_tic(data)
WidgetData data;
{
    
	struct tm *localtime();
	struct tm tm; 
	long	time_value;
	char	time_string[28];
	char	*time_ptr = time_string;

	(void) time(&time_value);
	tm = *localtime(&time_value);
	/*
	 * Beep on the half hour; double-beep on the hour.
	 */
	if (data->chime == TRUE) {
	    if (data->beeped && (tm.tm_min != 30) &&
		(tm.tm_min != 0))
	      data->beeped = 0;
	    if (((tm.tm_min == 30) || (tm.tm_min == 0)) 
		&& (!data->beeped)) {
		data->beeped = 1;
		XBell(data->dpy, 50);	
		if (tm.tm_min == 0)
		  XBell(data->dpy, 50);
	    }
	}
	if( data->analog == FALSE ) {
	    time_ptr = asctime(&tm);
	    time_ptr[strlen(time_ptr) - 1] = 0;
	    XDrawImageString(data->dpy, data->mywin, data->myGC,
			     2+data->padding
			     , 2+data->fontstruct->ascent+data->padding,
			     time_ptr, strlen(time_ptr));
	} else {
			/*
			 * The second (or minute) hand is sec (or min) 
			 * sixtieths around the clock face. The hour hand is
			 * (hour + min/60) twelfths of the way around the
			 * clock-face.  The derivation is left as an excercise
			 * for the reader.
			 */

			/*
			 * 12 hour clock.
			 */
			if(tm.tm_hour > 12)
				tm.tm_hour -= 12;

			/*
			 * Erase old hands.
			 */
			if(data->numseg > 0) {
			    if (data->show_second_hand == TRUE) {
				XDrawLines(data->dpy, data->mywin,
					data->EraseGC,
					data->sec,
					VERTICES_IN_HANDS-1,
					CoordModeOrigin);
				if(data->Hdpixel != data->bgpixel) {
				    XFillPolygon(data->dpy,
					data->mywin, data->EraseGC,
					data->sec,
					VERTICES_IN_HANDS-2,
					Convex, CoordModeOrigin
				    );
				}
			    }
			    if(	tm.tm_min != data->otm.tm_min ||
				tm.tm_hour != data->otm.tm_hour ) {
				XDrawLines( data->dpy,
					   data->mywin,
					   data->EraseGC,
					   data->segbuff,
					   VERTICES_IN_HANDS,
					   CoordModeOrigin);
				XDrawLines( data->dpy,
					   data->mywin,
					   data->EraseGC,
					   data->hour,
					   VERTICES_IN_HANDS,
					   CoordModeOrigin);
				if(data->Hdpixel != data->bgpixel) {
				    XFillPolygon( data->dpy,
					data->mywin, data->EraseGC,
					data->segbuff, VERTICES_IN_HANDS,
					Convex, CoordModeOrigin
				    );
				    XFillPolygon(data->dpy,
					data->mywin, data->EraseGC,
					data->hour,
					VERTICES_IN_HANDS,
					Convex, CoordModeOrigin
				    );
				}
			    }
		    }

		    if (data->numseg == 0 ||
			tm.tm_min != data->otm.tm_min ||
			tm.tm_hour != data->otm.tm_hour) {
			    data->segbuffptr = data->segbuff;
			    data->numseg = 0;
			    /*
			     * Calculate the hour hand, fill it in with its
			     * color and then outline it.  Next, do the same
			     * with the minute hand.  This is a cheap hidden
			     * line algorithm.
			     */
			    DrawHand(data,
				data->minute_hand_length, data->hand_width,
				((double) tm.tm_min)/60.0
			    );
			    if(data->Hdpixel != data->bgpixel)
				XFillPolygon( data->dpy,
				    data->mywin, data->HandGC,
				    data->segbuff, VERTICES_IN_HANDS,
				    Convex, CoordModeOrigin
				);
			    XDrawLines( data->dpy,
				data->mywin, data->HighGC,
				data->segbuff, VERTICES_IN_HANDS,
				       CoordModeOrigin);
			    data->hour = data->segbuffptr;
			    DrawHand(data, 
				data->hour_hand_length, data->hand_width,
				((((double)tm.tm_hour) + 
				    (((double)tm.tm_min)/60.0)) / 12.0)
			    );
			    if(data->Hdpixel != data->bgpixel) {
			      XFillPolygon( data->dpy,
					   data->mywin, data->HandGC,
					   data->hour,
					   VERTICES_IN_HANDS,
					   Convex, CoordModeOrigin
					   );
			    }
			    XDrawLines( data->dpy,
				       data->mywin, data->HighGC,
				       data->hour, VERTICES_IN_HANDS,
				       CoordModeOrigin );

			    data->sec = data->segbuffptr;
		    }
		    if (data->show_second_hand == TRUE) {
			    data->segbuffptr = data->sec;
			    DrawSecond(data,
				data->second_hand_length - 2, 
				data->second_hand_width,
				data->minute_hand_length + 2,
				((double) tm.tm_sec)/60.0
			    );
			    if(data->Hdpixel != data->bgpixel)
				XFillPolygon( data->dpy,
				    data->mywin, data->HandGC,
				    data->sec,
				    VERTICES_IN_HANDS -2,
				    Convex, CoordModeOrigin
			    );
			    XDrawLines( data->dpy,
				       data->mywin, data->HighGC,
				       data->sec,
				       VERTICES_IN_HANDS-1,
				       CoordModeOrigin
				        );
			    /*
			     * keep the pointer from walking too far.
			     */
			    data->segbuffptr = data->sec;
			    data->numseg = data->segbuffptr-data->segbuff;
			}
			data->otm = tm;
			
		}
}
	
/*
 * DrawLine - Draws a line.
 *
 * blank_length is the distance from the center which the line begins.
 * length is the maximum length of the hand.
 * Fraction_of_a_circle is a fraction between 0 and 1 (inclusive) indicating
 * how far around the circle (clockwise) from high noon.
 *
 * The blank_length feature is because I wanted to draw tick-marks around the
 * circle (for seconds).  The obvious means of drawing lines from the center
 * to the perimeter, then erasing all but the outside most pixels doesn't
 * work because of round-off error (sigh).
 */
static DrawLine(data, blank_length, length, fraction_of_a_circle)
WidgetData	data;
Dimension blank_length;
Dimension length;
double fraction_of_a_circle;
{
	register double angle, cosangle, sinangle;
	double cos();
	double sin();

	/*
	 *  A full circle is 2 PI radians.
	 *  Angles are measured from 12 o'clock, clockwise increasing.
	 *  Since in X, +x is to the right and +y is downward:
	 *
	 *	x = x0 + r * sin(theta)
	 *	y = y0 - r * cos(theta)
	 *
	 */
	angle = TWOPI * fraction_of_a_circle;
	cosangle = cos(angle);
	sinangle = sin(angle);

	SetSeg( data, 
	       data->centerX + (int)(blank_length * sinangle),
	       data->centerY - (int)(blank_length * cosangle),
	       data->centerX + (int)(length * sinangle),
	       data->centerY - (int)(length * cosangle));
}

/*
 * DrawHand - Draws a hand.
 *
 * length is the maximum length of the hand.
 * width is the half-width of the hand.
 * Fraction_of_a_circle is a fraction between 0 and 1 (inclusive) indicating
 * how far around the circle (clockwise) from high noon.
 *
 */
static DrawHand(data, length, width, fraction_of_a_circle)
WidgetData	data;
Dimension length, width;
double fraction_of_a_circle;
{

	register double angle, cosangle, sinangle;
	register double ws, wc;
	Position x, y, x1, y1, x2, y2;
	double cos();
	double sin();

	/*
	 *  A full circle is 2 PI radians.
	 *  Angles are measured from 12 o'clock, clockwise increasing.
	 *  Since in X, +x is to the right and +y is downward:
	 *
	 *	x = x0 + r * sin(theta)
	 *	y = y0 - r * cos(theta)
	 *
	 */
	angle = TWOPI * fraction_of_a_circle;
	cosangle = cos(angle);
	sinangle = sin(angle);
	/*
	 * Order of points when drawing the hand.
	 *
	 *		1,4
	 *		/ \
	 *	       /   \
	 *	      /     \
	 *	    2 ------- 3
	 */
	wc = width * cosangle;
	ws = width * sinangle;
	SetSeg( data,
	       x = data->centerX + round(length * sinangle),
	       y = data->centerY - round(length * cosangle),
	       x1 = data->centerX - round(ws + wc), 
	       y1 = data->centerY + round(wc - ws));  /* 1 ---- 2 */
	/* 2 */
	SetSeg( data, x1, y1, 
	       x2 = data->centerX - round(ws - wc), 
	       y2 = data->centerY + round(wc + ws));  /* 2 ----- 3 */

	SetSeg( data, x2, y2, x, y);	/* 3 ----- 1(4) */
}

/*
 * DrawSecond - Draws the second hand (diamond).
 *
 * length is the maximum length of the hand.
 * width is the half-width of the hand.
 * offset is direct distance from center to tail end.
 * Fraction_of_a_circle is a fraction between 0 and 1 (inclusive) indicating
 * how far around the circle (clockwise) from high noon.
 *
 */
static DrawSecond(data, length, width, offset, fraction_of_a_circle)
WidgetData data;
Dimension length, width, offset;
double fraction_of_a_circle;
{

	register double angle, cosangle, sinangle;
	register double ms, mc, ws, wc;
	register int mid;
	Position x, y;
	double cos();
	double sin();

	/*
	 *  A full circle is 2 PI radians.
	 *  Angles are measured from 12 o'clock, clockwise increasing.
	 *  Since in X, +x is to the right and +y is downward:
	 *
	 *	x = x0 + r * sin(theta)
	 *	y = y0 - r * cos(theta)
	 *
	 */
	angle = TWOPI * fraction_of_a_circle;
	cosangle = cos(angle);
	sinangle = sin(angle);
	/*
	 * Order of points when drawing the hand.
	 *
	 *		1,5
	 *		/ \
	 *	       /   \
	 *	      /     \
	 *	    2<       >4
	 *	      \     /
	 *	       \   /
	 *		\ /
	 *	-	 3
	 *	|
	 *	|
	 *   offset
	 *	|
	 *	|
	 *	-	 + center
	 */

	mid = (length + offset) / 2;
	mc = mid * cosangle;
	ms = mid * sinangle;
	wc = width * cosangle;
	ws = width * sinangle;
	/*1 ---- 2 */
	SetSeg( data,
	       x = data->centerX + round(length * sinangle),
	       y = data->centerY - round(length * cosangle),
	       data->centerX + round(ms - wc),
	       data->centerY - round(mc + ws) );
	SetSeg( data, data->centerX + round(offset *sinangle),
	       data->centerY - round(offset * cosangle), /* 2-----3 */
	       data->centerX + round(ms + wc), 
	       data->centerY - round(mc - ws));
	data->segbuffptr->x = x;
	data->segbuffptr++->y = y;
	data->numseg ++;
}

static SetSeg( data, x1, y1, x2, y2)
WidgetData	data;
int x1, y1, x2, y2;
{
	data->segbuffptr->x = x1;
	data->segbuffptr++->y = y1;
	data->segbuffptr->x = x2;
	data->segbuffptr++->y = y2;
	data->numseg += 2;
}

/*
 *  Draw the clock face (every fifth tick-mark is longer
 *  than the others).
 */
static DrawClockFace(data, second_hand, radius)
WidgetData data;
Dimension second_hand;
Dimension radius;
{
	register int i;
	register int delta = (radius - second_hand) / 3;
	
	XClearWindow(data->dpy, data->mywin);
	data->segbuffptr = data->segbuff;
	data->numseg = 0;
	for (i = 0; i < 60; i++)
		DrawLine(data, (i % 5) == 0 ? second_hand : (radius - delta), radius,
		 ((double) i)/60.);
	/*
	 * Go ahead and draw it.
	 */
	XDrawSegments(data->dpy, data->mywin, 
		      data->myGC, (XSegment *) &(data->segbuff[0]),
		      data->numseg/2);
	
	data->segbuffptr = data->segbuff;
	data->numseg = 0;
}

static round(x)
double x;
{
	return(x >= 0 ? (int)(x + .5) : (int)(x - .5));
}


/*
 *
 * Get Attributes
 *
 */

void XtClockGetValues (dpy, window, arglist, argCount)
Display *dpy;
Window window;
ArgList arglist;
int argCount;
{
    WidgetData data;
    data = DataFromWindow(dpy, window);
    if (data == NULL) return;
    globaldata = *data;
    XtGetValues(resourcelist, XtNumber(resourcelist), arglist, argCount);
}

/*
 *
 * Set Attributes
 *
 */

void XtClockSetValues (dpy, window, arglist, argCount)
Display *dpy;
Window window;
ArgList arglist;
int argCount;
{
      WidgetData data;
      int redisplay = FALSE;
      GCMask valuemask;
      XGCValues	myXGCV;
      
      
      data = DataFromWindow(dpy, window);
      if (data == NULL) return;
      globaldata = *data;
      XtSetValues(resourcelist, XtNumber(resourcelist), arglist, argCount);

      if(globaldata.update != data->update) {
	    (void) XtClearTimeOut(data->mywin, data->cookie);
	    XtSetTimeOut(data->mywin, data->cookie, globaldata.update*1000);
	    data->update = globaldata.update;
	    if (data->update <= SECOND_HAND_TIME)
	         data->show_second_hand = TRUE;

      }
      if (globaldata.brpixel != data->brpixel) {
	data->brpixel = globaldata.brpixel;
	if (data->brwidth != 0)
	  XSetWindowBorder(data->dpy, data->mywin, data->brpixel);
	redisplay = TRUE;
      }
      if ((globaldata.bgpixel != data->bgpixel) || 
	  (globaldata.fgpixel != data->fgpixel) ||
	  (globaldata.Hdpixel != data->Hdpixel) ||
	  (globaldata.Hipixel != data->Hipixel)) {
	  redisplay = TRUE;
	  if((globaldata.bgpixel != data->bgpixel) || 
	     (globaldata.fgpixel != data->fgpixel)) {
	        valuemask = GCForeground | GCFont | GCBackground | GCLineWidth;
		myXGCV.foreground = (*data).fgpixel;
		myXGCV.font = (*data).fontstruct->fid;
		myXGCV.background = data->bgpixel;
		data->myGC = XtGetGC(
			data->dpy, (XContext) XtNclock, data->mywin, valuemask, &myXGCV);
	  }
	  if (globaldata.bgpixel != data->bgpixel) {
	        valuemask = GCForeground | GCLineWidth;
		myXGCV.foreground = (*data).bgpixel;
		data->EraseGC = XtGetGC(
			data->dpy, (XContext) XtNclock, data->mywin, valuemask, &myXGCV);
	  }
	  if (globaldata.Hdpixel != data->Hdpixel) {
	        valuemask = GCForeground | GCLineWidth;
	        myXGCV.foreground = (*data).Hdpixel;
		data->HandGC = XtGetGC(
			data->dpy, (XContext) XtNclock, data->mywin, valuemask, &myXGCV);
	  }
	  if (globaldata.Hipixel != data->Hipixel) {
	         valuemask = GCForeground | GCLineWidth;
		 myXGCV.foreground = (*data).Hipixel;
		 data->HighGC = XtGetGC(
			data->dpy, (XContext) XtNclock, data->mywin, valuemask, &myXGCV);
	  }
     }
      *data = globaldata;
      if(redisplay == TRUE) {
	XClearWindow(data->dpy, data->mywin);
	redisplay_clock(data);
      }
}


