#ifndef lint
static char *rcsid_xload_c = "$Header: xload.c,v 1.32 87/09/09 12:06:08 swick Exp $";
#endif  lint
 
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
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Intrinsic.h>
#include <X11/Atoms.h>
#include <X11/Load.h>
#include <pwd.h>
#include "xload.bit"

extern void exit();

/* Command line options table.  Only resources are entered here...there is a
   pass over the remaining options after XtParseCommand is let loose. */

static XrmOptionDescRec opTable[] = {
{"=",		"geometry",	XrmoptionIsArg,		(caddr_t) NULL},
{"-bd",		XtNborder,	XrmoptionSepArg,	(caddr_t) NULL},
{"-bg",		XtNbackground,	XrmoptionSepArg,	(caddr_t) NULL},
{"-bw",		XtNborderWidth,	XrmoptionSepArg,	(caddr_t) NULL},
{"-fg",		XtNforeground,	XrmoptionSepArg,	(caddr_t) NULL},
{"-fn",		XtNfont,	XrmoptionSepArg,	(caddr_t) NULL},
{"-update",	XtNupdate,	XrmoptionSepArg,	(caddr_t) NULL},
{"-scale",	XtNminScale,	XrmoptionSepArg,	(caddr_t) NULL},
{"-rv",		XtNreverseVideo, XrmoptionNoArg,	(caddr_t) "on"},
{"+rv",		XtNreverseVideo, XrmoptionNoArg,	(caddr_t) "off"}
};

static char *geostr;

static Resource rlist[]=
{
      {"geometry", "Geometry", XrmRString,
	 sizeof(char *), (caddr_t) &geostr, (caddr_t) NULL}
};



/* Exit with message describing command line format */

void usage()
{
    (void) fprintf(stderr,
"usage: xload [-fn {font}] [-update {seconds}] [-scale {integer}] [-rv]\n"
);
    (void) fprintf(stderr,
"             [=[{width}][x{height}][{+-}{xoff}[{+-}{yoff}]]] [[{host}]:[{vs}]]\n"
);
    (void) fprintf(stderr,
"             [-fg {color}] [-bg {color}] [-hl {color}] [-bd {color}] [-bw {pixels}]\n");
    exit(1);
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
    char display[256];                      /* will contain vs host */
    char host[256];
    Display *dpy;
    Window win;
    XrmNameList	 names;
    XrmClassList classes;
    Arg arglist[20];
    int x, y;
    unsigned int height, width;
    int argCount = 0;
    long flags;
    XSizeHints sizehints;
    XWMHints	wmhints, *oldwmhints;

    
    (void) gethostname(host,255);
    XtSetArg(arglist[argCount], XtNlabel, host);
    argCount++;
    XtSetArg(arglist[argCount], XtNname, argv[0]);
    argCount++;
    display[0] = '\0';
    XtInitialize();
    XtGetUsersDataBase();
      
    XrmParseCommand( opTable, XtNumber(opTable), argv[0], &argc, argv);

    if(argc > 1 && index(argv[1], ':') != NULL) {
	    argc--;
	    (void) strncpy(display, argv[1], sizeof(display));
	}

    if (argc != 1) usage();
    /* Open display  */
    if (!(dpy = XOpenDisplay(display))) {
	(void) fprintf(stderr, "%s: Can't open display '%s'\n",
		argv[0], XDisplayName(display));
	exit(1);
    }
    geostr = NULL;
    XtGetResources(dpy,  rlist, XtNumber(rlist),arglist,2,
    		   DefaultRootWindow(dpy),
		   "Xload",  "xload", &names, &classes);
    sizehints.flags = PAspect | PMinSize | PPosition | PSize;
    sizehints.min_aspect.x = 1;
    sizehints.min_aspect.y = 5;
    sizehints.max_aspect.x = 5;
    sizehints.max_aspect.y = 1;
    sizehints.min_width = sizehints.min_height = 15;
    sizehints.width = 100;
    sizehints.height = 100;
    sizehints.x = 0;
    sizehints.y = 200;

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
		-sizehints.width;
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
    
    win = XtLoadCreate(dpy,  DefaultRootWindow(dpy),
	arglist, argCount);
    XtMakeMaster(dpy,  win);
/*    XSetSizeHints(dpy, win, &sizehints, XA_WM_NORMAL_HINTS);   
  */  
    XSetStandardProperties ( dpy, win, "xload","Load Ave",
		    XCreateBitmapFromData(dpy, DefaultRootWindow(dpy), 
			xload_bits, xload_width, xload_height),
			    argv, argc, &sizehints);
    oldwmhints = XGetWMHints(dpy, win);
    if (oldwmhints) {
        wmhints = *oldwmhints;
        free((char *)oldwmhints);
    } else wmhints.flags = 0;
    wmhints.flags |= InputHint /* | StateHint */;
    wmhints.input = FALSE;
/*    wmhints.initial_state = IconicState; */
    XSetWMHints( dpy, win, &wmhints);
    XMapWindow(dpy, win);			    /* Map window to screen */
    for(;;) {
	XEvent event;
	  
	XtNextEvent(dpy,  &event);
	(void) XtDispatchEvent(&event);
    }
}
