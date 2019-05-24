#include "copyright.h"

/* $Header: XSetHints.c,v 11.18 87/09/01 15:05:37 toddb Exp $ */

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

XSetSizeHints(dpy, w, hints, property)
	Display *dpy;
	Window w;
	XSizeHints *hints;
        Atom property;
{
        xPropSizeHints prop;
	prop.flags = hints->flags;
	prop.x = hints->x;
	prop.y = hints->y;
	prop.width = hints->width;
	prop.height = hints->height;
	prop.minWidth = hints->min_width;
	prop.minHeight = hints->min_height;
	prop.maxWidth  = hints->max_width;
	prop.maxHeight = hints->max_height;
	prop.widthInc = hints->width_inc;
	prop.heightInc = hints->height_inc;
	prop.minAspectX = hints->min_aspect.x;
	prop.minAspectY = hints->min_aspect.y;
	prop.maxAspectX = hints->max_aspect.x;
	prop.maxAspectY = hints->max_aspect.y;
	XChangeProperty (dpy, w, property, XA_WM_SIZE_HINTS, 32,
	     PropModeReplace, (unsigned char *) &prop, NumPropSizeElements);
}

/* 
 * XSetWMHints sets the property 
 *	WM_HINTS 	type: WM_HINTS	format:32
 */

XSetWMHints (dpy, w, wmhints)
	Display *dpy;
	Window w;
	XWMHints *wmhints; 
{
	xPropWMHints prop;
	prop.flags = wmhints->flags;
	prop.input = wmhints->input;
	prop.initialState = wmhints->initial_state;
	prop.iconPixmap = wmhints->icon_pixmap;
	prop.iconWindow = wmhints->icon_window;
	prop.iconX = wmhints->icon_x;
	prop.iconY = wmhints->icon_y;
	prop.iconMask = wmhints->icon_mask;
	prop.windowGroup = wmhints->window_group;
	XChangeProperty (dpy, w, XA_WM_HINTS, XA_WM_HINTS, 32,
	    PropModeReplace, (unsigned char *) &prop, NumPropWMHintsElements);
}



/* 
 * XSetZoomHints sets the property 
 *	WM_ZOOM_HINTS 	type: WM_SIZE_HINTS format: 32
 */

XSetZoomHints (dpy, w, zhints)
	Display *dpy;
	Window w;
	XSizeHints *zhints;
{
	XSetSizeHints (dpy, w, zhints, XA_WM_ZOOM_HINTS);
}


/* 
 * XSetNormalHints sets the property 
 *	WM_NORMAL_HINTS 	type: WM_SIZE_HINTS format: 32
 */

void XSetNormalHints (dpy, w, hints)
	Display *dpy;
	Window w;
	XSizeHints *hints;
{
	XSetSizeHints (dpy, w, hints, XA_WM_NORMAL_HINTS);
}


XSetIconSizes (dpy, w, list, count)
	Display *dpy;
	Window w;	/* typically, root */
	XIconSize *list;
	int count; 	/* number of items on the list */
{
	register int i;
	xPropIconSize *pp, *prop;
	unsigned nbytes = count * sizeof(xPropIconSize);
	prop = pp = (xPropIconSize *) Xmalloc (nbytes);
	for (i = 0; i < count; i++) {
	    pp->minWidth  = list->min_width;
	    pp->minHeight = list->min_height;
	    pp->maxWidth  = list->max_width;
	    pp->maxHeight = list->max_height;
	    pp->widthInc  = list->width_inc;
	    pp->heightInc = list->height_inc;
	    pp += 1;
	    list += 1;
	}
	XChangeProperty (dpy, w, XA_WM_ICON_SIZE, XA_WM_ICON_SIZE, 32, 
		 PropModeReplace, (unsigned char *) prop, 
			 count * NumPropIconSizeElements);
	Xfree ((char *)prop);
}

#include <strings.h>
XSetCommand (dpy, w, argv, argc)
	Display *dpy;
	Window w;
	char **argv;
	int argc;
{
	register int i;
	register unsigned nbytes;
	register char *buf, *bp;
	for (i = 0, nbytes = 0; i < argc; i++) {
		nbytes += strlen(argv[i]) + 1;
	}
	if (nbytes == 0) return;
	bp = buf = Xmalloc(nbytes);
	/* copy arguments into single buffer */
	for (i = 0; i < argc; i++) {
		(void) strcpy(bp, argv[i]);
		bp += strlen(argv[i]) + 1;
	}
	XChangeProperty (dpy, w, XA_WM_COMMAND, XA_STRING, 8, PropModeReplace,
		(unsigned char *)buf, nbytes);
	Xfree(buf);		
}
/* 
 * XSetStandardProperties sets the following properties:
 *	WM_NAME		  type: STRING		format: 8
 *	WM_ICON_NAME	  type: STRING		format: 8
 *	WM_HINTS	  type: WM_HINTS	format: 32
 *	WM_COMMAND	  type: STRING
 *	WM_NORMAL_HINTS	  type: WM_SIZE_HINTS 	format: 32
 */
	
XSetStandardProperties (dpy, w, name, icon_string, icon_pixmap, argv, argc, hints)
    	Display *dpy;
    	Window w;		/* window to decorate */
    	char *name;		/* name of application */
    	char *icon_string;	/* name string for icon */
	Pixmap icon_pixmap;	/* pixmap to use as icon, or None */
    	char *argv[];		/* command to be used to restart application */
    	int argc;		/* count of arguments */
    	XSizeHints *hints;	/* size hints for window in its normal state */
{
	XWMHints phints;
	phints.flags = 0;

	if (name != NULL) XStoreName (dpy, w, name);

	if (icon_string != NULL) {
	    XChangeProperty (dpy, w, XA_WM_ICON_NAME, XA_STRING, 8,
		PropModeReplace, (unsigned char *)icon_string, strlen(icon_string));
		}

	if (icon_pixmap != None) {
		phints.icon_pixmap = icon_pixmap;
		phints.flags |= IconPixmapHint;
		}
	if (argv != NULL) XSetCommand(dpy, w, argv, argc);
	
	if (hints != NULL) XSetNormalHints(dpy, w, hints);

	if (phints.flags != 0) XSetWMHints(dpy, w, &phints);
}

void
XSetTransientForHint(dpy, w, propWindow)
	Display *dpy;
	Window w;
	Window propWindow;
{
	XChangeProperty(dpy, w, XA_WM_TRANSIENT_FOR, XA_WINDOW, 32,
		PropModeReplace, (char *) &propWindow, 1);
}

void
XSetClassHint(dpy, w, classhint)
	Display *dpy;
	Window w;
	XClassHint *classhint;
{
	char *class_string = NULL;
	int len_nm, len_cl;

	len_nm = strlen(classhint->res_name);
	len_cl = strlen(classhint->res_class);
	class_string = Xmalloc(len_nm + len_cl + 2);
	strcpy(class_string, classhint->res_name);
	strcpy(class_string+strlen(classhint->res_name)+1, classhint->res_class);
	XChangeProperty(dpy, w, XA_WM_CLASS, XA_STRING, 8,
		PropModeReplace, class_string, len_nm+len_cl+2);
	Xfree(class_string);
}
