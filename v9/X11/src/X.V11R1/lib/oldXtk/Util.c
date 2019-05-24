/* $Header: Util.c,v 1.1 87/09/11 07:59:01 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)Util.c	1.1	4/01/87";
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


/* Util.c -- Utility routines useful to widget writers. */

#include "Xlib.h"
#include "Intrinsic.h"

Window XtCreateWindow(dpy, parent, x, y, width, height, 
		      borderWidth, border, background, bitgravity)
Display *dpy;
Window parent;
Position x, y;
Dimension width, height, borderWidth;
Pixel border;
Pixel background;
int bitgravity;
{
    XSetWindowAttributes values;
    values.border_pixel = border;
    values.background_pixel = background;
    values.bit_gravity = bitgravity;
    return XCreateWindow(dpy, parent, x, y, width, height, borderWidth, 0,
			 (unsigned int) CopyFromParent,
			 (Visual *) CopyFromParent,
			 CWBackPixel | CWBorderPixel | CWBitGravity, &values);
}
