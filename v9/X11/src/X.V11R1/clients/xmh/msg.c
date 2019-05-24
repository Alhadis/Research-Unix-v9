#ifndef lint
static char rcs_id[] = "$Header: msg.c,v 1.18 87/08/20 11:14:28 swick Exp $";
#endif lint
/*
 *			  COPYRIGHT 1987
 *		   DIGITAL EQUIPMENT CORPORATION
 *		       MAYNARD, MASSACHUSETTS
 *			ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR
 * ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT RIGHTS,
 * APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN ADDITION TO THAT
 * SET FORTH ABOVE.
 *
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting documentation,
 * and that the name of Digital Equipment Corporation not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 */

/* msgs.c -- handle operations on messages. */

#include "xmh.h"
#include "tocintrnl.h"
#include <sys/file.h>

#ifdef NOGRABFOCUS
/* Overload this Xlib routine, as the toolkit widgets use it directly. */

XSetInputFocus(d, w, r, t)
     Display d;
     Window w;
     int r;
     Time t;
{
}
#endif


/* Return the user-viewable name of the given message. */

static char *NameOfMsg(msg)
Msg msg;
{
    static char result[100];
    (void) sprintf(result, "%s:%d", msg->toc->foldername, msg->msgid);
    return result;
}


/* Update the message titlebar in the given scrn. */

static void ResetMsgLabel(scrn)
Scrn scrn;
{
    Msg msg;
    char str[200];
    if (scrn) {
 	msg = scrn->msg;
	if (msg == NULL) (void) strcpy(str, Version());
	else {
	    (void) strcpy(str, NameOfMsg(msg));
	    switch (msg->fate) {
	      case Fdelete:
		(void) strcat(str, " -> *Delete*");
 		break;
	      case Fcopy:
	      case Fmove:
		(void) strcat(str, " -> ");
		(void) strcat(str, msg->desttoc->foldername);
		if (msg->fate == Fcopy)
		    (void) strcat(str, " (Copy)");
		break;
	    }
	    if (msg->temporary) (void)strcat(str, " [Temporary]");
	}
	ChangeLabel(scrn->viewlabel, str);
    }
}


/* A major msg change has occured; redisplay it.  (This also should
work even if we now have a new source to display stuff from.)  This
routine arranges to hide boring headers, and also will set the text
insertion point to the proper place if this is a composition and we're
viewing it for the first time. */

static void RedisplayMsg(scrn)
Scrn scrn;
{
    Msg msg;
    XtTextPosition startPos, lastPos, nextPos;
    int length; char str[100];
    XtTextBlock text;
    if (scrn) {
	msg = scrn->msg;
	if (msg) {
	    startPos = 0;
	    if (defHideBoringHeaders && scrn->kind != STcomp) {
		lastPos = msg->source->getLastPos(msg->source);
 		while (startPos < lastPos) {
		    nextPos = startPos;
		    length = 0;
		    while (length < 8 && nextPos < lastPos) {
			nextPos = (*msg->source->read)(msg->source, nextPos,
						       &text, 8 - length);
			(void) strncpy(str + length, text.ptr, text.length);
 			length += text.length;
		    }
		    if (length == 8) {
			if (strncmp(str, "From:", 5) == 0 ||
			    strncmp(str, "To:", 3) == 0 ||
			    strncmp(str, "Date:", 5) == 0 ||
			    strncmp(str, "Subject:", 8) == 0) break;
		    }
		    startPos = (*msg->source->scan)
			(msg->source, startPos, XtstEOL, XtsdRight, 1,TRUE);
 		}
	    }
	    if (startPos >= lastPos) startPos = 0;
	    XtTextNewSource(DISPLAY scrn->viewwindow, msg->source, startPos);
	    if (msg->startPos > 0) {
#ifdef X10
	    /* Do an incredibly disgusting hack to make things display right.
	       The X10 toolkit stinks. */
		MapScrn(scrn);
		EmptyEventQueue();
#endif X10
		XtTextSetInsertionPoint(DISPLAY scrn->viewwindow,
					msg->startPos);
		msg->startPos = 0; /* Start in magic place only once. */
	    }
	} else {
	    XtTextNewSource(DISPLAY scrn->viewwindow,
			    NullSource, (XtTextPosition) 0);
 	}
    }
}



static char tempDraftFile[100] = "";

/* Temporarily move the draftfile somewhere else, so we can exec an mh
   command that affects it. */

static void TempMoveDraft()
{
    char *ptr;
    if (FileExists(draftFile)) {
	do {
	    ptr = MakeNewTempFileName();
	    (void) strcpy(tempDraftFile, draftFile);
	    (void) strcpy(rindex(tempDraftFile, '/'), rindex(ptr, '/'));
	} while (FileExists(tempDraftFile));
	RenameAndCheck(draftFile, tempDraftFile);
    }
}



/* Restore the draftfile from its temporary hiding place. */

static void RestoreDraft()
{
    if (*tempDraftFile) {
	RenameAndCheck(tempDraftFile, draftFile);
	*tempDraftFile = 0;
    }
}



/* Public routines */


/* Given a message, return the corresponding filename. */

char *MsgFileName(msg)
Msg msg;
{
    static char result[500];
    (void) sprintf(result, "%s/%d", msg->toc->path, msg->msgid);
    return result;
}



/* Save any changes to a message.  Also calls the toc routine to update the
   scanline for this msg. */

void MsgSaveChanges(msg)
Msg msg;
{
    if (msg->source) {
	XtEDiskSaveFile(msg->source);
	EnableProperButtons(msg->scrn);
	if (!msg->temporary)
	    TocMsgChanged(msg->toc, msg);
    }
}

/*ARGSUSED*/
static Boolean IfMapWindow(dpy, event, scrn)
Display *dpy;
XEvent *event;
Scrn scrn;
{
    return (event->type == MapNotify && event->xany.window == scrn->window);
}

/* Associate the given message with the given scrn.  If a message is
   changed, and we are removing it from any scrn, then ask for confirmation
   first.  If the message is a temporary one and it is removed from any scrn,
   it is deleted. */

static int SetScrn(msg, scrn, force)
Msg msg;
Scrn scrn;
Boolean force;	/* If TRUE, don't ask for confirm; just do it */
{
    char str[100];
    if (msg == NULL && scrn == NULL) return 0;
    if (scrn && scrn->msg != msg)
	if (SetScrn(scrn->msg, (Scrn) NULL, force))
	    return DELETEABORTED;
    if (msg == NULL)
	return 0;
    if (msg->scrn == scrn) return 0;
    if (msg->scrn) {
	if (msg->source && scrn == NULL) {
	    if (XtEDiskChanged(msg->source) && !force) {
		(void)sprintf(str,
			      "Are you sure you want to remove changes to %s?",
			      NameOfMsg(msg));
		if (!Confirm(msg->scrn, str)) return DELETEABORTED;
	    }
	    XtDestroyEDiskSource(msg->source);
	    msg->source = NULL;
	}
	msg->scrn->msg = NULL;
	ResetMsgLabel(msg->scrn);
	RedisplayMsg(msg->scrn);
	EnableProperButtons(msg->scrn);
	if (msg->scrn->kind != STtocAndView)
	    QXStoreName(theDisplay, msg->scrn->window, progName);
	msg->scrn = NULL;
	if (scrn == NULL) {
	    if (msg->temporary) {
		(void) unlink(MsgFileName(msg));
		TocRemoveMsg(msg->toc, msg);
		MsgFree(msg);
	    }		
	    return 0;
	}
    }
    if (msg->source == NULL)
	msg->source = XtCreateEDiskSource(MsgFileName(msg), XttextRead);
    if (scrn->kind == STcomp) {
	XtEDiskChangeEditMode(msg->source, XttextEdit);
	if (defGrabFocus) {
	    XEvent event;
	    QXMapWindow(theDisplay, scrn->window);
	    XPeekIfEvent(theDisplay, &event, IfMapWindow, (char *)scrn);
	    QXSetInputFocus(theDisplay, scrn->viewwindow,
			    RevertToParent, CurrentTime);
	}
    }

    msg->scrn = scrn;
    scrn->msg = msg;
    ResetMsgLabel(msg->scrn);
    RedisplayMsg(msg->scrn);
    EnableProperButtons(msg->scrn);
    if (msg->scrn->kind != STtocAndView)
	QXStoreName(theDisplay, msg->scrn->window, NameOfMsg(msg));
    return 0;
}


/* Associate the given msg and scrn, asking for confirmation if necessary. */

int MsgSetScrn(msg, scrn)
Msg msg;
Scrn scrn;
{
    return SetScrn(msg, scrn, FALSE);
}


/* Same as above, but with the extra information that the message is actually
   a composition.  (Nothing currently takes advantage of that extra fact.) */

int MsgSetScrnForComp(msg, scrn)
Msg msg;
Scrn scrn;
{
    return SetScrn(msg, scrn, FALSE);
}



/* Associate the given msg and scrn, even if it means losing some unsaved
   changes. */

void MsgSetScrnForce(msg, scrn)
Msg msg;
Scrn scrn;
{
    (void) SetScrn(msg, scrn, TRUE);
}



/* Return what screen (if any) is displaying the given msg. */

Scrn MsgGetScrn(msg)
Msg msg;
{
    return msg->scrn;
}



/* Set the fate of the given message. */

void MsgSetFate(msg, fate, desttoc)
  Msg msg;
  FateType fate;
  Toc desttoc;
{
    Toc toc = msg->toc;
    XtTextBlock text;
    msg->fate = fate;
    msg->desttoc = desttoc;
    if (fate == Fignore && msg == msg->toc->curmsg)
	text.ptr = "+";
    else {
	switch (fate) {
	    case Fignore:	text.ptr = " "; break;
	    case Fcopy:		text.ptr = "C"; break;
	    case Fmove:		text.ptr = "^"; break;
	    case Fdelete:	text.ptr = "D"; break;
	}
    }
    text.length = 1;
    text.firstPos = msg->position + MARKPOS;
    if (toc->stopupdate)
	toc->needsrepaint = TRUE;
    if (toc->scrn && msg->visible && !toc->needsrepaint &&
	    *text.ptr != msg->buf[MARKPOS])
	(void) XtTextReplace(DISPLAY toc->scrn->tocwindow,
			     msg->position + MARKPOS,
			     msg->position + MARKPOS + 1, &text);
    else
	msg->buf[MARKPOS] = *text.ptr;
    if (msg->scrn)
	ResetMsgLabel(msg->scrn);
}



/* Get the fate of this message. */

FateType MsgGetFate(msg, toc)
Msg msg;
Toc *toc;			/* RETURN */
{
    if (toc) *toc = msg->desttoc;
    return msg->fate;
}


/* Make this a temporary message. */

void MsgSetTemporary(msg)
Msg msg;
{
    msg->temporary = TRUE;
    ResetMsgLabel(msg->scrn);
}


/* Make this a permanent message. */

void MsgSetPermanent(msg)
Msg msg;
{
    msg->temporary = FALSE;
    ResetMsgLabel(msg->scrn);
}



/* Return the id# of this message. */

int MsgGetId(msg)
Msg msg;
{
    return msg->msgid;
}


/* Return the scanline for this message. */

char *MsgGetScanLine(msg)
Msg msg;
{
    return msg->buf;
}



/* Return the toc this message is in. */

Toc MsgGetToc(msg)
Msg msg;
{
    return msg->toc;
}


/* Set the reapable flag for this msg. */

void MsgSetReapable(msg)
Msg msg;
{
    msg->reapable = TRUE;
    EnableProperButtons(msg->scrn);
}



/* Clear the reapable flag for this msg. */

void MsgClearReapable(msg)
Msg msg;
{
    msg->reapable = FALSE;
    EnableProperButtons(msg->scrn);
}


/* Get the reapable value for this msg.  Returns TRUE iff the reapable flag
   is set AND no changes have been made. */

int MsgGetReapable(msg)
Msg msg;
{
    return msg == NULL || (msg->reapable &&
			   (msg->source == NULL ||
			    !XtEDiskChanged(msg->source)));
}


/* Make it possible to edit the given msg. */
void MsgSetEditable(msg)
Msg msg;
{
    if (msg && msg->source) {
	XtEDiskChangeEditMode(msg->source, XttextEdit);
	if (defGrabFocus && msg->scrn)
	    QXSetInputFocus(theDisplay, msg->scrn->viewwindow,
			    RevertToParent, CurrentTime);
	EnableProperButtons(msg->scrn);
    }
}



/* Turn off editing for the given msg. */

void MsgClearEditable(msg)
Msg msg;
{
    if (msg && msg->source) {
	XtEDiskChangeEditMode(msg->source, XttextRead);
	EnableProperButtons(msg->scrn);
    }
}



/* Get whether the msg is editable. */

int MsgGetEditable(msg)
Msg msg;
{
    return msg && msg->source &&
	(*msg->source->editType)(msg->source) == XttextEdit;
}


/* Get whether the msg has changed since last saved. */

int MsgChanged(msg)
Msg msg;
{
    return msg && msg->source && XtEDiskChanged(msg->source);
}



/* Call the given function when the msg changes. */

void MsgSetCallOnChange(msg, func, param)
Msg msg;
void (*func)();
caddr_t param;
{
    XtEDiskSetCallbackWhenChanged(msg->source, func, param);
}



/* Call no function when the msg changes. */

void MsgClearCallOnChange(msg)
Msg msg;
{
    XtEDiskSetCallbackWhenChanged(msg->source, NULL, (caddr_t) NULL);
}


/* Send (i.e., mail) the given message as is.  First break it up into lines,
   and copy it to a new file in the process.  The new file is one of 10
   possible draft files; we rotate amoung the 10 so that the user can have up
   to 10 messages being sent at once.  (Using a file in /tmp is a bad idea
   because these files never actually get deleted, but renamed with some
   prefix.  Also, these should stay in an area private to the user for
   security.) */

void MsgSend(msg)
Msg msg;
{
    FILEPTR from;
    FILEPTR to;
    int     p, c, l, inheader, sendwidth, sendbreakwidth;
    char   *ptr, *ptr2, **argv, str[100];
    static sendcount = -1;
    MsgSaveChanges(msg);
    from = FOpenAndCheck(MsgFileName(msg), "r");
    sendcount = (sendcount + 1) % 10;
    (void) sprintf(str, "%s%d", xmhDraftFile, sendcount);
    to = FOpenAndCheck(str, "w");
    sendwidth = defSendLineWidth;
    sendbreakwidth = defBreakSendLineWidth;
    inheader = TRUE;
    while (ptr = ReadLine(from)) {
	if (inheader) {
	    if (strncmpIgnoringCase(ptr, "sendwidth:", 10) == 0) {
		if (atoi(ptr+10) > 0) sendwidth = atoi(ptr+10);
		continue;
	    }
	    if (strncmpIgnoringCase(ptr, "sendbreakwidth:", 15) == 0) {
		if (atoi(ptr+15) > 0) sendbreakwidth = atoi(ptr+15);
		continue;
	    }
	    for (l = 0, ptr2 = ptr ; *ptr2 && !l ; ptr2++)
		l = (*ptr2 != ' ' && *ptr2 != '\t' && *ptr != '-');
	    if (l) {
		(void) fprintf(to, "%s\n", ptr);
		continue;
	    }
	    inheader = FALSE;
	    if (sendbreakwidth < sendwidth) sendbreakwidth = sendwidth;
	}
	do {
	    for (p = c = l = 0, ptr2 = ptr;
		 *ptr2 && c < sendbreakwidth;
		 p++, ptr2++) {
		 if (*ptr2 == ' ' && c < sendwidth)
		     l = p;
		 if (*ptr2 == '\t') {
		     if (c < sendwidth) l = p;
		     c += 8 - (c % 8);
		 }
		 else
		 c++;
	     }
	    if (c < sendbreakwidth) {
		(void) fprintf(to, "%s\n", ptr);
		*ptr = 0;
	    }
	    else
		if (l) {
		    ptr[l] = 0;
		    (void) fprintf(to, "%s\n", ptr);
		    ptr += l + 1;
		}
		else {
		    for (c = 0; c < sendwidth; ) {
			if (*ptr == '\t') c += 8 - (c % 8);
			else c++;
			(void) fputc(*ptr++, to);
		    }
		    (void) fputc('\n', to);
		}
	} while (*ptr);
    }
    (void) myfclose(from);
    (void) myfclose(to);
    argv = MakeArgv(3);
    argv[0] = "send";
    argv[1] = "-push";
    argv[2] = str;
    DoCommand(argv, (char *) NULL, (char *) NULL);
    XtFree((char *) argv);
}


/* Make the msg into the form for a generic composition.  Set msg->startPos
   so that the text insertion point will be placed at the end of the first
   line (which is usually the "To:" field). */

void MsgLoadComposition(msg)
Msg msg;
{
    static char *blankcomp = NULL; /* Array containing comp template */
    static int compsize = 0;
    static XtTextPosition startPos;
    char *file, **argv;
    int fid;
    if (blankcomp == NULL) {
	file = MakeNewTempFileName();
	argv = MakeArgv(4);
	argv[0] = "comp";
	argv[1] = "-file";
	argv[2] = file;
	argv[3] = "-nowhatnowproc";
	DoCommand(argv, (char *) NULL, "/dev/null");
	XtFree((char *) argv);
	compsize = GetFileLength(file);
	blankcomp = XtMalloc((unsigned) compsize);
	fid = myopen(file, O_RDONLY, 0666);
	if (compsize != read(fid, blankcomp, compsize))
	    Punt("Error reading in MsgLoadComposition!");
	(void) myclose(fid);
	DeleteFileAndCheck(file);
	startPos = index(blankcomp, '\n') - blankcomp;
    }
    fid = myopen(MsgFileName(msg), O_WRONLY | O_TRUNC | O_CREAT, 0666);
    if (compsize != write(fid, blankcomp, compsize))
	Punt("Error writing in MsgLoadComposition!");
    (void) myclose(fid);
    TocSetCacheValid(msg->toc);
    msg->startPos = startPos;
}



/* Load a msg with a template of a reply to frommsg.  Set msg->startPos so
   that the text insertion point will be placed at the beginning of the
   message body. */

void MsgLoadReply(msg, frommsg)
Msg msg, frommsg;
{
    char **argv;
    char str1[100], str2[10];
    TempMoveDraft();
    argv = MakeArgv(4);
    argv[0] = "repl";
    (void) sprintf(str1, "+%s", frommsg->toc->foldername);
    argv[1] = str1;
    (void) sprintf(str2, "%d", frommsg->msgid);
    argv[2] = str2;
    argv[3] = "-nowhatnowproc";
    DoCommand(argv, (char *) NULL, "/dev/null");
    RenameAndCheck(draftFile, MsgFileName(msg));
    RestoreDraft();
    TocSetCacheValid(frommsg->toc); /* If -anno is set, this keeps us from
				       rescanning folder. */
    TocSetCacheValid(msg->toc);
    msg->startPos = GetFileLength(MsgFileName(msg));
}



/* Load a msg with a template of forwarding a list of messages.  Set 
   msg->startPos so that the text insertion point will be placed at the end
   of the first line (which is usually a "To:" field). */

void MsgLoadForward(msg, mlist)
  Msg msg;
  MsgList mlist;
{
    char  **argv, str[100];
    int     i;
    TempMoveDraft();
    argv = MakeArgv(3 + mlist->nummsgs);
    argv[0] = "forw";
    (void) sprintf(str, "+%s", mlist->msglist[0]->toc->foldername);
    argv[1] = MallocACopy(str);
    for (i = 0; i < mlist->nummsgs; i++) {
        (void) sprintf(str, "%d", mlist->msglist[i]->msgid);
        argv[2 + i] = MallocACopy(str);
    }
    argv[2 + i] = "-nowhatnowproc";
    DoCommand(argv, (char *) NULL, "/dev/null");
    for (i = 1; i < 2 + mlist->nummsgs; i++)
        XtFree((char *) argv[i]);
    XtFree((char *) argv);
    RenameAndCheck(draftFile, MsgFileName(msg));
    RestoreDraft();
    TocSetCacheValid(msg->toc);
    msg->source = XtCreateEDiskSource(MsgFileName(msg), XttextEdit);
    msg->startPos = (*msg->source->scan)(msg->source, 0, XtstEOL, XtsdRight,
					 1, FALSE);
}


/* Load msg with a copy of frommsg. */

void MsgLoadCopy(msg, frommsg)
Msg msg, frommsg;
{
    char str[500];
    (void)strcpy(str, MsgFileName(msg));
    CopyFileAndCheck(MsgFileName(frommsg), str);
    TocSetCacheValid(msg->toc);
}



/* Checkpoint the given message. */

void MsgCheckPoint(msg)
Msg msg;
{
    if (msg && msg->source) {
	XtEDiskMakeCheckpoint(msg->source);
	TocSetCacheValid(msg->toc);
    }
}


/* Free the storage being used by the given msg. */

void MsgFree(msg)
Msg msg;
{
    XtFree(msg->buf);
    XtFree((char *)msg);
}
