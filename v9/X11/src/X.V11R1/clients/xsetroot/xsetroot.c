/* Copyright 1987, Massachusetts Institute of Technology */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>

/*
 * xsetroot.c 	MIT Project Athena, X Window system root window 
 *		parameter setting utility.  This program will set 
 *		various parameters of the X root window.
 *
 *  Author:	Mark Lillibridge, MIT Project Athena
 *		11-Jun-87
 */

/* Include routines to handle parsing defaults */
#include "wsimple.h"

#include "X11/bitmaps/gray"

usage()
{
	outl("%s: usage: %s [-fg <color>] [-bg <color>] [-rv] [-help] [-def] [-name <string>] [-cursor <cursor file> <mask file>] [-solid <color>] [-gray] [-grey] [-bitmap <filename>] [-mod <x> <y>] [<host>:<display>]\n\nNOTE: *** Use only one of -solid, -gray, -grey, -bitmap, and -mod ***\n\n", program_name, program_name);
	exit(1);
}

extern Pixmap MakeModulaBitmap();

main(argc, argv) 
     int argc;
     char **argv;
{
  int excl = 0;
  int restore_defaults = 0;
  char *name = 0;
  char *cursor_file = 0;
  char *cursor_mask = 0;
  Cursor cursor;
  char *solid_color = 0;
  int gray = 0;
  char *bitmap_file = 0;
  int mod_x = -1;
  int mod_y = -1;
  register int i;
  Pixmap bitmap;
  int ww, hh;

  INIT_NAME;

  /* Handle command line arguments, open the display */
  Get_X_Options(&argc, argv);

  for (i = 1; i < argc; i++) {
    if (!strcmp("-", argv[i]))
      continue;
    if (!strcmp("-help", argv[i]))
      usage();
    if (!strcmp("-def", argv[i])) {
      restore_defaults = 1;
      continue;
    }
    if (!strcmp("-name", argv[i])) {
      if (++i>=argc) usage();
      name = argv[i];
      continue;
    }
    if (!strcmp("-cursor", argv[i])) {
      if (i++>=argc) usage();
      cursor_file = argv[i];
      if (i++>=argc) usage();
      cursor_mask = argv[i];
      continue;
    }
    if (!strcmp("-solid", argv[i])) {
      if (i++>=argc) usage();
      solid_color = argv[i];
      excl++;
      continue;
    }
    if (!strcmp("-gray", argv[i]) || !strcmp("-grey", argv[i])) {
      gray = 1;
      excl++;
      continue;
    }
    if (!strcmp("-bitmap", argv[i])) {
      if (++i>=argc) usage();
      bitmap_file = argv[i];
      excl++;
      continue;
    }
    if (!strcmp("-mod", argv[i])) {
      if (++i>=argc) usage();
      mod_x = atoi(argv[i]);
      if (++i>=argc) usage();
      mod_y = atoi(argv[i]);
      excl++;
      continue;
    }
    usage();
  } 

  /* Check for multiple use of exclusive options */
  if (excl > 1)
    usage();

  /* If there are no arguments then restore defaults. */
  if (argc == 1 || (argc == 2 && !strcmp(argv[1],"-")))
    restore_defaults = 1;

  /* Resolve the X options */
  Resolve_X_Options();

  /* Handle a cursor file */
  if (cursor_file) {
	  cursor = CreateCursorFromFiles(cursor_file, cursor_mask, fore_color,
					 back_color);

	  XDefineCursor(dpy, RootWindow(dpy, screen), cursor);

	  XFreeCursor(dpy, cursor);
  }

  /* Handle -gray and -grey options */
  if (gray)
    SetBackgroundToData(gray_bits, gray_width, gray_height);

  /* Handle -solid option */
  if (solid_color)
    XSetWindowBackground(dpy, RootWindow(dpy, screen),
			 Resolve_Color(RootWindow(dpy, screen),
				       solid_color));

  /* Handle -bitmap option */
  if (bitmap_file) {
	  bitmap = ReadBitmapFile(RootWindow(dpy, screen), bitmap_file,
				  &ww, &hh, 0, 0);
	  SetBackgroundToBitmap(bitmap, ww, hh);
  }

  /* Handle set background to a modula pattern */
  if (mod_x != -1)
    SetBackgroundToBitmap(MakeModulaBitmap(mod_x, mod_y), 16, 16);

  /* Handle set name */
  if (name)
    XStoreName(dpy, RootWindow(dpy, screen), name);

  /* Handle restore defaults */
  if (restore_defaults) {
    if (!cursor_file)
      XUndefineCursor(dpy, RootWindow(dpy, screen)); 
    if (!excl)
      XSetWindowBackgroundPixmap(dpy, RootWindow(dpy, screen), (Pixmap) 0);
  }

  /* Clear the root window, flush all output and exit. */
  XClearWindow(dpy, RootWindow(dpy, screen));
  XCloseDisplay(dpy);
}


/*
 * SetBackgroundToBitmap: Set the root window background to a caller supplied 
 *                        bitmap.
 */
SetBackgroundToBitmap(bitmap, width, height)
     Pixmap bitmap;
     int width, height;
{
	Pixmap pix;
	GC gc;
	XGCValues gc_init;
	long temp;

	gc_init.foreground = Resolve_Color(RootWindow(dpy,screen),fore_color);
	gc_init.background = Resolve_Color(RootWindow(dpy,screen),back_color);
	if (reverse) {
		temp=gc_init.foreground;
		gc_init.foreground=gc_init.background;
		gc_init.background=temp;
	}
	gc = XCreateGC(dpy, RootWindow(dpy, screen),GCForeground|GCBackground,
		       &gc_init);

	pix = Bitmap_To_Pixmap(dpy, RootWindow(dpy, screen), gc, bitmap,
			       width, height);

	XSetWindowBackgroundPixmap(dpy, RootWindow(dpy, screen), pix);
}


/*
 * SetBackgroundToData: As SetBackgroundToBitmap but uses the data for
 *                      a bitmap instead of an actual bitmap.  Data format
 *                      is that used by XCreateBitmapFromData.
 */
SetBackgroundToData(data, width, height)
char *data;
int width, height;
{
	Pixmap bitmap;

	bitmap = XCreateBitmapFromData(dpy, RootWindow(dpy, screen),
				       data, width, height);

	SetBackgroundToBitmap(bitmap, width, height);
}


/*
 * CreateCursorFromFiles: make a cursor of the right colors from two bitmap
 *                        files.
 */
#define BITMAP_HOT_DEFAULT 8

CreateCursorFromFiles(cursor_file, mask_file, fore_color, back_color)
char *cursor_file, *mask_file;
char *fore_color, *back_color;
{
	Pixmap cursor_bitmap, mask_bitmap;
	int width, height, ww, hh, x_hot, y_hot;
	Cursor cursor;
	XColor fg, bg, temp, NameToXColor();

	fg = NameToXColor(fore_color);
	bg = NameToXColor(back_color);
	if (reverse) {
		temp = fg; fg = bg; bg = temp;
	}

	cursor_bitmap = ReadBitmapFile(RootWindow(dpy, screen),
				       cursor_file, &width, &height,
				       &x_hot, &y_hot);
	mask_bitmap = ReadBitmapFile(RootWindow(dpy, screen), mask_file,
				     &ww, &hh, 0, 0);

	if (width != ww || height != hh)
	  Fatal_Error("Dimensions of cursor bitmap and cursor mask bitmap are different!");

	if (x_hot == -1) {
		x_hot = BITMAP_HOT_DEFAULT;
		y_hot = BITMAP_HOT_DEFAULT;
	}

	cursor = XCreatePixmapCursor(dpy, cursor_bitmap, mask_bitmap, &fg, &bg,
				     x_hot, y_hot);

	return(cursor);
}


/*
 * MakeModulaBitmap: Returns a modula bitmap based on an x & y mod.
 */
Pixmap MakeModulaBitmap(mod_x, mod_y)
int mod_x, mod_y;
{
	int i;
	long pattern_line = 0;
	char modula_data[16*16/8];

	if (mod_x<1)
	  mod_x=1;
	if (mod_y<1)
	  mod_y=1;

	for (i=0; i<16; i++) {
		pattern_line <<=1;
		if (i % mod_x == 0) pattern_line |= 0x0001;
	}
	for (i=0; i<16; i++) {
		if (i % mod_y) {
			modula_data[i*2] = 0xff;
			modula_data[i*2+1] = 0xff;
		} else {
			modula_data[i*2] = pattern_line & 0xff;
			modula_data[i*2+1] = (pattern_line>>8) & 0xff;
		}
	}


	return(XCreateBitmapFromData(dpy, RootWindow(dpy, screen), modula_data,
				     16, 16));
}


/*
 * NameToXColor: Convert the name of a color to its Xcolor value.
 */
XColor NameToXColor(name)
char *name;
{
	XColor c;
	Colormap cmap;
	
	cmap = DefaultColormap(dpy, screen);

	if (!XParseColor(dpy, cmap, name, &c))
	  Fatal_Error("unknown color or bad color format: %s", name);

	return(c);
}
