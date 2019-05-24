#include "copyright.h"

/* $Header: XCursor.c,v 11.11 87/09/08 14:31:05 newman Exp $ */
/* Copyright    Massachusetts Institute of Technology    1987	*/

#include "Xlibint.h"
static XColor foreground = { 0,    0,     0,     0  };  /* black */
static XColor background = { 0, 65535, 65535, 65535 };  /* white */

Cursor XCreateFontCursor(dpy, which)
	Display *dpy;
	unsigned int which;
{
	static Font cfont = 0;
	Cursor result;
	static Display *olddpy = NULL;
	/* 
	 * the cursor font contains the shape glyph followed by the mask
	 * glyph; so character position 0 contains a shape, 1 the mask for 0,
	 * 2 a shape, etc.  <X11/cursorfont.h> contains hash define names
	 * for all of these.
	 */

	if (cfont == 0 || dpy != olddpy)	{
		if (cfont && dpy) XUnloadFont (olddpy, cfont);
		cfont = XLoadFont(dpy, CURSORFONT);
		olddpy = dpy;
		if (!cfont) return (Cursor) 0;
	}
	result = XCreateGlyphCursor 
	       (dpy, cfont, cfont, which, which + 1, &foreground, &background);
	return(result);
}

