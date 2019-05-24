#include "copyright.h"

/* $Header: XGetStCmap.c,v 1.3 87/09/11 08:15:49 toddb Exp $ */

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

Status XGetStandardColormap (dpy, w, cmap, property)
	Display *dpy;
	Window w;
	XStandardColormap *cmap;
        Atom property;		/* XA_RGB_BEST_MAP, etc. */
{
	xPropStandardColormap *prop;
        Atom actual_type;
        int actual_format;
        long leftover;
        unsigned long nitems;

	if (XGetWindowProperty (dpy, w, property, 0L,
	    (long)NumPropStandardColormapElements, False,
	    XA_RGB_COLOR_MAP, &actual_type, &actual_format,
            &nitems, &leftover, (unsigned char **)&prop)
            != Success) return (0);

        if ((nitems < NumPropStandardColormapElements)
	 || (actual_format != 32)) {
		Xfree ((char *)prop);
                return(0);
		}
	cmap->colormap	 = prop->colormap;
	cmap->red_max	 = prop->red_max;
	cmap->red_mult	 = prop->red_mult;
	cmap->green_max	 = prop->green_max;
	cmap->green_mult = prop->green_mult;
	cmap->blue_max	 = prop->blue_max;
	cmap->blue_mult	 = prop->blue_mult;
	cmap->base_pixel = prop->base_pixel;
	Xfree((char *)prop);
	return(1);
}
