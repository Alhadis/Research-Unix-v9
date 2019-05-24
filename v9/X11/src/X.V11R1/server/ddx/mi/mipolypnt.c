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
/* $Header: mipolypnt.c,v 1.7 87/09/11 07:20:17 toddb Exp $ */
#include "X.h"
#include "Xprotostr.h"
#include "pixmapstr.h"
#include "gcstruct.h"
#include "windowstr.h"

void
miPolyPoint(pDrawable, pGC, mode, npt, pptInit)
    DrawablePtr 	pDrawable;
    GCPtr 		pGC;
    int 		mode;		/* Origin or Previous */
    int 		npt;
    xPoint 		*pptInit;
{

    int 		xorg;
    int 		yorg;
    int 		nptTmp;
    int			fsOld, fsNew;
    int			*pwidthInit, *pwidth;
    int			i;
    register xPoint 	*ppt;


    if (pDrawable->type == DRAWABLE_WINDOW)
    {
	xorg = ((WindowPtr)pDrawable)->absCorner.x;
	yorg = ((WindowPtr)pDrawable)->absCorner.y;
    }
    else
    {
	xorg = 0;
	yorg = 0;
    }

    /* make pointlist origin relative */
    if (mode == CoordModePrevious)
    {
        ppt = pptInit;
        nptTmp = npt;
	nptTmp--;
	while(nptTmp--)
	{
	    ppt++;
	    ppt->x += (ppt-1)->x;
	    ppt->y += (ppt-1)->y;
	}
    }

    if(pGC->miTranslate)
    {
	ppt = pptInit;
	nptTmp = npt;
	while(nptTmp--)
	{
	    ppt->x += xorg;
	    ppt++->y += yorg;
	}
    }

    fsOld = pGC->fillStyle;
    fsNew = FillSolid;
    if(pGC->fillStyle != FillSolid)
    {
	DoChangeGC(pGC, GCFillStyle, &fsNew, 0);
	ValidateGC(pDrawable, pGC);
    }
    if(!(pwidthInit = (int *)ALLOCATE_LOCAL(npt * sizeof(int))))
	return;
    pwidth = pwidthInit;
    for(i = 0; i < npt; i++)
	*pwidth++ = 1;
    (*pGC->FillSpans)(pDrawable, pGC, npt, pptInit, pwidthInit, FALSE); 

    if(fsOld != FillSolid)
    {
	DoChangeGC(pGC, GCFillStyle, &fsOld, 0);
	ValidateGC(pDrawable, pGC);
    }
    DEALLOCATE_LOCAL(pwidthInit);
}

