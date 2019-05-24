#ifndef lint
static char rcsid[] = "$Header: Command.c,v 1.24 87/09/13 23:04:23 newman Exp $";
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
 * Command.c - Command button widget
 *
 * Rewritten for beta toolkit
 * Author:      Mark Ackerman
 *              MIT/Project Athena
 * Date:        August 27, 1987
 *
 * from Command.c (XToolkit, alpha version)
 *              Charles Haynes
 *              Digital Equipment Corporation
 *              Western Research Laboratory
 */

#define XtStrlen(s)	((s) ? strlen(s) : 0)

  /* The following are defined for the reader's convenience.  Any
     Xt..Field macro in this code just refers to some field in
     one of the substructures of the WidgetRec.  */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "Intrinsic.h"
#include "Label.h"
#include "LabelP.h"
#include "Command.h"
#include "CommandP.h"
#include "CommandInt.h"
#include "Atoms.h"

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

/* Private Data */

static char *defaultTranslation[] = {
    "<Btn1Down>:       set()",
    "<Btn1Up>:       notify() unset()",
    "<EnterWindow>:       highlight()",
    "<LeaveWindow>:       unhighlight()",
    NULL
};
static caddr_t defaultTranslations = (caddr_t)defaultTranslation;
static XtResource resources[] = { 

   {XtNfunction, XtCFunction, XtRFunction, sizeof(XtCallbackProc), 
      XtOffset(CommandWidget, command.callback), XtRFunction, (caddr_t)NULL},
   {XtNparameter, XtCParameter, XtRPointer, sizeof(caddr_t), 
      XtOffset(CommandWidget,command.closure), XtRPointer, (caddr_t)NULL},

   {XtNhighlightThickness, XtCThickness, XrmRInt, sizeof(Dimension),
      XtOffset(CommandWidget,command.highlight_thickness), XrmRString,"2"},
   {XtNtranslations, XtCTranslations, XtRTranslationTable,
      sizeof(_XtTranslations),
      XtOffset(CommandWidget, core.translations),XtRTranslationTable,
      (caddr_t)&defaultTranslations},
 };  

static XtActionsRec actionsList[] =
{
  {"set", (caddr_t) Set},
  {"notify", (caddr_t) Notify},
  {"highlight", (caddr_t) Highlight},
  {"unset", (caddr_t) Unset},
  {"unhighlight", (caddr_t) Unhighlight},
};

  /* ...ClassData must be initialized at compile time.  Must
     initialize all substructures.  (Actually, last two here
     need not be initialized since not used.)
  */
CommandClassRec commandClassRec = {
  {
    (WidgetClass) &labelClassRec,    /* superclass	*/    
    "Command",                               /* class_name	*/
    sizeof(CommandRec),                 /* size		*/
    ClassInitialize,                       /* class initialize  */
    FALSE,                                 /* class_inited      */
    Initialize,                            /* initialize	*/
    Realize,                               /* realize		*/
    actionsList,                           /* actions		*/
    XtNumber(actionsList),
    resources,                             /* resources	        */
    XtNumber(resources),                   /* resource_count	*/
    NULLQUARK,                             /* xrm_class	        */
    FALSE,
    FALSE,
    FALSE,                                 /* visible_interest	*/
    Destroy,                               /* destroy		*/
    Resize,                                /* resize		*/
    Redisplay,                             /* expose		*/
    SetValues,                             /* set_values	*/
    NULL,                                  /* accept_focus	*/
  },  /* CoreClass fields initialization */
  {
    0,                                     /* field not used    */
  },  /* LabelClass fields initialization */
  {
    0,                                     /* field not used    */
  },  /* CommandClass fields initialization */
};

  /* for public consumption */
WidgetClass commandWidgetClass = (WidgetClass) &commandClassRec;

XtCallbackKind  activateCommand;   /* for public consumption*/
/****************************************************************
 *
 * Private Procedures
 *
 ****************************************************************/

#ifdef saber_bug
static Cardinal commandCallbackOffset =
    XtOffset(CommandWidget,command.callback_list);
static void ClassInitialize()
{
  activateCommand = XtNewCallbackKind(
	commandWidgetClass, commandCallbackOffset);
						
} 
#else
static void ClassInitialize()
{
  activateCommand = XtNewCallbackKind(commandWidgetClass,
			 XtOffset(CommandWidget,command.callback_list));
						
} 
#endif

static void Get_inverseGC(cbw)
    CommandWidget cbw;
{
    XGCValues	values;

    /* Set up a GC for inverse (set) state */

    values.foreground   = ComWforeground;
    values.font		= ComWfont->fid;
    values.fill_style   = FillSolid;

    ComWinverseGC = XtGetGC((Widget)cbw,
    	(unsigned) GCForeground | GCFont | GCFillStyle, &values);
}

static void Get_inverseTextGC(cbw)
    CommandWidget cbw;
{
    XGCValues	values;

    /* Set up a GC for inverse (set) state */

    values.foreground   = ComWbackground; /* default is White */
    values.font		= ComWfont->fid;
    values.fill_style   = FillSolid;

    ComWinverseTextGC = XtGetGC((Widget)cbw,
    	(unsigned) GCForeground | GCFont | GCFillStyle, &values);
}

static void Get_highlightGC(cbw)
    CommandWidget cbw;
{
    XGCValues	values;
    
    /* Set up a GC for highlighted state.  It has a thicker
       line width for the highlight border */

    values.foreground   = ComWforeground;
    values.line_width   = ComWhighlightThickness;

    ComWhighlightGC = XtGetGC((Widget)cbw,
    	(unsigned) GCForeground | GCLineWidth, &values);
}


/* ARGSUSED */
static void Initialize(request, new, args, num_args)
 Widget request, new;
 ArgList args;
 Cardinal num_args;
{
    CommandWidget cbw = (CommandWidget) new;

    Get_inverseGC(cbw);
    Get_inverseTextGC(cbw);
    Get_highlightGC(cbw);
      /* Start the callback list if the client specified one in
	 the arglist */
    ComWcallbackList = NULL;
    if (ComWcallback != NULL)
      XtAddCallback((Widget)cbw,activateCommand,ComWcallback,ComWclosure);

      /* init flags for state */
    ComWset = FALSE;
    ComWhighlighted = FALSE;  
    ComWdisplayHighlighted = FALSE;
    ComWdisplaySet = FALSE;

} 

static void Realize(w, valueMask, attributes) 
     /* This is the same as LabelWidget */
    register Widget w;
    Mask valueMask;
    XSetWindowAttributes *attributes;
{
  XtCallParentProcedure3Args(realize,w,valueMask,attributes);
} 


/***************************
*
*  EVENT HANDLERS
*
***************************/

/* ARGSUSED */
static void Set(w,event)
     Widget w;
     XEvent *event;
{
  CommandWidget cbw = (CommandWidget)w;
  ComWset = TRUE;
  Redisplay(w, event);
}

/* ARGSUSED */
static void Unset(w,event)
     Widget w;
     XEvent *event;
{
  CommandWidget cbw = (CommandWidget)w;
  ComWset = FALSE;
  Redisplay(w, event);
}

/* ARGSUSED */
static void Highlight(w,event)
     Widget w;
     XEvent *event;
{
  CommandWidget cbw = (CommandWidget)w;
  ComWhighlighted = TRUE;
  Redisplay(w, event);
}

/* ARGSUSED */
static void Unhighlight(w,event)
     Widget w;
     XEvent *event;
{
  CommandWidget cbw = (CommandWidget)w;
  ComWhighlighted = FALSE;
  Redisplay(w, event);
}

/* ARGSUSED */
static void Notify(w,event)
     Widget w;
     XEvent *event;
{
  CommandWidget cbw = (CommandWidget)w;
  XtCallCallbacks((Widget)cbw,activateCommand,NULL);
}
/*
 * Repaint the widget window
 */

/************************
*
*  REDISPLAY (DRAW)
*
************************/

/* ARGSUSED */
static void Redisplay(w, event)
    Widget w;
    XEvent *event;
{
   CommandWidget cbw = (CommandWidget) w;
   XSetWindowAttributes window_attributes;

   /* Here's the scoop:  If the command button button is normal,
      you show the text.  If the command button is highlighted but 
      not set, you draw a thick border and show the text.
      If the command button is set, draw the button and text
      in inverse. */

   /* Just to make sure everything is okay, check for sensitivity,
      too.  If non-sensitive, then gray out the border. */

   /* Note that Redisplay must remember the state of its last
      draw to determine whether to erase the window before
      redrawing to avoid flicker.  If the state is the same,
      the window just needs to redraw (even on an expose). */

   if (!ComWsensitive && ComWdisplaySensitive) 
      {
	  /* change border to gray */
	window_attributes.border_pixmap = ComWgrayPixmap;
	XChangeWindowAttributes(XtDisplay(w),XtWindow(w),
				CWBorderPixmap,&window_attributes);
      }
   else if (ComWsensitive && !ComWdisplaySensitive)
     {
       /* change border to black */
       window_attributes.border_pixel = ComWforeground;
       XChangeWindowAttributes(XtDisplay(w),XtWindow(w),
			       CWBorderPixel,&window_attributes);
     }

   if ((!ComWhighlighted && ComWdisplayHighlighted) ||
       ComWsensitive != ComWdisplaySensitive ||
       (!ComWset && ComWdisplaySet))
     XClearWindow(XtDisplay(w),XtWindow(w));
     /* Don't clear the window if the button's in a set condition;
	instead, fill it with black to avoid flicker. (Must fil
	with black in case it was an expose */
   else if (ComWset) /* && ComWdisplaySet) ||  (ComWset && !ComWdisplaySet))*/
     XFillRectangle(XtDisplay(w),XtWindow(w), ComWinverseGC,
		    0,0,ComWwidth,ComWheight);

     /* check whether border is taken out of size of rectangle or
	is outside of rectangle */
   if (ComWhighlighted)
     XDrawRectangle(XtDisplay(w),XtWindow(w), ComWhighlightGC,
		    0,0,ComWwidth,ComWheight);

     /* draw the string:  there are three different "styles" for it,
	all in separate GCs */
   XDrawString(XtDisplay(w),XtWindow(w),
	       (ComWset ?  ComWinverseTextGC : 
		    (ComWsensitive ? ComWnormalGC : ComWgrayGC)),
		ComWlabelX, ComWlabelY, ComWlabel, (int) ComWlabelLen);

   ComWdisplayHighlighted = ComWhighlighted;
   ComWdisplaySet = ComWset;
   ComWdisplaySensitive = ComWsensitive;
}


static void Resize(w)
    Widget	w;
{
    /* Nothing changes specific to command.  Label must change. */
  XtCallParentProcedure(resize,w);
}

static void Destroy(w)
    Widget w;
{
  /* must free GCs and pixmaps */
}


/*
 * Set specified arguments into widget
 */
static Boolean SetValues (current, request, new, last)
    Widget current, request, new;
    Boolean last;
{
    CommandWidget cbw = (CommandWidget) current;
    CommandWidget newcbw = (CommandWidget) new;

     if (XtLField(newcbw,foreground) != ComWforeground)
       {
         XtDestroyGC(ComWinverseGC);
	 Get_inverseGC(newcbw);
         XtDestroyGC(ComWhighlightGC);
	 Get_highlightGC(newcbw);
       }
    else 
      {
	if (XtCField(newcbw,background_pixel) != ComWbackground ||
	     XtLField(newcbw,font) != ComWfont) {
	     XtDestroyGC(ComWinverseTextGC);
	     Get_inverseTextGC(newcbw);
	     }
	if (XtCBField(newcbw,highlight_thickness) != ComWhighlightThickness) {
	    XtDestroyGC(ComWhighlightGC);
	    Get_highlightGC(newcbw);
	}
      }
     
    /*  NEED TO RESET PROC AND CLOSURE */

     /* ACTIONS */
    /* Change Label to remove ClearWindow and Redisplay */
    /* Change Label to change GCs if foreground, etc */

    return (FALSE);  /* actually need to return TRUE if Redisplay is needed! */
}
