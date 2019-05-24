/* $Header: AsciiSink.c,v 1.2 87/08/06 14:35:30 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)AsciiSink.c	1.9	2/24/87";
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
#include "Xatom.h"
#include "Intrinsic.h"
#include "Text.h"
#include "TextDisp.h"

#define GETLASTPOS (*(source->scan))(source, 0, XtstFile, XtsdRight, 1, TRUE)
/* Private Ascii TextSink Definitions */

static unsigned bufferSize = 200;

typedef struct _AsciiSinkData {
    GC normgc, invgc, xorgc;
    XFontStruct *font;
    int tabwidth;
    Pixmap insertCursorOn;
    Pixmap insertCursorOff;
    InsertState laststate;
} AsciiSinkData;

static char *buf;

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

static int AsciiDisplayText (sink, source, w, x, y, pos1, pos2, highlight)
  XtTextSink *sink;
  XtTextSource *source;
  Window w;
  Position x, y;
  int highlight;
  XtTextPosition pos1, pos2;
{
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
		XDrawImageString(sink->dpy, w, gc, x - LBEARING(*buf), y, buf, j);
		buf[j] = 0;
		x += XTextWidth(data->font, buf, j);
		width = CharWidth(data, x, '\t');
		XFillRectangle(sink->dpy, w, invgc, x,
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
    XDrawImageString(sink->dpy, w, gc, x - LBEARING(*buf), y, buf, j);
}


static Pixmap CreateInsertCursor(dpy, d, gc, state)
Display *dpy;
Drawable d;
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
    insertCursor = XCreatePixmap(dpy, d, (Dimension) image.width,
				 (Dimension) image.height,
				 DefaultDepth(dpy, DefaultScreen(dpy)));
    /* !!! BOGUS -- should otherwise figure out the depth; this code may not
       work on multiple screen displays. ||| */
    XPutImage(dpy, insertCursor, gc, &image, 0, 0, 0, 0,
	      (Dimension)image.width, (Dimension)image.height);
    return insertCursor;
}

/*
 * The following procedure manages the "insert" cursor.
 */

static AsciiInsertCursor (sink, w, x, y, state)
  XtTextSink *sink;
  Window w;
  Position x, y;
  InsertState state;
{
    AsciiSinkData *data = (AsciiSinkData *) sink->data;

/*
    XCopyArea(sink->dpy,
	      (state == XtisOn) ? data->insertCursorOn : data->insertCursorOff, w,
	      data->normgc, 0, 0, insertCursor_width, insertCursor_height,
	      x - (insertCursor_width >> 1), y - (insertCursor_height));
*/
    if (state != data->laststate)
	XCopyArea(sink->dpy,
		  data->insertCursorOn, w,
		  data->xorgc, 0, 0, insertCursor_width, insertCursor_height,
		  x - (insertCursor_width >> 1), y - (insertCursor_height));
    data->laststate = state;
}

/*
 * Clear the passed region to the background color.
 */

static AsciiClearToBackground (sink, w, x, y, width, height)
  XtTextSink *sink;
  Window w;
  Position x, y;
  Dimension width, height;
{
    AsciiSinkData *data = (AsciiSinkData *) sink->data;
    XFillRectangle(sink->dpy, w, data->invgc, x, y, width, height);
}

/*
 * Given two positions, find the distance between them.
 */

static AsciiFindDistance (sink, source, fromPos, fromx, toPos,
			  resWidth, resPos, resHeight)
  XtTextSink *sink;
  XtTextSource *source;
  XtTextPosition fromPos;	/* First position. */
  int fromx;			/* Horizontal location of first position. */
  XtTextPosition toPos;		/* Second position. */
  int *resWidth;		/* Distance between fromPos and resPos. */
  int *resPos;			/* Actual second position used. */
  int *resHeight;		/* Height required. */
{
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


static AsciiFindPosition(sink, source, fromPos, fromx, width, stopAtWordBreak, 
			 resPos, resWidth, resHeight)
  XtTextSink *sink;	
  XtTextSource *source;
  XtTextPosition fromPos; 	/* Starting position. */
  int fromx;			/* Horizontal location of starting position. */
  int width;			/* Desired width. */
  int stopAtWordBreak;		/* Whether the resulting position should be at
				   a word break. */
  XtTextPosition *resPos;	/* Resulting position. */
  int *resWidth;		/* Actual width used. */
  int *resHeight;		/* Height required. */
{
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


static int AsciiResolveToPosition (sink, source, pos, fromx, width,
				   leftPos, rightPos)
  XtTextSink *sink;
  XtTextSource *source;
  XtTextPosition pos;
  int fromx,width;
  XtTextPosition *leftPos, *rightPos;
{
    int     resWidth, resHeight;

    AsciiFindPosition(sink, source, pos, fromx, width, FALSE,
	    leftPos, &resWidth, &resHeight);
    if (*leftPos > GETLASTPOS)
	*leftPos = GETLASTPOS;
    *rightPos = *leftPos;
}


static int AsciiMaxLinesForHeight (sink, height)
  XtTextSink *sink;
  int height;
{
    AsciiSinkData *data;

    data = (AsciiSinkData *) sink->data;
    return(height / (data->font->ascent + data->font->descent));
}


static int AsciiMaxHeightForLines (sink, lines)
  XtTextSink *sink;
  int lines;
{
    AsciiSinkData *data;

    data = (AsciiSinkData *) sink->data;
    return(lines * (data->font->ascent + data->font->descent));
}


/* Public routines */

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


caddr_t XtAsciiSinkCreate (dpy, font, ink, background)
    Display *dpy;
    XFontStruct *font;
    int 	ink, background;
{
    Window window;
    XtTextSink *sink;
    AsciiSinkData *data;
    unsigned long valuemask = (GCFont | GCGraphicsExposures |
			       GCForeground | GCBackground | GCFunction);
    XGCValues values;
    unsigned long w;

    if (!initialized)
    	AsciiSinkInitialize();

    window = RootWindow(dpy, DefaultScreen(dpy));
    sink = (XtTextSink *) XtMalloc(sizeof(XtTextSink));
    sink->dpy = dpy;
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
    values.function = GXcopy;
    values.font = font->fid;
    values.graphics_exposures = (Bool) FALSE;
    values.foreground = ink;
    values.background = background;
    data->normgc = XtGetGC(
	dpy, (XContext)asciiSinkContext, window, valuemask, &values);
    values.foreground = background;
    values.background = ink;
    data->invgc = XtGetGC(
	dpy, (XContext)asciiSinkContext, window, valuemask, &values);
    values.function = GXxor;
    values.foreground = ink ^ background;
    values.background = 0;
    data->xorgc = XtGetGC(
	dpy, (XContext)asciiSinkContext, window, valuemask, &values);

    w = -1;
    if ((!XGetFontProperty(font, XA_QUAD_WIDTH, &w)) || w <= 0) {
	if (font->per_char && font->min_char_or_byte2 <= '0' &&
	    		      font->max_char_or_byte2 >= '0')
	    w = font->per_char['0' - font->min_char_or_byte2].width;
	else
	    w = font->max_bounds.width;
    }
    if (w <= 0) w = 1;
    data->tabwidth = 8 * w;
    data->font = font;
/*    data->insertCursorOn = CreateInsertCursor(dpy, window, data->normgc, XtisOn);*/
/*    data->insertCursorOff = CreateInsertCursor(dpy, window, data->normgc, XtisOff);*/
    data->insertCursorOn = CreateInsertCursor(dpy, window, data->xorgc, XtisOn);
    data->laststate = XtisOff;
    return(caddr_t) sink;
}

void XtAsciiSinkDestroy (sink)
    XtTextSink *sink;
{
    AsciiSinkData *data;

    data = (AsciiSinkData *) sink->data;
    XtFree((char *) data);
    XtFree((char *) sink);
}
