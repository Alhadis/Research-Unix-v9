#ifndef lint
static char rcs_id[] = "$Header: pick.c,v 1.11 87/09/11 08:18:17 toddb Exp $";
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

/* pick.c -- handle a pick subwindow. */

#include "xmh.h"
#include "Form.h"

#define WTlabel		0
#define WTbutton	1
#define WTtextentry	2

#define	RTfrom		0
#define	RTto		1
#define	RTcc		2
#define RTdate		3
#define	RTsubject	4
#define	RTsearch	5
#define	RTother		6
#define	RTignore	7

#define FIRSTROWTYPE		RTfrom
#define LASTUSEFULROWTYPE	RTother
#define NUMROWTYPE		(RTignore+1)

static int stdwidth = -1;	/* Width to make text fields, and other
				   things that want to be the same width as
				   text fields. */

static char *TypeName[NUMROWTYPE];

typedef struct {
   short	type;		/* Encode what type of window this is. */
   Window 	window;		/* The window id itself. */
   struct _RowListRec *row;	/* Which row this window is in. */
   short	hilite;		/* Whether to hilight (if button subwindow) */
   char		*ptr;		/* Data (if text subwindow) */
} FormEntryRec, *FormEntryPtr;

typedef struct _RowListRec {
   short	type;		/* Encode what type of list this is. */
   Window	window;		/* Window containing this row */
   short	numwindows;	/* How many windows in this list. */
   FormEntryPtr *wlist;		/* List of windows. */
   struct _GroupRec *group;	/* Which group this is in. */
} RowListRec, *RowListPtr;

typedef struct _GroupRec {
   short	 numrows;	/* How many rows of window. */
   Window	window;		/* Window containing this group */
   RowListPtr	*rlist;		/* List of window rows. */
   struct _FormBoxRec *form;	/* Which form this is in. */
} GroupRec, *GroupPtr;

typedef struct _FormBoxRec {
   Window outer;	/* Outer window (contains scrollbars if any) */
   Window inner;	/* Inner window (contains master form) */
   short numgroups;	/* How many groups of form entries we have. */
   GroupPtr *glist;	/* List of form groups. */
   struct _PickRec *pick; /* Which pick this is in. */
} FormBoxRec, *FormBoxPtr;

typedef struct _PickRec {
   Scrn scrn;			/* Scrn containing this pick. */
   Window label;		/* Window with label for this pick. */
   Toc toc;			/* Toc for folder being scanned. */
   FormBoxPtr general;		/* Form for general info about this pick. */
   FormBoxPtr details;		/* Form for details about this pick. */
   Window errorwindow;		/* Pop-up error window. */
} PickRec;


InitPick()
{
    TypeName[RTfrom]	= "From:";
    TypeName[RTto]	= "To:";
    TypeName[RTcc]	= "Cc:";
    TypeName[RTdate]	= "Date:";
    TypeName[RTsubject] = "Subject:";
    TypeName[RTsearch]	= "Search:";
    TypeName[RTother]	= NULL;
}

static PickFlipColors(window)
Window window;
{
    static Arg arglist[] = {
	{XtNforeground, NULL},
	{XtNbackground, NULL},
    };
    XtArgVal temp;
    XtCommandGetValues(DISPLAY window, arglist, XtNumber(arglist));
    temp = arglist[0].value;
    arglist[0].value = arglist[1].value;
    arglist[1].value = temp;
    XtCommandSetValues(DISPLAY window, arglist, XtNumber(arglist));
}


static PrepareToUpdate(form)
  FormBoxPtr form;
{
    XtFormDoLayout(DISPLAY form->inner, FALSE);
}

static ExecuteUpdate(form)
  FormBoxPtr form;
{
    XtFormDoLayout(DISPLAY form->inner, TRUE);
}

static FormEntryPtr CreateEntry(window, type)
  Window window;
  int type;
{
    FormEntryPtr entry;
    entry = (FormEntryPtr) XtMalloc(sizeof(FormEntryRec));
    entry->row = NULL;
    entry->window = window;
    entry->type = type;
    return entry;
}


static void AddLabel(row, text, usestd)
  RowListPtr row;
  char *text;
  int usestd;
{
    Window window;
    static Arg arglist[] = {
	{XtNlabel, NULL},
	{XtNborderWidth, (XtArgVal) 0},
	{XtNjustify, (XtArgVal) XtjustifyRight},
	{XtNwidth, (XtArgVal) NULL}
    };
    arglist[0].value = (XtArgVal) text;
    arglist[XtNumber(arglist) - 1].value = (XtArgVal) stdwidth;
    window = XtLabelCreate(DISPLAY row->window, arglist,
			   usestd ? XtNumber(arglist) : XtNumber(arglist) - 1);
    AddWindow(row, CreateEntry(window, WTlabel));
}


static void AddButton(row, text, func, hilite)
  RowListPtr row;
  char *text;
  int (*func)();
  int hilite;
{
    FormEntryPtr entry;
    static Arg arglist[] = {
	{XtNlabel, NULL},
	{XtNfunction, NULL},
	{XtNparameter, NULL}
    };
    entry = CreateEntry((Window) 0, WTbutton);
    arglist[0].value = (XtArgVal)text;
    arglist[1].value = (XtArgVal)func;
    arglist[2].value = (XtArgVal)entry;
    entry->window = XtCommandCreate(DISPLAY  row->window,
				    arglist, XtNumber(arglist));
    entry->hilite = hilite;
    if (hilite) PickFlipColors(entry->window);
    AddWindow(row, entry);
}


static void AddTextEntry(row, str)
  RowListPtr row;
  char *str;
{
    static Arg arglist[] = {
	{XtNstring, (XtArgVal) NULL},
	{XtNwidth, (XtArgVal) NULL},
	{XtNlength, (XtArgVal) 300},
	{XtNtextOptions, (XtArgVal)(resizeWidth | resizeHeight)},
	{XtNeditType, (XtArgVal)XttextEdit},
    };
    char *ptr;
    FormEntryPtr entry;
    ptr = XtMalloc(310);
    arglist[0].value = (XtArgVal) ptr;
    arglist[1].value = (XtArgVal) stdwidth;
    (void) strcpy(ptr, str);
    entry = CreateEntry(XtTextStringCreate(DISPLAY row->window, arglist,
					   XtNumber(arglist)),
			WTtextentry);
    entry->ptr = ptr;
    AddWindow(row, entry);
}


static void ChangeTextEntry(entry, str)
FormEntryPtr entry;
char *str;
{
    static Arg arglist[] = {
	{XtNtextSource, (XtArgVal) NULL}
    };
    XtTextSource *source;
    if (strcmp(str, entry->ptr) == 0) return;
    XtTextGetValues(DISPLAY entry->window, arglist, XtNumber(arglist));
    source = (XtTextSource *) arglist[0].value;
    XtStringSourceDestroy(source);
    (void) strcpy(entry->ptr, str);
    source = (XtTextSource *)
	XtStringSourceCreate(entry->ptr, 300, XttextEdit);
    XtTextNewSource(DISPLAY entry->window, source, (XtTextPosition) 0);
}


static ExecYesNo(entry)
  FormEntryPtr entry;
{
    RowListPtr row = entry->row;
    int i;
    if (!entry->hilite) {
	entry->hilite = TRUE;
	PickFlipColors(entry->window);
	for (i = 0; i < row->numwindows; i++)
	    if (entry == row->wlist[i])
		break;
	if (i > 0 && row->wlist[i-1]->type == WTbutton)
	    i--;
	else
	    i++;
	entry = row->wlist[i];
	entry->hilite = FALSE;
	PickFlipColors(entry->window);
    }
}




static ExecRowOr(entry)
  FormEntryPtr entry;
{
    RowListPtr row = entry->row;
    FormBoxPtr form = row->group->form;
    PrepareToUpdate(form);
    DeleteWindow(entry);
    AddLabel(row, "or", FALSE);
    AddTextEntry(row, "");
    AddButton(row, "Or", ExecRowOr, FALSE);
    ExecuteUpdate(form);
}
    

static ExecGroupOr(entry)
  FormEntryPtr entry;
{
    FormBoxPtr form = entry->row->group->form;
    QXUnmapWindow(theDisplay, form->inner);
    PrepareToUpdate(form);
    AddDetailGroup(form);
    ExecuteUpdate(form);
    QXMapWindow(theDisplay, form->inner);
}

static char **argv;
static int argvsize;


static AppendArgv(ptr)
  char *ptr;
{
    argvsize++;
    argv = ResizeArgv(argv, argvsize);
    argv[argvsize - 1] = MallocACopy(ptr);
}

static EraseLast()
{
    argvsize--;
    XtFree((char *) argv[argvsize]);
    argv[argvsize] = 0;
}



static ParseRow(row)
  RowListPtr row;
{
    int     result = FALSE;
    int i;
    FormEntryPtr entry;
    char   str[1000];
    if (row->type > LASTUSEFULROWTYPE)
	return FALSE;
    for (i = 3; i < row->numwindows; i += 2) {
	entry = row->wlist[i];
	if (*(entry->ptr)) {
	    if (!result) {
		result = TRUE;
		if (row->wlist[1]->hilite)
		    AppendArgv("-not");
		AppendArgv("-lbrace");
	    }
	    switch (row->type) {
		case RTfrom: 
		    AppendArgv("-from");
		    break;
		case RTto: 
		    AppendArgv("-to");
		    break;
		case RTcc: 
		    AppendArgv("-cc");
		    break;
		case RTdate: 
		    AppendArgv("-date");
		    break;
		case RTsubject: 
		    AppendArgv("-subject");
		    break;
		case RTsearch: 
		    AppendArgv("-search");
		    break;
		case RTother: 
		    AppendArgv(sprintf(str, "--%s", row->wlist[2]->ptr));
		    break;
	    }
	    AppendArgv(entry->ptr);
	    AppendArgv("-or");
	}
    }
    if (result) {
	EraseLast();
	AppendArgv("-rbrace");
	AppendArgv("-and");
    }
    return result;
}
	    

static ParseGroup(group)
  GroupPtr group;
{
    int found = FALSE;
    int i;
    for (i=0 ; i<group->numrows ; i++)
	found |= ParseRow(group->rlist[i]);
    if (found) {
	EraseLast();
	AppendArgv("-rbrace");
	AppendArgv("-or");
	AppendArgv("-lbrace");
    }
    return found;
}



static void DestroyErrorWindow(pick)
Pick pick;
{
    if (pick->errorwindow) {
	(void) XtSendDestroyNotify(DISPLAY pick->errorwindow);
	QXDestroyWindow(theDisplay, pick->errorwindow);
	pick->errorwindow = NULL;
    }
}

static void MakeErrorWindow(pick, str)
Pick pick;
char *str;
{
    DestroyErrorWindow(pick);
    pick->errorwindow = XtDialogCreate(DISPLAY pick->scrn->window, str,
				       (char *)NULL, (ArgList)NULL, 0);
    XtDialogAddButton(DISPLAY pick->errorwindow, "OK", DestroyErrorWindow,
		      (caddr_t)pick);
    CenterWindow(pick->scrn->window, pick->errorwindow);
    QXMapWindow(theDisplay, pick->errorwindow);
}



static ExecOK(entry)
  FormEntryPtr entry;
{
    Pick pick = entry->row->group->form->pick;
    Toc toc = pick->toc;
    FormBoxPtr details = pick->details;
    FormBoxPtr general = pick->general;
    GroupPtr group = general->glist[0];
    RowListPtr row0 = group->rlist[0];
    RowListPtr row1 = group->rlist[1];
    RowListPtr row2 = group->rlist[2];
    char *fromseq = row0->wlist[3]->ptr;
    char *toseq = row0->wlist[1]->ptr;
    char *fromdate = row1->wlist[1]->ptr;
    char *todate = row1->wlist[3]->ptr;
    char *datefield = row1->wlist[5]->ptr;
    short removeoldmsgs = row2->wlist[1]->hilite;
    char str[1000];
    int i, found;

    DestroyErrorWindow(pick);
    if (strcmp(toseq, "all") == 0) {
	MakeErrorWindow(pick, "Can't create a sequence called \"all\".");
	return;
    }
    if (TocGetSeqNamed(toc, fromseq) == NULL) {
	(void) sprintf(str, "Sequence \"%s\" doesn't exist!", fromseq);
	MakeErrorWindow(pick, str);
	return;
    }
    argv = MakeArgv(1);
    argvsize = 0;
    AppendArgv("pick");
    AppendArgv(sprintf(str, "+%s", TocGetFolderName(toc)));
    AppendArgv(fromseq);
    AppendArgv("-sequence");
    AppendArgv(toseq);
    if (removeoldmsgs)
	AppendArgv("-zero");
    else
	AppendArgv("-nozero");
    if (*datefield) {
	AppendArgv("-datefield");
	AppendArgv(datefield);
    }
    if (*fromdate) {
	AppendArgv("-after");
	AppendArgv(fromdate);
	AppendArgv("-and");
    }
    if (*todate) {
	AppendArgv("-before");
	AppendArgv(todate);
	AppendArgv("-and");
    }
    found = FALSE;
    AppendArgv("-lbrace");
    AppendArgv("-lbrace");
    for (i=0 ; i<details->numgroups ; i++)
	found |= ParseGroup(details->glist[i]);
    EraseLast();
    EraseLast();
    if (found) AppendArgv("-rbrace");
    else if (*fromdate || *todate) EraseLast();
    if (debug) {
	for (i=0 ; i<argvsize ; i++)
	    (void) fprintf(stderr, "%s ", argv[i]);
	(void) fprintf(stderr, "\n");
    }
    (void) DoCommand(argv, (char *) NULL, "/dev/null");
    TocReloadSeqLists(toc);
    TocChangeViewedSeq(toc, TocGetSeqNamed(toc, toseq));
    DestroyScrn(pick->scrn);
    for (i=0 ; i<argvsize ; i++) XtFree((char *) argv[i]);
    XtFree((char *) argv);
}



static ExecCancel(entry)
  FormEntryPtr entry;
{
    Pick pick = entry->row->group->form->pick;
    Scrn scrn = pick->scrn;
    (void) DestroyScrn(scrn);
#ifdef X10
    {
	XEvent event;
	event.type = LeaveWindow;
	event.window = entry->window;
	XtDispatchEvent(DISPLAY &event);
    }
#endif
}



static AddWindow(row, entry)
  RowListPtr row;
  FormEntryPtr entry;
{
    static Arg arglist[] = {
	{XtNfromHoriz, (XtArgVal)NULL},
	{XtNresizable, (XtArgVal)TRUE},
	{XtNtop, (XtArgVal) XtChainTop},
	{XtNleft, (XtArgVal) XtChainLeft},
	{XtNbottom, (XtArgVal) XtChainTop},
	{XtNright, (XtArgVal) XtChainLeft},
    };

    row->numwindows++;
    row->wlist = (FormEntryPtr *)
	XtRealloc((char *) row->wlist,
		  (unsigned) row->numwindows * sizeof(FormEntryPtr));
    row->wlist[row->numwindows - 1] = entry;
    entry->row = row;
    if (row->numwindows > 1)
	arglist[0].value = (XtArgVal) row->wlist[row->numwindows - 2]->window;
    else
	arglist[0].value = (XtArgVal) NULL;
    XtFormAddWidget(DISPLAY row->window,
		    entry->window, arglist, XtNumber(arglist));
}
    

static DeleteWindow(entry)
  FormEntryPtr entry;
{
    RowListPtr row = entry->row;
    int i;
    QXDestroyWindow(theDisplay, entry->window);
    (void) XtSendDestroyNotify(DISPLAY entry->window);
    if (entry->type == WTtextentry)
	XtFree((char *) entry->ptr);
    for (i = 0; i < row->numwindows; i++)
	if (row->wlist[i] == entry)
	    break;
    row->numwindows--;
    for (; i < row->numwindows; i++)
	row->wlist[i] = row->wlist[i + 1];
}


/* Figure out how wide text fields and labels should be so that they'll all
   line up correctly, and be big enough to hold everything, but not too big. */

static void FindStdWidth()
{
    stdwidth = 100;		/* %%% HACK! */
}


static RowListPtr AddRow(group, type)
  GroupPtr group;
  int type;
{
    static Arg arglist1[] = {
	{XtNborderWidth, (XtArgVal) 0},
    };
    static Arg arglist2[] = {
	{XtNfromVert, (XtArgVal) NULL},
	{XtNresizable, (XtArgVal) TRUE},
	{XtNtop, (XtArgVal) XtChainTop},
	{XtNleft, (XtArgVal) XtChainLeft},
	{XtNbottom, (XtArgVal) XtChainTop},
	{XtNright, (XtArgVal) XtChainLeft}
    };
    RowListPtr row;
    group->numrows++;
    group->rlist = (RowListPtr *)
	XtRealloc((char *) group->rlist,
		  (unsigned) group->numrows * sizeof(RowListPtr));
    group->rlist[group->numrows - 1] = row =
	(RowListPtr) XtMalloc(sizeof(RowListRec));
    row->type = type;
    row->numwindows = 0;
    row->wlist = (FormEntryPtr *) XtMalloc(1);
    row->group = group;
    row->window = XtFormCreate(DISPLAY group->window,
			       arglist1, XtNumber(arglist1));
    if (group->numrows > 1)
	arglist2[0].value = (XtArgVal)group->rlist[group->numrows - 2]->window;
    else
	arglist2[0].value = (XtArgVal) NULL;
    XtFormAddWidget(DISPLAY group->window,
		    row->window, arglist2, XtNumber(arglist2));
    if (type == RTignore) return row;
    AddButton(row, "Pick", ExecYesNo, TRUE);
    AddButton(row, "Skip", ExecYesNo, FALSE);
    if (TypeName[type])
	AddLabel(row, TypeName[type], TRUE);
    else
	AddTextEntry(row, "");
    AddTextEntry(row, "");
    AddButton(row, "Or", ExecRowOr, FALSE);
    return row;
}


static GroupPtr AddGroup(form)
  FormBoxPtr form;
{
    static Arg arglist1[] = {
	{XtNborderWidth, (XtArgVal) 0},
    };
    static Arg arglist2[] = {
	{XtNfromVert, (XtArgVal) NULL},
	{XtNresizable, (XtArgVal) TRUE},
	{XtNtop, (XtArgVal) XtChainTop},
	{XtNleft, (XtArgVal) XtChainLeft},
	{XtNbottom, (XtArgVal) XtChainTop},
	{XtNright, (XtArgVal) XtChainLeft}
    };
    GroupPtr group;
    form->numgroups++;
    form->glist = (GroupPtr *)
	XtRealloc((char *) form->glist,
		  (unsigned) form->numgroups * sizeof(GroupPtr));
    form->glist[form->numgroups - 1] = group =
	(GroupPtr) XtMalloc(sizeof(GroupRec));
    group->numrows = 0;
    group->form = form;
    group->rlist = (RowListPtr *) XtMalloc(1);
    group->window = XtFormCreate(DISPLAY form->inner,
				 arglist1, XtNumber(arglist1));
    if (form->numgroups > 1)
	arglist2[0].value = (XtArgVal)form->glist[form->numgroups - 2]->window;
    else
	arglist2[0].value = (XtArgVal)NULL;
    XtFormAddWidget(DISPLAY form->inner,
		    group->window, arglist2, XtNumber(arglist2));
    return group;
}



static AddDetailGroup(form)
  FormBoxPtr form;
{
    GroupPtr group;
    RowListPtr row;
    int     type;
    if (form->numgroups > 0) {
	group = form->glist[form->numgroups - 1];
	row = group->rlist[group->numrows - 1];
	DeleteWindow(row->wlist[0]);
	AddLabel(row, "- or -", FALSE);
    }
    group = AddGroup(form);
    for (type = FIRSTROWTYPE; type <= LASTUSEFULROWTYPE; type++)
	(void) AddRow(group, type);
    row =  AddRow(group, RTignore);
    AddButton(row, "- Or -", ExecGroupOr, FALSE);
}


static AddGeneralGroup(form)
  FormBoxPtr form;
{
    GroupPtr group;
    RowListPtr row;
    group = AddGroup(form);
    row =  AddRow(group, RTignore);
    AddLabel(row, "Creating sequence:", FALSE);
    AddTextEntry(row, "");
    AddLabel(row, "with msgs from sequence:", FALSE);
    AddTextEntry(row, "");
    row =  AddRow(group, RTignore);
    AddLabel(row, "Date range:", FALSE);
    AddTextEntry(row, "");
    AddLabel(row, " - ", FALSE);
    AddTextEntry(row, "");
    AddLabel(row, "Date field:", FALSE);
    AddTextEntry(row, "");
    row =  AddRow(group, RTignore);
    AddLabel(row, "Clear old entries from sequence?", FALSE);
    AddButton(row, "Yes", ExecYesNo, TRUE);
    AddButton(row, "No", ExecYesNo, FALSE);
    row =  AddRow(group, RTignore);
    AddButton(row, "OK", ExecOK, FALSE);
    AddButton(row, "Cancel", ExecCancel, FALSE);
}


static void InitGeneral(pick, fromseq, toseq)
Pick pick;
char *fromseq, *toseq;
{
    RowListPtr row;
    row = pick->general->glist[0]->rlist[0];
    ChangeTextEntry(row->wlist[1], toseq);
    ChangeTextEntry(row->wlist[3], fromseq);
}


static void CleanForm(form)
FormBoxPtr form;
{
    int i, j, k;
    GroupPtr group;
    RowListPtr row;
    FormEntryPtr entry;
    for (i=0 ; i<form->numgroups ; i++) {
	group = form->glist[i];
	for (j=0 ; j<group->numrows ; j++) {
	    row = group->rlist[j];
	    for (k=0 ; k<row->numwindows ; k++) {
		entry = row->wlist[k];
		if (entry->type == WTtextentry)
		    ChangeTextEntry(entry, "");
	    }
	}
    }
}


static FormBoxPtr MakeAForm(pick, position)
Pick pick;
int position;
{
    static Arg arglist1[] = {
	{XtNname, (XtArgVal) "pick"},
	{XtNallowHoriz, (XtArgVal)TRUE},
	{XtNallowVert, (XtArgVal)TRUE},
    };
    static Arg arglist2[] = {
	{XtNborderWidth, (XtArgVal) 0}
    };
    FormBoxPtr result;
    result = (FormBoxPtr) XtMalloc(sizeof(FormBoxRec));
    result->outer = XtScrolledWindowCreate(DISPLAY pick->scrn->window,
					   arglist1, XtNumber(arglist1));
    result->inner = XtFormCreate(DISPLAY
				 XtScrolledWindowGetFrame(DISPLAY
							  result->outer),
				 arglist2, XtNumber(arglist2));
    XtScrolledWindowSetChild(DISPLAY result->outer, result->inner);
    XtVPanedWindowAddPane(DISPLAY pick->scrn->window, result->outer, position,
			  50, 1500, TRUE);
    result->pick = pick;
    result->numgroups = 0;
    result->glist = (GroupPtr *) XtMalloc(1);
    return result;
}



AddPick(scrn, toc, fromseq, toseq)
  Scrn scrn;
  Toc toc;
  char *fromseq, *toseq;
{
    Pick pick;
    FormBoxPtr general, details;
    char str[100];
    int width, height, scrnwidth, scrnheight;

    if (scrn->pick) {
	pick = scrn->pick;
	CleanForm(pick->details);
	CleanForm(pick->general);
    } else {
	pick = scrn->pick = (Pick) XtMalloc(sizeof(PickRec));
	pick->scrn = scrn;
	pick->errorwindow = NULL;

	pick->label = CreateTitleBar(scrn, 0);

	pick->details = details = MakeAForm(pick, 1);
	pick->general = general = MakeAForm(pick, 2);
	FindStdWidth();

	PrepareToUpdate(details);
	AddDetailGroup(details);
	ExecuteUpdate(details);
	PrepareToUpdate(general);
	AddGeneralGroup(general);
	ExecuteUpdate(general);
	GetWindowSize(general->inner, &width, &height);
	GetWindowSize(scrn->window, &scrnwidth, &scrnheight);
	if (width > scrnwidth)
	    height += XtScrollMgrGetThickness(DISPLAY general->outer);
	XtVPanedSetMinMax(DISPLAY scrn->window,
			  general->outer, height, height);
	XtVPanedSetMinMax(DISPLAY scrn->window,
			  general->outer, 10, 10000);
    }
    pick->toc = toc;
    InitGeneral(pick, fromseq, toseq);
    (void) sprintf(str, "Pick: %s", TocGetFolderName(toc));
    ChangeLabel(pick->label, str);
    QXStoreName(theDisplay, scrn->window, str);
}
