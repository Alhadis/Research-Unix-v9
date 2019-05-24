#include "Xlib.h"
#include "Intrinsic.h"
#include "Atoms.h"
#include "Label.h"

/* Command line options table.  Only resources are entered here...there is a
   pass over the remaining options after XtParseCommand is let loose. */

static XrmOptionDescRec options[] = {
{"-label",	XtNlabel,	XrmoptionSepArg,	NULL}
};


/*
 * Report the syntax for calling xlabel
 */
Syntax(call)
	char *call;
{
}

void main(argc, argv)
    unsigned int argc;
    char **argv;
{
    Widget toplevel, label;

    toplevel = XtInitialize(
	argv[0], "XLabel",
	options, XtNumber(options), &argc, argv);
    if (argc != 1) Syntax(argv[0]);

    label = XtCreateWidget(argv[0], labelWidgetClass, toplevel, NULL, (unsigned)0);
    XtManageChild(label);
    
    XtRealizeWidget(toplevel);
    XtMainLoop();
}
