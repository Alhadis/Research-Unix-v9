#ifndef lint
static char rcsid[] = "$Header: TextKinds.c,v 1.3 87/09/13 13:25:40 swick Exp $";
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
 * 
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
#include "Xresource.h"
#include "Intrinsic.h"
#include "Text.h"
#include "TextP.h"
#include "Atoms.h"
/* #include "TextDisp.h" */
#include <strings.h>

extern char *tmpnam();

/* ||| Bogus definitions to go in some .h file */

extern int *XtDiskSourceCreate (/* name, mode */);
    /* char       *name;	*/
    /* XtEditType mode;		*/

extern int *XtStringSourceCreate (/* str, maxLength, mode */);
    /* char 		*str;		*/
    /* int 		maxLength;	*/	
    /* XtEditType	mode;		*/


/* Private definitions. */

typedef enum _TextKinds {diskText, stringText} TextKinds;

static int *source;

static Boolean initialized = FALSE;

/* EditType enumeration constants */

static  XrmQuark  XtQTextRead;
static  XrmQuark  XtQTextAppend;
static  XrmQuark  XtQTextEdit;

/*ARGSUSED*/
#define done(address, type) \
        { (*toVal).size = sizeof(type); (*toVal).addr = (caddr_t) address; }

extern void _XLowerCase();

static void CvtStringToEditMode(dpy, fromVal, toVal)
    Display     *dpy;
    XrmValue    fromVal;
    XrmValue    *toVal;
{
    static XtEditType editType;
    XrmQuark    q;
    char        lowerName[1000];

/* ||| where to put LowerCase */
    _XLowerCase((char *) fromVal.addr, lowerName);
    q = XrmAtomToQuark(lowerName);
    if (q == XtQTextRead ) {
        editType = XttextRead;
        done(&editType, XtEditType);
        return;
    }
    if (q == XtQTextAppend) {
        editType = XttextAppend;
        done(&editType, XtEditType);
        return;
    }
    if (q == XtQTextEdit) {
        editType = XttextEdit;
        done(&editType, XtEditType);
        return;
    }
    (*toVal).size = 0;
    (*toVal).addr = NULL;
};

static TextKindsInitialize(w)
Widget w;
{
    if (initialized)
    	return;
    XtQTextRead = XrmAtomToQuark(XtEtextRead);
    XtQTextAppend   = XrmAtomToQuark(XtEtextAppend);
    XtQTextEdit  = XrmAtomToQuark(XtEtextEdit);
    XrmRegisterTypeConverter(XrmRString, XtREditMode, CvtStringToEditMode);
    initialized = TRUE;

}

static Widget AsciiTextCreate(w, args, argCount, src)
    Widget w;
    Arg 	*args;
    int 	argCount;
    int 	*src;
{
    TextWidget tw = (TextWidget)w;
    ArgList mergedArgs;
    Arg myArgs[2];

    XtSetArg(myArgs[0], XtNtextSource, (XtArgVal) src);
    XtSetArg(myArgs[1], XtNtextSink, 
	(XtArgVal) XtAsciiSinkCreate(w, NULL, 0));

    mergedArgs = XtMergeArgLists(args, argCount, myArgs, XtNumber(myArgs) );
/* XXX first param is bogus */
    tw = (TextWidget) XtCreateWidget("textname", textWidgetClass,
	w, mergedArgs, argCount+XtNumber(myArgs));

    XtFree((char *) mergedArgs);
    return (Widget)tw;
}

/***** Public routines. *****/

Widget XtTextDiskCreate(w, args, argCount)
    Widget w;
    ArgList args;
    int     argCount;
{
    if (!initialized) TextKindsInitialize(w);
    source = XtDiskSourceCreate (w);
    return (AsciiTextCreate (w, args, argCount, source));
}

XtTextDiskDestroy(w)
    Widget w;
{
    XtDestroyWidget(w);
}

Widget XtTextStringCreate(w, args, argCount)
    Widget w;    
    ArgList args;
    int     argCount;
{
    if (!initialized) TextKindsInitialize(w);

    /* need option for specifying read append mode */
    source = XtStringSourceCreate (w, args, argCount);
    return (AsciiTextCreate (w, args, argCount, source));
}

XtTextStringDestroy(w)
  Widget w;
{
    XtDestroyWidget(w);
}


Widget XtTextSourceCreate(w, args, argCount, passedSource)
    Widget w;
    ArgList args;
    int     argCount;
    int     *passedSource;  /* ||| NOTE:  should be in args */
{
    if (!initialized) TextKindsInitialize(w);
    return (AsciiTextCreate (w, args, argCount, passedSource));
}

XtTextSourceDestroy(w)
    Widget w;
{
    XtDestroyWidget(w);
}

