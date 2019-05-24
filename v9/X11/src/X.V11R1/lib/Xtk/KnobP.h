/*
* $Header: KnobPrivate.h,v 1.2 87/09/11 21:21:44 haynes Rel $
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
 *  KnobPrivate.h - Private definitions for Knob widget (Used by VPane Widget)
 *
 *  Author:       Jeanne M. Rich
 *                Digital Equipment Corporation
 *                Western Software Laboratory
 *  Date:         Wednesday September 9, 1987
 */

#ifndef _XtKnobPrivate_h
#define _XtKnobPrivate_h

/*******************************************************************************
 *
 * Knob Widget Private Date (Used by VPane Widget)
 *
 ******************************************************************************/

/* New fields for the Knob widget class record */
typedef struct {
   int mumble;
   /* no new procedures */
} KnobClassPart;

/* Full Class record declaration */
typedef struct _KnobClassRec {
    CoreClassPart    core_class;
    KnobClassPart    knob_class;
} KnobClassRec;

extern KnobClassRec knobClassRec;

/* New fields for the Knob widget record */
typedef struct {
   int mumble;
   /* no new fields */
} KnobPart;

/*****************************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************************/

typedef struct _KnobRec {
   CorePart    core;
   KnobPart    knob;
} KnobRec;

#endif _XtKnobPrivate_h
/* DON'T ADD STUFF AFTER THIS #endif */

