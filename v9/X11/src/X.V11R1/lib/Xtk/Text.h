/*
* $Header: Text.h,v 1.2 87/09/11 21:24:44 haynes Rel $
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

#define XtNtextOptions		"textOptions"
#define XtNdisplayPosition      "displayPosition"
#define XtNinsertPosition	"insertPosition"
#define XtNleftMargin		"leftMargin"
#define XtNselectionArray	"selectionArray"
#define XtNtextSource		"textSource"
#define XtNtextSink		"textSink"
#define XtNselection		"selection"

#define XtNeditType		"editType"
#define XtNfile			"file"
#define XtNstring		"string"
#define XtNlength		"length"
#define XtNfont			"font"

/* Class record constants */

extern WidgetClass textWidgetClass;

typedef struct _TextClassRec *TextWidgetClass;
typedef struct _TextRec      *TextWidget;

/* other stuff */

typedef long XtTextPosition;

extern Widget XtTextDiskCreate(); /* parent, args, argCount */
    /* Widget   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

extern Widget XtTextStringCreate(); /* parent, args, argCount */
    /* Widget   parent;     */
    /* ArgList  args;    */
    /* int      argCount;   */

typedef enum {XttextRead, XttextAppend, XttextEdit} XtEditType;
#define wordBreak		0x01
#define scrollVertical		0x02
#define scrollHorizontal	0x04
#define scrollOnOverflow	0x08
#define resizeWidth		0x10
#define resizeHeight		0x20
#define editable		0x40

extern void XtTextDisplay(); /* w */
    /* Widget w; */

extern void XtTextSetSelectionArray(); /* w, sarray */
    /* Widget        w;		*/
    /* SelectionType *sarray;   */

extern void XtTextSetLastPos(); /* w, lastPos */
    /* Widget        w;		*/
    /* XtTextPosition lastPos;  */

extern void XtTextGetSelectionPos(); /* dpy, w, left, right */
    /* Widget        w;		*/
    /* XtTextPosition *left, *right;    */

extern void XtTextNewSource(); /* dpy, w, source, startPos */
    /* Widget        w;		*/
    /* XtTextSource   *source;      */
    /* XtTextPosition startPos;     */

extern int XtTextReplace(); /* w, startPos, endPos, text */
    /* Widget        w;		*/
    /* XtTextPosition   startPos, endPos; */
    /* XtTextBlock      *text; */

extern XtTextPosition XtTextTopPosition(); /* w */
    /* Widget        w;		*/

extern void XtTextSetInsertionPoint(); /*  w, position */
    /* Widget        w;		*/
    /* XtTextPosition position; */

extern XtTextPosition XtTextGetInsertionPoint(); /* w */
    /* Widget        w;		*/

extern void XtTextUnsetSelection(); /* w */
    /* Widget        w;		*/

extern void XtTextChangeOptions(); /* w, options */
    /* Widget        w;		*/
    /* int    options; */

extern int XtTextGetOptions(); /* w */
    /* Widget        w;		*/

extern void XtTextSetNewSelection(); /* w, left, right */
    /* Widget        w;		*/
    /* XtTextPosition left, right; */

extern void XtTextInvalidate(); /* w, from, to */
    /* Widget        w;		*/
    /* XtTextPosition from, to; */

extern Window XtTextGetInnerWindow(); /* w */
    /* Widget        w;		*/
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
