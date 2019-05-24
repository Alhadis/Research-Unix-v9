/* xclock -- 
 *  Hacked from Tony Della Fera's much hacked clock program.
 */
#ifndef lint
static char *rcsid_xclock_c = "$Header: xclock.c,v 1.5 87/09/13 03:17:53 newman Exp $";
#endif  lint

#include "Xatom.h"
#include "Xlib.h"
#include "Intrinsic.h"
#include "Atoms.h"
#include "TopLevel.h"
#include "Clock.h"
#include "clock.bit"

extern void exit();

/* Command line options table.  Only resources are entered here...there is a
   pass over the remaining options after XtParseCommand is let loose. */

static XrmOptionDescRec options[] = {
{"-chime",	XtNchime,	XrmoptionNoArg,		"TRUE"},
{"-hd",		XtNhand,	XrmoptionSepArg,	NULL},
{"-hands",	XtNhand,	XrmoptionSepArg,	NULL},
{"-hl",		XtNhigh,	XrmoptionSepArg,	NULL},
{"-highlight",	XtNhigh,	XrmoptionSepArg,	NULL},
{"-u",		XtNupdate,	XrmoptionSepArg,	NULL},
{"-update",	XtNupdate,	XrmoptionSepArg,	NULL},
{"-padding",	XtNpadding,	XrmoptionSepArg,	NULL},
{"-d",		XtNanalog,	 XrmoptionNoArg,	"FALSE"},
{"-digital",	XtNanalog,	 XrmoptionNoArg,	"FALSE"},
{"-analog",	XtNanalog,	 XrmoptionNoArg,	"TRUE"},
{"-a",		XtNanalog,	 XrmoptionNoArg,	"TRUE"},
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

void main(argc, argv)
    int argc;
    char **argv;
{
    Widget toplevel, w;
    Arg arg;

    toplevel = XtInitialize("Clock", "Clock", options, XtNumber(options), &argc, argv);
    if (argc != 1) Syntax(argv[0]);

    arg.name = XtNiconPixmap;
    arg.value = (XtArgVal) XCreateBitmapFromData (XtDisplay(toplevel),
         XtScreen(toplevel)->root, clock_bits, clock_width, clock_height);
    XtSetValues (toplevel, &arg, 1); 

    w = XtCreateWidget (argv[0], clockWidgetClass, toplevel, NULL, 0);
    XtRealizeWidget (toplevel);
    XtMainLoop();
}
