/* $Header: Text.c,v 1.1 87/09/11 07:58:30 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)Text.c	1.27	2/25/87";
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


/* File: Text.c */

#include "Xlib.h"
#include "Intrinsic.h"
#include "Text.h"
#include "Scroll.h"
#include "Atoms.h"
#include "TextDisp.h"          /** all text subwindow programs include **/
#include <strings.h>

/* ||| Kludge, should be in header file somewhere */
extern caddr_t XtSetTextEventBindings();

extern void bcopy();

#define BufMax 1000
#define abs(x)	(((x) < 0) ? (-(x)) : (x))
#define min(x,y)	((x) < (y) ? (x) : (y))
#define max(x,y)	((x) > (y) ? (x) : (y))
#define GETLASTPOS  (*(ctx->source->scan)) (ctx->source, 0, XtstFile, XtsdRight, 1, TRUE)
#define BUTTONMASK 0x143d

#define  yMargin 2
#define zeroPosition ((XtTextPosition) 0)


static XContext textContext;

static TextContext glob, globinit;

static Resource resources[] = {
    {XtNwindow, XtCWindow, XrmRWindow, sizeof (Window),
	 (caddr_t) &glob.w, NULL},
    {XtNwidth, XtCWidth, XrmRInt, sizeof (int),
         (caddr_t) &glob.width, NULL},
    {XtNheight, XtCHeight, XrmRInt, sizeof (int),
         (caddr_t) &glob.height, NULL},
    {XtNbackground, XtCColor, XrmRPixel, sizeof (int),
        (caddr_t) &glob.background, (caddr_t) &XtDefaultBGPixel},
    {XtNborder, XtCColor, XrmRPixmap, sizeof (int),
        (caddr_t) &glob.border, (caddr_t) &XtDefaultFGPixel},
    {XtNborderWidth, XtCBorderWidth, XrmRInt, sizeof (int), 
        (caddr_t) &glob.borderWidth, NULL},
    {XtNdisplayPosition, XtCTextPosition, XrmRInt,
	 sizeof (XtTextPosition), (caddr_t) &glob.lt.top, NULL},
    {XtNselection, XtCSelection, XrmRPointer, sizeof(caddr_t),
	 (caddr_t) &glob.s, NULL},
    {XtNselectionArray, XtCSelectionArray, XrmRPointer, 
        sizeof(SelectionArray), (caddr_t) glob.sarray, NULL},
    {XtNtextOptions, XtCTextOptions, XrmRInt, sizeof (int),
        (caddr_t) &glob.options, NULL},
    {XtNleftMargin, XtCMargin, XrmRInt, sizeof (int), 
        (caddr_t) &glob.leftmargin, NULL},
/* |||   {XtNrightMargin, XtCMargin, XtRint, sizeof (int),
         (caddr_t) &rightMargin, NULL}, */
    {XtNinsertPosition, XtCTextPosition, XtRTextPosition, 
        sizeof(XtTextPosition), (caddr_t) &glob.insertPos, NULL},
    {XtNtextSink, XtCTextSink, XrmRPointer, sizeof (caddr_t),
         (caddr_t) &glob.sink, NULL},
    {XtNtextSource, XtCTextSource, XrmRPointer, sizeof (caddr_t), 
         (caddr_t) &glob.source, NULL},
    {XtNeventBindings, XtCEventBindings, XtREventBindings,
         sizeof (caddr_t), (caddr_t) &glob.eventTable, NULL},
};

static Boolean initialized = FALSE;

static void TextInitialize()
{
    if (initialized)
    	return;
    initialized = TRUE;

    textContext = XUniqueContext();
    globinit.source = NULL;
    globinit.sink = NULL;
    globinit.lt.top = 0;
    globinit.lt.lines = 0;
    globinit.lt.info = NULL;
    globinit.insertPos = 0;
    globinit.s.left = globinit.s.right = 0;
    globinit.s.type = XtselectPosition;
    globinit.width = 100;
    globinit.height = 0;
    globinit.leftmargin = 2;
    globinit.options = 0;
    globinit.sbar = globinit.outer = globinit.w = NULL;
    globinit.sarray[0] = XtselectPosition;
    globinit.sarray[1] = XtselectWord;
    globinit.sarray[2] = XtselectLine;
    globinit.sarray[3] = XtselectParagraph;
    globinit.sarray[4] = XtselectAll;
    globinit.sarray[5] = XtselectNull;
    globinit.showposition = TRUE;
    globinit.lastPos = 0;
    globinit.dialog = NULL;
    globinit.borderWidth = 1;
    globinit.eventTable = NULL;	/* This is WRONG; should be some nice */
				/* default table. */
}

/* Utility routines for support of Text */


/*
 * Procedure to manage insert cursor visibility for editable text.  It uses
 * the value of ctx->insertPos and an implicit argument. In the event that
 * position is immediately preceded by an eol graphic, then the insert cursor
 * is displayed at the beginning of the next line.
*/
static void InsertCursor (ctx, state)
  TextContext *ctx;
  InsertState state;
{
    Position x, y;
    int dy, line, visible;
    XtTextBlock text;

    if (ctx->lt.lines < 1) return;
    visible = LineAndXYForPosition(ctx, ctx->insertPos, &line, &x, &y);
    if (line < ctx->lt.lines)
	dy = (ctx->lt.info[line + 1].y - ctx->lt.info[line].y) + 1;
    else
	dy = (ctx->lt.info[line].y - ctx->lt.info[line - 1].y) + 1;

    /** If the insert position is just after eol then put it on next line **/
    if (x > ctx->leftmargin &&
	ctx->insertPos > 0 &&
	ctx->insertPos >= ctx->lastPos) {
	   /* reading the source is bogus and this code should use scan */
	   (*(ctx->source->read)) (ctx->source, ctx->insertPos - 1, &text, 1);
	   if (text.ptr[0] == '\n') {
	       x = ctx->leftmargin;
	       y += dy;
	   }
    }
    y += dy;
    if (visible)
	(*(ctx->sink->insertCursor))(ctx->sink, ctx->w, x, y, state);
}


/*
 * Procedure to register a span of text that is no longer valid on the display
 * It is used to avoid a number of small, and potentially overlapping, screen
 * updates. [note: this is really a private procedure but is used in
 * multiple modules].
*/
_XtTextNeedsUpdating(ctx, left, right)
  TextContext *ctx;
  XtTextPosition left, right;
{
    int     i;
    if (left < right) {
	for (i = 0; i < ctx->numranges; i++) {
	    if (left <= ctx->updateTo[i] && right >= ctx->updateFrom[i]) {
		ctx->updateFrom[i] = min(left, ctx->updateFrom[i]);
		ctx->updateTo[i] = max(right, ctx->updateTo[i]);
		return;
	    }
	}
	ctx->numranges++;
	if (ctx->numranges > ctx->maxranges) {
	    ctx->maxranges = ctx->numranges;
	    i = ctx->maxranges * sizeof(XtTextPosition);
	    ctx->updateFrom = (XtTextPosition *) 
   	        XtRealloc((char *)ctx->updateFrom, (unsigned) i);
	    ctx->updateTo = (XtTextPosition *) 
		XtRealloc((char *)ctx->updateTo, (unsigned) i);
	}
	ctx->updateFrom[ctx->numranges - 1] = left;
	ctx->updateTo[ctx->numranges - 1] = right;
    }
}


/*
 * Procedure to read a span of text in Ascii form. This is purely a hack and
 * we probably need to add a function to sources to provide this functionality.
 * [note: this is really a private procedure but is used in multiple modules].
*/
char *_XtTextGetText(ctx, left, right)
  TextContext *ctx;
  XtTextPosition left, right;
{
    char   *result, *tempResult;
    int length, resultLength;
    XtTextBlock text;
    XtTextPosition end, nend;
    
    resultLength = right - left + 10;	/* Bogus? %%% */
    result = (char *)XtMalloc((unsigned) resultLength);
    end = (*(ctx->source->read))(ctx->source, left, &text, right - left);
    (void) strncpy(result, text.ptr, text.length);
    length = text.length;
    while (end < right) {
        nend = (*(ctx->source->read))(ctx->source, end, &text, right - end);
	tempResult = result + length;
        (void) strncpy(tempResult, text.ptr, text.length);
	length += text.length;
        end = nend;
    }
    result[length] = 0;
    return result;
}



/* 
 * This routine maps an x and y position in a window that is displaying text
 * into the corresponding position in the source.
 */
static XtTextPosition PositionForXY (ctx, x, y)
  TextContext *ctx;
  Position x,y;
{
 /* it is illegal to call this routine unless there is a valid line table! */
    int     width, fromx, line;
    XtTextPosition position, resultstart, resultend;

    /*** figure out what line it is on ***/
    for (line = 0; line < ctx->lt.lines - 1; line++) {
	if (y <= ctx->lt.info[line + 1].y)
	    break;
    }
    position = ctx->lt.info[line].position;
    if (position >= ctx->lastPos)
	return ctx->lastPos;
    fromx = ctx->lt.info[line].x;	/* starting x in line */
    width = x - fromx;			/* num of pix from starting of line */
    (*(ctx->sink->resolve)) (ctx->sink, ctx->source, position, fromx, width,
	    &resultstart, &resultend);
    if (resultstart >= ctx->lt.info[line + 1].position)
	resultstart = (*(ctx->source->scan))(ctx->source,
		ctx->lt.info[line + 1].position, XtstPositions, XtsdLeft, 1, TRUE);
    return resultstart;
}

/*
 * This routine maps a source position in to the corresponding line number
 * of the text that is displayed in the window.
*/
int LineForPosition (ctx, position)
  TextContext *ctx;
  XtTextPosition position;
  /* it is illegal to call this routine unless there is a valid line table!*/
{
    int     line;

    if (position <= ctx->lt.info[0].position)
	return 0;
    for (line = 0; line < ctx->lt.lines; line++)
	if (position < ctx->lt.info[line + 1].position)
	    break;
    return line;
}

/*
 * This routine maps a source position into the corresponding line number
 * and the x, y coordinates of the text that is displayed in the window.
*/
static int LineAndXYForPosition (ctx, pos, line, x, y)
  TextContext *ctx;
  XtTextPosition pos;
  int *line;
  Position *x, *y;
  /* it is illegal to call this routine unless there is a valid line table!*/
{
    XtTextPosition linePos, endPos;
    int     visible, realW, realH;

    *line = 0;
    *x = ctx->leftmargin;
    *y = yMargin;
    visible = IsPositionVisible(ctx, pos);
    if (visible) {
	*line = LineForPosition(ctx, pos);
	*y = ctx->lt.info[*line].y;
	*x = ctx->lt.info[*line].x;
	linePos = ctx->lt.info[*line].position;
	(*(ctx->sink->findDistance))(ctx->sink, ctx->source, linePos,
                                     *x, pos, &realW, &endPos, &realH);
	*x = *x + realW;
    }
    return visible;
}

/*
 * This routine builds a line table. It does this by starting at the
 * specified position and measuring text to determine the staring position
 * of each line to be displayed. It also determines and saves in the
 * linetable all the required metrics for displaying a given line (e.g.
 * x offset, y offset, line length, etc.).
*/
static void BuildLineTable (ctx, position)
  TextContext *ctx;
  XtTextPosition position;
{
    Position x, y;
    Dimension width, realW, realH;
    int line, lines;
    XtTextPosition startPos, endPos;
    Boolean     rebuild;

    rebuild = (Boolean) (position != ctx->lt.top);
    lines = (*(ctx->sink->maxLines))(ctx->sink, ctx->height);
    if (ctx->lt.info != NULL && lines != ctx->lt.lines) {
	XtFree((char *) ctx->lt.info);
	ctx->lt.info = NULL;
    }
    if (ctx->lt.info == NULL) {
	ctx->lt.info = (LineTableEntry *)
	    XtMalloc((unsigned)sizeof(LineTableEntry) * (lines + 1));
	for (line = 0; line < lines; line++) {
	    ctx->lt.info[line].position = 0;
	    ctx->lt.info[line].y = 0;
	}
	rebuild = TRUE;
    }
    else
	lines = ctx->lt.lines;
    if (rebuild) {
	ctx->lt.top = position;
	ctx->lt.lines = lines;
	startPos = position;
	y = yMargin;
	for (line = 0; line <= ctx->lt.lines; line++) {
	    x = ctx->leftmargin;
	    ctx->lt.info[line].x = x;
	    ctx->lt.info[line].y = y;
	    ctx->lt.info[line].position = startPos;
	    if (startPos <= ctx->lastPos) {
		width = (ctx->options & resizeWidth) ? 9999 : ctx->width - x;
		(*(ctx->sink->findPosition))(ctx->sink, ctx->source, 
                        startPos, x,
			width, (ctx->options & wordBreak),
			&endPos, &realW, &realH);
		if (!(ctx->options & wordBreak) && endPos < ctx->lastPos) {
		    endPos = (*(ctx->source->scan))(ctx->source, startPos,
			    XtstEOL, XtsdRight, 1, TRUE);
		    if (endPos == startPos)
			endPos = ctx->lastPos + 1;
		}
		ctx->lt.info[line].endX = realW + x;
		startPos = endPos;
	    }
	    else ctx->lt.info[line].endX = x;
	    y = y + realH;
	}
    }
}

/*
 * This routine is used to re-display the entire window, independent of
 * its current state.
*/
void ForceBuildLineTable(ctx)
    TextContext *ctx;
{
    XtTextPosition position;

    position = ctx->lt.top;
    ctx->lt.top++; /* ugly, but it works */
    BuildLineTable(ctx, position);
}

/*
 * This routine is used by Text to notify an associated scrollbar of the
 * correct metrics (position and shown fraction) for the text being currently
 * displayed in the window.
*/
static void SetScrollBar(ctx)
    TextContext *ctx;
{
    float   first, last;
    if (ctx->sbar) {
	if ((ctx->lastPos > 0)  &&  (ctx->lt.lines > 0)) {
	    first = ctx->lt.top;
	    first /= ctx->lastPos; 
					/* Just an approximation */
	    last = ctx->lt.info[ctx->lt.lines].position;
	    last /= ctx->lastPos;
	}
	else {
	    first = 0.0;
	    last = 1.0;
	}
	XtScrollBarSetThumb(ctx->dpy, ctx->sbar, first, last - first);
    }
}


/*
 * The routine will scroll the displayed text by lines.  If the arg  is
 * positive, move up; otherwise, move down. [note: this is really a private
 * procedure but is used in multiple modules].
*/
_XtTextScroll(ctx, n)
  TextContext *ctx;
  int n;
{
    XtTextPosition top, target;
    if (n >= 0) {
	top = min(ctx->lt.info[n].position, ctx->lastPos);
	BuildLineTable(ctx, top);
	if (top >= ctx->lastPos)
	    DisplayTextWindow(ctx);
	else {
	    XCopyArea(ctx->dpy, ctx->w, ctx->w, ctx->gc,
		      0, ctx->lt.info[n].y,
		      9999, (Dimension)ctx->height - ctx->lt.info[n].y,
		      0, ctx->lt.info[0].y);
	    (*(ctx->sink->clearToBackground))(ctx->sink, ctx->w, 0,
		ctx->lt.info[0].y + ctx->height - ctx->lt.info[n].y,
		9999, 9999);
	    if (n < ctx->lt.lines) n++;
	    _XtTextNeedsUpdating(ctx,
		    ctx->lt.info[ctx->lt.lines - n].position, ctx->lastPos);
	    SetScrollBar(ctx);
	}
    } else {
	Dimension tempHeight;
	n = -n;
	target = ctx->lt.top;
	top = (*(ctx->source->scan))(ctx->source, target, XtstEOL,
				     XtsdLeft, n+1, FALSE);
	tempHeight = ctx->lt.info[ctx->lt.lines-n].y;
	BuildLineTable(ctx, top);
	if (ctx->lt.info[n].position == target) {
	    XCopyArea(ctx->dpy, ctx->w, ctx->w, ctx->gc,
		      0, ctx->lt.info[0].y, 9999, tempHeight,
		      0, ctx->lt.info[n].y);
	    _XtTextNeedsUpdating(ctx, 
		    ctx->lt.info[0].position, ctx->lt.info[n].position);
	    SetScrollBar(ctx);
	} else if (ctx->lt.top != target) DisplayTextWindow(ctx);
    }
}

/*
 * The routine will scroll the displayed text by pixels.  If the arg is
 * positive, move up; otherwise, move down.
*/
/*ARGSUSED*/ /* keep lint happy */
static int ScrollUpDownProc (dpy, sbar, w, pix)
  Display *dpy;
  Window sbar, w;
  int pix;
{
    TextContextPtr ctx;
    int     apix, line;
    if (XFindContext(dpy, w, textContext, (caddr_t *)&ctx))
	return;
    _XtTextPrepareToUpdate(ctx);
    apix = abs(pix);
    for (line = 1;
	    line < ctx->lt.lines && apix > ctx->lt.info[line + 1].y;
	    line++);
    if (pix >= 0)
	_XtTextScroll(ctx, line);
    else
	_XtTextScroll(ctx, -line);
    _XtTextExecuteUpdate(ctx);
}

/*
 * The routine "thumbs" the displayed text. Thumbing means reposition the
 * displayed view of the source to a new position determined by a fraction
 * of the way from beginning to end. Ideally, this should be determined by
 * the number of displayable lines in the source. This routine does it as a
 * fraction of the first position and last position and then normalizes to
 * the start of the line containing the position.
*/
/*ARGSUSED*/ /* keep lint happy */
static int ThumbProc (dpy, sbar, w, where)
  Display *dpy;
  Window sbar, w;
  float where;
  /* BUG/deficiency: The normalize to line portion of this routine will
   * cause thumbing to always position to the start of the source.
   */
{
  
    TextContextPtr ctx;
    XtTextPosition position;
    if (XFindContext(dpy, w, textContext, (caddr_t *)&ctx))
	return;
    _XtTextPrepareToUpdate(ctx);
    position = where * ctx->lastPos;
    position = (*(ctx->source->scan))(ctx->source, position, XtstEOL, XtsdLeft,
	    1, FALSE);
    BuildLineTable(ctx, position);
    DisplayTextWindow(ctx);
    _XtTextExecuteUpdate(ctx);
}


int _XtTextSetNewSelection(ctx, left, right)
  TextContext *ctx;
  XtTextPosition left, right;
{
    XtTextPosition pos;

    if (left < ctx->s.left) {
	pos = min(right, ctx->s.left);
	_XtTextNeedsUpdating(ctx, left, pos);
    }
    if (left > ctx->s.left) {
	pos = min(left, ctx->s.right);
	_XtTextNeedsUpdating(ctx, ctx->s.left, pos);
    }
    if (right < ctx->s.right) {
	pos = max(right, ctx->s.left);
	_XtTextNeedsUpdating(ctx, pos, ctx->s.right);
    }
    if (right > ctx->s.right) {
	pos = max(left, ctx->s.right);
	_XtTextNeedsUpdating(ctx, pos, right);
    }

    ctx->s.left = left;
    ctx->s.right = right;
}



/*
 * This internal routine deletes the text from pos1 to pos2 in a source and
 * then inserts, at pos1, the text that was passed. As a side effect it
 * "invalidates" that portion of the displayed text (if any).
*/
int ReplaceText (ctx, pos1, pos2, text)
  TextContext *ctx;
  XtTextPosition pos1, pos2;
  XtTextBlock *text;

 /* it is illegal to call this routine unless there is a valid line table!*/
{
    int i, line1, line2, visible, delta, error;
    Position x, y;
    Dimension realW, realH, width;
    XtTextPosition startPos, endPos, updateFrom;

    /* the insertPos may not always be set to the right spot in XttextAppend */
    if ((pos1 == ctx->insertPos) && 
        ((*(ctx->source->editType))(ctx->source) == XttextAppend)) {
      ctx->insertPos = GETLASTPOS;
      pos2 = pos2 - pos1 + ctx->insertPos;
      pos1 = ctx->insertPos;
    }
    updateFrom = (*(ctx->source->scan))(ctx->source, pos1, XtstWhiteSpace, XtsdLeft,
	    1, TRUE);
    updateFrom = (*(ctx->source->scan))(ctx->source, updateFrom, XtstPositions, XtsdLeft,
	    1, TRUE);
    startPos = max(updateFrom, ctx->lt.top);
    visible = LineAndXYForPosition(ctx, startPos, &line1, &x, &y);
    error = (*(ctx->source->replace))(ctx->source, pos1, pos2, text, &delta);
    if (error) return error;
    ctx->lastPos = GETLASTPOS;
    if (ctx->lt.top >= ctx->lastPos) {
	BuildLineTable(ctx, ctx->lastPos);
	ClearWindow(ctx);
	SetScrollBar(ctx);
	return error;
    }
    if (delta < ctx->lastPos) {
	for (i = 0; i < ctx->numranges; i++) {
	    if (ctx->updateFrom[i] > pos1)
		ctx->updateFrom[i] += delta;
	    if (ctx->updateTo[i] >= pos1)
		ctx->updateTo[i] += delta;
	}
    }

    line2 = LineForPosition(ctx, pos1);
    /* 
     * fixup all current line table entries to reflect edit.
     * BUG: it is illegal to do arithmetic on positions. This code should
     * either use scan or the source needs to provide a function for doing
     * position arithmetic.
    */
    for (i = line2 + 1; i <= ctx->lt.lines; i++)
	ctx->lt.info[i].position += delta;

    endPos = pos1;
    /*
     * Now process the line table and fixup in case edits caused
     * changes in line breaks. If we are breaking on word boundaries,
     * this code checks for moving words to and from lines.
    */
    if (visible) {
	for (i = line1; i < ctx->lt.lines; i++) {/* fixup line table */
	    width = (ctx->options & resizeWidth) ? 9999 : ctx->width - x;
	    if (startPos <= ctx->lastPos) {
		(*(ctx->sink->findPosition))(ctx->sink, 
                        ctx->source, startPos, x,
			width, (ctx->options & wordBreak),
			&endPos, &realW, &realH);
		if (!(ctx->options & wordBreak) && endPos < ctx->lastPos) {
		    endPos = (*(ctx->source->scan))(ctx->source, startPos,
			    XtstEOL, XtsdRight, 1, TRUE);
		    if (endPos == startPos)
			endPos = ctx->lastPos + 1;
		}
		ctx->lt.info[i].endX = realW + x;
		ctx->lt.info[i + 1].y = realH + ctx->lt.info[i].y;
		if ((endPos > pos1) &&
			(endPos == ctx->lt.info[i + 1].position))
		    break;
		startPos = endPos;
	    }
	    if (startPos > ctx->lastPos)
		ctx->lt.info[i + 1].endX = ctx->leftmargin;
	    ctx->lt.info[i + 1].position = startPos;
	    x = ctx->lt.info[i + 1].x;
	}
    }
    if (delta >= ctx->lastPos)
	endPos = ctx->lastPos;
    if (delta >= ctx->lastPos || pos2 >= ctx->lt.top)
	_XtTextNeedsUpdating(ctx, updateFrom, endPos);
    SetScrollBar(ctx);
    return error;
}


/*
 * This routine will display text between two arbitrary source positions.
 * In the event that this span contains highlighted text for the selection, 
 * only that portion will be displayed highlighted.
 */
static void DisplayText(ctx, pos1, pos2)
  TextContext *ctx;
  XtTextPosition pos1, pos2;
  /* it is illegal to call this routine unless there is a valid line table!*/
{
    Position x, y;
    Dimension height;
    int line, i, visible;
    XtTextPosition startPos, endPos;

    if (pos1 < ctx->lt.top)
	pos1 = ctx->lt.top;
    if (pos2 > ctx->lastPos) 
	pos2 = ctx->lastPos;
    if (pos1 >= pos2) return;
    visible = LineAndXYForPosition(ctx, pos1, &line, &x, &y);
    if (!visible)
	return;
    startPos = pos1;
    height = ctx->lt.info[1].y - ctx->lt.info[0].y;
    for (i = line; i < ctx->lt.lines; i++) {
	endPos = ctx->lt.info[i + 1].position;
	if (endPos > pos2)
	    endPos = pos2;
	if (endPos > startPos) {
	    if (x == ctx->leftmargin)
                (*(ctx->sink->clearToBackground))(ctx->sink, ctx->w,
	             0, y, ctx->leftmargin, height);
	    if (startPos >= ctx->s.right || endPos <= ctx->s.left) {
		(*(ctx->sink->display))(ctx->sink, ctx->source, ctx->w, x, y,
			startPos, endPos, FALSE);
	    } else if (startPos >= ctx->s.left && endPos <= ctx->s.right) {
		(*(ctx->sink->display))(ctx->sink, ctx->source, ctx->w, x, y,
			startPos, endPos, TRUE);
	    } else {
		DisplayText(ctx, startPos, ctx->s.left);
		DisplayText(ctx, max(startPos, ctx->s.left), 
			min(endPos, ctx->s.right));
		DisplayText(ctx, ctx->s.right, endPos);
	    }
	}
	startPos = endPos;
	height = ctx->lt.info[i + 1].y - ctx->lt.info[i].y;
        (*(ctx->sink->clearToBackground))(ctx->sink, ctx->w,
	    ctx->lt.info[i].endX, y, 999, height);
	x = ctx->leftmargin;
	y = ctx->lt.info[i + 1].y;
	if ((endPos == pos2) && (endPos != ctx->lastPos))
	    break;
    }
}

/*
 * This routine implements multi-click selection in a hardwired manner.
 * It supports multi-click entity cycling (char, word, line, file) and mouse
 * motion adjustment of the selected entitie (i.e. select a word then, with
 * button still down, adjust wich word you really meant by moving the mouse).
 * [NOTE: This routine is to be replaced by a set of procedures that
 * will allows clients to implements a wide class of draw through and
 * multi-click selection user interfaces.]
*/
static void DoSelection (ctx, position, time, motion)
  TextContext *ctx;
  XtTextPosition position;
  unsigned short time;
  Boolean motion;
{
    int     delta;
    XtTextPosition newLeft, newRight;
    XtSelectType newType;
    XtSelectType *sarray;

    delta = (time < ctx->lasttime) ?
	ctx->lasttime - time : time - ctx->lasttime;
    if (motion)
	newType = ctx->s.type;
    else {
	if ((delta < 500) && ((position >= ctx->s.left)
		    && (position <= ctx->s.right))) { /* multi-click event */
	    sarray = ctx->sarray;
	    for (sarray = ctx->sarray;
		*sarray != XtselectNull && *sarray != ctx->s.type;
		sarray++) ;
	    if (*sarray != XtselectNull) sarray++;
	    if (*sarray == XtselectNull) sarray = ctx->sarray;
	    newType = *sarray;
	} else {			/* single-click event */
	    newType = *(ctx->sarray);
	}
        ctx->lasttime = time;
    }
    switch (newType) {
	case XtselectPosition: 
            newLeft = newRight = position;
	    break;
	case XtselectChar: 
            newLeft = position;
            newRight = (*(ctx->source->scan))(
                    ctx->source, position, position, XtsdRight, 1, FALSE);
	    break;
	case XtselectWord: 
	    newLeft = (*(ctx->source->scan))(
		    ctx->source, position, XtstWhiteSpace, XtsdLeft, 1, FALSE);
	    newRight = (*(ctx->source->scan))(
		    ctx->source, position, XtstWhiteSpace, XtsdRight, 1, FALSE);
	    break;
	case XtselectLine: 
	case XtselectParagraph:  /* need "para" scan mode to implement pargraph */
 	    newLeft = (*(ctx->source->scan))(
		    ctx->source, position, XtstEOL, XtsdLeft, 1, FALSE);
	    newRight = (*(ctx->source->scan))(
		    ctx->source, position, XtstEOL, XtsdRight, 1, FALSE);
	    break;
	case XtselectAll: 
	    newLeft = (*(ctx->source->scan))(
		    ctx->source, position, XtstFile, XtsdLeft, 1, FALSE);
	    newRight = (*(ctx->source->scan))(
		    ctx->source, position, XtstFile, XtsdRight, 1, FALSE);
	    break;
    }
    if ((newLeft != ctx->s.left) || (newRight != ctx->s.right)
	    || (newType != ctx->s.type)) {
	_XtTextSetNewSelection(ctx, newLeft, newRight);
	ctx->s.type = newType;
	if (position - ctx->s.left < ctx->s.right - position)
	    ctx->insertPos = newLeft;
	else 
	    ctx->insertPos = newRight;
    }
    if (!motion) { /* setup so we can freely mix select extend calls*/
	ctx->origSel.type = ctx->s.type;
	ctx->origSel.left = ctx->s.left;
	ctx->origSel.right = ctx->s.right;
	if (position >= ctx->s.left + ((ctx->s.right - ctx->s.left) / 2))
	    ctx->extendDir = XtsdRight;
	else
	    ctx->extendDir = XtsdLeft;
    }
}

/*
 * This routine implements extension of the currently selected text in
 * the "current" mode (i.e. char word, line, etc.). It worries about
 * extending from either end of the selection and handles the case when you
 * cross through the "center" of the current selection (e.g. switch which
 * end you are extending!).
 * [NOTE: This routine will be replaced by a set of procedures that
 * will allows clients to implements a wide class of draw through and
 * multi-click selection user interfaces.]
*/
static void ExtendSelection (ctx, position, motion)
  TextContext *ctx;
  XtTextPosition position;
  Boolean motion;
{
    XtTextPosition newLeft, newRight;
	

    if (!motion) {		/* setup for extending selection */
	ctx->origSel.type = ctx->s.type;
	ctx->origSel.left = ctx->s.left;
	ctx->origSel.right = ctx->s.right;
	if (position >= ctx->s.left + ((ctx->s.right - ctx->s.left) / 2))
	    ctx->extendDir = XtsdRight;
	else
	    ctx->extendDir = XtsdLeft;
    }
    else /* check for change in extend direction */
	if ((ctx->extendDir == XtsdRight && position < ctx->origSel.left) ||
		(ctx->extendDir == XtsdLeft && position > ctx->origSel.right)) {
	    ctx->extendDir = (ctx->extendDir == XtsdRight)? XtsdLeft : XtsdRight;
	    _XtTextSetNewSelection(ctx, ctx->origSel.left, ctx->origSel.right);
	}
    newLeft = ctx->s.left;
    newRight = ctx->s.right;
    switch (ctx->s.type) {
	case XtselectPosition: 
	    if (ctx->extendDir == XtsdRight)
		newRight = position;
	    else
		newLeft = position;
	    break;
	case XtselectWord: 
	    if (ctx->extendDir == XtsdRight)
		newRight = position = (*(ctx->source->scan))(
			ctx->source, position, XtstWhiteSpace, XtsdRight, 1, FALSE);
	    else
		newLeft = position = (*(ctx->source->scan))(
			ctx->source, position, XtstWhiteSpace, XtsdLeft, 1, FALSE);
	    break;
        case XtselectLine:
	case XtselectParagraph: /* need "para" scan mode to implement pargraph */
	    if (ctx->extendDir == XtsdRight)
		newRight = position = (*(ctx->source->scan))(
			ctx->source, position, XtstEOL, XtsdRight, 1, TRUE);
	    else
		newLeft = position = (*(ctx->source->scan))(
			ctx->source, position, XtstEOL, XtsdLeft, 1, FALSE);
	    break;
	case XtselectAll: 
	    position = ctx->insertPos;
	    break;
    }
    _XtTextSetNewSelection(ctx, newLeft, newRight);
    ctx->insertPos = position;
}


/*
 * Clear the window to background color.
*/
static ClearWindow (ctx)
  TextContext *ctx;
{
    (*(ctx->sink->clearToBackground))(ctx->sink, ctx->w, 0, 0, ctx->width,
				      ctx->height);
}


/*
 * Internal redisplay entire window.
*/
DisplayTextWindow (ctx)
  TextContext *ctx;
{
    ClearWindow(ctx);
    BuildLineTable(ctx, ctx->lt.top);
    _XtTextNeedsUpdating(ctx, zeroPosition, ctx->lastPos);
    SetScrollBar(ctx);
}

/*
 * This routine checks to see if the window should be resized (grown or
 * shrunk) or scrolled then text to be painted overflows to the right or
 * the bottom of the window. It is used by the keyboard input routine.
*/
CheckResizeOrOverflow(ctx)
  TextContext *ctx;
{
    XtTextPosition posToCheck;
    int     visible, line, width;
    WindowBox rbox, abox;
    if (ctx->options & resizeWidth) {
	width = 0;
	for (line=0 ; line<ctx->lt.lines ; line++)
	    if (width < ctx->lt.info[line].endX)
		width = ctx->lt.info[line].endX;
	width += ctx->leftmargin;
	if (width > ctx->width) {
	    rbox.x = rbox.y = 0;
	    rbox.width = width;
	    rbox.height = ctx->height;
	    (void) XtMakeGeometryRequest(ctx->dpy, ctx->w, XtgeometryResize, &rbox, &abox);
	}
    }
    if ((ctx->options & resizeHeight) || (ctx->options & scrollOnOverflow)) {
	if (ctx->options & scrollOnOverflow)
	    posToCheck = ctx->insertPos;
	else
	    posToCheck = ctx->lastPos;
	visible = IsPositionVisible(ctx, posToCheck);
	if (visible)
	    line = LineForPosition(ctx, posToCheck);
	else
	    line = ctx->lt.lines;
	if ((ctx->options & scrollOnOverflow) && (line + 1 > ctx->lt.lines)) {
	    BuildLineTable(ctx, ctx->lt.info[1].position);
	    XCopyArea(ctx->dpy, ctx->w, ctx->w, ctx->gc,
		      (Position)ctx->leftmargin, ctx->lt.info[1].y, 9999, 9999,
		      (Position)ctx->leftmargin, ctx->lt.info[0].y);
	}
	else
	    if ((ctx->options & resizeHeight) && (line + 1 != ctx->lt.lines)) {
		rbox.x = 0;
		rbox.y = 0;
		rbox.width = ctx->width;
		rbox.height = (*(ctx->sink->maxHeight))
					(ctx->sink, line + 1) + (2*yMargin)+2;
		(void) XtMakeGeometryRequest(ctx->dpy, ctx->w, XtgeometryResize, &rbox, &abox);
	    }
    }
}

/* 
 * This routine processes all keyboard, button and mouse XEvents. It is
 * responsible for performing any key/button to function mapping (with the
 * help of the translation manager) as well as doing any edits of editable
 * sources. 
 */
static void ProcessKeysAndButtons (event, ctx)
  XEvent *event;
  TextContext *ctx;
{
    Boolean     more;
    XtTextBlock text;
    XEvent ev;
    XtActionTokenPtr actionList;
    ActionProc proc;

    if (event->type == MotionNotify) {
	while (QLength(ctx->dpy)) {
	    XPeekEvent(ctx->dpy, &ev);
	    if (ev.type == MotionNotify) {
		XNextEvent(ctx->dpy, &ev);
		event = &ev;
	    } else break;
	}
    }
		
    ctx->time = event->xbutton.time;
    ctx->x = event->xbutton.x;
    ctx->y = event->xbutton.y;
    do {
	actionList = (XtActionTokenPtr) XtTranslateEvent(
		event, (TranslationPtr) ctx->state);
	if (actionList == NULL)
	    return;
	while (actionList != NULL) {
	    switch (actionList->type) {
		case XttokenAction: 
		    proc = (ActionProc) XtInterpretAction(ctx->dpy, 
			    (TranslationPtr) ctx->state, actionList->value.action);
		    (*(proc))(ctx);
		    break;
		case XttokenChar: 
		case XttokenString: 
		    if (actionList->type == XttokenString) {
			text.ptr = actionList->value.str;
			text.length = strlen(actionList->value.str);
		    }
		    else {
			text.ptr = &actionList->value.c;
			text.length = 1;
		    }
		    text.firstPos = 0;
		    if (ReplaceText(ctx, ctx->insertPos, ctx->insertPos,
		                &text)) {
			XBell(ctx->dpy, 50);
			break;
		    }
		    ctx->insertPos =
			(*(ctx->source->scan))(ctx->source, ctx->insertPos,
			    XtstPositions, XtsdRight, text.length, TRUE);
		    _XtTextSetNewSelection(ctx,
			    ctx->insertPos, ctx->insertPos);
		    break;
	    }
	    actionList = actionList->next;
	}
	more = FALSE;
	if (QLength(ctx->dpy)) {
	    XPeekEvent(ctx->dpy, &ev);
	    if (ev.type == KeyPress) {
		XNextEvent(ctx->dpy, &ev);
		event = &ev;
		more = TRUE;
	    }
	}
    } while (more);
    CheckResizeOrOverflow(ctx);
}

/*
 * This routine is used to perform various selection functions. The goal is
 * to be able to specify all the more popular forms of draw-through and
 * multi-click selection user interfaces from the outside.
 */
void AlterSelection (ctx, mode, action)
    TextContext     *ctx;
    SelectionMode   mode;	/* {XtsmTextSelect, XtsmTextExtend}		  */
    SelectionAction action;	/* {XtactionStart, XtactionAdjust, XtactionEnd} */
{
    XtTextPosition position;
    char   *ptr;

    position = PositionForXY (ctx, (int) ctx->x, (int) ctx->y);
    if (action == XtactionStart) {
	switch (mode) {
	case XtsmTextSelect: 
	    DoSelection (ctx, position, ctx->time, FALSE);
	    break;
	case XtsmTextExtend: 
	    ExtendSelection (ctx, position, FALSE);
	    break;
	}
    }
    else {
	switch (mode) {
	case XtsmTextSelect: 
	    DoSelection (ctx, position, ctx->time, TRUE);
	    break;
	case XtsmTextExtend: 
	    ExtendSelection (ctx, position, TRUE);
	    break;
	}
    }
    if (action == XtactionEnd && ctx->s.left < ctx->s.right) {
	ptr = _XtTextGetText (ctx, ctx->s.left, ctx->s.right);
	XStoreBuffer (ctx->dpy, ptr, min (strlen (ptr), MAXCUT), 0);
	XtFree (ptr);
    }
}

/*
 * This routine processes all "expose region" XEvents. In general, its job
 * is to the best job at minimal re-paint of the text, displayed in the
 * window, that it can.
*/
static ProcessExposeRegion(event, ctx)
  XEvent *event;
  TextContextPtr ctx;
{
    XtTextPosition pos1, pos2, resultend;
    int line;
    int x = event->xexpose.x;
    int y = event->xexpose.y;
    int width = event->xexpose.width;
    int height = event->xexpose.height;
    LineTableEntryPtr info;
    if (x < ctx->leftmargin) /* stomp on caret tracks */
        (*(ctx->sink->clearToBackground))(ctx->sink, ctx->w, x, y,
					  width, height);
   /* figure out starting line that was exposed */
    line = LineForPosition(ctx, PositionForXY(ctx, x, y));
    while (line < ctx->lt.lines && ctx->lt.info[line + 1].y < y)
	line++;
    while (line < ctx->lt.lines) {
	info = &(ctx->lt.info[line]);
	if (info->y >= y + height)
	    break;
	(*(ctx->sink->resolve))(ctx->sink, ctx->source, 
                                info->position, info->x,
			        x - info->x, &pos1, &resultend);
	(*(ctx->sink->resolve))(ctx->sink, ctx->source, 
                                info->position, info->x,
			        x + width - info->x, &pos2, 
                                &resultend);
	pos2 = (*(ctx->source->scan))(ctx->source, pos2, XtstPositions, 
                                      XtsdRight, 1, TRUE);
	_XtTextNeedsUpdating(ctx, pos1, pos2);
	line++;
    }
}


static int oldinsert = -1;

/*
 * This routine does all setup required to syncronize batched screen updates
*/
int _XtTextPrepareToUpdate(ctx)
  TextContext *ctx;
{
    if (oldinsert < 0) {
	InsertCursor(ctx, XtisOff);
	ctx->numranges = 0;
	ctx->showposition = FALSE;
	oldinsert = ctx->insertPos;
    }
}


/*
 * This is a private utility routine used by _XtTextExecuteUpdate. It
 * processes all the outstanding update requests and merges update
 * ranges where possible.
*/
static FlushUpdate(ctx)
  TextContext *ctx;
{
    int     i, w;
    XtTextPosition updateFrom, updateTo;
    while (ctx->numranges > 0) {
	updateFrom = ctx->updateFrom[0];
	w = 0;
	for (i=1 ; i<ctx->numranges ; i++) {
	    if (ctx->updateFrom[i] < updateFrom) {
		updateFrom = ctx->updateFrom[i];
		w = i;
	    }
	}
	updateTo = ctx->updateTo[w];
	ctx->numranges--;
	ctx->updateFrom[w] = ctx->updateFrom[ctx->numranges];
	ctx->updateTo[w] = ctx->updateTo[ctx->numranges];
	for (i=ctx->numranges-1 ; i>=0 ; i--) {
	    while (ctx->updateFrom[i] <= updateTo && i < ctx->numranges) {
		updateTo = ctx->updateTo[i];
		ctx->numranges--;
		ctx->updateFrom[i] = ctx->updateFrom[ctx->numranges];
		ctx->updateTo[i] = ctx->updateTo[ctx->numranges];
	    }
	}
	DisplayText(ctx, updateFrom, updateTo);
    }
}


/*
 * This is a private utility routine used by _XtTextExecuteUpdate. This routine
 * worries about edits causing new data or the insertion point becoming
 * invisible (off the screen). Currently it always makes it visible by
 * scrolling. It probably needs generalization to allow more options.
*/
_XtTextShowPosition(ctx)
  TextContext *ctx;
{
    XtTextPosition top, first, second;
    if (ctx->insertPos < ctx->lt.top ||
		ctx->insertPos >= ctx->lt.info[ctx->lt.lines].position) {
	if (ctx->lt.lines > 0 && (ctx->insertPos < ctx->lt.top ||
		ctx->lt.info[ctx->lt.lines].position <= ctx->lastPos)) {
	    first = ctx->lt.top;
	    second = ctx->lt.info[1].position;
	    if (ctx->insertPos < first)
		top = (*(ctx->source->scan))(ctx->source, ctx->insertPos, XtstEOL,
			XtsdLeft, 1, FALSE);
	    else
		top = (*(ctx->source->scan))(ctx->source, ctx->insertPos, XtstEOL,
			XtsdLeft, ctx->lt.lines, FALSE);
	    BuildLineTable(ctx, top);
	    while (ctx->insertPos >= ctx->lt.info[ctx->lt.lines].position) {
		if (ctx->lt.info[ctx->lt.lines].position > ctx->lastPos)
		    break;
		BuildLineTable(ctx, ctx->lt.info[1].position);
	    }
	    if (ctx->lt.top == second) {
	        BuildLineTable(ctx, first);
		_XtTextScroll(ctx, 1);
	    } else if (ctx->lt.info[1].position == first) {
		BuildLineTable(ctx, first);
		_XtTextScroll(ctx, -1);
	    } else {
		ctx->numranges = 0;
		if (ctx->lt.top != first)
		    DisplayTextWindow(ctx);
	    }
	}
    }
}



/*
 * This routine causes all batched screen updates to be performed
*/
_XtTextExecuteUpdate(ctx)
  TextContext *ctx;
{
    if (oldinsert >= 0) {
	if (oldinsert != ctx->insertPos || ctx->showposition)
	    _XtTextShowPosition(ctx);
	FlushUpdate(ctx);
	InsertCursor(ctx, XtisOn);
	oldinsert = -1;
    }
}


static HandleDestroyNotify(ctx)
TextContext *ctx;
{
    if (ctx->dialog)
	(void) XtSendDestroyNotify(ctx->dpy, ctx->dialog);
    (void) XDeleteContext(ctx->dpy, ctx->w, textContext);
    if (ctx->outer)
	(void) XDeleteContext(ctx->dpy, ctx->outer, textContext);
    XtFree((char *)ctx->updateFrom);
    XtFree((char *)ctx->updateTo);
    XtFree((char *)ctx);
}


/*
 * This is the main routine for handling all selected XEvents. It is normally
 * the routine to be registered with the Xtoolkit Event dispatcher. It is
 * currently private and should probably be made public to allow interposition
 * programming techniques
*/
static XtEventReturnCode ProcessTextEvent(event, eventdata)
  XEvent *event;
  caddr_t eventdata;
{
    TextContext *ctx = (TextContext *) eventdata;
    XtEventReturnCode returnCode;

    if (event->type != DestroyNotify && event->type != FocusIn &&
	    event->type != FocusOut && event->type != EnterNotify &&
	    event->type != LeaveNotify)
	_XtTextPrepareToUpdate(ctx);
    returnCode = XteventHandled;
    switch (event->type) {
	case ButtonPress:
	    if (!ctx->hasfocus)
		XSetInputFocus(ctx->dpy, ctx->w, RevertToPointerRoot,
			       CurrentTime);
	    /* Fall through. */
	case ButtonRelease: 
	case MotionNotify: 
	case KeyPress:
	    ProcessKeysAndButtons(event, ctx);
	    break;
	case ConfigureNotify:
	    ctx->width = event->xconfigure.width;
	    ctx->height = event->xconfigure.height;
	    ForceBuildLineTable(ctx);
	    break;
	case Expose:
	case GraphicsExpose:
	    ProcessExposeRegion(event, ctx);
	    break;
	case DestroyNotify:
	    HandleDestroyNotify(ctx);
	    return XteventHandled; /* Avoid the call to _XtTextExecuteUpdate!*/
	case FocusIn:
	    ctx->hasfocus = TRUE;
	    return(returnCode);
	case FocusOut:
	    ctx->hasfocus = FALSE;
	    return(returnCode);
	case EnterNotify:
	case LeaveNotify:
	    ctx->hasfocus = event->xcrossing.focus;
	    return(returnCode);
	default: 
	    returnCode = XteventNotHandled;
    }
    _XtTextExecuteUpdate(ctx);
    return(returnCode);
}

/* Public routines */

Window XtTextCreate(dpy, parent, args, argCount)
    Display  *dpy;
    Window   parent;
    ArgList  args;
    int      argCount;
{
    TextContext *ctx;
    XrmNameList 	names;
    XrmClassList classes;

    /* stuff for scroll bars */
    static Arg scrollMgrArgs[] = {
	{XtNwindow, NULL},
    };

    static Arg scrollBarArgs[] = {
	{XtNvalue, NULL},
	{XtNorientation, (XtArgVal) XtorientVertical},
	{XtNscrollUpDownProc, (XtArgVal)ScrollUpDownProc},
	{XtNthumbProc, (XtArgVal)ThumbProc},
    };


    if (!initialized) TextInitialize();

    ctx = (TextContext *) XtMalloc(sizeof(TextContext));
    glob = globinit;
    glob.dpy = dpy;
    XtGetResources(dpy, resources, XtNumber(resources), args, argCount, parent,
		   "text", "Text", &names, &classes);
    *ctx = glob;
    ctx->state = XtSetTextEventBindings(ctx->dpy, ctx->eventTable);
    ctx->lastPos = GETLASTPOS;
    ctx->updateFrom = (XtTextPosition *) XtMalloc(1);
    ctx->updateTo = (XtTextPosition *) XtMalloc(1);
    ctx->numranges = ctx->maxranges = 0;
    if (ctx->height == 0)
	ctx->height = (*(ctx->sink->maxHeight))(ctx->sink, 1) + (2*yMargin) +2;
    if (ctx->w == NULL)
	ctx->w = XCreateSimpleWindow(ctx->dpy, parent, 0, 0,
			 ctx->width, ctx->height,
			 ctx->borderWidth, ctx->border, ctx->background);
    /*
     * Note that if the client passed a window, no checks are made to
     * ensure the size, border, borderWidth and background are consistent
     */
    XtSetNameAndClass(ctx->dpy, ctx->w, names, classes);
    XrmFreeNameList(names);
    XrmFreeClassList(classes);

    if (ctx->options & scrollVertical) {
	scrollMgrArgs[0].value =
	    scrollBarArgs[0].value = (caddr_t)(ctx->w);
	ctx->outer = 
	   XtScrollMgrCreate(ctx->dpy, parent, scrollMgrArgs, XtNumber(scrollMgrArgs));
	ctx->w = XtScrollMgrGetChild(ctx->dpy, ctx->outer);
	ctx->sbar =
	    XtScrollMgrAddBar(ctx->dpy, ctx->outer, scrollBarArgs, XtNumber(scrollBarArgs));
	XMapSubwindows(ctx->dpy, ctx->outer);
	(void) XSaveContext(ctx->dpy, ctx->outer, textContext, (caddr_t)ctx);
	(void) XtSetEventHandler(ctx->dpy, ctx->outer, ProcessTextEvent,
	 StructureNotifyMask, (caddr_t) ctx);
    }
    (void) XSaveContext(ctx->dpy, ctx->w, textContext, (caddr_t)ctx);
    (void) XtSetEventHandler(ctx->dpy, ctx->w, ProcessTextEvent,
			ButtonPressMask | ButtonReleaseMask | ButtonMotionMask
			| KeyPressMask | ExposureMask | StructureNotifyMask
			| FocusChangeMask | EnterWindowMask | LeaveWindowMask,
			(caddr_t) ctx);
    ctx->gc = DefaultGC(ctx->dpy, DefaultScreen(ctx->dpy));
    BuildLineTable(ctx, ctx->lt.top);
    if (ctx->outer) return ctx->outer;
    else return ctx->w;
}


/*
 * This routine allow the application program to Get attributes.
 */

void XtTextGetValues(dpy, window, args, argCount)
Display *dpy;
Window window;
ArgList args;
int argCount;
{
    TextContext *ctx;
    if (!XFindContext(dpy, window, textContext, (caddr_t *)&ctx)) {
	glob = *ctx;
	XtGetValues(resources, XtNumber(resources), args, argCount);
    }
}


/*
 * This routine allow the application program to Set attributes.
 */

void XtTextSetValues(dpy, window, args, argCount)
Display *dpy;
Window window;
ArgList args;
int argCount;
{
    TextContext *ctx;
    Boolean    redisplay = FALSE;
    if (XFindContext(dpy, window, textContext, (caddr_t *)&ctx)) return;

    _XtTextPrepareToUpdate(ctx);
    glob = *ctx;
    XtSetValues(resources, XtNumber(resources), args, argCount);
    
    if (ctx->sink != glob.sink) {
	ctx->sink = glob.sink;
	redisplay = TRUE;
    }
    if (ctx->source != glob.source) {
        ctx->source = glob.source;
	ForceBuildLineTable(ctx);
	redisplay = TRUE;
    }
    if (ctx->insertPos != glob.insertPos) {
        ctx->insertPos = glob.insertPos;
	ctx->showposition = TRUE;
    }
    if (ctx->lt.top != glob.lt.top) {
	ctx->lt.top = glob.lt.top;
	redisplay = TRUE;
    }
    ctx->s = glob.s;
    ctx->options = glob.options;
    if (ctx->leftmargin != glob.leftmargin) {
	ctx->leftmargin = glob.leftmargin;
	redisplay = TRUE;
    }
    bcopy(glob.sarray, ctx->sarray, sizeof(SelectionArray));
    ctx->background = glob.background;

    if (redisplay) 
	DisplayTextWindow(ctx);
    _XtTextExecuteUpdate(ctx);
}



void XtTextDestroy(dpy, w)
    Display *dpy;
    Window w;
{
    TextContext *ctx;

    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx)) {
	HandleDestroyNotify(ctx);
	XDestroyWindow(ctx->dpy, w);
    }
}

void XtTextDisplay (dpy, w)
    Display *dpy;
    Window w;
{
    TextContext *ctx;

    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx)) {
	_XtTextPrepareToUpdate(ctx);
	DisplayTextWindow(ctx);
	_XtTextExecuteUpdate(ctx);
    }
}

/*******************************************************************
The following routines provide procedural interfaces to Text window state
setting and getting. They need to be redone so than the args code can use
them. I suggest we create a complete set that takes the context as an
argument and then have the public version lookup the context and call the
internal one. The major value of this set is that they have actual application
clients and therefore the functionality provided is required for any future
version of Text.
********************************************************************/

void XtTextSetSelectionArray(dpy, w, sarray)
    Display *dpy;
    Window w;
    XtSelectType *sarray;
{
    TextContext *ctx;
    XtSelectType *s2;
    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx)) {
	s2 = ctx->sarray;
	while (XtselectNull != (*s2++ = *sarray++)) ;
    }
}

void XtTextSetLastPos (dpy, w, lastPos)
    Display *dpy;
  Window w;
  XtTextPosition lastPos;
{
    TextContext * ctx;

    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx)) {
	_XtTextPrepareToUpdate(ctx);
	(*(ctx->source->setLastPos))(ctx->source, lastPos);
	ctx->lastPos = GETLASTPOS;
	ForceBuildLineTable(ctx);
	DisplayTextWindow(ctx);
	_XtTextExecuteUpdate(ctx);
    }
}


void XtTextGetSelectionPos(dpy, w, left, right)
  Display *dpy;
  Window w;
  XtTextPosition *left, *right;
{
    TextContext *ctx;
    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx)){
	*left = ctx->s.left;
	*right = ctx->s.right;
    }
}


void XtTextNewSource(dpy, w, source, startPos)
    Display	   *dpy;
    Window 	   w;
    XtTextSource   *source;
    XtTextPosition startPos;
{
    TextContext *ctx;

    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx)) {
	ctx->source = source;
	ctx->lt.top = startPos;
	ctx->s.left = ctx->s.right = 0;
	ctx->insertPos = startPos;
	ctx->lastPos = GETLASTPOS;
	ForceBuildLineTable(ctx);
	_XtTextPrepareToUpdate(ctx);
	DisplayTextWindow(ctx);
	_XtTextExecuteUpdate(ctx);
    }
}

/*
 * This public routine deletes the text from startPos to endPos in a source and
 * then inserts, at startPos, the text that was passed. As a side effect it
 * "invalidates" that portion of the displayed text (if any), so that things
 * will be repainted properly.
 */
int XtTextReplace(dpy, w, startPos, endPos, text)
    Display	    *dpy;
    Window	    w;
    XtTextPosition  startPos, endPos;
    XtTextBlock     *text;
{
    TextContext *ctx;
    int result;
    result = EDITERROR;
    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx)) {
	_XtTextPrepareToUpdate(ctx);
	result = ReplaceText(ctx, startPos, endPos, text);
	_XtTextExecuteUpdate(ctx);
    }
    return result;
}


XtTextPosition XtTextTopPosition(dpy, w)
    Display *dpy;
    Window w;
{
    TextContext *ctx;

    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx))
        return ctx->lt.top;
    return 0;
}


void XtTextSetInsertionPoint(dpy, w, position)
    Display	   *dpy;
    Window 	   w;
    XtTextPosition position;
{
    TextContext *ctx;

    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx)) {
	_XtTextPrepareToUpdate(ctx);
	ctx->insertPos = position;
	ctx->showposition = TRUE;
	_XtTextExecuteUpdate(ctx);
    }
}


XtTextPosition XtTextGetInsertionPoint(dpy, w)
    Display	   *dpy;
    Window w;
{
    XtTextPosition position;
    TextContext *ctx;

    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx)) {
        position = ctx->insertPos;
    } else {
        position = 0;
    }
    return(position);
}


void XtTextUnsetSelection(dpy, w)
    Display	   *dpy;
    Window w;
{
    TextContext *ctx;

    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx)) {
	_XtTextPrepareToUpdate(ctx);
	_XtTextSetNewSelection(ctx, zeroPosition, zeroPosition);
	_XtTextExecuteUpdate(ctx);
    }
}


void XtTextChangeOptions(dpy, w, options)
    Display	   *dpy;
  Window w;
  int    options;
{
    TextContext *ctx;

    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx))
	ctx->options = options;
}


int XtTextGetOptions(dpy, w)
    Display	   *dpy;
  Window w;
{
    TextContext *ctx;

    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx))
	return ctx->options;
        else return 0;
}

void XtTextSetNewSelection (dpy, w, left, right)
    Display	   *dpy;
    Window 	   w;
    XtTextPosition left, right;
{
    TextContextPtr ctx;

    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx)){
	_XtTextPrepareToUpdate(ctx);
        _XtTextSetNewSelection(ctx, left, right);
	_XtTextExecuteUpdate(ctx);
    }
}

void XtTextInvalidate(dpy, w, from, to)
    Display	   *dpy;
    Window  	   w;
    XtTextPosition from,to;
{
    TextContextPtr ctx;

    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx)) {
        ctx->lastPos = (*(ctx->source->getLastPos))(ctx->source);
        _XtTextPrepareToUpdate(ctx);
        _XtTextNeedsUpdating(ctx, from, to);
        ForceBuildLineTable(ctx);
        _XtTextExecuteUpdate(ctx);
    }
}


/* Returns the window actually containing the text (which is not the same
   as the given window if the text window has scrollbars.) */

Window XtTextGetInnerWindow(dpy, w)
Display *dpy;
Window w;
{
    TextContextPtr ctx;
    if (!XFindContext(dpy, w, textContext, (caddr_t *)&ctx)) {
	return ctx->w;
    }
}
