#ifndef lint
static char rcs_id[] = "$Header: tocutil.c,v 1.14 87/09/11 08:19:27 toddb Exp $";
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

/* tocutil.c -- internal routines for toc stuff. */

#include "xmh.h"
#include "toc.h"
#include "tocutil.h"
#include "tocintrnl.h"
#include <sys/file.h>

Toc TUMalloc()
{
    Toc toc;
    toc = (Toc) XtMalloc(sizeof(TocRec));
    bzero((char *)toc, (int) sizeof(TocRec));
    toc->msgs = (Msg *) XtMalloc(1);
    toc->seqlist = (Sequence *) XtMalloc(1);
    toc->validity = unknown;
    return toc;
}


/* Returns TRUE if the scan file for the given toc is out of date. */

int TUScanFileOutOfDate(toc)
  Toc toc;
{
    return LastModifyDate(toc->path) > toc->lastreaddate;
}


/* Make sure the shown sequence buttons correspond exactly to the sequences
   for this toc.  If not, then rebuild the buttonbox. */

static void CheckSeqButtons(toc)
Toc toc;
{
    Scrn scrn = toc->scrn;
    int i, numinbox;
    int rebuild;
    extern void PrepareDoubleClickSequence();
    if (scrn == NULL) return;
    rebuild = FALSE;
    numinbox = BBoxNumButtons(scrn->seqbuttons);
    if (numinbox != toc->numsequences)
	rebuild = TRUE;
    else {
	for (i=0 ; i<toc->numsequences && !rebuild; i++)
	    rebuild =
		strcmp(toc->seqlist[i]->name,
		       BBoxNameOfButton(BBoxButtonNumber(scrn->seqbuttons,
							 i)));
    }
    if (rebuild) {
	BBoxStopUpdate(scrn->seqbuttons);
	for (i = 1; i < numinbox ; i++)
	    BBoxDeleteButton(BBoxButtonNumber(scrn->seqbuttons, 1));
	for (i = (numinbox ? 1 : 0); i < toc->numsequences; i++)
	    BBoxAddButton(scrn->seqbuttons, toc->seqlist[i]->name,
			  PrepareDoubleClickSequence, 999, TRUE);
	BBoxStartUpdate(scrn->seqbuttons);
    }
    if (scrn->seqbuttons) 
	BBoxSetRadio(scrn->seqbuttons,
		     BBoxFindButtonNamed(scrn->seqbuttons,
					 toc->viewedseq->name));
}



void TUScanFileForToc(toc)
  Toc toc;
{
    static Arg arglist[] = {
	{XtNlabel, NULL},
	{XtNx, (XtArgVal) 30},
	{XtNy, (XtArgVal) 30}
    };

    Window parent, label;
    Scrn scrn;
    char  **argv, str[100], str2[10];
    if (toc) {
	TUGetFullFolderInfo(toc);
	scrn = toc->scrn;
	if (!scrn) scrn = scrnList[0];
	parent = scrn->tocwindow;
	if (!parent) parent = scrn->window;
	(void) sprintf(str, "Rescanning %s", toc->foldername);
	arglist[0].value = (XtArgVal) str;
	label = XtLabelCreate(DISPLAY parent, arglist, XtNumber(arglist));
	QXMapWindow(theDisplay, label);
	(void) XtSendExpose(DISPLAY label);
	QXFlush(theDisplay);

	argv = MakeArgv(4);
	argv[0] = "scan";
	(void) sprintf(str, "+%s", toc->foldername);
	argv[1] = str;
	argv[2] = "-width";
	(void) sprintf(str2, "%d", defTocWidth);
	argv[3] = str2;
	DoCommand(argv, (char *) NULL, toc->scanfile);
	XtFree((char *) argv);

	(void) XtSendDestroyNotify(DISPLAY label);
	QXDestroyWindow(theDisplay, label);
	toc->validity = valid;
	toc->curmsg = NULL;	/* Get cur msg somehow! %%% */
    }
}



int TUGetMsgPosition(toc, msg)
  Toc toc;
  Msg msg;
{
    int msgid, h, l, m;
    char str[100];
    msgid = msg->msgid;
    l = 0;
    h = toc->nummsgs - 1;
    while (l < h - 1) {
	m = (l + h) / 2;
	if (toc->msgs[m]->msgid > msgid)
	    h = m;
	else
	    l = m;
    }
    if (toc->msgs[l] == msg) return l;
    if (toc->msgs[h] == msg) return h;
    (void) sprintf(str, "TUGetMsgPosition search failed! hi=%d, lo=%d, msgid=%d",
		   h, l, msgid);
    Punt(str);
    return 0; /* Keep lint happy. */
}


void TUResetTocLabel(scrn)
  Scrn scrn;
{
    char str[500];
    Toc toc;
    if (scrn) {
	toc = scrn->toc;
	if (toc == NULL)
	    (void) strcpy(str, " ");
	else {
	    if (toc->stopupdate) {
		toc->needslabelupdate = TRUE;
		return;
	    }
	    (void) sprintf(str, "%s:%s", toc->foldername,
			   toc->viewedseq->name);
	    toc->needslabelupdate = FALSE;
	}
	ChangeLabel(scrn->toclabel, str);
    }
}


/* A major toc change has occured; redisplay it.  (This also should work even
   if we now have a new source to display stuff from.) */

void TURedisplayToc(scrn)
  Scrn scrn;
{
    Toc toc;
    int lines, width, height;
    XtTextPosition position;
    if (scrn != NULL && scrn->tocwindow != NULL) {
	toc = scrn->toc;
 	if (toc) {
	    if (toc->stopupdate) {
		toc->needsrepaint = TRUE;
		return;
	    }
	    GetWindowSize(scrn->tocwindow, &width, &height);
	    lines = scrn->tocsink->maxLines(scrn->tocsink, height);
	    position = toc->source->getLastPos(toc->source);
	    position = toc->source->scan(toc->source, position, XtstEOL,
					 XtsdLeft, lines, FALSE);
	    XtTextNewSource(DISPLAY scrn->tocwindow, toc->source, position);
	    TocSetCurMsg(toc, TocGetCurMsg(toc));
	    CheckSeqButtons(toc);
	    toc->needsrepaint = FALSE;
	} else {
	    XtTextNewSource(DISPLAY scrn->tocwindow, NullSource, (XtTextPosition) 0);
	}
    }
}



void TULoadSeqLists(toc)
  Toc toc;
{
    Sequence seq;
    FILEPTR fid;
    char    str[500], *ptr, *ptr2, viewed[500];
    int     i;
    if (toc->viewedseq) (void) strcpy(viewed, toc->viewedseq->name);
    else *viewed = 0;
    for (i = 0; i < toc->numsequences; i++) {
	seq = toc->seqlist[i];
	XtFree((char *) seq->name);
	if (seq->mlist) FreeMsgList(seq->mlist);
	XtFree((char *)seq);
    }
    toc->numsequences = 1;
    toc->seqlist = (Sequence *) XtRealloc((char *) toc->seqlist,
					       sizeof(Sequence));
    seq = toc->seqlist[0] = (Sequence) XtMalloc(sizeof(SequenceRec));
    bzero((char *) seq, sizeof(SequenceRec));
    seq->name = MallocACopy("all");
    seq->mlist = NULL;
    toc->viewedseq = seq;
    (void) sprintf(str, "%s/.mh_sequences", toc->path);
    fid = myfopen(str, "r");
    if (fid) {
	while (ptr = ReadLine(fid)) {
	    ptr2 = index(ptr, ':');
	    if (ptr2) {
		*ptr2 = 0;
		if (strcmp(ptr, "all") != 0 &&
		    strcmp(ptr, "cur") != 0 &&
		    strcmp(ptr, "unseen") != 0) {
		    toc->numsequences++;
		    toc->seqlist = (Sequence *)
			XtRealloc((char *) toc->seqlist,
				  (unsigned) toc->numsequences * sizeof(Sequence));
		    seq = toc->seqlist[toc->numsequences - 1] =
			(Sequence) XtMalloc(sizeof(SequenceRec));
		    bzero((char *) seq, sizeof(SequenceRec));
		    seq->name = MallocACopy(ptr);
		    seq->mlist = StringToMsgList(toc, ptr2 + 1);
		    if (strcmp(seq->name, viewed) == 0) {
			toc->viewedseq = seq;
			*viewed = 0;
		    }
		}
	    }
	}
	(void) myfclose(fid);
    }
}



/* Refigure what messages are visible.  Also makes sure we're displaying the
   correct set of seq buttons. */

void TURefigureWhatsVisible(toc)
Toc toc;
{
    MsgList mlist;
    Msg msg, oldcurmsg;
    int     i, w, changed, newval, msgid;
    Sequence seq = toc->viewedseq;
    mlist = seq->mlist;
    oldcurmsg = toc->curmsg;
    TocSetCurMsg(toc, (Msg)NULL);
    w = 0;
    changed = FALSE;
    CheckSeqButtons(toc);
    for (i = 0; i < toc->nummsgs; i++) {
	msg = toc->msgs[i];
	msgid = msg->msgid;
	while (mlist && mlist->msglist[w] && mlist->msglist[w]->msgid < msgid)
	    w++;
	newval = (!mlist || mlist->msglist[w]->msgid == msgid);
	if (newval != msg->visible) {
	    changed = TRUE;
	    msg->visible = newval;
	}
    }
    if (changed) {
	TURefigureTocPositions(toc);
	if (oldcurmsg) {
	    if (!oldcurmsg->visible) {
		toc->curmsg = TocMsgAfter(toc, oldcurmsg);
		if (toc->curmsg == NULL)
		    toc->curmsg = TocMsgBefore(toc, oldcurmsg);
	    } else toc->curmsg = oldcurmsg;
	}
	TURedisplayToc(toc->scrn);
    } else TocSetCurMsg(toc, oldcurmsg);
    TUResetTocLabel(toc->scrn);
}


/* (Re)load the toc from the scanfile.  If reloading, this makes efforts to
   keep the fates of msgs, and to keep msgs that are being edited.  Note that
   this routine must know of all places that msg ptrs are stored; it expects
   them to be kept only in tocs, in scrns, and in msg sequences. */

#define SeemsIdentical(msg1, msg2) ((msg1)->msgid == (msg2)->msgid &&	      \
				    ((msg1)->temporary || (msg2)->temporary ||\
				     strcmp((msg1)->buf, (msg2)->buf) == 0))

void TULoadTocFile(toc)
  Toc toc;
{
    int maxmsgs, l, orignummsgs, i, j, origcurmsgid;
    FILEPTR fid;
    XtTextPosition position;
    char *ptr;
    Msg msg, curmsg;
    Msg *origmsgs;

    TocStopUpdate(toc);
    toc->lastreaddate = CurrentDate();
    if (toc->curmsg) {
	origcurmsgid = toc->curmsg->msgid;
	TocSetCurMsg(toc, (Msg)NULL);
    } else origcurmsgid = 0;
    fid = FOpenAndCheck(toc->scanfile, "r");
    maxmsgs = 10;
    orignummsgs = toc->nummsgs;
    toc->nummsgs = 0;
    origmsgs = toc->msgs;
    toc->msgs = (Msg *) XtMalloc((unsigned) maxmsgs * sizeof(Msg));
    position = 0;
    i = 0;
    curmsg = NULL;
    while (ptr = ReadLineWithCR(fid)) {
	toc->msgs[toc->nummsgs++] = msg = (Msg) XtMalloc(sizeof(MsgRec));
	bzero((char *) msg, sizeof(MsgRec));
	l = strlen(ptr);
	msg->toc = toc;
	msg->position = position;
	msg->length = l;
	msg->buf = MallocACopy(ptr);
	msg->msgid = atoi(ptr);
	if (msg->msgid == origcurmsgid)
	    curmsg = msg;
	msg->buf[MARKPOS] = ' ';
	position += l;
	msg->changed = FALSE;
	msg->fate = Fignore;
	msg->desttoc = NULL;
	msg->visible = TRUE;
	if (toc->nummsgs >= maxmsgs) {
	    maxmsgs += 100;
	    toc->msgs = (Msg *) XtRealloc((char *) toc->msgs,
					  (unsigned) maxmsgs * sizeof(Msg));
	}
	while (i < orignummsgs && origmsgs[i]->msgid < msg->msgid) i++;
	if (i < orignummsgs) {
	    origmsgs[i]->buf[MARKPOS] = ' ';
	    if (SeemsIdentical(origmsgs[i], msg))
		MsgSetFate(msg, origmsgs[i]->fate, origmsgs[i]->desttoc);
	}
    }
    toc->length = toc->origlength = toc->lastPos = position;
    toc->msgs = (Msg *) XtRealloc((char *) toc->msgs,
				  (unsigned) toc->nummsgs * sizeof(Msg));
    (void) myfclose(fid);
    if (toc->source == NULL)
	toc->source = TSourceCreate(toc);
    for (i=0 ; i<numScrns ; i++) {
	msg = scrnList[i]->msg;
	if (msg && msg->toc == toc) {
	    for (j=0 ; j<toc->nummsgs ; j++) {
		if (SeemsIdentical(toc->msgs[j], msg)) {
		    msg->position = toc->msgs[j]->position;
		    msg->visible = TRUE;
		    ptr = toc->msgs[j]->buf;
		    *(toc->msgs[j]) = *msg;
		    toc->msgs[j]->buf = ptr;
		    scrnList[i]->msg = toc->msgs[j];
		    break;
		}
	    }
	    if (j >= toc->nummsgs) {
		msg->temporary = FALSE;	/* Don't try to auto-delete msg. */
		MsgSetScrnForce(msg, (Scrn) NULL);
	    }
	}
    }
    for (i=0 ; i<orignummsgs ; i++)
	MsgFree(origmsgs[i]);
    XtFree((char *)origmsgs);
    TocSetCurMsg(toc, curmsg);
    TULoadSeqLists(toc);
    TocStartUpdate(toc);
}


void TUSaveTocFile(toc)
  Toc toc;
{
    extern long lseek();
    Msg msg;
    int fid;
    int i;
    XtTextPosition position;
    char c;
    if (toc->stopupdate) {
	toc->needscachesave = TRUE;
	return;
    }
    fid = -1;
    position = 0;
    for (i = 0; i < toc->nummsgs; i++) {
	msg = toc->msgs[i];
	if (fid < 0 && msg->changed) {
	    fid = myopen(toc->scanfile, O_RDWR, 0666);
	    (void) lseek(fid, (long)position, 0);
	}
	if (fid >= 0) {
	    c = msg->buf[MARKPOS];
	    msg->buf[MARKPOS] = ' ';
	    (void) write(fid, msg->buf, msg->length);
	    msg->buf[MARKPOS] = c;
	}
	position += msg->length;
    }
    if (fid < 0 && toc->length != toc->origlength)
	fid = myopen(toc->scanfile, O_RDWR, 0666);
    if (fid >= 0) {
	(void) ftruncate(fid, toc->length);
	toc->origlength = toc->length;
	(void) myclose(fid);
    } else
	(void) utime(toc->scanfile, (time_t *)NULL);
    toc->needscachesave = FALSE;
    toc->lastreaddate = CurrentDate();
}


void TUEnsureScanIsValidAndOpen(toc)
  Toc toc;
{
    if (toc) {
	TUGetFullFolderInfo(toc);
	if (TUScanFileOutOfDate(toc)) {
	    if (toc->source) {
		XtFree((char *) toc->source);
		toc->source = NULL;
	    }
	    TUScanFileForToc(toc);
	}
	if (toc->source == NULL)
	    TULoadTocFile(toc);
	toc->validity = valid;
    }
}



/* Refigure all the positions, based on which lines are visible. */

void TURefigureTocPositions(toc)
  Toc toc;
{
    int i;
    Msg msg;
    XtTextPosition position, length;
    position = length = 0;
    for (i=0; i<toc->nummsgs ; i++) {
	msg = toc->msgs[i];
	msg->position = position;
	if (msg->visible) position += msg->length;
	length += msg->length;
    }
    toc->lastPos = position;
    toc->length = length;
}



/* Make sure we've loaded ALL the folder info for this toc, including its
   path and sequence lists. */

void TUGetFullFolderInfo(toc)
  Toc toc;
{
    char str[500];
    if (toc->path == NULL) {
	(void) sprintf(str, "%s/%s", mailDir, toc->foldername);
	toc->path = MallocACopy(str);
	(void) sprintf(str, "%s/.xmhcache", toc->path);
	toc->scanfile = MallocACopy(str);
	toc->lastreaddate = LastModifyDate(toc->scanfile);
	if (TUScanFileOutOfDate(toc))
	    toc->validity = invalid;
	else {
	    toc->validity = valid;
	    TULoadTocFile(toc);
	}
    }
}

/* Append a message to the end of the toc.  It has the given scan line.  This
   routine will figure out the message number, and change the scan line
   accordingly. */

Msg TUAppendToc(toc, ptr)
  Toc toc;
  char *ptr;
{
    char str[10];
    Msg msg;
    int msgid, i;
    TUGetFullFolderInfo(toc);
    if (toc->validity != valid)
	return NULL;
	    
    if (toc->nummsgs > 0)
	msgid = toc->msgs[toc->nummsgs - 1]->msgid + 1;
    else
	msgid = 1;
    (toc->nummsgs)++;
    toc->msgs = (Msg *) XtRealloc((char *) toc->msgs,
				  (unsigned) toc->nummsgs * sizeof(Msg));
    toc->msgs[toc->nummsgs - 1] = msg = (Msg) XtMalloc(sizeof(MsgRec));
    bzero((char *) msg, (int) sizeof(MsgRec));
    msg->toc = toc;
    msg->buf = MallocACopy(ptr);
    (void)sprintf(str, "%4d", msgid);
    for (i=0; i<4 ; i++) msg->buf[i] = str[i];
    msg->buf[MARKPOS] = ' ';
    msg->msgid = msgid;
    msg->position = toc->lastPos;
    msg->length = strlen(ptr);
    msg->changed = TRUE;
    msg->fate = Fignore;
    msg->desttoc = NULL;
    if (toc->viewedseq == toc->seqlist[0]) {
	msg->visible = TRUE;
	toc->lastPos += msg->length;
    }
    else
	msg->visible = FALSE;
    toc->length += msg->length;
    TURedisplayToc(toc->scrn);
    TUSaveTocFile(toc);
    return msg;
}
