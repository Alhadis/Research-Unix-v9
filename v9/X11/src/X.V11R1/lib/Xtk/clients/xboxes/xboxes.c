/*
 *	$Source: /orpheus/u1/X11/lib/Xtk/clients/xboxes/RCS/xboxes.c,v $
 *	$Header: xboxes.c,v 1.1 87/09/14 00:49:28 swick Exp $
 */

#ifndef lint
static char *rcsid_xboxes_c = "$Header: xboxes.c,v 1.1 87/09/14 00:49:28 swick Exp $";
#endif	lint

/* xclock -- 
 *  Hacked from Tony Della Fera's much hacked clock program.
 */
#ifndef lint
static char *rcsid_xclock_c = "$Header: xboxes.c,v 1.1 87/09/14 00:49:28 swick Exp $";
#endif  lint

#include <stdio.h>
#include "Xatom.h"
#include "Intrinsic.h"
#include "Atoms.h"
#include "Label.h"
#include "Command.h"
#include "ButtonBox.h"
/* Command line options table.  Only resources are entered here...there is a
   pass over the remaining options after XtParseCommand is let loose. */

static void exit();

static XrmOptionDescRec options[] = {
{"-label",	XtNlabel,	XrmoptionSepArg,	NULL}
};


/*
 * Report the syntax for calling xclock.
 */
static Widget toplevel;

/* ARGSUSED */
Syntax(call)
	char *call;
{
}

/* ARGSUSED */
void callback(widget,closure,callData)
    Widget widget;
    caddr_t closure;
    caddr_t callData;
{
 (void) fprintf(stderr,"xcommand callback\n");
  XtDestroyWidget(toplevel);
  exit(0);
}

void main(argc, argv)
    unsigned int argc;
    char **argv;
{
    Widget  label,command,box,box1,box2,box3;

    static Arg arg[] = { {XtNfunction,(XtArgVal)(caddr_t)callback} };


    toplevel = XtInitialize(
	NULL, "XBoxes",
	options, XtNumber(options), &argc, argv);
    if (argc != 1) Syntax(argv[0]);

    box = XtCreateWidget(
	argv[0], buttonBoxWidgetClass, toplevel, NULL, (unsigned)0);
    XtManageChild(box);

    box1 = XtCreateWidget(
	"box1", buttonBoxWidgetClass, box, NULL, (unsigned)0);
    XtManageChild(box1);
    box2 = XtCreateWidget(
	"box2", buttonBoxWidgetClass, box, NULL, (unsigned)0);
    XtManageChild(box2);
    box3 = XtCreateWidget(
	"box3", buttonBoxWidgetClass, box, NULL, (unsigned)0);
    XtManageChild(box3);

    label = XtCreateWidget(
	"label1",labelWidgetClass,box1,NULL,(unsigned)0);
    XtManageChild(label);

    label = XtCreateWidget(
	"label2",labelWidgetClass,box2,NULL,(unsigned)0);
    XtManageChild(label);

    label = XtCreateWidget(
	"label3",labelWidgetClass,box3,NULL,(unsigned)0);
    XtManageChild(label);


    command = XtCreateWidget(
	"command1",commandWidgetClass,box1,arg,(unsigned)1);
    XtManageChild(command);

    command = XtCreateWidget(
	"command2",commandWidgetClass,box2,NULL,(unsigned)0);
    XtManageChild(command);

    command = XtCreateWidget(
	"command3",commandWidgetClass,box3,NULL,(unsigned)0);
    XtManageChild(command);
    
    XtRealizeWidget(toplevel);
    XtMainLoop();
}
