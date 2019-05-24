/*
* $Header: TopLevel.h,v 1.5 87/09/11 21:25:01 haynes Rel $
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
#ifndef _XtTopLevel_h
#define _XtTopLevel_h

/***********************************************************************
 *
 * TopLevel Widget
 *
 ***********************************************************************/
/*
 * TopLevel specific atoms
 */
#define XtNiconName	"iconName"
#define XtCIconName	"IconName"
#define XtNiconPixmap	"iconPixmap"
#define XtCIconPixmap	"IconPixmap"
#define XtNiconWindow	"iconWindow"
#define XtCIconWindow	"IconWindow"
#define XtNallowtopresize	"allowTopResizeRequest"
#define XtCAllowtopresize	"AllowTopResizeRequest"
#define XtNtitle	"title"
#define XtCTitle	"Title"

/* 
 * The following are only used at creation and can not be changed via 
 * SetValues.
 */
#define XtNinput	"input"
#define XtCInput	"Input"
#define XtNiconic	"iconic"
#define XtCIconic	"Iconic"
#define XtNinitial	"initialstate"
#define XtCInitial	"InitialState"
#define XtNgeometry	"geometry"
#define XtCGeometry	"Geometry"

/* Class record constants */

typedef struct _TopLevelClassRec *TopLevelWidgetClass;

extern WidgetClass topLevelWidgetClass;

#endif _XtTopLevel_h
/* DON'T ADD STUFF AFTER THIS #endif */
