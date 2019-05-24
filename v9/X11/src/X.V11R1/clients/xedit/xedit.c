#ifndef lint
static char rcs_id[] = "$Header: xedit.c,v 1.10 87/09/11 08:22:18 toddb Exp $";
#endif

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

#include "xedit.h"

int Editable;
int saved;
int backedup;
int lastChangeNumber;
char *displayname;
char *filename;
char *loadedfile;
char *savedfile;
char *searchstring;
char *replacestring;

Window master;
Window Row1;
Window Row2;
Window fileBox;
Window replaceBox;
Window searchBox;
Window textwindow;
Window messwindow;
Window undoBox;
Window labelwindow;

Window quitbutton;
Window loadbutton;
Window savebutton;
Window editbutton;
Window filenamewindow;

Window searchstringwindow;
Window replacestringwindow;
Window qbutton;
Window ubutton;
Window umbutton;
Window searchbutton;
Window lsearchbutton;
Window replacebutton;
Window replaceallbutton;
Window jumpbutton;

Display *CurDpy;

XtTextSource *source, *asource, *dsource, *psource, *messsource;
XtTextSink *sink;

extern DoQ();

static Arg Row1Desc[]={
        { XtNwindow,    (caddr_t) &quitbutton },
        { XtNwindow,    (caddr_t) &savebutton },
        { XtNwindow,    (caddr_t) &editbutton },
/*        { XtNwindow,    (caddr_t) &qbutton },   */
        { XtNwindow,    (caddr_t) &loadbutton },
        { XtNwindow,	(caddr_t) &filenamewindow },
        { XtNwindow,    (caddr_t) &ubutton },
        { XtNwindow,    (caddr_t) &umbutton },
        { XtNwindow,    (caddr_t) &jumpbutton },
/*        { XtNwindow,    (caddr_t) &linebutton }, */
	{ NULL, NULL }
};

static Arg Row2Desc[]={
        { XtNwindow,    (caddr_t) &lsearchbutton },
        { XtNwindow,    (caddr_t) &searchbutton },
        { XtNwindow,    (caddr_t) &searchstringwindow },
        { XtNwindow,    (caddr_t) &replacebutton },
        { XtNwindow,    (caddr_t) &replaceallbutton },
        { XtNwindow,    (caddr_t) &replacestringwindow },
	{ NULL, NULL }
};


Arg *fixup(args)
  Arg *args;
{
  Arg *arglist = args;
        while(arglist->name){
            arglist->value = *((caddr_t*)arglist->value);
	    arglist++;
        }
    return args;
}

makeButtonsAndBoxes()
{
  int boxHeight;
  static Arg TextArgDesc[] = {
	{ XtNtextOptions , (caddr_t)(scrollVertical | wordBreak) }, 
	{ XtNname,    (caddr_t)"EditWindow" },
  };
  static Arg MessArgs[] = {
	{ XtNtextOptions, (caddr_t)(scrollVertical | wordBreak) }, 
	{ XtNname,    (caddr_t)"MessageWindow" },
  };
  static Arg labelArgs[] = {
	{ XtNjustify,	 (caddr_t)XtjustifyCenter },
	{ XtNlabel, 	(caddr_t)"no file yet" },
  };
    Row1 =  QXtButtonBoxCreate(CurDpy, master, 0,0);
        quitbutton = makeCommandButton(Row1, "Quit", DoQuit);
        savebutton = makeCommandButton(Row1, "Save", DoSave);
        editbutton = makeCommandButton(Row1, "Edit", DoEdit);
/*        qbutton = makeCommandButton(Row1, "q", DoQ);   */
        loadbutton = makeCommandButton(Row1, "Load", DoLoad);
        filenamewindow = makeStringBox(Row1, filename,110);
        ubutton = makeCommandButton(Row1, "Undo", DoUndo);
        umbutton = makeCommandButton(Row1, "More", DoUndoMore);
        jumpbutton = makeCommandButton(Row1, "Jump", DoJump);
/*        linebutton = makeCommandButton(Row1, "Line?", DoLine);  */
    Row2 =  QXtButtonBoxCreate(CurDpy, master, 0,0 );
        lsearchbutton = makeCommandButton(Row2, "<< ", DoSearchLeft);
        searchbutton = makeCommandButton(Row2,"Search >>",DoSearchRight); 
        searchstringwindow = makeStringBox(Row2, searchstring, 120);
        replacebutton= makeCommandButton(Row2, "Replace", DoReplaceOne);
        replaceallbutton = makeCommandButton(Row2, "All", DoReplaceAll);
        replacestringwindow = makeStringBox(Row2, replacestring, 120);
    QXtButtonBoxAddButton(CurDpy, Row1, fixup(Row1Desc), XtNumber(Row1Desc)); 
    QXtButtonBoxAddButton(CurDpy, Row2, fixup(Row2Desc), XtNumber(Row2Desc)); 
    {
	static Arg getargs[] = {
	    { XtNheight, 0 }
        };
        QXtButtonBoxGetValues(CurDpy, Row1, getargs, XtNumber(getargs));
	boxHeight = (int)(getargs[0].value);
    }

    messsource = (XtTextSource*)TCreateEDiskSource("/dev/null", XttextEdit);; 
    messwindow = QXtTextSourceCreate(CurDpy, master, MessArgs,
				    XtNumber(MessArgs), messsource);
    labelwindow=  QXtLabelCreate(CurDpy,master, labelArgs, XtNumber(labelArgs)); 
    textwindow = QXtTextSourceCreate(CurDpy, master, TextArgDesc, 
			            XtNumber(TextArgDesc), source);
    QXtVPanedRefigureMode(CurDpy, master, FALSE);
    QXtVPanedWindowAddPane(CurDpy, master, Row1, 0, boxHeight, 100, FALSE);
    QXtVPanedWindowAddPane(CurDpy, master, Row2, 1, boxHeight,100, FALSE);
    QXtVPanedWindowAddPane(CurDpy, master, messwindow, 2, 30, 5000,TRUE);
    QXtVPanedWindowAddPane(CurDpy, master, labelwindow, 3, 5, 50,FALSE); 
    QXtVPanedWindowAddPane(CurDpy, master, textwindow, 4, 50, 5000,TRUE);
    QXtVPanedRefigureMode(CurDpy, master, TRUE);
}

static XrmResourceDataBase  rdb;
static int mHeight, mWidth, mX, mY;
static char *geometry;
int false = 0;
char *backupNamePrefix;
char *backupNameSuffix;
int editInPlace;
int enableBackups;

static Resource resourcelist[] = {
   {XtNwidth, XtCWidth, XrmRInt, sizeof(int),
         (caddr_t)&mWidth, (caddr_t)NULL},
   {XtNheight, XtCHeight, XrmRInt, sizeof(int),
         (caddr_t)&mHeight, (caddr_t)NULL},
   {XtNx, XtCX, XrmRInt, sizeof(int),
         (caddr_t)&mX, (caddr_t)NULL},
   {XtNy, XtCY, XrmRInt, sizeof(int),
         (caddr_t)&mY, (caddr_t)NULL},
   {"geometry", "Geometry", XrmRString, sizeof(char *),
         (caddr_t)&geometry, NULL},
   {"EditInPlace", "editInPlace", XrmRBoolean, sizeof(int),
         (XtArgVal)&editInPlace, (caddr_t)&false},
   {"EnableBackups", "enableBackups", XrmRBoolean, sizeof(int),
         (XtArgVal)&enableBackups, (caddr_t)&false},
   {"BackupNamePrefix", "backupNamePrefix", XrmRString, sizeof(char *),
         (XtArgVal)&backupNamePrefix, NULL},
   {"BackupNameSuffix", "backupNameSuffix", XrmRString, sizeof(char *),
         (XtArgVal)&backupNameSuffix, NULL},
};


initResources(argc, argv)
  int argc;
  char **argv;
{
  FILE *rfile;
  char str[1024], *displayName;
  int numargs;
  XrmNameList name;
  XrmClassList class;
  static XrmOptionDescRec opTable[] = {
	{"=", "geometry", XrmoptionStickyArg,  (caddr_t)NULL },
  };
  Arg args[5];
    numargs = 0;
   XtInitialize();
    sprintf(str, "%s/.Xdefaults", getenv("HOME"));
    if (rfile = fopen(str, "r")){
        XrmGetDataBase(rfile, &rdb);
        XrmSetCurrentDataBase(rdb);
        fclose(rfile);
    } else {
    /*  use system defaults here? */
    }
    XrmParseCommand(opTable, XtNumber(opTable), "xedit", &argc, argv);
    if(argv[1] && rindex(argv[1], ':')){
       	displayName = argv[1];
   	    argc--; argv++;
    } else
	       displayName = "";
    if ((CurDpy = XOpenDisplay(displayName)) == 0) {
        printf("xedit: Error While trying to open display!\n");
        exit(1);
    }
     QXtGetResources(CurDpy, resourcelist, XtNumber(resourcelist),
           	0,0, QRootWindow(CurDpy, DefaultScreen(CurDpy)),
		         "xedit","Xedit", &name, &class);
    XParseGeometry(geometry, &mX, &mY, &mWidth, &mHeight);
    if((!mHeight) || (!mWidth)){
#ifdef X11
        mWidth = 500;
        mHeight = 500;
#endif X11
#ifdef X10
        OpaqueFrame frame;
        int cwidth, cheight;
        WindowInfo  info;
        frame.bdrwidth = 1;
        frame.border = BlackPixmap;
        frame.background = WhitePixmap;
        master = XCreate(argv[0], argv[0], geometry, "=504x470-0-300",
            &frame, 400, 250);
        XQueryWindow(master, &info);
        XDestroyWindow(master);
        mX = info.x;             
        mY = info.y;
        mWidth = info.width;
        mHeight = info.height;
#endif X10
    }
    MakeArg(XtNname, (caddr_t) "xedit");
    MakeArg(XtNheight, (caddr_t) mHeight);
    MakeArg(XtNwidth, (caddr_t) mWidth);
    MakeArg(XtNx, (caddr_t) mX);
    MakeArg(XtNy, (caddr_t) mY);
    master = QXtVPanedWindowCreate(CurDpy, QRootWindow(CurDpy, 
					DefaultScreen(CurDpy)),
							     args, numargs); 
#ifdef X11
    XtMakeMaster(CurDpy, master);
#endif X11
    QXStoreName(CurDpy, master, "xedit");
#ifdef X11
    XDefineCursor(CurDpy, master, XtGetCursor(CurDpy, XC_left_ptr));  
#endif X11
#ifdef X10
    XDefineCursor(master, XtGetCursor("left_ptr")); 
#endif X10
    if(argv[1]) strcpy(filename, argv[1]);
}

main(argc, argv)
  int argc;
  char **argv;
{
  XEvent event;
    backedup = 0;
    saved = 0;
    filename = malloc(1000);
    searchstring = malloc(1000);
    replacestring = malloc(1000);
    initResources(argc, argv);
    dsource = (XtTextSource*)TCreateEDiskSource("/dev/null", XttextRead); 
    asource = TCreateApAsSource(); 
    source = CreatePSource(dsource, asource);  
    makeButtonsAndBoxes();
    QXMapWindow(CurDpy, master); 
    if (*filename) DoLoad();
    while (1) {
	QXNextEvent(CurDpy, &event);
	XtDispatchEvent(&event);
    }
}

