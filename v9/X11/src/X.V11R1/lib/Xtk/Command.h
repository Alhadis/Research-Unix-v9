/*
* $Header: Command.h,v 1.5 87/09/11 21:18:54 haynes Rel $
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
#ifndef _XtCommand_h
#define _XtCommand_h

/***********************************************************************
 *
 * Command Widget
 *
 ***********************************************************************/

#ifndef _XtJustify_e
#define _XtJustify_e




typedef enum {
    XtJustifyLeft,       /* justify text to left side of button   */
    XtJustifyCenter,     /* justify text in center of button      */
    XtJustifyRight       /* justify text to right side of button  */
} XtJustify;
#endif _XtJustify_e

#define XtRjustify		"Justify"
#define XtNhighlightThickness    "hightlightThickness"
#define XtNtranslations         "Translations"
#define XtCTranslations         "Translations"
#define XtRTranslationTable     "TranslationTable"

extern XtCallbackKind  activateCommand;
extern WidgetClass     commandWidgetClass;

typedef struct _CommandClassRec   *CommandWidgetClass;
typedef struct _CommandRec        *CommandWidget;


#endif _XtCommand_h
/* DON'T ADD STUFF AFTER THIS */
