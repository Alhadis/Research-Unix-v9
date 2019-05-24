/*
 *	sccsid:	@(#)Conversion.h	1.6	2/25/87
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


/* $Header: Conversion.h,v 1.109 87/09/11 08:16:06 toddb Exp $ */

/* 
 * Conversion.h - interface to resource type conversion procs
 * 
 * Author:	haynes
 * 		Digital Equipment Corporation
 * 		Western Research Laboratory
 * Date:	Mon Jan  5 1987
 */


#ifndef _Conversion_h_
#define _Conversion_h_

typedef struct {unsigned int width, height; } Dims;
typedef struct {int xpos, ypos; } Pos;
typedef struct {Dims dims; Pos pos; } Geometry;

extern void LowerCase(); /* source, dest */
    /* char *source;		*/
    /* char *dest;    /* RETURN */

#endif
