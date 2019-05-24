/*
* $Header: Atoms.h,v 1.10 87/09/11 21:18:32 swick Locked $
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
#ifndef _XtAtom_h_
#define _XtAtom_h_

#include <X11/Xresource.h>

/* Resource names */

#define XtNallowHoriz		"allowHoriz"
#define XtNallowVert		"allowVert"
#define XtNbackground		"background"
#define XtNborder		"border"
#define XtNborderWidth		"borderWidth"
#define XtNchildWindow		"childWindow"
#define XtNdepth		"depth"
#define XtNdialogButtons	"dialogButtons"
#define XtNdialogButtonCount	"dialogButtonCount"
#define XtNdialogValue		"dialogValue"
#define XtNeditType		"editType"
#define XtNeventBindings	"eventBindings"
#define XtNfont			"font"
#define XtNforceBars		"forceBars"
#define XtNforeground		"foreground"
#define XtNfunction		"function"
#define XtNheight		"height"
#define XtNhSpace		"hSpace"
#define XtNindex		"index"
#define XtNinnerHeight		"innerHeight"
#define XtNinnerWidth		"innerWidth"
#define XtNinnerWindow		"innerWindow"
#define XtNinsertPosition	"insertPosition"
#define XtNinternalHeight	"internalHeight"
#define XtNinternalWidth	"internalWidth"
#define XtNjustify		"justify"
#define XtNknobHeight		"knobHeight"
#define XtNknobIndent		"knobIndent"
#define XtNknobPixel		"knobPixel"
#define XtNknobWidth		"knobWidth"
#define XtNlabel		"label"
#define XtNlength		"length"
#define XtNlowerRight		"lowerRight"
#define XtNmappedWhenManaged	"mappedWhenManaged"
#define XtNmenuEntry		"menuEntry"
#define XtNname			"name"
#define XtNnotify		"notify"
#define XtNorientation		"orientation"
#define XtNparameter		"parameter"
#define XtNreverseVideo		"reverseVideo"
#define XtNscrollProc		"scrollProc"
#define XtNscrollDCursor	"scrollDownCursor"
#define XtNscrollHCursor	"scrollHorizontalCursor"
#define XtNscrollLCursor	"scrollLeftCursor"
#define XtNscrollRCursor	"scrollRightCursor"
#define XtNscrollUCursor	"scrollUpCursor"
#define XtNscrollVCursor	"scrollVerticalCursor"
#define XtNselection		"selection"
#define XtNselectionArray	"selectionArray"
#define XtNsensitive		"sensitive"
#define XtNshown		"shown"
#define XtNspace		"space"
#define XtNstring		"string"
#define XtNtextOptions		"textOptions"
#define XtNtextSink		"textSink"
#define XtNtextSource		"textSource"
#define XtNthickness		"thickness"
#define XtNthumb		"thumb"
#define XtNthumbProc		"thumbProc"
#define XtNtop			"top"
#define XtNuseBottom		"useBottom"
#define XtNuseRight		"useRight"

#define XtNvalue		"value"
#define XtNvSpace		"vSpace"
#define XtNwidth		"width"
#define XtNwindow		"window"
#define XtNx			"x"
#define XtNy			"y"


/* Class types */ 

#define XtCBackground		"Background"
#define XtCBitmap		"Bitmap"
#define XtCBoolean		"Boolean"
#define XtCBorderColor		"BorderColor"
#define XtCBorderWidth		"BorderWidth"
#define XtCColor		"Color"
#define XtCCursor		"Cursor"
#define XtCDepth		"Depth"
#define XtCDialogButtons	"DialogButtons"
#define XtCDialogValue		"DialogValue"
#define XtCEditType		"EditType"
#define XtCEventBindings	"EventBindings"
#define XtCFile			"File"
#define XtCFont			"Font"
#define XtCForeground		"Foreground"
#define XtCFraction		"Fraction"
#define XtCFunction		"Function"
#define XtCHeight		"Height"
#define XtCHighlight		"Highlight"
#define XtCHSpace		"HSpace"
#define XtCIndex		"Index"
#define XtCInterval		"Interval"
#define XtCJustify		"Justify"
#define XtCKnobHeight		"KnobHeight"
#define XtCKnobIndent		"KnobIndent"
#define XtCKnobPixel		"KnobPixel"
#define XtCKnobWidth		"KnobWidth"
#define XtCLabel		"Label"
#define XtCLength		"Length"
#define XtCMappedWhenManaged	"MappedWhenManaged"
#define XtCMargin		"Margin"
#define XtCMenuEntry		"MenuEntry"
#define XtCNotify		"Notify"
#define XtCOff			"Off"
#define XtCOn			"On"
#define XtCOrientation		"Orientation"
#define XtCParameter		"Parameter"
#define XtCPixmap		"Pixmap"
#define XtCPosition		"Position"
#define XtCScrollProc		"ScrollProc"
#define XtCScrollDCursor	"ScrollDownCursor"
#define XtCScrollHCursor	"ScrollHorizontalCursor"
#define XtCScrollLCursor	"ScrollLeftCursor"
#define XtCScrollRCursor	"ScrollRightCursor"
#define XtCScrollUCursor	"ScrollUpCursor"
#define XtCScrollVCursor	"ScrollVerticalCursor"
#define XtCSelection		"Selection"
#define XtCSensitive		"Sensitive"
#define XtCSelectionArray	"SelectionArray"
#define XtCSpace		"Space"
#define XtCString		"String"
#define XtCTextOptions		"TextOptions"
#define XtCTextPosition		"TextPosition"
#define XtCTextSelection	"TextSelection"
#define XtCTextSink		"TextSink"
#define XtCTextSource		"TextSource"
#define XtCThickness		"Thickness"
#define XtCThumb		"Thumb"
#define XtCToggle		"Toggle"
#define XtCUnhighlight		"Unhighlight"
#define XtCValue		"Value"
#define XtCVSpace		"VSpace"
#define XtCWidth		"Width"
#define XtCWindow		"Window"
#define XtCX			"X"
#define XtCY			"Y"


/* Representation types */
#define XtRBoolean		XrmRBoolean
#define XtRColor		XrmRColor
#define XtRCursor		XrmRCursor
#define XtRDims			XrmRDims
#define XtRDisplay		XrmRDisplay
#define XtREditMode		"EditMode"
#define XtREventBindings	"EventBindings"
#define XtRFile			XrmRFile
#define XtRFloat		XrmRFloat
#define XtRFont			XrmRFont
#define XtRFontStruct		XrmRFontStruct
#define XtRFunction		"Function"
#define XtRJustify		"Justify"
#define XtRGeometry		XrmRGeometry
#define XtRInt			XrmRInt
#define XtROrientation		"Orientation"
#define XtRPixel		XrmRPixel
#define XtRPixmap		XrmRPixmap
#define XtRPointer		XrmRPointer
#define XtRString               XrmRString
#define XtRStringTable		"StringTable"
#define XtRTextPosition 	"XtTextPosition"
#define XtRWindow		XrmRWindow

/* Orientation enumeration constants */

#define XtEvertical		"vertical"
#define XtEhorizontal		"horizontal"

/* text edit enumeration constants */

#define XtEtextRead		"read"
#define XtEtextAppend		"append"
#define XtEtextEdit		"edit"

#endif _XtAtom_h_
