/* $Header: wsimple.c,v 1.2 87/09/11 23:21:27 sun Exp $ */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <stdio.h>
/*
 * Other_stuff.h: Definitions of routines in other_stuff.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 *
 * Send bugs, etc. to chariot@athena.mit.edu.
 */

unsigned long Resolve_Color();
Pixmap Bitmap_To_Pixmap();
Window Select_Window();
void out();
void blip();
Window Window_With_Name();
/*
 * Just_display: A group of routines designed to make the writting of simple
 *               X11 applications which open a display but do not open
 *               any windows much faster and easier.  Unless a routine says
 *               otherwise, it may be assumed to require program_name, dpy,
 *               and screen already defined on entry.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 *
 * Send bugs, etc. to chariot@athena.mit.edu.
 */

#include <strings.h>

#define NULL 0

/* This stuff is defined in the calling program by just_display.h */
extern char *program_name;
extern Display *dpy;
extern int screen;


/*
 * Standard fatal error routine - call like printf but maximum of 7 arguments.
 * Does not require dpy or screen defined.
 */
void Fatal_Error(msg, arg0,arg1,arg2,arg3,arg4,arg5,arg6)
char *msg;
char *arg0, *arg1, *arg2, *arg3, *arg4, *arg5, *arg6;
{
	fflush(stdout);
	fflush(stderr);
	fprintf(stderr, "%s: error: ", program_name);
	fprintf(stderr, msg, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
	fprintf(stderr, "\n");
	exit(1);
}


/*
 * Malloc: like malloc but handles out of memory using Fatal_Error.
 */
char *Malloc(size)
     unsigned size;
{
	char *data, *malloc();

	if (!(data = malloc(size)))
	  Fatal_Error("Out of memory!");

	return(data);
}
	

/*
 * Realloc: like Malloc except for realloc, handles NULL using Malloc.
 */
char *Realloc(ptr, size)
        char *ptr;
        int size;
{
	char *new_ptr, *realloc();

	if (!ptr)
	  return(Malloc(size));

	if (!(new_ptr = realloc(ptr, size)))
	  Fatal_Error("Out of memory!");

	return(new_ptr);
}


/*
 * Get_Display_Name: Routine which returns the name of the display we
 *                   are supposed to use.  This is either the display name
 *                   given in the list of command arguments or if no name
 *                   is given, the value of the DISPLAY environmental
 *                   variable.  If DISPLAY is unset, NULL is returned.
 *                   The main program should pass its command arguments
 *                   to this routine.  The display argument if any is found
 *                   will be removed from the list of command arguments.
 *                   Any command argument containing a ':' which occurs
 *                   before a '-' is considered to be a display.  If
 *                   two or more of these occur, only the first will be used.
 *                   Does not require dpy or screen defined on entry.
 */
char *Get_Display_Name(argc, argv)
     int *argc;          /* MODIFIED */
     char **argv;        /* MODIFIED */
{
	char *display;
	char *getenv();
	int nargc=1;
	int count;
	char **nargv;
	
	/* Get default display name from environmental variable DISPLAY */
	display = getenv("DISPLAY");

	/*
	 * Search for a user supplied overide command argument of the
         * form host:display and remove it from command arguments if found.
	 */
	count = *argc;
	nargv = argv + 1;
	for (count--, argv++; count>0; count--, argv++) {
	  if (!strcmp("-", *argv)) {          /* Don't search past a "-" */
	    break;
	  }
	  if (index(*argv, ':')) {
	    display = *(argv++);
	    count--;
	    break;                            /* Only use first display name */
	  }
	  *(nargv++) = *argv;
	  nargc++;
	}
	while (count>0) {
	  *(nargv++) = *(argv++);
	  nargc++; count--;
	}
	*argc = nargc;
	
	return(display);
}


/*
 * Open_Display: Routine to open a display with correct error handling.
 *               Does not require dpy or screen defined on entry.
 */
Display *Open_Display(display_name)
char *display_name;
{
	Display *d;

	d = XOpenDisplay(display_name);
	if (d == NULL) {
		if (display_name == NULL)
		  Fatal_Error("Could not open default display!");
		else
		  Fatal_Error("Could not open display %s!", display_name);
	}

	return(d);
}


/*
 * Setup_Display_And_Screen: This routine opens up the correct display (i.e.,
 *                           it calls Get_Display_Name) and then stores a
 *                           pointer to it in dpy.  The default screen
 *                           for this display is then stored in screen.
 *                           Does not require dpy or screen defined.
 */
void Setup_Display_And_Screen(argc, argv)
int *argc;      /* MODIFIED */
char **argv;    /* MODIFIED */
{
	dpy = Open_Display(Get_Display_Name(argc, argv));
	screen = DefaultScreen(dpy);
}


/*
 * Open_Font: This routine opens a font with error handling.
 */
XFontStruct *Open_Font(name)
char *name;
{
	XFontStruct *font;

	if (!(font=XLoadQueryFont(dpy, name)))
	  Fatal_Error("Unable to open font %s!", name);

	return(font);
}


/*
 * Beep: Routine to beep the display.
 */
void Beep()
{
	XBell(dpy, 50);
}


/*
 * ReadBitmapFile: same as XReadBitmapFile except it returns the bitmap
 *                 directly and handles errors using Fatal_Error.
 */
static void _bitmap_error(status, filename)
     int status;
     char *filename;
{
  if (status == BitmapOpenFailed)
    Fatal_Error("Can't open file %s!", filename);
  else if (status == BitmapFileInvalid)
    Fatal_Error("file %s: Bad bitmap format.", filename);
  else
    Fatal_Error("Out of memory!");
}

Pixmap ReadBitmapFile(d, filename, width, height, x_hot, y_hot)
     Drawable d;
     char *filename;
     int *width, *height, *x_hot, *y_hot;
{
  Pixmap bitmap;
  int status;

  status = XReadBitmapFile(dpy, RootWindow(dpy, screen), filename, width,
			   height, &bitmap, x_hot, y_hot);
  if (status != BitmapSuccess)
    _bitmap_error(status, filename);

  return(bitmap);
}


/*
 * WriteBitmapFile: same as XWriteBitmapFile except it handles errors
 *                  using Fatal_Error.
 */
void WriteBitmapFile(filename, bitmap, width, height, x_hot, y_hot)
     char *filename;
     Pixmap bitmap;
     int width, height, x_hot, y_hot;
{
  int status;

  status= XWriteBitmapFile(dpy, filename, bitmap, width, height, x_hot,
			   y_hot);
  if (status != BitmapSuccess)
    _bitmap_error(status, filename);
}


/*
 * Select_Window_Args: a rountine to provide a common interface for
 *                     applications that need to allow the user to select one
 *                     window on the screen for special consideration.
 *                     This routine implements the following command line
 *                     arguments:
 *
 *                       -root            Selects the root window.
 *                       -id <id>         Selects window with id <id>. <id> may
 *                                        be either in decimal or hex.
 *                       -name <name>     Selects the window with name <name>.
 *
 *                     Call as Select_Window_Args(&argc, argv) in main before
 *                     parsing any of your program's command line arguments.
 *                     Select_Window_Args will remove its arguments so that
 *                     your program does not have to worry about them.
 *                     The window returned is the window selected or 0 if
 *                     none of the above arguments was present.  If 0 is
 *                     returned, Select_Window should probably be called after
 *                     all command line arguments, and other setup is done.
 *                     For examples of usage, see xwininfo, xwd, or xprop.
 */
Window Select_Window_Args(rargc, argv)
     int *rargc;
     char **argv;
#define ARGC (*rargc)
{
	int nargc=1;
	int argc;
	char **nargv;
	Window w=0;

	nargv = argv+1; argc = ARGC;
#define OPTION argv[0]
#define NXTOPTP ++argv, --argc>0
#define NXTOPT if (++argv, --argc==0) usage()
#define COPYOPT nargv++[0]=OPTION; nargc++

	while (NXTOPTP) {
		if (!strcmp(OPTION, "-")) {
			COPYOPT;
			while (NXTOPTP)
			  COPYOPT;
			break;
		}
		if (!strcmp(OPTION, "-root")) {
			w=RootWindow(dpy, screen);
			continue;
		}
		if (!strcmp(OPTION, "-name")) {
			NXTOPT;
			w = Window_With_Name(dpy, RootWindow(dpy, screen),
					     OPTION);
			if (!w)
			  Fatal_Error("No window with name %s exists!",OPTION);
			continue;
		}
		if (!strcmp(OPTION, "-id")) {
			NXTOPT;
			w=0;
			sscanf(OPTION, "0x%lx", &w);
			if (!w)
			  sscanf(OPTION, "%ld", &w);
			if (!w)
			  Fatal_Error("Invalid window id format: %s.", OPTION);
			continue;
		}
		COPYOPT;
	}
	ARGC = nargc;
	
	return(w);
}

/*
 * Just_one_window: A group of routines designed to make the writting of simple
 *                  X11 applications which open a display and exactly one real
 *                  window much faster and easier.  Unless a routine says
 *                  otherwise, it requires program_name, dpy, screen, and
 *                  wind to be defined on entry.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 *
 * Send bugs, etc. to chariot@athena.mit.edu.
 */


#define NULL 0

/* This stuff is defined in program by just_display.h */
extern char *program_name;
extern Display *dpy;
extern int screen;

/* This stuff is defined in the calling program by just_one_window.h */
extern Window wind;
extern char **_commands;
extern int _number_of_commands;
extern char *title, *icon_name, *icon_bitmap_file;
extern char *geometry,*border_color, *back_color, *fore_color, *body_font_name;
extern int border_width, reverse;
extern unsigned long border, background, foreground;
extern XFontStruct *body_font;
extern XSizeHints size_hints;
extern Pixmap icon_pixmap;

/*
 * Get_X_Defaults: This routine reads in the user's .Xdefaults file and
 *                 uses it to update the X Options to the user's personal
 *                 defaults.  Note: this routines does not require wind
 *                 defined on entry.
 */
void Get_X_Defaults()
{
	char *option;

	if (option = XGetDefault(dpy, program_name, "ReverseVideo")) {
		if (!strcmp("on", option))
		  reverse=1;
		if (!strcmp("off", option))
		  reverse=0;
	}
	if (option = XGetDefault(dpy, program_name, "BorderWidth"))
	  border_width = atoi(option);
	if (option = XGetDefault(dpy, program_name, "BorderColor"))
	  border_color = option;
	if (option = XGetDefault(dpy, program_name, "Border"))
	  border_color = option;
	if (option = XGetDefault(dpy, program_name, "Background"))
	  back_color = option;
	if (option = XGetDefault(dpy, program_name, "Foreground"))
	  fore_color = option;
	if (option = XGetDefault(dpy, program_name, "BodyFont"))
	  body_font_name = option;
	if (option = XGetDefault(dpy, program_name, "Title"))
	  title = option;
	if (option = XGetDefault(dpy, program_name, "IconName"))
	  icon_name = option;
	if (option = XGetDefault(dpy, program_name, "IconBitmap"))
	  icon_bitmap_file = option;
}


/*
 * Get_X_Arguments: This routine takes a program's command argument list and
 *                  extracts all standard X arguments and uses these to
 *                  set the X Options with the user's overides.  For Options
 *                  requiring a lookup to get the real value (e.g., a color)
 *                  the lookup is not done and only the name is stored.
 *                  The X arguments are removed from the command argument
 *                  list by compressing the list.  *argc is changed
 *                  appropiatly.  All arguments looking like a geometry
 *                  are also removed unless they follow a '-'.  In no case,
 *                  are any arguments beyond a singe '-' examined or
 *                  changed.  This is to allow a file name with a ':'
 *                  for instance.  The '-' will be left in the argument list.
 *                  This routine does not require wind be defined.
 *                  
 */
void Get_X_Arguments(argc, argv)
int *argc;       /* Modified */
char **argv;     /* Modified */
{
	int i;
	int nargc;
	char **nargv;

	nargv = argv+1;
	nargc = 1;

	for (i=1; i<*argc; i++) {
		if (argv[i][0] == '=') {
			geometry = argv[i];
			continue;
		}
		if (!strcmp(argv[i],"-rv") || !strcmp(argv[i],"-reverse")) {
                        reverse = 1;
                        continue;
                }
                if (!strcmp(argv[i],"-nm") || !strcmp(argv[i],"-normal")) {
			reverse = 0;
			continue;
		}
		if (!strcmp(argv[i],"-fw") || !strcmp(argv[i],"-forward")) {
			reverse = 0;
			continue;
		}
		if (!strcmp(argv[i],"-bw") || !strcmp(argv[i],"-borderwidth")){
			if (++i >= *argc)
			  usage();
			border_width = atoi(argv[i]);
			continue;
		}
		if (!strcmp(argv[i],"-bd") || !strcmp(argv[i],"-bordercolor")){
			if (++i >= *argc)
			  usage();
			border_color = argv[i];
			continue;
		}
		if (!strcmp(argv[i],"-fg") || !strcmp(argv[i],"-foreground")) {
			if (++i >= *argc)
			  usage();
			fore_color = argv[i];
			continue;
		}
		if (!strcmp(argv[i],"-bg") || !strcmp(argv[i],"-background")) {
			if (++i >= *argc)
			  usage();
			back_color = argv[i];
			continue;
		}
		if (!strcmp(argv[i],"-bf") || !strcmp(argv[i],"-bodyfont")) {
			if (++i >= *argc)
			  usage();
			body_font_name = argv[i];
			continue;
		}
		if (!strcmp(argv[i],"-tl") || !strcmp(argv[i],"-title")) {
			if (++i >= *argc)
			  usage();
			title = argv[i];
			continue;
		}
		if (!strcmp(argv[i],"-in") || !strcmp(argv[i],"-iconname")) {
			if (++i >= *argc)
			  usage();
			icon_name = argv[i];
			continue;
		}
		if (!strcmp(argv[i],"-ib") || !strcmp(argv[i],"-icon")) {
			if (++i >= *argc)
			  usage();
			icon_bitmap_file = argv[i];
			continue;
		}

		if (!strcmp(argv[i],"-")) {
			while (i<*argc) {
				nargv[0] = argv[i++];
				nargc++;
			}
			continue;
		}
			
		*(nargv++) = argv[i];
		nargc++;
	}
	*argc = nargc;
}

/*
 * Get_X_Options: The routine does the "right" thing to get the X Options.
 *                It is provided basically as a binding procedure.
 *                This routine requires only that program_name is defined on
 *                entry.  As a side effect, both dpy & screen are setup & the
 *                "right" display is opened.
 */
void Get_X_Options(argc, argv)
int *argc;      /* Modified */
char **argv;    /* Modified */
{
	Setup_Display_And_Screen(argc, argv);
	Get_X_Defaults();

	Get_X_Arguments(argc, argv);
}

/*
 * Resolve_X_Options: This routine takes off where Get_X_Options left off.
 *                    It resolves each of the X Options left as names by
 *                    Get_X_Options to their real value.  Wind need not
 *                    be set on entry.
 *                    Colors are not resolved.  They must be resolved with
 *                    Resolve_X_Colors AFTER the window wind is created.
 */
void Resolve_X_Options()
{
	XFontStruct *Open_Font();
	int status, width, height;
	Pixmap bitmap;
	GC gc;
	XGCValues gc_init;
	unsigned long temp;

	/* Handle Geometry */
	if (geometry) {
		status = XParseGeometry(geometry, &(size_hints.x),
					&(size_hints.y),
					&(size_hints.width),
					&(size_hints.height));
		if (status & (XValue|YValue)) {
			size_hints.flags |= USPosition;
			size_hints.flags &= ~PPosition;
		}
		if (status & (WidthValue|HeightValue)) {
			size_hints.flags |= USSize;
			size_hints.flags &= ~PSize;
		}
	}

	/* Handle body font */
	body_font = Open_Font(body_font_name);

	/* Handle no icon name specified, default = use title */
	if (!icon_name)
	  icon_name = title;

	/* Handle icon bitmap if any */
	if (icon_bitmap_file) {
		bitmap = ReadBitmapFile(RootWindow(dpy, screen),
					icon_bitmap_file,
					&width, &height, NULL, NULL);
		gc_init.foreground = Resolve_Color(RootWindow(dpy, screen),
						   fore_color);
		gc_init.background = Resolve_Color(RootWindow(dpy, screen),
						   back_color);
		if (reverse) {
			temp=gc_init.foreground;
			gc_init.foreground=gc_init.background;
			gc_init.background=temp;
		}
		gc = XCreateGC(dpy, RootWindow(dpy, screen),
			       GCForeground|GCBackground, &gc_init);
		icon_pixmap = Bitmap_To_Pixmap(dpy, RootWindow(dpy, screen),
					       gc, bitmap, width, height);
	}
}

/*
 * Resolve_X_Colors: This routine resolves the X Options involving colors.
 *                   This must be called AFTER the default window has
 *                   been created and stored in wind.
 */
void Resolve_X_Colors()
{
	unsigned long temp;

	foreground = Resolve_Color(wind, fore_color);
	background = Resolve_Color(wind, back_color);
	border = Resolve_Color(wind, border_color);
	if (reverse) {
		temp = foreground;
		foreground = background;
		background = temp;
	}
}

/*
 * Create_Default_Window: This routine is used once the X Options have been
 *                        gotten and resolved (except for resolving the colors)
 *                        It opens up the default window with the placement,
 *                        size, colors, etc. specified by the X Options.
 *                        The default window is stored in wind.  Wind need
 *                        not be defined on entry but the X Options must be
 *                        set before entry.
 */
void Create_Default_Window()
{
	wind = XCreateSimpleWindow(dpy, RootWindow(dpy, screen), 
				   size_hints.x, size_hints.y,
				   size_hints.width, size_hints.height,
				   border_width, 0, 0);
	if (!wind)
	  Fatal_Error("Unable to create a window!");

	Resolve_X_Colors();

	XSetWindowBackground(dpy, wind, background);
	XSetWindowBorder(dpy, wind, border);

	XSetStandardProperties(dpy, wind, title, icon_name, None, _commands,
			       _number_of_commands, &size_hints);
}

/*
 * Get_Default: This routine returns a graphics context suitable for use in
 *              drawing into the default window.  I.e., it has the right
 *              colors, font, etc.  Note: This routine always returns a 
 *              new graphics context.  X Options must be set on entry.
 */
GC Get_Default_GC()
{
	XGCValues gc_init;

	gc_init.foreground = foreground;
	gc_init.background = background;
	gc_init.font = body_font->fid;

	return(XCreateGC(dpy, wind, GCFont|GCForeground|GCBackground,
			 &gc_init));
}


/*
 * _Save_Commands: internal procedure to save away list of commands to program
 *                 and set command name of program.
 */
_Save_Commands(argc, argv)
     int argc;
     char **argv;
{
	program_name = argv[0];
	_number_of_commands = argc;

	_commands=(char **)Malloc(argc*sizeof(char *));

	bcopy(argv, _commands, argc*sizeof(char *));
}
/*
 * Other_stuff: A group of routines which do common X11 tasks.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 *
 * Send bugs, etc. to chariot@athena.mit.edu.
 */


#define NULL 0

extern Display *dpy;
extern int screen;

/*
 * Resolve_Color: This routine takes a color name and returns the pixel #
 *                that when used in the window w will be of color name.
 *                (WARNING:  The colormap of w MAY be modified! )
 *                If colors are run out of, only the first n colors will be
 *                as correct as the hardware can make them where n depends
 *                on the display.  This routine does not require wind to
 *                be defined.
 */
unsigned long Resolve_Color(w, name)
     Window w;
     char *name;
{
	XColor c;
	Colormap colormap;
	XWindowAttributes wind_info;

	/*
	 * The following is a hack to insure machines without a rgb table
	 * handle at least white & black right.
	 */
	if (!strcmp(name, "white"))
	  name="#ffffffffffff";
	if (!strcmp(name, "black"))
	  name="#000000000000";

	XGetWindowAttributes(dpy, w, &wind_info);
	colormap = wind_info.colormap;

	if (!XParseColor(dpy, colormap, name, &c))
	  Fatal_Error("Bad color format '%s'.", name);

	if (!XAllocColor(dpy, colormap, &c))
	  Fatal_Error("XAllocColor failed!");

	return(c.pixel);
}


/*
 * Bitmap_To_Pixmap: Convert a bitmap to a 2 colored pixmap.  The colors come
 *                   from the foreground and background colors of the gc.
 *                   Width and height are required solely for efficiency.
 *                   If needed, they can be obtained via. XGetGeometry.
 */
Pixmap Bitmap_To_Pixmap(dpy, d, gc, bitmap, width, height)
     Display *dpy;
     Drawable d;
     GC gc;
     Pixmap bitmap;
     int width, height;
{
  Pixmap pix;
  int x, depth;
  Drawable root;

  if (!XGetGeometry(dpy, d, &root, &x, &x, &x, &x, &x, &depth))
    return(0);

  pix = XCreatePixmap(dpy, d, width, height, depth);

  XCopyPlane(dpy, bitmap, pix, gc, 0, 0, width, height, 0, 0, 1);

  return(pix);
}


/*
 * outl: a debugging routine.  Flushes stdout then prints a message on stderr
 *       and flushes stderr.  Used to print messages when past certain points
 *       in code so we can tell where we are.  Outl may be invoked like
 *       printf with up to 7 arguments.
 */
outl(msg, arg0,arg1,arg2,arg3,arg4,arg5,arg6)
     char *msg;
     char *arg0, *arg1, *arg2, *arg3, *arg4, *arg5, *arg6;
{
	fflush(stdout);
	fprintf(stderr, msg, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
	fprintf(stderr, "\n");
	fflush(stderr);
}


/*
 * blip: a debugging routine.  Prints Blip! on stderr with flushing. 
 */
void blip()
{
  outl("blip!");
}


/*
 * Routine to let user select a window using the mouse
 */

Window Select_Window(dpy)
     Display *dpy;
{
  int status;
  Cursor cursor;
  XEvent event;
  Window target_win;

  /* Make the target cursor */
  cursor = XCreateFontCursor(dpy, XC_crosshair);

  /* Grab the pointer using target cursor, letting it room all over */
  status = XGrabPointer(dpy, RootWindow(dpy, screen), True,
			ButtonPressMask, GrabModeAsync,
			GrabModeAsync, None, cursor, CurrentTime);
  if (status != GrabSuccess) Fatal_Error("Can't grab the mouse.");

  /* Let the user select a window... */
  XNextEvent(dpy, &event);
  target_win = event.xbutton.subwindow;  /* Get which window selected */

  XUngrabPointer(dpy, CurrentTime);      /* Done with pointer */

  /* 0 for subwindow means root window */
  if (target_win == 0)
    target_win = RootWindow(dpy, screen);

  return(target_win);
}


/*
 * Window_With_Name: routine to locate a window with a given name on a display.
 *                   If no window with the given name is found, 0 is returned.
 *                   If more than one window has the given name, the first
 *                   one found will be returned.  Only top and its subwindows
 *                   are looked at.  Normally, top should be the RootWindow.
 */
Window Window_With_Name(dpy, top, name)
     Display *dpy;
     Window top;
     char *name;
{
	Window *children, dummy;
	int nchildren, i;
	Window w=0;
	char *window_name;

	if (XFetchName(dpy, top, &window_name) && !strcmp(window_name, name))
	  return(top);

	if (!XQueryTree(dpy, top, &dummy, &dummy, &children, &nchildren))
	  return(0);

	for (i=0; i<nchildren; i++) {
		w = Window_With_Name(dpy, children[i], name);
		if (w)
		  break;
	}
	XFree(children);
	return(w);
}
