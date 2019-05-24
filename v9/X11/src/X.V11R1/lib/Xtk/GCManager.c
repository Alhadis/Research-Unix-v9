#ifndef lint
static char rcsid[] = "$Header: GCManager.c,v 1.12 87/09/13 16:44:10 newman Exp $";
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
#include "Intrinsic.h"

typedef struct _GCrec {
    Display	*dpy;		/* Display for GC */
    Screen	*screen;	/* Screen for GC */
    int		depth;		/* Depth for GC */
    int         ref_count;      /* # of shareholders */
    GC 		gc;		/* The GC itself. */
    int 	valueMask;	/* What fields are being used right now. */
    XGCValues 	values;		/* What values those fields have. */
    struct _GCrec *next;	/* Next GC for this widgetkind. */
} GCrec, *GCptr;


static GCrec *GClist = NULL;

static int Matches(ptr,widget, valueMask, v)
    GCptr		ptr;
    Widget		widget;
    unsigned long	valueMask;
    register XGCValues	*v;
{
#define CheckGCField(MaskBit,fieldName) \
    if (m & MaskBit) if (p->fieldName != v->fieldName) return 0

    register int m = ptr->valueMask & valueMask;
    register XGCValues *p = &(ptr->values);

    if (ptr->valueMask != valueMask) return 0;
    if (ptr->depth != widget->core.depth) return 0;
    if (ptr->screen != XtScreen(widget)) return 0;

    CheckGCField( GCFunction,		function);
    CheckGCField( GCPlaneMask,		plane_mask);
    CheckGCField( GCForeground,		foreground);
    CheckGCField( GCBackground,		background);
    CheckGCField( GCLineWidth,		line_width);
    CheckGCField( GCLineStyle,		line_style);
    CheckGCField( GCCapStyle,		cap_style);
    CheckGCField( GCJoinStyle,		join_style);
    CheckGCField( GCFillStyle,		fill_style);
    CheckGCField( GCFillRule,		fill_rule);
    CheckGCField( GCArcMode,		arc_mode);
    CheckGCField( GCTile,		tile);
    CheckGCField( GCStipple,		stipple);
    CheckGCField( GCTileStipXOrigin,	ts_x_origin);
    CheckGCField( GCTileStipYOrigin,	ts_y_origin);
    CheckGCField( GCFont,		font);
    CheckGCField( GCSubwindowMode,	subwindow_mode);
    CheckGCField( GCGraphicsExposures,	graphics_exposures);
    CheckGCField( GCClipXOrigin,	clip_x_origin);
    CheckGCField( GCClipYOrigin,	clip_y_origin);
    CheckGCField( GCClipMask,		clip_mask);
    CheckGCField( GCDashOffset,		dash_offset);
    CheckGCField( GCDashList,		dashes);
#undef CheckGCField
    return 1;
}

static void SetFields(ptr, valueMask, v)
GCptr ptr;
    register unsigned long valueMask;
    register XGCValues    *v;
{
#define SetGCField(MaskBit,fieldName) \
    if (valueMask & MaskBit) p->fieldName = v->fieldName

    register XGCValues *p = &(ptr->values);

    SetGCField( GCFunction,		function);
    SetGCField( GCPlaneMask,		plane_mask);
    SetGCField( GCForeground,		foreground);
    SetGCField( GCBackground,		background);
    SetGCField( GCLineWidth,		line_width);
    SetGCField( GCLineStyle,		line_style);
    SetGCField( GCCapStyle,		cap_style);
    SetGCField( GCJoinStyle,		join_style);
    SetGCField( GCFillStyle,		fill_style);
    SetGCField( GCFillRule,		fill_rule);
    SetGCField( GCArcMode,		arc_mode);
    SetGCField( GCTile,			tile);
    SetGCField( GCStipple,		stipple);
    SetGCField( GCTileStipXOrigin,	ts_x_origin);
    SetGCField( GCTileStipYOrigin,	ts_y_origin);
    SetGCField( GCFont,			font);
    SetGCField( GCSubwindowMode,	subwindow_mode);
    SetGCField( GCGraphicsExposures,	graphics_exposures);
    SetGCField( GCClipXOrigin,		clip_x_origin);
    SetGCField( GCClipYOrigin,		clip_y_origin);
    SetGCField( GCClipMask,		clip_mask);
    SetGCField( GCDashOffset,		dash_offset);
    SetGCField( GCDashList,		dashes);
    ptr->valueMask |= valueMask;
    XChangeGC(ptr->dpy, ptr->gc, valueMask, p);
#undef SetGCField
}


/* 
 * Return a read-only GC with the given values.  
 */

GC XtGetGC(widget, valueMask, values)
    Widget	widget;
    XtGCMask	valueMask;
    XGCValues	*values;
{
    GCptr first=GClist;
    register GCptr cur;
    Drawable drawable;

    for (cur = first; cur != NULL; cur = cur->next) {
	if (Matches(cur, widget,valueMask, values)) {
	    valueMask &= ~cur->valueMask;
	    if (valueMask) SetFields(cur, valueMask, values);
            cur->ref_count++;
	    return cur->gc;
	}
    }
    cur = (GCptr) XtMalloc((unsigned)sizeof(GCrec));
    cur->dpy = XtDisplay(widget);
    cur->screen = XtScreen(widget);
    cur->depth = widget->core.depth;
    cur->ref_count = 1;
    if (XtWindow(widget) == NULL)
	drawable = XCreatePixmap(XtDisplay(widget), XtScreen(widget)->root,
			1,1,widget->core.depth);
    else drawable = XtWindow(widget);
    cur->gc = XCreateGC(XtDisplay(widget), drawable, valueMask, values);
    cur->valueMask = valueMask;
    cur->values = *values;
    cur->next = first;
    GClist = cur;
    return cur->gc;
}

void  XtDestroyGC(gc)
    GC gc;
{
    GCptr first=GClist;
    GCptr cur;
    
    for (cur = first; cur != NULL; cur = cur->next) 
      if (cur->gc == gc) 
         if (--(cur->ref_count) == 0) XFreeGC(cur->dpy, gc);     
    return;
}

