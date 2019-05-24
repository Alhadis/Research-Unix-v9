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
/* $Header: mifpolycon.c,v 1.10 87/09/11 07:20:50 toddb Exp $ */
#include "X.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmapstr.h"
#include "mifpoly.h"

/*
 *	Written by Todd Newman; April. 1987.
 *
 *	Fill a convex polygon.  If the given polygon
 *	is not convex, then the result is undefined.
 *	The algorithm is to order the edges from smallest
 *	y to largest by partitioning the array into a left
 *	edge list and a right edge list.  The algorithm used
 *	to traverse each edge is digital differencing analyzer
 *	line algorithm with y as the major axis. There's some funny linear
 *	interpolation involved because of the subpixel postioning.
 *
 *	Given the opportunity to write non-portable code, I would replace
 * 	this with a fixed point extended precision Bresenham line algorithm.
 *	Instead of doubles in the SppPointRecs, I'd use 64 bit fixed point
 *	values.  I'd have to write a fixed point math pack in assembler,
 *	but these things are relatively straight forward. (Alas, not portable.)
 *	
 *	It can be shown that given 16 bit values for x and y, there's no
 *	room to extend the precision and still fit intermediate results in
 *	a 32 bit integer.  It should also be clear that trying to write a
 *	extended precision math pack in C would not produce code that runs
 * 	much faster than your average floating point coprocessor.  Even
 *	floating point emulation should be able to run faster if the assembly
 *	language coding is handled at all well.
 *
 *	"So why not at least use float, instead of double," my friends ask
 *	me.  Well, C is going to cast everything to double as soon as you
 * 	try to do anything with it anyway.  (Check out K&R.)  I'd rather
 *	trade a little space and skip all the superflous type conversions.
 *	(This decision could be revisited if you have an ANSI C compiler.)
 *
 *	So, in sum, I apologize for doing all the wide lines with doubles,
 *	but it seems the best trade off  of complexity, performance, and
 *	time pressure.
 */
void
miFillSppPoly(dst, pgc, count, ptsIn, xTrans, yTrans)
    DrawablePtr 	dst;
    GCPtr		pgc;
    int			count;          /* number of points */
    SppPointPtr 	ptsIn;          /* the points */
    int			xTrans, yTrans;	/* Translate each point by this */
{
    double		xl, xr,		/* x vals of left and right edges */
          		ml,       	/* left edge slope */
          		mr,             /* right edge slope */
          		dy,             /* delta y */
    			i;              /* loop counter */
    int			y,              /* current scanline */
    			j,
    			imin,           /* index of vertex with smallest y */
    			ymin,           /* y-extents of polygon */
    			ymax,
    			*width,
    			*FirstWidth,    /* output buffer */
    	 		*Marked;	/* set if this vertex has been used */
    register int	left, right,	/* indices to first endpoints */
    			nextleft,
                 	nextright;	/* indices to second endpoints */
    DDXPointPtr 	ptsOut,
    			FirstPoint;	/* output buffer */
    double 		ceil();

    if (pgc->miTranslate && (dst->type == DRAWABLE_WINDOW) )
    {
	xTrans += ((WindowPtr)dst)->absCorner.x;
	yTrans += ((WindowPtr)dst)->absCorner.y;
    }

    imin = GetFPolyYBounds(ptsIn, count, &ymin, &ymax);

    y = ymax - ymin + 1;
    ptsOut = FirstPoint = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * y);
    width = FirstWidth = (int *) ALLOCATE_LOCAL(sizeof(int) * y);
    Marked = (int *) ALLOCATE_LOCAL(sizeof(int) * count);

    if(!ptsOut || !width || !Marked || (count < 3) || (y <= 0))
    {
	DEALLOCATE_LOCAL(Marked);
	DEALLOCATE_LOCAL(width);
	DEALLOCATE_LOCAL(ptsOut);
	return;
    }

    for(j = 0; j < count; j++)
	Marked[j] = 0;
    nextleft = nextright = imin;
    Marked[imin] = -1;
    y = (int) ceil(ptsIn[nextleft].y);

    /*
     *  loop through all edges of the polygon
     */
    do
    {
        /* add a left edge if we need to */
        if ((y > ptsIn[nextleft].y || ISEQUAL(y, ptsIn[nextleft].y)) &&
	     Marked[nextleft] != 1)
	{
	    Marked[nextleft]++;
            left = nextleft++;

            /* find the next edge, considering the end conditions */
            if (nextleft >= count)
                nextleft = 0;

            /* now compute the starting point and slope */
	    dy = ptsIn[nextleft].y - ptsIn[left].y;
	    if (dy != 0.0)
	    { 
		ml = (ptsIn[nextleft].x - ptsIn[left].x) / dy;
		dy = y - ptsIn[left].y;
		xl = ptsIn[left].x  + ml * max(dy, 0); 
	    }
        }

        /* add a right edge if we need to */
        if (((y > ptsIn[nextright].y) || ISEQUAL(y, ptsIn[nextright].y))
	     && Marked[nextright] != 1)
	{
	    Marked[nextright]++;
            right = nextright--;

            /* find the next edge, considering the end conditions */
            if (nextright < 0)
                nextright = count - 1;

            /* now compute the starting point and slope */
	    dy = ptsIn[nextright].y - ptsIn[right].y;
	    if (dy != 0.0) 
	    { 
		mr = (ptsIn[nextright].x - ptsIn[right].x) / dy;
		dy = y - ptsIn[right].y; 
		xr = ptsIn[right].x + mr * max(dy, 0);
	    }
        }


        /*
         *  generate scans to fill while we still have
         *  a right edge as well as a left edge.
         */
        i = min(ptsIn[nextleft].y, ptsIn[nextright].y) - y;

	if (i < EPSILON)
	{
	    if(Marked[nextleft] && Marked[nextright])
	    {
	        /* Arrgh, we're trapped! (no more points) 
	         * Out, we've got to get out of here before this decadence saps
	         * our will completely! */
	        break;
	    }
	    continue;
	}
	else
	{
		j = (int) i;
		if(!j)
		    j++;
	}
        while (j > 0) 
        {
            ptsOut->y = (y) + yTrans;

            /* reverse the edges if necessary */
            if (xl < xr) 
            {
                *(width++) = ROUNDTOINT(xr) - ROUNDTOINT(xl);
                (ptsOut++)->x = ROUNDTOINT(xl) + xTrans;
            }
            else 
            {
                *(width++) = ROUNDTOINT(xl) - ROUNDTOINT(xr);
                (ptsOut++)->x = ROUNDTOINT(xr) + xTrans;
            }
            y++;

            /* increment down the edges */
	    xl += ml;
	    xr += mr;
	    j--;
        }
    }  while (y < ymax);

    /* Finally, fill the spans we've collected */
    (*pgc->FillSpans)(dst, pgc, 
		      ptsOut-FirstPoint, FirstPoint, FirstWidth, 1);
    DEALLOCATE_LOCAL(Marked);
    DEALLOCATE_LOCAL(FirstWidth);
    DEALLOCATE_LOCAL(FirstPoint);
}


/* Find the index of the point with the smallest y.also return the
 * smallest and largest y */
static
int
GetFPolyYBounds(pts, n, by, ty)
    register SppPointPtr	pts;
    int 			n;
    int 			*by, *ty;
{
    register SppPointPtr	ptMin;
    double 			ymin, ymax, ceil();
    SppPointPtr			ptsStart = pts;

    ptMin = pts;
    ymin = ymax = (pts++)->y;

    while (--n > 0) {
        if (pts->y < ymin)
	{
            ptMin = pts;
            ymin = pts->y;
        }
	if(pts->y > ymax)
            ymax = pts->y;

        pts++;
    }

    *by = ROUNDTOINT(ymin);
    *ty = (int) ceil(ymax);
    return(ptMin-ptsStart);
}
