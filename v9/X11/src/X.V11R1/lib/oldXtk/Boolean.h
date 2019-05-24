/* $Header: Boolean.h,v 1.1 87/09/11 07:58:52 toddb Exp $ */
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


#ifndef _XtBoolean_h
#define _XtBoolean_h

/***********************************************************************
 *
 * Boolean Button Widget
 *
 ***********************************************************************/

#ifndef _XtJustify_e
#define _XtJustify_e
typedef enum {
    XtjustifyLeft,       /* justify text to left side of button   */
    XtjustifyCenter,     /* justify text in center of button      */
    XtjustifyRight       /* justify text to right side of button  */
} XtJustify;
#endif _XtJustify_e

#define XtNvalue		"value"
#define XtNwindow		"window"
#define XtNborderWidth		"borderWidth"
#define XtNwidth		"width"
#define XtNheight		"height"
#define XtNinternalHeight	"internalHeight"
#define XtNinternalWidth	"internalWidth"
#define XtNlabel		"label"
#define XtNforeground		"foreground"
#define XtNbackground		"background"
#define XtNborder		"border"
#define XtNfont			"font"
#define XtNjustify		"justify"
#define XtNfunction		"function"
#define XtNparameter		"parameter"

/*

 Parameters
 ==========

 Name		Class		RepType		Default Value
 ----		-----		-------		-------------
 value		Value		Pointer		NONE, required parameter
 window		Window		Window		<create window>
 borderWidth	BorderWidth	int		1
 width		Width		int		text width + 2*internalWidth
 height		Height		int		font height + 2*internalHeight
 internalWidth	Width		int		4
 internalHeight	Height		int		2
 label		String		String		""
 foreground	Color		Pixel		BlackPixel
 background	Color		Pixel		WhitePixel
 border		Color		Pixel		BlackPixel
 font		Font		FontStruct	fixed
 justify	Justify		Justify		center

 eventBindings  EventBindings   EventBindings   <see below> 

 function       Function	Function	Dummy
 parameter      Parameter       Pointer		NULL


Event bindings
==============

 Function	Default Binding(s)
 ----		------------------
 highlight      EnterWindow
 unhighlight    LeaveWindow
 toggle		ButtonPress left
 notify		ButtonRelease left
 
*/

extern Window XtBooleanCreate(); /* parent, args, argCount */
    /* Window   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

extern void XtBooleanSetValues(); /* dpy, window, args, argCount */
    /* Display  *dpy;       */
    /* Window   window;     */
    /* ArgList  args;       */
    /* int      argCount;   */

extern void XtBooleanGetValues(); /* dpy, window, args, argCount */
    /* Display  *dpy;       */
    /* Window   window;     */
    /* ArgList  args;       */
    /* int      argCount;   */

#endif _XtBoolean_h
/* DON'T ADD STUFF AFTER THIS #endif */
