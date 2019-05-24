#include "copyright.h"

/* $Header: XSetLStyle.c,v 11.9 87/09/11 08:07:06 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1986	*/

#include "Xlibint.h"

XSetLineAttributes(dpy, gc, linewidth, linestyle, capstyle, joinstyle)
register Display *dpy;
GC gc;
unsigned int linewidth; /* CARD16 */
int linestyle;
int capstyle;
int joinstyle;
{
    XGCValues *gv = &gc->values;

    LockDisplay(dpy);
    if (linewidth != gv->line_width) {
	gv->line_width = linewidth;
	gc->dirty |= GCLineWidth;
    }
    if (linestyle != gv->line_style) {
	gv->line_style = linestyle;
	gc->dirty |= GCLineStyle;
    }
    if (capstyle != gv->cap_style) {
	gv->cap_style = capstyle;
	gc->dirty |= GCCapStyle;
    }
    if (joinstyle != gv->join_style) {
	gv->join_style = joinstyle;
	gc->dirty |= GCJoinStyle;
    }
    UnlockDisplay(dpy);
    SyncHandle();
}
