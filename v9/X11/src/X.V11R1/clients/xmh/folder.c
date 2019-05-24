#ifndef lint
static char rcs_id[] = "$Header: folder.c,v 1.11 87/09/11 08:18:05 toddb Exp $";
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

/* folder.c -- implement buttons relating to folders and other globals. */

#include "xmh.h"


/* Close this toc&view scrn.  If this is the last toc&view, quit xmh. */

void ExecCloseScrn(scrn)
Scrn scrn;
{
    extern void exit();
    Toc toc;
    int i, count;
    count = 0;
    for (i=0 ; i<numScrns ; i++)
	if (scrnList[i]->kind == STtocAndView && scrnList[i]->mapped)
	    count++;
    if (count <= 1) {
	for (i = numScrns - 1; i >= 0; i--)
	    if (scrnList[i] != scrn) {
		if (MsgSetScrn((Msg) NULL, scrnList[i]))
		    return;
	    }
	for (i = 0; i < numFolders; i++) {
	    toc = folderList[i];
	    if (TocConfirmCataclysm(toc))
		return;
	}
	if (MsgSetScrn((Msg) NULL, scrn))
	    return;
	DestroyPromptWindow();
/*	for (i = 0; i < numFolders; i++) {
	    toc = folderList[i];
	    if (toc->scanfile && toc->curmsg)
		CmdSetSequence(toc, "cur", MakeSingleMsgList(toc->curmsg));
	}
*/
	exit(0);
    }
    else {
	if (MsgSetScrn((Msg) NULL, scrn)) return;
	DestroyScrn(scrn);
    }
}


/* Open the selected folder in this screen. */

void ExecOpenFolder(scrn)
Scrn scrn;
{
    Toc toc;
    toc = SelectedToc(scrn);
    TocSetScrn(toc, scrn);
}



/* Compose a new message. */

void ExecComposeMessage(scrn)
Scrn scrn;
{
    Msg msg;
    scrn = NewCompScrn();
    msg = TocMakeNewMsg(DraftsFolder);
    MsgLoadComposition(msg);
    MsgSetTemporary(msg);
    MsgSetReapable(msg);
    (void) MsgSetScrnForComp(msg, scrn);
    MapScrn(scrn);
}



/* Make a new scrn displaying the given folder. */

void ExecOpenFolderInNewWindow(scrn)
Scrn scrn;
{
    Toc toc;
    toc = SelectedToc(scrn);
    scrn = CreateNewScrn(STtocAndView);
    TocSetScrn(toc, scrn);
    MapScrn(scrn);
}



/* Create a new xmh folder. */

void ExecCreateFolder(scrn)
Scrn scrn;
{
    void CreateFolder();
    MakePrompt(scrn, "Create folder named:", CreateFolder);
}



/* Delete the selected folder.  Requires confirmation! */

void ExecDeleteFolder(scrn)
Scrn scrn;
{
    char *foldername, str[100];
    Toc toc;
    int i;
    foldername = BBoxNameOfButton(scrn->curfolder);
    toc = TocGetNamed(foldername);
    if (TocConfirmCataclysm(toc)) return;
    (void) sprintf(str, "Are you sure you want to destroy %s?", foldername);
    if (!Confirm(scrn, str)) return;
    TocSetScrn(toc, (Scrn) NULL);
    TocDeleteFolder(toc);
    for (i=0 ; i<numScrns ; i++)
	if (scrnList[i]->folderbuttons)
	    BBoxDeleteButton(BBoxFindButtonNamed(scrnList[i]->folderbuttons,
						 foldername));
}



			/* Debugging stuff only. */
void ExecSyncOn()
{
#ifdef X11
    (void) XSynchronize(theDisplay, TRUE);
#endif
}
void ExecSyncOff()
{
#ifdef X11
    (void) XSynchronize(theDisplay, FALSE);
#endif
}



/* Create a new folder with the given name. */

void CreateFolder(name)
  char *name;
{
    Toc toc;
    int i, position;
    extern void PrepareDoubleClickFolder();
    for (i=0 ; name[i] > ' ' ; i++) ;
    name[i] = 0;
    toc = TocGetNamed(name);
    if (toc || i == 0) {
	Feep();
	return;
    }
    toc = TocCreateFolder(name);
    if (toc == NULL) {
	Feep();
	return;
    }
    for (position = 0; position < numFolders; position++)
	if (folderList[position] == toc)
	    break;
    for (i = 0; i < numScrns; i++)
	if (scrnList[i]->folderbuttons)
	    BBoxAddButton(scrnList[i]->folderbuttons, name,
			  PrepareDoubleClickFolder, position, TRUE);
}
