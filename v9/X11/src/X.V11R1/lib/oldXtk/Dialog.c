/* $Header: Dialog.c,v 1.1 87/09/11 07:57:23 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)Dialog.c	1.26	5/18/87";
#endif lint
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


/* NOTE: THIS IS NOT A WIDGET!  Rather, this is an interface to a widget.
   It implements policy, and gives a (hopefully) easier-to-use interface
   than just directly making your own form. */


#include <strings.h>
#include "Xlib.h"
#include "Intrinsic.h"
#include "Form.h"
#include "Dialog.h"
#include "Text.h"
#include "Command.h"
#include "Label.h"
#include "Atoms.h"


/* Private Definitions. */

typedef struct {
    Display	*dpy;		/* Form window display connection */
    Window	mywin;		/* Form window. */
    int		width, height;	/* Size of form window. */
    Window	label;		/* Window containing description of dialog. */
    Window	value;		/* Window for entering user response. */
    int		numbuttons;	/* How many buttons currently in window. */
    Window	*button;	/* Array of buttons. */
    char	*labelstring;	/* String containing data in label. */
    char	*valuestring;	/* String containing data in value. */
} WidgetDataRec, *WidgetData;

static XContext dialogContext = NULL;



static WidgetData DataFromWindow(dpy, window)
Display *dpy;
Window window;
{
    WidgetData result;
    if (XFindContext(dpy, window, dialogContext, (caddr_t *)&result))
	return NULL;
    return result;
}


static XtGeometryReturnCode DialogGeometryHandler(dpy, window, request,
						  reqBox, replBox)
Display *dpy;
Window window;
XtGeometryRequest request;
WindowBox *reqBox, *replBox;
{
    WidgetData data;
    data = DataFromWindow(dpy, window);
    if (data && request == XtgeometryResize) {
	data->width = reqBox->width;
	data->height = reqBox->height;
	XResizeWindow(data->dpy, window, reqBox->width, reqBox->height);
	*replBox = *reqBox;
	return XtgeometryYes;
    }
    return XtgeometryNo;
}


static XtEventReturnCode DialogEventHandler(event, data)
XEvent *event;
WidgetData data;
{
    if (event->type == DestroyNotify) {
	XtFree(data->labelstring);
	if (data->valuestring) XtFree(data->valuestring);
	XtFree((char *)data->button);
	XtFree((char *)data);
	return XteventHandled;
    }
    return XteventNotHandled;
}


/* Public definitions. */

Window XtDialogCreate(dpy, parent, description, valueinit, args, argCount)
Display *dpy;			/* Display connection for the form */
Window parent;			/* Window to put the form in. */
char *description;		/* Title for this dialog box. */
char *valueinit;		/* Initial string for value field (use NULL
				   if you don't want a value field) */
ArgList args;			/* Args to pass on to form (if any). */
int argCount;
{
    WidgetData data;
    static Arg arglist1[] = {
	{XtNlabel, (XtArgVal)NULL},
	{XtNborderWidth, (XtArgVal) 0}
    };
    static Arg arglist2[] = {
	{XtNwidth, (XtArgVal)NULL},
	{XtNstring, (XtArgVal)NULL},
	{XtNlength, (XtArgVal)1000},
	{XtNtextOptions, (XtArgVal) (resizeWidth | resizeHeight)},
	{XtNeditType, (XtArgVal) XttextEdit}
    };
    static Arg arglist3[] = {
	{XtNfromVert, (XtArgVal)NULL},
	{XtNresizable, (XtArgVal)TRUE}
    };
    if (dialogContext == NULL) dialogContext = XUniqueContext();
    data = (WidgetData) XtMalloc(sizeof(WidgetDataRec));
    data->dpy = dpy;
    data->numbuttons = 0;
    data->button = (Window *) XtMalloc(sizeof(Window));
    data->mywin = XtFormCreate(dpy, parent, args, argCount);
    (void)XSaveContext(data->dpy, data->mywin, dialogContext, (caddr_t) data);
    (void)XtSetGeometryHandler(
	data->dpy, data->mywin, (XtGeometryHandler) DialogGeometryHandler);
    data->labelstring = XtMalloc((unsigned) strlen(description) + 1);
    (void) strcpy(data->labelstring, description);
    arglist1[0].value = (XtArgVal) data->labelstring;
    data->label = XtLabelCreate(
	data->dpy, data->mywin, arglist1, XtNumber(arglist1));
    XtFormAddWidget(data->dpy, data->mywin, data->label, (ArgList) NULL, 0);
    if (valueinit) {
	static int grabfocus;
	static Resource resources[] = {
	    {XtNgrabFocus, XtCGrabFocus, XrmRBoolean, sizeof(int),
		 (caddr_t)&grabfocus, (caddr_t)NULL}
	};
	XrmNameList names;
	XrmClassList classes;
	grabfocus = FALSE;
	XtGetResources(dpy, resources, XtNumber(resources), args, argCount,
		       parent, "dialog", "Dialog", &names, &classes);
	XrmFreeNameList(names);
	XrmFreeClassList(classes);
	data->valuestring = XtMalloc(1010);
	(void) strcpy(data->valuestring, valueinit);
	arglist2[0].value = (XtArgVal) data->width;
	arglist2[1].value = (XtArgVal) data->valuestring;
	data->value = XtTextStringCreate(
		data->dpy, data->mywin, arglist2, XtNumber(arglist2));
	arglist3[0].value = (XtArgVal) data->label;
	if (grabfocus) XSetInputFocus(dpy, data->value, RevertToParent,
				      CurrentTime); /* !!! Hackish. |||*/
	XtFormAddWidget(
		data->dpy, data->mywin, data->value,
		arglist3, XtNumber(arglist3));
    } else {
	data->valuestring = NULL;
	data->value = NULL;
    }
    XtSetEventHandler(data->dpy, data->mywin, (XtEventHandler) DialogEventHandler,
		      StructureNotifyMask, (caddr_t)data);
    return data->mywin;
}


void XtDialogAddButton(dpy, window, name, function, param)
Display *dpy;
Window window;
char *name;
void (*function)();
caddr_t param;
{
    WidgetData data;
    static Arg arglist1[] = {
	{XtNname, (XtArgVal) NULL},
	{XtNfunction, (XtArgVal) NULL},
	{XtNparameter, (XtArgVal) NULL}
    };
    static Arg arglist2[] = {
	{XtNfromHoriz, (XtArgVal) NULL},
	{XtNfromVert, (XtArgVal) NULL},
	{XtNleft, (XtArgVal) XtChainLeft},
	{XtNright, (XtArgVal) XtChainLeft}
    };
    data = DataFromWindow(dpy, window);
    if (data == NULL) return;
    arglist1[0].value = (XtArgVal) name;
    arglist1[1].value = (XtArgVal) function;
    arglist1[2].value = (XtArgVal) param;
    data->button = (Window *)
	XtRealloc((char *)data->button,
		  (unsigned) ++data->numbuttons * sizeof(Window));
    data->button[data->numbuttons - 1] =
	XtCommandCreate(data->dpy, window, arglist1, XtNumber(arglist1));
    if (data->numbuttons > 1)
	arglist2[0].value = (XtArgVal) data->button[data->numbuttons - 2];
    else
	arglist2[0].value = (XtArgVal) NULL;
    if (data->value) arglist2[1].value = (XtArgVal) data->value;
    else arglist2[1].value = (XtArgVal) data->label;
    XtFormAddWidget(data->dpy, data->mywin, data->button[data->numbuttons - 1],
		    arglist2, XtNumber(arglist2));
}


char *XtDialogGetValueString(dpy, window)
Display *dpy;
Window window;
{
    WidgetData data;
    data = DataFromWindow(dpy, window);
    if (data) return data->valuestring;
    else return NULL;
}
