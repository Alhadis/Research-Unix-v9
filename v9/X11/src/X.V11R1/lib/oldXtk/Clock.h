/* $Header: Clock.h,v 1.1 87/09/11 07:59:04 toddb Exp $ */
/*
 *	sccsid:	%W%	%G%
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

#ifndef _XtClock_h
#define _XtClock_h

/***********************************************************************
 *
 * Clock Widget
 *
 ***********************************************************************/

#define XtNwindow		"window"
#define XtNborderWidth		"borderWidth"
#define XtNjustify		"justify"
#define XtNclock		"clock"
#define XtNborder		"border"
#define XtNforeground		"foreground"
#define XtNbackground		"background"
#define XtNfont			"font"
#define XtNwidth		"width"
#define XtNheight		"height"
#define XtNupdate		"update"
#define XtNhand			"hands"
#define XtNhigh			"highlight"
#define XtNanalog		"analog"
#define XtNchime		"chime"
#define XtNpadding		"padding"
#define XtNactive		"active"

/* classes */
#define XtCAnalog		"Analog"
#define XtCChime		"Chime"
#define XtCPadding		"Padding"
#define XtCActive		"Active"



extern Window XtCreateClock(); /* dpy, parent, args, argCount */
    /* Display  *dpy;	    */
    /* Window   parent;     */
    /* ArgList  args;       */
    /* int      argCount;   */

extern void XtClockSetValues(); /* dpy, window, args, argCount */
    /* Display  *dpy;       */
    /* Window   window;     */
    /* ArgList  args;       */
    /* int      argCount;   */

extern void XtClockGetValues(); /* dpy, window, args, argCount */
    /* Display  *dpy;       */
    /* Window   window;     */
    /* ArgList  args;       */
    /* int      argCount;   */

#endif _XtClock_h
/* DON'T ADD STUFF AFTER THIS #endif */
