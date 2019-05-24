/* $Header: Xtlib.h,v 1.1 87/09/11 07:59:54 toddb Exp $ */
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

/***********************************************************************
 *
 * X Toolkit Header File
 *
 ***********************************************************************/

/*
 *
 * This header file includes the X Toolkit header files for the toolkit
 * intrinsics and all of the base widgets.  If your application will not
 * be using all of the widgets, your code will compile MUCH faster if
 * you only include the headers applicable to the widgets you need.
 *
 */

#ifndef _Xtlib_h
#define _Xtlib_h

#include <X11/Intrinsic.h>
#include <X11/Atoms.h>
#include <X11/Boolean.h>
#include <X11/ButtonBox.h>
#include <X11/Command.h>
#include <X11/Dialog.h>
#include <X11/Form.h>
#include <X11/Label.h>
#include <X11/Menu.h>
#include <X11/Scroll.h>
#include <X11/Text.h>
#include <X11/VPane.h>

#endif _Xtlib_h
/* DON'T ADD STUFF AFTER THIS #endif */
