/* $Header: Menu.h,v 1.1 87/09/11 07:59:32 toddb Exp $ */
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

#ifndef _XtMenu_h
#define _XtMenu_h

/****************************************************************
 *
 * Basic Menu Widget
 *
 ****************************************************************/


/*
 Parameters
 ==========

 Name		Class		RepType		Default Value
 ----		-----		-------		-------------
 Window		window		Window		<Created>
 Width		int		int		1
 Height		int		int		1
 Orientation	orientation	XrmAtom		vertical
 BorderWidth	int		int		1
 Border		color		Pixmap		BlackPixel
 Background	color		Pixmap		WhitePixel
 InternalWidth	int		int		2
 InternalHeight	int		int		2
 Space		space		int		1
 Function	function	function	<built-in debug function>
 Parameter	pointer		caddr_t		0
 Notify		int		int		0

 Parameters for XtAddMenuEntry():
 ================================

 Name		Class		RepType		Default Value
 ----		-----		-------		-------------
 MenuEntry	menuentry	XtMenuEntry	<none>



The callback function should look like:

void function(tag)
caddr_t tag;
{
}

*/

#ifndef _XtOrientation_e
#define _XtOrientation_e
typedef enum {XtorientHorizontal, XtorientVertical} XtOrientation;
#endif _XtOrientation_e

#define XtNwindow		"window"
#define XtNx			"x"
#define XtNy			"y"
#define XtNwidth		"width"
#define XtNheight		"height"
#define XtNorientation		"orientation"
#define XtNborderWidth		"borderWidth"
#define XtNborder		"border"
#define XtNbackground		"background"
#define XtNinternalWidth	"internalWidth"
#define XtNinternalHeight	"internalHeight"
#define XtNspace		"space"
#define XtNnotify		"notify"
#define XtNparameter		"parameter"
#define XtNmenuEntry		"menuEntry"


typedef struct {
    Window index;	/* insert w in front of this window; NULL=append*/
    Window w;		/* window id of entry to be added */
} XtMenuEntry, *XtMenuEntryPtr;

extern Window XtMenuCreate(); /* dpy, parent, args, argCount */
    /* Display	*dpy; */
    /* Window   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

extern XtStatus XtMenuAddEntry(); /* dpy, parent, args, argCount */
    /* Display	*dpy; */
    /* Window   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

extern XtStatus XtMenuDeleteEntry(); /* dpy, parent, args, argCount */
    /* Display	*dpy; */
    /* Window   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

/*
 * Note: The following default menu geometry manager is provided as an
 * aid to prototyping pop-up menus.  It says "XtgeometryYes" to everything.
 * Clients should replace this with their own geometry manager as
 * necessary.
 */
extern XtGeometryReturnCode XtMenuGeometryManager();
    /* dpy,w,req,reqBox,repBox */
    /* Display *dpy; */
    /* Window w; */	/* window requesting geometry change */
    /* XtGeometryRequest req; */	/* ignored, but must be present */
    /* WindowBox *reqBox, *RepBox; */ /* size boxes for request */

extern void XtMenuGetValues (); /* dpy, window, args, argCount */
    /* Display *dpy; */
    /* Window window; */
    /* ArgList args; */
    /* int argCount; */

extern void XtMenuSetValues (); /* dpy, window, args, argCount */
    /* Display *dpy; */
    /* Window window; */
    /* ArgList args; */
    /* int argCount; */

#endif _XtMenu_h
/* DON'T ADD STUFF AFTER THIS #endif */
