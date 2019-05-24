#ifndef lint
static char rcsid[] = "$Header: VPane.c,v 1.7 87/09/13 13:31:15 swick Exp $";
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
 * VPane.c - VPane composite Widget (Converted to classing toolkit)
 *
 * Author:   Jeanne M. Rich (Original Terry Weissman)
 *           Digital Equipment Corporation
 *           Western Research Laboratory
 * Date:     Friday, Aug 28 1987
 *
 */

#include "Xlib.h"
#include "cursorfont.h"
#include "Intrinsic.h"
#include "VPaneP.h"
#include "VPane.h"
#include "Knob.h"
#include "Atoms.h"

#define offset(field) XtOffset(VPaneWidget,v_pane.field)
#define suboffset(field) XtOffset(SubWidgetPtr,field)

/**********************************************************************************
 *
 * Full instance record declaration
 *
 *********************************************************************************/

static XtResource resources[] = {
    {XtNforeground, XtCColor, XrmRPixel, sizeof(Pixel),
         offset(foreground_pixel), XrmRString, "Black"},
    {XtNknobWidth, XtCWidth, XrmRInt, sizeof(int),
         offset(knob_width), XrmRString, "9"},
    {XtNknobHeight, XtCHeight, XrmRInt, sizeof(int),
	 offset(knob_height), XrmRString, "9"},
    {XtNknobIndent, XtCKnobIndent, XrmRInt, sizeof(int),
	 offset(knob_indent), XrmRString, "16"},
    {XtNknobPixel, XtCKnobPixel, XrmRPixel, sizeof(int),
	 offset(knob_pixel), XrmRString, "Black"},
    {XtNrefigureMode, XtCRefigureMode, XrmRBoolean, sizeof(int),
         offset(refiguremode), XrmRString, "On"}
};

static XtResource subresources[] = {
    {XtNposition, XtCPosition, XrmRInt, sizeof(int),
         suboffset(position), XrmRString, "0"},
    {XtNmin, XtCMin, XrmRInt, sizeof(int),
         suboffset(min), XrmRString, "0"},
    {XtNmax, XtCMax, XrmRInt, sizeof(int),
         suboffset(max), XrmRString, "1000"},
    {XtNautoChange, XtCAutoChange, XrmRBoolean, sizeof(int),
         suboffset(autochange), XrmRString, "On"}
};

static void Initialize();
static void Realize();
static void Destroy();
static void Resize();
static void SetValues();
static XtGeometryResult GeometryManager();
static void ChangeManaged();
static void InsertChild();
static void DeleteChild();

VPaneClassRec vPaneClassRec = {
   {
/* core class fields */
    /* superclass         */   (WidgetClass) &constraintClassRec,
    /* class name         */   "VPane",
    /* size               */   sizeof(VPaneRec),
    /* class initialize   */   NULL,
    /* class_inited       */   FALSE,
    /* initialize         */   Initialize,
    /* realize            */   Realize,
    /* actions            */   NULL,
    /* num_actions        */   0,
    /* resourses          */   resources,
    /* resource_count     */   XtNumber(resources),
    /* xrm_class          */   NULLQUARK,
    /* compress_motion    */   TRUE,
    /* compress_exposure  */   TRUE,
    /* visible_interest   */   FALSE,
    /* destroy            */   Destroy,
    /* resize             */   Resize,
    /* expose             */   NULL,
    /* set_values         */   SetValues,
    /* accept_focus       */   NULL
   }, {
/* composite class fields */
    /* geometry_manager   */   GeometryManager,
    /* change_managed     */   ChangeManaged,
    /* insert_child       */   InsertChild,
    /* delete_child       */   DeleteChild,
    /* move_focus_to_next */   NULL,
    /* move_focus_to_prev */   NULL
   }, {
/* constraint class fields */
    /* subresourses       */   subresources,
			       /* subresource_count  */   XtNumber(subresources)
   }, {
    /* mumble             */   0  /* make C compiler happy */
   }
};

static int CursorNums[4] = {0,
			    XC_sb_up_arrow,
			    XC_sb_v_double_arrow,
			    XC_sb_down_arrow};

static Boolean NeedsAdjusting(vpw)
VPaneWidget vpw;
{

   int needed, i;

   for (i = 0; i < (vpw->composite.num_children/2); i++) {
      if (vpw->composite.children[i * 2]->core.managed)
         needed += sublist[i].dheight + BORDERWIDTH;
   }
   needed -= BORDERWIDTH;
   if (needed > vpw->composite.children[i * 2]->core.height)
      return(TRUE);
   else return(FALSE);

}

static void AdjustVPaneHeight(vpw, newheight)
  VPaneWidget vpw;
  Dimension newheight;
{
    XtWidgetGeometry request, reply;
    XtGeometryResult result;
    if (newheight < 1) newheight = 1;
    request.height = newheight;
    request.request_mode = CWHeight;
    result = XtMakeGeometryRequest(vpw, &request, &reply);
    if (result == XtGeometryAlmost) {
	request = reply;
	result = XtMakeGeometryRequest(vpw, &request, &reply);
    }
    if (result == XtGeometryYes) {
	vpw->core.height = reply.height;
    }
}


static RefigureLocations(vpw, position, dir)
  VPaneWidget vpw;
  int position;
  int dir;		/* -1 = up, 1 = down, 0 = this border only */
{
    SubWidgetPtr sub, firstsub;
    WidgetList children;
    int     i, old, y, cdir, num_children, child, firstchild;

    if (vpw->composite.num_mapped_children == 0 || !vpw->v_pane.refiguremode)
	return;

    children = vpw->composite.children;
    num_children = vpw->composite.num_children / 2;
    do {
	vpw->v_pane.heightused = 0;
	for (i = 0, sub = vpw->v_pane.sublist; i < num_children; i++, sub++) {
            if (children[i * 2]->core.managed) {
	        if (sub->dheight < sub->min)
		    sub->dheight = sub->min;
	        if (sub->dheight > sub->max)
		    sub->dheight = sub->max;
	        vpw->v_pane.heightused += sub->dheight + BORDERWIDTH;
            }
	}
        vpw->v_pane.heightused -= BORDERWIDTH;
	if (dir == 0 && vpw->v_pane.heightused != vpw->core.height) {
	    for (i = 0, sub = vpw->v_pane.sublist; i < num_children; i++, sub++) {
                if (children[i * 2]->core.managed) {
		    if (sub->dheight != children[i * 2]->core.height)
		        sub->dheight += vpw->core.height - vpw->v_pane.heightused;
                }
            }
	}
    } while (dir == 0 && vpw->v_pane.heightused != vpw->core.height);

    firstsub = vpw->v_pane.sublist + position;
    firstchild = position * 2;
    sub = firstsub;
    child = firstchild;
    cdir = dir;
    while (vpw->v_pane.heightused != vpw->core.height) {
        if (children[child]->core.managed) {
	    if (sub->autochange || cdir != dir) {
	        old = sub->dheight;
	        sub->dheight = vpw->core.height - vpw->v_pane.heightused + old;
	        if (sub->dheight < sub->min)
		    sub->dheight = sub->min;
	        if (sub->dheight > sub->max)
		    sub->dheight = sub->max;
	        vpw->v_pane.heightused += (sub->dheight - old);
	    }
	    sub += cdir;
            child += (cdir * 2);
	    while (sub < vpw->v_pane.sublist ||
                  sub - vpw->v_pane.sublist == num_children) {
	        cdir = -cdir;
	        if (cdir == dir) goto triplebreak;
	        sub = firstsub + cdir;
                child = firstchild + (cdir * 2);
	    }
        }
    }
triplebreak:
    y = -BORDERWIDTH;
    for (i = 0, sub = vpw->v_pane.sublist; i < num_children; i++, sub++) {
        if (child[i * 2]->core.managed) {
	   sub->dy = y;
	   y += sub->dheight + BORDERWIDTH;
        }
    }
}


static CommitNewLocations(vpw)
VPaneWidget vpw;
{
    int i, kx, ky, num_children;
    WidgetList children;
    KnobWidget lastknob;
    SubWidgetPtr sub;

    num_children = vpw->composite.num_children / 2;
    children = vpw->composite.children
    lastknob = NULL:

    /* First unmap all the knobs that are not managed */
    for (i = 0; i < vpw_composite.num_children; i+2)
       if (children[i]->core.managed == FALSE) {
          if (children[i + 1]->core.managed == TRUE) {
             XtUnmapWidget(children[i + 1]);
             children[i + 1]->core.managed = FALSE;
          }
       }
    }

    for (i = 0, sub = vpw->v_pane.sublist; i < num_children; i++, sub++) {
        if (children[i * 2]->core.managed) {

            /* see if the widget needs to be moved */
            if (sub->dy != children[i * 2]->core.y) 
                XtMoveWidget(children[i * 2], sub->dy, -BORDERWIDTH);
 
            /* see if the widget needs to be resized */
            if ((sub->dheight != children[i * 2]->core.height) ||
                (children[i * 2]->core.width != vpw->core.width) ||
                (children[i * 2]->core.borderwidth != BORDERWIDTH)) 
                XtResizeWidget(children[i * 2], vpw->core.height, sub->dheight,
                        BORDERWIDTH);

            /* Move and Display the Knob */
	    kx = vpw->core.width - vpw->v_pane.knobindent;
	    ky = children[i * 2]->core.y +  children[i * 2]->core.height -
                  (vpw->v_pane.knobheight/2)+1;
            XtMoveWidget(sub->knob, kx, ky);
            if (sub->knob->core.managed == FALSE) {
                XtMapWidget(sub->knob);
                sub->knob->core.managed = TRUE;
            }
            XRaiseWindow(XtDisplay(sub->knob), XtWindow(sub->knob));
            lastknob = sub->knob;
            
	}
    }

    if (lastknob != NULL) {
        XtUnMapWidget(lastknob);
        lastknob->core.managed = FALSE;
    }
}


static RefigureLocationsAndCommit(vpw, position, dir)
  VPaneWidget data;
  int position, dir;
{
    RefigureLocations(vpw, position, dir);
    CommitNewLocations(vpw);
}


EraseInternalBorders(data)
  WidgetData data;
{
    int     i;
    for (i = 1; i < data->numsubwindows; i++)
	XFillRectangle(data->dpy, data->window, data->invgc,
		       0, data->sub[i].y, data->width, BORDERWIDTH);
}
/*

DrawInternalBorders(data)
  WidgetData data;
{
    int     i;
    for (i = 1; i < data->numsubwindows; i++)
	XFillRectangle(data->dpy, data->window, data->normgc,
		       0, data->sub[i].y, data->width, BORDERWIDTH);
}


static DrawTrackLines(data)
  WidgetData data;
{
    int     i;
    SubWindowPtr sub;
    for (i = 1, sub = data->sub + 1; i < data->numsubwindows; i++, sub++) {
	if (sub->olddy != sub->dy) {
	    XDrawLine(data->dpy, data->window, data->flipgc,
		  0, sub->olddy, (Position) data->width, sub->olddy);
	    XDrawLine(data->dpy, data->window, data->flipgc,
		  0, sub->dy, (Position) data->width, sub->dy);
	    sub->olddy = sub->dy;
	}
    }
}

static EraseTrackLines(data)
  WidgetData data;
{
    int     i;
    SubWindowPtr sub;
    for (i = 1, sub = data->sub + 1; i < data->numsubwindows; i++, sub++)
	XDrawLine(data->dpy, data->window, data->flipgc,
	      0, sub->olddy, (Position) data->width, sub->olddy);
}

static XtEventReturnCode HandleKnob(event)
  XEvent *event;
{
    WidgetData data;
    int     position, diff, y, i;

    data = DataFromWindow(event->xbutton.display, event->xbutton.window);
    if (!data) return XteventNotHandled;
    switch (event->type) {
	case ButtonPress: 
	    y = event->xbutton.y;
	    if (data->whichtracking != -1)
		return XteventNotHandled;
	    for (position = 0; position < data->numsubwindows; position++)
		if (data->sub[position].knob == event->xbutton.window)
		    break;
	    if (position >= data->numsubwindows)
		return XteventNotHandled;
	    
	    (void) XGrabPointer(data->dpy, event->xbutton.window, FALSE,
				(unsigned int)PointerMotionMask | ButtonReleaseMask,
				GrabModeAsync, GrabModeAsync, None,
				XtGetCursor(data->dpy,
					    CursorNums[event->xbutton.button]),
				CurrentTime);
	    data->whichadd = data->whichsub = NULL;
	    data->whichdirection = 2 - event->xbutton.button; * Hack! *
	    data->starty = y;
	    if (data->whichdirection >= 0) {
		data->whichadd = data->sub + position;
		while (data->whichadd->max == data->whichadd->min &&
			data->whichadd > data->sub)
		    (data->whichadd)--;
	    }
	    if (data->whichdirection <= 0) {
		data->whichsub = data->sub + position + 1;
		while (data->whichsub->max == data->whichsub->min &&
			data->whichsub < data->sub + data->numsubwindows - 1)
		    (data->whichsub)++;
	    }
	    data->whichtracking = position;
	    if (data->whichdirection == 1)
		(data->whichtracking)++;
	    EraseInternalBorders(data);
	    for (i = 0; i < data->numsubwindows; i++)
		data->sub[i].olddy = -99;
	* Fall through *

	case MotionNotify: 
	case ButtonRelease:
	    if (event->type == MotionNotify) y = event->xmotion.y;
	    else y = event->xbutton.y;
	    if (data->whichtracking == -1)
		return XteventNotHandled;
	    for (i = 0; i < data->numsubwindows; i++)
		data->sub[i].dheight = data->sub[i].height;
	    diff = y - data->starty;
	    if (data->whichadd)
		data->whichadd->dheight = data->whichadd->height + diff;
	    if (data->whichsub)
		data->whichsub->dheight = data->whichsub->height - diff;
	    RefigureLocations(data, data->whichtracking, data->whichdirection);

	    if (event->type != ButtonRelease) {
		DrawTrackLines(data); * Draw new borders *
		return XteventHandled;
	    }
	    XUngrabPointer(data->dpy, CurrentTime);
	    EraseTrackLines(data);
	    CommitNewLocations(data);
	    DrawInternalBorders(data);
	    data->whichtracking = -1;
	    return XteventHandled;
    }
    return XteventNotHandled;
}
*/

/* Semi-public routines. */

static XtGeometryReturnCode GeometryManager(w, request, reply)
Widget w;
WidgetGeometry *request, *reply;
{

   return(XtgeometryNo);

}

static void Initialize(w)
Widget w;
{

    VPaneWidget vpw = (VPaneWidget) w;
    unsigned long valuemask;
    XGCValues values;

    values.foreground = vpw->v_pane.foreground_pixel;
    values.function = GXcopy;
    values.plane_mask = ~0;
    values.fill_style = FillSolid;
    values.fill_rule = EvenOddRule;
    values.subwindow_mode = IncludeInferiors;
    valuemask = GCForeground | GCFunction | GCPlaneMask | GCFillStyle
	| GCFillRule | GCSubwindowMode;
    vpw->v_pane.normgc = XtGetGC(vpw->core.display, vpw->core.window, valuemask,
                                   &values);
    values.foreground = vpw->core.background_pixel;
    vpw->v_pane.invgc = XtGetGC(vpw->core.display, vpw->core.window, valuemask,
                                  &values);
    values.function = GXinvert;
#if BORDERWIDTH == 1
    values.line_width = 0;	/* Take advantage of fast server lines. */
#else
    values.line_width = BORDERWIDTH;
#endif
    values.line_style = LineSolid;
    valuemask |= GCLineWidth | GCLineStyle;
    vpw->v_pane.flipgc = XtGetGC(vpw->core.display, vpw->core.window, valuemask,
                                 &values);
    vpw->v_pane.heightused = 0;
    vpw->v_pane.sublist = NULL;
    vpw->v_pane.whichtracking = -1;
    vpw->v_pane.refiguremode = TRUE;
}


static void Realize(w, valueMask, attributes)
register Widget w;
Mask valueMask;
XSetWindowAttributes *attributes;

{

    attributes->bit_gravity = NorthWestGravity;
    valueMask |= CWBitGravity;

    w->core.window = 
          XCreateWindow(w->core.display, w->core.parent, w->core.x, w->core.y,
                        w->core.width, w->core.height, w_core.border_width,
                        0, InputOutput, (Visual *)CopyFromParent, valueMask,
                        attributes);
} /* Realize */

static void Destroy(vpw)
register VPaneWidget vpw;
{

   XtFree((char *) vpw->v_pane.sublist);

} /* Destroy */


static void InsertChild(w, args, argcount)
register Widget w;
ArgList args;
int argcount;
{

   static Arg knobargs[] = {
      {XtNwidth, (XtArgVal) 0},
      {XtNheight, (XtArgVal) 0},
      {XtNbackground, (XtArgVal) "Black"}
   };

   Cardinal     position;
   VPaneWidgetClass  myclass;
   ConstraintWidgetClass superclass;
   ConstraintWidget cw;
   SubWidgetPtr sub;
   int i;

   myclass = (VPaneWidgetClass) vPaneWidgetClass;
   superclass = (ConstraintWidgetClass) myclass->core_class.superclass;

   /* insert the child widget in the composite children list with the */
   /* superclass insert_child routine.                                */
   superclass->composite_class.insert_child(w, args, argcount);

   if (!XtIsSubclass(w, KnobWidgetClass)) {
      cw = w->core.parent;

      /* ||| Panes will be added in the order they are created, temporarilly */
      /* they are also managed at creation time.                             */

      /* will normally get the position from the insert position routine */
      position = cw->composite_widget.num_children / 2;
      cw->v_pane.sublist =
         (SubWidgetPtr) XtRealloc((caddr_t) cw->v_pane.sublist,
         ((cw->composite_widget.num_children/2 + 1) * sizeof(SubWidgetInfo)));

      /* shift up pane info then position new pane info */
      for (i = cw->composite_widget.num_children/2; i > position; i++)
         cw->v_pane.sublist[i] = cw->v_pane.sublist[i - 1];

      /* set the desired height of the pane */
      cw->v_pane.sublist[position].dheight = w->core.height;

      /* Get the subresources for this child widget */
      sub = &(cw->v_pane.sublist[position]);
      XtGetSubResources(cw, sub, w->core.name,
         w->core.widget_class->core_class.class_name,
         subresources, XtNumber(subresources), args, argcount);

      /* Create the Knob Widget */
      knobargs[0].value = (XtArgVal) cw->v_pane.knob_width;
      knobargs[1].value = (XtArgVal) cw->v_pane.knob_height;
      knobargs[2].value = (XtArgVal) cw->v_pane.knob_pixel;
      sub->knob = XtCreateWidget("Knob", KnobWidgetClass, (CompositeWidget) cw,
              knobargs, XtNumber(knobargs));
/*
      XtSetEventHandler(sub->knob,
          ButtonPressMask | ButtonReleaseMask | ButtonMotionMask, FALSE, HandleKnob,
          (caddr_t) NULL);
*/

      XDefineCursor(XtDisplay(sub->knob), sub->knob,
                    XtGetCursor(XtDisplay(sub->knob), XC_double_arrow));

   }

} /* InsertChild */

static void DeleteChild(w)
Widget w;
{

   Cardinal     position;
   VPaneWidgetClass  myclass;
   ConstraintWidgetClass superclass;
   ConstraintWidget cw;
   SubWidgetPtr sub;

   /* remove the subwidget info and destroy the knob */
   if (!XtIsSubClass(w, KnobWidgetClass)) {
      cw = w->core.parent;

      /* find out the postion of the widget */
      for (position = 0; position < cw->composite.num_children; position++) {
         if (cw->composite.children[position] == w)
            break;
      }
      position = position/2;

      /* Destroy the knob */
      XtDestroyWidget(cw->v_pane.sublist[position].knob);

      /* Ripple the subwidget info down, over the deleted pane */
      for (i = position; i < cw->composite.num_children/2; i++)
         cw->v_pane.sublist[i] = cw->v_pane.sublist[i + 1];

   } 

   myclass = (VPaneWidgetClass) vPaneWidgetClass;
   superclass = (ConstraintWidgetClass) myclass->core_class.superclass;

   /* delete the child widget in the composite children list with the */
   /* superclass delete_child routine.                                */
   superclass->composite_class.delete_child(w);

} /* DeleteChild */

static void ChangeManaged(vpw)
VPaneWidget vpw;
{

   /* see if the height of the VPane needs to be adjusted to fit all the panes */
   if (NeedsAdjusting(vpw))
      AdjustVPaneHeight(vpw);
   RefigureLocationsAndCommit(vpw, 0, 1);

} /* ChangeManaged */

static void Resize(vpw)
VPaneWidget vpw;
{

   RefigureLocationsAndCommit(vpw, 0, 1);

} /* Resize */

static void SetValues(oldvpw, newvpw)
VPaneWidget oldvpw, newvpw;
{
} /* SetValues */


/* Change the min and max size of the given sub window. */

void XtVPanedSetMinMax(vpw, paneWidget, min, max)
VPaneWidget vpw;
Widget paneWidget;
int    min, max;
{

    int i;

    for (i = 0; i < vpw->composite.num_children; i + 2) {
	if (vpw->composite.children[i] == paneWidget) {
	    vpw->v_pane.sublist[i / 2].min = min;
	    vpw->v_pane.sublist[i / 2].max = max;
	    RefigureLocationsAndCommit(vpw, i/2, 1);
	    return;
	}
    }
}


/* Get the min and max size of the given sub window. */

void XtVPanedGetMinMax(vpw, paneWidget, min, max)
VPaneWidget vpw;
Widget paneWindow;
int    *min, *max;
{

    int i;

    for (i = 0; i < vpw->composite.num_children; i + 2) {
	if (vpw->composite.children[i] == paneWidget) {
	    *min = vpw->v_pane.sublist[i / 2].min;
	    *max = vpw->v_pane.sublist[i / 2].max;
	    return;
	}
    }
}

int XtVPanedGetNumSub(vpw)
VPaneWidget vpw;
{

   int i, num_sub_widgets;

   num_sub_widgets = 0;

   for (i = 0; i < vpw->composite.num_children; i + 2) {
      if (vpw->composite.children[i]->core.managed)
          num_sub_widgets++;
   }

   return(num_sub_widgets);
}




void XtVPanedRefigureMode(vpw, mode)
  VPaneWidget vpw;
  Boolean  mode;
{
    vpw->v_pane.refiguremode = mode;
	if (mode)
	    RefigureLocationsAndCommit(vpw, vpw->composite.num_children/2 - 1, -1);
    }
}
