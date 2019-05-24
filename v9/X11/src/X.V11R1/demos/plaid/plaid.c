/* 
 * $Locker: sun $ 
 */ 
static char	*rcsid = "$Header: plaid.c,v 1.2 87/09/07 16:07:45 sun Locked $";
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
#include "X11/Xlib.h"
#include "X11/Xatom.h"
#include <stdio.h>
#include <errno.h>

extern int errno;

int rand();
char *malloc();

Window XCreateWindow();

Window myWin, newWin, threeWin;

Display *dpy;

#define NUMRECTS 10
XRectangle rects[NUMRECTS];
GContext gc;

main(argc, argv)
int	argc;
char	*argv[];
{
    int amount, i, j ;
    char *geom = NULL;
    int winx, winy, winw, winh;
    register int xdir, ydir;
    register int xoff, yoff;
    register int centerX, centerY;
    XGCValues xgcv;
    XSetWindowAttributes xswa;
    XEvent pe;
    XExposeEvent *ee;
    Window root;
    int x, y, w, h;
    unsigned int bw, d;
    char *display;
    int bs = NotUseful;
    Visual visual;

    for (i=1; i < argc; i++)
    {
	if (argv [i] [0] == '=') 
	        geom = argv[i];
	else if (index(argv[i], ':') != NULL)
	    display = argv[i];
	else if (argv[i][0] == 'b')
	    bs = Always;
    }

    if (!(dpy = XOpenDisplay(display)))
    {
	perror("Cannot open display\n");
	exit(-1);
    }

    winx = 0;
    winy = 0;
    winw = 101;
    winh = 201;

    if (geom) 
    {
        if (XParseGeometry(geom, &winx, &winy, &winw, &winh))
	{
	    printf("Geometry specified incorrectly.  Using default\n");
	}
    }

    xswa.backing_store = bs;
    xswa.event_mask = ExposureMask | StructureNotifyMask;
    xswa.background_pixel = BlackPixel(dpy, DefaultScreen(dpy));
    xswa.border_pixel = WhitePixel(dpy, DefaultScreen(dpy));
    visual.visualid = CopyFromParent;
    myWin = XCreateWindow(dpy,
		RootWindow(dpy, DefaultScreen(dpy)),
		winx, winy, winw, winh, 1, 
		DefaultDepth(dpy, DefaultScreen(dpy)), InputOutput,
		&visual, 
	        CWEventMask | CWBackingStore | CWBorderPixel | CWBackPixel, 
		&xswa);

    XChangeProperty(dpy, myWin, XA_WM_NAME, XA_STRING, 8,
		PropModeReplace, "Plaid", 5);
    XMapWindow(dpy, myWin);

    xgcv.function = GXinvert;
    xgcv.fill_style = FillSolid;
    gc = (GContext)XCreateGC(dpy, myWin, GCFunction | GCFillStyle, &xgcv);
    j=0;
    while(1)
    {
        XNextEvent(dpy, &pe);	/* this should get first exposure event */    
	if (pe.type == Expose)
	{
	    ee = (XExposeEvent *) &pe;
	    while (ee->count)
	    {
                XNextEvent(dpy, &pe);	    
	        ee = (XExposeEvent *) &pe;
	    }
	}
	else if (pe.type == ConfigureNotify)
	{
	    XConfigureEvent *ce = (XConfigureEvent *)&pe;
	    winx = ce->x;
	    winy = ce->y;
	    winw = ce->width;
	    winh = ce->height;
	}
        else 
	    printf("Unknown event type: %d\n", pe.type);

	printf("PLAID: Dealing with exposures\n");	
	XClearArea(dpy, myWin, 0, 0, winw, winh, 0);
        printf("PLAID: drawing rects\n");

	centerX = winw / 2;
	centerY = winh / 2;
	xdir = ydir = -2;
	xoff = yoff = 2;

	i = 0;
        while (! XPending(dpy))
	{
	    
	    rects[i].x = centerX - xoff;
	    rects[i].y = centerY - yoff;
	    rects[i].width = 2 * xoff;
	    rects[i].height = 2 * yoff;
	    xoff += xdir;
	    yoff += ydir;
	    if ((xoff <= 0) || (xoff >= centerX))
	    {
		xoff -= 2*xdir;
	        xdir = -xdir;
	    }
	    if ((yoff <= 0) || (yoff >= centerY))
	    {
	        yoff -= 2*ydir;
		ydir = -ydir;
	    }
	    if (i == (NUMRECTS - 1))
	    {
                XFillRectangles(dpy, myWin, gc, rects, NUMRECTS);
		i = 0;
	    }
	    else
		i++;
	}
    }
}
