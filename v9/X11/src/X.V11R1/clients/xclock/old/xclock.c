
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
/* xclock -- 
 *  Hacked from Tony Della Fera's much hacked clock program.
 */
#ifndef lint
static char *rcsid_xclock_c = "$Header: xclock.c,v 1.27 87/09/09 12:03:33 swick Exp $";
#endif  lint

#include <stdio.h>
#include <strings.h>
#include <signal.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/Atoms.h>
#include <X11/Clock.h>
#include <pwd.h>
#include "clock.bit"

extern void exit();

/* Command line options table.  Only resources are entered here...there is a
   pass over the remaining options after XtParseCommand is let loose. */

static XrmOptionDescRec opTable[] = {
{"=",		"geometry",	XrmoptionIsArg,		(caddr_t) NULL},
{"-bd",		XtNborder,	XrmoptionSepArg,	(caddr_t) NULL},
{"-bordercolor",XtNborder,	XrmoptionSepArg,	(caddr_t) NULL},
{"-bg",		XtNbackground,	XrmoptionSepArg,	(caddr_t) NULL},
{"-background",	XtNbackground,	XrmoptionSepArg,	(caddr_t) NULL},
{"-bw",		XtNborderWidth,	XrmoptionSepArg,	(caddr_t) NULL},
{"-border",	XtNborderWidth,	XrmoptionSepArg,	(caddr_t) NULL},
{"-chime",	XtNchime,	XrmoptionNoArg,		(caddr_t) "1"},
{"-fg",		XtNforeground,	XrmoptionSepArg,	(caddr_t) NULL},
{"-foreground",	XtNforeground,	XrmoptionSepArg,	(caddr_t) NULL},
{"-fn",		XtNfont,	XrmoptionSepArg,	(caddr_t) NULL},
{"-font",	XtNfont,	XrmoptionSepArg,	(caddr_t) NULL},
{"-hd",		XtNhand,	XrmoptionSepArg,	(caddr_t) NULL},
{"-hands",	XtNhand,	XrmoptionSepArg,	(caddr_t) NULL},
{"-hl",		XtNhigh,	XrmoptionSepArg,	(caddr_t) NULL},
{"-highlight",	XtNhigh,	XrmoptionSepArg,	(caddr_t) NULL},
{"-u",		XtNupdate,	XrmoptionSepArg,	(caddr_t) NULL},
{"-update",	XtNupdate,	XrmoptionSepArg,	(caddr_t) NULL},
{"-padding",	XtNpadding,	XrmoptionSepArg,	(caddr_t) NULL},
{"-d",		XtNanalog,	 XrmoptionNoArg,	(caddr_t) "0"},
{"-digital",	XtNanalog,	 XrmoptionNoArg,	(caddr_t) "0"},
{"-analog",	XtNanalog,	 XrmoptionNoArg,	(caddr_t) "1"},
{"-a",		XtNanalog,	 XrmoptionNoArg,	(caddr_t) "1"},
{"-rv",		XtNreverseVideo, XrmoptionNoArg,	(caddr_t) "on"},
{"-reverse",	XtNreverseVideo, XrmoptionNoArg,	(caddr_t) "on"},
{"-active",	XtNactive,	 XrmoptionNoArg,	(caddr_t) "1"},
{"+rv",		XtNreverseVideo, XrmoptionNoArg,	(caddr_t) "off"}
};

static char *geostr;
int analog, def_analog = 1;


static Resource rlist[]=
{
      {"geometry", "Geometry", XrmRString,
	 sizeof(char *), (caddr_t) &geostr, (caddr_t) NULL},
      {XtNanalog, XtCAnalog, XrmRInt, sizeof(int), (caddr_t) & analog, 
	(caddr_t) & def_analog}
};


/*
 * Report the syntax for calling xclock.
 */
Syntax(call)
	char *call;
{
	(void) printf ("Usage: %s [-analog] [-bw <pixels>] [-digital]\n", call);
	(void) printf ("       [-fg <color>] [-bg <color>] [-hl <color>] [-bd <color>]\n");
	(void) printf ("       [-fn <font_name>] [-help] [-padding <pixels>]\n");
	(void) printf ("       [-rv] [-update <seconds>] [[<host>]:[<vs>]]\n");
	(void) printf ("       [=[<width>][x<height>][<+-><xoff>[<+-><yoff>]]]\n\n");
	exit(0);
}

void XtGetUsersDataBase()
{
	XrmResourceDataBase resources, userResources;
	int uid;
	extern struct passwd *getpwuid();
	struct passwd *pw;
	char filename[1024];
	FILE *f;

	/* Open .Xdefaults file and merge into existing data base */
	uid = getuid();
	pw = getpwuid(uid);
	if (pw) {
		(void) strcpy(filename, pw->pw_dir);
		(void) strcat(filename, "/.Xdefaults");
		f = fopen(filename, "r");
		if (f) {
			XrmGetCurrentDataBase(&resources);
			XrmGetDataBase(f, &userResources);
			XrmMergeDataBases(userResources, &resources);
			XrmSetCurrentDataBase(userResources);
			(void) fclose(f);
		}
	}
}


void main(argc, argv)
    int argc;
    char **argv;
{
    char displayName[256];	/* will contain DISPLAY name */
    char host[256];
    Display *dpy;
    Window win;
    XrmNameList	 names;
    XrmClassList classes;
    Arg arglist[20];
    int x, y;
    unsigned height, width;
    int argCount = 0;
    int flags = 0;
    XSizeHints sizehints;
    XWMHints	wmhints, *oldwmhints;

    (void) gethostname(host,255);
    XtSetArg(arglist[argCount], XtNlabel, host);
    argCount++;
    XtSetArg(arglist[argCount], XtNname, argv[0]);
    argCount++;
    displayName[0] = '\0';
    XtInitialize();
    XtGetUsersDataBase();
      
    XrmParseCommand( opTable, XtNumber(opTable), argv[0], &argc, argv);

    if(argc > 1 && index(argv[1], ':') != NULL) {
	    argc--;
	    (void) strncpy(displayName, argv[1], sizeof(displayName));
    }
    if (argc != 1) Syntax(argv[0]);

    /* Open display  */
    if (!(dpy = XOpenDisplay(displayName))) {
	(void) fprintf(stderr, "%s: Can't open display '%s'\n",
		argv[0], XDisplayName(displayName));
	exit(1);
    }
    geostr = NULL;
    XtGetResources(dpy, rlist, XtNumber(rlist),arglist,2,
		DefaultRootWindow(dpy),
		   "Xclock",  "xclock", &names, &classes);
    sizehints.flags = PMinSize | PPosition | PSize;
    if(analog) {
	sizehints.flags |= PAspect;
	sizehints.min_aspect.x = 1;
	sizehints.min_aspect.y = 1;
        sizehints.max_aspect.x = 1;
        sizehints.max_aspect.y = 1;
    }
    sizehints.min_width = sizehints.min_height = 15;
    sizehints.width = sizehints.height = 120;
    sizehints.x = 100;
    sizehints.y = 300;

    if( geostr != NULL ) {
	  flags = XParseGeometry(geostr, &x, &y, &width, &height);
	  if(WidthValue & flags) {
	    	sizehints.flags |= USSize;
		sizehints.width = width;
		XtSetArg(arglist[argCount], XtNwidth, width);
		argCount++;
	  }
	  if(HeightValue & flags) {
	    	sizehints.flags |= USSize;
		sizehints.height = height;
		XtSetArg(arglist[argCount], XtNheight, height);
		argCount++;
	  }
	  if(XValue & flags) {
	    if(XNegative & flags)
	      x = DisplayWidth(dpy, DefaultScreen(dpy)) + x 
		- sizehints.width;
	    sizehints.flags |= USPosition;
	    sizehints.x = x;
	    XtSetArg(arglist[argCount], XtNx, x);
	    argCount++;
	  }
	  if(YValue & flags) {
	    if(YNegative & flags)
	      y = DisplayHeight(dpy, DefaultScreen(dpy)) + y
		-sizehints.height;
	    sizehints.flags |= USPosition;
	    sizehints.y = y;
	    XtSetArg(arglist[argCount], XtNy, y);
	    argCount++;
	  }
    }
    
    win = XtCreateClock(dpy, DefaultRootWindow(dpy),
		arglist, argCount);
    XtMakeMaster(dpy, win);
    
    XSetStandardProperties ( dpy, win, "xclock","Clock", 
		    XCreateBitmapFromData(dpy, DefaultRootWindow(dpy), 
		    	       clock_bits, clock_width, clock_height),
			    argv, argc, &sizehints);
    oldwmhints = XGetWMHints(dpy, win);
    if (oldwmhints) {
        wmhints = *oldwmhints;
        free((char *)oldwmhints);
    } else wmhints.flags = 0;
    wmhints.flags |= InputHint /*| StateHint */;
    wmhints.input = FALSE;
/*  wmhints.initial_state = IconicState; */
    XSetWMHints( dpy, win, &wmhints);
    XMapWindow(dpy, win);			    /* Map window to screen */
    for(;;) {
	XEvent event;
	  
	XtNextEvent(dpy, &event);
	(void) XtDispatchEvent(&event);
    }
}
