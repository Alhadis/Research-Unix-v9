#include "copyright.h"

/* $Header: XGeom.c,v 1.2 87/09/11 08:09:25 toddb Exp $/
/* Copyright Massachusetts Institute of Technology 1985 */

#include "Xlibint.h"
#include "Xutil.h"

/*
 * This routine given a user supplied positional argument and a default
 * argument (fully qualified) will return the position the window should take
 * returns 0 if there was some problem, else the position bitmask.
 */

int XGeometry (dpy, screen, pos, def, bwidth, fwidth, fheight, xadd, yadd, x, y, width, height)
     Display *dpy;			/* user's display connection */
     int screen;			/* screen on which to do computation */
     char *pos;				/* user provided geometry spec */
     char *def;				/* default geometry spec for window */
     unsigned int bwidth;		/* border width */
     unsigned int fwidth, fheight;	/* size of position units */
     int xadd, yadd;			/* any additional interior space */
     register *x, *y, *width, *height;	/* always set on successful RETURN */
{
	int px, py, pwidth, pheight;	/* returned values from parse */
	int dx, dy, dwidth, dheight;	/* default values from parse */
	int pmask, dmask;		/* values back from parse */

	pmask = XParseGeometry(pos, &px, &py, &pwidth, &pheight);
	dmask = XParseGeometry(def, &dx, &dy, &dwidth, &dheight);

	/* set default values */
	*x = (dmask & XNegative) ? 
	    DisplayWidth(dpy, screen)  + dx - dwidth * fwidth - 
	        2 * bwidth - xadd : dx;
	*y = (dmask & YNegative) ? 
	    DisplayHeight(dpy, screen) + dy - dheight * fheight - 
	        2 * bwidth - yadd : dy;
	*width  = dwidth;
	*height = dheight;

	if (pmask & WidthValue)  *width  = pwidth;
	if (pmask & HeightValue) *height = pheight;

	if (pmask & XValue)
	    *x = (pmask & XNegative) ?
	      DisplayWidth(dpy, screen) + px - *width * fwidth - 
		  2 * bwidth - xadd : px;
	if (pmask & YValue)
	    *y = (pmask & YNegative) ?
	      DisplayHeight(dpy, screen) + py - *height * fheight - 
		  2 * bwidth - yadd : py;
	return (pmask);
}
