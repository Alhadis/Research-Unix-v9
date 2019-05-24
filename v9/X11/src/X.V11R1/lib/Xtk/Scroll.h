/*
* $Header: Scroll.h,v 1.2 87/09/11 21:24:26 haynes Rel $
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
#ifndef _XtScroll_h
#define _XtScroll_h

/****************************************************************
 *
 * Scrollbar Widget
 *
 ****************************************************************/


/* Parameters:

 Name		Class		RepType		Default Value
 ----		-----		-------		-------------
 Orientation	orientation	XtOrientation	XtorientHorizontal
 Width		width		int		1
 Height		height		int		1
 BorderWidth	borderwidth	int		1
 Border		color		pixel		Default foreground
 ScrollProc	function	function	No-op
 ThumbProc	function	function	No-op
 Parameter	parameter	parameter	0
 Foreground	color		pixel		Default foreground
 Background	color		pixel		Default background
 Thumb		pixmap		pixmap		Grey
 Top		fraction	float		0.0
 Shown		fraction	float		0.0

*/

#ifndef _XtOrientation_e
#define _XtOrientation_e

typedef enum {XtorientHorizontal, XtorientVertical} XtOrientation;
#endif _XtOrientation_e

typedef struct _ScrollbarRec	  *ScrollbarWidget;
typedef struct _ScrollbarClassRec *ScrollbarWidgetClass;

extern WidgetClass scrollbarWidgetClass;

extern void XtScrollBarSetThumb(); /* scrollBar, top, shown */
/* ScrollbarWidget scrollBar; */
/* float top, shown; */

extern Window XtScrollMgrCreate(); /* dpy, parent, args, argCount */
/* Display *dpy; */
    /* Window   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

extern Window XtScrollMgrGetChild(); /* dpy, parent */
/* Display *dpy; */
    /* Window   parent;     */

extern Window XtScrollMgrSetChild(); /* dpy, parent, frame */
/* Display *dpy; */
/* Window parent, frame; */

extern XtScrollMgrSetThickness(); /* dpy, w, thickness */
/* Display *dpy; */
    /* Window w; */
    /* Int thickness; */

extern int XtScrollMgrGetThickness(); /* dpy, w */
/* Display *dpy; */
    /* Window w; */

extern Window XtScrollMgrAddBar(); /* dpy, parent, args, argCount */
/* Display *dpy; */
    /* Window   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

extern int XtScrollMgrDeleteBar(); /* dpy, parent, scrollbar */
/* Display *dpy; */
    /* Window parent, scrollbar; */

extern Window XtScrolledWindowCreate(); /* dpy, parent, args, argCount */
/* Display *dpy; */
    /* Window   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

extern void XtScrolledWindowSetChild(); /* dpy, parent, child */
/* Display *dpy; */
    /* Window parent, child; */

extern Window XtScrolledWindowGetFrame(); /* dpy, parent */
/* Display *dpy; */
    /* Window parent; */

extern Window XtUnmakeScrolledWindow(); /* dpy, parent */
/* Display *dpy; */
    /*  Window parent; */

#endif _XtScroll_h
/* DON'T ADD STUFF AFTER THIS #endif */
