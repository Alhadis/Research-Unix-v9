/* $Header: wsimple.h,v 1.1 87/09/11 08:19:42 toddb Exp $ */
/*
 * Just_display.h: This file contains the definitions needed to use the
 *                 functions in just_display.c.  It also declares the global
 *                 variables dpy, screen, and program_name which are needed to
 *                 use just_display.c.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 *
 * Send bugs, etc. to chariot@athena.mit.edu.
 */

    /* Global variables used by routines in just_display.c */

char *program_name = "unknown_program";       /* Name of this program */
Display *dpy;                                 /* The current display */
int screen;                                   /* The current screen */

#define INIT_NAME program_name=argv[0]        /* use this in main to setup
                                                 program_name */

    /* Declaritions for functions in just_display.c */

void Fatal_Error();
char *Malloc();
char *Realloc();
char *Get_Display_Name();
Display *Open_Display();
void Setup_Display_And_Screen();
XFontStruct *Open_Font();
void Beep();
Pixmap ReadBitmapFile();
void WriteBitmapFile();
Window Select_Window_Args();

#define X_USAGE "[host:display]"              /* X arguments handled by
						 Get_Display_Name */
#define SELECT_USAGE "[{-root|-id <id>|-font <font>|-name <name>}]"

/*
 * Just_one_window.h: This file contains the definitions needed to use the
 *                    functions in just_display.c and just_one_window.c.
 *                    It also declares the global variables dpy, screen,
 *                    window, and program_name which are needed to use
 *                    just_display.c and just_one_window.c.
 *
 * Written by Mark Lillibridge.   Last updated 6/16/87
 *
 * Send bugs, etc. to chariot@athena.mit.edu.
 */


    /* All functions in just_display.c are usable from just_one_window */



    /* INIT_NAME changes slightly since we need to save command arguments
       before munging them */

#undef INIT_NAME
#define INIT_NAME _Save_Commands(argc, argv)


    /* Declarations for new functions in just_one_window.c */

void Get_X_Defaults();
void Get_X_Arguments();
void Get_X_Options();
void Resolve_X_Options();
void Resolve_X_Colors();
void Create_Default_Window();
GC Get_Default_GC();


    /* New global variables */

Window wind;                                  /* The current window */
char **_commands;                             /* internal variable */
int _number_of_commands;                      /* internal variable */

     /* The X Options themselves - the variables that store them */

#undef X_USAGE                                /* now what X options handled */
#define X_USAGE "[host:display] [=geom] [-fw] [-rv] [-bw <borderwidth>] [-bd <color>] [-fg <color>] [-bg <color>] [-bf <fontname>] [-tl <title>] [-in <icon name>] [-icon <icon bitmap filename>]"

/*
 * Universe-wide defaults used in absence of a .Xdefault setting,
 * command line argument, or program specific default setting.
 */

#ifndef TITLE_DEFAULT               /* Title of application */
#define TITLE_DEFAULT "No_name"
#endif
#ifndef ICON_NAME_DEFAULT           /* Name to use for the application */
#define ICON_NAME_DEFAULT 0            /* Use title of application */
#endif
#ifndef DEFX_DEFAULT                /* Put window in upper left hand corner */
#define DEFX_DEFAULT 0
#endif
#ifndef DEFY_DEFAULT
#define DEFY_DEFAULT 0
#endif
#ifndef DEFWIDTH_DEFAULT            /* Window size is 300 by 300 */
#define DEFWIDTH_DEFAULT 300
#endif
#ifndef DEFHEIGHT_DEFAULT
#define DEFHEIGHT_DEFAULT 300
#endif
#ifndef BORDER_WIDTH_DEFAULT        /* Border size is 2 */
#define BORDER_WIDTH_DEFAULT 2
#endif
#ifndef REVERSE_DEFAULT             /* Normal video */
#define REVERSE_DEFAULT 0
#endif
#ifndef BORDER_DEFAULT              /* Black border */
#define BORDER_DEFAULT "black"
#endif
#ifndef BACK_COLOR_DEFAULT          /* White background */
#define BACK_COLOR_DEFAULT "white"
#endif
#ifndef FORE_COLOR_DEFAULT          /* Black foreground */
#define FORE_COLOR_DEFAULT "black"
#endif
#ifndef BODY_FONT_DEFAULT           /* Use fixed as the body font */
#define BODY_FONT_DEFAULT "fixed"
#endif

/*
 * The X options:
 */

/* These hold names for the values some options should have */
char *geometry = NULL;                    /*The window geometry the user gave*/
char *border_color = BORDER_DEFAULT;      /* The color for the border */
char *back_color = BACK_COLOR_DEFAULT;    /* The color for the background */
char *fore_color = FORE_COLOR_DEFAULT;    /* The color for the foreground */
char *body_font_name = BODY_FONT_DEFAULT; /*The name of the font for the body*/
char *icon_bitmap_file = NULL;            /* Name of bitmap to use for icon */

/* These hold the actual values of the options */
char *title = TITLE_DEFAULT;              /* Title of application */
char *icon_name = ICON_NAME_DEFAULT;      /* Name of the icon */
int border_width = BORDER_WIDTH_DEFAULT;  /* Width of the border */
int reverse = REVERSE_DEFAULT;            /* Non-zero iff reverse video */
unsigned long border;                     /* Color of the border */
unsigned long background;                 /* Color of the background */
unsigned long foreground;                 /* Color of the foreground */
XFontStruct *body_font;                   /* Data for the body font */
Pixmap icon_pixmap = NULL;                /* The actual pixmap for icon */

/* size_hints holds the size hints we are to give the window manager */
XSizeHints size_hints = { PPosition | PSize |
#ifdef MIN_X_SIZE
			  PMinSize |
#endif
#ifdef MAX_X_SIZE
			  PMaxSize |
#endif
#ifdef RESIZE_X_INC
			  PResizeInc |
#endif
			  0, DEFX_DEFAULT, DEFY_DEFAULT,
			  DEFWIDTH_DEFAULT, DEFHEIGHT_DEFAULT,
#ifdef MIN_X_SIZE
			  MIN_X_SIZE, MIN_Y_SIZE,
#else
			  0, 0,
#endif
#ifdef MAX_X_SIZE
			  MAX_X_SIZE, MAX_Y_SIZE,
#else
			  0, 0,
#endif
#ifdef RESIZE_X_INC
			  RESIZE_X_INC, RESIZE_Y_INC,
#else
			  0, 0,
#endif
			  { 0, 0 }, { 0, 0 } };
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
