/*
 *	$Source: /@/orpheus/u1/X11/clients/xcalc/RCS/sr.c,v $
 *	$Header: sr.c,v 1.2 87/09/11 01:50:21 newman Exp $
 */

#ifndef lint
static char *rcsid_sr_c = "$Header: sr.c,v 1.2 87/09/11 01:50:21 newman Exp $";
#endif	lint

/* Slide Rule */

#include <stdio.h>
#include <math.h>
#include <strings.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <sys/time.h>


#define SLIDETOP 	26
#define SLIDEHIGH 	38
#define WIDTH 		800
#define HEIGHT		100
#define START		70
#define END		20
#define LABEL		55
#define MAXWIDTH	32766

#define FOFFSET		3
#define MAJORH		10
#define MIDDH		7
#define MINORH		4
#define HALVES		5
#define FIFTHS		15
#define TENTHS		40
#define LABELFIFTH	41
#define LABELTENTHS	200
#define LABELSPACE	20

#define LSCALEH		2
#define ASCALEH		16
#define BSCALEH		0
#define CISCALEH	14
#define CSCALEH		28
#define DSCALEH		66
#define DISCALEH	84

extern Display *dpy;
Window	slidewid,hairlwid,blackwid;
extern Window theWindow,dispwid;
extern XFontStruct *kfontinfo;
Font	scalefont, sscalefont;
extern Pixmap stipplePix,regBorder,dimBorder,IconPix;
Pixmap	slidePix = NULL;
Pixmap	framePix = NULL;
extern Cursor	arrow;
GC	sgc, cgc;
int	height,scalelen,width,fheight,foffset;
int	xo,hx,xm;
extern int ForeColor,BackColor;
double log2;
extern short check_bits[];



/**************/
do_sr(argc, argv, geom, border)
/**************/
char	**argv;
char	*geom;
int	border;
{
    char def_geom[32];

    XSizeHints szhint;
    XEvent event;
    XGCValues gcv;

    /* figure out sizes of keypad and display */

    sscalefont = scalefont = kfontinfo->fid;
    fheight = kfontinfo->max_bounds.ascent + FOFFSET;
    foffset = kfontinfo->max_bounds.ascent;

    /* Open the main window. */
    width = WIDTH;
    scalelen = width - START - END;
    height = HEIGHT;
    xo = -1;
    hx = width/2;
    log2 = log10(2.0);

    sprintf (def_geom, "=%dx%d+0-0", width, height);
    if (geom == NULL) {
      geom = (char *) malloc(24);
      sprintf(geom, "=%dx%d", width, height);
    }

    theWindow = XCreateSimpleWindow(dpy, RootWindow(dpy, DefaultScreen(dpy)),
				    0, 0, START+END+2, height, border,
				    ForeColor, BackColor);

    szhint.flags = PSize|PMinSize;
    szhint.width = szhint.min_width = WIDTH;
    szhint.height = szhint.min_height = HEIGHT;
    XSetStandardProperties(dpy, theWindow, "Slide Rule", NULL, IconPix,
			   argv, argc, &szhint);

    blackwid = XCreateSimpleWindow(dpy, theWindow,
				   0,SLIDETOP,WIDTH,SLIDEHIGH+2,0,
				   BackColor,ForeColor);
    XSetWindowBackgroundPixmap(dpy, blackwid, stipplePix);

    slidewid = XCreateSimpleWindow(dpy, theWindow,
				   -1,SLIDETOP,WIDTH,SLIDEHIGH,1,
				   ForeColor, BackColor);
    hairlwid = XCreateSimpleWindow(dpy, theWindow,
				   WIDTH/2,0,1,height,0,
				   BackColor, ForeColor);
    dispwid = XCreateSimpleWindow(dpy, theWindow,
				  0, 0, START-END, HEIGHT,1,
				  ForeColor, BackColor);

    gcv.foreground = ForeColor;
    gcv.background = BackColor;
    gcv.font = scalefont;
    sgc = XCreateGC(dpy, theWindow, GCForeground|GCBackground|GCFont, &gcv);
    gcv.foreground = BackColor;
    gcv.background = ForeColor;
    cgc = XCreateGC(dpy, theWindow, GCForeground|GCBackground|GCFont, &gcv);

#define STDEVENTS	ExposureMask|EnterWindowMask|LeaveWindowMask
#define BUTEVENTS	ButtonPressMask|ButtonReleaseMask
#define MOVEVENTS	Button1MotionMask|Button2MotionMask|Button3MotionMask
    XSelectInput(dpy, theWindow, STDEVENTS|BUTEVENTS|MOVEVENTS);
    XSelectInput(dpy, hairlwid,  STDEVENTS);
    XSelectInput(dpy, dispwid,   STDEVENTS);
    XSelectInput(dpy, slidewid,  STDEVENTS|BUTEVENTS|MOVEVENTS);
    XSelectInput(dpy, blackwid,  STDEVENTS|BUTEVENTS);

    XMapWindow    (dpy, theWindow);
    XMapSubwindows(dpy, theWindow);
    arrow=XCreateFontCursor(dpy, XC_hand2);
    XDefineCursor (dpy, theWindow, arrow);


    /**************** Main loop *****************/

    while (1) {
        Window wind,mwid;
	int	dummy, w;

        XNextEvent(dpy, &event);

        switch (event.type) {

        case Expose: {
	    XExposeEvent *exp_event = (XExposeEvent *) &event;
            wind = exp_event->window;

            if (wind==theWindow) {
		XGetGeometry(dpy, theWindow, &mwid, &dummy, &dummy, &w, &dummy,
			     &dummy, &dummy);
		if (width != w) {
		    rescale(w);
		    break;
		}
		redrawframe(exp_event->x, exp_event->y, exp_event->width,
			    exp_event->height);
	    } else if (wind==slidewid)
	      redrawslide(exp_event->x, exp_event->y, exp_event->width,
			  exp_event->height);
            else if (wind==hairlwid)
	      drawhairl();
	    else if (wind==blackwid)
	      XClearWindow(dpy, blackwid, exp_event->x, exp_event->y,
			   exp_event->width, exp_event->height, 0);
	    else if (wind==dispwid)
	      drawnums();
            break;
	}
        case ButtonPress: {
            XButtonPressedEvent *but_event = (XButtonPressedEvent *) &event;

            mwid = wind = but_event->window;
	    if (wind == slidewid || wind == blackwid) {
		switch (but_event->button & 0xff) {
		case Button1:
		    xo = (wind==slidewid?xo:0) + but_event->x - START;
		    break;
		case Button2:
		    if (but_event->state & ShiftMask)
		      exit(0);
		    if (wind == slidewid)
		      xm = but_event->x;
		    break;
		case Button3:
		    if (but_event->state & ShiftMask)
		      exit(0);
		    xo = (wind==slidewid?xo:0) + but_event->x + END - width;
		    break;
		}
		XMoveWindow(dpy, slidewid, xo - 1, SLIDETOP);
		drawnums();
		break;
	    }
	    if (wind == theWindow) {
		switch (but_event->button & 0xff) {
		case Button1:
		    hx = but_event->x;
		    break;
		case Button2:
		    if (but_event->state & ShiftMask)
		      exit(0);
		    if (width*2 < MAXWIDTH)
		      rescale(width*2);
		    break;
		case Button3:
		    rescale(width/2);
		}
		drawhairl();
		drawnums();
		break;
	    }
            break;
	  }
	case MotionNotify: {
            XPointerMovedEvent *mov_event = (XPointerMovedEvent *) &event;
	    int	x, y, newx, dummy, mask;
	    Window mwid;

            wind = mwid;
	    XQueryPointer(dpy, theWindow, &mwid, &mwid, &x, &y, &dummy, &dummy,
			  &mask);
	    while (XPending(dpy) && (XPeekEvent(dpy, &event),
		   (event.type == MotionNotify)))
	      XNextEvent(dpy, &event);
	    if (wind == slidewid) {
		if (mask & Button1Mask)
		  newx = x - START;
		else if (mask & Button2Mask)
		  newx = x + END - width;
		else
		  newx = x - xm;
		if (newx != xo) {
		    xo = newx;
		    XMoveWindow(dpy, slidewid, xo, SLIDETOP);
		    drawnums();
		}
	    } else if (wind == theWindow) {
		if (mask & Button1Mask) {
		    newx = x;
		    if (newx != hx) {
			hx = newx;
			drawhairl();
			drawnums();
		    }
		}
	    }
	    break;
	  }
	case ButtonRelease:
	    break;
	case EnterNotify:
	case LeaveNotify: {
	    XCrossingEvent *cross_event = (XCrossingEvent *) &event;

	    if (event.type == EnterNotify)
	      XSetWindowBorder(dpy,theWindow,ForeColor);
	    else if ((cross_event->window == theWindow) &&
		(cross_event->detail != NotifyInferior)) {
	      XSetWindowBorderPixmap(dpy,theWindow,dimBorder);
            }
            break;
	  }
	case NoExpose:
	    break;
        default:
           printf("event type=%ld\n",event.type); 
/*           XSRError("Unexpected X_Event"); */

        }  /* end of switch */
    }  /* end main loop */
}


/***********************************/
XSRError (identifier)
       char *identifier;
{
    fprintf(stderr, "xsr: %s\n", identifier);
    exit(1);
}


rescale(w)
int	w;
{
    int	x,y,wx,wy;
    Window win;
    int	oldwidth, dummy;

    XGetGeometry(dpy, theWindow, &win, &wx, &wy, &oldwidth, &dummy,
		 &dummy, &dummy);
    if (oldwidth != w) {
	XQueryPointer(dpy, theWindow, &win, &win, &dummy, &dummy, &x, &y,
		      &dummy);
	XMoveResizeWindow(dpy, theWindow, wx + x - (x * w)/oldwidth, wy,
			  w, HEIGHT);
    }
    hx = (hx * w) / width;
    xo = (xo * w) / width;
    width = w;
    scalelen = width - START - END;
    XResizeWindow(dpy, slidewid, width, SLIDEHIGH);
    XResizeWindow(dpy, blackwid, width, SLIDEHIGH+2);
    if (framePix)
      XFreePixmap(dpy, framePix);
    framePix = NULL;
    drawframe();
    if (slidePix)
      XFreePixmap(dpy, slidePix);
    slidePix = NULL;
    drawslide();
    XMoveWindow(dpy, slidewid, xo, SLIDETOP);
    drawnums();
    drawhairl();
}

drawmark(win, x, y, height, topp)
Window	win;
int	x,y,height,topp;
{
    XDrawLine(dpy, win, sgc, x, (topp?y:y+MAJORH-height),
	      x, height + (topp?y:y+MAJORH-height), ForeColor);
}

dolabel(win, x, y, str, topp, majorp)
Window	win;
int	x,y;
char	*str;
int	topp,majorp;
{
    XDrawString(dpy, win, sgc, x + 2,
		(topp?y+FOFFSET+foffset:y+MAJORH-fheight+foffset),
		str, strlen(str));
}


drawframe()
{
    int		i,x,j,xx;
    char	str[5];
    int midpt = scalelen/2;

    if (framePix == NULL)
      framePix = XCreatePixmap(dpy, theWindow, width, HEIGHT, 1);
    XFillRectangle(dpy, framePix, cgc, 0, 0, width, HEIGHT);
    XDrawString(dpy, framePix, sgc, LABEL, LSCALEH+foffset, "L", 1);
    for (i = 0; i <= 10; i++) {
	sprintf(str, "%d", i);
	x = START+(i*scalelen)/10;
	dolabel(framePix, x, LSCALEH, str, 0, 1);
	drawmark(framePix, x, LSCALEH, MAJORH, 0);
	for (j = 1; j < 10; j++)
	  drawmark(framePix, x+(j*scalelen)/100, LSCALEH, (j==5?MIDDH:MINORH),
		   0);
    }
    XDrawString(dpy, framePix, sgc, LABEL, ASCALEH+foffset, "A", 1);
    doscale(framePix, ASCALEH, START, midpt, 0);
    doscale(framePix, ASCALEH, START + midpt, scalelen - midpt, 0);
    XDrawString(dpy, framePix, sgc, LABEL, DSCALEH+foffset, "D", 1);
    doscale(framePix, DSCALEH, START, scalelen, 1);
    XDrawString(dpy, framePix, sgc, LABEL, DISCALEH+foffset, "DI", 2);
    for (i = 1; i <= 10; i++) {
	x = START + scalelen * (1 - log10((float) i)) + 0.5;
	sprintf(str, "%d", (i==10?1:i));
	dolabel(framePix, x, DISCALEH, str, 1, 1);
	drawmark(framePix, x, DISCALEH, MAJORH, 1);
    }
    XCopyArea(dpy, framePix, theWindow, sgc, 0, 0, scalelen + START+END, HEIGHT, 0, 0);
}

doscale(win, high, offset, len, topp)
     Window	win;
     int	high, offset, len, topp;
{
  int	i,x,xx,j;
  int	xs[11];
  char	str[6];

  xs[0] = offset - 100;
  for (i = 1; i <= 10; i++)
    xs[i] = offset + len * log10((float) i) + 0.5;

  for (i = 1; i < 10; i++) {
      if (xs[i] > xs[i-1] + LABELSPACE) {
	  sprintf(str, "%d", (i==10?1:i));
	  dolabel(win, xs[i], high, str, topp, 1);
	  drawmark(win, xs[i], high, MAJORH, topp);
      } else
	drawmark(win, xs[i], high, MIDDH, topp);

      sprintf(str, "%d.", i);
      dotenths(win, high, xs[i], xs[i+1]-xs[i], str, topp);
  }
  dolabel(win, xs[i], high, "1", topp, 1);
  drawmark(win, xs[i], high, MAJORH, topp);
}


dotenths(win, high, offset, len, str, topp)
Window	win;
int	high, offset, len;
char	*str;
int	topp;
{
    int	i,x;
    int	xs[11];
    char nstr[8];

    for (i = 0; i <= 10; i++ )
      xs[i] = offset +
	len * log10((float) 1.0 + ((float) i) / 10.0) / log2;

    if (len < HALVES)
      return;
    if (len < FIFTHS) {
	drawmark(win, xs[5], high, MINORH, topp);
	return;
    }	
    if (len < TENTHS) {
	for (i = 0; i < 10; i += 2)
	  drawmark(win, xs[i], high, MINORH, topp);
	return;
    }
    if (len < LABELFIFTH) {
	for (i = 0; i < 10; i++) {
	    drawmark(win, xs[i], high, (x==5?MIDDH:MINORH), topp);
	    dotenths(win, high, xs[i], xs[i+1]-xs[i], "", topp);
	}
	return;
    }
    if (len < LABELTENTHS) {
	for (i = 0; i < 10; i++) {
	    if (i == 5) {
		sprintf(nstr, "%s%d", str, 5);
		dolabel(win, xs[i], high, nstr, topp, 0);
	    }
	    drawmark(win, xs[i], high, (x==5?MAJORH:MIDDH), topp);
	    dotenths(win, high, xs[i], xs[i+1]-xs[i], "", topp);
	}
    } else {
	for (i = 0; i < 10; i++) {
	    sprintf(nstr, "%s%d", str, i);
	    if (i > 0) {
		dolabel(win, xs[i], high, nstr, topp, 0);
		drawmark(win, xs[i], high, MAJORH, topp);
	    }
	    dotenths(win, high, xs[i], xs[i+1]-xs[i], nstr, topp);
	}
    }
}




drawslide()
{
    int		i,x,j;
    char	str[5];
    int 	midpt = scalelen/2;

    if (slidePix == NULL)
      slidePix = XCreatePixmap(dpy, theWindow, width, SLIDEHIGH, 1);
    XFillRectangle(dpy, slidePix, cgc, 0, 0, width, SLIDEHIGH);
    XDrawString(dpy, slidePix, sgc, LABEL, BSCALEH+foffset, "B", 1);
    doscale(slidePix, BSCALEH, START, midpt, 1);
    doscale(slidePix, BSCALEH, START + midpt, scalelen - midpt, 1);
    XDrawString(dpy, slidePix, sgc, LABEL, CISCALEH+foffset, "CI", 2);
    for (i = 1; i <= 10; i++) {
	x = START + scalelen * (1 - log10((float) i)) + 0.5;
	sprintf(str, "%d", (i==10?1:i));
	dolabel(slidePix, x, CISCALEH, str, 1, 1);
	drawmark(slidePix, x, CISCALEH, MAJORH, 1);
    }
    XDrawString(dpy, slidePix, sgc, LABEL, CSCALEH+foffset, "C", 1);
    doscale(slidePix, CSCALEH, START, scalelen, 0);
    XCopyArea(dpy, slidePix, slidewid, sgc, 0, 0, scalelen+START+END, SLIDEHIGH, 0, 0);
}

redrawslide(x, y, w, h)
int	x,y,w,h;
{
    int	i;

    if (slidePix != NULL)
      XCopyArea(dpy, slidePix, slidewid, sgc, 0, 0, scalelen + START + END, SLIDEHIGH,
		0, 0);
    else
      drawslide();
}

redrawframe(x, y, w, h)
int	x,y,w,h;
{
    if (framePix != NULL)
      XCopyArea(dpy, framePix, theWindow, sgc, 0, 0, scalelen + START + END, HEIGHT,
		0, 0);
    else
      drawframe();
}

drawhairl()
{
    XMoveWindow(dpy, hairlwid, hx, 0);
    XClearWindow(dpy, hairlwid);
}

drawnums()
{
    char	str[10];
    float	x = ((float) (hx - START))/((float) scalelen);
    float	xs = ((float) (hx - START - xo))/((float) scalelen);

    XClearWindow(dpy, dispwid);
    sprintf(str, "%5f", 10. * x);
    XDrawImageString(dpy, dispwid, sgc, 5, LSCALEH+foffset, str, strlen(str));
    sprintf(str, "%5f", pow(100., x));
    XDrawImageString(dpy, dispwid, sgc, 5, ASCALEH+foffset, str, strlen(str));
    sprintf(str, "%5f", pow(100., xs));
    XDrawImageString(dpy, dispwid, sgc, 5, SLIDETOP+foffset + BSCALEH, str, strlen(str));
    sprintf(str, "%5f", pow(10., 1. - xs));
    XDrawImageString(dpy, dispwid, sgc, 5, SLIDETOP+foffset + CISCALEH, str, strlen(str));
    sprintf(str, "%5f", pow(10., xs));
    XDrawImageString(dpy, dispwid, sgc, 5, SLIDETOP+foffset + CSCALEH, str, strlen(str));
    sprintf(str, "%5f", pow(10., x));
    XDrawImageString(dpy, dispwid, sgc, 5, DSCALEH+foffset, str, strlen(str));
    sprintf(str, "%5f", pow(10., 1. - x));
    XDrawImageString(dpy, dispwid, sgc, 5, DISCALEH+foffset, str, strlen(str));
}
