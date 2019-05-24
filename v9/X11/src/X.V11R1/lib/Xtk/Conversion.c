#ifndef lint
static char *sccsid = "@(#)Conversion.c	1.11	3/19/87";
#endif lint

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


/* Conversion.c - implementations of resource type conversion procs */

#include	"Xlib.h"
#include	"Xutil.h"
#include	"Xresource.h"
#include	"XrmConvert.h"
#include	"Conversion.h"
#include	"Quarks.h"
#include	"cursorfont.h"
#include	<sys/file.h>
#include	<stdio.h>

#define	done(address, type) \
	{ (*toVal).size = sizeof(type); (*toVal).addr = (caddr_t) address; }

static void CvtXColorToPixel();

static void CvtGeometryToDims();

static void CvtIntToBoolean();
static void CvtIntToFont();
static void CvtIntOrPixelToXColor();
static void CvtIntToPixel();

static void CvtStringToBoolean();
static void CvtStringToXColor();
static void CvtStringToCursor();
static void CvtStringToDisplay();
extern void CvtStringToEventBindings();
static void CvtStringToFile();
static void CvtStringToFont();
static void CvtStringToFontStruct();
static void CvtStringToGeometry();
static void CvtStringToInt();
static void CvtStringToPixel();
static void CvtStringToPixmap();

void _XLowerCase(source, dest)
    register char  *source, *dest;
{
    register char ch;

    for (; (ch = *source) != 0; source++, dest++) {
    	if ('A' <= ch && ch <= 'Z')
	    *dest = ch - 'A' + 'a';
	else
	    *dest = ch;
    }
    *dest = 0;
}


/*ARGSUSED*/
static void CvtIntToBoolean(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
    static int	b;

    b = (int) (*(int *)fromVal.addr != 0);
    done(&b, int);
};


/*ARGSUSED*/
static void CvtStringToBoolean(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
    static int	b;
    XrmQuark	q;
    char	lowerName[1000];

    _XLowerCase((char *) fromVal.addr, lowerName);
    q = XrmAtomToQuark(lowerName);

    if (q == XrmQEtrue)	{ b = 1; done(&b, int); return; }
    if (q == XrmQEon)	{ b = 1; done(&b, int); return; }
    if (q == XrmQEyes)	{ b = 1; done(&b, int); return; }

    if (q == XrmQEfalse)	{ b = 0; done(&b, int); return; }
    if (q == XrmQEoff)	{ b = 0; done(&b, int); return; }
    if (q == XrmQEno)	{ b = 0; done(&b, int); return; }
};


/*ARGSUSED*/
static void CvtIntOrPixelToXColor(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{    
    static XColor	c;

/*
    if (DefaultVisual(dpy, DefaultScreen(dpy))->class == StaticGray)
	return;
*/

    c.pixel = *(int *)fromVal.addr;
    XQueryColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &c);
				/* !!! Need some error checking ... ||| */
    done(&c, XColor);
};


/*ARGSUSED*/
static void CvtStringToXColor(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
    static XColor	c;
    Status		s;

/*
    if (DefaultVisual(dpy, DefaultScreen(dpy))->class == StaticGray)
	return;
*/

    s = XParseColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)),
			(char *)fromVal.addr, &c);
    if (s == 0) return;
    s = XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &c);
    if (s == 0) return;
    done(&c, XColor);
};


/*ARGSUSED*/
static void CvtStringToCursor(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
#define count(array)	(sizeof(array)/sizeof(array[0]))

    static struct _CursorName {
	char		*name;
	unsigned int	shape;
    } cursor_names[] = {
			{"X_cursor",		XC_X_cursor},
			{"arrow",		XC_arrow},
			{"based_arrow_down",	XC_based_arrow_down},
			{"based_arrow_up",	XC_based_arrow_up},
			{"boat",		XC_boat},
			{"bogosity",		XC_bogosity},
			{"bottom_left_corner",	XC_bottom_left_corner},
			{"bottom_right_corner",	XC_bottom_right_corner},
			{"bottom_side",		XC_bottom_side},
			{"bottom_tee",		XC_bottom_tee},
			{"box_spiral",		XC_box_spiral},
			{"center_ptr",		XC_center_ptr},
			{"circle",		XC_circle},
			{"clock",		XC_clock},
			{"coffee_mug",		XC_coffee_mug},
			{"cross",		XC_cross},
			{"cross_reverse",	XC_cross_reverse},
			{"crosshair",		XC_crosshair},
			{"diamond_cross",	XC_diamond_cross},
			{"dot",			XC_dot},
			{"dotbox",		XC_dotbox},
			{"double_arrow",	XC_double_arrow},
			{"draft_large",		XC_draft_large},
			{"draft_small",		XC_draft_small},
			{"draped_box",		XC_draped_box},
			{"exchange",		XC_exchange},
			{"fleur",		XC_fleur},
			{"gobbler",		XC_gobbler},
			{"gumby",		XC_gumby},
			{"hand1",		XC_hand1},
			{"hand2",		XC_hand2},
			{"heart",		XC_heart},
			{"icon",		XC_icon},
			{"iron_cross",		XC_iron_cross},
			{"left_ptr",		XC_left_ptr},
			{"left_side",		XC_left_side},
			{"left_tee",		XC_left_tee},
			{"leftbutton",		XC_leftbutton},
			{"ll_angle",		XC_ll_angle},
			{"lr_angle",		XC_lr_angle},
			{"man",			XC_man},
			{"middlebutton",	XC_middlebutton},
			{"mouse",		XC_mouse},
			{"pencil",		XC_pencil},
			{"pirate",		XC_pirate},
			{"plus",		XC_plus},
			{"question_arrow",	XC_question_arrow},
			{"right_ptr",		XC_right_ptr},
			{"right_side",		XC_right_side},
			{"right_tee",		XC_right_tee},
			{"rightbutton",		XC_rightbutton},
			{"rtl_logo",		XC_rtl_logo},
			{"sailboat",		XC_sailboat},
			{"sb_down_arrow",	XC_sb_down_arrow},
			{"sb_h_double_arrow",	XC_sb_h_double_arrow},
			{"sb_left_arrow",	XC_sb_left_arrow},
			{"sb_right_arrow",	XC_sb_right_arrow},
			{"sb_up_arrow",		XC_sb_up_arrow},
			{"sb_v_double_arrow",	XC_sb_v_double_arrow},
			{"shuttle",		XC_shuttle},
			{"sizing",		XC_sizing},
			{"spider",		XC_spider},
			{"spraycan",		XC_spraycan},
			{"star",		XC_star},
			{"target",		XC_target},
			{"tcross",		XC_tcross},
			{"top_left_arrow",	XC_top_left_arrow},
			{"top_left_corner",	XC_top_left_corner},
			{"top_right_corner",	XC_top_right_corner},
			{"top_side",		XC_top_side},
			{"top_tee",		XC_top_tee},
			{"trek",		XC_trek},
			{"ul_angle",		XC_ul_angle},
			{"umbrella",		XC_umbrella},
			{"ur_angle",		XC_ur_angle},
			{"watch",		XC_watch},
			{"xterm",		XC_xterm},
    };
    struct _CursorName *cache;
    Cursor cursor;
    char *name = (char *)fromVal.addr;
    int i, found = False;

    /* linear search, for now; can improve on this later */
    for( i=0,cache=cursor_names; i < count(cursor_names); i++,cache++ ) {
	if (strcmp(name, cache->name) == 0) {
	    found = True;
	    break;
	}
    }

    if (found) {
        /* cacheing is actually done by higher layers of Xrm */
	cursor = XCreateFontCursor( dpy, cache->shape );
	done(&cursor, Cursor);
    }
};


/*ARGSUSED*/
static void CvtGeometryToDims(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
    done(&((Geometry *)fromVal.addr)->dims, Dims);
};


/*ARGSUSED*/
static void CvtStringToDisplay(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
    static Display	*d;

    d = XOpenDisplay((char *)fromVal.addr);
    if (d != NULL) { done(d, Display); }
};


/*ARGSUSED*/
static void CvtStringToFile(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
    static FILE	*f;

    f = fopen((char *)fromVal.addr, "r");
    if (f != NULL) { done(f, FILE); }
};


/*ARGSUSED*/
static void CvtStringToFont(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
    static Font	f;

    f = XLoadFont(dpy, (char *)fromVal.addr);
    if (f != 0) { done(&f, Font); }
}


/*ARGSUSED*/
static void CvtIntToFont(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
    done(fromVal.addr, int);
};


/*ARGSUSED*/
static void CvtStringToFontStruct(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
    static XFontStruct	*f;

    f = XLoadQueryFont(dpy, (char *)fromVal.addr);
    if (f != NULL) { done(&f, XFontStruct *); }
}

/*ARGSUSED*/
static void CvtStringToGeometry(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
    static Geometry	g;
    int			i;

    g.pos.xpos = g.pos.ypos = g.dims.width = g.dims.height = 0;
    i = XParseGeometry((char *) fromVal.addr,
	    &g.pos.xpos, &g.pos.ypos, &g.dims.width, &g.dims.height);
    if (i == NoValue) return;
    if (i & XNegative)
	g.pos.xpos = DisplayWidth(dpy, DefaultScreen(dpy))-1-g.pos.xpos;
    if (i & YNegative)
	g.pos.ypos = DisplayHeight(dpy, DefaultScreen(dpy))-1-g.pos.ypos;
    done(&g, Geometry);
};


/*ARGSUSED*/
static void CvtStringToInt(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
    static int	i;

    if (sscanf((char *)fromVal.addr, "%d", &i) == 1) { done(&i, int); }
}


/*ARGSUSED*/
static void CvtStringToPixel(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
    _XrmConvert(dpy, XrmQString, fromVal, XrmQColor, toVal);
    if ((*toVal).addr == NULL) return;
    done(&((XColor *)((*toVal).addr))->pixel, int)
};


/*ARGSUSED*/
static void CvtXColorToPixel(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
    done(&((XColor *)fromVal.addr)->pixel, int);
};


/*ARGSUSED*/
static void CvtIntToPixel(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
	done(fromVal.addr, int);
};


/*ARGSUSED*/
static void CvtStringToPixmap(dpy, fromVal, toVal)
    Display	*dpy;
    XrmValue	fromVal;
    XrmValue	*toVal;
{
    XrmValue	pixelVal;

    _XrmConvert(dpy, XrmQString, fromVal, XrmQPixel, &pixelVal);
    if (pixelVal.addr == NULL) return;
    _XrmConvert(dpy, XrmQPixel, pixelVal, XrmQPixmap, toVal);
}


static Bool initialized = False;

void XrmInitialize()
{
    if (initialized)
    	return;
    initialized = True;

    _XrmRegisterTypeConverter(XrmQColor, XrmQPixel,	CvtXColorToPixel);

    _XrmRegisterTypeConverter(XrmQGeometry, XrmQDims,	CvtGeometryToDims);

    _XrmRegisterTypeConverter(XrmQInt, 	XrmQBoolean,	CvtIntToBoolean);
    _XrmRegisterTypeConverter(XrmQInt, 	XrmQPixel,	CvtIntToPixel);
    _XrmRegisterTypeConverter(XrmQInt, 	XrmQFont,	CvtIntToFont);
    _XrmRegisterTypeConverter(XrmQInt, 	XrmQColor,	CvtIntOrPixelToXColor);

    _XrmRegisterTypeConverter(XrmQString, XrmQBoolean,	CvtStringToBoolean);
    _XrmRegisterTypeConverter(XrmQString, XrmQColor,	CvtStringToXColor);
    _XrmRegisterTypeConverter(XrmQString, XrmQCursor,	CvtStringToCursor);
    _XrmRegisterTypeConverter(XrmQString, XrmQDisplay,	CvtStringToDisplay);
    _XrmRegisterTypeConverter(XrmQString, XrmQFile,	CvtStringToFile);
    _XrmRegisterTypeConverter(XrmQString, XrmQFont,	CvtStringToFont);
    _XrmRegisterTypeConverter(XrmQString, XrmQFontStruct,	CvtStringToFontStruct);
    _XrmRegisterTypeConverter(XrmQString, XrmQGeometry,	CvtStringToGeometry);
    _XrmRegisterTypeConverter(XrmQString, XrmQInt,	CvtStringToInt);
    _XrmRegisterTypeConverter(XrmQString, XrmQPixel,	CvtStringToPixel);
    _XrmRegisterTypeConverter(XrmQString, XrmQPixmap,	CvtStringToPixmap);

    _XrmRegisterTypeConverter(XrmQPixel, XrmQColor,	CvtIntOrPixelToXColor);

}
