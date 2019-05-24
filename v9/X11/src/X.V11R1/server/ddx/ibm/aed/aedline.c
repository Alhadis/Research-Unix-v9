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
#include "X.h"
#include "gc.h"
#include "gcstruct.h"
#include "regionstr.h"
#include "pixmapstr.h"
#include "windowstr.h"
#include "mistruct.h"
#include "mfb.h"
#include "xaed.h"

void
aedSolidLine(dst, pGC, mode, n, pPoint)
    DrawablePtr dst;
    GCPtr pGC;
    int mode;
    int n;
    DDXPointPtr pPoint;
{
    int cmd, i;
    DDXPointPtr ppt;
    RegionPtr pRegion;
    int nbox;
    BoxPtr pbox;
    int xorg, yorg, nptTmp;
    
    pRegion = ((mfbPrivGC *)(pGC->devPriv))->pCompositeClip;
    nbox = pRegion->numRects;
    pbox = pRegion->rects;
    if( nbox == 0 )
	return;
    
    xorg = ((WindowPtr)dst)->absCorner.x;
    yorg = ((WindowPtr)dst)->absCorner.y;

    /* translate the point list */
    ppt = pPoint;
    nptTmp = n;
    if (mode == CoordModeOrigin)
    {
	while(nptTmp--)
	{
	    ppt->x += xorg;
	    ppt++->y += yorg;
	}
    }
    else
    {
	ppt->x += xorg;
	ppt->y += yorg;
/*
	nptTmp--;
	while(nptTmp--)
	{
	    ppt++;
	    ppt->x += (ppt-1)->x;
	    ppt->y += (ppt-1)->y;
	}
*/
    }

    vforce();
    clear(3*n);
    if( mode == CoordModeOrigin )
	cmd = 7;
    else
	cmd = 8;
    if( pGC->fgPixel )
        vikint[ORMERGE] = mergexlate[pGC->alu];
    else
        vikint[ORMERGE] = mergexlate[InverseAlu[pGC->alu]];
    vikint[ORXPOSN] = pPoint->x;
    vikint[ORYPOSN] = pPoint->y;
    ppt=pPoint+1;
    for(i=0; i<(n-1); i++)
    {
	vikint[vikoff++] = cmd;
	vikint[vikoff++] = ppt->x;
	vikint[vikoff++] = ppt->y;
	ppt++;
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

