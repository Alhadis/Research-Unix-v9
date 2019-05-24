/*
* $Header: ButtonBoxP.h,v 1.2 87/09/11 21:18:41 haynes Rel $
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
 * ButtonBoxPrivate.h - Private definitions for ButtonBox widget
 * 
 * Author:	Joel McCormack
 * 		Digital Equipment Corporation
 * 		Western Software Laboratory
 * Date:	Mon Aug 31 1987
 */

#ifndef _XtButtonBoxPrivate_h
#define _XtButtonBoxPrivate_h

/***********************************************************************
 *
 * ButtonBox Widget Private Data
 *
 ***********************************************************************/

/* New fields for the ButtonBox widget class record */
typedef struct {
     int mumble;   /* No new procedures */
} ButtonBoxClassPart;

/* Full class record declaration */
typedef struct _ButtonBoxClassRec {
    CoreClassPart	core_class;
    CompositeClassPart  composite_class;
    ButtonBoxClassPart	button_box_class;
} ButtonBoxClassRec;

extern ButtonBoxClassRec buttonBoxClassRec;

/* New fields for the ButtonBox widget record */
typedef struct {
    Dimension   h_space, v_space;
} ButtonBoxPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _ButtonBoxRec {
    CorePart	    core;
    CompositePart   composite;
    ButtonBoxPart   button_box;
} ButtonBoxRec;

/* $Log:	ButtonBoxP.h,v $
 * Revision 1.2  87/09/11  21:18:41  haynes
 * ship it. clean up copyright, add rcs headers.
 * 
 * Revision 1.1  87/08/31  07:37:27  joel
 * Initial revision
 * 
 *  */


#endif _XtButtonBoxPrivate_h
/* DON'T ADD STUFF AFTER THIS #endif */

