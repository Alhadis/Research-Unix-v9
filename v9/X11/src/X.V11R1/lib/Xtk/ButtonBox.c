#ifndef lint
static char rcsid[] = "$Header: ButtonBox.c,v 1.13 87/09/13 20:35:58 newman Exp $";
#endif lint

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
/* 
 * ButtonBox.c - Button box composite widget
 * 
 * Author:	Joel McCormack
 * 		Digital Equipment Corporation
 * 		Western Research Laboratory
 * Date:	Mon Aug 31 1987
 *
 */

#include	"Intrinsic.h"
#include	"ButtonBox.h"
#include	"ButtonBoxP.h"
#include	"Atoms.h"
#include	"Misc.h"

/****************************************************************
 *
 * ButtonBox Resources
 *
 ****************************************************************/

static XtResource resources[] = {
    {XtNhSpace, XtCHSpace, XrmRInt, sizeof(int),
	 XtOffset(ButtonBoxWidget, button_box.h_space), XtRString, "4"},
    {XtNvSpace, XtCVSpace, XrmRInt, sizeof(int),
	 XtOffset(ButtonBoxWidget, button_box.v_space), XtRString, "4"},
};

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

static void Initialize();
static void Realize();
static void Resize();
static Boolean SetValues();
static XtGeometryResult GeometryManager();
static void ChangeManaged();
static void ClassInitialize();

ButtonBoxClassRec buttonBoxClassRec = {
  {
/* core_class fields      */
    /* superclass         */    (WidgetClass) &compositeClassRec,
    /* class_name         */    "ButtonBox",
    /* widget_size        */    sizeof(ButtonBoxRec),
    /* class_initialize   */    ClassInitialize,
    /* class_inited       */	FALSE,
    /* initialize         */    Initialize,
    /* realize            */    Realize,
    /* actions            */    NULL,
    /* num_actions	  */	0,
    /* resources          */    resources,
    /* num_resources      */    XtNumber(resources),
    /* xrm_class          */    NULLQUARK,
    /* compress_motion	  */	TRUE,
    /* compress_exposure  */	TRUE,
    /* visible_interest   */    FALSE,
    /* destroy            */    NULL,
    /* resize             */    Resize,
    /* expose             */    NULL,
    /* set_values         */    SetValues,
    /* accept_focus       */    NULL
  },{
/* composite_class fields */
    /* geometry_manager   */    GeometryManager,
    /* change_managed     */    ChangeManaged,
    /* insert_child	  */	NULL,	/* Inherit from superclass */
    /* delete_child	  */	NULL,	/* Inherit from superclass */
    /* move_focus_to_next */    NULL,
    /* move_focus_to_prev */    NULL
  },{
    /* mumble		  */	0	/* Make C compiler happy   */
  }
};

WidgetClass buttonBoxWidgetClass = (WidgetClass)&buttonBoxClassRec;


/****************************************************************
 *
 * Private Routines
 *
 ****************************************************************/

static void ClassInitialize()
{
    CompositeWidgetClass superclass;
    ButtonBoxWidgetClass myclass;

    myclass = (ButtonBoxWidgetClass) buttonBoxWidgetClass;
    superclass = (CompositeWidgetClass) myclass->core_class.superclass;

    /* Inherit insert_child and delete_child from Composite */
    myclass->composite_class.insert_child =
        superclass->composite_class.insert_child;
    myclass->composite_class.delete_child =
        superclass->composite_class.delete_child;
}

/*
 *
 * Do a layout, either actually assigning positions, or just calculating size.
 * Returns TRUE on success; FALSE if it couldn't make things fit.
 *
 */

/* ARGSUSED */
static DoLayout(bbw, width, height, replyWidth, replyHeight, position)
    ButtonBoxWidget	bbw;
    Dimension		width, height;
    Dimension		*replyWidth, *replyHeight;	/* RETURN */
    Boolean		position;	/* actually reposition the windows? */
{
    Cardinal  i;
    Dimension w, h;	/* Width and height needed for button box 	*/
    Dimension lw, lh;	/* Width and height needed for current line 	*/
    Dimension bw, bh;	/* Width and height needed for current button 	*/
    Dimension h_space;  /* Local copy of bbw->buttonBox.h_space 	*/
    Widget    widget;	/* Current button 				*/

    /* ButtonBox width and height */
    h_space = bbw->button_box.h_space;
    w = h_space;
    h = bbw->button_box.v_space;
   
    /* Line width and height */
    lh = 0;
    lw = h_space;
  
    for (i = 0; i < bbw->composite.num_children; i++) {
	widget = bbw->composite.children[i];
	if (widget->core.managed) {
	    /* Compute button width */
	    bw = widget->core.width + 2*widget->core.border_width + h_space;
	    if ((lw + bw > width) && (lw > h_space)) {
		/* At least one button on this line, and can't fit any more.
		   Start new line */
		AssignMax(w, lw);
		h += lh + bbw->button_box.v_space;
		lh = 0;
		lw = h_space;
	    }
	    if (position && (lw != widget->core.x || h != widget->core.y)) {
		XtMoveWidget(bbw->composite.children[i], (int)lw, (int)h);
	    }
	    lw += bw;
	    bh = widget->core.height + 2*widget->core.border_width;
	    AssignMax(lh, bh);
	} /* if managed */
    } /* for */

    /* Finish off last line */
    if (lw > h_space) {
	AssignMax(w, lw);
        h += lh + bbw->button_box.v_space;
    }

    *replyWidth = w;
    *replyHeight = h;
}

/*
 *
 * Calculate preferred size, given constraining box
 *
 */

static Boolean PreferredSize(bbw, width, height, replyWidth, replyHeight)
    ButtonBoxWidget	bbw;
    Dimension		width, height;
    Dimension		*replyWidth, *replyHeight;
{
    DoLayout(bbw, width, height, replyWidth, replyHeight, FALSE);
    return ((*replyWidth <= width) && (*replyHeight <= height));
}

/*
 *
 * Actually layout the button box
 *
 */

static void Resize(bbw)
    ButtonBoxWidget	bbw;
{
    Dimension junk;

    DoLayout(bbw, bbw->core.width, bbw->core.height, &junk, &junk, TRUE);
} /* Resize */

/*
 *
 * Try to do a new layout within the current width and height;
 * if that fails try to do it within the box returned by PreferredSize.
 *
 * TryNewLayout just says if it's possible, and doesn't actually move the kids
 */

static Boolean TryNewLayout(bbw)
    ButtonBoxWidget	bbw;
{
    Dimension		width, height;

    if (!PreferredSize(bbw, bbw->core.width, bbw->core.height, &width, &height))
	(void) PreferredSize(bbw, width, height, &width, &height);

    if ((bbw->core.width == width) && (bbw->core.height == height)) {
        /* Same size */
	return (TRUE);
    }

    /* let's see if our parent will go for a new size. */
    switch (XtMakeResizeRequest((Widget) bbw, width, height, &width, &height)) {

	case XtGeometryYes:
	    return (TRUE);

	case XtGeometryNo:
	    return (FALSE);

	case XtGeometryAlmost:
	    if (! PreferredSize(bbw, width, height, &width, &height))
	        return (FALSE);
	    (void) XtMakeResizeRequest((Widget) bbw, width, height, 
					&width, &height);
	    return (TRUE);
    }
}

/*
 *
 * Geometry Manager
 *
 */

/*ARGSUSED*/
static XtGeometryResult GeometryManager(w, request, reply)
    Widget		w;
    XtWidgetGeometry	*request;
    XtWidgetGeometry	*reply;	/* RETURN */

{
    Dimension	width, height, borderWidth, junk;
    ButtonBoxWidget bbw;

    /* Position request always denied */
    if (request->request_mode & (CWX | CWY))
        return (XtGeometryNo);

    /* Size changes must see if the new size can be accomodated */
    if (request->request_mode & (CWWidth | CWHeight | CWBorderWidth)) {

	/* Make all three fields in the request valid */
	if ((request->request_mode & CWWidth) == 0)
	    request->width = w->core.width;
	if ((request->request_mode & CWHeight) == 0)
	    request->height = w->core.height;
        if ((request->request_mode & CWBorderWidth) == 0)
	    request->border_width = w->core.border_width;

	/* Save current size and set to new size */
	width = w->core.width;
	height = w->core.height;
	borderWidth = w->core.border_width;
	w->core.width = request->width;
	w->core.height = request->height;
	w->core.border_width = request->border_width;

	/* Decide if new layout works: (1) new button is smaller,
	   (2) new button fits in existing ButtonBox, (3) ButtonBox can be
	   expanded to allow new button to fit */

	bbw = (ButtonBoxWidget) w->core.parent;
	if (((request->width + request->border_width <= width + borderWidth) &&
	    (request->height + request->border_width <= height + borderWidth))
	|| PreferredSize(bbw, bbw->core.width, bbw->core.height, &junk, &junk)
	|| TryNewLayout(bbw)) {
	    /* Fits in existing or new space, relayout */
	    Resize(bbw);
	    return (XtGeometryYes);
	} else {
	    /* Cannot satisfy request, change back to original geometry */
	    w->core.width = width;
	    w->core.height = height;
	    w->core.border_width = borderWidth;
	    return (XtGeometryNo);
	}
    }; /* if any size changes requested */

    /* Any stacking changes don't make a difference, so allow if that's all */
    return (XtGeometryYes);
}

static void ChangeManaged(bbw)
    ButtonBoxWidget bbw;
{
    /* Reconfigure the button box */
    (void) TryNewLayout(bbw);
    Resize(bbw);
}

static void Initialize(request, new, args, num_args)
    ButtonBoxWidget request, new;
    ArgList args;
    Cardinal num_args;
{
/* ||| What are consequences of letting height, width be 0? If okay, then
       Initialize can be NULL */

    if (new->core.width == 0)
        new->core.width =
	    ((new->button_box.h_space != 0) ? new->button_box.h_space : 10);
    if (new->core.height == 0)
	new->core.height = 
	    ((new->button_box.v_space != 0) ? new->button_box.v_space : 10);
} /* Initialize */

/* ||| Should Realize just return a modified mask and attributes?  Or will some
   of the other parameters change from class to class? */
static void Realize(w, valueMask, attributes)
    register Widget w;
    Mask valueMask;
    XSetWindowAttributes *attributes;
{
    attributes->bit_gravity = NorthWestGravity;
    valueMask |= CWBitGravity;
    
    w->core.window =
	XCreateWindow(XtDisplay(w), XtWindow(w->core.parent),
	w->core.x, w->core.y, w->core.width, w->core.height,
	w->core.border_width, w->core.depth,
	InputOutput, (Visual *)CopyFromParent,
	valueMask, attributes);
} /* Realize */

/*
 *
 * Set Values
 *
 */

static Boolean SetValues (current, request, new, last)
    ButtonBoxWidget current, request, new;
    Boolean last;
{
    /* ||| Old code completely bogus, need background, etc., then
    XtMakeGeometryRequest, then relayout */
    return (FALSE);
}

