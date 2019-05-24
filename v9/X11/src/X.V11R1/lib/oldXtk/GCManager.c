/* $Header: GCManager.c,v 1.1 87/09/11 07:57:35 toddb Exp $ */
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

typedef struct _GCrec {
    Display	*dpy;		/* Display for GC */
    GC 		gc;		/* The GC itself. */
    int 	valueMask;	/* What fields are being used right now. */
    XGCValues 	values;		/* What values those fields have. */
    struct _GCrec *next;	/* Next GC for this widgetkind. */
} GCrec, *GCptr;

static XContext gcManagerContext;

static Boolean initialized = FALSE;

void GCManagerInitialize()
{
    if (initialized)
    	return;
    initialized = TRUE;

    gcManagerContext = XUniqueContext();
}

static int Matches(ptr, valueMask, v)
    GCptr		ptr;
    unsigned long	valueMask;
    register XGCValues	*v;
{
    register int m = ptr->valueMask & valueMask;
    register XGCValues *p = &(ptr->values);
    int result;
    result = 
	(((m & GCFunction) == 0) || p->function == v->function) &&
	(((m & GCPlaneMask) == 0) || p->plane_mask == v->plane_mask) &&
        (((m & GCForeground) == 0) || p->foreground == v->foreground) &&
	(((m & GCBackground) == 0) || p->background == v->background) &&
	(((m & GCLineWidth) == 0) || p->line_width == v->line_width) &&
	(((m & GCLineStyle) == 0) || p->line_style == v->line_style) &&
	(((m & GCCapStyle) == 0) || p->cap_style == v->cap_style) &&
	(((m & GCJoinStyle) == 0) || p->join_style == v->join_style);
    result = result &&
	(((m & GCFillStyle) == 0) || p->fill_style == v->fill_style) &&
	(((m & GCFillRule) == 0) || p->fill_rule == v->fill_rule) &&
	(((m & GCArcMode) == 0) || p->arc_mode == v->arc_mode) &&
	(((m & GCTile) == 0) || p->tile == v->tile) &&
	(((m & GCStipple) == 0) || p->stipple == v->stipple) &&
	(((m & GCTileStipXOrigin) == 0) || p->ts_x_origin == v->ts_x_origin) &&
	(((m & GCTileStipYOrigin) == 0) || p->ts_y_origin == v->ts_y_origin) &&
	(((m & GCFont) == 0) || p->font == v->font);
    result = result &&
	(((m & GCSubwindowMode) == 0) || p->subwindow_mode == v->subwindow_mode) &&
	(((m & GCGraphicsExposures) == 0) || p->graphics_exposures == v->graphics_exposures) &&
	(((m & GCClipXOrigin) == 0) || p->clip_x_origin == v->clip_x_origin) &&
	(((m & GCClipYOrigin) == 0) || p->clip_y_origin == v->clip_y_origin) &&
	(((m & GCClipMask) == 0) || p->clip_mask == v->clip_mask) &&
	(((m & GCDashOffset) == 0) || p->dash_offset == v->dash_offset) &&
	(((m & GCDashList) == 0) || p->dashes == v->dashes);
     return result;
}

static void SetFields(ptr, valueMask, v)
GCptr ptr;
    register unsigned long valueMask;
    register XGCValues    *v;
{
    register XGCValues *p = &(ptr->values);
    if (valueMask & GCFunction)
	p->function = v->function;
    if (valueMask & GCPlaneMask)
	p->plane_mask = v->plane_mask;
    if (valueMask & GCForeground)
	p->foreground = v->foreground;
    if (valueMask & GCBackground)
	p->background = v->background;
    if (valueMask & GCLineWidth)
	p->line_width = v->line_width;
    if (valueMask & GCLineStyle)
	p->line_style = v->line_style;
    if (valueMask & GCCapStyle)
	p->cap_style = v->cap_style;
    if (valueMask & GCJoinStyle)
	p->join_style = v->join_style;
    if (valueMask & GCFillStyle)
	p->fill_style = v->fill_style;
    if (valueMask & GCFillRule)
	p->fill_rule = v->fill_rule;
    if (valueMask & GCArcMode)
	p->arc_mode = v->arc_mode;
    if (valueMask & GCTile)
	p->tile = v->tile;
    if (valueMask & GCStipple)
	p->stipple = v->stipple;
    if (valueMask & GCTileStipXOrigin)
	p->ts_x_origin = v->ts_x_origin;
    if (valueMask & GCTileStipYOrigin)
	p->ts_y_origin = v->ts_y_origin;
    if (valueMask & GCFont)
	p->font = v->font;
    if (valueMask & GCSubwindowMode)
	p->subwindow_mode = v->subwindow_mode;
    if (valueMask & GCGraphicsExposures)
	p->graphics_exposures = v->graphics_exposures;
    if (valueMask & GCClipXOrigin)
	p->clip_x_origin = v->clip_x_origin;
    if (valueMask & GCClipYOrigin)
	p->clip_y_origin = v->clip_y_origin;
    if (valueMask & GCClipMask)
	p->clip_mask = v->clip_mask;
    if (valueMask & GCDashOffset)
	p->dash_offset = v->dash_offset;
    if (valueMask & GCDashList)
	p->dashes = v->dashes;
    ptr->valueMask |= valueMask;
    XChangeGC(ptr->dpy, ptr->gc, valueMask, p);
}


/* 
 * Return a read-only GC with the given values.  WidgetKind is some kind of 
 * unique context identifying the kind of widget that is sharing this GC.
 * It's purely an efficiency hack, based on the theory that only identical
 * widgets will be able to share GC's.
 */

GC XtGetGC(dpy, widgetKind, drawable, valueMask, values)
    Display	*dpy;
    XContext	widgetKind;
    Drawable	drawable;
    GCMask	valueMask;
    XGCValues	*values;
{
    GCptr first;
    register GCptr cur;

    if (XFindContext(
	dpy, (Window) widgetKind, gcManagerContext, (caddr_t *) &first)
     == XCNOENT)
	first = NULL;
    for (cur = first; cur != NULL; cur = cur->next) {
	if (Matches(cur, valueMask, values)) {
	    valueMask &= ~cur->valueMask;
	    if (valueMask) SetFields(cur, valueMask, values);
	    return cur->gc;
	}
    }
    cur = (GCptr) XtMalloc(sizeof(GCrec));
    cur->dpy = dpy;
    cur->gc = XCreateGC(dpy, drawable, valueMask, values);
    cur->valueMask = valueMask;
    cur->values = *values;
    cur->next = first;
    (void) XSaveContext(
	dpy, (Window) widgetKind, gcManagerContext, (caddr_t) cur);
    return cur->gc;
}
