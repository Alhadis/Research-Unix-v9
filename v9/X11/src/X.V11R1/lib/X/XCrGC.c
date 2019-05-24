#include "copyright.h"

/* $Header: XCrGC.c,v 11.21 87/09/09 15:58:25 newman Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

static XGCValues initial_GC = {
    GXcopy, 	/* function */
    AllPlanes,	/* plane_mask */
    0L,		/* foreground */
    1L,		/* background */
    0,		/* line_width */
    LineSolid,	/* line_style */
    CapButt,	/* cap_style */
    JoinMiter,	/* join_style */
    FillSolid,	/* fill_style */
    EvenOddRule,/* fill_rule */
    ArcPieSlice,/* arc_mode */
    ~0,		/* tile, impossible (unknown) resource */
    ~0,		/* stipple, impossible (unknown) resource */
    0,		/* ts_x_origin */
    0,		/* ts_y_origin */
    ~0,		/* font, impossible (unknown) resource */
    ClipByChildren, /* subwindow_mode */
    True,	/* graphics_exposures */
    0,		/* clip_x_origin */
    0,		/* clip_y_origin */
    None,	/* clip_mask */
    0,		/* dash_offset */
    4		/* dashes (list [4,4]) */
};

GC XCreateGC (dpy, d, valuemask, values)
     register Display *dpy;
     Drawable d;		/* Window or Pixmap for which depth matches */
     unsigned long valuemask;	/* which ones to set initially */
     XGCValues *values;		/* the values themselves */
{
    register GC gc;
    register xCreateGCReq *req;
    register _XExtension *ext;

    LockDisplay(dpy);
    if ((gc = (GC)Xmalloc (sizeof(struct _XGC))) == NULL) {
        errno = ENOMEM;
	(*_XIOErrorFunction)(dpy);
	UnlockDisplay(dpy);
	return (NULL);
    }
    gc->rects = 0;
    gc->dashes = 0;
    gc->ext_data = NULL;
    gc->values = initial_GC;
    gc->dirty = 0L;

    if (valuemask) _XUpdateGCCache (gc, valuemask, values);

    GetReq(CreateGC, req);
    req->drawable = d;
    req->gc = gc->gid = XAllocID(dpy);

    if (req->mask = gc->dirty)
        _XGenerateGCList (dpy, gc, (xReq *) req);
    ext = dpy->ext_procs;
    while (ext) {		/* call out to any extensions interested */
	if (ext->create_GC != NULL) (*ext->create_GC)(dpy, gc, &ext->codes);
	ext = ext->next;
	}    
    UnlockDisplay(dpy);
    SyncHandle();
    return (gc);
    }

/*
 * GenerateGCList looks at the GC dirty bits, and appends all the required
 * long words to the request being generated.  Clears the dirty bits in
 * the GC.
 */

_XGenerateGCList (dpy, gc, req)
    register Display *dpy;
    xReq *req;
    GC gc;
    {
    /* Warning!  This code assumes that "unsigned long" is 32-bits wide */

    unsigned long values[32];
    register unsigned long *value = values;
    long nvalues;
    register XGCValues *gv = &gc->values;
    register unsigned long dirty = gc->dirty;

    /*
     * Note: The order of these tests are critical; the order must be the
     * same as the GC mask bits in the word.
     */
    if (dirty & GCFunction)          *value++ = gv->function;
    if (dirty & GCPlaneMask)         *value++ = gv->plane_mask;
    if (dirty & GCForeground)        *value++ = gv->foreground;
    if (dirty & GCBackground)        *value++ = gv->background;
    if (dirty & GCLineWidth)         *value++ = gv->line_width;
    if (dirty & GCLineStyle)         *value++ = gv->line_style;
    if (dirty & GCCapStyle)          *value++ = gv->cap_style;
    if (dirty & GCJoinStyle)         *value++ = gv->join_style;
    if (dirty & GCFillStyle)         *value++ = gv->fill_style;
    if (dirty & GCFillRule)          *value++ = gv->fill_rule;
    if (dirty & GCTile)              *value++ = gv->tile;
    if (dirty & GCStipple)           *value++ = gv->stipple;
    if (dirty & GCTileStipXOrigin)   *value++ = gv->ts_x_origin;
    if (dirty & GCTileStipYOrigin)   *value++ = gv->ts_y_origin;
    if (dirty & GCFont)              *value++ = gv->font;
    if (dirty & GCSubwindowMode)     *value++ = gv->subwindow_mode;
    if (dirty & GCGraphicsExposures) *value++ = gv->graphics_exposures;
    if (dirty & GCClipXOrigin)       *value++ = gv->clip_x_origin;
    if (dirty & GCClipYOrigin)       *value++ = gv->clip_y_origin;
    if (dirty & GCClipMask)          *value++ = gv->clip_mask;
    if (dirty & GCDashOffset)        *value++ = gv->dash_offset;
    if (dirty & GCDashList)          *value++ = gv->dashes;
    if (dirty & GCArcMode)           *value++ = gv->arc_mode;

    req->length += (nvalues = value - values);

    /* 
     * note: Data is a macro that uses its arguments multiple
     * times, so "nvalues" is changed in a separate assignment
     * statement 
     */

    nvalues <<= 2;
    Data (dpy, (char *) values, nvalues);
    gc->dirty = 0L;

    }


_XUpdateGCCache (gc, mask, att)
    register long mask;
    register XGCValues *att;
    register GC gc;
    {
    register XGCValues *gv = &gc->values;

    if (mask & GCFunction)
        if (gv->function != att->function) {
	  gv->function = att->function;
	  gc->dirty |= GCFunction;
	}
	
    if (mask & GCPlaneMask)
        if (gv->plane_mask != att->plane_mask) {
            gv->plane_mask = att->plane_mask;
	    gc->dirty |= GCPlaneMask;
	  }

    if (mask & GCForeground)
        if (gv->foreground != att->foreground) {
            gv->foreground = att->foreground;
	    gc->dirty |= GCForeground;
	  }

    if (mask & GCBackground)
        if (gv->background != att->background) {
            gv->background = att->background;
	    gc->dirty |= GCBackground;
	  }

    if (mask & GCLineWidth)
        if (gv->line_width != att->line_width) {
            gv->line_width = att->line_width;
	    gc->dirty |= GCLineWidth;
	  }

    if (mask & GCLineStyle)
        if (gv->line_style != att->line_style) {
            gv->line_style = att->line_style;
	    gc->dirty |= GCLineStyle;
	  }

    if (mask & GCCapStyle)
        if (gv->cap_style != att->cap_style) {
            gv->cap_style = att->cap_style;
	    gc->dirty |= GCCapStyle;
	  }
    
    if (mask & GCJoinStyle)
        if (gv->join_style != att->join_style) {
            gv->join_style = att->join_style;
	    gc->dirty |= GCJoinStyle;
	  }

    if (mask & GCFillStyle)
        if (gv->fill_style != att->fill_style) {
            gv->fill_style = att->fill_style;
	    gc->dirty |= GCFillStyle;
	  }

    if (mask & GCFillRule)
        if (gv->fill_rule != att->fill_rule) {
    	    gv->fill_rule = att->fill_rule;
	    gc->dirty |= GCFillRule;
	  }

    if (mask & GCArcMode)
        if (gv->arc_mode != att->arc_mode) {
	    gv->arc_mode = att->arc_mode;
	    gc->dirty |= GCArcMode;
	  }

    /* always write through resource ID changes */
    if (mask & GCTile) {
	    gv->tile = att->tile;
	    gc->dirty |= GCTile;
	  }

    /* always write through resource ID changes */
    if (mask & GCStipple) {
	    gv->stipple = att->stipple;
	    gc->dirty |= GCStipple;
	  }

    if (mask & GCTileStipXOrigin)
        if (gv->ts_x_origin != att->ts_x_origin) {
    	    gv->ts_x_origin = att->ts_x_origin;
	    gc->dirty |= GCTileStipXOrigin;
	  }

    if (mask & GCTileStipYOrigin)
        if (gv->ts_y_origin != att->ts_y_origin) {
	    gv->ts_y_origin = att->ts_y_origin;
	    gc->dirty |= GCTileStipYOrigin;
	  }

    /* always write through resource ID changes */
    if (mask & GCFont) {
	    gv->font = att->font;
	    gc->dirty |= GCFont;
	  }

    if (mask & GCSubwindowMode)
        if (gv->subwindow_mode != att->subwindow_mode) {
	    gv->subwindow_mode = att->subwindow_mode;
	    gc->dirty |= GCSubwindowMode;
	  }

    if (mask & GCGraphicsExposures)
        if (gv->graphics_exposures != att->graphics_exposures) {
	    gv->graphics_exposures = att->graphics_exposures;
	    gc->dirty |= GCGraphicsExposures;
	  }

    if (mask & GCClipXOrigin)
        if (gv->clip_x_origin != att->clip_x_origin) {
	    gv->clip_x_origin = att->clip_x_origin;
	    gc->dirty |= GCClipXOrigin;
	  }

    if (mask & GCClipYOrigin)
        if (gv->clip_y_origin != att->clip_y_origin) {
	    gv->clip_y_origin = att->clip_y_origin;
	    gc->dirty |= GCClipYOrigin;
	  }

    if (mask & GCClipMask) 
        if ((gv->clip_mask != att->clip_mask) || (gc->rects == True)) {
           gv->clip_mask = att->clip_mask;
	   gc->dirty |= GCClipMask;
	   gc->rects = 0;
	   }

    if (mask & GCDashOffset)
        if (gv->dash_offset != att->dash_offset) {
	    gv->dash_offset = att->dash_offset;
	    gc->dirty |= GCDashOffset;
	  }

    if (mask & GCDashList)
        if ((gv->dashes != att->dashes) || (gc->dashes == True)) {
            gv->dashes = att->dashes;
	    gc->dirty |= GCDashList;
	    gc->dashes = 0;
	    }
    return;
    }

/* can only call when display is already locked. */

_XFlushGCCache(dpy, gc)
     Display *dpy;
     GC gc;
{
    register xChangeGCReq *req;
    register _XExtension *ext;

    if (gc->dirty) {
        GetReq(ChangeGC, req);
        req->gc = gc->gid;
	req->mask = gc->dirty;
        _XGenerateGCList (dpy, gc, (xReq *) req);
	ext = dpy->ext_procs;
	while (ext) {		/* call out to any extensions interested */
		if (ext->flush_GC != NULL) (*ext->flush_GC)(dpy, gc, &ext->codes);
		ext = ext->next;
	}    
    }
}

GContext XGContextFromGC(gc)
    GC gc;
    { return (gc->gid); }
