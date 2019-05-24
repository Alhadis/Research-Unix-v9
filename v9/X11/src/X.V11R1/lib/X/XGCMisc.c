#include "copyright.h"

/* $Header: XGCMisc.c,v 11.2 87/09/11 08:16:24 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XSetArcMode (dpy, gc, arc_mode)
register Display *dpy;
register GC gc;
int arc_mode;
{
    LockDisplay(dpy);
    if (gc->values.arc_mode != arc_mode) {
	gc->values.arc_mode = arc_mode;
	gc->dirty |= GCArcMode;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}

XSetFillRule (dpy, gc, fill_rule)
register Display *dpy;
register GC gc;
int fill_rule;
{
    LockDisplay(dpy);
    if (gc->values.fill_rule != fill_rule) {
	gc->values.fill_rule = fill_rule;
	gc->dirty |= GCFillRule;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}

XSetFillStyle (dpy, gc, fill_style)
register Display *dpy;
register GC gc;
int fill_style;
{
    LockDisplay(dpy);
    if (gc->values.fill_style != fill_style) {
	gc->values.fill_style = fill_style;
	gc->dirty |= GCFillStyle;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}

XSetGraphicsExposures (dpy, gc, graphics_exposures)
register Display *dpy;
register GC gc;
Bool graphics_exposures;
{
    LockDisplay(dpy);
    if (gc->values.graphics_exposures != graphics_exposures) {
	gc->values.graphics_exposures = graphics_exposures;
	gc->dirty |= GCGraphicsExposures;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}

XSetSubwindowMode (dpy, gc, subwindow_mode)
register Display *dpy;
register GC gc;
int subwindow_mode;
{
    LockDisplay(dpy);
    if (gc->values.subwindow_mode != subwindow_mode) {
	gc->values.subwindow_mode = subwindow_mode;
	gc->dirty |= GCSubwindowMode;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
