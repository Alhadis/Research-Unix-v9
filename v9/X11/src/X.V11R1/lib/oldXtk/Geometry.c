/* $Header: Geometry.c,v 1.1 87/09/11 07:57:54 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)Geometry.c	1.7	2/25/87";
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


/* File: Geometry.c */

#include "Xlib.h"
#include "Intrinsic.h"

/* Private  Definitions */

static XContext geometryContext;

static Boolean initialized = FALSE;

extern void GeometryInitialize()
{
    if (initialized)
    	return;
    initialized = TRUE;

    geometryContext = XUniqueContext();
}

/* Public routines */

/*
 * The "normal" case is for the geometry manager to be associated with a
 * windows parent. This code currently looks for an association on the passed
 * window then it's parent. In the real version of this code the request
 * should propagate up the tree. Note that parent windows may not be in the
 * requesting windows address space and the propagation will require a server
 * dialog.
 */

XtGeometryReturnCode XtMakeGeometryRequest
                            (dpy, window, request, requestBox, replyBox)
    Display		*dpy;
    Window     		window;
    XtGeometryRequest   request;
    WindowBox 		*requestBox, *replyBox;
{
    XtGeometryHandler proc;
    int error;

    error  = XFindContext (dpy, window, geometryContext, (caddr_t *) &proc);
    if (error == XCNOENT)
	return(XtgeometryNoManager);
    return ((*(proc)) (dpy, window, request, requestBox, replyBox));
}

/*
 * Register a geometry routine with the toolkit dispatcher.
 */

XtStatus XtSetGeometryHandler (dpy, w, proc)
    Display	      *dpy;
    Window 	      w;
    XtGeometryHandler proc;
{
    return (XSaveContext (dpy, w, geometryContext, (caddr_t) proc));
}

/*
 * Return the geometry procedure associated with a widget.
 */

XtStatus XtGetGeometryHandler (dpy, w, proc)
    Display	      *dpy;
    Window 	      w;
    XtGeometryHandler *proc;
{
    return (XFindContext(dpy, w, geometryContext, (caddr_t *) proc));
}

/*
 * Delete the geometry routine's entry in the toolkit dispatcher.
 */

XtStatus XtClearGeometryHandler (dpy, w)
Display *dpy;
Window w;
{
    return(XDeleteContext (dpy, w, geometryContext));
}
