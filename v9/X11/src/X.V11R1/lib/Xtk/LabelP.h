/*
* $Header: LabelPrivate.h,v 1.7 87/09/11 21:21:50 haynes Rel $
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
 * LabelPrivate.h - Private definitions for Label widget
 * 
 * Author:	Charles Haynes
 * 		Digital Equipment Corporation
 * 		Western Software Laboratory
 * Date:	Thu Aug 27 1987
 */

#ifndef _XtLabelPrivate_h
#define _XtLabelPrivate_h

/***********************************************************************
 *
 * Label Widget Private Data
 *
 ***********************************************************************/

/* New fields for the Label widget class record */

typedef struct {int foo;} LabelClassPart;

/* Full class record declaration */
typedef struct _LabelClassRec {
    CoreClassPart	core_class;
    LabelClassPart	label_class;
} LabelClassRec;

extern LabelClassRec labelClassRec;

/* New fields for the Label widget record */
typedef struct {
    Pixel	foreground;
    XFontStruct	*font;
    char	*label;
    XtJustify	justify;
    Dimension	internal_width;
    Dimension	internal_height;

    GC		normal_GC;
    GC          gray_GC;
    Pixmap      gray_pixmap;
    Position	label_x;
    Position	label_y;
    Dimension	label_width;
    Dimension	label_height;
    unsigned int label_len;
    Boolean     display_sensitive;
} LabelPart;


/****************************************************************
 *
 * Full instance record declaration
 *
 ****************************************************************/

typedef struct _LabelRec {
    CorePart	core;
    LabelPart	label;
} LabelRec;

/* $Log:	LabelPrivate.h,v $
 * Revision 1.7  87/09/11  21:21:50  haynes
 * ship it. clean up copyright, add rcs headers.
 * 
 * Revision 1.6  87/09/10  14:39:17  haynes
 * major renaming cataclysm, de-linted, cleaned up
 * 
 * Revision 1.5  87/08/31  07:35:16  chow
 * bugs
 * 
 * Revision 1.4  87/08/30  20:16:17  ackerman
 * Added grayGC, displaySensitive to LabelPart; renamed to normalGC
 *   Also changed to new naming scheme.
 * 
 * Revision 1.3  87/08/28  18:38:51  ackerman
 * Moved full instance declaration of LabelData, LabelWidget into file
 * 
 * Revision 1.2  87/08/28  15:19:38  haynes
 * changed how resizing works
 * 
 * Revision 1.1  87/08/27  16:55:13  haynes
 * Initial revision
 *  */


#endif _XtLabelPrivate_h
/* DON'T ADD STUFF AFTER THIS #endif */

