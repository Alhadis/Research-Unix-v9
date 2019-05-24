/*
* $Header: Label.h,v 1.8 87/09/11 21:21:47 haynes Rel $
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
#ifndef _XtLabel_h
#define _XtLabel_h

/***********************************************************************
 *
 * Label Widget
 *
 ***********************************************************************/

#ifndef _XtJustify_e
#define _XtJustify_e

typedef enum {
    XtJustifyLeft,       /* justify text to left side of button   */
    XtJustifyCenter,     /* justify text in center of button      */
    XtJustifyRight       /* justify text to right side of button  */
} XtJustify;
#endif _XtJustify_e

#define XtNjustify		"justify"
#define XtNforeground		"foreground"
#define XtNlabel		"label"
#define XtNfont			"font"
#define XtNinternalWidth	"internalWidth"
#define XtNinternalHeight	"internalHeight"
 
/* Class record constants */

extern WidgetClass labelWidgetClass;

typedef struct _LabelClassRec *LabelWidgetClass;
typedef struct _LabelRec      *LabelWidget;

#endif _XtLabel_h
/* DON'T ADD STUFF AFTER THIS #endif */
