/* xclock -- 
 *  Hacked from Tony Della Fera's much hacked clock program.
 */
#ifndef lint
static char *rcsid_xclock_c = "$Header: xbuttonbox.c,v 1.5 87/09/14 00:47:14 swick Exp $";
#endif  lint

#include "Xatom.h"
#include "Xlib.h"
#include "Intrinsic.h"
#include "Atoms.h"
#include "TopLevel.h"
#include "Label.h"
#include "ButtonBox.h"
#include "icon.bits"

extern void exit();

/* Command line options table.  Only resources are entered here...there is a
   pass over the remaining options after XtParseCommand is let loose. */

static XrmOptionDescRec options[] = {
    {"-hspace",	XtNhSpace,	XrmoptionSepArg,	NULL},
    {"-vspace",	XtNvSpace,	XrmoptionSepArg,	NULL},
};


/*
 * Report the syntax for calling xbuttonbox.
 */
Syntax(call)
    char *call;
{
    (void) printf ("Usage: %s [-hspace <pixels>] [-vspace <pixels>]\n\n", call);
}

void main(argc, argv)
    int argc;
    char **argv;
{
    Display *dpy;
    Widget toplevel, w;
    Arg 	args[10];
    Widget	labels[10];
    Cardinal	i;
    char	name[100];

    toplevel = XtInitialize("XButtonBox", "XButtonBox", options, XtNumber(options), &argc, argv);
    if (argc != 1) Syntax(argv[0]);

    args[0].name = XtNiconPixmap;
    args[0].value =
        (XtArgVal) XCreateBitmapFromData (XtDisplay(toplevel), XtScreen(toplevel)->root, 
        buttonbox_bits, buttonbox_width, buttonbox_height);
     XtSetValues (toplevel, args, 1);

    w = XtCreateWidget (argv[0], buttonBoxWidgetClass, toplevel, NULL, 0);

    for (i = 0; i < 10; i++) {
	(void) sprintf(name, "%s%d", "longLabelWidgetName", i*i*i);
	args[0].name = XtNsensitive;
	args[0].value = (XtArgVal) (i % 2);
        labels[i] = XtCreateWidget(name, labelWidgetClass,
		w, args, 1);
    }
    XtManageChildren(labels, XtNumber(labels));

    XtRealizeWidget (toplevel, 0, NULL);
    XtMainLoop();
}
