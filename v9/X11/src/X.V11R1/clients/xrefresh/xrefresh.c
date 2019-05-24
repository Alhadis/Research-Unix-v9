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
#include <stdio.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <sys/types.h>
#include <sys/timeb.h>

char *malloc();

Window XCreateWindow();

Window win;

Display *StartConnectionToServer(argc, argv)
int	argc;
char	*argv[];
{
    char *display;
    int i;
    Display *dpy;

    display = NULL;
    for(i = 1; i < argc; i++)
    {
        if(index(argv[i], ':') != NULL)
	    display = argv[i];
    }
    if (!(dpy = XOpenDisplay(display)))
    {
	printf("refresh: failed to open display\n");
	exit(1);
    }
    return dpy;
}

main(argc, argv)
int	argc;
char	*argv[];
{
    int     image[8];
    int     amount, i;
    int     stuff[4];
    int     fg, bg;
    Visual visual;
    XImage ximage;
    XSetWindowAttributes xswa;
    XWindowChanges xwc;
    char    line[30];
    struct timeb    start, stop;
    Display *dpy;

    dpy = StartConnectionToServer(argc, argv);

    xswa.background_pixel = BlackPixel(dpy, DefaultScreen(dpy));
    xswa.override_redirect = True;
    visual.visualid = CopyFromParent;
    win = XCreateWindow(dpy, DefaultRootWindow(dpy), 0, 0, 9999, 9999,
	    0, DefaultDepth(dpy, DefaultScreen(dpy)), InputOutput, &visual,
	    CWBackPixel | CWOverrideRedirect, &xswa);

    XMapWindow(dpy, win);
    XFlush(dpy);
    exit(0);
}

