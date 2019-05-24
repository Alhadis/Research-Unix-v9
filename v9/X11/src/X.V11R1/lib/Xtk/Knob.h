/*
* $Header: Knob.h,v 1.2 87/09/11 21:21:42 haynes Rel $
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
 *  Knob.h - Public Definitions for Knob widget (used by VPane Widget)
 *
 *  Author:       Jeanne M. Rich
 *                Digital Equipment Corporation
 *                Western Software Laboratory
 *  Date:         Wednesday, September 9, 1987
 */

#ifndef _XtKnob_h
#define _XtKnob_h

/***************************************************************************
 *
 * Knob Widget (Used by VPane Widget)
 *
 **************************************************************************/

/* Class Record Constant */

extern WidgetClass knobWidgetClass;

typedef struct _KnobClassRec *KnobWidgetClass;
typedef struct _KnobRec      *KnobWidget;

#endif _XtKnob_h
/* DON'T ADD STUFF AFTER THIS #endif */
