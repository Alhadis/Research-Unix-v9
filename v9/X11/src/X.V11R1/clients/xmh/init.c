#ifndef lint
static char rcs_id[] = "$Header: init.c,v 1.21 87/09/12 08:04:03 swick Exp $";
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

/* Init.c - Handle start-up initialization. */

#include "xmh.h"

/* Xmh-specific resources. */

static Resource resourcelist[] = {
    {"debug", "Debug", XrmRBoolean, sizeof(int),
	 (XtArgVal)&debug, NULL},
    {"tempdir", "tempDir", XrmRString, sizeof(char *),
	 (XtArgVal)&tempDir, NULL},
    {"mhpath", "MhPath", XrmRString, sizeof(char *),
	 (XtArgVal)&defMhPath, NULL},
    {"initialfolder", "InitialFolder", XrmRString, sizeof(char *),
	 (XtArgVal)&initialFolderName, NULL},
    {"initialincfile", "InitialIncFile", XrmRString, sizeof(char *),
         (XtArgVal)&initialIncFile, NULL},
    {"draftsfolder", "DraftsFolder", XrmRString, sizeof(char *),
	 (XtArgVal)&draftsFolderName, NULL},
    {"sendwidth", "SendWidth", XrmRInt, sizeof(int),
	 (XtArgVal)&defSendLineWidth, NULL},
    {"sendbreakwidth", "SendBreakWidth", XrmRInt, sizeof(int),
	 (XtArgVal)&defBreakSendLineWidth, NULL},
    {"printcommand", "PrintCommand", XrmRString, sizeof(char *),
	 (XtArgVal)&defPrintCommand, NULL},
    {"tocwidth", "TocWidth", XrmRInt, sizeof(int),
	 (XtArgVal)&defTocWidth, NULL},
    {"skipdeleted", "SkipDeleted", XrmRString, sizeof(char *),
	 (XtArgVal)&SkipDeleted, NULL},
    {"skipmoved", "SkipMoved", XrmRString, sizeof(char *),
	 (XtArgVal)&SkipMoved, NULL},
    {"skipCopied", "SkipCopied", XrmRString, sizeof(char *),
	 (XtArgVal)&SkipCopied, NULL},
    {"hideboringheaders", "HideBoringHeaders", XrmRBoolean, sizeof(int),
	 (XtArgVal)&defHideBoringHeaders, NULL},
    {"hidenullseqboxes", "HideNullSeqBoxes", XrmRBoolean, sizeof(int),
	 (XtArgVal)&defHideNullSeqBoxes, NULL},
    {"geometry", "Geometry", XrmRString, sizeof(char *),
	 (XtArgVal)&defGeometry, NULL},
    {"tocgeometry", "TocGeometry", XrmRString, sizeof(char *),
	 (XtArgVal)&defTocGeometry, NULL},
    {"viewgeometry", "ViewGeometry", XrmRString, sizeof(char *),
	 (XtArgVal)&defViewGeometry, NULL},
    {"compgeometry", "CompGeometry", XrmRString, sizeof(char *),
	 (XtArgVal)&defCompGeometry, NULL},
    {"pickgeometry", "PickGeometry", XrmRString, sizeof(char *),
	 (XtArgVal)&defPickGeometry, NULL},
    {"tocpercentage", "TocPercentage", XrmRInt, sizeof(int),
	 (XtArgVal)&defTocPercentage, NULL},
    {"checknewmail", "CheckNewMail", XrmRBoolean, sizeof(int),
	 (XtArgVal)&defNewMailCheck, NULL},
    {"makecheckpoints", "MakeCheckPoints", XrmRBoolean, sizeof(int),
	 (XtArgVal)&defMakeCheckpoints, NULL},
    {"grabFocus", "GrabFocus", XrmRBoolean, sizeof(int),
	 (XtArgVal)&defGrabFocus, NULL},
    {"doubleClick", "DoubleClick", XrmRBoolean, sizeof(int),
	 (XtArgVal)&defDoubleClick, NULL}
};


/* Tell the user how to use this program. */
Syntax()
{
    extern void exit();
    (void)fprintf(stderr, "usage:  xmh [display] [=geometry] \n");
    exit(2);
}


ProcessCommandLine(argc, argv)
int argc;
char **argv;
{
    int i;
    char *ptr;
    ptr = rindex(argv[0], '/');
    if (ptr) progName = ptr + 1;
    else progName = argv[0];
    if (strcmp(progName, "xmh_d") == 0) progName = "xmh";
    displayName = "";
    defTocGeometry = NULL;
    for (i=1 ; i<argc ; i++) {
	if (argv[i][0] == '=') defTocGeometry = argv[i];
	else if (index(argv[i], ':')) displayName = argv[i];
	else Syntax();
    }
}

static char *defaultFile[] = { "%s/xmh.Xdefaults",	/* LIBDIR */
			       "%s/xmh.X11defaults",	/* LIBDIR */
			       "%s/.Xdefaults",		/* homeDir */
			       "%s/.X11defaults"	/* homeDir */
			     };

/* All the start-up initialization goes here. */

InitializeWorld(argc, argv)
int argc;
char **argv;
{
    int gbits, l;
    Position x, y;
    Dimension width, height;
    FILEPTR fid;
    XrmResourceDataBase db = NULL, db2;
    char str[500], str2[500], *ptr;
    XrmNameList names;
    XrmClassList classes;
    Scrn scrn;
    int defaultIndex;

    XtInitialize();
    ProcessCommandLine(argc, argv);
    theDisplay = XOpenDisplay(displayName);

    theScreen = 0;
    if (theDisplay == NULL)
	Punt("Couldn't open display!");

    homeDir = MallocACopy(getenv("HOME"));

    (void) XrmInitialize();

    for (defaultIndex=0; defaultIndex<XtNumber(defaultFile); defaultIndex++) {
        (void) sprintf( str, defaultFile[defaultIndex],
		        (defaultIndex<2 ? LIBDIR : homeDir) );
	fid = myfopen(str, "r");
	if (fid) {
	    XrmGetDataBase(fid, &db2);
	    (void)myfclose(fid);
	    if (db) XrmMergeDataBases(db2, &db);
	       else db = db2;
	}
    }

    if (db) XrmSetCurrentDataBase(db);

    (void) sprintf(str, "%s/.mh_profile", homeDir);
    fid = myfopen(str, "r");
    if (fid) {
	while (ptr = ReadLine(fid)) {
	    if (strncmp(ptr, "Path:", 5) == 0) {
		ptr += 5;
		while (*ptr == ' ' || *ptr == '\t')
		    ptr++;
		(void) strcpy(str, ptr);
	    }
	}
	(void) myfclose(fid);
    } else {
	fid = myfopen(str, "w");
	if (fid) {
	    (void) fprintf(fid, "Path: Mail\n");
	    (void) myfclose(fid);
	} else Punt("Can't read or create .mh_profile!");
	(void) strcpy(str, "Mail");
    }
    for (l = strlen(str) - 1; l >= 0 && (str[l] == ' ' || str[l] == '\t'); l--)
	str[l] = 0;
    if (str[0] == '/')
	(void) strcpy(str2, str);
    else
	(void) sprintf(str2, "%s/%s", homeDir, str);
    mailDir = MallocACopy(str2);
    (void) sprintf(str, "%s/draft", mailDir);
    draftFile = MallocACopy(str);
    (void) sprintf(str, "%s/xmhdraft", mailDir);
    xmhDraftFile = MallocACopy(str);

    debug = FALSE;
    tempDir = "/tmp";
    defMhPath = "/usr/local/mh6";
    initialFolderName = "inbox";
    draftsFolderName = "drafts";

    defSendLineWidth = 72;
    defBreakSendLineWidth = 85;
    defPrintCommand = "enscript >/dev/null 2>/dev/null";

    defTocWidth = 300;

    SkipDeleted = TRUE;
    SkipMoved = TRUE;
    SkipCopied = FALSE;

    defHideBoringHeaders = TRUE;
    defHideNullSeqBoxes = FALSE;

    defGeometry = "";
    defViewGeometry = NULL;
    defCompGeometry = NULL;
    defPickGeometry = NULL;

    defTocPercentage = 33;
    defNewMailCheck = TRUE;
    defMakeCheckpoints = FALSE;
    defGrabFocus = FALSE;
    defDoubleClick = FALSE;

    ptr = defTocGeometry;
    XtGetResources(DISPLAY
		   resourcelist, XtNumber(resourcelist), (ArgList)NULL, 0,
		   QDefaultRootWindow(theDisplay),
		   progName, "Xmh", &names, &classes);
    if (ptr) defTocGeometry = ptr;
    XrmFreeNameList(names);
    XrmFreeClassList(classes);

    NullSource = XtCreateEDiskSource("/dev/null", XttextRead);

    x = strlen(defMhPath) - 1;
    if (x > 0 && defMhPath[x] == '/')
	defMhPath[x] = 0;

    if (defTocGeometry == NULL)
	defTocGeometry = defGeometry;
    if (defViewGeometry == NULL)
	defViewGeometry = defGeometry;
    if (defCompGeometry == NULL)
	defCompGeometry = defGeometry;
    if (defPickGeometry == NULL)
	defPickGeometry = defGeometry;

#ifdef X11
    rootwidth = DisplayWidth(theDisplay, theScreen);
    rootheight = DisplayHeight(theDisplay, theScreen);
#endif X11
#ifdef X10
    {
	WindowInfo info;
	XQueryWindow(RootWindow, &info);
	rootwidth = info.width;
	rootheight = info.height;
    }
#endif X10

    gbits = XParseGeometry(defTocGeometry, &x, &y, &width, &height);
    if (!(gbits & HeightValue)) {
        height = 3 * rootheight / 4;
        gbits |= HeightValue;
    }
    if (!(gbits & WidthValue)) {
        width = rootwidth / 2;
        gbits |= WidthValue;
    }
    defTocGeometry = CreateGeometry(gbits, x, y, width, height);

    gbits = XParseGeometry(defViewGeometry, &x, &y, &width, &height);
    if (!(gbits & HeightValue)) {
	height = rootheight / 2;
	gbits |= HeightValue;
    }
    if (!(gbits & WidthValue)) {
	width = rootwidth / 2;
	gbits |= WidthValue;
    }
    defViewGeometry = CreateGeometry(gbits, x, y, width, height);

    gbits = XParseGeometry(defCompGeometry, &x, &y, &width, &height);
    if (!(gbits & HeightValue)) {
	height = rootheight / 2;
	gbits |= HeightValue;
    }
    if (!(gbits & WidthValue)) {
	width = rootwidth / 2;
	gbits |= WidthValue;
    }
    defCompGeometry = CreateGeometry(gbits, x, y, width, height);

    gbits = XParseGeometry(defPickGeometry, &x, &y, &width, &height);
    if (!(gbits & HeightValue)) {
	height = rootheight / 2;
	gbits |= HeightValue;
    }
    if (!(gbits & WidthValue)) {
	width = rootwidth / 2;
	gbits |= WidthValue;
    }
    defPickGeometry = CreateGeometry(gbits, x, y, width, height);

    numScrns = 0;
    scrnList = (Scrn *) XtMalloc(1);
    LastButtonPressed = NULL;

    windowarglist[0].name = XtNwindow;
    labelarglist[0].name = XtNlabel;
    TocInit();
    InitPick();
    IconInit();

if (debug) {(void)fprintf(stderr, "Making screen ... "); (void)fflush(stderr);}

    scrn = CreateNewScrn(STtocAndView);

if (debug) {(void)fprintf(stderr, " setting toc ... "); (void)fflush(stderr);}

    TocSetScrn(TocGetNamed(initialFolderName), scrn);

if (debug) (void)fprintf(stderr, "done\n");

/* if (debug) {(void)fprintf(stderr, "Syncing ... "); (void)fflush(stderr); QXSync(theDisplay, 0); (void)fprintf(stderr, "done\n");} */

    MapScrn(scrn);
    DoubleClickProc = NULL;
}
