/*
* $Header: VPane.h,v 1.4 87/09/13 03:11:39 newman Exp $
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
 * Vertical Pane Widget (SubClass of CompositeClass)
 *
 ****************************************************************/

/* New Fields */
#define XtNforeground           "foreground"
#define XtNknobWidth		"knobWidth"
#define XtNknobHeight		"knobHeight"
#define XtNknobIndent		"knobIndent"
#define XtNknobPixel		"knobPixel"
#define XtNrefigureMode         "refigureMode"
#define XtNposition             "position"
#define XtNmin                  "min"
#define XtNmax                  "max"
#define XtNallowResize          "allowResize"
#define XtNautoChange           "autoChange"

#define XtCKnobWidth            "KnobWidth"
#define XtCKnobHeight           "KnobHeight"
#define XtCKnobIndent           "KnobIndent"
#define XtCKnobPixel            "KnobPixel"
#define XtCRefigureMode         "RefigureMode"
#define XtCPosition             "Position"
#define XtCMin                  "Min"
#define XtCMax                  "Max"
#define XtCAllowResize          "AllowResize"
#define XtCAutoChange           "AutoChange"

/* Class record constant */

extern WidgetClass vPaneWidgetClass;

typedef struct _VPaneRec       *VPaneWidget;

#endif _XtVPane_h
/* DON'T ADD STUFF AFTER THIS #endif */
