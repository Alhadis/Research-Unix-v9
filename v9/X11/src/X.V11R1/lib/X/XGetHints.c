#include "copyright.h"

/* $Header: XGetHints.c,v 11.16 87/09/09 15:42:48 swick Exp $ */

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

#include <stdio.h>
#include "Xlibint.h"
#include "Xutil.h"
#include "Xatomtype.h"
#include "Xatom.h"

Status XGetSizeHints (dpy, w, hints, property)
	Display *dpy;
	Window w;
	XSizeHints *hints;
        Atom property;
{
	xPropSizeHints *prop = NULL;
        Atom actual_type;
        int actual_format;
        long leftover;
        unsigned long nitems;
	if (XGetWindowProperty(dpy, w, property, 0L, (long)NumPropSizeElements,
	    False, XA_WM_SIZE_HINTS, &actual_type, &actual_format,
            &nitems, &leftover, (unsigned char **)&prop)
            != Success) return (0);

        if ((actual_type != XA_WM_SIZE_HINTS) ||
	    (nitems < NumPropSizeElements) || (actual_format != 32)) {
		if (prop != NULL) Xfree ((char *)prop);
                return(0);
		}
	hints->flags	  = prop->flags;
	hints->x 	  = prop->x;
	hints->y 	  = prop->y;
	hints->width      = prop->width;
	hints->height     = prop->height;
	hints->min_width  = prop->minWidth;
	hints->min_height = prop->minHeight;
	hints->max_width  = prop->maxWidth;
	hints->max_height = prop->maxHeight;
	hints->width_inc  = prop->widthInc;
	hints->height_inc = prop->heightInc;
	hints->min_aspect.x = prop->minAspectX;
	hints->min_aspect.y = prop->minAspectY;
	hints->max_aspect.x = prop->maxAspectX;
	hints->max_aspect.y = prop->maxAspectY;
	Xfree((char *)prop);
	return(1);
}

/* 
 * must return a pointer to the hint, in malloc'd memory, or routine is not
 * extensible; any use of the caller's memory would cause things to be stepped
 * on.
 */

XWMHints *XGetWMHints (dpy, w)
	Display *dpy;
	Window w;
{
	xPropWMHints *prop = NULL;
	register XWMHints *hints;
        Atom actual_type;
        int actual_format;
        long leftover;
        unsigned long nitems;
	if (XGetWindowProperty(dpy, w, XA_WM_HINTS, 
	    0L, (long)NumPropWMHintsElements,
	    False, XA_WM_HINTS, &actual_type, &actual_format,
            &nitems, &leftover, (unsigned char **)&prop)
            != Success) return (NULL);

	/* If the property is undefined on the window, return null pointer. */

        if ((actual_type != XA_WM_HINTS) ||
	    (nitems < NumPropWMHintsElements) || (actual_format != 32)) {
		if (prop != NULL) Xfree ((char *)prop);
                return(NULL);
		}
	/* static copies not allowed in library, due to reentrancy constraint*/
	hints = (XWMHints *) Xcalloc (1, (unsigned) sizeof(XWMHints));
	hints->flags = prop->flags;
	hints->input = prop->input;
	hints->initial_state = prop->initialState;
	hints->icon_pixmap = prop->iconPixmap;
	hints->icon_window = prop->iconWindow;
	hints->icon_x = prop->iconX;
	hints->icon_y = prop->iconY;
	hints->icon_mask = prop->iconMask;
	hints->window_group = prop->windowGroup;
	Xfree ((char *)prop);
	return(hints);
}

Status
XGetZoomHints (dpy, w, zhints)
	Display *dpy;
	Window w;
	XSizeHints *zhints;
{
	return (XGetSizeHints(dpy, w, zhints, XA_WM_ZOOM_HINTS));
}

Status
XGetNormalHints (dpy, w, hints)
	Display *dpy;
	Window w;
	XSizeHints *hints;
{
	return (XGetSizeHints(dpy, w, hints, XA_WM_NORMAL_HINTS));
}

				
/*
 * XGetIconSizes reads the property 
 *	ICONSIZE_ATOM	type: ICONSIZE_ATOM format: 32
 */

Status XGetIconSizes (dpy, w, size_list, count)
	Display *dpy;
	Window w;	/* typically, root */
	XIconSize **size_list;
	int *count; /* number of items on the list */
{
	xPropIconSize *prop = NULL;
	register xPropIconSize *pp;
	register XIconSize *hp, *hints;
        Atom actual_type;
        int actual_format;
        long leftover;
        unsigned long nitems;
	register int i;

	if (XGetWindowProperty(dpy, w, XA_WM_ICON_SIZE, 0L, 60L,
	    False, XA_WM_ICON_SIZE, &actual_type, &actual_format,
            &nitems, &leftover, (unsigned char **)&prop)
            != Success) return (0);

	pp = prop;

        if ((actual_type != XA_WM_ICON_SIZE) ||
	    (nitems < NumPropIconSizeElements) || (actual_format != 32)) {
		if (prop != NULL) Xfree ((char *)prop);
                return(0);
		}
	/* static copies not allowed in library, due to reentrancy constraint*/
	*count = nitems / NumPropIconSizeElements;
	if (*count < 1) {
	  if (prop != NULL) Xfree((char *)prop);
	  return(0);
	}

	hp = hints =  (XIconSize *) 
	  Xcalloc ((unsigned)*count, (unsigned) sizeof(XIconSize));

	/* march down array putting things into native form */
	for (i = 0; i < *count; i++) {
	    hp->min_width  = pp->minWidth;
	    hp->min_height = pp->minHeight;
	    hp->max_width  = pp->maxWidth;
	    hp->max_height = pp->maxHeight;
	    hp->width_inc  = pp->widthInc;
	    hp->height_inc = pp->heightInc;
	    hp += 1;
	    pp += 1;
	  }
	*size_list = hints;
	if (prop != NULL) Xfree ((char *)prop);
	return(1);
	
}

Status
XGetTransientForHint(dpy, w, propWindow)
	Display *dpy;
	Window w;
	Window *propWindow;
{
    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    long leftover;
    Window *data = NULL;
    if (XGetWindowProperty(dpy, w, XA_WM_TRANSIENT_FOR, 0L, 1L, False,
        XA_WINDOW, 
	&actual_type,
	&actual_format, &nitems, &leftover, (char **) &data) != Success) {
	*propWindow = None;
	return (0);
	}
    if ( (actual_type == XA_WINDOW) && (actual_format == 32) ) {
	*propWindow = *data;
	Xfree( (char *) data);
	return (1);
	}
    *propWindow = None;
    Xfree( (char *) data);
    return(0);
}

Status
XGetClassHint(dpy, w, classhint)
	Display *dpy;
	Window w;
	XClassHint *classhint;
{
    int len_name, len_class;
    Atom actual_type;
    int actual_format;
    unsigned long nitems;
    long leftover;
    unsigned char *data = NULL;
    if (XGetWindowProperty(dpy, w, XA_WM_CLASS, 0L, (long)BUFSIZ, False,
        XA_STRING, 
	&actual_type,
	&actual_format, &nitems, &leftover, &data) != Success) {
	classhint = NULL;
	return (0);
	}
	
   if ( (actual_type == XA_STRING) && (actual_format == 8) ) {
	len_name = strlen(data);
	len_class = strlen(data+len_name+1);
	classhint->res_name = Xmalloc(len_name+1);
	strcpy(classhint->res_name, data);
	classhint->res_class = Xmalloc(len_class+1);
	strcpy(classhint->res_class, data+len_name+1);
	Xfree( (char *) data);
	return(Success);
	}
    classhint = NULL;
    Xfree( (char *) data);
    return(0);
}
