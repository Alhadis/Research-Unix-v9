#include "Xlib.h"
#include "Intrinsic.h"
#include "Atoms.h"
#include "Command.h"

/* Command line options table.  Only resources are entered here...there is a
   pass over the remaining options after XtParseCommand is let loose. */

static XrmOptionDescRec options[] = {
{"-label",	XtNlabel,	XrmoptionSepArg,	NULL}
};




/*
 * Report the syntax for calling xcommand
 */
Syntax(call)
	char *call;
{
}

void main(argc, argv)
    unsigned int argc;
    char **argv;
{
    Widget toplevel, command;

    toplevel = XtInitialize(
	argv[0], "XCommand",
	options, XtNumber(options), &argc, argv);
    if (argc != 1) Syntax(argv[0]);

    command = XtCreateWidget(
	argv[0], commandWidgetClass, toplevel, (ArgList)NULL, (unsigned)0);
    XtManageChild(command);
    
    XtRealizeWidget(toplevel);
    XtMainLoop();
}
