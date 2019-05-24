#ifndef lint
static char rcs_id[] = "$Header: screen.c,v 1.14 87/09/11 08:18:29 toddb Exp $";
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

/* scrn.c -- management of scrns. */

#include "xmh.h"

static int compscrncount = 0;	/* How many comp screens we currently have. */

/* Handle geometry requests from the scrn. */
/*ARGSUSED*/	/* -- keeps lint from complaining. */

XtGeometryReturnCode
ScrnGeometryRequest(window, request, requestBox, replyBox)
Window window;
XtGeometryRequest request;
WindowBox *requestBox, *replyBox;
{
    return XtgeometryNo;
}



/* Fill in the buttons for the view commands. */

static FillViewButtons(scrn)
Scrn scrn;
{
    extern void ExecCloseView(), ExecViewReply(), ExecViewForward();
    extern void ExecViewUseAsComposition(), ExecEditView();
    extern void ExecSaveView(), ExecPrintView();
    ButtonBox buttonbox = scrn->viewbuttons;
    BBoxStopUpdate(buttonbox);
    if (scrn->tocwindow == NULL)
	BBoxAddButton(buttonbox, "close", ExecCloseView, 999, TRUE);
    BBoxAddButton(buttonbox, "reply", ExecViewReply, 999, TRUE);
    BBoxAddButton(buttonbox, "forward", ExecViewForward, 999, TRUE);
    BBoxAddButton(buttonbox, "useAsComp", ExecViewUseAsComposition,
		  999, TRUE);
    BBoxAddButton(buttonbox, "edit", ExecEditView, 999, TRUE);
    BBoxAddButton(buttonbox, "save", ExecSaveView, 999, FALSE);
    BBoxAddButton(buttonbox, "print", ExecPrintView, 999, TRUE);
    BBoxStartUpdate(buttonbox);
    BBoxForceFullSize(buttonbox);
    BBoxAllowAnySize(buttonbox);
}
    


static FillCompButtons(scrn)
Scrn scrn;
{
    extern void ExecCloseView();
    extern void ExecCompReset();
    extern void ExecComposeMessage();
    extern void ExecSaveDraft();
    extern void ExecSendDraft();
    ButtonBox buttonbox = scrn->viewbuttons;
    BBoxStopUpdate(buttonbox);
    if (scrn->tocwindow == NULL)
	BBoxAddButton(buttonbox, "close", ExecCloseView, 999, TRUE);
    BBoxAddButton(buttonbox, "send", ExecSendDraft, 999, TRUE);
    BBoxAddButton(buttonbox, "reset", ExecCompReset, 999, TRUE);
    BBoxAddButton(buttonbox, "compose", ExecComposeMessage, 999, TRUE);
    BBoxAddButton(buttonbox, "save", ExecSaveDraft, 999, TRUE);
    BBoxStartUpdate(buttonbox);
    BBoxForceFullSize(buttonbox);
    BBoxAllowAnySize(buttonbox);
}


/* Figure out which buttons should and shouldn't be enabled in the given
   screen.  This should be called whenever something major happens to the
   screen. */

#define SetButton(buttonbox, name, value) \
    if (value) BBoxEnable(BBoxFindButtonNamed(buttonbox, name)); \
    else BBoxDisable(BBoxFindButtonNamed(buttonbox, name));

void EnableProperButtons(scrn)
Scrn scrn;
{
    int value, changed, reapable;
    if (scrn) {
	switch (scrn->kind) {
	  case STtocAndView:
	    SetButton(scrn->tocbuttons, "inc", TocCanIncorporate(scrn->toc));
	    value = TocHasSequences(scrn->toc);
	    SetButton(scrn->tocbuttons, "openSeq", value);
	    SetButton(scrn->tocbuttons, "addToSeq", value);
	    SetButton(scrn->tocbuttons, "removeFromSeq", value);
	    SetButton(scrn->tocbuttons, "deleteSeq", value);
	    /* Fall through */

	  case STview:
	    value = (scrn->msg != NULL && !MsgGetEditable(scrn->msg));
	    SetButton(scrn->viewbuttons, "edit", value);
	    SetButton(scrn->viewbuttons, "save", scrn->msg != NULL && !value);
	    break;

	  case STcomp:
	    if (scrn->msg != NULL) {
		changed = MsgChanged(scrn->msg);
		reapable = MsgGetReapable(scrn->msg);
		SetButton(scrn->viewbuttons, "send", changed || !reapable);
		SetButton(scrn->viewbuttons, "save", changed || reapable);
		if (!changed) MsgSetCallOnChange(scrn->msg,
						 EnableProperButtons,
						 (caddr_t) scrn);
		else MsgClearCallOnChange(scrn->msg);
	    } else {
		SetButton(scrn->viewbuttons, "send", FALSE);
		SetButton(scrn->viewbuttons, "save", FALSE);
	    }
	    break;
	}
    }
}

void PrepareDoubleClickFolder(scrn)
Scrn scrn;
{
    extern void ExecOpenFolder();
    DoubleClickProc = ExecOpenFolder;
    DoubleClickParam = (caddr_t)scrn;
}

void PrepareDoubleClickSequence(scrn)
Scrn scrn;
{
    extern void ExecOpenSeq();
    DoubleClickProc = ExecOpenSeq;
    DoubleClickParam = (caddr_t)scrn;
}

/* Create subwindows for a toc&view window. */

static MakeTocAndView(scrn)
Scrn scrn;
{
    extern void ExecCloseScrn();
    extern void ExecComposeMessage();
    extern void ExecOpenFolder();
    extern void ExecOpenFolderInNewWindow();
    extern void ExecCreateFolder();
    extern void ExecDeleteFolder();
    extern XtEventReturnCode HandleTocButtons();
    extern void ExecIncorporate();
    extern void ExecNextView();
    extern void ExecPrevView();
    extern void ExecMarkDelete();
    extern void ExecMarkMove();
    extern void ExecMarkCopy();
    extern void ExecMarkUnmarked();
    extern void ExecViewNew();
    extern void ExecTocReply();
    extern void ExecTocForward();
    extern void ExecTocUseAsComposition();
    extern void ExecCommitChanges();
    extern void ExecOpenSeq();
    extern void ExecAddToSeq();
    extern void ExecRemoveFromSeq();
    extern void ExecPick();
    extern void ExecDeleteSeq();
    extern void ExecPrintMessages();
    extern void ExecPack();
    extern void ExecSort();
    extern void ExecForceRescan();
    int i, width, height, theight, vheight;
    ButtonBox buttonbox;
    static Arg arglist[] = {
	{XtNtextSink, NULL}
    };
    static XtSelectType sarray[] = {XtselectLine,
				    XtselectPosition,
				    XtselectWord,
				    XtselectAll,
				    XtselectNull};


    XtVPanedRefigureMode(DISPLAY scrn->window, FALSE);
    scrn->folderbuttons = BBoxRadioCreate(scrn, 0, "folders",
					  &(scrn->curfolder));
    scrn->mainbuttons = BBoxCreate(scrn, 1, "folderButtons");
    scrn->toclabel = CreateTitleBar(scrn, 2);
    scrn->tocwindow = CreateTextSW(scrn, 3, "toc", 0);
    scrn->seqbuttons = BBoxRadioCreate(scrn, 4, "seqButtons", &scrn->curseq);
    scrn->tocbuttons = BBoxCreate(scrn, 5, "tocButtons");
    scrn->viewlabel = CreateTitleBar(scrn, 6);
    scrn->viewwindow = CreateTextSW(scrn, 7, "view", wordBreak);
    scrn->viewbuttons = BBoxCreate(scrn, 8, "viewButtons");

    XtTextGetValues(DISPLAY scrn->tocwindow, arglist, XtNumber(arglist));
    scrn->tocsink = (XtTextSink *) arglist[0].value;

    buttonbox = scrn->folderbuttons;
    BBoxStopUpdate(buttonbox);
    for (i=0 ; i<numFolders ; i++)
	BBoxAddButton(buttonbox, TocGetFolderName(folderList[i]),
		      PrepareDoubleClickFolder, 999, TRUE);
    BBoxForceFullSize(buttonbox);
    BBoxStartUpdate(buttonbox);

    buttonbox = scrn->mainbuttons;
    BBoxStopUpdate(buttonbox);
    BBoxAddButton(buttonbox, "close", ExecCloseScrn, 999, TRUE);
    BBoxAddButton(buttonbox, "compose", ExecComposeMessage, 999, TRUE);
    BBoxAddButton(buttonbox, "open", ExecOpenFolder, 999, TRUE);
    BBoxAddButton(buttonbox, "openInNew", ExecOpenFolderInNewWindow,
		  999, TRUE);
    BBoxAddButton(buttonbox, "create", ExecCreateFolder, 999, TRUE);
    BBoxAddButton(buttonbox, "delete", ExecDeleteFolder, 999, TRUE);
    BBoxForceFullSize(buttonbox);
    BBoxStartUpdate(buttonbox);

    buttonbox = scrn->seqbuttons;
    BBoxStopUpdate(buttonbox);
    BBoxAddButton(buttonbox, "all", PrepareDoubleClickSequence, 999, TRUE);
    BBoxForceFullSize(buttonbox);
    BBoxStartUpdate(buttonbox);

    if (defDoubleClick) {
	XtSetEventHandler(theDisplay,
			  XtTextGetInnerWindow(theDisplay, scrn->tocwindow),
			  HandleTocButtons, ButtonReleaseMask, (caddr_t) scrn);
	sarray[1] = XtselectNull;
    }
    XtTextSetSelectionArray(DISPLAY scrn->tocwindow, (XtSelectType *)sarray);

    buttonbox = scrn->tocbuttons;
    BBoxStopUpdate(buttonbox);
    BBoxAddButton(buttonbox, "inc", ExecIncorporate, 999, TRUE);
    BBoxAddButton(buttonbox, "next", ExecNextView, 999, TRUE);
    BBoxAddButton(buttonbox, "prev", ExecPrevView, 999, TRUE);
    BBoxAddButton(buttonbox, "delete", ExecMarkDelete, 999, TRUE);
    BBoxAddButton(buttonbox, "move", ExecMarkMove, 999, TRUE);
    BBoxAddButton(buttonbox, "copy", ExecMarkCopy, 999, TRUE);
    BBoxAddButton(buttonbox, "unmark", ExecMarkUnmarked, 999, TRUE);
    BBoxAddButton(buttonbox, "viewNew", ExecViewNew, 999, TRUE);
    BBoxAddButton(buttonbox, "reply", ExecTocReply, 999, TRUE);
    BBoxAddButton(buttonbox, "forward", ExecTocForward, 999, TRUE);
    BBoxAddButton(buttonbox, "useAsComp", ExecTocUseAsComposition,
		  999, TRUE);
    BBoxAddButton(buttonbox, "commit", ExecCommitChanges, 999, TRUE);
    BBoxAddButton(buttonbox, "print", ExecPrintMessages, 999, TRUE);
    BBoxAddButton(buttonbox, "pack", ExecPack, 999, TRUE);
    BBoxAddButton(buttonbox, "sort", ExecSort, 999, TRUE);
    BBoxAddButton(buttonbox, "rescan", ExecForceRescan, 999, TRUE);
    BBoxAddButton(buttonbox, "pick", ExecPick, 999, TRUE);
    BBoxAddButton(buttonbox, "openSeq", ExecOpenSeq, 999, TRUE);
    BBoxAddButton(buttonbox, "addToSeq", ExecAddToSeq, 999, TRUE);
    BBoxAddButton(buttonbox, "removeFromSeq", ExecRemoveFromSeq, 999, TRUE);
    BBoxAddButton(buttonbox, "deleteSeq", ExecDeleteSeq, 999, TRUE);
    BBoxForceFullSize(buttonbox);
    BBoxStartUpdate(buttonbox);

    FillViewButtons(scrn);
    BBoxForceFullSize(scrn->viewbuttons);
    XtVPanedRefigureMode(DISPLAY scrn->window, TRUE);
    BBoxAllowAnySize(scrn->folderbuttons);
    BBoxAllowAnySize(scrn->mainbuttons);
    BBoxAllowAnySize(scrn->seqbuttons);
    BBoxAllowAnySize(scrn->tocbuttons);
    BBoxAllowAnySize(scrn->viewbuttons);
    GetWindowSize(scrn->tocwindow, &width, &theight);
    GetWindowSize(scrn->viewwindow, &width, &vheight);
    height = theight + vheight;
    theight = defTocPercentage * height / 100;
    XtVPanedSetMinMax(DISPLAY scrn->window, scrn->tocwindow, theight, theight);
    XtVPanedSetMinMax(DISPLAY scrn->window, scrn->tocwindow, 50, 1000);
}


MakeView(scrn)
Scrn scrn;
{
    scrn->viewlabel = CreateTitleBar(scrn, 0);
    scrn->viewwindow = CreateTextSW(scrn, 1, "view", wordBreak);
    scrn->viewbuttons = BBoxCreate(scrn, 2, "viewButtons");
    FillViewButtons(scrn);
}


MakeComp(scrn)
Scrn scrn;
{
    scrn->viewlabel = CreateTitleBar(scrn, 0);
    scrn->viewwindow = CreateTextSW(scrn, 1, "comp", wordBreak);
    scrn->viewbuttons = BBoxCreate(scrn, 2, "compButtons");
    FillCompButtons(scrn);
    compscrncount++;
}



/* Create a scrn of the given type. */

Scrn CreateNewScrn(kind)
ScrnKind kind;
{
    extern XSetSizeHints();
    int i;
    Position x, y;
    Dimension width, height;
    Scrn scrn;
    char *geometry;
    static Arg arglist[] = {
	{XtNname, NULL},
	{XtNx, NULL},
	{XtNy, NULL},
	{XtNwidth, NULL},
	{XtNheight, NULL},
    };
#ifdef X11
    int bits;
    XSizeHints sizehints;
#endif
#ifdef X10
    Window window;
    OpaqueFrame frame;
    WindowInfo info;
    static FontInfo *font = NULL;
#endif X10

    for (i=0 ; i<numScrns ; i++)
	if (scrnList[i]->kind == kind && !scrnList[i]->mapped)
	    return scrnList[i];
    switch (kind) {
	case STtocAndView:	geometry = defTocGeometry;	break;
	case STview:		geometry = defViewGeometry;	break;
	case STcomp:		geometry = defCompGeometry;	break;
	case STpick:		geometry = defPickGeometry;	break;
    }

#ifdef X11
    bits = XParseGeometry(geometry, &x, &y, &width, &height);
    if (!(bits & XValue)) x = 0;
    else if (bits & XNegative) x = rootwidth - abs(x) - width;
    if (!(bits & YValue)) y = 0;
    else if (bits & YNegative) y = rootheight - abs(y) - height;
#endif
#ifdef X10
    if (font == NULL) font = XOpenFont("vtsingle");
    EmptyEventQueue();
    ClearButtonTracks();
    frame.bdrwidth = 1;
    frame.border = BlackPixmap;
    frame.background = WhitePixmap;
    window = XCreateTerm(progName, progName, geometry, geometry,
            &frame, 20, 20, 0, 0, &width, &height, font, 1, 1);
    (void) XQueryWindow(window, &info);
    x = info.x;
    y = info.y;
    width = info.width;
    height = info.height;
    XDestroyWindow(window);
#endif X10

    arglist[0].value = (XtArgVal)progName;
    arglist[1].value = (XtArgVal)x;
    arglist[2].value = (XtArgVal)y;
    arglist[3].value = (XtArgVal)width;
    arglist[4].value = (XtArgVal)height;
    numScrns++;
    scrnList = (Scrn *)
	XtRealloc((char *) scrnList, (unsigned) numScrns*sizeof(Scrn));
    scrn = scrnList[numScrns - 1] = (Scrn)XtMalloc(sizeof(ScrnRec));
    bzero((char *)scrn, sizeof(ScrnRec));
    scrn->kind = kind;
    scrn->window = XtVPanedWindowCreate(DISPLAY QDefaultRootWindow(theDisplay),
					arglist, XtNumber(arglist));
    (void) XtSetGeometryHandler(DISPLAY scrn->window,
				(XtGeometryHandler)ScrnGeometryRequest);

#ifdef X11
#include <Xatom.h>
    XtMakeMaster(DISPLAY scrn->window);
    sizehints.flags = 0;
    sizehints.x = x;
    sizehints.y = y;
    sizehints.width = width;
    sizehints.height = height;
    if ((bits & XValue) && (bits & YValue))
	sizehints.flags |= USPosition;
    if ((bits & WidthValue) && (bits & HeightValue))
	sizehints.flags |= USSize;
    else
	sizehints.flags |= PSize;
    XSetSizeHints(theDisplay, scrn->window, &sizehints, XA_WM_NORMAL_HINTS);
    scrn->hints.flags = InputHint;
    scrn->hints.input = FALSE;
    if (kind == STtocAndView) {
	scrn->hints.flags |= IconPixmapHint;
	scrn->hints.icon_pixmap = NoMailPixmap;
    }
    XSetWMHints(theDisplay, scrn->window, &(scrn->hints));
    XDefineCursor(theDisplay, scrn->window, XtGetCursor(DISPLAY XC_left_ptr));
#endif X11
#ifdef X10
    XDefineCursor(scrn->window, XtGetCursor("left_ptr"));
#endif

    switch (kind) {
	case STtocAndView:	MakeTocAndView(scrn);	break;
	case STview:		MakeView(scrn);	break;
	case STcomp:		MakeComp(scrn);	break;
    }
    return scrn;
}


Scrn NewViewScrn()
{
/*
    Scrn scrn;
    int i;
    for (i=0 ; i<numScrns ; i++) {
	scrn = scrnList[i];
	if (scrn->kind == STview && scrn->msg == NULL)
	    return scrn;
    }
*/
    return CreateNewScrn(STview);
}

Scrn NewCompScrn()
{
/*
    Scrn scrn;
    int i;
    for (i=0 ; i<numScrns ; i++) {
	scrn = scrnList[i];
	if (scrn->kind == STcomp && MsgGetReapable(scrn->msg)) {
	    (void) MsgSetScrn((Msg) NULL, scrn);
	    return scrn;
	}
    }
*/
    return CreateNewScrn(STcomp);
}


/* Destroy the screen.  If unsaved changes are in a msg, too bad. */

void DestroyScrn(scrn)
  Scrn scrn;
{
    DestroyConfirmWindow();
    TocSetScrn((Toc) NULL, scrn);
    MsgSetScrnForce((Msg) NULL, scrn);
    if (scrn->kind == STcomp) compscrncount--;
/*    (void) XtSendDestroyNotify(DISPLAY scrn->window);
    QXDestroyWindow(theDisplay, scrn->window);
    for (i = 0; i < numScrns; i++)
	if (scrnList[i] == scrn)
	    break;
    scrnList[i] = scrnList[--numScrns];
    XtFree((char *) scrn);
*/
    QXUnmapWindow(theDisplay, scrn->window);
    scrn->mapped = FALSE;
#ifdef X10
    ClearButtonTracks();
#endif
}



void MapScrn(scrn)
Scrn scrn;
{
    if (!scrn->mapped) {
	XRaiseWindow(DISPLAY scrn->window);
	QXMapWindow(theDisplay, scrn->window);
	scrn->mapped = TRUE;
    }
}
