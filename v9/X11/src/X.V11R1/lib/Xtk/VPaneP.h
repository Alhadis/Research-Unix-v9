/*
* $Header: VPanePrivate.h,v 1.4 87/09/13 03:11:49 newman Exp $
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
/*
 *  VPanePrivate.h - Private definitions for VPane widget
 *
 *  Author:       Jeanne M. Rich
 *                Digital Equipment Corporation
 *                Western Software Laboratory
 *  Date:         Friday Aug 28 1987
 */

#ifndef _XtVPanePrivate_h
#define _XtVPanePrivate_h

/************************************************************************************
 *
 * VPane Widget Private Data
 *
 ***********************************************************************************/

#define BORDERWIDTH          1 /* Size of borders between panes.  */
                               /* %%% Should not be a constant    */
#define DEFAULTKNOBWIDTH     9
#define DEFAULTKNOBHEIGHT    9
#define DEFAULTKNOBINDENT    16


/* New fields for the VPane widget class record */
typedef struct {
    int mumble;
    /* no new procedures */
} VPaneClassPart;

/* Full Class record declaration */
typedef struct _VPaneClassRec {
    CoreClassPart       core_class;
    CompositeClassPart  composite_class;
    ConstraintClassPart constraint_class;
    VPaneClassPart      v_pane_class;
} VPaneClassRec;

extern VPaneClassRec vPaneClassRec;


/* VPane SubWidget record */
typedef struct {
    int         position;                   /* position location in VPane */
    Position    dy;                         /* Desired Location */
    Position    olddy;                      /* The last value of dy. */
    Dimension   min;                        /* Minimum height */
    Dimension   max;                        /* Maximum height */
    short       autochange;                 /* Whether we're allowed to change this */
                                            /* subwidget's height without an        */
                                            /* explicit command from the user       */
    Dimension   dheight;                    /* Desired height */
    Widget      knob;                       /* The knob for this subwidget */
} SubWidgetInfo, *SubWidgetPtr;    

/* New Fields for the VPane widget record */
typedef struct {
    Pixel       foreground_pixel;          /* window foreground */
    Dimension   knob_width, knob_height;   /* Dimension of knobs */
    Position    knob_indent;               /* Location of knobs (offset from right */
                                           /* margin)                              */
    Pixel       knob_pixel;                /* Color of knobs                       */
    Dimension   heightused;                /* Total height used by subwidgets */
    SubWidgetInfo *sublist;                /* Array of info about subwidgets */
    int         whichtracking;             /* Which know we are tracking if any */
    Position    starty;                    /* Starting y value */
    int         whichdirection;            /* Which direction to refigure things in */
    SubWidgetPtr whichadd;                 /* Which subwidget to add changes to */
    SubWidgetPtr whichsub;                 /* Which subwidget to sub changes from */
    Boolean     refiguremode;              /* Whether to refigure changes right now */
    GC          normgc;                    /* GC to use when redrawing borders */
    GC          invgc;                     /* GC to use when erasing borders */
    GC          flipgc;                    /* GC to use when animating borders */
} VPanePart;

/*********************************************************************************
 *
 * Full instance record declaration
 *
 ********************************************************************************/

typedef struct _VPaneRec {
    CorePart       core;
    CompositePart  composite;
    ConstraintPart contstraint;
    VPanePart      v_pane;
} VPaneRec;

#endif _XtVPanePrivate_h
/* DON'T ADD STUFF AFTER THIS #endif */
