#ifndef lint
static char rcs_id[] = "$Header: tocfuncs.c,v 1.13 87/09/11 08:18:56 toddb Exp $";
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

/* tocfuncs.c -- handle things in the toc window. */

#include "xmh.h"


void ExecNextView(scrn)
  Scrn scrn;
{
    Toc toc = scrn->toc;
    MsgList mlist;
    FateType fate;
    Msg msg;
    if (toc == NULL) return;
    mlist = TocCurMsgList(toc);
    if (mlist->nummsgs)
	msg = mlist->msglist[0];
    else {
	msg = TocGetCurMsg(toc);
	if (msg && msg == scrn->msg) msg = TocMsgAfter(toc, msg);
	if (msg) fate = MsgGetFate(msg, (Toc *)NULL);
	while (msg && ((SkipDeleted && fate == Fdelete)
		|| (SkipMoved && fate == Fmove)
		|| (SkipCopied && fate == Fcopy))) {
	    msg = TocMsgAfter(toc, msg);
	    if (msg) fate = MsgGetFate(msg, (Toc *)NULL);
	}
    }
    if (msg) {
	if (!MsgSetScrn(msg, scrn)) {
	    TocUnsetSelection(toc);
	    TocSetCurMsg(toc, msg);
	}
    }
    FreeMsgList(mlist);
}



void ExecPrevView(scrn)
  Scrn scrn;
{
    Toc toc = scrn->toc;
    MsgList mlist;
    FateType fate;
    Msg msg;
    if (toc == NULL) return;
    mlist = TocCurMsgList(toc);
    if (mlist->nummsgs)
	msg = mlist->msglist[mlist->nummsgs - 1];
    else {
	msg = TocGetCurMsg(toc);
	if (msg && msg == scrn->msg) msg = TocMsgBefore(toc, msg);
	if (msg) fate = MsgGetFate(msg, (Toc *)NULL);
	while (msg && ((SkipDeleted && fate == Fdelete)
		|| (SkipMoved && fate == Fmove)
		|| (SkipCopied && fate == Fcopy))) {
	    msg = TocMsgBefore(toc, msg);
	    if (msg) fate = MsgGetFate(msg, (Toc *)NULL);
	}
    }
    if (msg) {
	if (!MsgSetScrn(msg, scrn)) {
	    TocUnsetSelection(toc);
	    TocSetCurMsg(toc, msg);
	}
    }
    FreeMsgList(mlist);
}



void ExecViewNew(scrn)
Scrn scrn;
{
    Toc toc = scrn->toc;
    Scrn vscrn;
    MsgList mlist;
    if (toc == NULL) return;
    mlist = CurMsgListOrCurMsg(toc);
    if (mlist->nummsgs) {
	vscrn = NewViewScrn();
	(void) MsgSetScrn(mlist->msglist[0], vscrn);
	MapScrn(vscrn);
    }
    FreeMsgList(mlist);
}



void ExecTocForward(scrn)
Scrn scrn;
{
    Toc toc = scrn->toc;
    MsgList mlist;
    if (toc == NULL) return;
    mlist = CurMsgListOrCurMsg(toc);
    if (mlist->nummsgs)
	CreateForward(mlist);
    FreeMsgList(mlist);
}


void ExecTocUseAsComposition(scrn)
Scrn scrn;
{
    Toc toc = scrn->toc;
    Scrn vscrn;
    MsgList mlist;
    Msg msg;
    if (toc == NULL) return;
    mlist = CurMsgListOrCurMsg(toc);
    if (mlist->nummsgs) {
	vscrn = NewCompScrn();
	if (DraftsFolder == toc) {
	    msg = mlist->msglist[0];
	} else {
	    msg = TocMakeNewMsg(DraftsFolder);
	    MsgLoadCopy(msg, mlist->msglist[0]);
	    MsgSetTemporary(msg);
	}
	(void)MsgSetScrnForComp(msg, vscrn);
	MapScrn(vscrn);
    }
    FreeMsgList(mlist);
}



/* Utility: change the fate of a set of messages. */

static MarkMessages(scrn, fate, skip)
Scrn scrn;
FateType fate;
int skip;
{
    Toc toc = scrn->toc;
    Toc desttoc;
    int i;
    MsgList mlist;
    Msg msg;
    if (toc == NULL) return;
    if (fate == Fcopy || fate == Fmove)
	desttoc = SelectedToc(scrn);
    else
	desttoc = NULL;
    if (desttoc == toc)
	Feep();
    else {
	mlist = TocCurMsgList(toc);
	if (mlist->nummsgs == 0) {
	    msg = TocGetCurMsg(toc);
	    if (msg) {
		MsgSetFate(msg, fate, desttoc);
		if (skip)
		    ExecNextView(scrn);
	    }
	} else {
	    for (i = 0; i < mlist->nummsgs; i++)
		MsgSetFate(mlist->msglist[i], fate, desttoc);
	}
	FreeMsgList(mlist);
    }
}



void ExecMarkDelete(scrn)
Scrn scrn;
{
    MarkMessages(scrn, Fdelete, SkipDeleted);
}



void ExecMarkCopy(scrn)
Scrn scrn;
{
    MarkMessages(scrn, Fcopy, SkipCopied);
}


void ExecMarkMove(scrn)
Scrn scrn;
{
    MarkMessages(scrn, Fmove, SkipMoved);
}


void ExecMarkUnmarked(scrn)
Scrn scrn;
{
    MarkMessages(scrn, Fignore, FALSE);
}


void ExecCommitChanges(scrn)
Scrn scrn;
{
    if (scrn->toc == NULL) return;
    TocCommitChanges(scrn->toc);
}


void ExecPrintMessages(scrn)
Scrn scrn;
{
    Toc toc = scrn->toc;
    MsgList mlist;
    char str[200];
    int i;
    if (toc == NULL) return;
    mlist = CurMsgListOrCurMsg(toc);
    for (i = 0; i < mlist->nummsgs; i++) {
	(void) sprintf(str, "%s %s", defPrintCommand,
		MsgFileName(mlist->msglist[i]));
	(void) system(str);
    }
    FreeMsgList(mlist);
}



void ExecPack(scrn)
Scrn scrn;
{
    Toc toc = scrn->toc;
    char **argv, str[100];
    if (toc == NULL) return;
    if (TocConfirmCataclysm(toc)) return;
    argv = MakeArgv(4);
    argv[0] = "folder";
    (void) sprintf(str, "+%s", TocGetFolderName(toc));
    argv[1] = str;
    argv[2] = "-pack";
    argv[3] = "-fast";
    DoCommand(argv, (char *) NULL, "/dev/null");
    XtFree((char *) argv);
    TocForceRescan(toc);
}



void ExecSort(scrn)
Scrn scrn;
{
    Toc toc = scrn->toc;
    char **argv, str[100];
    if (toc == NULL) return;
    if (TocConfirmCataclysm(toc)) return;
    argv = MakeArgv(3);
    argv[0] = "sortm";
    (void) sprintf(str, "+%s", TocGetFolderName(toc));
    argv[1] = str;
    argv[2] = "-noverbose";
    DoCommand(argv, (char *) NULL, "/dev/null");
    XtFree((char *) argv);
    TocForceRescan(toc);
}




void ExecForceRescan(scrn)
Scrn scrn;
{
    Toc toc = scrn->toc;
    if (toc == NULL) return;
    TocForceRescan(toc);
}



/* Incorporate new mail. */

void ExecIncorporate(scrn)
Scrn scrn;
{
    if (scrn->toc == NULL) return;
    TocIncorporate(scrn->toc);
    TocCheckForNewMail();
}


void ExecTocReply(scrn)
Scrn scrn;
{
    Toc toc = scrn->toc;
    Scrn nscrn;
    MsgList mlist;
    Msg msg;
    if (toc == NULL) return;
    mlist = CurMsgListOrCurMsg(toc);
    if (mlist->nummsgs) {
	nscrn = NewCompScrn();
	msg = TocMakeNewMsg(DraftsFolder);
	MsgSetTemporary(msg);
	MsgLoadReply(msg, mlist->msglist[0]);
	(void)MsgSetScrnForComp(msg, nscrn);
	MapScrn(nscrn);
    }
    FreeMsgList(mlist);
}


void ExecPick(scrn)
Scrn scrn;
{
    Toc toc = scrn->toc;
    Scrn nscrn;
    char *toseq;
    if (toc == NULL) return;
    if (scrn->curseq)
	toseq = BBoxNameOfButton(scrn->curseq);
    else toseq = "temp";
    if (strcmp(toseq, "all") == 0)
	toseq = "temp";
    nscrn = CreateNewScrn(STpick);
    AddPick(nscrn, toc, TocViewedSequence(toc)->name, toseq);
    MapScrn(nscrn);
}


void ExecOpenSeq(scrn)
Scrn scrn;
{
    Toc toc = scrn->toc;
    if (toc == NULL) return;
    TocChangeViewedSeq(toc, TocGetSeqNamed(toc,
					   BBoxNameOfButton(scrn->curseq)));
}



typedef enum {ADD, REMOVE, DELETE} TwiddleOperation;

static TwiddleSequence(scrn, op)
Scrn scrn;
TwiddleOperation op;
{
    Toc toc = scrn->toc;
    char **argv, plus[100], str[100], *seqname;
    int i;
    MsgList mlist;
    if (toc == NULL || scrn->curseq == NULL) return;
    seqname = BBoxNameOfButton(scrn->curseq);
    if (strcmp(seqname, "all") == 0) {
	Feep();
	return;
    }
    if (op == DELETE)
	mlist = MakeNullMsgList();
    else {
	mlist = CurMsgListOrCurMsg(toc);
	if (mlist->nummsgs == 0) {
	    FreeMsgList(mlist);
	    Feep();
	    return;
	}
    }
    argv = MakeArgv(6 + mlist->nummsgs);
    argv[0] = "mark";
    (void) sprintf(plus, "+%s", TocGetFolderName(toc));
    argv[1] = plus;
    argv[2] = "-sequence";
    argv[3] = seqname;
    switch (op) {
      case ADD:
	argv[4] = "-add";
	argv[5] = "-nozero";
	break;
      case REMOVE:
	argv[4] = "-delete";
	argv[5] = "-nozero";
	break;
      case DELETE:
	argv[4] = "-delete";
	argv[5] = "all";
	break;
    }
    for (i = 0; i < mlist->nummsgs; i++) {
	(void) sprintf(str, "%d", MsgGetId(mlist->msglist[i]));
	argv[6 + i] = MallocACopy(str);
    }
    DoCommand(argv, (char *) NULL, "/dev/null");
    for (i = 0; i < mlist->nummsgs; i++)
        free((char *) argv[6 + i]);
    free((char *) argv);
    FreeMsgList(mlist);
    TocReloadSeqLists(toc);
}

    


void ExecAddToSeq(scrn)
Scrn scrn;
{
    TwiddleSequence(scrn, ADD);
}



void ExecRemoveFromSeq(scrn)
Scrn scrn;
{
    TwiddleSequence(scrn, REMOVE);
}



void ExecDeleteSeq(scrn)
Scrn scrn;
{
    TwiddleSequence(scrn, DELETE);
}


XtEventReturnCode HandleTocButtons(event, scrn)
XEvent *event;
Scrn scrn;
{
    DoubleClickProc = ExecNextView;
    DoubleClickParam = (caddr_t)scrn;
    return XteventHandled;
}
