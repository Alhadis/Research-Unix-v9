/* $Header: XrmConvert.h,v 1.1 87/09/11 08:16:13 toddb Exp $ */
/*
 *	sccsid:	@(#)XrmConvert.h	1.1	2/25/87
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
 * XrmConvert.h - more resource manager private definitions
 * 
 * Author:	joel
 * 		Digital Equipment Corporation
 * 		Western Research Laboratory
 * Date:	Sat Mar 26 1987
 */

#ifndef _XrmConvert_h_
#define _XrmConvert_h_

typedef XrmQuark     XrmRepresentation;
#define XrmAtomToRepresentation(atom)    ((XrmRepresentation)XrmAtomToQuark(atom))
#define	XrmRepresentationToAtom(type)	(XrmQuarkToAtom((XrmQuark) type))


extern void XrmRegisterTypeConverter(); /* fromType, toType, convertProc */
  /*  XrmRepresentation	fromType, toType;   */
  /*  ConvertTypeProc	convertProc;	    */

extern void XrmConvert(); /* dpy, fromType, from, toType, to */
  /*  XrmRepresentation	fromType;   */
  /*  XrmValue		from;       */
  /*  XrmRepresentation	toType;     */
  /*  XrmValue		*to;	    */

extern void _XrmConvert(); /* dpy, fromType, from, toType to */
  /*  Display		*dpy;	   */
  /*  XrmRepresentation	fromType;  */
  /*  XrmValue		from;      */
  /*  XrmRepresentation	toType;    */
  /*  XrmValue		*to;       */

void _XrmRegisterTypeConverter(); /* fromType, toType, convertProc */
  /* XrmRepresentation		fromType, toType; */
  /*  XrmTypeConverter		convertProc;  */
#endif
