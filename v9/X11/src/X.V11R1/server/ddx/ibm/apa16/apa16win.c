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

/* $Header: apa16win.c,v 5.2 87/09/13 03:21:22 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/apa16/RCS/apa16win.c,v $ */

#ifndef lint
static char *rcsid = "$Header: apa16win.c,v 5.2 87/09/13 03:21:22 erik Exp $";
#endif


#include "X.h"
#include "Xmd.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "mfb.h"
#include "mistruct.h"
#include "regionstr.h"

#include <machinecons/xio.h>

#include "rtutils.h"

#include "apa16decls.h"
#include "apa16hdwr.h"

extern WindowRec WindowTable[];

Bool apa16CreateWindow(pWin)
WindowPtr pWin;
{
    mfbPrivWin *pPrivWin;

    TRACE(("apa16CreateWindow( pWin= 0x%x )\n",pWin));

    pWin->ClearToBackground = miClearToBackground;
    pWin->PaintWindowBackground = mfbPaintWindowNone;
    pWin->PaintWindowBorder = mfbPaintWindowPR;
    pWin->CopyWindow = apa16CopyWindow;
    pPrivWin = (mfbPrivWin *)Xalloc(sizeof(mfbPrivWin));
    pWin->devPrivate = (pointer)pPrivWin;
    pPrivWin->pRotatedBorder = NullPixmap;
    pPrivWin->pRotatedBackground = NullPixmap;
    pPrivWin->fastBackground = 0;
    pPrivWin->fastBorder = 0;

    /* backing store stuff 
       is this ever called with backing store turned on ???
    */
    if ((pWin->backingStore == WhenMapped) ||
	(pWin->backingStore == Always))
    {
    }
    else
    {
    }
    return TRUE;
}

/* UNCLEAN!
   this code calls the bitblt helper code directly.

   mfbCopyWindow copies only the parts of the destination that are
visible in the source.
*/


void 
apa16CopyWindow(pWin, ptOldOrg, prgnSrc)
    WindowPtr pWin;
    DDXPointRec ptOldOrg;
    RegionPtr prgnSrc;
{
    DDXPointPtr pptSrc;
    register DDXPointPtr ppt;
    RegionPtr prgnDst;
    register BoxPtr pbox;
    register int dx, dy;
    register int i, nbox;
    WindowPtr pwinRoot;

    TRACE(("apa16CopyWindow(pWin= 0x%x, ptOldOrg= 0x%x, prgnSrc= 0x%x)\n",
							pWin,ptOldOrg,prgnSrc));

    pwinRoot = &WindowTable[pWin->drawable.pScreen->myNum];

    prgnDst = (* pWin->drawable.pScreen->RegionCreate)(NULL, 
						       pWin->borderClip->numRects);

    dx = ptOldOrg.x - pWin->absCorner.x;
    dy = ptOldOrg.y - pWin->absCorner.y;
    (* pWin->drawable.pScreen->TranslateRegion)(prgnSrc, -dx, -dy);
    (* pWin->drawable.pScreen->Intersect)(prgnDst, pWin->borderClip, prgnSrc);

    pbox = prgnDst->rects;
    nbox = prgnDst->numRects;
    if(!(pptSrc = (DDXPointPtr )ALLOCATE_LOCAL( prgnDst->numRects *
      sizeof(DDXPointRec))))
	return;
    ppt = pptSrc;

    for (i=0; i<nbox; i++, ppt++, pbox++)
    {
	ppt->x = pbox->x1 + dx;
	ppt->y = pbox->y1 + dy;
    }

    apa16DoBitblt(pwinRoot, pwinRoot, GXcopy, prgnDst, pptSrc);
    DEALLOCATE_LOCAL(pptSrc);
    (* pWin->drawable.pScreen->RegionDestroy)(prgnDst);
}



/* swap in correct PaintWindow* routine.  If we can use a fast output
routine (i.e. the pixmap is paddable to 32 bits), also pre-rotate a copy
of it in devPrivate.
*/
Bool
apa16ChangeWindowAttributes(pWin, mask)
    WindowPtr pWin;
    int mask;
{
mfbPrivWin	*pPrivWin;

    TRACE(("apa16ChangeWindowAttributes( pWin= 0x%x, mask= 0x%x )\n",
								pWin,mask));

    pPrivWin = (mfbPrivWin *)(pWin->devPrivate);

    if (mask&CWBackPixel) {
	pWin->PaintWindowBackground = apa16PaintWindowSolid;
	pPrivWin->fastBackground = 0;
	mask&= (~CWBackPixel);
    }
    if (mask&CWBorderPixel) {
	pWin->PaintWindowBorder = apa16PaintWindowSolid;
	pPrivWin->fastBorder = 0;
	mask&= (~CWBorderPixel);
    }
    if (mask) 
       return mfbChangeWindowAttributes(pWin,mask);
}

