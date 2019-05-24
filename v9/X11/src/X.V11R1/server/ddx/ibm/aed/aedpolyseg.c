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
/***********************************************************
		Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $Header: aedpolyseg.c,v 1.1 87/09/13 03:35:14 erik Exp $ */
#include "X.h"
#include "Xprotostr.h"
#include "miscstruct.h"
#include "mistruct.h"
#include "gcstruct.h"
#include "windowstr.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "mfb.h"
#include "rtutils.h"
#include "xaed.h"

/*****************************************************************
 * aedPolySegment
 *
 *    For each segment, draws a line between (x1, y1) and (x2, y2).  The
 *    lines are drawn in the order listed.
 *
 *****************************************************************/


void
aedPolySegment(pDraw, pGC, nseg, pSegs)
    DrawablePtr pDraw;
    GCPtr 	pGC;
    int		nseg;
    xSegment	*pSegs;
{
    int i;
    RegionPtr pRegion;
    int xorg, yorg, nbox;
    BoxPtr pbox;

    TRACE(("aedPolySegment( pDraw = 0x%x, pGC = 0x%x, nseg = %d, pSegs = 0x%x)\n", pDraw, pGC, nseg, pSegs));
    pRegion = ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip;
    nbox = pRegion->numRects;
    pbox = pRegion->rects;
    if( nbox == 0 )
	return;
    
    xorg = ((WindowPtr)pDraw)->absCorner.x;
    yorg = ((WindowPtr)pDraw)->absCorner.y;

    vforce();
    clear(6*nseg);
    if( pGC->fgPixel )
        vikint[ORMERGE] = mergexlate[pGC->alu];
    else
        vikint[ORMERGE] = mergexlate[InverseAlu[pGC->alu]];
    for(i=0; i<nseg; i++)
    {
	vikint[vikoff++] = 5;		/* absolute move */
        vikint[vikoff++] = pSegs->x1 + xorg;
        vikint[vikoff++] = pSegs->y1 + yorg;
	vikint[vikoff++] = 7;		/* absolute line */
        vikint[vikoff++] = pSegs->x2 + xorg;
        vikint[vikoff++] = pSegs->y2 + yorg;
	pSegs++;
    }

    vikint[ORCLIPLX] = pbox->x1;
    vikint[ORCLIPLY] = pbox->y1;
    vikint[ORCLIPHX] = pbox->x2-1;
    vikint[ORCLIPHY] = pbox->y2-1;
    nbox--;
    pbox++;
    
    vforce();

    vikint[VIKCMD] = 2; /* reprocess orders */
    for( i = 0; i < nbox; i++, pbox++ )
    {
	vikint[ORCLIPLX] = pbox->x1;
	vikint[ORCLIPLY] = pbox->y1;
	vikint[ORCLIPHX] = pbox->x2-1;
	vikint[ORCLIPHY] = pbox->y2-1;
	command(ORDATA);
    }
    clear(2);

}
