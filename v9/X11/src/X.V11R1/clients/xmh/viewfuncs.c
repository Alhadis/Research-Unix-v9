#ifndef lint
static char rcs_id[] = "$Header: viewfuncs.c,v 1.10 87/09/11 08:19:35 toddb Exp $";
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

/* view.c -- handle viewing of a message */

#include "xmh.h"


void ExecCloseView(scrn)
Scrn scrn;
{
    if (MsgSetScrn((Msg) NULL, scrn)) return;
    DestroyScrn(scrn);
}


void ExecViewReply(scrn)
Scrn scrn;
{
    Msg msg;
    Scrn nscrn;
    if (scrn->msg == NULL) return;
    nscrn = NewCompScrn();
    msg = TocMakeNewMsg(DraftsFolder);
    MsgSetTemporary(msg);
    MsgLoadReply(msg, scrn->msg);
    (void) MsgSetScrnForComp(msg, nscrn);
    MapScrn(nscrn);
}


void ExecViewForward(scrn)
Scrn scrn;
{
    MsgList mlist;
    if (scrn->msg == NULL) return;
    mlist = MakeSingleMsgList(scrn->msg);
    CreateForward(mlist);
    FreeMsgList(mlist);
}


void ExecViewUseAsComposition(scrn)
Scrn scrn;
{
    Msg msg;
    Scrn nscrn;
    if (scrn->msg == NULL) return;
    nscrn = NewCompScrn();
    if (MsgGetToc(scrn->msg) == DraftsFolder)
	msg = scrn->msg;
    else {
	msg = TocMakeNewMsg(DraftsFolder);
	MsgLoadCopy(msg, scrn->msg);
	MsgSetTemporary(msg);
    }
    (void) MsgSetScrnForComp(msg, nscrn);
    MapScrn(nscrn);
}



void ExecEditView(scrn)
Scrn scrn;
{
    if (scrn->msg == NULL) return;
    MsgSetEditable(scrn->msg);
}
    


void ExecSaveView(scrn)
Scrn scrn;
{
    if (scrn->msg == NULL) return;
    MsgSaveChanges(scrn->msg);
    MsgClearEditable(scrn->msg);
}
    


void ExecPrintView(scrn)
Scrn scrn;
{
    char str[200];
    if (scrn->msg == NULL) return;
    (void) sprintf(str, "%s %s", defPrintCommand, MsgFileName(scrn->msg));
    (void) system(str);
}

