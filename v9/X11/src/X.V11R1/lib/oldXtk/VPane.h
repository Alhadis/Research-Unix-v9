/* $Header: VPane.h,v 1.1 87/09/11 07:59:29 toddb Exp $ */
/*
 *	sccsid:	%W%	%G%
 */

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

#ifndef _XtVPane_h
#define _XtVPane_h

/****************************************************************
 *
 * Vertical Pane Widget
 *
 ****************************************************************/


#define XtNwindow		"window"
#define XtNknobWidth		"knobWidth"
#define XtNknobHeight		"knobHeight"
#define XtNknobIndent		"knobIndent"
#define XtNknobPixel		"knobPixel"
#define XtNborder		"border"
#define XtNborderWidth		"borderWidth"
#define XtNforeground		"foreground"
#define XtNbackground		"background"
#define XtNx			"x"
#define XtNy			"y"
#define XtNwidth		"width"
#define XtNheight		"height"

extern Window XtVPanedWindowCreate(); /* dpy, parent, args, argCount */
    /* Display	*dpy;	*/
    /* Window   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

extern void XtVPanedWindowDelete(); /* dpy, w */
    /* Display	*dpy;	*/
    /* Window w; */

extern void XtVPanedWindowAddPane();
		/* dpy, w, paneWindow, position, min, max, autochange */
    /* Display	*dpy;	*/
    /* Window w, paneWindow; */
    /* int position, min, max, autochange; */

extern void XtVPanedSetMinMax(); /* dpy, w, paneWindow, min, max */
    /* Display	*dpy;	*/
    /* Window w, paneWindow; */
    /* int    min, max; */

extern void XtVPanedGetMinMax(); /* dpy, w, paneWindow, min, max */
    /* Display	*dpy;	*/
    /* Window w, paneWindow; */
    /* int    *min, *max; */

extern void XtVPanedWindowDeletePane(); /* dpy, w, paneWindow */
    /* Display	*dpy;	*/
    /* Window w, paneWindow; */

extern void XtVPanedAllowResize();  /* dpy, window, paneWindow,allowresize */
    /*    Display *dpy; */
    /*     Window window, paneWindow; */
    /*    Boolean  allowresize; */

extern Boolean XtVPanedGetResize();  /* dpy, window, paneWindow */
    /*    Display *dpy; */
    /*     Window window, paneWindow; */

extern int XtVPanedGetNumSub(); /* dpy, window */
    /*    Display *dpy; */
    /*    Window window; */

extern void XtVPanedRefigureMode(); /* dpy, window, mode */
    /* Display	*dpy;	*/
    /* Window window; */
    /* short mode; */

extern void XtVPaneSetValues(); /* dpy, window, args, argCount */
    /* Display  *dpy;       */
    /* Window   window;     */
    /* ArgList  args;       */
    /* int      argCount;   */

extern void XtVPaneGetValues(); /* dpy, window, args, argCount */
    /* Display  *dpy;       */
    /* Window   window;     */
    /* ArgList  args;       */
    /* int      argCount;   */

#endif _XtVPane_h
/* DON'T ADD STUFF AFTER THIS #endif */
