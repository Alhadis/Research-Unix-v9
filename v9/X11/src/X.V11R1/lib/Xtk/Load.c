#ifndef lint
static char rcsid[] = "$Header: Load.c,v 1.12 87/09/13 22:50:22 newman Exp $";
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
#ifndef lint
static char *sccsid = "@(#)Load.c	1.15	2/25/87";
#endif lint

#include <stdio.h>
#include <string.h>
#include "Xlib.h"
#include "Xutil.h"
#include "Intrinsic.h"
#include "Load.h"
#include "LoadP.h"
#include "Atoms.h"
#include <nlist.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/param.h>

extern long lseek();
extern void exit();

/* Private Data */

#define offset(field) XtOffset(LoadWidget,load.field)
#define goffset(field) XtOffset(Widget,core.field)

static XtResource resources[] = {
    {XtNwidth, XtCWidth, XrmRInt, sizeof(int),
	goffset(width), XrmRString, "120"},
    {XtNheight, XtCHeight, XrmRInt, sizeof(int),
	goffset(height), XrmRString, "120"},
    {XtNupdate, XtCInterval, XrmRInt, sizeof(int),
        offset(update), XrmRString, "5"},
    {XtNscale, XtCScale, XrmRInt, sizeof(int),
        offset(scale), XrmRString, "1"},
    {XtNminScale, XtCScale, XrmRInt, sizeof(int),
        offset(min_scale), XrmRString, "1"},
    {XtNlabel, XtCLabel, XrmRString, sizeof(char *),
        offset(text), XrmRString, "Amnesia"},
    {XtNforeground, XtCForeground, XrmRPixel, sizeof(Pixel),
        offset(fgpixel), XrmRString, "Black"},
    {XtNfont, XtCFont, XrmRFontStruct, sizeof(XFontStruct *),
        offset(font), XrmRString, "fixed"},
};

#undef offset
#undef goffset

static void Initialize(), Realize(), Destroy(), Redisplay();
static Boolean SetValues();
static int repaint_window();

LoadClassRec loadClassRec = {
    { /* core fields */
    /* superclass */	   &widgetClassRec,
    /* class_name */       "Load",
    /* size */		   sizeof(LoadRec),
    /* class_initialize */ NULL,
    /* class_inited */     FALSE,
    /* initialize */	   Initialize,
    /* realize */	   Realize,
    /* actions */	   NULL,
    /* num_actions */      0,
    /* resources */	   resources,
    /* resource_count*/    XtNumber(resources),
    /* xrm_class */        NULL,
    /* compress_motion */  TRUE,
    /* compress_exposure*/ TRUE,
    /* visible_interest */ FALSE,
    /* destroy */          Destroy,
    /* resize */           NULL,
    /* expose */	   Redisplay,
    /* set_values */       SetValues,
    /* accept_focus */     NULL,
    }
};

WidgetClass loadWidgetClass = (WidgetClass) &loadClassRec;

/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

static void draw_it();

static void EventHandler(gw, closure, event)
    Widget gw;
    char *closure;
    XEvent *event;
{
    if (event->type == ClientMessage && event->xclient.message_type == XtTimerExpired)
        draw_it ((LoadWidget)gw);
}

static void Initialize (greq, gnew)
    Widget greq, gnew;
{
    LoadWidget w = (LoadWidget)gnew;
    XtGCMask	valuemask;
    XGCValues	myXGCV;

    valuemask = GCForeground | GCFont | GCBackground;
    myXGCV.foreground = w->load.fgpixel;
    myXGCV.font = w->load.font->fid;
    myXGCV.background = w->core.background_pixel;
    w->load.myGC = XtGetGC(w, valuemask, &myXGCV);

    XtAddEventHandler (gnew, 0, TRUE, EventHandler, NULL);

    w->load.interval_id = XtAddTimeOut(gnew, w->load.update*1000);
    w->load.interval = 0;
    w->load.max_value = 0.0;
}
 
static void Realize (gw, valueMask, attrs)
     Widget gw;
     XtValueMask valueMask;
     XSetWindowAttributes *attrs;
{
     gw->core.window = XCreateWindow (XtDisplay(gw), gw->core.parent->core.window,
	  gw->core.x, gw->core.y, gw->core.width, gw->core.height, gw->core.border_width,
          gw->core.depth, InputOutput, /* visualID */ CopyFromParent, valueMask, attrs);
}

static void Destroy (gw)
     Widget gw;
{
     LoadWidget w = (LoadWidget)gw;
     XtRemoveTimeOut (w->load.interval_id);
     XtDestroyGC (w->load.myGC);
}

static void Redisplay(gw)
     Widget gw;
{
     (void) repaint_window ((LoadWidget)gw);
}

static void draw_it(w)
LoadWidget w;
{
	double value, GetLoadPoint();

	if (w->load.interval >= w->core.width) 
	  w->load.interval = 
	    repaint_window(w);
	/* Get the value, stash the point and draw corresponding line. */
     
	value = GetLoadPoint();
	/* Keep w->load.max_value up to date, and if this data point is off the
	   graph, change the scale to make it fit. */
	if (value > w->load.max_value) {
	    w->load.max_value = value;
	    if (w->load.max_value > w->load.scale) {
		w->load.scale = ((int)w->load.max_value) + 1;
		w->load.interval = 
		  repaint_window(w);
	    }
	}

	w->load.valuedata[w->load.interval] = value;
	if (XtIsRealized(w)) {
	    XDrawLine(XtDisplay(w), XtWindow(w), w->load.myGC,
		  w->load.interval, w->core.height, w->load.interval, 
		    (int)(w->core.height - (w->core.height * value) /w->load.scale));
	    XFlush(XtDisplay(w));		    /* Flush output buffers */
	}
	w->load.interval++;		    /* Next point */
} /* draw_it */

/* Blts data according to current size, then redraws the load average window.
 * Next represents the number of valid points in data.  Returns the (possibly)
 * adjusted value of next.  If next is 0, this routine draws an empty window
 * (scale - 1 lines for graph).  If next is less than the current window width,
 * the returned value is identical to the initial value of next and data is
 * unchanged.  Otherwise keeps half a window's worth of data.  If data is
 * changed, then w->load.max_value is updated to reflect the largest data point.
 */

static int repaint_window(w)
LoadWidget w;
{
    register int i, j;
    register int next = w->load.interval;
    extern void bcopy();

    if (next >= w->core.width) {
	j = w->core.width >> 1;
	bcopy((char *)(w->load.valuedata + next - j),
	      (char *)(w->load.valuedata), j * sizeof(double));
	next = j;
	/* Since we just lost some data, recompute the w->load.max_value. */
	w->load.max_value = 0.0;
	for (i = 0; i < next; i++) {
	    if (w->load.valuedata[i] > w->load.max_value) 
	      w->load.max_value = w->load.valuedata[i];
	}
    }

    /* Compute the minimum scale required to graph the data, but don't go
       lower than min_scale. */
    if (w->load.max_value > w->load.min_scale) 
      w->load.scale = ((int)w->load.max_value) + 1;
    else 
      w->load.scale = w->load.min_scale;

    if (XtIsRealized(w)) {
	Display *dpy = XtDisplay(w);
	Window win = XtWindow(w);

	XClearWindow(dpy, win);

	/* Print hostname */
	XDrawString(dpy, win, w->load.myGC, 2, 
	      2 + w->load.font->ascent, w->load.text, strlen(w->load.text));

	/* Draw graph reference lines */
	for (i = 1; i < w->load.scale; i++) {
	    j = (i * w->core.height) / w->load.scale;
	    XDrawLine(dpy, win, w->load.myGC, 0, j,
		      (int)w->core.width, j);
	}

	/* Draw data point lines. */
	for (i = 0; i < next; i++)
	    XDrawLine(dpy, win, w->load.myGC, i, w->core.height,
		  i, (int)(w->core.height-(w->load.valuedata[i] * w->core.height)
			  /w->load.scale));
        }

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
    (void) fprintf(stderr,"xload: %s\n",str);
    exit(-1);
}

static Boolean SetValues (w, newvals)
   Widget w, newvals;
{
return (FALSE);
}

#ifdef notdef
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
    if(w->core.width >= 2048) w->core.width = 2047;
      
    *data = globaldata;
}
#endif
