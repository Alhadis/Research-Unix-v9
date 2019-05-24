/* $Header: ico.c,v 1.1 87/09/11 08:23:25 toddb Exp $ */
/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/******************************************************************************
 * Description
 *	Display a wire-frame rotating icosahedron, with hidden lines removed
 *
 * Arguments:
 *	-r		display on root window instead of creating a new one
 *	=wxh+x+y	X geometry for new window (default 600x600 centered)
 *	host:display	X display on which to run
 *****************************************************************************/



#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdio.h>



typedef struct
	{
	double x, y, z;
	} Point3D;

typedef double Transform3D[4][4];



extern GC XCreateGC();
extern long time();
extern long random();

Display *dpy;

/******************************************************************************
 * Description
 *	Main routine.  Process command-line arguments, then bounce a bounding
 *	box inside the window.  Call DrawIco() to redraw the icosahedron.
 *****************************************************************************/

main(argc, argv)
int argc;
char **argv;
	{
	char *display = NULL;
	char *geom = NULL;
	int useRoot = 0;
	int fg, bg;
	int invert = 0;
	int dash = 0;

	Window win;
	int winX, winY, winW, winH;
	XSetWindowAttributes xswa;

	GC gc;

	int icoX, icoY;
	int icoDeltaX, icoDeltaY;
	int icoW, icoH;

	XEvent xev;
	XGCValues xgcv;

	/* Process arguments: */

	while (*++argv)
		{
		if (**argv == '=') 
			geom = *argv;
		else if (index(*argv, ':'))
			display = *argv;
		else if (!strcmp(*argv, "-r"))
			useRoot = 1;
		else if (!strcmp (*argv, "-d"))
			dash = atoi(*++argv);
		else if (!strcmp(*argv, "-i"))
			invert = 1;
		}


	if (!(dpy= XOpenDisplay(display)))
	        {
		perror("Cannot open display\n");
		exit(-1);
	        }

	if (invert)
		{
		fg = BlackPixel(dpy, DefaultScreen(dpy));
		bg = WhitePixel(dpy, DefaultScreen(dpy));
		}
	else
		{
		fg = WhitePixel(dpy, DefaultScreen(dpy));
		bg = BlackPixel(dpy, DefaultScreen(dpy));
		}

	/* Set up window parameters, create and map window if necessary: */

	if (useRoot)
		{
		win = DefaultRootWindow(dpy);
		winX = 0;
		winY = 0;
		winW = DisplayWidth(dpy, DefaultScreen(dpy));
		winH = DisplayHeight(dpy, DefaultScreen(dpy));
		}
	else
		{
		winW = 600;
		winH = 600;
		winX = (DisplayWidth(dpy, DefaultScreen(dpy)) - winW) >> 1;
		winY = (DisplayHeight(dpy, DefaultScreen(dpy)) - winH) >> 1;
		if (geom) 
			XParseGeometry(geom, &winX, &winY, &winW, &winH);

		xswa.event_mask = 0;
		xswa.background_pixel = bg;
		xswa.border_pixel = fg;
		win = XCreateWindow(dpy, DefaultRootWindow(dpy), 
		    winX, winY, winW, winH, 0, 
		    DefaultDepth(dpy, DefaultScreen(dpy)), 
		    InputOutput, DefaultVisual(dpy, DefaultScreen(dpy)),
		    CWEventMask | CWBackPixel | CWBorderPixel, &xswa);
		XChangeProperty(dpy, win, XA_WM_NAME, XA_STRING, 8, 
				PropModeReplace, "Ico", 3);
		XMapWindow(dpy, win);
		}


	/* Set up a graphics context: */

	gc = XCreateGC(dpy, win, 0, NULL);
	XSetForeground(dpy, gc, fg);
	XSetBackground(dpy, gc, bg);

	if (dash)
		{
		xgcv.line_style = LineDoubleDash;
		xgcv.dashes = dash;
		XChangeGC(dpy, gc, GCLineStyle | GCDashList, &xgcv);
		}

	/* Get the initial position, size, and speed of the bounding-box: */

	icoW = icoH = 150;
	srandom((int) time(0) % 231);
	icoX = ((winW - icoW) * (random() & 0xFF)) >> 8;
	icoY = ((winH - icoH) * (random() & 0xFF)) >> 8;
	icoDeltaX = 13;
	icoDeltaY = 9;


	/* Bounce the box in the window: */

	for (;;)
		{
		int prevX;
		int prevY;

		if (XPending(dpy))
			XNextEvent(dpy, &xev);

		prevX = icoX;
		prevY = icoY;

		icoX += icoDeltaX;
		if (icoX < 0 || icoX + icoW > winW)
			{
			icoX -= (icoDeltaX << 1);
			icoDeltaX = - icoDeltaX;
			}
		icoY += icoDeltaY;
		if (icoY < 0 || icoY + icoH > winH)
			{
			icoY -= (icoDeltaY << 1);
			icoDeltaY = - icoDeltaY;
			}

		drawIco(win, gc, icoX, icoY, icoW, icoH, prevX, prevY);
		}
	}



/******************************************************************************
 * Description
 *	Undraw previous icosahedron (by erasing its bounding box).
 *	Rotate and draw the new icosahedron.
 *
 * Input
 *	win		window on which to draw
 *	gc		X11 graphics context to be used for drawing
 *	icoX, icoY	position of upper left of bounding-box
 *	icoW, icoH	size of bounding-box
 *	prevX, prevY	position of previous bounding-box
 *****************************************************************************/

drawIco(win, gc, icoX, icoY, icoW, icoH, prevX, prevY)
Window win;
GC gc;
int icoX, icoY, icoW, icoH;
int prevX, prevY;
	{
	static int initialized = 0;
	static Point3D v[] =	/* icosahedron vertices */
		{
		{ 0.00000000,  0.00000000, -0.95105650},
		{ 0.00000000,  0.85065080, -0.42532537},
		{ 0.80901698,  0.26286556, -0.42532537},
		{ 0.50000000, -0.68819095, -0.42532537},
		{-0.50000000, -0.68819095, -0.42532537},
		{-0.80901698,  0.26286556, -0.42532537},
		{ 0.50000000,  0.68819095,  0.42532537},
		{ 0.80901698, -0.26286556,  0.42532537},
		{ 0.00000000, -0.85065080,  0.42532537},
		{-0.80901698, -0.26286556,  0.42532537},
		{-0.50000000,  0.68819095,  0.42532537},
		{ 0.00000000,  0.00000000,  0.95105650}
		};
	static int f[] =	/* icosahedron faces (indices in v) */
		{
		 0,  2,  1,
		 0,  3,  2,
		 0,  4,  3,
		 0,  5,  4,
		 0,  1,  5,
		 1,  6, 10,
		 1,  2,  6,
		 2,  7,  6,
		 2,  3,  7,
		 3,  8,  7,
		 3,  4,  8,
		 4,  9,  8,
		 4,  5,  9,
		 5, 10,  9,
		 5,  1, 10,
		10,  6, 11,
		 6,  7, 11,
		 7,  8, 11,
		 8,  9, 11,
		 9, 10, 11
		};
#	define NV (sizeof(v) / sizeof(v[0]))
#	define NF (sizeof(f) / (3 * sizeof(f[0])))

	static Transform3D xform;
	static Point3D xv[2][NV];
	static int buffer;
	register int p0;
	register int p1;
	register int p2;
	register XPoint *pv2;
	XSegment *pe;
	char drawn[NV][NV];
	register Point3D *pxv;
	static double wo2, ho2;
	XPoint v2[NV];
	XSegment edges[30];
	register int i;
	register int *pf;


	/* Set up points, transforms, etc.:  */

	if (!initialized)	
		{
		Transform3D r1;
		Transform3D r2;

		FormatRotateMat('x', 5 * 3.1416 / 180.0, r1);
		FormatRotateMat('y', 5 * 3.1416 / 180.0, r2);
		ConcatMat(r1, r2, xform);

		bcopy((char *) v, (char *) xv[0], NV * sizeof(Point3D));
		buffer = 0;

		wo2 = icoW / 2.0;
		ho2 = icoH / 2.0;

		initialized = 1;
		}


	/* Switch double-buffer and rotate vertices: */

	buffer = !buffer;
	PartialNonHomTransform(NV, xform, xv[!buffer], xv[buffer]);


	/* Convert 3D coordinates to 2D window coordinates: */

	pxv = xv[buffer];
	pv2 = v2;
	for (i = NV - 1; i >= 0; --i)
		{
		pv2->x = (int) ((pxv->x + 1.0) * wo2) + icoX;
		pv2->y = (int) ((pxv->y + 1.0) * ho2) + icoY;
		++pxv;
		++pv2;
		}


	/* Accumulate edges to be drawn, eliminating duplicates for speed: */

	pxv = xv[buffer];
	pv2 = v2;
	pf = f;
	pe = edges;
	bzero(drawn, sizeof(drawn));
	for (i = NF - 1; i >= 0; --i)
		{
		p0 = *pf++;
		p1 = *pf++;
		p2 = *pf++;

		/* If facet faces away from viewer, don't consider it: */
		if (pxv[p0].z + pxv[p1].z + pxv[p2].z < 0.0)
			continue;

		if (!drawn[p0][p1])
			{
			drawn[p0][p1] = 1;
			drawn[p1][p0] = 1;
			pe->x1 = pv2[p0].x;
			pe->y1 = pv2[p0].y;
			pe->x2 = pv2[p1].x;
			pe->y2 = pv2[p1].y;
			++pe;
			}
		if (!drawn[p1][p2])
			{
			drawn[p1][p2] = 1;
			drawn[p2][p1] = 1;
			pe->x1 = pv2[p1].x;
			pe->y1 = pv2[p1].y;
			pe->x2 = pv2[p2].x;
			pe->y2 = pv2[p2].y;
			++pe;
			}
		if (!drawn[p2][p0])
			{
			drawn[p2][p0] = 1;
			drawn[p0][p2] = 1;
			pe->x1 = pv2[p2].x;
			pe->y1 = pv2[p2].y;
			pe->x2 = pv2[p0].x;
			pe->y2 = pv2[p0].y;
			++pe;
			}
		}


	/* Erase previous, draw current icosahedrons; sync for smoothness. */

	XClearArea(dpy, win, prevX, prevY, icoW + 1, icoH + 1, 0);
	XDrawSegments(dpy, win, gc, edges, pe - edges);
	XSync(dpy, 0);
	}



/******************************************************************************
 * Description
 *	Concatenate two 4-by-4 transformation matrices.
 *
 * Input
 *	l		multiplicand (left operand)
 *	r		multiplier (right operand)
 *
 * Output
 *	*m		Result matrix
 *****************************************************************************/

ConcatMat(l, r, m)
register Transform3D l;
register Transform3D r;
register Transform3D m;
	{
	register int i;
	register int j;
	register int k;

	for (i = 0; i < 4; ++i)
		for (j = 0; j < 4; ++j)
			m[i][j] = l[i][0] * r[0][j]
			    + l[i][1] * r[1][j]
			    + l[i][2] * r[2][j]
			    + l[i][3] * r[3][j];
	}



/******************************************************************************
 * Description
 *	Format a matrix that will perform a rotation transformation
 *	about the specified axis.  The rotation angle is measured
 *	counterclockwise about the specified axis when looking
 *	at the origin from the positive axis.
 *
 * Input
 *	axis		Axis ('x', 'y', 'z') about which to perform rotation
 *	angle		Angle (in radians) of rotation
 *	A		Pointer to rotation matrix
 *
 * Output
 *	*m		Formatted rotation matrix
 *****************************************************************************/

FormatRotateMat(axis, angle, m)
char axis;
double angle;
register Transform3D m;
	{
	double s, c;
	double sin(), cos();

	IdentMat(m);

	s = sin(angle);
	c = cos(angle);

	switch(axis)
		{
		case 'x':
			m[1][1] = m[2][2] = c;
			m[1][2] = s;
			m[2][1] = -s;
			break;
		case 'y':
			m[0][0] = m[2][2] = c;
			m[2][0] = s;
			m[0][2] = -s;
			break;
		case 'z':
			m[0][0] = m[1][1] = c;
			m[0][1] = s;
			m[1][0] = -s;
			break;
		}
	}



/******************************************************************************
 * Description
 *	Format a 4x4 identity matrix.
 *
 * Output
 *	*m		Formatted identity matrix
 *****************************************************************************/

IdentMat(m)
register Transform3D m;
	{
	register int i;
	register int j;

	for (i = 3; i >= 0; --i)
		{
		for (j = 3; j >= 0; --j)
			m[i][j] = 0.0;
		m[i][i] = 1.0;
		}
	}



/******************************************************************************
 * Description
 *	Perform a partial transform on non-homogeneous points.
 *	Given an array of non-homogeneous (3-coordinate) input points,
 *	this routine multiplies them by the 3-by-3 upper left submatrix
 *	of a standard 4-by-4 transform matrix.  The resulting non-homogeneous
 *	points are returned.
 *
 * Input
 *	n		number of points to transform
 *	m		4-by-4 transform matrix
 *	in		array of non-homogeneous input points
 *
 * Output
 *	*out		array of transformed non-homogeneous output points
 *****************************************************************************/

PartialNonHomTransform(n, m, in, out)
int n;
register Transform3D m;
register Point3D *in;
register Point3D *out;
	{
	for (; n > 0; --n, ++in, ++out)
		{
		out->x = in->x * m[0][0] + in->y * m[1][0] + in->z * m[2][0];
		out->y = in->x * m[0][1] + in->y * m[1][1] + in->z * m[2][1];
		out->z = in->x * m[0][2] + in->y * m[1][2] + in->z * m[2][2];
		}
	}
