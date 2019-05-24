#ifndef lint
static char rcsid[] = "$Header: Text.c,v 1.4 87/09/13 23:32:44 newman Exp $";
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

#include "Intrinsic.h"
#include "Text.h"
#include "TextP.h"
#include "Scroll.h"
#include "Atoms.h"
#include <sys/file.h>
#include <X11/Xutil.h>
#include <strings.h>

extern void bcopy();

#define BufMax 1000
#define abs(x)	(((x) < 0) ? (-(x)) : (x))
#define min(x,y)	((x) < (y) ? (x) : (y))
#define max(x,y)	((x) > (y) ? (x) : (y))
#define GETLASTPOS  (*(ctx->text.source->scan)) (ctx->text.source, 0, XtstFile, XtsdRight, 1, TRUE)
#define BUTTONMASK 0x143d

#define  yMargin 2
#define zeroPosition ((XtTextPosition) 0)

extern void BuildLineTable ();

/****************************************************************
 *
 * Full class record constant
 *
 ****************************************************************/

extern caddr_t defaultTextTranslations;

static XtResource resources[] = {
    {XtNtextOptions, XtCTextOptions, XrmRInt, sizeof (int),
        XtOffset(TextWidget, text.options), XrmRString, "0"},
    {XtNdisplayPosition, XtCTextPosition, XrmRInt,
	 sizeof (XtTextPosition), XtOffset(TextWidget, text.lt.top), 
	 XrmRString, "0"},
    {XtNinsertPosition, XtCTextPosition, XtRInt, 
        sizeof(XtTextPosition), XtOffset(TextWidget, text.insertPos), 
	XrmRString, "0"},
    {XtNleftMargin, XtCMargin, XrmRInt, sizeof (int), 
        XtOffset(TextWidget, text.leftmargin), XrmRString, "2"},
    {XtNselectionArray, XtCSelectionArray, XrmRPointer, 
        sizeof(SelectionArray), XtOffset(TextWidget, text.sarray), 
	XrmRPointer, NULL},
    {XtNtextSource, XtCTextSource, XrmRPointer, sizeof (caddr_t), 
         XtOffset(TextWidget, text.source), XrmRPointer, NULL},
    {XtNtextSink, XtCTextSink, XrmRPointer, sizeof (caddr_t),
         XtOffset(TextWidget, text.sink), XrmRPointer, NULL},
    {XtNselection, XtCSelection, XrmRPointer, sizeof(caddr_t),
	 XtOffset(TextWidget, text.s), XrmRPointer, NULL},
    {XtNeventBindings, XtCEventBindings, XtRStringTable, 
        sizeof(_XtTranslations), XtOffset(TextWidget, core.translations), 
	XtRStringTable, (caddr_t)&defaultTextTranslations},
};

  
static void Initialize(request, new)
 Widget request, new;
{
    TextWidget ctx = (TextWidget) new;

    if (ctx->core.width == 0)
	ctx->core.width = 200;
    if (ctx->core.height == 0)
	ctx->core.height = 200;

    ctx->text.lt.lines = 0;
    ctx->text.lt.info = NULL;
    ctx->text.s.left = ctx->text.s.right = 0;
    ctx->text.s.type = XtselectPosition;
    ctx->text.sbar = ctx->text.outer = NULL;
    ctx->text.sarray[0] = XtselectPosition;
    ctx->text.sarray[1] = XtselectWord;
    ctx->text.sarray[2] = XtselectLine;
    ctx->text.sarray[3] = XtselectParagraph;
    ctx->text.sarray[4] = XtselectAll;
    ctx->text.sarray[5] = XtselectNull;
    ctx->text.showposition = TRUE;
    ctx->text.lastPos = GETLASTPOS;
    ctx->text.dialog = NULL;
    ctx->text.updateFrom = (XtTextPosition *) XtMalloc(1);
    ctx->text.updateTo = (XtTextPosition *) XtMalloc(1);
    ctx->text.numranges = ctx->text.maxranges = 0;
    ctx->text.gc = DefaultGCOfScreen(XtScreen(ctx));
    BuildLineTable(ctx, ctx->text.lt.top);
/* what about this ugly scrollbar stuff?
    if (ctx->text.options & scrollVertical) {
	scrollMgrArgs[0].value =
	    scrollBarArgs[0].value = (caddr_t)(XtWindow(ctx));
	ctx->text.outer = 
	   XtScrollMgrCreate(ctx->core.display, parent, scrollMgrArgs, XtNumber(scrollMgrArgs));
	XtWindow(ctx) = XtScrollMgrGetChild(ctx->core.display, ctx->text.outer);
	ctx->text.sbar =
	    XtCreateWidget("scrollbar", scrollbarWidgetClass, ctx->text.outer,
	       scrollBarArgs, XtNumber(scrollBarArgs));
	XMapSubwindows(ctx->core.display, ctx->text.outer);
	(void) XtSetEventHandler(ctx->core.display, ctx->text.outer, ProcessTextEvent,
	 StructureNotifyMask, (caddr_t) ctx);
    }
*/
}

static void Realize( w, valueMask, attributes )
   Widget w;
   Mask valueMask;
   XSetWindowAttributes *attributes;
{
   TextWidget ctx = (TextWidget)w;
   if (ctx->text.sbar) XtRealizeWidget(ctx->text.sbar);
   ctx->core.window = 
          XCreateWindow(
                XtDisplay(w), w->core.parent->core.window,
                w->core.x, w->core.y,
                w->core.width, w->core.height, w->core.border_width,
                w->core.depth, InputOutput, (Visual *)CopyFromParent,
                valueMask, attributes);
}

/* Utility routines for support of Text */


/*
 * Procedure to manage insert cursor visibility for editable text.  It uses
 * the value of ctx->insertPos and an implicit argument. In the event that
 * position is immediately preceded by an eol graphic, then the insert cursor
 * is displayed at the beginning of the next line.
*/
static void InsertCursor (ctx, state)
  TextWidget ctx;
  InsertState state;
{
    Position x, y;
    int dy, line, visible;
    XtTextBlock text;

    if (ctx->text.lt.lines < 1) return;
    visible = LineAndXYForPosition(ctx, ctx->text.insertPos, &line, &x, &y);
    if (line < ctx->text.lt.lines)
	dy = (ctx->text.lt.info[line + 1].y - ctx->text.lt.info[line].y) + 1;
    else
	dy = (ctx->text.lt.info[line].y - ctx->text.lt.info[line - 1].y) + 1;

    /** If the insert position is just after eol then put it on next line **/
    if (x > ctx->text.leftmargin &&
	ctx->text.insertPos > 0 &&
	ctx->text.insertPos >= ctx->text.lastPos) {
	   /* reading the source is bogus and this code should use scan */
	   (*(ctx->text.source->read)) (ctx->text.source, ctx->text.insertPos - 1, &text, 1);
	   if (text.ptr[0] == '\n') {
	       x = ctx->text.leftmargin;
	       y += dy;
	   }
    }
    y += dy;
    if (visible)
	(*(ctx->text.sink->insertCursor))(ctx, x, y, state);
}


/*
 * Procedure to register a span of text that is no longer valid on the display
 * It is used to avoid a number of small, and potentially overlapping, screen
 * updates. [note: this is really a private procedure but is used in
 * multiple modules].
*/
_XtTextNeedsUpdating(ctx, left, right)
  TextWidget ctx;
  XtTextPosition left, right;
{
    int     i;
    if (left < right) {
	for (i = 0; i < ctx->text.numranges; i++) {
	    if (left <= ctx->text.updateTo[i] && right >= ctx->text.updateFrom[i]) {
		ctx->text.updateFrom[i] = min(left, ctx->text.updateFrom[i]);
		ctx->text.updateTo[i] = max(right, ctx->text.updateTo[i]);
		return;
	    }
	}
	ctx->text.numranges++;
	if (ctx->text.numranges > ctx->text.maxranges) {
	    ctx->text.maxranges = ctx->text.numranges;
	    i = ctx->text.maxranges * sizeof(XtTextPosition);
	    ctx->text.updateFrom = (XtTextPosition *) 
   	        XtRealloc((char *)ctx->text.updateFrom, (unsigned) i);
	    ctx->text.updateTo = (XtTextPosition *) 
		XtRealloc((char *)ctx->text.updateTo, (unsigned) i);
	}
	ctx->text.updateFrom[ctx->text.numranges - 1] = left;
	ctx->text.updateTo[ctx->text.numranges - 1] = right;
    }
}


/*
 * Procedure to read a span of text in Ascii form. This is purely a hack and
 * we probably need to add a function to sources to provide this functionality.
 * [note: this is really a private procedure but is used in multiple modules].
*/
char *_XtTextGetText(ctx, left, right)
  TextWidget ctx;
  XtTextPosition left, right;
{
    char   *result, *tempResult;
    int length, resultLength;
    XtTextBlock text;
    XtTextPosition end, nend;
    
    resultLength = right - left + 10;	/* Bogus? %%% */
    result = (char *)XtMalloc((unsigned) resultLength);
    end = (*(ctx->text.source->read))(ctx->text.source, left, &text, right - left);
    (void) strncpy(result, text.ptr, text.length);
    length = text.length;
    while (end < right) {
        nend = (*(ctx->text.source->read))(ctx->text.source, end, &text, right - end);
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
  TextWidget ctx;
  Position x,y;
{
 /* it is illegal to call this routine unless there is a valid line table! */
    int     width, fromx, line;
    XtTextPosition position, resultstart, resultend;

    /*** figure out what line it is on ***/
    for (line = 0; line < ctx->text.lt.lines - 1; line++) {
	if (y <= ctx->text.lt.info[line + 1].y)
	    break;
    }
    position = ctx->text.lt.info[line].position;
    if (position >= ctx->text.lastPos)
	return ctx->text.lastPos;
    fromx = ctx->text.lt.info[line].x;	/* starting x in line */
    width = x - fromx;			/* num of pix from starting of line */
    (*(ctx->text.sink->resolve)) (ctx, position, fromx, width,
	    &resultstart, &resultend);
    if (resultstart >= ctx->text.lt.info[line + 1].position)
	resultstart = (*(ctx->text.source->scan))(ctx->text.source,
		ctx->text.lt.info[line + 1].position, XtstPositions, XtsdLeft, 1, TRUE);
    return resultstart;
}

/*
 * This routine maps a source position in to the corresponding line number
 * of the text that is displayed in the window.
*/
int LineForPosition (ctx, position)
  TextWidget ctx;
  XtTextPosition position;
  /* it is illegal to call this routine unless there is a valid line table!*/
{
    int     line;

    if (position <= ctx->text.lt.info[0].position)
	return 0;
    for (line = 0; line < ctx->text.lt.lines; line++)
	if (position < ctx->text.lt.info[line + 1].position)
	    break;
    return line;
}

/*
 * This routine maps a source position into the corresponding line number
 * and the x, y coordinates of the text that is displayed in the window.
*/
static int LineAndXYForPosition (ctx, pos, line, x, y)
  TextWidget ctx;
  XtTextPosition pos;
  int *line;
  Position *x, *y;
  /* it is illegal to call this routine unless there is a valid line table!*/
{
    XtTextPosition linePos, endPos;
    int     visible, realW, realH;

    *line = 0;
    *x = ctx->text.leftmargin;
    *y = yMargin;
    visible = IsPositionVisible(ctx, pos);
    if (visible) {
	*line = LineForPosition(ctx, pos);
	*y = ctx->text.lt.info[*line].y;
	*x = ctx->text.lt.info[*line].x;
	linePos = ctx->text.lt.info[*line].position;
	(*(ctx->text.sink->findDistance))(ctx, linePos,
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
  TextWidget ctx;
  XtTextPosition position;
{
    Position x, y;
    Dimension width, realW, realH;
    int line, lines;
    XtTextPosition startPos, endPos;
    Boolean     rebuild;

    rebuild = (Boolean) (position != ctx->text.lt.top);
    lines = (*(ctx->text.sink->maxLines))(ctx, ctx->core.height);
    if (ctx->text.lt.info != NULL && lines != ctx->text.lt.lines) {
	XtFree((char *) ctx->text.lt.info);
	ctx->text.lt.info = NULL;
    }
    if (ctx->text.lt.info == NULL) {
	ctx->text.lt.info = (LineTableEntry *)
	    XtMalloc((unsigned)sizeof(LineTableEntry) * (lines + 1));
	for (line = 0; line < lines; line++) {
	    ctx->text.lt.info[line].position = 0;
	    ctx->text.lt.info[line].y = 0;
	}
	rebuild = TRUE;
    }
    else
	lines = ctx->text.lt.lines;
    if (rebuild) {
	ctx->text.lt.top = position;
	ctx->text.lt.lines = lines;
	startPos = position;
	y = yMargin;
	for (line = 0; line <= ctx->text.lt.lines; line++) {
	    x = ctx->text.leftmargin;
	    ctx->text.lt.info[line].x = x;
	    ctx->text.lt.info[line].y = y;
	    ctx->text.lt.info[line].position = startPos;
	    if (startPos <= ctx->text.lastPos) {
		width = (ctx->text.options & resizeWidth) ? 9999 : ctx->core.width - x;
		(*(ctx->text.sink->findPosition))(ctx, 
                        startPos, x,
			width, (ctx->text.options & wordBreak),
			&endPos, &realW, &realH);
		if (!(ctx->text.options & wordBreak) && endPos < ctx->text.lastPos) {
		    endPos = (*(ctx->text.source->scan))(ctx->text.source, startPos,
			    XtstEOL, XtsdRight, 1, TRUE);
		    if (endPos == startPos)
			endPos = ctx->text.lastPos + 1;
		}
		ctx->text.lt.info[line].endX = realW + x;
		startPos = endPos;
	    }
	    else ctx->text.lt.info[line].endX = x;
	    y = y + realH;
	}
    }
}

/*
 * This routine is used to re-display the entire window, independent of
 * its current state.
*/
void ForceBuildLineTable(ctx)
    TextWidget ctx;
{
    XtTextPosition position;

    position = ctx->text.lt.top;
    ctx->text.lt.top++; /* ugly, but it works */
    BuildLineTable(ctx, position);
}

/*
 * This routine is used by Text to notify an associated scrollbar of the
 * correct metrics (position and shown fraction) for the text being currently
 * displayed in the window.
*/
static void SetScrollBar(ctx)
    TextWidget ctx;
{
    float   first, last;
    if (ctx->text.sbar) {
	if ((ctx->text.lastPos > 0)  &&  (ctx->text.lt.lines > 0)) {
	    first = ctx->text.lt.top;
	    first /= ctx->text.lastPos; 
					/* Just an approximation */
	    last = ctx->text.lt.info[ctx->text.lt.lines].position;
	    last /= ctx->text.lastPos;
	}
	else {
	    first = 0.0;
	    last = 1.0;
	}
	XtScrollBarSetThumb(ctx->text.sbar, first, last - first);
    }
}


/*
 * The routine will scroll the displayed text by lines.  If the arg  is
 * positive, move up; otherwise, move down. [note: this is really a private
 * procedure but is used in multiple modules].
*/
_XtTextScroll(ctx, n)
  TextWidget ctx;
  int n;
{
    XtTextPosition top, target;
    if (n >= 0) {
	top = min(ctx->text.lt.info[n].position, ctx->text.lastPos);
	BuildLineTable(ctx, top);
	if (top >= ctx->text.lastPos)
	    DisplayTextWindow(ctx);
	else {
	    XCopyArea(XtDisplay(ctx), XtWindow(ctx), XtWindow(ctx), ctx->text.gc,
		      0, ctx->text.lt.info[n].y,
		      9999, (Dimension)ctx->core.height - ctx->text.lt.info[n].y,
		      0, ctx->text.lt.info[0].y);
	    (*(ctx->text.sink->clearToBackground))(ctx, 0,
		ctx->text.lt.info[0].y + ctx->core.height - ctx->text.lt.info[n].y,
		9999, 9999);
	    if (n < ctx->text.lt.lines) n++;
	    _XtTextNeedsUpdating(ctx,
		    ctx->text.lt.info[ctx->text.lt.lines - n].position, ctx->text.lastPos);
	    SetScrollBar(ctx);
	}
    } else {
	Dimension tempHeight;
	n = -n;
	target = ctx->text.lt.top;
	top = (*(ctx->text.source->scan))(ctx->text.source, target, XtstEOL,
				     XtsdLeft, n+1, FALSE);
	tempHeight = ctx->text.lt.info[ctx->text.lt.lines-n].y;
	BuildLineTable(ctx, top);
	if (ctx->text.lt.info[n].position == target) {
	    XCopyArea(XtDisplay(ctx), XtWindow(ctx), XtWindow(ctx), ctx->text.gc,
		      0, ctx->text.lt.info[0].y, 9999, tempHeight,
		      0, ctx->text.lt.info[n].y);
	    _XtTextNeedsUpdating(ctx, 
		    ctx->text.lt.info[0].position, ctx->text.lt.info[n].position);
	    SetScrollBar(ctx);
	} else if (ctx->text.lt.top != target) DisplayTextWindow(ctx);
    }
}

/*
 * The routine will scroll the displayed text by pixels.  If the arg is
 * positive, move up; otherwise, move down.
*/
/*ARGSUSED*/ /* keep lint happy */
static int ScrollUpDownProc (w, data, pix)
  Widget w;
  int data;
  int pix;
{
    TextWidget ctx = (TextWidget)w;
    int     apix, line;
    _XtTextPrepareToUpdate(ctx);
    apix = abs(pix);
    for (line = 1;
	    line < ctx->text.lt.lines && apix > ctx->text.lt.info[line + 1].y;
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
static int ThumbProc (w, data, where)
  Widget w;
  int data;
  float where;
  /* BUG/deficiency: The normalize to line portion of this routine will
   * cause thumbing to always position to the start of the source.
   */
{
  
    TextWidget ctx= (TextWidget)w;
    XtTextPosition position;
    _XtTextPrepareToUpdate(ctx);
    position = where * ctx->text.lastPos;
    position = (*(ctx->text.source->scan))(ctx->text.source, position, XtstEOL, XtsdLeft,
	    1, FALSE);
    BuildLineTable(ctx, position);
    DisplayTextWindow(ctx);
    _XtTextExecuteUpdate(ctx);
}


int _XtTextSetNewSelection(ctx, left, right)
  TextWidget ctx;
  XtTextPosition left, right;
{
    XtTextPosition pos;

    if (left < ctx->text.s.left) {
	pos = min(right, ctx->text.s.left);
	_XtTextNeedsUpdating(ctx, left, pos);
    }
    if (left > ctx->text.s.left) {
	pos = min(left, ctx->text.s.right);
	_XtTextNeedsUpdating(ctx, ctx->text.s.left, pos);
    }
    if (right < ctx->text.s.right) {
	pos = max(right, ctx->text.s.left);
	_XtTextNeedsUpdating(ctx, pos, ctx->text.s.right);
    }
    if (right > ctx->text.s.right) {
	pos = max(left, ctx->text.s.right);
	_XtTextNeedsUpdating(ctx, pos, right);
    }

    ctx->text.s.left = left;
    ctx->text.s.right = right;
}



/*
 * This internal routine deletes the text from pos1 to pos2 in a source and
 * then inserts, at pos1, the text that was passed. As a side effect it
 * "invalidates" that portion of the displayed text (if any).
*/
int ReplaceText (ctx, pos1, pos2, text)
  TextWidget ctx;
  XtTextPosition pos1, pos2;
  XtTextBlock *text;

 /* it is illegal to call this routine unless there is a valid line table!*/
{
    int i, line1, line2, visible, delta, error;
    Position x, y;
    Dimension realW, realH, width;
    XtTextPosition startPos, endPos, updateFrom;

    /* the insertPos may not always be set to the right spot in XttextAppend */
    if ((pos1 == ctx->text.insertPos) && 
        ((*(ctx->text.source->editType))(ctx->text.source) == XttextAppend)) {
      ctx->text.insertPos = GETLASTPOS;
      pos2 = pos2 - pos1 + ctx->text.insertPos;
      pos1 = ctx->text.insertPos;
    }
    updateFrom = (*(ctx->text.source->scan))(ctx->text.source, pos1, XtstWhiteSpace, XtsdLeft,
	    1, TRUE);
    updateFrom = (*(ctx->text.source->scan))(ctx->text.source, updateFrom, XtstPositions, XtsdLeft,
	    1, TRUE);
    startPos = max(updateFrom, ctx->text.lt.top);
    visible = LineAndXYForPosition(ctx, startPos, &line1, &x, &y);
    error = (*(ctx->text.source->replace))(ctx->text.source, pos1, pos2, text, &delta);
    if (error) return error;
    ctx->text.lastPos = GETLASTPOS;
    if (ctx->text.lt.top >= ctx->text.lastPos) {
	BuildLineTable(ctx, ctx->text.lastPos);
	ClearWindow(ctx);
	SetScrollBar(ctx);
	return error;
    }
    if (delta < ctx->text.lastPos) {
	for (i = 0; i < ctx->text.numranges; i++) {
	    if (ctx->text.updateFrom[i] > pos1)
		ctx->text.updateFrom[i] += delta;
	    if (ctx->text.updateTo[i] >= pos1)
		ctx->text.updateTo[i] += delta;
	}
    }

    line2 = LineForPosition(ctx, pos1);
    /* 
     * fixup all current line table entries to reflect edit.
     * BUG: it is illegal to do arithmetic on positions. This code should
     * either use scan or the source needs to provide a function for doing
     * position arithmetic.
    */
    for (i = line2 + 1; i <= ctx->text.lt.lines; i++)
	ctx->text.lt.info[i].position += delta;

    endPos = pos1;
    /*
     * Now process the line table and fixup in case edits caused
     * changes in line breaks. If we are breaking on word boundaries,
     * this code checks for moving words to and from lines.
    */
    if (visible) {
	for (i = line1; i < ctx->text.lt.lines; i++) {/* fixup line table */
	    width = (ctx->text.options & resizeWidth) ? 9999 : ctx->core.width - x;
	    if (startPos <= ctx->text.lastPos) {
		(*(ctx->text.sink->findPosition))(ctx, startPos, x,
			width, (ctx->text.options & wordBreak),
			&endPos, &realW, &realH);
		if (!(ctx->text.options & wordBreak) && endPos < ctx->text.lastPos) {
		    endPos = (*(ctx->text.source->scan))(ctx->text.source, startPos,
			    XtstEOL, XtsdRight, 1, TRUE);
		    if (endPos == startPos)
			endPos = ctx->text.lastPos + 1;
		}
		ctx->text.lt.info[i].endX = realW + x;
		ctx->text.lt.info[i + 1].y = realH + ctx->text.lt.info[i].y;
		if ((endPos > pos1) &&
			(endPos == ctx->text.lt.info[i + 1].position))
		    break;
		startPos = endPos;
	    }
	    if (startPos > ctx->text.lastPos)
		ctx->text.lt.info[i + 1].endX = ctx->text.leftmargin;
	    ctx->text.lt.info[i + 1].position = startPos;
	    x = ctx->text.lt.info[i + 1].x;
	}
    }
    if (delta >= ctx->text.lastPos)
	endPos = ctx->text.lastPos;
    if (delta >= ctx->text.lastPos || pos2 >= ctx->text.lt.top)
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
  TextWidget ctx;
  XtTextPosition pos1, pos2;
  /* it is illegal to call this routine unless there is a valid line table!*/
{
    Position x, y;
    Dimension height;
    int line, i, visible;
    XtTextPosition startPos, endPos;

    if (pos1 < ctx->text.lt.top)
	pos1 = ctx->text.lt.top;
    if (pos2 > ctx->text.lastPos) 
	pos2 = ctx->text.lastPos;
    if (pos1 >= pos2) return;
    visible = LineAndXYForPosition(ctx, pos1, &line, &x, &y);
    if (!visible)
	return;
    startPos = pos1;
    height = ctx->text.lt.info[1].y - ctx->text.lt.info[0].y;
    for (i = line; i < ctx->text.lt.lines; i++) {
	endPos = ctx->text.lt.info[i + 1].position;
	if (endPos > pos2)
	    endPos = pos2;
	if (endPos > startPos) {
	    if (x == ctx->text.leftmargin)
                (*(ctx->text.sink->clearToBackground))(ctx,
	             0, y, ctx->text.leftmargin, height);
	    if (startPos >= ctx->text.s.right || endPos <= ctx->text.s.left) {
		(*(ctx->text.sink->display))(ctx, x, y,
			startPos, endPos, FALSE);
	    } else if (startPos >= ctx->text.s.left && endPos <= ctx->text.s.right) {
		(*(ctx->text.sink->display))(ctx, x, y,
			startPos, endPos, TRUE);
	    } else {
		DisplayText(ctx, startPos, ctx->text.s.left);
		DisplayText(ctx, max(startPos, ctx->text.s.left), 
			min(endPos, ctx->text.s.right));
		DisplayText(ctx, ctx->text.s.right, endPos);
	    }
	}
	startPos = endPos;
	height = ctx->text.lt.info[i + 1].y - ctx->text.lt.info[i].y;
        (*(ctx->text.sink->clearToBackground))(ctx,
	    ctx->text.lt.info[i].endX, y, 999, height);
	x = ctx->text.leftmargin;
	y = ctx->text.lt.info[i + 1].y;
	if ((endPos == pos2) && (endPos != ctx->text.lastPos))
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
  TextWidget ctx;
  XtTextPosition position;
  unsigned short time;
  Boolean motion;
{
    int     delta;
    XtTextPosition newLeft, newRight;
    XtSelectType newType;
    XtSelectType *sarray;

    delta = (time < ctx->text.lasttime) ?
	ctx->text.lasttime - time : time - ctx->text.lasttime;
    if (motion)
	newType = ctx->text.s.type;
    else {
	if ((delta < 500) && ((position >= ctx->text.s.left)
		    && (position <= ctx->text.s.right))) { /* multi-click event */
	    sarray = ctx->text.sarray;
	    for (sarray = ctx->text.sarray;
		*sarray != XtselectNull && *sarray != ctx->text.s.type;
		sarray++) ;
	    if (*sarray != XtselectNull) sarray++;
	    if (*sarray == XtselectNull) sarray = ctx->text.sarray;
	    newType = *sarray;
	} else {			/* single-click event */
	    newType = *(ctx->text.sarray);
	}
        ctx->text.lasttime = time;
    }
    switch (newType) {
	case XtselectPosition: 
            newLeft = newRight = position;
	    break;
	case XtselectChar: 
            newLeft = position;
            newRight = (*(ctx->text.source->scan))(
                    ctx->text.source, position, position, XtsdRight, 1, FALSE);
	    break;
	case XtselectWord: 
	    newLeft = (*(ctx->text.source->scan))(
		    ctx->text.source, position, XtstWhiteSpace, XtsdLeft, 1, FALSE);
	    newRight = (*(ctx->text.source->scan))(
		    ctx->text.source, position, XtstWhiteSpace, XtsdRight, 1, FALSE);
	    break;
	case XtselectLine: 
	case XtselectParagraph:  /* need "para" scan mode to implement pargraph */
 	    newLeft = (*(ctx->text.source->scan))(
		    ctx->text.source, position, XtstEOL, XtsdLeft, 1, FALSE);
	    newRight = (*(ctx->text.source->scan))(
		    ctx->text.source, position, XtstEOL, XtsdRight, 1, FALSE);
	    break;
	case XtselectAll: 
	    newLeft = (*(ctx->text.source->scan))(
		    ctx->text.source, position, XtstFile, XtsdLeft, 1, FALSE);
	    newRight = (*(ctx->text.source->scan))(
		    ctx->text.source, position, XtstFile, XtsdRight, 1, FALSE);
	    break;
    }
    if ((newLeft != ctx->text.s.left) || (newRight != ctx->text.s.right)
	    || (newType != ctx->text.s.type)) {
	_XtTextSetNewSelection(ctx, newLeft, newRight);
	ctx->text.s.type = newType;
	if (position - ctx->text.s.left < ctx->text.s.right - position)
	    ctx->text.insertPos = newLeft;
	else 
	    ctx->text.insertPos = newRight;
    }
    if (!motion) { /* setup so we can freely mix select extend calls*/
	ctx->text.origSel.type = ctx->text.s.type;
	ctx->text.origSel.left = ctx->text.s.left;
	ctx->text.origSel.right = ctx->text.s.right;
	if (position >= ctx->text.s.left + ((ctx->text.s.right - ctx->text.s.left) / 2))
	    ctx->text.extendDir = XtsdRight;
	else
	    ctx->text.extendDir = XtsdLeft;
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
  TextWidget ctx;
  XtTextPosition position;
  Boolean motion;
{
    XtTextPosition newLeft, newRight;
	

    if (!motion) {		/* setup for extending selection */
	ctx->text.origSel.type = ctx->text.s.type;
	ctx->text.origSel.left = ctx->text.s.left;
	ctx->text.origSel.right = ctx->text.s.right;
	if (position >= ctx->text.s.left + ((ctx->text.s.right - ctx->text.s.left) / 2))
	    ctx->text.extendDir = XtsdRight;
	else
	    ctx->text.extendDir = XtsdLeft;
    }
    else /* check for change in extend direction */
	if ((ctx->text.extendDir == XtsdRight && position < ctx->text.origSel.left) ||
		(ctx->text.extendDir == XtsdLeft && position > ctx->text.origSel.right)) {
	    ctx->text.extendDir = (ctx->text.extendDir == XtsdRight)? XtsdLeft : XtsdRight;
	    _XtTextSetNewSelection(ctx, ctx->text.origSel.left, ctx->text.origSel.right);
	}
    newLeft = ctx->text.s.left;
    newRight = ctx->text.s.right;
    switch (ctx->text.s.type) {
	case XtselectPosition: 
	    if (ctx->text.extendDir == XtsdRight)
		newRight = position;
	    else
		newLeft = position;
	    break;
	case XtselectWord: 
	    if (ctx->text.extendDir == XtsdRight)
		newRight = position = (*(ctx->text.source->scan))(
			ctx->text.source, position, XtstWhiteSpace, XtsdRight, 1, FALSE);
	    else
		newLeft = position = (*(ctx->text.source->scan))(
			ctx->text.source, position, XtstWhiteSpace, XtsdLeft, 1, FALSE);
	    break;
        case XtselectLine:
	case XtselectParagraph: /* need "para" scan mode to implement pargraph */
	    if (ctx->text.extendDir == XtsdRight)
		newRight = position = (*(ctx->text.source->scan))(
			ctx->text.source, position, XtstEOL, XtsdRight, 1, TRUE);
	    else
		newLeft = position = (*(ctx->text.source->scan))(
			ctx->text.source, position, XtstEOL, XtsdLeft, 1, FALSE);
	    break;
	case XtselectAll: 
	    position = ctx->text.insertPos;
	    break;
    }
    _XtTextSetNewSelection(ctx, newLeft, newRight);
    ctx->text.insertPos = position;
}


/*
 * Clear the window to background color.
*/
static ClearWindow (ctx)
  TextWidget ctx;
{
    (*(ctx->text.sink->clearToBackground))(ctx, 0, 0, ctx->core.width, ctx->core.height);
}


/*
 * Internal redisplay entire window.
*/
DisplayTextWindow (w)
  Widget w;
{
    TextWidget ctx = (TextWidget) w;
    ClearWindow(ctx);
    BuildLineTable(ctx, ctx->text.lt.top);
    _XtTextNeedsUpdating(ctx, zeroPosition, ctx->text.lastPos);
    SetScrollBar(ctx);
}

/*
 * This routine checks to see if the window should be resized (grown or
 * shrunk) or scrolled then text to be painted overflows to the right or
 * the bottom of the window. It is used by the keyboard input routine.
*/
CheckResizeOrOverflow(ctx)
  TextWidget ctx;
{
    XtTextPosition posToCheck;
    int     visible, line, width;
    XtWidgetGeometry rbox;
    if (ctx->text.options & resizeWidth) {
	width = 0;
	for (line=0 ; line<ctx->text.lt.lines ; line++)
	    if (width < ctx->text.lt.info[line].endX)
		width = ctx->text.lt.info[line].endX;
	width += ctx->text.leftmargin;
	if (width > ctx->core.width) {
	    rbox.request_mode = CWWidth;
	    rbox.width = width;
	    if ( XtMakeGeometryRequest(ctx, &rbox, NULL) != XtGeometryNo) {
	      ctx->core.width = rbox.width;
	      }
	}
    }
    if ((ctx->text.options & resizeHeight) || (ctx->text.options & scrollOnOverflow)) {
	if (ctx->text.options & scrollOnOverflow)
	    posToCheck = ctx->text.insertPos;
	else
	    posToCheck = ctx->text.lastPos;
	visible = IsPositionVisible(ctx, posToCheck);
	if (visible)
	    line = LineForPosition(ctx, posToCheck);
	else
	    line = ctx->text.lt.lines;
	if ((ctx->text.options & scrollOnOverflow) && (line + 1 > ctx->text.lt.lines)) {
	    BuildLineTable(ctx, ctx->text.lt.info[1].position);
	    XCopyArea(XtDisplay(ctx), XtWindow(ctx), XtWindow(ctx), 
	    	      ctx->text.gc, (Position)ctx->text.leftmargin, 
		      ctx->text.lt.info[1].y, 9999, 9999,
		      (Position)ctx->text.leftmargin, ctx->text.lt.info[0].y);
	}
	else
	    if ((ctx->text.options & resizeHeight) && (line + 1 != ctx->text.lt.lines)) {
		rbox.request_mode = CWHeight;
		rbox.height = (*(ctx->text.sink->maxHeight))
				(ctx, line + 1) + (2*yMargin)+2;
		if (XtMakeGeometryRequest(ctx, &rbox, NULL) != XtGeometryNo) {
	      		ctx->core.width = rbox.height ;
	      };
	    }
    }
}

/*
 * This routine is used to perform various selection functions. The goal is
 * to be able to specify all the more popular forms of draw-through and
 * multi-click selection user interfaces from the outside.
 */
void AlterSelection (ctx, mode, action)
    TextWidget     ctx;
    SelectionMode   mode;	/* {XtsmTextSelect, XtsmTextExtend}		  */
    SelectionAction action;	/* {XtactionStart, XtactionAdjust, XtactionEnd} */
{
    XtTextPosition position;
    char   *ptr;

    position = PositionForXY (ctx, (int) ctx->text.ev_x, (int) ctx->text.ev_y);
    if (action == XtactionStart) {
	switch (mode) {
	case XtsmTextSelect: 
	    DoSelection (ctx, position, ctx->text.time, FALSE);
	    break;
	case XtsmTextExtend: 
	    ExtendSelection (ctx, position, FALSE);
	    break;
	}
    }
    else {
	switch (mode) {
	case XtsmTextSelect: 
	    DoSelection (ctx, position, ctx->text.time, TRUE);
	    break;
	case XtsmTextExtend: 
	    ExtendSelection (ctx, position, TRUE);
	    break;
	}
    }
    if (action == XtactionEnd && ctx->text.s.left < ctx->text.s.right) {
	ptr = _XtTextGetText (ctx, ctx->text.s.left, ctx->text.s.right);
	XStoreBuffer (XtDisplay(ctx), ptr, min (strlen (ptr), MAXCUT), 0);
	XtFree (ptr);
    }
}

/*
 * This routine processes all "expose region" XEvents. In general, its job
 * is to the best job at minimal re-paint of the text, displayed in the
 * window, that it can.
*/
static void ProcessExposeRegion(w, event)
  Widget w;
  XEvent *event;
{
    TextWidget ctx = (TextWidget) w;
    XtTextPosition pos1, pos2, resultend;
    int line;
    int x = event->xexpose.x;
    int y = event->xexpose.y;
    int width = event->xexpose.width;
    int height = event->xexpose.height;
    LineTableEntryPtr info;

   _XtTextPrepareToUpdate(ctx);
    if (x < ctx->text.leftmargin) /* stomp on caret tracks */
        (*(ctx->text.sink->clearToBackground))(ctx, x, y, width, height);
   /* figure out starting line that was exposed */
    line = LineForPosition(ctx, PositionForXY(ctx, x, y));
    while (line < ctx->text.lt.lines && ctx->text.lt.info[line + 1].y < y)
	line++;
    while (line < ctx->text.lt.lines) {
	info = &(ctx->text.lt.info[line]);
	if (info->y >= y + height)
	    break;
	(*(ctx->text.sink->resolve))(ctx, 
                                info->position, info->x,
			        x - info->x, &pos1, &resultend);
	(*(ctx->text.sink->resolve))(ctx, 
                                info->position, info->x,
			        x + width - info->x, &pos2, 
                                &resultend);
	pos2 = (*(ctx->text.source->scan))(ctx->text.source, pos2, XtstPositions, 
                                      XtsdRight, 1, TRUE);
	_XtTextNeedsUpdating(ctx, pos1, pos2);
	line++;
    }
    _XtTextExecuteUpdate(ctx);
}


static int oldinsert = -1;

/*
 * This routine does all setup required to syncronize batched screen updates
*/
int _XtTextPrepareToUpdate(ctx)
  TextWidget ctx;
{
    if (oldinsert < 0) {
	InsertCursor(ctx, XtisOff);
	ctx->text.numranges = 0;
	ctx->text.showposition = FALSE;
	oldinsert = ctx->text.insertPos;
    }
}


/*
 * This is a private utility routine used by _XtTextExecuteUpdate. It
 * processes all the outstanding update requests and merges update
 * ranges where possible.
*/
static FlushUpdate(ctx)
  TextWidget ctx;
{
    int     i, w;
    XtTextPosition updateFrom, updateTo;
    while (ctx->text.numranges > 0) {
	updateFrom = ctx->text.updateFrom[0];
	w = 0;
	for (i=1 ; i<ctx->text.numranges ; i++) {
	    if (ctx->text.updateFrom[i] < updateFrom) {
		updateFrom = ctx->text.updateFrom[i];
		w = i;
	    }
	}
	updateTo = ctx->text.updateTo[w];
	ctx->text.numranges--;
	ctx->text.updateFrom[w] = ctx->text.updateFrom[ctx->text.numranges];
	ctx->text.updateTo[w] = ctx->text.updateTo[ctx->text.numranges];
	for (i=ctx->text.numranges-1 ; i>=0 ; i--) {
	    while (ctx->text.updateFrom[i] <= updateTo && i < ctx->text.numranges) {
		updateTo = ctx->text.updateTo[i];
		ctx->text.numranges--;
		ctx->text.updateFrom[i] = ctx->text.updateFrom[ctx->text.numranges];
		ctx->text.updateTo[i] = ctx->text.updateTo[ctx->text.numranges];
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
  TextWidget ctx;
{
    XtTextPosition top, first, second;
    if (ctx->text.insertPos < ctx->text.lt.top ||
	ctx->text.insertPos >= ctx->text.lt.info[ctx->text.lt.lines].position) {
	if (ctx->text.lt.lines > 0 && (ctx->text.insertPos < ctx->text.lt.top 
	    || ctx->text.lt.info[ctx->text.lt.lines].position <= ctx->text.lastPos)) {
	    first = ctx->text.lt.top;
	    second = ctx->text.lt.info[1].position;
	    if (ctx->text.insertPos < first)
		top = (*(ctx->text.source->scan))(
			ctx->text.source, ctx->text.insertPos, XtstEOL,
			XtsdLeft, 1, FALSE);
	    else
		top = (*(ctx->text.source->scan))(
			ctx->text.source, ctx->text.insertPos, XtstEOL,
			XtsdLeft, ctx->text.lt.lines, FALSE);
	    BuildLineTable(ctx, top);
	    while (ctx->text.insertPos >= ctx->text.lt.info[ctx->text.lt.lines].position) {
		if (ctx->text.lt.info[ctx->text.lt.lines].position > ctx->text.lastPos)
		    break;
		BuildLineTable(ctx, ctx->text.lt.info[1].position);
	    }
	    if (ctx->text.lt.top == second) {
	        BuildLineTable(ctx, first);
		_XtTextScroll(ctx, 1);
	    } else if (ctx->text.lt.info[1].position == first) {
		BuildLineTable(ctx, first);
		_XtTextScroll(ctx, -1);
	    } else {
		ctx->text.numranges = 0;
		if (ctx->text.lt.top != first)
		    DisplayTextWindow(ctx);
	    }
	}
    }
}



/*
 * This routine causes all batched screen updates to be performed
*/
_XtTextExecuteUpdate(ctx)
  TextWidget ctx;
{
    if (oldinsert >= 0) {
	if (oldinsert != ctx->text.insertPos || ctx->text.showposition)
	    _XtTextShowPosition(ctx);
	FlushUpdate(ctx);
	InsertCursor(ctx, XtisOn);
	oldinsert = -1;
    }
}


static void TextDestroy(ctx)
TextWidget ctx;
{
    if (ctx->text.dialog)
	(void) XtDestroyWidget(ctx->text.dialog);
    if (ctx->text.outer)
	(void) XtDestroyWidget(ctx->text.outer);
    if (ctx->text.sbar)
	(void) XtDestroyWidget(ctx->text.sbar);
    XtFree((char *)ctx->text.updateFrom);
    XtFree((char *)ctx->text.updateTo);
}


static void Resize(w)
    Widget          w;
{
    TextWidget ctx = (TextWidget) w;
    _XtTextPrepareToUpdate(ctx);
    ForceBuildLineTable(ctx);
    _XtTextExecuteUpdate(ctx);
}


/*
 * This routine allow the application program to Set attributes.
 */

Boolean XtTextSetValues(current, request, new, last)
Widget current, request, new;
Boolean last;
{
    TextWidget oldtw = (TextWidget) current;
    TextWidget newtw = (TextWidget) new;
    Boolean    redisplay = FALSE;

    _XtTextPrepareToUpdate(oldtw);
    
    if (oldtw->text.sink != newtw->text.sink)
	redisplay = TRUE;

    if (oldtw->text.source != newtw->text.source) {
	ForceBuildLineTable(oldtw);
	redisplay = TRUE;
    }
    if (oldtw->text.insertPos != newtw->text.insertPos)
	oldtw->text.showposition = TRUE;

    if (oldtw->text.lt.top != newtw->text.lt.top)
	redisplay = TRUE;

    if (oldtw->text.leftmargin != newtw->text.leftmargin)
	redisplay = TRUE;

    if (redisplay) 
	DisplayTextWindow(oldtw);
    _XtTextExecuteUpdate(oldtw);
    return (FALSE);  /*  this needs looking at! */
}



void XtTextDisplay (w)
    Widget w;
{
    TextWidget ctx = (TextWidget) w;;

	_XtTextPrepareToUpdate(ctx);
	DisplayTextWindow(ctx);
	_XtTextExecuteUpdate(ctx);
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

void XtTextSetSelectionArray(w, sarray)
    Widget w;
    XtSelectType *sarray;
{
    TextWidget ctx = (TextWidget) w;
    XtSelectType *s2;

    s2 = ctx->text.sarray;
    while (XtselectNull != (*s2++ = *sarray++)) ;
}

void XtTextSetLastPos (w, lastPos)
    Widget w;
    XtTextPosition lastPos;
{
    TextWidget  ctx = (TextWidget) w;

	_XtTextPrepareToUpdate(ctx);
	(*(ctx->text.source->setLastPos))(ctx->text.source, lastPos);
	ctx->text.lastPos = GETLASTPOS;
	ForceBuildLineTable(ctx);
	DisplayTextWindow(ctx);
	_XtTextExecuteUpdate(ctx);
}


void XtTextGetSelectionPos(w, left, right)
  Widget w;
  XtTextPosition *left, *right;
{
    TextWidget ctx = (TextWidget) w;
    *left = ctx->text.s.left;
    *right = ctx->text.s.right;
}


void XtTextNewSource(w, source, startPos)
    Widget w;
    XtTextSource   *source;
    XtTextPosition startPos;
{
    TextWidget ctx = (TextWidget) w;

	ctx->text.source = source;
	ctx->text.lt.top = startPos;
	ctx->text.s.left = ctx->text.s.right = 0;
	ctx->text.insertPos = startPos;
	ctx->text.lastPos = GETLASTPOS;
	ForceBuildLineTable(ctx);
	_XtTextPrepareToUpdate(ctx);
	DisplayTextWindow(ctx);
	_XtTextExecuteUpdate(ctx);
}

/*
 * This public routine deletes the text from startPos to endPos in a source and
 * then inserts, at startPos, the text that was passed. As a side effect it
 * "invalidates" that portion of the displayed text (if any), so that things
 * will be repainted properly.
 */
int XtTextReplace(w, startPos, endPos, text)
    Widget	    w;
    XtTextPosition  startPos, endPos;
    XtTextBlock     *text;
{
    TextWidget ctx = (TextWidget) w;
    int result;
    result = EDITERROR;
	_XtTextPrepareToUpdate(ctx);
	result = ReplaceText(ctx, startPos, endPos, text);
	_XtTextExecuteUpdate(ctx);
    return result;
}


XtTextPosition XtTextTopPosition(w)
    Widget	    w;
{
    TextWidget ctx = (TextWidget) w;

     return ctx->text.lt.top;
}


void XtTextSetInsertionPoint(w, position)
    Widget	    w;
    XtTextPosition position;
{
    TextWidget ctx = (TextWidget) w;

	_XtTextPrepareToUpdate(ctx);
	ctx->text.insertPos = position;
	ctx->text.showposition = TRUE;
	_XtTextExecuteUpdate(ctx);
}


XtTextPosition XtTextGetInsertionPoint(w)
    Widget	    w;
{
    TextWidget ctx = (TextWidget) w;

    return(ctx->text.insertPos);
}


void XtTextUnsetSelection(w)
    Widget	    w;
{
    TextWidget ctx = (TextWidget) w;

	_XtTextPrepareToUpdate(ctx);
	_XtTextSetNewSelection(ctx, zeroPosition, zeroPosition);
	_XtTextExecuteUpdate(ctx);
}


void XtTextChangeOptions(w, options)
    Widget	    w;
    int    options;
{
    TextWidget ctx = (TextWidget) w;

	ctx->text.options = options;
}


int XtTextGetOptions(w)
    Widget	    w;
{
    TextWidget ctx = (TextWidget) w;

 	return ctx->text.options;
}

void XtTextSetNewSelection (w, left, right)
    Widget	    w;
    XtTextPosition left, right;
{
    TextWidget ctx = (TextWidget) w;

	_XtTextPrepareToUpdate(ctx);
        _XtTextSetNewSelection(ctx, left, right);
	_XtTextExecuteUpdate(ctx);
}

void XtTextInvalidate(w, from, to)
    Widget	    w;
    XtTextPosition from,to;
{
    TextWidget ctx = (TextWidget) w;

        ctx->text.lastPos = (*(ctx->text.source->getLastPos))(ctx->text.source);
        _XtTextPrepareToUpdate(ctx);
        _XtTextNeedsUpdating(ctx, from, to);
        ForceBuildLineTable(ctx);
        _XtTextExecuteUpdate(ctx);
}


/* Returns the window actually containing the text (which is not the same
   as the given window if the text window has scrollbars.) */

Window XtTextGetInnerWindow(w)
    Widget	    w;
{
    TextWidget ctx = (TextWidget) w;
	return XtWindow(ctx);
}

/* The following used to be a separate file, TextActs.c, but
   is included here because textActionsTable can't be external
   to the file declaring textClassRec */

/* Misc. routines */
void TextAcceptFocus(w)
    Widget          w;
{
    TextWidget ctx = (TextWidget) w;
    if (!ctx->text.hasfocus)
	XSetInputFocus(XtDisplay(ctx), XtWindow(ctx), 
	  RevertToPointerRoot,  CurrentTime);
}

static StartAction(ctx, event)
   TextWidget ctx;
   XEvent *event;
{
    _XtTextPrepareToUpdate(ctx);
    if (event->type == ButtonPress) TextAcceptFocus(ctx);
/* this code is wrong if actions bound to non-button events! */
    ctx->text.time = event->xbutton.time;
    ctx->text.ev_x = event->xbutton.x;
    ctx->text.ev_y = event->xbutton.y;
}

static EndAction(ctx)
   TextWidget ctx;
{
    CheckResizeOrOverflow(ctx);
    _XtTextExecuteUpdate(ctx);
}

/*ARGSUSED*/
static DoFeep(ctx)
    TextWidget ctx;
{
    XBell(XtDisplay(ctx), 50);
}

static DeleteOrKill(ctx, from, to, kill)
    TextWidget	   ctx;
    XtTextPosition from, to;
    Boolean	   kill;
{
    XtTextBlock text;
    char *ptr;

    if (kill && from < to) {
	ptr = _XtTextGetText(ctx, from, to);
	XStoreBuffer(XtDisplay(ctx), ptr, strlen(ptr), 1);
	XtFree(ptr);
    }
    text.length = 0;
    if (ReplaceText(ctx, from, to, &text)) {
	XBell(XtDisplay(ctx), 50);
	return;
    }
    _XtTextSetNewSelection(ctx, from, from);
    ctx->text.insertPos = from;
    ctx->text.showposition = TRUE;
}


StuffFromBuffer(ctx, buffer)
  TextWidget ctx;
  int buffer;
{
    extern char *XFetchBuffer();
    XtTextBlock text;
    text.ptr = XFetchBuffer(XtDisplay(ctx), &(text.length), buffer);
    if (ReplaceText(ctx, ctx->text.insertPos, ctx->text.insertPos, &text)) {
	XBell(XtDisplay(ctx), 50);
	return;
    }
    ctx->text.insertPos = ctx->text.source->scan(ctx->text.source, 
    	ctx->text.insertPos, XtstPositions, XtsdRight, text.length, TRUE);
    _XtTextSetNewSelection(ctx, ctx->text.insertPos, ctx->text.insertPos);
    XtFree(text.ptr);
}


static UnKill(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    StuffFromBuffer(ctx, 1);
   EndAction(ctx);
}

static Stuff(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    StuffFromBuffer(ctx, 0);
   EndAction(ctx);
}


static XtTextPosition NextPosition(ctx, position, kind, direction)
    TextWidget ctx;
    XtTextPosition position;
    ScanType kind;
    ScanDirection direction;
{
    XtTextPosition pos;

     pos = ctx->text.source->scan(
	    ctx->text.source, position, kind, direction, 1, FALSE);
     if (pos == ctx->text.insertPos) 
         pos = ctx->text.source->scan(
            ctx->text.source, position, kind, direction, 2, FALSE);
     return pos;
}

/* routines for moving around */

static MoveForwardChar(ctx, event)
   TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
   ctx->text.insertPos = ctx->text.source->scan(
        ctx->text.source, ctx->text.insertPos, XtstPositions, XtsdRight, 1, 
	TRUE);
   EndAction(ctx);
}

static MoveBackwardChar(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    ctx->text.insertPos = ctx->text.source->scan(
            ctx->text.source, ctx->text.insertPos, XtstPositions, XtsdLeft,
	    1, TRUE);
   EndAction(ctx);
}

static MoveForwardWord(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    ctx->text.insertPos = NextPosition(ctx, ctx->text.insertPos, 
      XtstWhiteSpace, XtsdRight);
   EndAction(ctx);
}

static MoveBackwardWord(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    ctx->text.insertPos = NextPosition(ctx, ctx->text.insertPos, 
      XtstWhiteSpace, XtsdLeft);
   EndAction(ctx);
}

static MoveBackwardParagraph(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    ctx->text.insertPos = NextPosition(ctx, ctx->text.insertPos, 
      XtstEOL, XtsdLeft);
   EndAction(ctx);
}

static MoveForwardParagraph(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    ctx->text.insertPos = NextPosition(ctx, ctx->text.insertPos, 
      XtstEOL, XtsdRight);
   EndAction(ctx);
}


static MoveToLineStart(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
    int line;
   StartAction(ctx, event);
    _XtTextShowPosition(ctx);
    line = LineForPosition(ctx, ctx->text.insertPos);
    ctx->text.insertPos = ctx->text.lt.info[line].position;
   EndAction(ctx);
}

static MoveToLineEnd(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
    int line;
    XtTextPosition next;
   StartAction(ctx, event);
    _XtTextShowPosition(ctx);
    line = LineForPosition(ctx, ctx->text.insertPos);
    next = ctx->text.lt.info[line+1].position;
    if (next > ctx->text.lastPos)
	next = ctx->text.lastPos;
    else
	next = ctx->text.source->scan(ctx->text.source, next, XtstPositions, 
	  XtsdLeft, 1, TRUE);
    ctx->text.insertPos = next;
   EndAction(ctx);
}


static int LineLastWidth = 0;
static XtTextPosition LineLastPosition = 0;

static MoveNextLine(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
    int     width, width2, height, line;
    XtTextPosition position, maxp;
   StartAction(ctx, event);
    _XtTextShowPosition(ctx);
    line = LineForPosition(ctx, ctx->text.insertPos);
    if (line == ctx->text.lt.lines - 1) {
	_XtTextScroll(ctx, 1);
	line = LineForPosition(ctx, ctx->text.insertPos);
    }
    if (LineLastPosition == ctx->text.insertPos)
	width = LineLastWidth;
    else
	ctx->text.sink->findDistance(ctx,
		ctx->text.lt.info[line].position, ctx->text.lt.info[line].x,
		ctx->text.insertPos, &width, &position, &height);
    line++;
    if (ctx->text.lt.info[line].position > ctx->text.lastPos) {
	ctx->text.insertPos = ctx->text.lastPos;
	EndAction(ctx);
	return;
    }
    ctx->text.sink->findPosition(ctx,
	    ctx->text.lt.info[line].position, ctx->text.lt.info[line].x,
	    width, FALSE, &position, &width2, &height);
    maxp = ctx->text.source->scan(ctx->text.source,
            ctx->text.lt.info[line+1].position,
	    XtstPositions, XtsdLeft, 1, TRUE);
    if (position > maxp)
	position = maxp;
    ctx->text.insertPos = position;
    LineLastWidth = width;
    LineLastPosition = position;
   EndAction(ctx);
}

static MovePreviousLine(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
    int     width, width2, height, line;
    XtTextPosition position, maxp;
   StartAction(ctx, event);
    _XtTextShowPosition(ctx);
    line = LineForPosition(ctx, ctx->text.insertPos);
    if (line == 0) {
	_XtTextScroll(ctx, -1);
	line = LineForPosition(ctx, ctx->text.insertPos);
    }
    if (line > 0) {
	if (LineLastPosition == ctx->text.insertPos)
	    width = LineLastWidth;
	else
	    ctx->text.sink->findDistance(ctx,
		    ctx->text.lt.info[line].position, 
		    ctx->text.lt.info[line].x,
		    ctx->text.insertPos, &width, &position, &height);
	line--;
	ctx->text.sink->findPosition(ctx,
		ctx->text.lt.info[line].position, ctx->text.lt.info[line].x,
		width, FALSE, &position, &width2, &height);
	maxp = ctx->text.source->scan(ctx->text.source, 
		ctx->text.lt.info[line+1].position,
		XtstPositions, XtsdLeft, 1, TRUE);
	if (position > maxp)
	    position = maxp;
	ctx->text.insertPos = position;
	LineLastWidth = width;
	LineLastPosition = position;
    }
   EndAction(ctx);
}



static MoveBeginningOfFile(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    ctx->text.insertPos = ctx->text.source->scan(ctx->text.source, 
    	ctx->text.insertPos, XtstFile, XtsdLeft, 1, TRUE);
   EndAction(ctx);
}


static MoveEndOfFile(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    ctx->text.insertPos = ctx->text.source->scan(ctx->text.source, 
    	ctx->text.insertPos, XtstFile,  XtsdRight, 1, TRUE);
   EndAction(ctx);
}

static ScrollOneLineUp(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    _XtTextScroll(ctx, 1);
   EndAction(ctx);
}

static ScrollOneLineDown(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    _XtTextScroll(ctx, -1);
   EndAction(ctx);
}

static MoveNextPage(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    _XtTextScroll(ctx, max(1, ctx->text.lt.lines - 2));
    ctx->text.insertPos = ctx->text.lt.top;
   EndAction(ctx);
}

static MovePreviousPage(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    _XtTextScroll(ctx, -max(1, ctx->text.lt.lines - 2));
    ctx->text.insertPos = ctx->text.lt.top;
   EndAction(ctx);
}




/* delete routines */

static DeleteForwardChar(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
    XtTextPosition next;

   StartAction(ctx, event);
    next = ctx->text.source->scan(
            ctx->text.source, ctx->text.insertPos, XtstPositions, 
	    XtsdRight, 1, TRUE);
    DeleteOrKill(ctx, ctx->text.insertPos, next, FALSE);
   EndAction(ctx);
}

static DeleteBackwardChar(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
    XtTextPosition next;

   StartAction(ctx, event);
    next = ctx->text.source->scan(
            ctx->text.source, ctx->text.insertPos, XtstPositions, 
	    XtsdLeft, 1, TRUE);
    DeleteOrKill(ctx, next, ctx->text.insertPos, FALSE);
   EndAction(ctx);
}

static DeleteForwardWord(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
    XtTextPosition next;

   StartAction(ctx, event);
    next = NextPosition(ctx, ctx->text.insertPos, XtstWhiteSpace, XtsdRight);
    DeleteOrKill(ctx, ctx->text.insertPos, next, FALSE);
   EndAction(ctx);
}

static DeleteBackwardWord(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
    XtTextPosition next;

   StartAction(ctx, event);
    next = NextPosition(ctx, ctx->text.insertPos, XtstWhiteSpace, XtsdLeft);
    DeleteOrKill(ctx, next, ctx->text.insertPos, FALSE);
   EndAction(ctx);
}

static KillForwardWord(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
    XtTextPosition next;

   StartAction(ctx, event);
    next = NextPosition(ctx, ctx->text.insertPos, XtstWhiteSpace, XtsdRight);
    DeleteOrKill(ctx, ctx->text.insertPos, next, TRUE);
   EndAction(ctx);
}

static KillBackwardWord(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
    XtTextPosition next;

   StartAction(ctx, event);
    next = NextPosition(ctx, ctx->text.insertPos, XtstWhiteSpace, XtsdLeft);
    DeleteOrKill(ctx, next, ctx->text.insertPos, TRUE);
   EndAction(ctx);
}

static KillCurrentSelection(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    DeleteOrKill(ctx, ctx->text.s.left, ctx->text.s.right, TRUE);
   EndAction(ctx);
}

static DeleteCurrentSelection(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    DeleteOrKill(ctx, ctx->text.s.left, ctx->text.s.right, FALSE);
   EndAction(ctx);
}

static KillToEndOfLine(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
    int     line;
    XtTextPosition last, next;
   StartAction(ctx, event);
    _XtTextShowPosition(ctx);
    line = LineForPosition(ctx, ctx->text.insertPos);
    last = ctx->text.lt.info[line + 1].position;
    next = ctx->text.source->scan(ctx->text.source, ctx->text.insertPos,
       XtstEOL, XtsdRight, 1, FALSE);
    if (last > ctx->text.lastPos)
	last = ctx->text.lastPos;
    if (last > next && ctx->text.insertPos < next)
	last = next;
    DeleteOrKill(ctx, ctx->text.insertPos, last, TRUE);
   EndAction(ctx);
}

static KillToEndOfParagraph(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
    XtTextPosition next;

   StartAction(ctx, event);
    next = ctx->text.source->scan(ctx->text.source, ctx->text.insertPos, XtstEOL, XtsdRight,
	    1, FALSE);
    if (next == ctx->text.insertPos)
	next = ctx->text.source->scan(ctx->text.source, next, XtstEOL, XtsdRight, 1, TRUE);
    DeleteOrKill(ctx, ctx->text.insertPos, next, TRUE);
   EndAction(ctx);
}

static int InsertNewLineAndBackup(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
   InsertNewLineAndBackupInternal(ctx);
   EndAction(ctx);
}

static int InsertNewLineAndBackupInternal(ctx)
  TextWidget ctx;
{
    XtTextBlock text;
    text.length = 1;
    text.ptr = "\n";
    text.firstPos = 0;
    if (ReplaceText(ctx, ctx->text.insertPos, ctx->text.insertPos, &text)) {
	XBell( XtDisplay(ctx), 50);
	return(EDITERROR);
    }
    _XtTextSetNewSelection(ctx, (XtTextPosition) 0, (XtTextPosition) 0);
    ctx->text.showposition = TRUE;
    return(EDITDONE);
}



static int InsertNewLine(ctx, event)
    TextWidget ctx;
   XEvent *event;
{
    XtTextPosition next;

   StartAction(ctx, event);
    if (InsertNewLineAndBackupInternal(ctx))
	return(EDITERROR);
    next = ctx->text.source->scan(ctx->text.source, ctx->text.insertPos,
	    XtstPositions, XtsdRight, 1, TRUE);
    ctx->text.insertPos = next;
   EndAction(ctx);
    return(EDITDONE);
}


static InsertNewLineAndIndent(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
    XtTextBlock text;
    XtTextPosition pos1, pos2;

   StartAction(ctx, event);
    pos1 = ctx->text.source->scan(ctx->text.source, ctx->text.insertPos, 
    	XtstEOL, XtsdLeft, 1, FALSE);
    pos2 = ctx->text.source->scan(ctx->text.source, pos1, XtstEOL, 
    	XtsdLeft, 1, TRUE);
    pos2 = ctx->text.source->scan(ctx->text.source, pos2, XtstWhiteSpace, 
    	XtsdRight, 1, TRUE);
    text.ptr = _XtTextGetText(ctx, pos1, pos2);
    text.length = strlen(text.ptr);
    if (InsertNewLine(ctx, event)) return;
    if (ReplaceText(ctx, ctx->text.insertPos, ctx->text.insertPos, &text)) {
	XBell(XtDisplay(ctx), 50);
	EndAction(ctx);
	return;
    }
    ctx->text.insertPos = ctx->text.source->scan(ctx->text.source, 
    	ctx->text.insertPos, XtstPositions, XtsdRight, text.length, TRUE);
    XtFree(text.ptr);
   EndAction(ctx);
}

static NewSelection(ctx, l, r)
  TextWidget ctx;
  XtTextPosition l, r;
{
    char   *ptr;
    _XtTextSetNewSelection(ctx, l, r);
    if (l < r) {
	ptr = _XtTextGetText(ctx, l, r);
	XStoreBuffer(XtDisplay(ctx), ptr, min(strlen(ptr), MAXCUT), 0);
	XtFree(ptr);
    }
}

static SelectWord(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
    XtTextPosition l, r;
   StartAction(ctx, event);
    l = ctx->text.source->scan(ctx->text.source, ctx->text.insertPos, 
    	XtstWhiteSpace, XtsdLeft, 1, FALSE);
    r = ctx->text.source->scan(ctx->text.source, l, XtstWhiteSpace, 
    	XtsdRight, 1, FALSE);
    NewSelection(ctx, l, r);
   EndAction(ctx);
}


static SelectAll(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
   NewSelection(ctx, (XtTextPosition) 0, ctx->text.lastPos);
   EndAction(ctx);
}

static SelectStart(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    AlterSelection(ctx, XtsmTextSelect, XtactionStart);
   EndAction(ctx);
}

static SelectAdjust(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    AlterSelection(ctx, XtsmTextSelect, XtactionAdjust);
   EndAction(ctx);
}

static SelectEnd(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    AlterSelection(ctx, XtsmTextSelect, XtactionEnd);
   EndAction(ctx);
}

static ExtendStart(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    AlterSelection(ctx, XtsmTextExtend, XtactionStart);
   EndAction(ctx);
}

static ExtendAdjust(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    AlterSelection(ctx, XtsmTextExtend, XtactionAdjust);
   EndAction(ctx);
}

static ExtendEnd(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    AlterSelection(ctx, XtsmTextExtend, XtactionEnd);
   EndAction(ctx);
}


static RedrawDisplay(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    ForceBuildLineTable(ctx);
    DisplayTextWindow(ctx);
   EndAction(ctx);
}


_XtTextAbortDialog(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   StartAction(ctx, event);
    if (ctx->text.dialog) {
	XtDestroyWidget(ctx->text.dialog);
	ctx->text.dialog = NULL;
    }
   EndAction(ctx);
}


/* Insert a file of the given name into the text.  Returns 0 if file found, 
   -1 if not. */

static InsertFileNamed(ctx, str)
  TextWidget ctx;
  char *str;
{
    int fid;
    XtTextBlock text;
    char    buf[1000];
    XtTextPosition position;

    if (str == NULL || strlen(str) == 0) return -1;
    fid = open(str, O_RDONLY);
    if (fid <= 0) return -1;
    _XtTextPrepareToUpdate(ctx);
    position = ctx->text.insertPos;
    while ((text.length = read(fid, buf, 512)) > 0) {
	text.ptr = buf;
	(void) ReplaceText(ctx, position, position, &text);
	position = ctx->text.source->scan(ctx->text.source, position, 
		XtstPositions, XtsdRight, text.length, TRUE);
    }
    (void) close(fid);
    ctx->text.insertPos = position;
    _XtTextExecuteUpdate(ctx);
    return 0;
}

static DoInsert(ctx)
  TextWidget ctx;
{
/* can't do dialog boxes yet
    if (InsertFileNamed(ctx, XtDialogGetValueString(XtDisplay(ctx), ctx->text.dialog)))
	XBell(XtDisplay(ctx), 50);
    else
	_XtTextAbortDialog(ctx);
*/
}

static TextFocusIn (ctx, event)
  TextWidget ctx;
   XEvent *event;
{ ctx->text.hasfocus = TRUE; }

static void TextFocusOut(ctx, event)
  TextWidget ctx;
   XEvent *event;
{ ctx->text.hasfocus = FALSE; }

#define STRBUFSIZE 100

static XComposeStatus compose_status = {NULL, 0};
static InsertChar(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
   char strbuf[STRBUFSIZE];
   int     keycode;
   XtTextBlock text;
   text.length = XLookupString (event, strbuf, STRBUFSIZE,
                &keycode, &compose_status);
   if (text.length==0) return;
   StartAction(ctx, event);
   text.ptr = &strbuf[0];;
   text.firstPos = 0;
   if (ReplaceText(ctx, ctx->text.insertPos, ctx->text.insertPos, &text)) {
	XBell(XtDisplay(ctx), 50);
	EndAction(ctx);
	return;
    }
    ctx->text.insertPos =
	(*(ctx->text.source->scan))(ctx->text.source, ctx->text.insertPos,
			    XtstPositions, XtsdRight, text.length, TRUE);
	 _XtTextSetNewSelection(ctx, ctx->text.insertPos, ctx->text.insertPos);

   EndAction(ctx);
}

static InsertFile(ctx, event)
  TextWidget ctx;
   XEvent *event;
{
    char *ptr;
    XtTextBlock text;

/* this depends on dialog boxes...

   StartAction(ctx, event);
    if (ctx->text.source->editType(ctx->text.source) != XttextEdit) {
	XBell(XtDisplay(ctx), 50);
	EndAction(ctx);
	return;
    }
    if (ctx->text.s.left < ctx->text.s.right) {
	ptr = _XtTextGetText(ctx, ctx->text.s.left, ctx->text.s.right);
	DeleteCurrentSelection(ctx);
	if (InsertFileNamed(ctx, ptr)) {
	    XBell( XtDisplay(ctx), 50);
	    text.ptr = ptr;
	    text.length = strlen(ptr);
	    (void) ReplaceText(ctx, ctx->text.insertPos, ctx->text.insertPos, &text);
	    ctx->text.s.left = ctx->text.insertPos;
	    ctx->text.s.right = ctx->text.insertPos = 
		ctx->text.source->scan(ctx->text.source, ctx->text.insertPos, 
		  XtstPositions, XtsdRight, text.length, TRUE);
	}
	XtFree(ptr);
	EndAction(ctx);
	return;
    }
    if (ctx->text.dialog)
	_XtTextAbortDialog(ctx);
    ctx->text.dialog = XtDialogCreate( XtDisplay(ctx), ctx->text.w, "Insert File:", "", (ArgList)NULL, 0);
    XtDialogAddButton( XtDisplay(ctx), ctx->text.dialog, "Abort", _XtTextAbortDialog, (caddr_t)ctx);
    XtDialogAddButton( XtDisplay(ctx), ctx->text.dialog, "DoIt", DoInsert, (caddr_t)ctx);
    XMapWindow( XtDisplay(ctx), ctx->text.dialog);
   EndAction(ctx);
*/
}

/* Actions Table */

XtActionsRec textActionsTable [] = {
/* motion bindings */
  {"forward-character", 	(caddr_t)MoveForwardChar},
  {"backward-character", 	(caddr_t)MoveBackwardChar},
  {"forward-word", 		(caddr_t)MoveForwardWord},
  {"backward-word", 		(caddr_t)MoveBackwardWord},
  {"forward-paragraph", 	(caddr_t)MoveForwardParagraph},
  {"backward-paragraph", 	(caddr_t)MoveBackwardParagraph},
  {"beginning-of-line", 	(caddr_t)MoveToLineStart},
  {"end-of-line", 		(caddr_t)MoveToLineEnd},
  {"next-line", 		(caddr_t)MoveNextLine},
  {"previous-line", 		(caddr_t)MovePreviousLine},
  {"next-page", 		(caddr_t)MoveNextPage},
  {"previous-page", 		(caddr_t)MovePreviousPage},
  {"beginning-of-file", 	(caddr_t)MoveBeginningOfFile},
  {"end-of-file", 		(caddr_t)MoveEndOfFile},
  {"scroll-one-line-up", 	(caddr_t)ScrollOneLineUp},
  {"scroll-one-line-down", 	(caddr_t)ScrollOneLineDown},
/* delete bindings */
  {"delete-next-character", 	(caddr_t)DeleteForwardChar},
  {"delete-previous-character", (caddr_t)DeleteBackwardChar},
  {"delete-next-word", 		(caddr_t)DeleteForwardWord},
  {"delete-previous-word", 	(caddr_t)DeleteBackwardWord},
  {"delete-selection", 		(caddr_t)DeleteCurrentSelection},
/* kill bindings */
  {"kill-word", 		(caddr_t)KillForwardWord},
  {"backward-kill-word", 	(caddr_t)KillBackwardWord},
  {"kill-selection", 		(caddr_t)KillCurrentSelection},
  {"kill-to-end-of-line", 	(caddr_t)KillToEndOfLine},
  {"kill-to-end-of-paragraph", 	(caddr_t)KillToEndOfParagraph},
/* unkill bindings */
  {"unkill", 			(caddr_t)UnKill},
  {"stuff", 			(caddr_t)Stuff},
/* new line stuff */
  {"newline-and-indent", 	(caddr_t)InsertNewLineAndIndent},
  {"newline-and-backup", 	(caddr_t)InsertNewLineAndBackup},
  {"newline", 			(caddr_t)InsertNewLine},
/* Selection stuff */
  {"select-word", 		(caddr_t)SelectWord},
  {"select-all", 		(caddr_t)SelectAll},
  {"select-start", 		(caddr_t)SelectStart},
  {"select-adjust", 		(caddr_t)SelectAdjust},
  {"select-end", 		(caddr_t)SelectEnd},
  {"extend-start", 		(caddr_t)ExtendStart},
  {"extend-adjust", 		(caddr_t)ExtendAdjust},
  {"extend-end", 		(caddr_t)ExtendEnd},
/* Miscellaneous */
  {"redraw-display", 		(caddr_t)RedrawDisplay},
  {"insert-file", 		(caddr_t)InsertFile},
  {"insert-char", 		(caddr_t)InsertChar},
  {"focus-in", 	 	        (caddr_t)TextFocusIn},
  {"focus-out", 		(caddr_t)TextFocusOut},
  {NULL,NULL}
};

static char *defaultTextEventBindings[] = {
/* motion bindings */
    "Ctrl<Key>F:		forward-character()",
    "<Key>0xff53:	forward-character()", 
    "Ctrl<Key>B:		backward-character()",
    "<Key>0xff51:	backward-character()",
    "Meta<Key>F:		forward-word()",
    "Meta<Key>B:		backward-word()",
    "Meta<Key>]:		forward-paragraph()",
    "Ctrl<Key>[:		backward-paragraph()",
    "Ctrl<Key>A:		beginning-of-line()",
    "Ctrl<Key>E:		end-of-line()",
    "Ctrl<Key>N:           next-line()",
    "<Key>0xff54:	next-line()",
    "Ctrl<Key>P:		previous-line()",
    "<Key>0xff52:       previous-line()",
    "Ctrl<Key>V:		next-page()",
    "Meta<Key>V:		previous-page()",
    "Meta<Key>\\<:		beginning-of-file()",
    "Meta<Key>\\>:		end-of-file()",
    "Ctrl<Key>Z:		scroll-one-line-up()",
    "Meta<Key>Z:		scroll-one-line-down()",
/* delete binding*/
    "Ctrl<Key>D:		delete-next-character()",
    "Ctrl<Key>H:		delete-previous-character()",
    "<Key>0xff7f:	delete-previous-character()",
    "<Key>0xffff:	delete-previous-character()",
    "<Key>0xff08:	delete-previous-character()",
    "Meta<Key>D:		delete-next-word()",
    "Meta<Key>H:		delete-previous-word()",
/* kill bindings */
    "Shift Meta<Key>D:		kill-word()",
    "Shift Meta<Key>H:		backward-kill-word()",
    "Ctrl<Key>W:		kill-selection()",
    "Ctrl<Key>K:		kill-to-end-of-line()",
    "Meta<Key>K:		kill-to-end-of-paragraph()",
/* unkill bindings */
    "Ctrl<Key>Y:		unkill()",
    "Meta<Key>Y:		stuff()",
/* new line stuff */
    "Ctrl<Key>J:		newline-and-indent()",
    "<Key>0xff0a:	newline-and-indent()",
    "Ctrl<Key>O:		newline-and-backup()",
    "Ctrl<Key>M:		newline()",
    "<Key>0xff0d:	newline()",
/* Miscellaneous */
    "Ctrl<Key>L:		redraw-display()",
    "Meta<Key>I:		insert-file()",
   "<FocusIn>:		focus-in()",
   "<FocusOut>:		focus-out()",
/* selection stuff */
    "<Btn1Down>:	select-start()",
    "Button1<PtrMoved>:	extend-adjust()",
    "<Btn1Up>:		extend-end()",
    "<Btn2Down>:	stuff()",
    "<Btn3Down>:	extend-start()",
    "Button3<PtrMoved>: 	extend-adjust()",
    "<Btn3Up>:		extend-end()",
/* default character handling */
    "<Key>:		insert-char()",
    
    NULL
};

/* grodyness needed because Xrm wants pointer to thing, not thing... */
caddr_t defaultTextTranslations = (caddr_t)defaultTextEventBindings;

TextClassRec textClassRec = {
/* core fields */
    /* superclass       */      (WidgetClass) &widgetClassRec,
    /* class_name       */      "Text",
    /* widget_size      */      sizeof(TextRec),
    /* class_initialize */      NULL,
    /* class_inited     */      FALSE,
    /* initialize       */      Initialize,
    /* realize          */      Realize,
    /* actions          */      textActionsTable,
    /* num_actions      */      XtNumber(textActionsTable),
    /* resources        */      resources,
    /* num_ resource    */      XtNumber(resources),
    /* xrm_class        */      NULLQUARK,
    /* compress_motion    */    TRUE,
    /* compress_exposure  */    FALSE,
    /* visible_interest */      FALSE,
    /* destroy          */      TextDestroy,
    /* resize           */      Resize,
    /* expose           */      ProcessExposeRegion,
    /* set_values       */      XtTextSetValues,
    /* accept_focus     */      TextAcceptFocus,
};

WidgetClass textWidgetClass = (WidgetClass)&textClassRec;

 
