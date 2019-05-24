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
/* $Header: mizerline.c,v 1.3 87/09/11 07:20:38 toddb Exp $ */
#include "X.h"

#include "misc.h"
#include "scrnintstr.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "pixmap.h"

int
miZeroLine(dst, pgc, mode, nptInit, pptInit)
DrawablePtr dst;
GCPtr pgc;
int mode;
int nptInit;		/* number of points in polyline */
DDXPointRec *pptInit;	/* points in the polyline */
{
    int xorg, yorg;
    DDXPointRec *ppt;
    int npt;

    DDXPointRec pt1, pt2;
    int dx, dy;
    int adx, ady;
    int signdx, signdy;
    register int len;

    int du, dv;
    register int e, e1, e2;

    register int x,y;	/* current point on the line */
    int i;		/* traditional name for loop counter */

    ppt = pptInit;
    npt = nptInit;
    if (pgc->miTranslate)
    {
        if (dst->type == DRAWABLE_WINDOW) 
        {
	    xorg = ((WindowPtr)dst)->absCorner.x;
	    yorg = ((WindowPtr)dst)->absCorner.y;
        }
        else 
        {
	    xorg = 0;
	    yorg = 0;
        }

        if (mode == CoordModeOrigin) 
        {
	        for (i = 0; i<npt; i++) 
                {    
	            ppt->x += xorg;
	            ppt++->y += yorg;
	        }
        }
        else 
        {
	    ppt->x += xorg;
	    ppt++->y += yorg;
	    for (i = 1; i<npt; i++) 
            {
	        ppt->x += (ppt-1)->x;
	        ppt->y += (ppt-1)->y;
	        ppt++;
	    }
        }
    }
    else
    {
	if (mode == CoordModePrevious)
        {
	    ppt++;
	    for (i = 1; i<npt; i++) 
            {
	        ppt->x += (ppt-1)->x;
	        ppt->y += (ppt-1)->y;
	        ppt++;
	    }
        }
    }

    ppt = pptInit;
    npt = nptInit;
    while (--npt)
    {

	DDXPointPtr pspan;
	DDXPointPtr pspanInit;
 	int *pwidth;
	int *pwidthInit;
	int width;

	pt1 = *ppt++;
	pt2 = *ppt;
	dx = pt2.x - pt1.x;
	dy = pt2.y - pt1.y;
	adx = abs(dx);
	ady = abs(dy);
	signdx = sign(dx);
	signdy = sign(dy);

	if (adx > ady)
	{
	    du = adx;
	    dv = ady;
	    len = adx;
	}
	else
	{
	    du = ady;
	    dv = adx;
	    len = ady;
	}

	e1 = dv * 2;
	e2 = e1 - 2*du;
	e = e1 - du;

	pspan = (DDXPointPtr)ALLOCATE_LOCAL(sizeof(DDXPointRec) * (ady+1));
	pwidth = (int *)ALLOCATE_LOCAL(sizeof(int) * (ady+1));
	pspanInit = pspan;
	pwidthInit = pwidth;

	x = pt1.x;
	y = pt1.y;
	if (adx > ady)
	{
	    /* X_AXIS */
	    width = 0;
	    *pspan = pt1;
	    while(len--)
	    {
		if (((signdx > 0) && (e < 0)) ||
		    ((signdx <=0) && (e <=0))
		   )
		{
		    e += e1;
		    x+= signdx;
		    width++;
		}
		else
		{
		    /* give this span a width */
		    width++;
		    *pwidth++ = width;

		    /* force the span the right way */
		    if (signdx < 0)
			pspan->x -= (width-1);

		    /* initialize next span */
		    x += signdx;
		    y += signdy;
		    e += e2;

		    width = 0;
		    pspan++;
		    pspan->x = x;
		    pspan->y = y;

		}
	    };
	    /* do the last span */
	    *pwidth++ = width;
	    if (signdx < 0)
		pspan->x -= (width-1);
	}
	else
	{
	    /* Y_AXIS */
	    while(len--)
	    {
		if (((signdx > 0) && (e < 0)) ||
		    ((signdx <=0) && (e <=0))
		   )
		{
		    e +=e1;
		}
		else
		{
		    x += signdx;
		    e += e2;
		}
		y += signdy;
		pspan->x = x;
		pspan++->y = y;
		*pwidth++ = 1;
	    };
	}

	(*pgc->FillSpans)(dst, pgc, pwidth-pwidthInit,
			  pspanInit, pwidthInit, FALSE);
	DEALLOCATE_LOCAL(pspanInit);
	DEALLOCATE_LOCAL(pwidthInit);
    } 

    if ((pgc->capStyle != CapNotLast) &&
	((ppt->x != pptInit->x) ||
	 (ppt->y != pptInit->y)))
    {
	int width = 1;
	(*pgc->FillSpans)(dst, pgc, 1, ppt, &width, TRUE);
    }
} 
