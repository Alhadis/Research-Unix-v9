/* $Header: TextActs.c,v 1.1 87/09/11 07:59:49 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)TextActions.c	1.17	2/25/87";
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
#include "Intrinsic.h"
#include "Text.h"
#include "TextDisp.h"
#include "Dialog.h"
#include <sys/file.h>

#define min(x,y)        ((x) < (y) ? (x) : (y))
#define max(x,y)        ((x) > (y) ? (x) : (y))

/* ||| Kludge, declar in .h file somewhere */

extern char *_XtTextGetText(); /* ctx, left, right */
    /* TextContext    *ctx;		*/
    /* XtTextPosition left, right;	*/

extern caddr_t XtSetActionBindings();/* eventTable, actionTable, defaultValue */
    /* XtEventsPtr  eventTable;		*/
    /* XtActionsPtr actionTable;	*/
    /* caddr_t 	    defaultValue;	*/

extern void AlterSelection(); /* ctx, mode, action */
    /* TextContext     *ctx;						  */
    /* SelectionMode   mode;	/* {XtsmTextSelect, XtsmTextExtend} 	  */
    /* SelectionAction action;	/* {XtactionStart, XtactionAdjust, XtactionEnd} */

extern void ForceBuildLineTable(); /* ctx */
    /* TextContext *ctx;		*/

/* Misc. routines */

/*ARGSUSED*/
static DoFeep(ctx)
    TextContext *ctx;
{
    XBell(ctx->dpy, 50);
}

static DeleteOrKill(ctx, from, to, kill)
    TextContext	   *ctx;
    XtTextPosition from, to;
    Boolean	   kill;
{
    XtTextBlock text;
    char *ptr;

    if (kill && from < to) {
	ptr = _XtTextGetText(ctx, from, to);
	XStoreBuffer(ctx->dpy, ptr, strlen(ptr), 1);
	XtFree(ptr);
    }
    text.length = 0;
    if (ReplaceText(ctx, from, to, &text)) {
	XBell(ctx->dpy, 50);
	return;
    }
    _XtTextSetNewSelection(ctx, from, from);
    ctx->insertPos = from;
    ctx->showposition = TRUE;
}


StuffFromBuffer(ctx, buffer)
  TextContext *ctx;
  int buffer;
{
    extern char *XFetchBuffer();
    XtTextBlock text;
    text.ptr = XFetchBuffer(ctx->dpy, &(text.length), buffer);
    if (ReplaceText(ctx, ctx->insertPos, ctx->insertPos, &text)) {
	XBell(ctx->dpy, 50);
	return;
    }
    ctx->insertPos = ctx->source->scan(ctx->source, ctx->insertPos,
	    XtstPositions, XtsdRight, text.length, TRUE);
    _XtTextSetNewSelection(ctx, ctx->insertPos, ctx->insertPos);
    XtFree(text.ptr);
}


static UnKill(ctx)
  TextContext *ctx;
{
    StuffFromBuffer(ctx, 1);
}

static Stuff(ctx)
  TextContext *ctx;
{
    StuffFromBuffer(ctx, 0);
}


static XtTextPosition NextPosition(ctx, position, kind, direction)
    TextContext *ctx;
    XtTextPosition position;
    ScanType kind;
    ScanDirection direction;
{
    XtTextPosition pos;

     pos = ctx->source->scan(
	    ctx->source, position, kind, direction, 1, FALSE);
     if (pos == ctx->insertPos) 
         pos = ctx->source->scan(
            ctx->source, position, kind, direction, 2, FALSE);
     return pos;
}

/* routines for moving around */

static MoveForwardChar(ctx)
    TextContext *ctx;
{
    ctx->insertPos = ctx->source->scan(
            ctx->source, ctx->insertPos, XtstPositions, XtsdRight, 1, TRUE);
}

static MoveBackwardChar(ctx)
    TextContext *ctx;
{
    ctx->insertPos = ctx->source->scan(
            ctx->source, ctx->insertPos, XtstPositions, XtsdLeft, 1, TRUE);
}

static MoveForwardWord(ctx)
    TextContext *ctx;
{
    ctx->insertPos = NextPosition(ctx, ctx->insertPos, XtstWhiteSpace, XtsdRight);
}

static MoveBackwardWord(ctx)
    TextContext *ctx;
{
    ctx->insertPos = NextPosition(ctx, ctx->insertPos, XtstWhiteSpace, XtsdLeft);
}

static MoveBackwardParagraph(ctx)
    TextContext *ctx;
{
    ctx->insertPos = NextPosition(ctx, ctx->insertPos, XtstEOL, XtsdLeft);
}

static MoveForwardParagraph(ctx)
    TextContext *ctx;
{
    ctx->insertPos = NextPosition(ctx, ctx->insertPos, XtstEOL, XtsdRight);
}


static MoveToLineStart(ctx)
  TextContext *ctx;
{
    int line;
    _XtTextShowPosition(ctx);
    line = LineForPosition(ctx, ctx->insertPos);
    ctx->insertPos = ctx->lt.info[line].position;
}

static MoveToLineEnd(ctx)
  TextContext *ctx;
{
    int line;
    XtTextPosition next;
    _XtTextShowPosition(ctx);
    line = LineForPosition(ctx, ctx->insertPos);
    next = ctx->lt.info[line+1].position;
    if (next > ctx->lastPos)
	next = ctx->lastPos;
    else
	next = ctx->source->scan(ctx->source, next, XtstPositions, XtsdLeft, 1, TRUE);
    ctx->insertPos = next;
}


static int LineLastWidth = 0;
static XtTextPosition LineLastPosition = 0;

static MoveNextLine(ctx)
  TextContext *ctx;
{
    int     width, width2, height, line;
    XtTextPosition position, maxp;
    _XtTextShowPosition(ctx);
    line = LineForPosition(ctx, ctx->insertPos);
    if (line == ctx->lt.lines - 1) {
	_XtTextScroll(ctx, 1);
	line = LineForPosition(ctx, ctx->insertPos);
    }
    if (LineLastPosition == ctx->insertPos)
	width = LineLastWidth;
    else
	ctx->sink->findDistance(ctx->sink, ctx->source,
		ctx->lt.info[line].position, ctx->lt.info[line].x,
		ctx->insertPos, &width, &position, &height);
    line++;
    if (ctx->lt.info[line].position > ctx->lastPos) {
	ctx->insertPos = ctx->lastPos;
	return;
    }
    ctx->sink->findPosition(ctx->sink, ctx->source,
	    ctx->lt.info[line].position, ctx->lt.info[line].x,
	    width, FALSE, &position, &width2, &height);
    maxp = ctx->source->scan(ctx->source, ctx->lt.info[line+1].position,
	    XtstPositions, XtsdLeft, 1, TRUE);
    if (position > maxp)
	position = maxp;
    ctx->insertPos = position;
    LineLastWidth = width;
    LineLastPosition = position;
}

static MovePreviousLine(ctx)
  TextContext *ctx;
{
    int     width, width2, height, line;
    XtTextPosition position, maxp;
    _XtTextShowPosition(ctx);
    line = LineForPosition(ctx, ctx->insertPos);
    if (line == 0) {
	_XtTextScroll(ctx, -1);
	line = LineForPosition(ctx, ctx->insertPos);
    }
    if (line > 0) {
	if (LineLastPosition == ctx->insertPos)
	    width = LineLastWidth;
	else
	    ctx->sink->findDistance(ctx->sink, ctx->source,
		    ctx->lt.info[line].position, ctx->lt.info[line].x,
		    ctx->insertPos, &width, &position, &height);
	line--;
	ctx->sink->findPosition(ctx->sink, ctx->source,
		ctx->lt.info[line].position, ctx->lt.info[line].x,
		width, FALSE, &position, &width2, &height);
	maxp = ctx->source->scan(ctx->source, ctx->lt.info[line+1].position,
		XtstPositions, XtsdLeft, 1, TRUE);
	if (position > maxp)
	    position = maxp;
	ctx->insertPos = position;
	LineLastWidth = width;
	LineLastPosition = position;
    }
}



static MoveBeginningOfFile(ctx)
  TextContext *ctx;
{
    ctx->insertPos = ctx->source->scan(ctx->source, ctx->insertPos, XtstFile,
	    XtsdLeft, 1, TRUE);
}


static MoveEndOfFile(ctx)
  TextContext *ctx;
{
    ctx->insertPos = ctx->source->scan(ctx->source, ctx->insertPos, XtstFile,
	    XtsdRight, 1, TRUE);
}

static ScrollOneLineUp(ctx)
  TextContext *ctx;
{
    _XtTextScroll(ctx, 1);
}

static ScrollOneLineDown(ctx)
  TextContext *ctx;
{
    _XtTextScroll(ctx, -1);
}

static MoveNextPage(ctx)
  TextContext *ctx;
{
    _XtTextScroll(ctx, max(1, ctx->lt.lines - 2));
    ctx->insertPos = ctx->lt.top;
}

static MovePreviousPage(ctx)
  TextContext *ctx;
{
    _XtTextScroll(ctx, -max(1, ctx->lt.lines - 2));
    ctx->insertPos = ctx->lt.top;
}




/* delete routines */

static DeleteForwardChar(ctx)
    TextContext *ctx;
{
    XtTextPosition next;

    next = ctx->source->scan(
            ctx->source, ctx->insertPos, XtstPositions, XtsdRight, 1, TRUE);
    DeleteOrKill(ctx, ctx->insertPos, next, FALSE);
}

static DeleteBackwardChar(ctx)
    TextContext *ctx;
{
    XtTextPosition next;

    next = ctx->source->scan(
            ctx->source, ctx->insertPos, XtstPositions, XtsdLeft, 1, TRUE);
    DeleteOrKill(ctx, next, ctx->insertPos, FALSE);
}

static DeleteForwardWord(ctx)
    TextContext *ctx;
{
    XtTextPosition next;

    next = NextPosition(ctx, ctx->insertPos, XtstWhiteSpace, XtsdRight);
    DeleteOrKill(ctx, ctx->insertPos, next, FALSE);
}

static DeleteBackwardWord(ctx)
    TextContext *ctx;
{
    XtTextPosition next;

    next = NextPosition(ctx, ctx->insertPos, XtstWhiteSpace, XtsdLeft);
    DeleteOrKill(ctx, next, ctx->insertPos, FALSE);
}

static KillForwardWord(ctx)
    TextContext *ctx;
{
    XtTextPosition next;

    next = NextPosition(ctx, ctx->insertPos, XtstWhiteSpace, XtsdRight);
    DeleteOrKill(ctx, ctx->insertPos, next, TRUE);
}

static KillBackwardWord(ctx)
    TextContext *ctx;
{
    XtTextPosition next;

    next = NextPosition(ctx, ctx->insertPos, XtstWhiteSpace, XtsdLeft);
    DeleteOrKill(ctx, next, ctx->insertPos, TRUE);
}

static KillCurrentSelection(ctx)
    TextContext *ctx;
{
    DeleteOrKill(ctx, ctx->s.left, ctx->s.right, TRUE);
}

static DeleteCurrentSelection(ctx)
    TextContext *ctx;
{
    DeleteOrKill(ctx, ctx->s.left, ctx->s.right, FALSE);
}

static KillToEndOfLine(ctx)
    TextContext *ctx;
{
    int     line;
    XtTextPosition last, next;
    _XtTextShowPosition(ctx);
    line = LineForPosition(ctx, ctx->insertPos);
    last = ctx->lt.info[line + 1].position;
    next = ctx->source->scan(ctx->source, ctx->insertPos, XtstEOL, XtsdRight,
	    1, FALSE);
    if (last > ctx->lastPos)
	last = ctx->lastPos;
    if (last > next && ctx->insertPos < next)
	last = next;
    DeleteOrKill(ctx, ctx->insertPos, last, TRUE);
}

static KillToEndOfParagraph(ctx)
    TextContext *ctx;
{
    XtTextPosition next;

    next = ctx->source->scan(ctx->source, ctx->insertPos, XtstEOL, XtsdRight,
	    1, FALSE);
    if (next == ctx->insertPos)
	next = ctx->source->scan(ctx->source, next, XtstEOL, XtsdRight, 1, TRUE);
    DeleteOrKill(ctx, ctx->insertPos, next, TRUE);
}

static int InsertNewLineAndBackup(ctx)
  TextContext *ctx;
{
    XtTextBlock text;
    text.length = 1;
    text.ptr = "\n";
    text.firstPos = 0;
    if (ReplaceText(ctx, ctx->insertPos, ctx->insertPos, &text)) {
	XBell(ctx->dpy, 50);
	return(EDITERROR);
    }
    _XtTextSetNewSelection(ctx, (XtTextPosition) 0, (XtTextPosition) 0);
    ctx->showposition = TRUE;
    return(EDITDONE);
}



static int InsertNewLine(ctx)
    TextContext *ctx;
{
    XtTextPosition next;

    if (InsertNewLineAndBackup(ctx))
	return(EDITERROR);
    next = ctx->source->scan(ctx->source, ctx->insertPos,
	    XtstPositions, XtsdRight, 1, TRUE);
    ctx->insertPos = next;
    return(EDITDONE);
}


static InsertNewLineAndIndent(ctx)
  TextContext *ctx;
{
    XtTextBlock text;
    XtTextPosition pos1, pos2;

    pos1 = ctx->source->scan(ctx->source, ctx->insertPos, XtstEOL, XtsdLeft,
	    1, FALSE);
    pos2 = ctx->source->scan(ctx->source, pos1, XtstEOL, XtsdLeft, 1, TRUE);
    pos2 = ctx->source->scan(ctx->source, pos2, XtstWhiteSpace, XtsdRight, 1, TRUE);
    text.ptr = _XtTextGetText(ctx, pos1, pos2);
    text.length = strlen(text.ptr);
    if (InsertNewLine(ctx)) return;
    if (ReplaceText(ctx, ctx->insertPos, ctx->insertPos, &text)) {
	XBell(ctx->dpy, 50);
	return;
    }
    ctx->insertPos = ctx->source->scan(ctx->source, ctx->insertPos,
	    XtstPositions, XtsdRight, text.length, TRUE);
    XtFree(text.ptr);
}

static NewSelection(ctx, l, r)
  TextContext *ctx;
  XtTextPosition l, r;
{
    char   *ptr;
    _XtTextSetNewSelection(ctx, l, r);
    if (l < r) {
	ptr = _XtTextGetText(ctx, l, r);
	XStoreBuffer(ctx->dpy, ptr, min(strlen(ptr), MAXCUT), 0);
	XtFree(ptr);
    }
}

static SelectWord(ctx)
  TextContext *ctx;
{
    XtTextPosition l, r;
    l = ctx->source->scan(ctx->source, ctx->insertPos, XtstWhiteSpace, XtsdLeft,
	1, FALSE);
    r = ctx->source->scan(ctx->source, l, XtstWhiteSpace, XtsdRight, 1, FALSE);
    NewSelection(ctx, l, r);
}


static SelectAll(ctx)
  TextContext *ctx;
{
   NewSelection(ctx, (XtTextPosition) 0, ctx->lastPos);
}

static SelectStart(ctx)
  TextContext *ctx;
{
    AlterSelection(ctx, XtsmTextSelect, XtactionStart);
}

static SelectAdjust(ctx)
  TextContext *ctx;
{
    AlterSelection(ctx, XtsmTextSelect, XtactionAdjust);
}

static SelectEnd(ctx)
  TextContext *ctx;
{
    AlterSelection(ctx, XtsmTextSelect, XtactionEnd);
}

static ExtendStart(ctx)
  TextContext *ctx;
{
    AlterSelection(ctx, XtsmTextExtend, XtactionStart);
}

static ExtendAdjust(ctx)
  TextContext *ctx;
{
    AlterSelection(ctx, XtsmTextExtend, XtactionAdjust);
}

static ExtendEnd(ctx)
  TextContext *ctx;
{
    AlterSelection(ctx, XtsmTextExtend, XtactionEnd);
}


static RedrawDisplay(ctx)
  TextContext *ctx;
{
    ForceBuildLineTable(ctx);
    DisplayTextWindow(ctx);
}


_XtTextAbortDialog(ctx)
  TextContext *ctx;
{
    if (ctx->dialog) {
	XDestroyWindow(ctx->dpy, ctx->dialog);
        (void) XtSendDestroyNotify(ctx->dpy, ctx->dialog);
	ctx->dialog = NULL;
    }
}


/* Insert a file of the given name into the text.  Returns 0 if file found, 
   -1 if not. */

static InsertFileNamed(ctx, str)
  TextContext *ctx;
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
    position = ctx->insertPos;
    while ((text.length = read(fid, buf, 512)) > 0) {
	text.ptr = buf;
	(void) ReplaceText(ctx, position, position, &text);
	position = ctx->source->scan(ctx->source, position, XtstPositions,
		XtsdRight, text.length, TRUE);
    }
    (void) close(fid);
    ctx->insertPos = position;
    _XtTextExecuteUpdate(ctx);
    return 0;
}

static DoInsert(ctx)
  TextContext *ctx;
{
    if (InsertFileNamed(ctx, XtDialogGetValueString(ctx->dpy, ctx->dialog)))
	XBell(ctx->dpy, 50);
    else
	_XtTextAbortDialog(ctx);
}

static InsertFile(ctx)
  TextContext *ctx;
{
    char *ptr;
    XtTextBlock text;

    if (ctx->source->editType(ctx->source) != XttextEdit) {
	XBell(ctx->dpy, 50);
	return;
    }
    if (ctx->s.left < ctx->s.right) {
	ptr = _XtTextGetText(ctx, ctx->s.left, ctx->s.right);
	DeleteCurrentSelection(ctx);
	if (InsertFileNamed(ctx, ptr)) {
	    XBell(ctx->dpy, 50);
	    text.ptr = ptr;
	    text.length = strlen(ptr);
	    (void) ReplaceText(ctx, ctx->insertPos, ctx->insertPos, &text);
	    ctx->s.left = ctx->insertPos;
	    ctx->s.right = ctx->insertPos = 
		ctx->source->scan(ctx->source, ctx->insertPos, XtstPositions,
			XtsdRight, text.length, TRUE);
	}
	XtFree(ptr);
	return;
    }
    if (ctx->dialog)
	_XtTextAbortDialog(ctx);
    ctx->dialog = XtDialogCreate(ctx->dpy, ctx->w, "Insert File:", "", (ArgList)NULL, 0);
    XtDialogAddButton(ctx->dpy, ctx->dialog, "Abort", _XtTextAbortDialog, (caddr_t)ctx);
    XtDialogAddButton(ctx->dpy, ctx->dialog, "DoIt", DoInsert, (caddr_t)ctx);
    XMapWindow(ctx->dpy, ctx->dialog);
}

/* Actions Table */

static XtActionsRec actionsTable [] = {
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
  {NULL,NULL}
};

static char *defaultTextEventBindings[] = {
/* motion bindings */
    "<Ctrl>F:		forward-character\n",
    "<Key>0xff53:	forward-character\n",
    "<Ctrl>B:		backward-character\n",
    "<Key>0xff51:	backward-character\n",
    "<Meta>F:		forward-word\n",
    "<Meta>B:		backward-word\n",
    "<Meta>]:		forward-paragraph\n",
    "<Ctrl>[:		backward-paragraph\n",
    "<Ctrl>A:		beginning-of-line\n",
    "<Ctrl>E:		end-of-line\n",
    "<Ctrl>N:           next-line\n",
    "<Key>0xff54:	next-line\n",
    "<Ctrl>P:		previous-line\n",
    "<Key>0xff52:       previous-line\n",
    "<Ctrl>V:		next-page\n",
    "<Meta>V:		previous-page\n",
    "<Meta>\\<:		beginning-of-file\n",
    "<Meta>\\>:		end-of-file\n",
    "<Ctrl>Z:		scroll-one-line-up\n",
    "<Meta>Z:		scroll-one-line-down\n",
/* delete bindings */
    "<Ctrl>D:		delete-next-character\n",
    "<Ctrl>H:		delete-previous-character\n",
    "<Key>0xff7f:	delete-previous-character\n",
    "<Key>0xffff:	delete-previous-character\n",
    "<Key>0xff08:	delete-previous-character\n",
    "<Meta>D:		delete-next-word\n",
    "<Meta>H:		delete-previous-word\n",
/* kill bindings */
    "s<Meta>D:		kill-word\n",
    "s<Meta>H:		backward-kill-word\n",
    "<Ctrl>W:		kill-selection\n",
    "<Ctrl>K:		kill-to-end-of-line\n",
    "<Meta>K:		kill-to-end-of-paragraph\n",
/* unkill bindings */
    "<Ctrl>Y:		unkill\n",
    "<Meta>Y:		stuff\n",
/* new line stuff */
    "<Ctrl>J:		newline-and-indent\n",
    "<Key>0xff0a:	newline-and-indent\n",
    "<Ctrl>O:		newline-and-backup\n",
    "<Ctrl>M:		newline\n",
    "<Key>0xff0d:	newline\n",
/* Miscellaneous */
    "<Ctrl>L:		redraw-display\n",
    "<Meta>I:		insert-file\n",
/* selection stuff */
    "<BtnDown>1:	select-start\n",
    "<PtrMoved>1:	extend-adjust\n",
    "<BtnUp>1:		extend-end\n",
    "<BtnDown>2:	stuff\n",
    "<BtnDown>3:	extend-start\n",
    "<PtrMoved>3:	extend-adjust\n",
    "<BtnUp>3:		extend-end\n",
    NULL
};

caddr_t XtSetTextEventBindings(dpy, eventTable)
  Display *dpy;
  XtEventsPtr eventTable;
{
    caddr_t state;

    if (eventTable == NULL) /* supply defaults */
       eventTable = XtParseEventBindings(defaultTextEventBindings);
    state = XtSetActionBindings(
	dpy, eventTable, actionsTable, (caddr_t) DoFeep);
    return state;
}

