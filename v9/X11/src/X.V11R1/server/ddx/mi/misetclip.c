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
/* $Header: misetclip.c,v 1.4 87/09/11 07:20:53 toddb Exp $ */
/* Author: Todd Newman */

#include "X.h"
#include "Xprotostr.h"
#include "miscstruct.h"
#include "scrnintstr.h"
#include "pixmap.h"
#include "regionstr.h"
#include "gcstruct.h"

/* The file name is something of a misnomer. Actually, this just contains 
 * a routine to convert a set of rectangles into a region 
 */

typedef struct _ybucket
{
    short 	y;
    short	count;
    BoxPtr	boxes;
    short	miny2;
    short	fdiff;	/* do any have different y2s */
    struct _ybucket	*next;
} YBUCKET;


/* NEWBUCKET -- a private helper to build a bucket for sorting */
static
YBUCKET *
newbucket(y, miny2)
{
    YBUCKET 	*pb;

    pb = (YBUCKET *) Xalloc( sizeof(YBUCKET));
    pb->count = 0;
    pb->y = y;
    pb->miny2 = miny2;
    pb->fdiff = FALSE;
    pb->boxes = (BoxPtr)Xalloc(sizeof(BoxRec));
    pb->next = (YBUCKET *) NULL;
    return(pb);
}

/* MIRECTSTOREGION -- helper called by the device private routine called when
 * the clipmask changes.  Converts the set of rectangles to a region.  
 * dix/gc/SetClipRects has already checked that the rectangles are sorted as
 * advertised.
 * We just take those rectangles and convert to a YX banded region.
 * First we sort the rectangles by starting Y values. Then we make sure that
 * all rectangles that start at a given line all end at the same line.  Bits
 * that hang over fall into the next Y bucket. (Sort of Procrustean, isn't
 * it?)  Next, we sort within each Y bucket by X starting values. Finally,
 * we take the resulting set of rectangles and stuff it into a region.
 */
RegionPtr
miRectsToRegion(pGC, nrects, prect, ordering)
    GCPtr	pGC;
    int		nrects;
    xRectangle	*prect;
    int		ordering;
{
    register YBUCKET	*pb;
    register BoxRec	*pbox, *pboxNew;
    BoxRec              tbox;
    RegionPtr		pReg;
    YBUCKET		*pbFirst, *pbNew, *newbucket();
    int			count, miny2;
    Bool 		fInserted, fChanged;
    int			xMin, xMax, yMin, yMax;
    BoxRec 		b;

    /* if there's only one rectangle, we don't care about ordering */
    if (nrects == 1)   
    {
        b.x1 = prect->x;
        b.y1 = prect->y;
        b.x2 = b.x1 + prect->width;
        b.y2 = b.y1 + prect->height;
        return ((* pGC->pScreen->RegionCreate)(&b, 1));
    }
    /* setting the number of rectangles to zero stops all output */
    if (nrects == 0)   
    {
        b.x1 = 0;
        b.y1 = 0;
        b.x2 = 0;
        b.y2 = 0;
        return ((* pGC->pScreen->RegionCreate)(&b, 1));
    }

    xMin = yMin = MAXSHORT;
    xMax = yMax = -1;
    if ((nrects < 0) ||
	((pReg = (* pGC->pScreen->RegionCreate)(NULL, 1)) == NullRegion))
    {
	return NullRegion;
    }

    if (ordering == YXBanded)
    {
	pReg->rects = (BoxPtr)Xrealloc(pReg->rects, nrects * sizeof(BoxRec));
	pbox = pReg->rects;
	while(pbox < pReg->rects + nrects)
	{
	    pbox->x1 = prect->x;
	    pbox->y1 = prect->y;
	    pbox->x2 = prect->x + prect->width;
	    pbox->y2 = prect->y + prect->height;
	    xMin = min(xMin, pbox->x1);
	    xMax = max(xMax, pbox->x2);
	    yMin = min(yMin, pbox->y1);
	    yMax = max(yMax, pbox->y2);
	    prect++;
	    pbox++;
	}
	pReg->numRects = nrects;
	pReg->size = nrects;
	pReg->extents.x1 = xMin;
	pReg->extents.y1 = yMin;
	pReg->extents.x2 = xMax;
	pReg->extents.y2 = yMax;
	return (pReg);
    }

    pbFirst = (YBUCKET *) newbucket(prect->y, prect->y + prect->height);
    pb = pbFirst;
    count = nrects;

    /* Sort into Y buckets -- everthing in a bucket has the same starting
     * scanline.  Also note the smallest box in the band and whether any
     * boxes are of other sizes */
    while(count--)
    {
	if(ordering == Unsorted)
	{
	    /* We have to search from the top, otherwise we know no
	     * box will be before the current one */
	    pb = pbFirst;
	}

	fInserted = FALSE;
	while(!fInserted)
	{
	    if(prect->y == pb->y)
	    {
		/* add rectangle at the end of this bucket */
		pb->count++;
		pb->boxes = (BoxPtr)Xrealloc(pb->boxes,
		                             pb->count * sizeof(BoxRec));
		pbox = &pb->boxes[pb->count - 1];

		pbox->x1 = prect->x;
		pbox->y1 = prect->y;
		pbox->x2 = prect->x + prect->width;
		pbox->y2 = prect->y + prect->height;
	        xMin = min(xMin, pbox->x1);
	        xMax = max(xMax, pbox->x2);
	        yMin = min(yMin, pbox->y1);
	        yMax = max(yMax, pbox->y2);

		if (pbox->y2 != pb->miny2)
		    pb->fdiff = TRUE;
		if (pbox->y2 < pb->miny2)
		    pb->miny2 = pbox->y2;
		fInserted = TRUE;
	    }
	    else if(prect->y < pbFirst->y)
	    {
		/* Create a new first record in the list */
		pbNew = newbucket(prect->y, prect->y + prect->height);
		pbNew->next = pbFirst;
		pbFirst = pbNew;
		pb = pbNew;
	    }
	    else if ((pb->next == (YBUCKET *) NULL) ||
	             (pb->next->y > prect->y))
	    {
		/* create a new ybucket linked between this and the next.
		 * set pb to new bucket */
		 pbNew = newbucket(prect->y, prect->y + prect->height);
		 pbNew->next = pb->next;
		 pb->next = pbNew;
		 pb = pbNew;
		
	    }
	    else
	    {
		/* try with next bucket */
		pb = pb->next;
	    }
	}
	prect++;
    }

    /* YBand the buckets */
    pb = pbFirst;
    while(pb)
    {
	if(pb->fdiff)
	{
	    miny2 = pb->miny2;

	    /* Figure out which ybucket the lopped-off part of these boxes
	     * will go in */
	    if(pb->next ? (miny2 < pb->next->y) : TRUE)
	    {
		/* create new y bucket */
		pbNew = newbucket(miny2, MAXSHORT);
		pbNew->next = pb->next;
		pb->next = pbNew;
	    }
	    else
	    {
		pbNew = pb->next;
	    }

	    /* if any rectangle is longer than the shortest in this ybucket,
	     * drop the rest of the rectangle into the next lowest bucket */
	    pbox = pb->boxes;
	    while(pbox < pb->boxes + pb->count)
	    {
		if(pbox->y2 > miny2)
		{
		    if(pbNew->count == 0)
		    {
			pbNew->miny2 = pbox->y2;
		    }
		    pbNew->count++;
		    pbNew->boxes = (BoxPtr)Xrealloc(pbNew->boxes, pbNew->count *
					    sizeof(BoxRec));
		    pboxNew = &pbNew->boxes[pbNew->count - 1];
		    pboxNew->x1 = pbox->x1;
		    pboxNew->y1 = pbNew->y;
		    pboxNew->x2 = pbox->x2;
		    pboxNew->y2 = pbox->y2;
		    if(pboxNew->y2 !=pbNew->miny2)
			pbNew->fdiff = TRUE;
		    if(pboxNew->y2 < pbNew->miny2)
			pbNew->miny2 = pboxNew->y2;
		    
		}
		pbox++;
	    }
	}
	else
	{
	   pbNew = pb->next;
	}

	pb = pbNew;
    }

    /* X sort the buckets, and tally total size */
    pb = pbFirst;
    count = 0;
    while(pb)
    {
	count += pb->count;
	/* Yes, it's a bubble sort.  I wanted something (a) in place,
	 * (b) with a low startup cost, because I expect to find few
	 * rectangles per yband and don't want to spend more effort setting
	 * up the sort than I do performing it.
	 */
	fChanged = TRUE;
	while(fChanged)
	{
	    fChanged = FALSE;
	    pbox = pb->boxes;
	    while(pbox < pb->boxes + pb->count - 1)
	    {
		if(pbox->x1 > (pbox+1)->x1)
		{
		    tbox = *pbox;
		    *pbox = *(pbox + 1);
		    *(pbox + 1) = tbox;
		    fChanged = TRUE;
		}
		pbox++;
	    }
	}
	pb = pb->next;
    }

    /* copy the rectangles into the region and free up the buckets */
    pReg->rects = (BoxPtr) Xrealloc (pReg->rects, count * sizeof(BoxRec));
    pReg->numRects = count;
    pReg->size = count;
    pboxNew = pReg->rects;
    pb = pbFirst;
    while(pb)
    {
	for(pbox = pb->boxes; pbox < pb->boxes + pb->count; pbox++)
	{
	    *pboxNew++ = *pbox;
	}
	Xfree((char *)pb->boxes);

	pb = pb->next;
	Xfree(pb);
    }
    pReg->extents.x1 = xMin;
    pReg->extents.y1 = yMin;
    pReg->extents.x2 = xMax;
    pReg->extents.y2 = yMax;
    return(pReg);

}

