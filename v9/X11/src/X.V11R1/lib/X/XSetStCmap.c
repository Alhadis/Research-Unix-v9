#include "copyright.h"

/* $Header: XSetStCmap.c,v 1.2 87/09/11 08:15:53 toddb Exp $ */

/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

#include "Xlibint.h"
#include "Xutil.h"
#include "Xatomtype.h"
#include "Xatom.h"

void XSetStandardColormap(dpy, w, cmap, property)
	Display *dpy;
	Window w;
	XStandardColormap *cmap;
        Atom property;		/* XA_RGB_BEST_MAP, etc. */
{
        xPropStandardColormap prop;

	prop.colormap	 = cmap->colormap;
	prop.red_max	 = cmap->red_max;
	prop.red_mult	 = cmap->red_mult;
	prop.green_max	 = cmap->green_max;
	prop.green_mult  = cmap->green_mult;
	prop.blue_max	 = cmap->blue_max;
	prop.blue_mult	 = cmap->blue_mult;
	prop.base_pixel  = cmap->base_pixel;
	XChangeProperty (dpy, w, property, XA_RGB_COLOR_MAP, 32,
	     PropModeReplace, (unsigned char *) &prop,
	     NumPropStandardColormapElements);
}
