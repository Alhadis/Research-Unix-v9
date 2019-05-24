#ifndef lint
static char *rcsid_xload_c = "$Header: xload.c,v 1.4 87/09/13 03:18:40 newman Exp $";
#endif  lint

#include <stdio.h> 
#include "Xatom.h"
#include "Xlib.h"
#include "Intrinsic.h"
#include "Atoms.h"
#include "TopLevel.h"
#include "Load.h"
#include "xload.bit"

extern void exit();

/* Command line options table.  Only resources are entered here...there is a
   pass over the remaining options after XtParseCommand is let loose. */

static XrmOptionDescRec options[] = {
{"-u",		XtNupdate,	XrmoptionSepArg,	 NULL},
{"-update",	XtNupdate,	XrmoptionSepArg,	 NULL},
{"-scale",	XtNminScale,	XrmoptionSepArg,	 NULL},
};


/* Exit with message describing command line format */

void usage()
{
    fprintf(stderr,
"usage: xload [-fn {font}] [-update {seconds}] [-scale {integer}] [-rv]\n"
);
    fprintf(stderr,
"             [=[{width}][x{height}][{+-}{xoff}[{+-}{yoff}]]] [[{host}]:[{vs}]]\n"
);
    fprintf(stderr,
"             [-fg {color}] [-bg {color}] [-hl {color}] [-bd {color}] [-bw {pixels}]\n");
    exit(1);
}


void main(argc, argv)
    int argc;
    char **argv;
{
    char host[256];
    Widget toplevel, w;
    Arg arg;
    
    (void) gethostname(host,255);
    toplevel = XtInitialize("XLoad", "XLoad", options, XtNumber(options), &argc, argv);
      
    if (argc != 1) usage();
    
    arg.name = XtNiconPixmap;
    arg.value = (XtArgVal) XCreateBitmapFromData (XtDisplay(toplevel),
       XtScreen(toplevel)->root,  xload_bits, xload_width, xload_height);
    XtSetValues (toplevel, &arg, 1);

    arg.name = XtNlabel;
    arg.value = (XtArgVal) host;
    w = XtCreateWidget (argv[0], loadWidgetClass, toplevel, &arg, 1);
    XtRealizeWidget (toplevel);
    XtMainLoop();
}
