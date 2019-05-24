/* $Header: Form.h,v 1.1 87/09/11 07:59:26 toddb Exp $ */
/*
 *	sccsid:	@(#)Form.h	1.4	5/18/87
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
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

#ifndef _Form_h
#define _Form_h

/***********************************************************************
 *
 * Form Widget
 *
 ***********************************************************************/

#define XtNname			"name"
#define XtNx			"x"
#define XtNy			"y"
#define XtNwindow		"window"
#define XtNborderWidth		"borderWidth"
#define XtNwidth		"width"
#define XtNheight		"height"
#define XtNbackground		"background"
#define XtNborder		"border"
#define XtNdefaultDistance	"defaultDistance"
#define XtNtop			"top"
#define XtNbottom		"bottom"
#define XtNleft			"left"
#define XtNright		"right"
#define XtNfromHoriz		"fromHoriz"
#define XtNfromVert		"fromVert"
#define XtNhorizDistance	"horizDistance"
#define XtNvertDistance		"vertDistance"
#define XtNresizable		"resizable"

#define XtCEdge			"Edge"

#ifndef _XtEdgeType_e
#define _XtEdgeType_e
typedef enum {
    XtChainTop,			/* Keep this edge a constant distance from
				   the top of the form */
    XtChainBottom,		/* Keep this edge a constant distance from
				   the bottom of the form */
    XtChainLeft,		/* Keep this edge a constant distance from
				   the left of the form */
    XtChainRight,		/* Keep this edge a constant distance from
				   the right of the form */
    XtRubber,			/* Keep this edge a proportional distance
				   from the edges of the form*/
} XtEdgeType;
#endif  _XtEdgeType_e


#ifndef _XtJustify_e
#define _XtJustify_e
typedef enum {
    XtjustifyLeft,       /* justify text to left side of button   */
    XtjustifyCenter,     /* justify text in center of button      */
    XtjustifyRight       /* justify text to right side of button  */
} XtJustify;
#endif _XtJustify_e

 
extern Window XtFormCreate(); /* parent, args, argCount */
    /* Window   parent;     */
    /* ArgList  args;       */
    /* int      argCount;   */

extern void XtFormGetValues(); /* dpy, w, args, argCount */
    /* Display *dpy; */
    /* Window w; */
    /* ArgList  args;       */
    /* int      argCount;   */

extern void XtFormSetValues(); /* dpy, w, args, argCount */
    /* Display *dpy; */
    /* Window w; */
    /* ArgList  args;       */
    /* int      argCount;   */

extern void XtFormAddWidget(); /* mywin, w, args, argCount */
    /* Window mywin, w; */
    /* ArgList  args;       */
    /* int      argCount;   */

extern void XtFormRemoveWidget(); /* mywin, w */
    /* Window mywin, w; */

extern void XtFormDoLayout(); /* dpy, mywin, doit */
    /* Display *dpy; */
    /* Window mywin; */
    /* Boolean doit; */

#endif _Form_h
/* DON'T ADD STUFF AFTER THIS #endif */
