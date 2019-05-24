/* $Header: uwm.c,v 1.8 87/08/20 19:17:40 swick Exp $ */
#include <X11/copyright.h>

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


/*
 * MODIFICATION HISTORY
 *
 * 000 -- M. Gancarz, DEC Ultrix Engineering Group
 * 001 -- Loretta Guarino Reid, DEC Ultrix Engineering Group,
 *  Western Software Lab. Convert to X11.
 */

#ifndef lint
static char *sccsid = "%W%	%G%";
#endif

#include <sys/time.h>
#include "uwm.h"

#ifdef PROFIL
#include <signal.h>
/*
 * Dummy handler for profiling.
 */
ptrap()
{
    exit(0);
}
#endif

#include <fcntl.h>

#define gray_width 16
#define gray_height 16
static char gray_bits[] = {
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa,
   0x55, 0x55, 0xaa, 0xaa, 0x55, 0x55, 0xaa, 0xaa
};



Bool NeedRootInput=FALSE;
Bool ChkMline();
char *sfilename;
extern FILE *yyin;

/*
 * Main program.
 */
main(argc, argv, environ)
int argc;
char **argv;
char **environ;
{
    int hi;			/* Button event high detail. */
    int lo;			/* Button event low detail. */
    int x, y;                   /* Mouse X and Y coordinates. */
    int root_x, root_y;         /* Mouse root X and Y coordinates. */
    int cur_x, cur_y;		/* Current mouse X and Y coordinates. */
    int down_x, down_y;		/* mouse X and Y at ButtonPress. */
    int str_width;              /* Width in pixels of output string. */
    int pop_width, pop_height;  /* Pop up window width and height. */
    int context;		/* Root, window, or icon context. */
    int ptrmask;		/* for QueryPointer */
    Bool func_stat;		/* If true, function swallowed a ButtonUp. */
    Bool delta_done;		/* If true, then delta functions are done. */
    Bool local;			/* If true, then do not use system defaults. */
    register Binding *bptr;	/* Pointer to Bindings list. */
    char *root_name;		/* Root window name. */
    char *display = NULL;	/* Display name pointer. */
    char message[128];		/* Error message buffer. */
    char *rc_file;		/* Pointer to $HOME/.uwmrc. */
    Window event_win;           /* Event window. */
    Window sub_win;		/* Subwindow for XUpdateMouse calls. */
    Window root;		/* Root window for QueryPointer. */
    XWindowAttributes event_info;	/* Event window info. */
    XEvent button_event; 	 /* Button input event. */
    GC gc;			/* graphics context for gray background */
    XImage grayimage;		/* for gray background */
    XGCValues xgc;		/* to create font GCs */
    char *malloc();
    Bool fallbackMFont = False,	/* using default GC font for menus, */
         fallbackPFont = False,	/* popups, */
         fallbackIFont = False;	/* icons */

#ifdef PROFIL
    signal(SIGTERM, ptrap);
#endif

    /*
     * Set up internal defaults.
     */
    SetVarDefaults();

    /* 
     * Parse the command line arguments.
     */
    Argv = argv;
    Environ = environ;
    argc--, argv++;
    while (argc) {
        if (**argv == '-') {
            if (!(strcmp(*argv, "-f"))) {
                argc--, argv++;
                if ((argc == 0) || (Startup_File[0] != '\0'))
                    Usage();
                strncpy(Startup_File, *argv, NAME_LEN);
            }
            else if (!(strcmp(*argv, "-b")))
                local = TRUE;
            else Usage();
        }
        else display = *argv;
	argc--, argv++;
    }

    /*
     * Initialize the default bindings.
     */
    if (!local)
        InitBindings();

    /*
     * Read in and parse $HOME/.uwmrc, if it exists.
     */
    sfilename = rc_file = malloc(NAME_LEN);
    sprintf(rc_file, "%s/.uwmrc", getenv("HOME"));
    if ((yyin = fopen(rc_file, "r")) != NULL) {
        Lineno = 1;
        yyparse();
        fclose(yyin);
        if (Startup_File_Error)
            Error("Bad .uwmrc file...aborting");
    }

    /* 
     * Read in and parse the startup file from the command line, if
     * specified.
     */
    if (Startup_File[0] != '\0') {
        sfilename = Startup_File;
        if ((yyin = fopen(Startup_File, "r")) == NULL) {
    	sprintf(message, "Cannot open startup file '%s'", Startup_File);
            Error(message);
        }
        Lineno = 1;
        yyparse();
        fclose(yyin);
        if (Startup_File_Error)
            Error("Bad startup file...aborting");
    }

    /*
     * Verify the menu bindings.
     */
    VerifyMenuBindings();
    if (Startup_File_Error)
        Error("Bad startup file...aborting");

    /* 
     * Open the display.
     */
    if ((dpy = XOpenDisplay(display)) == NULL)
        Error("Unable to open display");
    scr = DefaultScreen(dpy); 
/*    XSynchronize(dpy, 1); */

    /*
     * Set XErrorFunction to be non-terminating.
     */
    XSetErrorHandler(XError);


    /*
     * Force child processes to disinherit the TCP file descriptor.
     * This helps shell commands forked and exec'ed from menus
     * to work properly.
     */
    if ((status = fcntl(ConnectionNumber(dpy), F_SETFD, 1)) == -1) {
        perror("uwm: child cannot disinherit TCP fd");
        Error("TCP file descriptor problems");
    }

    /*
     * If the root window has not been named, name it.
     */
    status = XFetchName(dpy, RootWindow(dpy, scr), &root_name);
    if (root_name == NULL) 
    	XStoreName(dpy, RootWindow(dpy, scr), " X Root Window ");
    else free(root_name);


    ScreenHeight = DisplayHeight(dpy, scr);
    ScreenWidth = DisplayWidth(dpy, scr);

    /*
     * Create and store the icon background pixmap.
     */
    GrayPixmap = (Pixmap)XCreatePixmap(dpy, RootWindow(dpy, scr), 
    	gray_width, gray_height, DefaultDepth(dpy,scr));
    xgc.foreground = BlackPixel(dpy, scr);
    xgc.background = WhitePixel(dpy, scr);
    gc = XCreateGC(dpy, GrayPixmap, GCForeground+GCBackground, &xgc);
    grayimage.height = gray_width;
    grayimage.width = gray_height;
    grayimage.xoffset = 0;
    grayimage.format = XYBitmap;
    grayimage.data = (char *)gray_bits;
    grayimage.byte_order = LSBFirst;
    grayimage.bitmap_unit = 8;
    grayimage.bitmap_bit_order = LSBFirst;
    grayimage.bitmap_pad = 16;
    grayimage.bytes_per_line = 2;
    grayimage.depth = 1;
    XPutImage(dpy, GrayPixmap, gc, &grayimage, 0, 0,
	 0, 0, gray_width, gray_height);
    XFreeGC(dpy, gc);
    

    /*
     * Set up icon window, icon cursor and pop-up window color parameters.
     */
    if (Reverse) {
        IBorder = WhitePixel(dpy, scr);
        IBackground =  GrayPixmap;
        ITextForground = WhitePixel(dpy, scr);
        ITextBackground = BlackPixel(dpy, scr);
        PBorder = BlackPixel(dpy, scr);
        PBackground = WhitePixel(dpy, scr);
        PTextForground = BlackPixel(dpy, scr);
        PTextBackground = WhitePixel(dpy, scr);
        MBorder = WhitePixel(dpy, scr);
        MBackground = BlackPixel(dpy, scr);
        MTextForground = WhitePixel(dpy, scr);
        MTextBackground = BlackPixel(dpy, scr);
    }
    else {
        IBorder = BlackPixel(dpy, scr);
        IBackground = GrayPixmap;
        ITextForground = BlackPixel(dpy, scr);
        ITextBackground = WhitePixel(dpy, scr);
        PBorder = WhitePixel(dpy, scr);
        PBackground = BlackPixel(dpy, scr);
        PTextForground = WhitePixel(dpy, scr);
        PTextBackground = BlackPixel(dpy, scr);
        MBorder = BlackPixel(dpy, scr);
        MBackground = WhitePixel(dpy, scr);
        MTextForground = BlackPixel(dpy, scr);
        MTextBackground = WhitePixel(dpy, scr);
    }

    /*
     * Store all the cursors.
     */
    StoreCursors();

    /* 
     * grab the mouse buttons according to the map structure
     */
    Grab_Buttons();

    /*
     * Set initial focus to PointerRoot.
     */
    XSetInputFocus(dpy, PointerRoot, None, CurrentTime);

    /* 
     * watch for initial window mapping and window destruction
     */
    XSelectInput(dpy, RootWindow(dpy, scr), 
      SubstructureNotifyMask|SubstructureRedirectMask|FocusChangeMask|
      (NeedRootInput ? EVENTMASK|OwnerGrabButtonMask : 0));

    /*
     * Retrieve the information structure for the specifed fonts and
     * set the global font information pointers.
     */
    IFontInfo = XLoadQueryFont(dpy, IFontName);
    if (IFontInfo == NULL) {
        fprintf(stderr, "uwm: Unable to open icon font '%s', using server default.\n",
                IFontName);
	IFontInfo = XQueryFont(dpy, DefaultGC(dpy, scr)->gid);
	fallbackIFont = True;
    }
    PFontInfo = XLoadQueryFont(dpy, PFontName);
    if (PFontInfo == NULL) {
        fprintf(stderr, "uwm: Unable to open resize font '%s', using server default.\n",
                PFontName);
	if (fallbackIFont)
	    PFontInfo = IFontInfo;
	else
	    PFontInfo = XQueryFont(dpy, DefaultGC(dpy, scr)->gid);
	fallbackPFont = True;
    }
    MFontInfo = XLoadQueryFont(dpy, MFontName);
    if (MFontInfo == NULL) {
        fprintf(stderr, "uwm: Unable to open menu font '%s', using server default.\n",
                MFontName);
	if (fallbackIFont || fallbackPFont)
	    MFontInfo = fallbackPFont ? PFontInfo : IFontInfo;
	else
	    MFontInfo = XQueryFont(dpy, DefaultGC(dpy, scr)->gid);
	fallbackMFont = True;
    }

    /*
     * Calculate size of the resize pop-up window.
     */
    str_width = XTextWidth(PFontInfo, PText, strlen(PText));
    pop_width = str_width + (PPadding << 1);
    PWidth = pop_width + (PBorderWidth << 1);
    pop_height = PFontInfo->ascent + PFontInfo->descent + (PPadding << 1);
    PHeight = pop_height + (PBorderWidth << 1);

    /*
     * Create the pop-up window.  Create it at (0, 0) for now.  We will
     * move it where we want later.
     */
    Pop = XCreateSimpleWindow(dpy, RootWindow(dpy, scr),
                        0, 0,
                        pop_width, pop_height,
                        PBorderWidth,
                        PBorder, PBackground);
    if (Pop == FAILURE) Error("Can't create pop-up dimension display window.");

    /*
     * Create the menus for later use.
     */
    CreateMenus();

    /*
     * Create graphics context.
     */
    xgc.foreground = ITextForground;
    xgc.background = ITextBackground;
    xgc.font = IFontInfo->fid;
    xgc.graphics_exposures = FALSE;
    IconGC = XCreateGC(dpy, 
    	RootWindow(dpy, scr),
	GCForeground+GCBackground+GCGraphicsExposures
		       +(fallbackIFont ? 0 : GCFont), &xgc);
    xgc.foreground = MTextForground;
    xgc.background = MTextBackground;
    xgc.font = MFontInfo->fid;
    MenuGC = XCreateGC(dpy, 
    	RootWindow(dpy, scr),
	GCForeground+GCBackground+(fallbackMFont ? 0 : GCFont), &xgc);
    xgc.function = GXinvert;
    xgc.plane_mask = MTextForground ^ MTextBackground;
    MenuInvGC = XCreateGC(dpy, 
    	RootWindow(dpy, scr), GCForeground+GCFunction+GCPlaneMask, &xgc);
    xgc.foreground = PTextForground;
    xgc.background = PTextBackground;
    xgc.font = PFontInfo->fid;
    PopGC = XCreateGC(dpy, 
    	RootWindow(dpy, scr),
	GCForeground+GCBackground+(fallbackPFont ? 0 : GCFont), &xgc);
    xgc.line_width = DRAW_WIDTH;
    xgc.foreground = DRAW_VALUE;
    xgc.function = DRAW_FUNC;
    xgc.subwindow_mode = IncludeInferiors;
    DrawGC = XCreateGC(dpy, RootWindow(dpy, scr), 
      GCLineWidth+GCForeground+GCFunction+GCSubwindowMode, &xgc);


    /*
     * Tell the user we're alive and well.
     */
    XBell(dpy, VOLUME_PERCENTAGE(Volume));

    /* 
     * Main command loop.
     */
    while (TRUE) {

        delta_done = func_stat = FALSE;

        /*
         * Get the next mouse button event.  Spin our wheels until
         * a ButtonPressed event is returned.
         * Note that mouse events within an icon window are handled
         * in the "GetButton" function or by the icon's owner if
         * it is not uwm.
         */
        while (TRUE) {
            if (!GetButton(&button_event)) continue;
            if (button_event.type == ButtonPress) break;
        }

	/* save mouse coords in case we want them later for a delta action */
	down_x = ((XButtonPressedEvent *)&button_event)->x;
	down_y = ((XButtonPressedEvent *)&button_event)->y;
        /*
         * Okay, determine the event window and mouse coordinates.
         */
        status = XTranslateCoordinates(dpy, 
				    RootWindow(dpy, scr), RootWindow(dpy, scr),
                                    ((XButtonPressedEvent *)&button_event)->x, 
				    ((XButtonPressedEvent *)&button_event)->y,
                                    &x, &y,
                                    &event_win);

        if (status == FAILURE) continue;

        /*
         * Determine the event window and context.
         */
        if (event_win == 0) {
                event_win = RootWindow(dpy, scr);
                context = ROOT;
        } else {
            if (IsIcon(event_win, 0, 0, FALSE, NULL))
                context = ICON;
            else context = WINDOW;
        }

        /*
         * Get the button event detail.
         */
        lo = ((XButtonPressedEvent *)&button_event)->button;
        hi = ((XButtonPressedEvent *)&button_event)->state;

        /*
         * Determine which function was selected and invoke it.
         */
        for(bptr = Blist; bptr; bptr = bptr->next) {

            if ((bptr->button != lo) ||
                (((int)bptr->mask & ModMask) != hi))
                continue;

            if (bptr->context != context)
                continue;

            if (!(bptr->mask & ButtonDown))
                continue;

            /*
             * Found a match! Invoke the function.
             */
            if ((*bptr->func)(event_win,
                              (int)bptr->mask & ModMask,
                              bptr->button,
                              x, y,
                              bptr->menu)) {
                func_stat = TRUE;
                break;
            }
        }

        /*
         * If the function ate the ButtonUp event, then restart the loop.
         */
        if (func_stat) continue;

        while(TRUE) {
            /*
             * Wait for the next button event.
             */
            if (XPending(dpy) && GetButton(&button_event)) {
    
                /*
                 * If it's not a release of the same button that was pressed,
                 * don't do the function bound to 'ButtonUp'.
                 */
                if (button_event.type != ButtonRelease)
                    break;
                if (lo != ((XButtonReleasedEvent *)&button_event)->button)
                    break;
                if ((hi|ButtonMask(lo)) != 
		     ((XButtonReleasedEvent *)&button_event)->state)
                    break;
        
                /*
                 * Okay, determine the event window and mouse coordinates.
                 */
                status = XTranslateCoordinates(dpy, 
				    RootWindow(dpy, scr), RootWindow(dpy, scr),
                                    ((XButtonReleasedEvent *)&button_event)->x,
				    ((XButtonReleasedEvent *)&button_event)->y,
                                    &x, &y,
                                    &event_win);

                if (status == FAILURE) break;

                if (event_win == 0) {
                        event_win = RootWindow(dpy, scr);
                        context = ROOT;
                } else {
                    if (IsIcon(event_win, 0, 0, FALSE, NULL))
                        context = ICON;
                    else context = WINDOW;
                }
        
                /*
                 * Determine which function was selected and invoke it.
                 */
                for(bptr = Blist; bptr; bptr = bptr->next) {
        
                    if ((bptr->button != lo) ||
                        (((int)bptr->mask & ModMask) != hi))
                        continue;
        
                    if (bptr->context != context)
                        continue;
        
                    if (!(bptr->mask & ButtonUp))
                        continue;
        
                    /*
                     * Found a match! Invoke the function.
                     */
                    (*bptr->func)(event_win,
                                  (int)bptr->mask & ModMask,
                                  bptr->button,
                                  x, y,
                                  bptr->menu);
                }
                break;
            }
    
            XQueryPointer(dpy, RootWindow(dpy, scr), 
	    	&root, &event_win, &root_x, &root_y, &cur_x, &cur_y, &ptrmask);
            if (!delta_done &&
                ((abs(cur_x - x) > Delta) || (abs(cur_y - y) > Delta))) {
                /*
                 * Delta functions are done once (and only once.)
                 */
                delta_done = TRUE;

                /*
                 * Determine the new event window's coordinates.
		 * from the original ButtonPress event
                 */
                status = XTranslateCoordinates(dpy, 
			  RootWindow(dpy, scr), RootWindow(dpy, scr),
			  down_x, down_y, &x, &y, &event_win);
                if (status == FAILURE) break;

                /*
                 * Determine the event window and context.
                 */
                if (event_win == 0) {
                        event_win = RootWindow(dpy, scr);
                        context = ROOT;
                } else {
                    if (IsIcon(event_win, 0, 0, FALSE, NULL))
                        context = ICON;
                    else context = WINDOW;
                }
    
                /*
                 * Determine which function was selected and invoke it.
                 */
                for(bptr = Blist; bptr; bptr = bptr->next) {
        
                    if ((bptr->button != lo) ||
                        (((int)bptr->mask & ModMask) != hi))
                        continue;
        
                    if (bptr->context != context)
                        continue;
        
                    if (!(bptr->mask & DeltaMotion))
                        continue;
        
                    /*
                     * Found a match! Invoke the function.
                     */
                    if ((*bptr->func)(event_win,
                                      (int)bptr->mask & ModMask,
                                      bptr->button,
                                      x, y,
                                      bptr->menu)) {
                        func_stat = TRUE;
                        break;
                    }
                }
                /*
                 * If the function ate the ButtonUp event,
                 * then restart the loop.
                 */
                if (func_stat) break;
            }
        }
    }
}

/*
 * Initialize the default bindings.  First, write the character array
 * out to a temp file, then point the parser to it and read it in.
 * Afterwards, we unlink the temp file.
 */
InitBindings()
{
    char *mktemp();
    char *tempfile = TEMPFILE;	/* Temporary filename. */
    register FILE *fp;		/* Temporary file pointer. */
    register char **ptr;	/* Default bindings string array pointer. */

    /*
     * Create and write the temp file.
     */
    sfilename = mktemp(tempfile);
    if ((fp = fopen(tempfile, "w")) == NULL) {
        perror("uwm: cannot create temp file");
        exit(1);
    }
    for (ptr = DefaultBindings; *ptr; ptr++) {
        fputs(*ptr, fp);
        fputc('\n', fp);
    }
    fclose(fp);

    /*
     * Read in the bindings from the temp file and parse them.
     */
    if ((yyin = fopen(tempfile, "r")) == NULL) {
        perror("uwm: cannot open temp file");
        exit(1);
    }
    Lineno = 1;
    yyparse();
    fclose(yyin);
    unlink(tempfile);
    if (Startup_File_Error)
        Error("Bad default bindings...aborting");

    /*
     * Parse the system startup file, if one exists.
     */
    if ((yyin = fopen(SYSFILE, "r")) != NULL) {
        sfilename = SYSFILE;
        Lineno = 1;
        yyparse();
        fclose(yyin);
        if (Startup_File_Error)
            Error("Bad system startup file...aborting");
    }
}

/*
 * Verify menu bindings by checking that a menu that is mapped actually
 * exists.  Stash a pointer in the binding to the relevant menu info data
 * structure.
 * Check nested menu consistency.
 */
VerifyMenuBindings()
{
    Binding *bptr;
    MenuLink *mptr;

    for(bptr = Blist; bptr; bptr = bptr->next) {
        if (bptr->func == Menu) {
            for(mptr = Menus; mptr; mptr = mptr->next) {
                if(!(strcmp(bptr->menuname, mptr->menu->name))) {
                    bptr->menu = mptr->menu;
                    break;
                }
            }
            if (mptr == NULL) {
                fprintf(stderr,
                        "uwm: non-existent menu reference: \"%s\"\n",
                        bptr->menuname);
                Startup_File_Error = TRUE;
            }
        }
    }
    CheckMenus();
}

/*
 * Check nested menu consistency by verifying that every menu line that
 * calls another menu references a menu that actually exists.
 */
CheckMenus()
{
    MenuLink *ptr;
    Bool errflag = FALSE;

    for(ptr = Menus; ptr; ptr = ptr->next) {
        if (ChkMline(ptr->menu))
            errflag = TRUE;
    }
    if (errflag)
        Error("Nested menu inconsistency");
}

Bool ChkMline(menu)
MenuInfo *menu;
{
    MenuLine *ptr;
    MenuLink *lptr;
    Bool errflag = FALSE;

    for(ptr = menu->line; ptr; ptr = ptr->next) {
        if (ptr->type == IsMenuFunction) {
            for(lptr = Menus; lptr; lptr = lptr->next) {
                if(!(strcmp(ptr->text, lptr->menu->name))) {
                    ptr->menu = lptr->menu;
                    break;
                }
            }
            if (lptr == NULL) {
                fprintf(stderr,
                        "uwm: non-existent menu reference: \"%s\"\n",
                        ptr->text);
                errflag = TRUE;
            }
        }
    }
    return(errflag);
}

/*
 * Grab the mouse buttons according to the bindings list.
 */
Grab_Buttons()
{
    Binding *bptr;

    for(bptr = Blist; bptr; bptr = bptr->next)
        if ((bptr->context & (WINDOW | ICON | ROOT)) == ROOT) {

	    /* don't grab buttons if you don't have to - allow application
	    access to buttons unless context includes window or icon */

	    NeedRootInput = TRUE;
	}
	else { 
	    /* context includes a window, so must grab */
	    Grab(bptr->mask);
	}
}

/*
 * Grab a mouse button according to the given mask.
 */
Grab(mask)
unsigned int mask;
{
    unsigned int m = LeftMask | MiddleMask | RightMask;

    switch (mask & m) {
    case LeftMask:
        XGrabButton(dpy, LeftButton,  mask & ModMask,
		RootWindow(dpy, scr), TRUE, EVENTMASK,
		GrabModeAsync, GrabModeAsync, None, LeftButtonCursor);
        break;

    case MiddleMask:
        XGrabButton(dpy, MiddleButton,  mask & ModMask,
		RootWindow(dpy, scr), TRUE, EVENTMASK,
		GrabModeAsync, GrabModeAsync, None, MiddleButtonCursor);
        break;

    case RightMask:
        XGrabButton(dpy, RightButton,  mask & ModMask,
		RootWindow(dpy, scr), TRUE, EVENTMASK,
		GrabModeAsync, GrabModeAsync, None, RightButtonCursor);
        break;
    }
}

/*
 * Restore cursor to normal state.
 */
ResetCursor(button)
int button;
{

    switch (button) {
    case LeftButton:
        XChangeActivePointerGrab(
		dpy, EVENTMASK, LeftButtonCursor, CurrentTime);
        break;

    case MiddleButton:
        XChangeActivePointerGrab(
		dpy, EVENTMASK, MiddleButtonCursor, CurrentTime);
        break;

    case RightButton:
        XChangeActivePointerGrab(
		dpy, EVENTMASK, RightButtonCursor, CurrentTime);
        break;
    }
}

/*
 * error routine for .uwmrc parser
 */
yyerror(s)
char*s;
{
    fprintf(stderr, "uwm: %s: %d: %s\n", sfilename, Lineno, s);
    Startup_File_Error = TRUE;
}

/*
 * Print usage message and quit.
 */
Usage()
{
    fputs("Usage:  uwm [-b] [-f <file>] [<host>:<display>]\n\n", stderr);
    fputs("The -b option bypasses system and default bindings\n", stderr);
    fputs("The -f option specifies an additional startup file\n", stderr);
    exit(1);
}

/*
 * error handler for X I/O errors
 */
XIOError(dsp)
Display *dsp;
{
    perror("uwm");
    exit(3);
}

SetVarDefaults()
{
    strcpy(IFontName, DEF_FONT);
    strcpy(PFontName, DEF_FONT);
    strcpy(MFontName, DEF_FONT);
    Delta = DEF_DELTA;
    IBorderWidth = DEF_ICON_BORDER_WIDTH;
    HIconPad = DEF_ICON_PADDING;
    VIconPad = DEF_ICON_PADDING;
    PBorderWidth = DEF_POP_BORDER_WIDTH;
    PPadding = DEF_POP_PADDING;
    MBorderWidth = DEF_MENU_BORDER_WIDTH;
    HMenuPad = DEF_MENU_PADDING;
    VMenuPad = DEF_MENU_PADDING;
    Volume = DEF_VOLUME;
    Pushval = DEF_PUSH;
    FocusSetByUser = FALSE;
}
