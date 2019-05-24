/* $Header: Text.h,v 1.1 87/09/11 07:58:49 toddb Exp $ */
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

#ifndef _XtText_h
#define _XtText_h

/****************************************************************
 *
 * Text widget
 *
 ****************************************************************/

typedef long XtTextPosition;


#define XtNwindow		"window"
#define XtNeditType		"editType"
#define XtNtextOptions		"textOptions"
#define XtNdisplayPosition      "displayPosition"
#define XtNinsertPosition	"insertPosition"
#define XtNleftMargin		"leftMargin"
#define XtNselectionArray	"selectionArray"
#define XtNforeground		"foreground"
#define XtNbackground		"background"
#define XtNborder		"border"
#define XtNborderWidth		"borderWidth"
#define XtNfile			"file"
#define XtNstring		"string"
#define XtNlength		"length"
#define XtNfont			"font"
#define XtNeventBindings	"eventBindings"
#define XtNtextSource		"textSource"
#define XtNtextSink		"textSink"
#define XtNselection		"selection"
#define XtNwidth		"width"
#define XtNheight		"height"

extern Window XtTextDiskCreate(); /* parent, args, argCount */
    /* Window   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

extern Window XtTextStringCreate(); /* parent, args, argCount */
    /* Window   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

extern XtTextDiskDestroy(); /* w */
    /* Window w; */

extern XtTextStringDestroy(); /* w */
    /* Window w; */


typedef enum {XttextRead, XttextAppend, XttextEdit} XtEditType;
#define wordBreak		0x01
#define scrollVertical		0x02
#define scrollHorizontal	0x04
#define scrollOnOverflow	0x08
#define resizeWidth		0x10
#define resizeHeight		0x20
#define editable		0x40

extern Window XtTextCreate(); /* dpy, parent, args, argCount */
    /* Display *dpy; */
    /* Window   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

extern void XtTextSetValues(); /* dpy, window, args, argCount */
    /* Display  *dpy;       */
    /* Window   window;     */
    /* ArgList  args;       */
    /* int      argCount;   */

extern void XtTextGetValues(); /* dpy, window, args, argCount */
    /* Display  *dpy;       */
    /* Window   window;     */
    /* ArgList  args;       */
    /* int      argCount;   */

extern void XtTextDestroy(); /* dpy, w */
    /* Display *dpy; */
    /* Window w; */

extern void XtTextDisplay(); /* dpy, w */
    /* Display *dpy; */
    /* Window w; */

extern void XtTextSetSelectionArray(); /* dpy, w, sarray */
    /* Display *dpy; */
    /* Window        w;		*/
    /* SelectionType *sarray;   */

extern void XtTextSetLastPos(); /* dpy, w, lastPos */
    /* Display *dpy; */
    /* Window         w;	*/
    /* XtTextPosition lastPos;  */

extern void XtTextGetSelectionPos(); /* dpy, w, left, right */
    /* Display *dpy; */
    /* Window         w;		*/
    /* XtTextPosition *left, *right;    */

extern void XtTextNewSource(); /* dpy, w, source, startPos */
    /* Display *dpy; */
    /* Window         w;	    */
    /* XtTextSource   *source;      */
    /* XtTextPosition startPos;     */

extern int XtTextReplace(); /* dpy, w, startPos, endPos, text */
    /* Display *dpy; */
    /* Window		w; */
    /* XtTextPosition   startPos, endPos; */
    /* XtTextBlock      *text; */

extern XtTextPosition XtTextTopPosition(); /* dpy, w */
    /* Display *dpy; */
    /* Window w; */

extern void XtTextSetInsertionPoint(); /* dpy, w, position */
    /* Display *dpy; */
    /* Window         w; */
    /* XtTextPosition position; */

extern XtTextPosition XtTextGetInsertionPoint(); /* dpy, w */
    /* Display *dpy; */
    /* Window w; */

extern void XtTextUnsetSelection(); /* dpy, w */
    /* Display *dpy; */
    /* Window w; */

extern void XtTextChangeOptions(); /* dpy, w, options */
    /* Display *dpy; */
    /* Window w; */
    /* int    options; */

extern int XtTextGetOptions(); /* dpy, w */
    /* Display *dpy; */
    /* Window w; */

extern void XtTextSetNewSelection(); /* dpy, w, left, right */
    /* Display *dpy; */
    /* Window         w; */
    /* XtTextPosition left, right; */

extern void XtTextInvalidate(); /* dpy, w, from, to */
    /* Display *dpy; */
    /* Window w; */
    /* XtTextPosition from, to; */

extern Window XtTextGetInnerWindow(); /* dpy, w */
    /* Display *dpy; */
    /* Window w; */

/*
 * Stuff from AsciiSink
 */

extern caddr_t XtAsciiSinkCreate(); /* dpy, font, ink, background */
    /* Display *dpy; */
    /* XFontStruct  *font; */
    /* int	    ink, background */

extern void XtAsciiSinkDestroy(); /* sink */
    /* XtTextSink *sink */

#endif _XtText_h
/* DON'T ADD STUFF AFTER THIS #endif */
