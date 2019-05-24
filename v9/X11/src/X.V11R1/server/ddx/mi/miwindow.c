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
/* $Header: miwindow.c,v 1.14 87/09/11 07:20:06 toddb Exp $ */
#include "X.h"
#include "miscstruct.h"
#include "region.h"
#include "mi.h"
#include "windowstr.h"
#include "scrnintstr.h"
#include "pixmapstr.h"

/* 
 * miwindow.c : machine independent window routines 
 *  miClearToBackground
 *  miPaintWindow
 *
 *  author: drewry
 *          Dec 1986
 */


void 
miClearToBackground(pWin, x, y, w, h, generateExposures)
    WindowPtr pWin;
    short x,y;
    unsigned short w,h;
    Bool generateExposures;
{
    BoxRec box;
    RegionPtr pReg;

    if ((pWin->backgroundTile == (PixmapPtr)None) ||
	(pWin->class == InputOnly))
        return ;
    box.x1 = pWin->absCorner.x + x;
    box.y1 = pWin->absCorner.y + y;
    if (w)
        box.x2 = box.x1 + w;
    else
        box.x2 = box.x1 + pWin->clientWinSize.width - x;
    if (h)
        box.y2 = box.y1 + h;	
    else
        box.y2 = box.y1 + pWin->clientWinSize.height - y;

    pReg = (* pWin->drawable.pScreen->RegionCreate)(&box, 1);
    if (generateExposures)
    {
        (* pWin->drawable.pScreen->Intersect)(pWin->exposed, pReg, pWin->clipList);
        HandleExposures(pWin);
    }
    else
    {
        (* pWin->drawable.pScreen->Intersect)(pReg, pReg, pWin->clipList);
        (*pWin->PaintWindowBackground)(pWin, pReg, PW_BACKGROUND);
    }
    (* pWin->drawable.pScreen->RegionDestroy)(pReg);
}


