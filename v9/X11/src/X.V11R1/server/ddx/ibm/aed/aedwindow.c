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

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "mfb.h"
#include "mistruct.h"
#include "regionstr.h"
#include "xaed.h"
#include "rtutils.h"

extern WindowRec WindowTable[];

Bool aedCreateWindow(pWin)
WindowPtr pWin;
{
    mfbPrivWin *pPrivWin;
    TRACE(("aedCreateWindow(pWin= 0x%x)\n", pWin));

    pWin->ClearToBackground = miClearToBackground;
    pWin->PaintWindowBackground = mfbPaintWindowNone;
    pWin->PaintWindowBorder = mfbPaintWindowPR;

    pWin->CopyWindow = aedCopyWindow;
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


/*
   aedCopyWindow copies only the parts of the destination that are
visible in the source.
*/


void 
aedCopyWindow(pWin, ptOldOrg, prgnSrc)
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
    BoxPtr pboxTmp, pboxNext, pboxBase, pboxNew1, pboxNew2;
				/* temporaries for shuffling rectangles */

    TRACE(("aedCopyWindow(pWin= 0x%x, ptOldOrg= 0x%x, prgnSrc= 0x%x)\n", pWin, ptOldOrg, prgnSrc));
    pwinRoot = &WindowTable[pWin->drawable.pScreen->myNum];

    prgnDst = (* pWin->drawable.pScreen->RegionCreate)(NULL, 
				     pWin->borderClip->numRects);

    dx = ptOldOrg.x - pWin->absCorner.x;
    dy = ptOldOrg.y - pWin->absCorner.y;
    (* pWin->drawable.pScreen->TranslateRegion)(prgnSrc, -dx, -dy);
    (* pWin->drawable.pScreen->Intersect)(prgnDst, pWin->borderClip, prgnSrc);


    pbox = prgnDst->rects;
    nbox = prgnDst->numRects;

    pboxNew1 = 0;
    pboxNew2 = 0;
    if ( (pbox->y1 + dy) < pbox->y1) 
    {
        /* walk source bottom to top */

	if (nbox > 1)
	{
	    /* keep ordering in each band, reverse order of bands */
	    pboxNew1 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    if(!pboxNew1)
	    {
	        DEALLOCATE_LOCAL(pboxNew1);
	        return;
	    }
	    pboxBase = pboxNext = pbox+nbox-1;
	    while (pboxBase >= pbox)
	    {
	        while ((pboxNext >= pbox) && 
		       (pboxBase->y1 == pboxNext->y1))
		    pboxNext--;
	        pboxTmp = pboxNext+1;
	        while (pboxTmp <= pboxBase)
	        {
		    *pboxNew1++ = *pboxTmp++;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew1 -= nbox;
	    pbox = pboxNew1;
        }
    }

    if ( (pbox->x1 + dx) < pbox->x1)
    {
	/* walk source right to left */

	if (nbox > 1)
	{
	    /* reverse order of rects in each band */
	    pboxNew2 = (BoxPtr)ALLOCATE_LOCAL(sizeof(BoxRec) * nbox);
	    pboxBase = pboxNext = pbox;
	    if(!pboxNew2)
	    {
	        DEALLOCATE_LOCAL(pboxNew2);
	        return;
	    }
	    while (pboxBase < pbox+nbox)
	    {
	        while ((pboxNext < pbox+nbox) &&
		       (pboxNext->y1 == pboxBase->y1))
		    pboxNext++;
	        pboxTmp = pboxNext;
	        while (pboxTmp != pboxBase)
	        {
		    *pboxNew2++ = *--pboxTmp;
	        }
	        pboxBase = pboxNext;
	    }
	    pboxNew2 -= nbox;
	    pbox = pboxNew2;
	}
    }

/*
    ErrorF("prgnSrc = \n");
    miprintRects(prgnSrc);
    ErrorF("prgnDst = \n");
    miprintRects(prgnDst);
*/

/*
    pbox = prgnDst->rects;
    nbox = prgnDst->numRects;
*/

    vforce();
    vikint[1] = 3;			/* copy area command */
    vikint[8] = mergexlate[GXcopy];	/* merge mode */
    vikint[9] = vikint[10] = 0;		/* area colors */
    for (i=0; i<nbox; i++, pbox++)
    {
	vikint[2] = pbox->x1 + dx;	/* src x */
	vikint[3] = pbox->y1 + dy;	/* src y */
	vikint[4] = pbox->x1;		/* dest x */
	vikint[5] = pbox->y1;		/* dest y */
	vikint[6] = pbox->x2 - pbox->x1;/* area width */
	vikint[7] = pbox->y2 - pbox->y1;/* area height */
	vikwait();
	command(10);
    }
    clear(2);

    /* free up stuff */
    if (pboxNew1)
    {
	DEALLOCATE_LOCAL(pboxNew1);
    }
    if (pboxNew2)
    {
	DEALLOCATE_LOCAL(pboxNew2);
    }

    (* pWin->drawable.pScreen->RegionDestroy)(prgnDst);
}




/* swap in correct PaintWindow* routine.  If we can use a fast output
routine (i.e. the pixmap is paddable to 32 bits), also pre-rotate a copy
of it in devPrivate.
*/
Bool
aedChangeWindowAttributes(pWin, mask)
    WindowPtr pWin;
    int mask;
{
    register int index;
    register mfbPrivWin *pPrivWin;
    TRACE(("aedChangeWindowAttributes(pWin = 0x%x, mask = 0x%x)\n",pWin,mask));
    pPrivWin = (mfbPrivWin *)(pWin->devPrivate);
    while(mask)
    {
	index = ffs(mask) -1;
	mask &= ~(index = 1 << index);
	switch(index)
	{
	  case CWBackingStore:
	    break;

	  case CWBackPixmap:
	      switch((int)pWin->backgroundTile)
	      {
		case None:
		  pWin->PaintWindowBackground = mfbPaintWindowNone;
		  pPrivWin->fastBackground = 0;
		  break;
		case ParentRelative:
		  pWin->PaintWindowBackground = mfbPaintWindowPR;
		  pPrivWin->fastBackground = 0;
		  break;
		default:
		  pPrivWin->fastBackground = 1;
		  pPrivWin->oldRotate.x = pWin->absCorner.x;
		  pPrivWin->oldRotate.y = pWin->absCorner.y;
		  if (pPrivWin->pRotatedBackground)
			  mfbDestroyPixmap(pPrivWin->pRotatedBackground);
		  pPrivWin->pRotatedBackground =
			mfbCopyPixmap(pWin->backgroundTile);
		  aedXRotatePixmap(pPrivWin->pRotatedBackground,
				    pWin->absCorner.x);
		  mfbYRotatePixmap(pPrivWin->pRotatedBackground,
				    pWin->absCorner.y);
		  pWin->PaintWindowBackground = aedPaintWindowTile;
		  break;
	      }
	      break;

	  case CWBackPixel:
              pWin->PaintWindowBackground = aedPaintWindowSolid;
	      pPrivWin->fastBackground = 0;
	      break;

	  case CWBorderPixmap:
	      switch((int)pWin->borderTile)
	      {
		case None:
		  pWin->PaintWindowBorder = mfbPaintWindowNone;
		  pPrivWin->fastBorder = 0;
		  break;
		case ParentRelative:
		  pWin->PaintWindowBorder = mfbPaintWindowPR;
		  pPrivWin->fastBorder = 0;
		  break;
		default:
		  pPrivWin->fastBorder = 1;
		  pPrivWin->oldRotate.x = pWin->absCorner.x;
		  pPrivWin->oldRotate.y = pWin->absCorner.y;
		  if (pPrivWin->pRotatedBorder)
			  mfbDestroyPixmap(pPrivWin->pRotatedBorder);
		  pPrivWin->pRotatedBorder =
			mfbCopyPixmap(pWin->borderTile);
		  aedXRotatePixmap(pPrivWin->pRotatedBorder,
				    pWin->absCorner.x);
		  mfbYRotatePixmap(pPrivWin->pRotatedBorder,
				    pWin->absCorner.y);
		  pWin->PaintWindowBorder = aedPaintWindowTile;
		  break;
	      }
	      break;
	    case CWBorderPixel:
	      pWin->PaintWindowBorder = aedPaintWindowSolid;
	      pPrivWin->fastBorder = 0;
	      break;

	}
    }
    return TRUE;
}

Bool aedPositionWindow(pWin, x, y)
WindowPtr pWin;
int x, y;
{
    mfbPrivWin *pPrivWin;

    pPrivWin = (mfbPrivWin *)(pWin->devPrivate);
    if (((unsigned)(pWin->backgroundTile) > 4 ) &&
	(pPrivWin->fastBackground != 0))
    {
	aedXRotatePixmap(pPrivWin->pRotatedBackground,
		      pWin->absCorner.x - pPrivWin->oldRotate.x);
	mfbYRotatePixmap(pPrivWin->pRotatedBackground,
		      pWin->absCorner.y - pPrivWin->oldRotate.y);
    }

    if (((unsigned)(pWin->borderTile) > 4 ) &&
	(pPrivWin->fastBorder != 0))
    {
	aedXRotatePixmap(pPrivWin->pRotatedBorder,
		      pWin->absCorner.x - pPrivWin->oldRotate.x);
	mfbYRotatePixmap(pPrivWin->pRotatedBorder,
		      pWin->absCorner.y - pPrivWin->oldRotate.y);
    }
    if ( (((unsigned)(pWin->borderTile) > 4) && 
	  (pPrivWin->fastBorder != 0))
	||
	 (((unsigned)(pWin->backgroundTile) > 4) && 
	  (pPrivWin->fastBackground != 0)))
    {
	pPrivWin->oldRotate.x = pWin->absCorner.x;
	pPrivWin->oldRotate.y = pWin->absCorner.y;
    }
}
