/* $Header: Resources.c,v 1.1 87/09/11 07:59:15 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)ResourceList.c	1.10	2/25/87";
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


/* XtResourceList.c -- compile and process resource lists. */

#include "Xlib.h"
#include "Intrinsic.h"
#include "Atoms.h"
#include "Xresource.h"
#include <stdio.h>


#define XtNewNameList()			((XrmNameList) XrmNewQuarkList())
#define XtCopyNameList(names)		((XrmNameList) XrmCopyQuarkList(names))

#define XtNewClassList()		((XrmClassList) XrmNewQuarkList())
#define XtCopyClassList(classes)	((XrmClassList) XrmCopyQuarkList(classes))

int    XtDefaultFGPixel, XtDefaultBGPixel;
Pixmap XtDefaultFGPixmap, XtDefaultBGPixmap;

static XrmName	Qname, QreverseVideo;
static XrmClass	QBoolean, QString;

static XContext nameListContext;    /* Context to attach NameList to window  */
static XContext classListContext;   /* Context to attach ClassList to window */

extern void bcopy();

static void CopyFromArg(src, dst, size)
    XtArgVal src, dst;
    register unsigned int size;
{
    if (size == sizeof(XtArgVal))
	*(XtArgVal *)dst = src;
#ifdef BIGENDIAN
    else if (size == sizeof(short)) 
	*(short *)dst = (short)src;
#endif BIGENDIAN
    else if (size < sizeof(XtArgVal))
	bcopy((char *) &src, (char *) dst, (int) size);
    else
	bcopy((char *) src, (char *) dst, (int) size);

}

static void CopyToArg(src, dst, size)
    XtArgVal src, *dst;
    register unsigned int size;
{
    if (size == sizeof(XtArgVal))
	*dst = *(XtArgVal *)src;
#ifdef BIGENDIAN
    else if (size == sizeof(short)) 
	*dst = (XtArgVal) *((short *) src);
#endif BIGENDIAN
    else if (size < sizeof(XtArgVal))
	bcopy((char *) src, (char *) dst, (int) size);
    else
	bcopy((char *) src, (char *) *dst, (int) size);

}

void PrintResourceList(list, count)
    register ResourceList list;
    register int count;
{
    for (; --count >= 0; list++) {
        (void) printf("    name: %s, class, %s, type: %s,\n",
	    list->name, list->class, list->type);
	(void) printf("    size: %d, addr: 0x%x, defaddr: 0x%x\n",
	    list->size, list->addr, list->defaultaddr);
    }
}

static void GetNameAndClass(dpy, w, names, classes)
    register Display	*dpy;
    register Window	 w;
    	     XrmNameList  *names;
	     XrmClassList *classes;
{
    if (XFindContext(dpy, w, nameListContext,  (caddr_t *)names))
    	*names = XtNewNameList();
    if (XFindContext(dpy, w, classListContext, (caddr_t *)classes))
        *classes = XtNewClassList();
}

void XtSetNameAndClass(dpy, w, names, classes)
    register Display	*dpy;
    register Window	 w;
    	     XrmNameList	 names;
	     XrmClassList classes;
{
    (void)XSaveContext(
	dpy, w, nameListContext,  (caddr_t)XtCopyNameList(names));
    (void)XSaveContext(
	dpy, w, classListContext, (caddr_t)XtCopyClassList(classes));
}

static	XrmName		resourceNames[1000];

void XtGetResources(
  dpy,
  resources, resourceCount, args, argCount,
  parent, widgetName, widgetClass, names, classes)

  Display	*dpy;		/* The widget's display connection	   */
  ResourceList	resources;	/* The list of resources required. 	   */
  int 		resourceCount;  /* number of items in resource list   	   */
  ArgList 	args;		/* ArgList to override resources	   */
  int 		argCount;	/* number of items in arg list		   */
  Window 	parent;		/* Parent window for computing inheritance */
  XrmAtom 	widgetName;	/* name of this widget (may be overridden) */
  XrmAtom	widgetClass;   	/* class of this widget 		   */
  register XrmNameList *names;	/* Full inheritance name of widget window  */
  register XrmClassList *classes;/* Full inheritance class of widget window */
{
    register 	ArgList		arg;
    register 	XrmName		argName;
    register 	ResourceList 	res;
    		XrmNameList	parentNames;
		XrmClassList	parentClasses;
    		XrmValue	val;
    register 	int		j;
    		int		length, i;
		Boolean		 reverseVideo, getReverseVideo;
    		XrmHashTable	searchList[100];
    static	Boolean		found[1000];

    reverseVideo    = FALSE;
    getReverseVideo = TRUE;

    if (((args == NULL) && (argCount != 0)) ||
        ((resources == NULL) && (resourceCount != 0))) {
	/* ||| call warning handler here */
	return;
    }

    if (resourceCount != 0) {
	for (res = resources, j = 0; j < resourceCount; j++, res++) {
	    found[j] = FALSE;
	    resourceNames[j] = XrmAtomToName(res->name);
    	}

	/* find the widget name and copy the args into the resources */
	for (arg = args, i = 0; i < argCount; i++, arg++) {
	    argName = XrmAtomToName(arg->name);
	    if (argName == Qname) {
		widgetName = (XrmAtom) arg->value;
	    } else if (argName == QreverseVideo) {
		reverseVideo = (Boolean) arg->value;
		getReverseVideo = FALSE;
	    } else {
		for (j = 0, res = resources; j < resourceCount; j++, res++) {
		    if (argName == resourceNames[j]) {
			CopyFromArg(arg->value, res->addr, res->size);
			found[j] = TRUE;
			break;
		    }
		}
	    }
	}
    }

    /* set up the name of the widget */
    GetNameAndClass(dpy, parent, &parentNames, &parentClasses);
    length = XrmNameListLength(parentNames);
    *names = (XrmNameList) XtCalloc((unsigned) length+3, sizeof(XrmName));
    *classes = (XrmClassList) XtCalloc((unsigned) length+3, sizeof(XrmClass));
    for (i=0; i<length; i++) {
	(*names)[i] = parentNames[i];
	(*classes)[i] = parentClasses[i];
    }
    (*names)[length]     = XrmAtomToName(widgetName);
    (*classes)[length]   = XrmAtomToClass(widgetClass);
    (*names)[length+1]   = NULLQUARK;
    (*classes)[length+1] = NULLQUARK;
    (*names)[length+2]   = NULLQUARK;
    (*classes)[length+2] = NULLQUARK;

    if (XDisplayCells(dpy, DefaultScreen(dpy)) > 2) {
    	/* Color box, ignore ReverseVideo */
	reverseVideo = FALSE;
    } else if (getReverseVideo) {
	(*names)[length+1] = QreverseVideo;
	(*classes)[length+1] = QBoolean;
	XrmGetResource(dpy, *names, *classes, QBoolean, &val);
	if (val.addr)
	    reverseVideo = *((Boolean *) val.addr);
    }
    if (reverseVideo) {
	XtDefaultFGPixel = WhitePixel(dpy, DefaultScreen(dpy));
	XtDefaultBGPixel = BlackPixel(dpy, DefaultScreen(dpy));
    } else {
	XtDefaultFGPixel = BlackPixel(dpy, DefaultScreen(dpy));
	XtDefaultBGPixel = WhitePixel(dpy, DefaultScreen(dpy));
    }

    (*names)[length+1] = NULLQUARK;
    (*classes)[length+1] = NULLQUARK;

    if (resourceCount != 0) {

	/* Ask resource manager for a list of database levels that we can
	   do a single-level search on each resource */

	XrmGetSearchList(*names, *classes, searchList);
	
	/* go to the resource manager for those resources not found yet */
	/* if it's not in the resource database copy the default value in */
    
	for (res = resources, j = 0; j < resourceCount; j++, res++) {
	    if (! found[j]) {
		XrmGetSearchResource(dpy, searchList, resourceNames[j],
		 XrmAtomToClass(res->class), res->type, &val);
		if (val.addr) {
/* ||| Kludgy */
		    if (XrmAtomToQuark(res->type) == QString) {
			*((caddr_t *)(res->addr)) = val.addr;
#ifdef BIGENDIAN
/* ||| Why? This should be handled by string to short, etc. conversions */
          	    } else if (res->size == sizeof(short)) {
		        *(short *) (res->addr) = (short)*((int *)val.addr);
#endif BIGENDIAN
		    } else {
		        bcopy(
			    (char *) val.addr,
			    (char *) res->addr, 
			    (int) res->size);
		    }
		} else if (res->defaultaddr != NULL) {
		    bcopy(
			(char *)res->defaultaddr,
			(char *)res->addr,
			(int) res->size);
		}
	    }
	}
    }
}

void XtGetValues(resources, resourceCount, args, argCount)
  register ResourceList resources;	/* The current resource values. */
  int 			resourceCount;	/* number of items in resources */
  ArgList 		args;		/* The resource values requested */
  int			argCount;	/* number of items in arg list */
{
    register ArgList		arg;
    register ResourceList	res;
    register int 		i;
    register XrmName		argName;

    if (argCount == 0) return;
    if (((args == NULL) && (argCount != 0)) ||
        ((resources == NULL) && (resourceCount != 0))) {
	/* call warning handler here */
	return;
    }

    for (res = resources, i = 0; i < resourceCount; i++, res++) {
	resourceNames[i] = XrmAtomToName(res->name);
    }
    for (arg = args ; --argCount >= 0; arg++) {
	argName = XrmAtomToName(arg->name);
	for (res = resources, i = 0; i < resourceCount; i++, res++) {
	    if (argName == resourceNames[i]) {
		CopyToArg(res->addr, &arg->value, res->size);
		break;
	    }
	}
    }
}

void XtSetValues(resources, resourceCount, args, argCount)
  register ResourceList resources;	/* The current resource values. */
  int 			resourceCount;	/* number of items in resources */
  ArgList 		args;		/* The resource values to set */
  int			argCount;	/* number of items in arg list */
{
    register ArgList		arg;
    register ResourceList	res;
    register int 	        i;
    register XrmName		argName;

    if (argCount == 0) return;
    if (((args == NULL) && (argCount != 0)) ||
        ((resources == NULL) && (resourceCount != 0))) {
	/* call warning handler here */
	return;
    }

    for (res = resources, i = 0; i < resourceCount; i++, res++) {
	resourceNames[i] = XrmAtomToQuark(res->name);
    }
    for (arg = args ; --argCount >= 0; arg++) {
	argName = XrmAtomToName(arg->name);
	for (res = resources, i = 0; i < resourceCount; i++, res++) {
	    if (argName == resourceNames[i]) {
		CopyFromArg(arg->value, res->addr, res->size);
		break;
	    }
	}
    }
}

static Boolean initialized = FALSE;

extern void ResourceListInitialize()
{
    if (initialized)
    	return;
    initialized = TRUE;

    nameListContext  = XUniqueContext();
    classListContext = XUniqueContext();

    Qname = XrmAtomToName(XtNname);
    QreverseVideo = XrmAtomToName(XtNreverseVideo);
    QBoolean = XrmAtomToClass(XtCBoolean);
    QString = XrmAtomToClass(XtCString);
}
