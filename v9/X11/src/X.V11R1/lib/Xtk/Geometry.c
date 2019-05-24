#ifndef lint
static char rcsid[] = "$Header: Geometry.c,v 1.15 87/09/11 21:23:59 haynes Rel $";
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

#include "Intrinsic.h"

/* Public routines */

void XtResizeWindow(w)
    Widget w;
{
    if (XtIsRealized(w)) {
	XWindowChanges changes;
	changes.width = w->core.width;
	changes.height = w->core.height;
	changes.border_width = w->core.border_width;
	XConfigureWindow(XtDisplay(w), XtWindow(w),
	    (unsigned) CWWidth | CWHeight | CWBorderWidth, &changes);
    }
} /* XtResizeWindow */


void XtResizeWidget(w, width, height, borderWidth)
    Widget w;
    Dimension height, width, borderWidth;
{
    XWindowChanges changes;
    Cardinal mask = 0;

    if (w->core.width != width) {
	changes.width = w->core.width = width;
	mask |= CWWidth;
    }

    if (w->core.height != height) {
	changes.height = w->core.height = height;
	mask |= CWHeight;
    }

    if (w->core.border_width != borderWidth) {
	changes.border_width = w->core.border_width = borderWidth;
	mask |= CWBorderWidth;
    }

    if (mask != 0) {
	if (XtIsRealized(w))
	    XConfigureWindow(XtDisplay(w), XtWindow(w), mask, &changes);
    if ((mask & (CWWidth | CWHeight)) &&
	XtClass(w)->core_class.resize != (XtWidgetProc) NULL)
	    w->core.widget_class->core_class.resize(w);
    }
} /* XtResizeWidget */

XtGeometryResult XtMakeGeometryRequest (widget, request, reply)
    Widget         widget;
    XtWidgetGeometry *request, *reply;
{
    XtWidgetGeometry    junk;
    XtGeometryHandler manager;
    XtGeometryResult returnCode;

    if (! XtIsComposite(widget->core.parent)) {
	/* Should never happen - XtCreateWidget should have checked */
	XtError("XtMakeGeometryRequest - parent not composite");
    }
    manager = ((CompositeWidgetClass) (widget->core.parent->core.widget_class))
    		->composite_class.geometry_manager;
    if (manager == (XtGeometryHandler) NULL) {
	XtError("XtMakeGeometryRequest - parent has no geometry manger");
    }
    if ( ! widget->core.managed) {
	XtWarning("XtMakeGeometryRequest - widget not managed");
	return XtGeometryNo;
    }
    if (widget->core.being_destroyed) {
        return XtGeometryNo;
    }
    if (reply == (XtWidgetGeometry *) NULL) {
        returnCode = (*manager)(widget, request, &junk);
    } else {
	returnCode = (*manager)(widget, request, reply);
    }
    /* ||| Right now this is automatic.  However, we may want it to be
    explicitely called by geometry manager in order to effect the window resize
    (especially to smaller size) before the windows are layed out. */
    if (returnCode == XtGeometryYes) {
	XtResizeWindow(widget);
    }
    return returnCode;
} /* XtMakeGeometryRequest */

XtGeometryResult XtMakeResizeRequest
	(widget, width, height, replyWidth, replyHeight)
    Widget	widget;
    Dimension	width, height;
    Dimension	*replyWidth, *replyHeight;
{
    XtWidgetGeometry request, reply;
    XtGeometryResult r;

    request.request_mode = CWWidth | CWHeight;
    request.width = width;
    request.height = height;
    r = XtMakeGeometryRequest(widget, &request, &reply);
    if (replyWidth != NULL)
	if (r == XtGeometryAlmost && reply.request_mode & CWWidth)
	    *replyWidth = reply.width;
	else
	    *replyWidth = width;
    if (replyHeight != NULL)
	if (r == XtGeometryAlmost && reply.request_mode & CWHeight)
	    *replyHeight = reply.height;
	else
	    *replyWidth = width;
    return r;
} /* XtMakeResizeRequest */

void XtMoveWidget(w, x, y)
    Widget w;
    Position x, y;
{
    if ((w->core.x != x) || (w->core.y != y)) {
	w->core.x = x;
	w->core.y = y;
	if (XtIsRealized(w)) {
	    XMoveWindow(XtDisplay(w), XtWindow(w), w->core.x, w->core.y);
        }
    }
} /* XtMoveWidget */

