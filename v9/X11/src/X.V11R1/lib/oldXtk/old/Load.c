/* $Header: Load.c,v 1.2 87/09/12 12:43:16 swick Exp $ */
#ifndef lint
static char *sccsid = "@(#)Load.c	1.15	2/25/87";
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
#include "Xutil.h"
#include "Intrinsic.h"
#include "Load.h"
#include "Atoms.h"
#include <nlist.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/param.h>

extern long lseek();

/* Private Definitions */

typedef struct WidgetDataRec {
	Display *dpy;		/* widget display connection */
	 Window	window;          /* widget window */
	 Dimension brwidth;	/* border width in pixels */
	 Dimension width, height;	/* width/height in pixels */
	 Position x, y;
	 Pixel	fgpixel;	/* color index for text */
	 Pixel	bgpixel;	/* color index for background */
	 Pixel	brpixel;	/* pixel for border */
	 XFontStruct	*fontstruct;	/* font for text */
	 GC	myGC;		/* pointer to GraphicsContext */
/* start of graph stuff */
	 int	update;		/* update frequence */
	 int	scale;		/* scale factor */
	 int	 min_scale;	/* smallest scale factor */
	 int	 interval;	/* data point interval */
	 char	*text;		/* label */
	 double max_value;	/* Max Value in window */
	 int	mapped;		/* is exposed */
	 double valuedata[2048];/* record of data points */
/* start of xload stuff
   	 char *vmunix;		/* path of namelist */
	 caddr_t cookie;
   } WidgetDataRec, *WidgetData;


/* Private Data */

static WidgetDataRec globaldata;

static int  initDone = 0;       /* initialization flag */
static XContext widgetContext;
static int def_border = 1;
static int def_width = 120;
static int def_height = 120;
static int def_update = 5;
static int def_x = 0;
static int def_y = 0;
static int def_scale = 1;

static Resource resourcelist[] = {
    {XtNwindow, XtCWindow, XrmRWindow,
	sizeof(Window), (caddr_t)&globaldata.window, (caddr_t)NULL},
    {XtNborderWidth, XtCBorderWidth, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.brwidth, (caddr_t) &def_border},
    {XtNwidth, XtCWidth, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.width, (caddr_t) &def_width},
    {XtNheight, XtCHeight, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.height, (caddr_t) &def_height },
    {XtNx, XtCX, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.x, (caddr_t) &def_x },
    {XtNy, XtCX, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.y, (caddr_t) &def_y },
    {XtNupdate, XtCInterval, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.update, (caddr_t) &def_update },
    {XtNscale, XtCScale, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.scale, (caddr_t) &def_scale },
    {XtNminScale, XtCScale, XrmRInt,
	sizeof(int), (caddr_t)&globaldata.min_scale, (caddr_t) &def_scale },
    {XtNlabel, XtCLabel, XrmRString,
	sizeof(char *), (caddr_t)&globaldata.text, (caddr_t) "Amnesia" },
    {XtNforeground, XtCColor, XrmRPixel,
	sizeof(int), (caddr_t)&globaldata.fgpixel, (caddr_t)&XtDefaultFGPixel},
    {XtNbackground, XtCColor, XrmRPixel,
	sizeof(int), (caddr_t)&globaldata.bgpixel, (caddr_t)&XtDefaultBGPixel},
    {XtNborder, XtCColor, XrmRPixel,
	sizeof(int),(caddr_t)&globaldata.brpixel, (caddr_t)&XtDefaultFGPixel},
    {XtNfont, XtCFont, XrmRFontStruct,
	sizeof(XFontStruct *), (caddr_t)&globaldata.fontstruct, (caddr_t)NULL}
};


/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

static void LoadInitialize (dpy)
Display	*dpy;
{
    if(!initDone)
      globaldata.fontstruct = XLoadQueryFont(dpy,"fixed");
    widgetContext = XUniqueContext();

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
		  (unsigned) XtTimerExpired)
		  draw_it(data);
	    break;
	
        case Expose:
	    data->mapped = 1;
	    if( ((XExposeEvent *)event)->count == 0)
	   	 data->interval = repaint_window(data);
	    break;

	  case NoExpose:
	    data->mapped = 0;
	    break;
    }

    return (XteventHandled);
}

/*
 *
 * Destroy the widget
 *
 */

static void Destroy(data)
WidgetData data;
{

    XtClearEventHandlers(data->dpy, data->window);
    (void) XtClearTimeOut(data->window, data->cookie);
    XtFree ((char *) data);
}



/****************************************************************
 *
 * Public Procedures
 *
 ****************************************************************/

Window XtLoadCreate(dpy, pw, arglist, argCount)
Display *dpy;
Window pw;                      /* parent window */
ArgList arglist;
int argCount;
{
    WidgetData data;
    int		eventmask;
    GCMask	valuemask;
    XrmNameList   names;
    XrmClassList	 classes;
    XGCValues	myXGCV;
    if (!initDone)LoadInitialize (dpy);

    data = (WidgetData ) XtMalloc (sizeof (WidgetDataRec));

    /* Set Default Values */
    globaldata.dpy = dpy;
    XtGetResources(dpy,  resourcelist, XtNumber(resourcelist), arglist, argCount, pw,
	"load", "Load",	&names, &classes);
    *data = globaldata;
    valuemask = GCForeground | GCFont | GCBackground;
    myXGCV.foreground = (*data).fgpixel;
    myXGCV.font = (*data).fontstruct->fid;
    myXGCV.background = data->bgpixel;
    (*data).myGC = XtGetGC(data->dpy, widgetContext, pw, valuemask, &myXGCV);

    if (data->window != NULL) {
	XWindowAttributes wi;
	/* set global data from window parameters */
	if (! XGetWindowAttributes(data->dpy,data->window, &wi)) {
	    data->window = NULL;
	} else {
	    data->brwidth = wi.border_width;
	    data->width = wi.width;
	    data->height = wi.height;
	}
    }
    if (data->window == NULL) {
	/* create the Load window */
	  if(data->width >= 2048) data->width = 2047;
	  data->window = XCreateSimpleWindow(data->dpy, pw, data->x, data->y,
			      data->width, data->height,
			      data->brwidth, data->brpixel, data->bgpixel);
    }

    XtSetNameAndClass(data->dpy, data->window, names, classes);
    XrmFreeNameList(names);
    XrmFreeClassList(classes);

    (void)XSaveContext(data->dpy, data->window, widgetContext, (caddr_t)data);

    /* set handler for expose, resize, and message events */

    eventmask = ExposureMask+StructureNotifyMask;
    XtSetEventHandler (
	data->dpy, data->window, (XtEventHandler)EventHandler,
	(unsigned long) eventmask, (caddr_t)data);

    XtSetTimeOut(data->window, XtNLoad,data->update*1000);

    return (data->window);
}
 
/*
 * Get Attributes
 */

void XtLoadGetValues (dpy, window, arglist, argCount)
Display *dpy;
Window window;
ArgList arglist;
int argCount;
{
    WidgetData data;
    data = DataFromWindow(dpy, window);
    if (data) {
	globaldata = *data;
	XtGetValues(resourcelist, XtNumber(resourcelist), arglist, argCount);
    }
}

/*
 * Set Attributes
 */

void XtLoadSetValues (dpy, window, arglist, argCount)
Display *dpy;
Window window;
ArgList arglist;
int argCount;
{
    WidgetData data;
    data = DataFromWindow(dpy, window);
    globaldata = *data;
    if (data == NULL) return;

    XtSetValues(resourcelist, XtNumber(resourcelist), arglist, argCount);

    if (globaldata.update != data->update) {
	(void) XtClearTimeOut(data->window, data->cookie);
	XtSetTimeOut(data->window, data->cookie, globaldata.update*1000);
	data->update = globaldata.update;
    }
    if (globaldata.brpixel != data->brpixel) {
	data->brpixel = globaldata.brpixel;
	if (data->brwidth != 0)
	    XSetWindowBorder(data->dpy, data->window, data->brpixel);
    }
    if(!strcmp(globaldata.text, data->text)) {
	data->interval = repaint_window(data);
    }
    if(data->width >= 2048) data->width = 2047;
      
    *data = globaldata;
}


static draw_it(data)
WidgetDataRec *data;
{
	double value, GetLoadPoint();

	if (data->interval >= data->width) 
	  data->interval = 
	    repaint_window(data);
	/* Get the value, stash the point and draw corresponding line. */
     
	value = GetLoadPoint();
	/* Keep data->max_value up to date, and if this data point is off the
	   graph, change the scale to make it fit. */
	if (value > data->max_value) {
	    data->max_value = value;
	    if (data->max_value > data->scale) {
		data->scale = ((int)data->max_value) + 1;
		data->interval = 
		  repaint_window(data);
	    }
	}

	data->valuedata[data->interval] = value;
	if (data->mapped) {

	    XDrawLine(data->dpy, data->window, data->myGC,
		  data->interval, (int)data->height, data->interval, 
		    (int)(data->height - (data->height * value) /data->scale));
	    XFlush(data->dpy);		    /* Flush output buffers */
	}
	data->interval++;		    /* Next point */
} /* draw_it */

/* Blts data according to current size, then redraws the load average window.
 * Next represents the number of valid points in data.  Returns the (possibly)
 * adjusted value of next.  If next is 0, this routine draws an empty window
 * (scale - 1 lines for graph).  If next is less than the current window width,
 * the returned value is identical to the initial value of next and data is
 * unchanged.  Otherwise keeps half a window's worth of data.  If data is
 * changed, then data->max_value is updated to reflect the largest data point.
 */

static int repaint_window(data)
WidgetDataRec *data;
{
    register int i, j;
    register int next = data->interval;
    extern void bcopy(), exit();

    if (data->mapped)
	XClearWindow(data->dpy, data->window);
    if (next >= data->width) {
	j = data->width >> 1;
	bcopy((char *)(data->valuedata + next - j),
	      (char *)(data->valuedata), j * sizeof(double));
	next = j;
	/* Since we just lost some data, recompute the data->max_value. */
	data->max_value = 0.0;
	for (i = 0; i < next; i++) {
	    if (data->valuedata[i] > data->max_value) 
	      data->max_value = data->valuedata[i];
	}
    }

    /* Compute the minimum scale required to graph the data, but don't go
       lower than min_scale. */
    if (data->max_value > data->min_scale) 
      data->scale = ((int)data->max_value) + 1;
    else 
      data->scale = data->min_scale;

    if (!data->mapped) return(next);

    /* Print hostname */
    XDrawString(data->dpy, data->window, data->myGC, 2, 
	  2 + data->fontstruct->ascent, data->text, strlen(data->text));

    /* Draw graph reference lines */
    for (i = 1; i < data->scale; i++) {
	j = (i * data->height) / data->scale;
	XDrawLine(data->dpy, data->window, data->myGC, 0, j,
		  (int)data->width, j);
    }

    /* Draw data point lines. */
    for (i = 0; i < next; i++)
    	XDrawLine(data->dpy, data->window, data->myGC , i, (int)data->height,
	      i, (int)(data->height-(data->valuedata[i] * data->height)
		      /data->scale));
    return(next);
}

#define KMEM_FILE "/dev/kmem"
#define KMEM_ERROR "cannot open /dev/kmem"

static struct nlist namelist[] = {	    /* namelist for vmunix grubbing */
#define LOADAV 0
    {"_avenrun"},
    {0}
};

static double GetLoadPoint()
{
  	double loadavg;
	static int init = 0;
	static kmem;
	static long loadavg_seek;
	extern void nlist();
	
	if(!init)   {
	    nlist( "/vmunix", namelist);
	    if (namelist[LOADAV].n_type == 0){
		xload_error("xload: cannot get name list");
		exit(-1);
	    }
	    loadavg_seek = namelist[LOADAV].n_value;
	    kmem = open(KMEM_FILE, O_RDONLY);
	    if (kmem < 0) xload_error(KMEM_ERROR);
	    init = 1;
	}
	

	(void) lseek(kmem, loadavg_seek, 0);
#ifdef	sun
	{
		long temp;
		(void) read(kmem, (char *)&temp, sizeof(long));
		loadavg = (double)temp/FSCALE;
	}
#else
	(void) read(kmem, (char *)&loadavg, sizeof(double));
#endif
	return(loadavg);
}

static xload_error(str)
char *str;
{
    extern void exit();

    (void) fprintf(stderr,"xload: %s\n",str);
    exit(-1);
}
