/* $Header: Load.h,v 1.1 87/09/11 08:00:10 toddb Exp $ */
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

#ifndef _XtLoad_h
#define _XtLoad_h

/***********************************************************************
 *
 * Load Widget
 *
 ***********************************************************************/

#define XtNwindow		"window"
#define XtNborderWidth		"borderWidth"
#define XtNjustify		"justify"
#define XtNLoad			"Load"
#define XtNborder		"border"
#define XtNforeground		"foreground"
#define XtNbackground		"background"
#define XtNfont			"font"
#define XtNinternalWidth	"internalWidth"
#define XtNinternalHeight	"internalHeight"
#define XtNwidth		"width"
#define XtNheight		"height"
#define XtNupdate		"update"
#define XtNscale		"scale"
#define XtNvmunix		"vmunix"
#define XtNminScale		"minScale"
 
#define XtCScale		"Scale"

extern Window XtLoadCreate(); /* dpy, parent, args, argCount */
    /* Display  *dpy;	    */
    /* Window   parent;     */
    /* ArgList  args;       */
    /* int      argCount;   */

extern void XtLoadSetValues(); /* dpy, window, args, argCount */
    /* Display  *dpy;       */
    /* Window   window;     */
    /* ArgList  args;       */
    /* int      argCount;   */

extern void XtLoadGetValues(); /* dpy, window, args, argCount */
    /* Display  *dpy;       */
    /* Window   window;     */
    /* ArgList  args;       */
    /* int      argCount;   */

#endif _XtLoad_h
/* DON'T ADD STUFF AFTER THIS #endif */
