/* $Header: Conversion.c,v 1.1 87/09/11 08:16:01 toddb Exp $ */
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

    if (DefaultVisual(dpy, DefaultScreen(dpy))->class == StaticGray)
	return;

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

    if (DefaultVisual(dpy, DefaultScreen(dpy))->class == StaticGray)
	return;

    s = XParseColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)),
			(char *)fromVal.addr, &c);
    if (s == 0) return;
    s = XAllocColor(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), &c);
    if (s == 0) return;
    done(&c, XColor);
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
