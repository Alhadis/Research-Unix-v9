#ifndef lint
static char rcsid[] = "$Header: AsciiSink.c,v 1.3 87/09/13 13:28:59 swick Exp $";
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
#include "Xlib.h"
#include "Xutil.h"
#include "Xatom.h"
#include "Intrinsic.h"
#include "Text.h"
#include "TextP.h"
#include "Atoms.h"

#define GETLASTPOS (*(source->scan))(source, 0, XtstFile, XtsdRight, 1, TRUE)
/* Private Ascii TextSink Definitions */

static unsigned bufferSize = 200;

typedef struct _AsciiSinkData {
    Pixel foreground;
    GC normgc, invgc, xorgc;
    XFontStruct *font;
    int tabwidth;
    Pixmap insertCursorOn;
    Pixmap insertCursorOff;
    InsertState laststate;
} AsciiSinkData, *AsciiSinkPtr;

static char *buf;

/* XXX foreground default should be XtDefaultFGPixel. How do i do that?? */

static XtResource SinkResources[] = {
    {XtNfont, XtCFont, XrmRFontStruct, sizeof (XFontStruct *),
        XtOffset(AsciiSinkPtr, font), XrmRString, "Fixed"},
    {XtNforeground, XtCColor, XrmRPixel, sizeof (int),
        XtOffset(AsciiSinkPtr, foreground), XrmRString, "Black"},    
};

/* Utilities */

static int CharWidth (data, x, c)
  AsciiSinkData *data;
  int x;
  char c;
{
    int     width, nonPrinting;
    XFontStruct *font = data->font;

    if (c == '\t')
        /* This is totally bogus!! need to know tab settings etc.. */
	return data->tabwidth - (x % data->tabwidth);
    if (c == LF)
	c = SP;
    nonPrinting = (c < SP);
    if (nonPrinting) c += '@';

    if (font->per_char &&
	    (c >= font->min_char_or_byte2 && c <= font->max_char_or_byte2))
	width = font->per_char[c - font->min_char_or_byte2].width;
    else
	width = font->min_bounds.width;

    if (nonPrinting)
	width += CharWidth(data, x, '^');

    return width;
}

/* Sink Object Functions */

#define LBEARING(x) \
    ((font->per_char != NULL && \
      ((x) >= font->min_char_or_byte2 && (x) <= font->max_char_or_byte2)) \
	? font->per_char[(x) - font->min_char_or_byte2].lbearing \
	: font->min_bounds.lbearing)

static int AsciiDisplayText (w, x, y, pos1, pos2, highlight)
  Widget w;
  Position x, y;
  int highlight;
  XtTextPosition pos1, pos2;
{
    XtTextSink *sink = ((TextWidget)w)->text.sink;
    XtTextSource *source = ((TextWidget)w)->text.source;
    AsciiSinkData *data = (AsciiSinkData *) sink->data ;

    XFontStruct *font = data->font;
    int     j, k;
    Dimension width;
    XtTextBlock blk;
    GC gc = highlight ? data->invgc : data->normgc;
    GC invgc = highlight ? data->normgc : data->invgc;

    y += font->ascent;
    j = 0;
    while (pos1 < pos2) {
	pos1 = (*(source->read))(source, pos1, &blk, pos2 - pos1);
	for (k = 0; k < blk.length; k++) {
	    if (j >= bufferSize - 5) {
		bufferSize *= 2;
		buf = XtRealloc(buf, bufferSize);
	    }
	    buf[j] = blk.ptr[k];
	    if (buf[j] == LF)
		buf[j] = ' ';
	    else if (buf[j] == '\t') {
	        XDrawImageString(XtDisplay(w), XtWindow(w),
			gc, x - LBEARING(*buf), y, buf, j);
		buf[j] = 0;
		x += XTextWidth(data->font, buf, j);
		width = CharWidth(data, x, '\t');
		XFillRectangle(XtDisplay(w), XtWindow(w), invgc, x,
			       y - font->ascent, width,
			       (Dimension) (data->font->ascent +
					    data->font->descent));
		x += width;
		j = -1;
	    }
	    else
		if (buf[j] < ' ') {
		    buf[j + 1] = buf[j] + '@';
		    buf[j] = '^';
		    j++;
		}
	    j++;
	}
    }
    XDrawImageString(XtDisplay(w), XtWindow(w), gc, x - LBEARING(*buf), y, buf, j);
}


static Pixmap CreateInsertCursor(dpy, gc, state)
Display *dpy;
GC gc;
InsertState state;
{
    XImage image;
    Pixmap insertCursor;

    /* stuff for the text insertion cursor */

#   define insertCursor_width 6
#   define insertCursor_height 3

    static short insertCursor_bits[] = {0x000c, 0x001e, 0x0033};
/*    static short insertCursor_bits[] = {0xfff3, 0xffe1, 0xffcc}; */
    static short blank_bits[] = {0x0000, 0x0000, 0x0000};

    image.height = insertCursor_height;
    image.width = insertCursor_width;
    image.xoffset = 0;
    image.format = XYBitmap;
    image.data = (char *) ((state == XtisOn) ? insertCursor_bits : blank_bits);
/*    image.data = (char *) insertCursor_bits;*/
    image.byte_order = LSBFirst;
    image.bitmap_unit = 16;
    image.bitmap_bit_order = LSBFirst;
    image.bitmap_pad = 16;
    image.depth = 1;
    image.bytes_per_line = ((insertCursor_width+15) / 16) * 2;
    image.bits_per_pixel = 1;
    image.obdata = NULL;
    insertCursor = XCreatePixmap(dpy, DefaultRootWindow(dpy), (Dimension) image.width,
				 (Dimension) image.height,
				 DefaultDepth(dpy, 0));
    /* !!! BOGUS -- should otherwise figure out the depth; this code may not
       work on multiple screen displays. ||| */
    XPutImage(dpy, insertCursor, gc, &image, 0, 0, 0, 0,
	      (Dimension)image.width, (Dimension)image.height);
    return insertCursor;
}

/*
 * The following procedure manages the "insert" cursor.
 */

static AsciiInsertCursor (w, x, y, state)
  Widget w;
  Position x, y;
  InsertState state;
{
    XtTextSink *sink = ((TextWidget)w)->text.sink;
    AsciiSinkData *data = (AsciiSinkData *) sink->data;

/*
    XCopyArea(sink->dpy,
	      (state == XtisOn) ? data->insertCursorOn : data->insertCursorOff, w,
	      data->normgc, 0, 0, insertCursor_width, insertCursor_height,
	      x - (insertCursor_width >> 1), y - (insertCursor_height));
*/
    if (state != data->laststate)
	XCopyArea(XtDisplay(w),
		  data->insertCursorOn, XtWindow(w),
		  data->xorgc, 0, 0, insertCursor_width, insertCursor_height,
		  x - (insertCursor_width >> 1), y - (insertCursor_height));
    data->laststate = state;
}

/*
 * Clear the passed region to the background color.
 */

static AsciiClearToBackground (w, x, y, width, height)
  Widget w;
  Position x, y;
  Dimension width, height;
{
    XtTextSink *sink = ((TextWidget)w)->text.sink;
    AsciiSinkData *data = (AsciiSinkData *) sink->data;
    XFillRectangle(XtDisplay(w), XtWindow(w), data->invgc, x, y, width, height);
}

/*
 * Given two positions, find the distance between them.
 */

static AsciiFindDistance (w, fromPos, fromx, toPos,
			  resWidth, resPos, resHeight)
  Widget w;
  XtTextPosition fromPos;	/* First position. */
  int fromx;			/* Horizontal location of first position. */
  XtTextPosition toPos;		/* Second position. */
  int *resWidth;		/* Distance between fromPos and resPos. */
  int *resPos;			/* Actual second position used. */
  int *resHeight;		/* Height required. */
{
    XtTextSink *sink = ((TextWidget)w)->text.sink;
    XtTextSource *source = ((TextWidget)w)->text.source;

    AsciiSinkData *data;
    register    XtTextPosition index, lastPos;
    register char   c;
    XtTextBlock blk;

    data = (AsciiSinkData *) sink->data;
    /* we may not need this */
    lastPos = GETLASTPOS;
    (*(source->read))(source, fromPos, &blk, toPos - fromPos);
    *resWidth = 0;
    for (index = fromPos; index != toPos && index < lastPos; index++) {
	if (index - blk.firstPos >= blk.length)
	    (*(source->read))(source, index, &blk, toPos - fromPos);
	c = blk.ptr[index - blk.firstPos];
	if (c == LF) {
	    *resWidth += CharWidth(data, fromx + *resWidth, SP);
	    index++;
	    break;
	}
	*resWidth += CharWidth(data, fromx + *resWidth, c);
    }
    *resPos = index;
    *resHeight = data->font->ascent + data->font->descent;
}


static AsciiFindPosition(w, fromPos, fromx, width, stopAtWordBreak, 
			 resPos, resWidth, resHeight)
  Widget w;
  XtTextPosition fromPos; 	/* Starting position. */
  int fromx;			/* Horizontal location of starting position. */
  int width;			/* Desired width. */
  int stopAtWordBreak;		/* Whether the resulting position should be at
				   a word break. */
  XtTextPosition *resPos;	/* Resulting position. */
  int *resWidth;		/* Actual width used. */
  int *resHeight;		/* Height required. */
{
    XtTextSink *sink = ((TextWidget)w)->text.sink;
    XtTextSource *source = ((TextWidget)w)->text.source;
    AsciiSinkData *data;
    XtTextPosition lastPos, index, whiteSpacePosition;
    int     lastWidth, whiteSpaceWidth;
    Boolean whiteSpaceSeen;
    char    c;
    XtTextBlock blk;
    data = (AsciiSinkData *) sink->data;
    lastPos = GETLASTPOS;

    (*(source->read))(source, fromPos, &blk, bufferSize);
    *resWidth = 0;
    whiteSpaceSeen = FALSE;
    c = 0;
    for (index = fromPos; *resWidth <= width && index < lastPos; index++) {
	lastWidth = *resWidth;
	if (index - blk.firstPos >= blk.length)
	    (*(source->read))(source, index, &blk, bufferSize);
	c = blk.ptr[index - blk.firstPos];
	if (c == LF) {
	    *resWidth += CharWidth(data, fromx + *resWidth, SP);
	    index++;
	    break;
	}
	*resWidth += CharWidth(data, fromx + *resWidth, c);
	if ((c == SP || c == TAB) && *resWidth <= width) {
	    whiteSpaceSeen = TRUE;
	    whiteSpacePosition = index;
	    whiteSpaceWidth = *resWidth;
	}
    }
    if (*resWidth > width && index > fromPos) {
	*resWidth = lastWidth;
	index--;
	if (stopAtWordBreak && whiteSpaceSeen) {
	    index = whiteSpacePosition + 1;
	    *resWidth = whiteSpaceWidth;
	}
    }
    if (index == lastPos && c != LF) index = lastPos + 1;
    *resPos = index;
    *resHeight = data->font->ascent + data->font->descent;
}


static int AsciiResolveToPosition (w, pos, fromx, width,
				   leftPos, rightPos)
  Widget w;
  XtTextPosition pos;
  int fromx,width;
  XtTextPosition *leftPos, *rightPos;
{
    int     resWidth, resHeight;
    XtTextSink *sink = ((TextWidget)w)->text.sink;
    XtTextSource *source = ((TextWidget)w)->text.source;

    AsciiFindPosition(w, pos, fromx, width, FALSE,
	    leftPos, &resWidth, &resHeight);
    if (*leftPos > GETLASTPOS)
	*leftPos = GETLASTPOS;
    *rightPos = *leftPos;
}


static int AsciiMaxLinesForHeight (w, height)
  Widget w;
  int height;
{
    AsciiSinkData *data;
    XtTextSink *sink = ((TextWidget)w)->text.sink;

    data = (AsciiSinkData *) sink->data;
    return(height / (data->font->ascent + data->font->descent));
}


static int AsciiMaxHeightForLines (w, lines)
  Widget w;
  int lines;
{
    AsciiSinkData *data;
    XtTextSink *sink = ((TextWidget)w)->text.sink;

    data = (AsciiSinkData *) sink->data;
    return(lines * (data->font->ascent + data->font->descent));
}


/***** Public routines *****/

static Boolean initialized = FALSE;
static XContext asciiSinkContext;

AsciiSinkInitialize()
{
    if (initialized)
    	return;
    initialized = TRUE;

    asciiSinkContext = XUniqueContext();

    buf = (char *) XtMalloc(bufferSize);
}


caddr_t XtAsciiSinkCreate (w, args, argCount)
    Widget w;
    ArgList 	args;
    int 	argCount;
{
    XtTextSink *sink;
    AsciiSinkData *data;
    unsigned long valuemask = (GCFont | GCGraphicsExposures |
			       GCForeground | GCBackground | GCFunction);
    XGCValues values;
    unsigned long wid;
    XFontStruct *font;

    if (!initialized)
    	AsciiSinkInitialize();

    sink = (XtTextSink *) XtMalloc(sizeof(XtTextSink));
    sink->display = AsciiDisplayText;
    sink->insertCursor = AsciiInsertCursor;
    sink->clearToBackground = AsciiClearToBackground;
    sink->findPosition = AsciiFindPosition;
    sink->findDistance = AsciiFindDistance;
    sink->resolve = AsciiResolveToPosition;
    sink->maxLines = AsciiMaxLinesForHeight;
    sink->maxHeight = AsciiMaxHeightForLines;
    sink->data = (int *) XtMalloc(sizeof(AsciiSinkData));
    data = (AsciiSinkData *) sink->data;

    XtGetSubresources (w, data, "subclass", "subclass", 
        SinkResources, XtNumber(SinkResources),
	args, argCount);

/* XXX do i have to XLoadQueryFont or does the resource guy do it for me */

    font = data->font;
    values.function = GXcopy;
    values.font = font->fid;
    values.graphics_exposures = (Bool) FALSE;
    values.foreground = data->foreground;
    values.background = w->core.background_pixel;
    data->normgc = XtGetGC(w, valuemask, &values);
    values.foreground = w->core.background_pixel;
    values.background = data->foreground;
    data->invgc = XtGetGC(w, valuemask, &values);
    values.function = GXxor;
    values.foreground = data->foreground ^ w->core.background_pixel;
    values.background = 0;
    data->xorgc = XtGetGC(w, valuemask, &values);

    wid = -1;
    if ((!XGetFontProperty(font, XA_QUAD_WIDTH, &wid)) || wid <= 0) {
	if (font->per_char && font->min_char_or_byte2 <= '0' &&
	    		      font->max_char_or_byte2 >= '0')
	    wid = font->per_char['0' - font->min_char_or_byte2].width;
	else
	    wid = font->max_bounds.width;
    }
    if (wid <= 0) wid = 1;
    data->tabwidth = 8 * wid;
    data->font = font;
/*    data->insertCursorOn = CreateInsertCursor(XtDisplay(w),
	data->normgc, XtisOn);*/
/*    data->insertCursorOff = CreateInsertCursor(XtDisplay(w),
	data->normgc, XtisOff);*/
    data->insertCursorOn = CreateInsertCursor(XtDisplay(w),
        data->xorgc, XtisOn);
    data->laststate = XtisOff;
    return(caddr_t) sink;
}

void XtAsciiSinkDestroy (w)
    Widget w;
{
    XtTextSink *sink = ((TextWidget)w)->text.sink;
    AsciiSinkData *data;

    data = (AsciiSinkData *) sink->data;
    XtFree((char *) data);
    XtFree((char *) sink);
}
