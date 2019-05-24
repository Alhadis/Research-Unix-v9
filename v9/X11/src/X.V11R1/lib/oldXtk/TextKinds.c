/* $Header: TextKinds.c,v 1.1 87/09/11 07:58:58 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)TextKinds.c	1.18	2/25/87";
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

#include "Xlib.h"
#include "Intrinsic.h"
#include "Text.h"
#include "Atoms.h"
#include "TextDisp.h"
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

typedef enum TextKinds {diskText, stringText};

static XtEditType editMode;
static XtEditType defaultEditMode = XttextRead;
static XFontStruct *defaultFont;
static XFontStruct *textFontStruct;
static char *nullValue = NULL;
static int *source, textLength;
static int defaultLength = 100;
static int foreground, background;
static char *fileName, *textString;

static Resource rawResources[] = {
    {XtNfont, XtCFont, XrmRFontStruct, sizeof (XFontStruct *),
        (caddr_t) &textFontStruct, (caddr_t) &defaultFont},
    {XtNforeground, XtCColor, XrmRPixel, sizeof (int),
        (caddr_t) &foreground, (caddr_t) &XtDefaultFGPixel},
    {XtNbackground, XtCColor, XrmRPixel, sizeof (int),
        (caddr_t) &background, (caddr_t) &XtDefaultBGPixel},
};

static Resource stringResources[] = {
    {XtNforeground, XtCColor, XrmRPixel, sizeof (int),
        (caddr_t) &foreground, (caddr_t) &XtDefaultFGPixel},
    {XtNbackground, XtCColor, XrmRPixel, sizeof (int),
        (caddr_t) &background, (caddr_t) &XtDefaultBGPixel},
    {XtNstring, XtCString, XrmRString, sizeof (char *),
	 (caddr_t) &textString, (caddr_t) &nullValue},
    {XtNfont, XtCFont, XrmRFontStruct, sizeof (XFontStruct *),
        (caddr_t) &textFontStruct, (caddr_t) &defaultFont},
    {XtNlength, XtCLength, XrmRInt, sizeof (int), (caddr_t) &textLength,
        (caddr_t)&defaultLength},
    {XtNeditType, XtCEditType, XtREditMode, sizeof(editMode), 
 	(caddr_t)&editMode, (caddr_t)&defaultEditMode},
};

static Resource diskResources[] = {
    {XtNforeground, XtCColor, XrmRPixel, sizeof (int),
        (caddr_t) &foreground, (caddr_t) &XtDefaultFGPixel},
    {XtNbackground, XtCColor, XrmRPixel, sizeof (int),
        (caddr_t) &background, (caddr_t) &XtDefaultBGPixel},
    {XtNfile, XtCFile, XrmRString, sizeof (char *),
        (caddr_t) &fileName, (caddr_t) &nullValue},
    {XtNfont, XtCFont, XrmRFontStruct, sizeof (XFontStruct *),
        (caddr_t) &textFontStruct, (caddr_t) &defaultFont},
    {XtNeditType, XtCEditType, XtREditMode, sizeof(editMode), 
	(caddr_t)&editMode, (caddr_t)&defaultEditMode},
};

static Boolean initialized = FALSE;

static TextKindsInitialize(dpy)
Display *dpy;
{
    if (initialized)
    	return;
    initialized = TRUE;

    defaultFont = XLoadQueryFont(dpy, "fixed");
}

static Window AsciiTextCreate(dpy, parent, args, argCount, names, classes, src)
    Display	*dpy;
    Window 	parent;
    Arg 	*args;
    int 	argCount;
    XrmNameList 	names;
    XrmClassList classes;
    int 	*src;
{
    Window textWidget;
    ArgList mergedArgs;
    Arg myArgs[2];

    XtSetArg(myArgs[0], XtNtextSource, (XtArgVal) src);
    XtSetArg(myArgs[1], XtNtextSink, 
	(XtArgVal) XtAsciiSinkCreate(dpy, textFontStruct,
				     foreground, background));

    mergedArgs = XtMergeArgLists(args, argCount, myArgs, XtNumber(myArgs) );
    textWidget = XtTextCreate(
	dpy, parent, mergedArgs, argCount+XtNumber(myArgs));

    XtSetNameAndClass(dpy, textWidget, names, classes);
    XrmFreeNameList(names);
    XrmFreeClassList(classes);
    XtFree((char *) mergedArgs);
    return textWidget;
}

/* Public routines. */

Window XtTextDiskCreate(dpy, parent, args, argCount)
    Display	*dpy;
    Window  parent;
    ArgList args;
    int     argCount;
{
    XrmNameList 	names;
    XrmClassList classes;
    char *tmpName;

    if (!initialized) TextKindsInitialize(dpy);
    XtGetResources (dpy,
	diskResources, XtNumber(diskResources), args, argCount, parent,
	"text", "Text",	&names, &classes);
    /* NOTE:  Do not want to leave temp file around.  Must be unlinked
    somewhere */
    if (fileName == NULL) {
	tmpName = tmpnam (fileName);
	fileName = XtMalloc (sizeof (tmpName));
	(void) strcpy (fileName, tmpName);
    }
    source = XtDiskSourceCreate (fileName, editMode);
    return (AsciiTextCreate (dpy, parent, args, argCount, names, classes, source));
}

XtTextDiskDestroy(dpy, w)
  Display *dpy;
  Window w;
{
    XDestroyWindow(dpy, w);
    (void) XtSendDestroyNotify(dpy, w);
}

Window XtTextStringCreate(dpy, parent, args, argCount)
    Display *dpy;
    Window  parent;
    ArgList args;
    int     argCount;
{
    XrmNameList 	names;
    XrmClassList classes;

    if (!initialized) TextKindsInitialize(dpy);
    XtGetResources (dpy, stringResources, XtNumber(stringResources), args, argCount,
        parent, "text", "Text", &names, &classes);
    if (textString == NULL) {
	textString = (char *) XtMalloc (1000);
	textLength = 1000;
    }
    /* need option for specifying read append mode */
    source = XtStringSourceCreate (textString, textLength, editMode);
    return (AsciiTextCreate (dpy, parent, args, argCount, names, classes, source));
}

XtTextStringDestroy(dpy, w)
  Display *dpy;
  Window w;
{
    XDestroyWindow(dpy, w);
    (void) XtSendDestroyNotify(dpy, w);
}

Window XtTextSourceCreate(dpy, parent, args, argCount, passedSource)
    Display *dpy;
    Window  parent;
    ArgList args;
    int     argCount;
    int     *passedSource;  /* ||| NOTE:  should be in args */
{
    XrmNameList 	names;
    XrmClassList classes;

    if (!initialized) TextKindsInitialize(dpy);
    XtGetResources (dpy, rawResources, XtNumber(rawResources), args, argCount, parent,
	"text", "Text",	&names, &classes);
    return (AsciiTextCreate (dpy, parent, args, argCount, names, classes,
        passedSource));
}

XtTextSourceDestroy(dpy, w)
  Display *dpy;
  Window w;
{
    XDestroyWindow(dpy, w);
    (void) XtSendDestroyNotify(dpy, w);
}

