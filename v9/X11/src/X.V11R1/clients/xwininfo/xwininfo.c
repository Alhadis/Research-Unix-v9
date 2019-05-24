/* Copyright 1987, Massachusetts Institute of Technology */

/*
 * xwininfo.c	- MIT Project Athena, X Window system window
 *		  information utility.
 *
 *	This program will report all relavent information
 *	about a specific window.
 *
 *  Author:	Mark Lillibridge, MIT Project Athena
 *		16-Jun-87
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>
#include <strings.h>

/* Include routines to handle parsing defaults */
#include "dsimple.h"

#define TRUE 1
#define FALSE 0

#define FAILURE 0

Window window;
static char *window_id_format = " 0x%x";

/*
 * Report the syntax for calling xwininfo:
 */
usage()
{
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage: %s [-help] %s [-int] ",
	    program_name, SELECT_USAGE);
    fprintf(stderr, "[host:vs] [-tree] [-stats] [-bits] [-events] [-size] [-wm]\n\n");
    exit(0);
}


main(argc, argv)
     int argc;
     char **argv;
{
  register int i;
  int tree, stats, bits, events, wm, size  = 0;

  INIT_NAME;

  /* Open display, handle command line arguments */
  Setup_Display_And_Screen(&argc, argv);

  /* Get window selected on command line, if any */
  window = Select_Window_Args(&argc, argv);

  /* Handle our command line arguments */
  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-help"))
      usage();
    if (!strcmp(argv[i], "-int")) {
      window_id_format = " %d";
      continue;
    }
    if (!strcmp(argv[i], "-tree")) {
      tree = 1;
      continue;
    }
    if (!strcmp(argv[i], "-stats")) {
      stats = 1;
      continue;
    }
    if (!strcmp(argv[i], "-bits")) {
      bits = 1;
      continue;
    }
    if (!strcmp(argv[i], "-events")) {
      events = 1;
      continue;
    }
    if (!strcmp(argv[i], "-wm")) {
      wm = 1;
      continue;
    }
    if (!strcmp(argv[i], "-size")) {
      size = 1;
      continue;
    }
    usage();
  }

  /* If no window selected on command line, let user pick one the hard way */
  if (!window) {
	  printf("\nxwininfo ==> Please select the window you wish\n");
	  printf("         ==> information on by clicking the\n");
	  printf("         ==> mouse in that window.\n");
	  window = Select_Window(dpy);
  }

  /*
   * Do the actual displaying as per parameters
   */
  if (!(tree || bits || events || wm || size))
    stats = 1;

  printf("\nxwininfo ==> Window id:");
  Display_Window_Id(window);
  if (tree)
    Display_Tree_Info(window);
  if (stats)
    Display_Stats_Info(window);
  if (bits)
    Display_Bits_Info(window);
  if (events)
    Display_Events_Info(window);
  if (wm)
    Display_WM_Info(window);
  if (size)
    Display_Size_Hints(window);
  printf("\n");
}


/*
 * Lookup: lookup a code in a table.
 */
typedef struct {
	long code;
	char *name;
} binding;

static char _lookup_buffer[100];

char *Lookup(code, table)
long code;
binding *table;
{
	char *name;

	sprintf(_lookup_buffer, "unknown (code = %ld. = 0x%lx)", code, code);
	name = _lookup_buffer;

	while (table->name) {
		if (table->code == code) {
			name = table->name;
			break;
		}
		table++;
	}

	return(name);
}


/*
 * Routine to display a window id in dec/hex with name if window has one
 */

Display_Window_Id(window)
     Window window;
{
  char *win_name;

  printf(window_id_format, window);         /* print id # in hex/dec */
  if (!window) {
	  printf(" (none)\n");
	  return;
  }
  if (!XFetchName(dpy, window, &win_name)) { /* Get window name if any */
	  printf(" (has no name)\n");
	  return;
  }
  if (win_name) {
    printf(" (%s)\n", win_name);
    free(win_name);
  } else if (window == RootWindow(dpy, screen))
    printf(" (the root window)\n");
  else
    printf(" (has no name)\n");
}


/*
 * Display Stats on window
 */
static binding _window_classes[] = {
	{ InputOutput, "InputOutput" },
	{ InputOnly, "InputOnly" },
        { 0, 0 } };

static binding _map_states[] = {
	{ IsUnmapped, "IsUnMapped" },
	{ IsUnviewable, "IsUnviewable" },
	{ IsViewable, "IsViewable" },
	{ 0, 0 } };

Display_Stats_Info(window)
     Window window;
{
  XWindowAttributes win_attributes;

  if (!XGetWindowAttributes(dpy, window, &win_attributes))
    Fatal_Error("Can't get window attributes.");

  printf("\n         ==> Upper left X: %d\n", win_attributes.x);
  printf("         ==> Upper left Y: %d\n", win_attributes.y);
  printf("         ==> Width: %d\n", win_attributes.width);
  printf("         ==> Height: %d\n", win_attributes.height);
  printf("         ==> Depth: %d\n", win_attributes.depth);
  printf("         ==> Border width: %d\n", win_attributes.border_width);
  printf("         ==> Window class: %s\n", Lookup(win_attributes.class,
						   _window_classes));
  printf("         ==> Window Map State: %s\n",
	 Lookup(win_attributes.map_state, _map_states));
}


/*
 * Display bits info:
 */
static binding _gravities[] = {
	{ UnmapGravity, "UnMapGravity" },      /* WARNING: both of these have*/
	{ ForgetGravity, "ForgetGravity" },    /* the same value - see code */
	{ NorthWestGravity, "NorthWestGravity" },
	{ NorthGravity, "NorthGravity" },
	{ NorthEastGravity, "NorthEastGravity" },
	{ WestGravity, "WestGravity" },
	{ CenterGravity, "CenterGravity" },
	{ EastGravity, "EastGravity" },
	{ SouthWestGravity, "SouthWestGravity" },
	{ SouthGravity, "SouthGravity" },
	{ SouthEastGravity, "SouthEastGravity" },
	{ StaticGravity, "StaticGravity" },
	{ 0, 0 } };

static binding _backing_store_hint[] = {
	{ NotUseful, "NotUseful" },
	{ WhenMapped, "WhenMapped" },
	{ Always, "Always" },
	{ 0, 0 } };

static binding _bool[] = {
	{ 0, "No" },
	{ 1, "Yes" },
	{ 0, 0 } };

Display_Bits_Info(window)
     Window window;
{
  XWindowAttributes win_attributes;

  if (!XGetWindowAttributes(dpy, window, &win_attributes))
    Fatal_Error("Can't get window attributes.");

  printf("\n         ==> Bit gravity: %s\n",
	 Lookup(win_attributes.bit_gravity, _gravities+1));
  printf("         ==> Window gravity: %s\n",
	 Lookup(win_attributes.win_gravity, _gravities));
  printf("         ==> Backing-store hint: %s\n",
	 Lookup(win_attributes.backing_store, _backing_store_hint));
  printf("         ==> Backing-planes to be preserved: 0x%x\n",
	 win_attributes.backing_planes);
  printf("         ==> Backing pixel: %d\n", win_attributes.backing_pixel);
  printf("         ==> Save-under?: %s\n",
	 Lookup(win_attributes.save_under, _bool));
}


/*
 * Routine to display all events in an event mask
 */
static binding _event_mask_names[] = {
	{ KeyPressMask, "KeyPress" },
	{ KeyReleaseMask, "KeyRelease" },
	{ ButtonPressMask, "ButtonPress" },
	{ ButtonReleaseMask, "ButtonRelease" },
	{ EnterWindowMask, "EnterWindow" },
	{ LeaveWindowMask, "LeaveWindow" },
	{ PointerMotionMask, "PointerMotion" },
	{ PointerMotionHintMask, "PointerMotionHint" },
	{ Button1MotionMask, "Button1Motion" },
	{ Button2MotionMask, "Button2Motion" },
	{ Button3MotionMask, "Button3Motion" },
	{ Button4MotionMask, "Button4Motion" },
	{ Button5MotionMask, "Button5Motion" },
	{ ButtonMotionMask, "ButtonMotion" },
	{ KeymapStateMask, "KeymapState" },
	{ ExposureMask, "Exposure" },
	{ VisibilityChangeMask, "VisibilityChange" },
	{ StructureNotifyMask, "StructureNotify" },
	{ ResizeRedirectMask, "ResizeRedirect" },
	{ SubstructureNotifyMask, "SubstructureNotify" },
	{ SubstructureRedirectMask, "SubstructureRedirect" },
	{ FocusChangeMask, "FocusChange" },
	{ PropertyChangeMask, "PropertyChange" },
	{ ColormapChangeMask, "ColormapChange" },
	{ OwnerGrabButtonMask, "OwnerGrabButton" },
	{ 0, 0 } };

Display_Event_Mask(mask)
     long mask;
{
  long bit, bit_mask;

  for (bit=0, bit_mask=1; bit<sizeof(long)*8; bit++, bit_mask <<= 1)
    if (mask & bit_mask)
      printf("             ==> %s\n",
	     Lookup(bit_mask, _event_mask_names));
}


/*
 * Display info on events
 */
Display_Events_Info(window)
     Window window;
{
  XWindowAttributes win_attributes;

  if (!XGetWindowAttributes(dpy, window, &win_attributes))
    Fatal_Error("Can't get window attributes.");

  printf("\n         ==> Someone wants these events:\n");
  Display_Event_Mask(win_attributes.all_event_masks);

  printf("         ==> Do not prograte these events:\n");
  Display_Event_Mask(win_attributes.do_not_propagate_mask);

  printf("         ==> Overide redirection?: %s\n",
	 Lookup(win_attributes.override_redirect, _bool));
}


  /* left out visual stuff */
  /* left out colormap */
  /* left out map_installed */


/*
 * Display root, parent, and children window id's of window
 */
Display_Tree_Info(window)
     Window window;
{
  int i;
  Window root_win, parent_win;
  int num_children;
  Window *child_list;
  
  if (!XQueryTree(dpy, window, &root_win, &parent_win, &child_list,
		  &num_children))
    Fatal_Error("Can't query window tree.");

  printf("\n         ==> Root window id:");
  Display_Window_Id(root_win);
  printf("         ==> Parent window id:");
  Display_Window_Id(parent_win);

  printf("         ==> Number of children: %d\n", num_children);

  for (i = num_children - 1; i >= 0; i--) {
    printf("             ==> Child window id:"); 
    Display_Window_Id(child_list[i]);
  }

  free(child_list);
}


/*
 * Display a set of size hints
 */
Display_Hints(hints)
     XSizeHints hints;
{
	long flags;

	flags = hints.flags;
	
	if (flags & USPosition)
	  printf("             ==> User supplied location: %d, %d\n",
		 hints.x, hints.y);

	if (flags & PPosition)
	  printf("             ==> Program supplied location: %d, %d\n",
		 hints.x, hints.y);

	if (flags & USSize)
	  printf("             ==> User supplied size: %d by %d\n",
		 hints.width, hints.height);

	if (flags & PSize)
	  printf("             ==> Program supplied size: %d by %d\n",
		 hints.width, hints.height);

	if (flags & PMinSize)
	  printf("             ==> Program supplied minimum size: %d by %d\n",
		 hints.min_width, hints.min_height);

	if (flags & PMaxSize)
	  printf("             ==> Program supplied maximum size: %d by %d\n",
		 hints.max_width, hints.max_height);

	if (flags & PResizeInc) {
	  printf("             ==> Program supplied x resize increment: %d\n",
		 hints.width_inc);
	  printf("             ==> Program supplied y resize increment: %d\n",
		 hints.height_inc);
        }

	if (flags & PAspect) {
	  printf("             ==> Program supplied min aspect ratio: %d/%d\n",
		 hints.min_aspect.x, hints.min_aspect.y);
	  printf("             ==> Program supplied max aspect ratio: %d/%d\n",
		 hints.max_aspect.x, hints.max_aspect.y);
        }
}


/*
 * Display Size Hints info
 */
Display_Size_Hints(window)
     Window window;
{
	XSizeHints hints;

	if (!XGetNormalHints(dpy, window, &hints))
	  printf("\n         ==> No normal window size hints defined\n");
	else {
		printf("\n         ==> Normal window size hints:\n\n");
		Display_Hints(hints);
	}

	if (!XGetZoomHints(dpy, window, &hints))
	  printf("\n         ==> No zoom window size hints defined\n");
	else {
		printf("\n         ==> Zoom window size hints:\n\n");
		Display_Hints(hints);
	}
}


/*
 * Display Window Manager Info
 */
static binding _state_hints[] = {
	{ DontCareState, "Don't Care State" },
	{ NormalState, "Normal State" },
	{ ZoomState, "Zoomed State" },
	{ IconicState, "Iconic State" },
	{ InactiveState, "Inactive State" },
	{ 0, 0 } };

Display_WM_Info(window)
     Window window;
{
        XWMHints *wmhints;
	long flags;

	wmhints = XGetWMHints(dpy, window);
	if (!wmhints) {
		printf("\n         ==> No window manager hints defined\n");
		return;
	}
	flags = wmhints->flags;

	printf("\n         ==> Window manager hints:\n\n");

	if (flags & InputHint)
	  printf("             ==> Application accepts input?  %s\n",
		 Lookup(wmhints->input, _bool));

	if (flags & IconWindowHint) {
		printf("             ==> Icon window id:");
		Display_Window_Id(wmhints->icon_window);
	}

	if (flags & IconPositionHint)
	  printf("             ==> Initial icon position: %d, %d\n",
		 wmhints->icon_x, wmhints->icon_y);

	if (flags & StateHint)
	  printf("             ==> Initial state is %s\n",
		 Lookup(wmhints->initial_state, _state_hints));
}
