/* Copyright 1987, Massachusetts Institute of Technology */

/*
 * xfd: program to display a font for perusal by the user.
 *
 * Written by Mark Lillibridge
 *
 */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdio.h>

#define BUFFERSIZE 10
#define VERBOSE_LINES 4         /* Number of lines in verbose display */


    /* Global variables */

int space_per_line;             /* How much space to reserve per line */
int line_offset;                /* Where to start writting (base line) */
int number_of_lines=1;          /* number of lines in bottom area display */
int verbose = 0;                /* verbose mode? */
int box_x = 0;                  /* The size of one box in grid */
int box_y = 0;
int x_offset = 0;               /* Point in box to display character from */
int y_offset = 0;
int x_boxes = 0;                /* Current size of window in # of boxes */
int y_boxes = 0;
int bottom = 0;                 /* Size of grid in pixels */
int right = 0;
int first_char = 0;             /* Character # of first character displayed on
				   the grid */
GC body_gc, real_gc;            /* Graphics contexts */
XFontStruct *real_font;         /* The font we are to display */

/* Gray pattern for use as background */
#include "X11/bitmaps/light_gray"

/* Include routines to handle parsing defaults */
#define TITLE_DEFAULT "xfd"     /* Our name... */
#define DEFX_DEFAULT 300        /* Default window pop-up location */
#define DEFY_DEFAULT 300
#define RESIZE_X_INC 1          /* We will specify a resize inc later ... */
#define RESIZE_Y_INC 1
#define MIN_X_SIZE 1            /* Ditto for minimum window size */
#define MIN_Y_SIZE 1

#include "wsimple.h"

/*
 * usage: routine to show usage then exit.
 */
usage()
{
	fprintf(stderr, "%s: usage: %s %s [-v[erbose]] [-gray] [-start <char number>] fontname\n", program_name,
		program_name, X_USAGE);
	exit(1);
}


/*
 * The main program:
 */
main(argc, argv) 

     int argc;
     char **argv;
{
	register int i;
	GC gc;
	XGCValues gc_init;
	XEvent event;
	char *fontname = BODY_FONT_DEFAULT;     /* Display default body font */
	int gray = 0;                           /* use gray background ? */
	char buffer[BUFFERSIZE];                /* buffer for XLookupString */

	INIT_NAME;

	/* Handle command line arguments, open the display */
	Get_X_Options(&argc, argv);
	for (i = 1; i < argc; i++) {
		if (!strcmp("-", argv[i]))
		  continue;
		if (!strcmp("-gray", argv[i])) {
		  gray = 1;
		  continue;
		}		  
		if (!strcmp("-start", argv[i])) {
			if (++i >= argc) usage();
			first_char = atoi(argv[i]);
			continue;
		}
		if (!strcmp("-verbose", argv[i]) || !strcmp("-v", argv[i])) {
			verbose = 1;
			number_of_lines = VERBOSE_LINES;
			continue;
		}
		if (argv[i][0] == '-')
		  usage();
		fontname = argv[i];
	} 

	/* Load in the font to display */
	real_font = Open_Font(fontname);

	/* Resolve the X options */
	Resolve_X_Options();

	line_offset = 2 + body_font->ascent;
	space_per_line = body_font->descent + line_offset + 2;

	/* Get minimun perfered size */
	Calc_Default_Size();

	/* Create the window */
	Create_Default_Window();
	if (gray)
	  SetBackgroundToBitmap(XCreateBitmapFromData(dpy,
						      wind,
						      light_gray_bits,
						      light_gray_width,
						      light_gray_height),
				light_gray_width, light_gray_height);

	/* Setup graphics contexts */
	body_gc = Get_Default_GC();   /* This one has body font */
	real_gc = Get_Default_GC();   /* This one has font to display */

	gc_init.font = real_font->fid;
	XChangeGC(dpy, real_gc, GCFont, &gc_init);

	/* Start main loop by selecting events then mapping window */
	XSelectInput(dpy, wind, ButtonPressMask|ExposureMask|KeyPressMask);
	XMapWindow(dpy, wind);

	/* Main event loop */
	for (;;) {
		XNextEvent(dpy, &event);
		if (event.type == ButtonPress) {
			if (event.xbutton.button == 1)
			  Go_Back();
			else if (event.xbutton.button == 2)
			  Identify_character(event.xbutton.x, event.xbutton.y);
			else if (event.xbutton.button == 3)
			  Go_Forward();
		} else if (event.type == KeyPress) {
			i = XLookupString(&event, buffer, BUFFERSIZE,
					  NULL, NULL);
			if (i==1 && (buffer[0]=='q' || buffer[0]=='Q' ||
				     buffer[0]==' ' || buffer[0]=='\03'))
			  exit(0);
			if (i==1 && buffer[0]=='<') {
				minimum_bounds();
				continue;
			}
			if (i==1 && buffer[0]=='>') {
				maximum_bounds();
				continue;
			}
			if (i && buffer[0])
			  Beep();
		}
		else if (event.type== GraphicsExpose || event.type == Expose) {
                /* Only redisplay if this is the last exposure in a series */
			if (!event.xexpose.count)
			  Display_Contents();
                }
	}
}

/*
 * Calc_Default_Size: This routine calculates the size of a box in the grid
 * and where to write a character from so that every character will fit
 * in a box.  The size of an ideal window (16 boxes by 16 boxes with room
 * for the bottom text) is then calculated.
 */
Calc_Default_Size()
{
	XCharStruct min_bounds;
	XCharStruct max_bounds;

	min_bounds = real_font->min_bounds;
	max_bounds = real_font->max_bounds;

	/*
	 * Calculate size of box which will hold 1 character as well
	 * as were to draw it from in the box.
	 */
	x_offset = y_offset = 0;
	if (min_bounds.lbearing<0)
	  x_offset = -min_bounds.lbearing;
	if (max_bounds.ascent>0)
	  y_offset = max_bounds.ascent;
	if (real_font->ascent > y_offset)
	  y_offset = real_font->ascent;

	box_x = x_offset;
	box_y = y_offset;

	if (max_bounds.rbearing + x_offset > box_x)
	  box_x = max_bounds.rbearing + x_offset;
	if (x_offset + max_bounds.width > box_x)
	  box_x = x_offset + max_bounds.width;

	if (max_bounds.descent + y_offset > box_y)
	  box_y = max_bounds.descent + y_offset;
	if (real_font->descent + y_offset > box_y)
	  box_y = real_font->descent + y_offset;

	/* Leave room for grid lines & a little space */
	x_offset += 2; y_offset += 2;
	box_x += 3;  box_y += 3;

	if (!geometry) {        /* if user didn't override, use ideal size */
		size_hints.width = box_x*16+1;
		size_hints.height = box_y*16 + space_per_line *number_of_lines;
	}

	size_hints.width_inc = box_x;
	size_hints.height_inc = box_y;

	size_hints.min_width = box_x+1;
	size_hints.min_height = box_y + space_per_line*number_of_lines;
}

char s[4] = { 0, 0, 0, 0 };

/*
 * Display_Contents: Routine to (re)display the contents of the window.
 */
Display_Contents()
{
	int i, x, y;
	XWindowAttributes wind_info;

	/* Get the size of the window */
	if (!XGetWindowAttributes(dpy, wind, &wind_info))
	  Fatal_Error("Can't get window atrributes!");
	size_hints.width = wind_info.width;
	size_hints.height = wind_info.height;

	/* Erase previous contents if any */
	XClearWindow(dpy, wind);

	/* Calculate the size of the grid */
	x_boxes = (size_hints.width-1) / box_x;
	y_boxes = (size_hints.height - space_per_line * number_of_lines)
	  / box_y;
	right = x_boxes * box_x;
	bottom = y_boxes * box_y;

	/* Draw the grid */
	for (i = 0; i<=x_boxes; i++)
	  XDrawLine(dpy, wind, body_gc, i*box_x, 0, i*box_x, bottom);
 	for (i = 0; i<=y_boxes; i++)
	  XDrawLine(dpy, wind, body_gc, 0, i*box_y, right, i*box_y);

	/* Draw one character in every box */
	for (y=0; y<y_boxes; y++) {
		for (x=0; x<x_boxes; x++) {
			s[0] = (first_char + x + y*x_boxes) / 256;
			s[1] = (first_char + x + y*x_boxes) % 256;
			XDrawImageString16(dpy, wind, real_gc, x*box_x+x_offset,
				     y*box_y+y_offset, (XChar2b *) s, 1);
		}
	}
}

char short_format[] = " %d. 0x%x";
char line1_alt[] = " %s bounds:";
char line1_format[] = " character # = %d. (0x%x):";
char line2_format[] = " left bearing = %d, right bearing = %d";
char line3_format[] = " ascent = %d, descent = %d";
char line4_format[] = " width = %d";
char buf[80*2];

/*
 * Identify_character: Routine to print the number of the character that was
 * clicked on by the mouse on the bottom line or beep if no character was
 * clicked on.
 */
Identify_character(x, y)
int x,y;
{
	int xbox, ybox;
	int char_number;
	XCharStruct char_info;
	int index, byte1, byte2;
	char *msg;

	/* If not in grid, beep */
	if (x>=right | y>=bottom) {
		Beep();
		return;
	}

	/* Find out which box clicked in */
	xbox = x / box_x;
	ybox = y / box_y;

	/* Convert that to the character number */
	char_number = first_char + xbox + ybox * x_boxes;

	char_info = real_font->max_bounds;
	index = char_number;
	if (real_font->per_char) {
		if (!real_font->min_byte1 && !real_font->max_byte1) {
			if (char_number < real_font->min_char_or_byte2 ||
			    char_number > real_font->max_char_or_byte2)
			  index = real_font->default_char;
			index -= real_font->min_char_or_byte2;
		} else {
			byte2 = index & 0xff;
			byte1 = (index>>8) & 0xff;
			if (byte1 < real_font->min_byte1 ||
			    byte1 > real_font->max_byte1 ||
			    byte2 < real_font->min_char_or_byte2 ||
			    byte2 > real_font->max_char_or_byte2) {
				    byte2 = real_font->default_char & 0xff;
				    byte1 = (real_font->default_char>>8)&0xff;
			    }
			byte1 -= real_font->min_byte1;
			byte2 -= real_font->min_char_or_byte2;
			index = byte1 * (real_font->max_char_or_byte2 -
					 real_font->min_char_or_byte2 + 1) +
					   byte2;
		}
		char_info = real_font->per_char[index];
	}

	if (!verbose) {
		sprintf(buf, short_format, char_number, char_number);
		put_line(buf, 0);
	} else {
		sprintf(buf, line1_format, char_number, char_number);
		put_line(buf, 0);

		display_char_info(char_info);
	}
}

/*
 * maximum_bounds: display info for maximum bounds
 */
maximum_bounds()
{
	sprintf(buf, line1_alt, "maximum");
	put_line(buf, 0);

	display_char_info(real_font->max_bounds);
}


/*
 * minimum_bounds: display info for minimum bounds
 */
minimum_bounds()
{
	sprintf(buf, line1_alt, "minimum");
	put_line(buf, 0);

	display_char_info(real_font->min_bounds);
}

/*
 * display_char_info: routine to display char info on bottom 3 lines
 */
display_char_info(char_info)
XCharStruct char_info;
{
	sprintf(buf, line2_format, char_info.lbearing,
		char_info.rbearing);
	put_line(buf, 1);
	
	sprintf(buf, line3_format, char_info.ascent,
		char_info.descent);
	put_line(buf, 2);

	sprintf(buf, line4_format, char_info.width);
	put_line(buf, 3);
}

/*
 * Put_line: print a line in bottom area at a given line #
 */
put_line(line, n)
     char *line;
     int n;
{
	strcat(line,"                                                                              ");
	XDrawImageString(dpy, wind, body_gc, 5, bottom + line_offset +
			 space_per_line*n, line, 80);
}


/*
 * Go_Back: Routine to page back a gridful of characters.
 */
Go_Back()
{
	/* If try and page back past first 0th character, beep */
	if (first_char == 0) {
		Beep();
		return;
	}

	first_char -= x_boxes*y_boxes;
	if (first_char<0)
	  first_char = 0;

	Display_Contents();
}

/*
 * Go_Forward: Routine to page forward a gridful of characters.
 */
Go_Forward()
{
	first_char += x_boxes*y_boxes;

	Display_Contents();
}


/*
 * SetBackgroundToBitmap: Set the window's background to a caller supplied 
 *                        bitmap.
 */
SetBackgroundToBitmap(bitmap, width, height)
     Pixmap bitmap;
     int width, height;
{
	Pixmap pix;
	GC gc;
	XGCValues gc_init;

	gc_init.foreground = foreground;
	gc_init.background = background;
	gc = XCreateGC(dpy, RootWindow(dpy, screen),GCForeground|GCBackground,
		       &gc_init);

	pix = Bitmap_To_Pixmap(dpy, wind, gc, bitmap, width, height);

	XSetWindowBackgroundPixmap(dpy, wind, pix);
}
