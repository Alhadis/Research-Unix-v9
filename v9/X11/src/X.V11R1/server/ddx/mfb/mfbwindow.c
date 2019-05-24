/* $Header: mfbwindow.c,v 1.2 87/09/03 13:48:44 toddb Exp $ */
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

#include "X.h"
#include "scrnintstr.h"
#include "windowstr.h"
#include "mfb.h"
#include "mistruct.h"
#include "regionstr.h"

extern WindowRec WindowTable[];

Bool mfbCreateWindow(pWin)
register WindowPtr pWin;
{
    register mfbPrivWin *pPrivWin;

    pWin->ClearToBackground = miClearToBackground;
    pWin->PaintWindowBackground = mfbPaintWindowNone;
    pWin->PaintWindowBorder = mfbPaintWindowPR;

    pWin->CopyWindow = mfbCopyWindow;
    if(!(pPrivWin = (mfbPrivWin *)Xalloc(sizeof(mfbPrivWin))))
	 return (FALSE);
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
    return (TRUE);
}

/* This always returns true, because Xfree can't fail.  It might be possible
 * on some devices for Destroy to fail */
Bool 
mfbDestroyWindow(pWin)
WindowPtr pWin;
{
    register mfbPrivWin *pPrivWin;

    pPrivWin = (mfbPrivWin *)(pWin->devPrivate);

    /* mfbDestroyPixmap() deals with any NULL pointers */
    mfbDestroyPixmap(pPrivWin->pRotatedBorder);
    mfbDestroyPixmap(pPrivWin->pRotatedBackground);
    Xfree(pWin->devPrivate);
    return (TRUE);
}

Bool mfbMapWindow(pWindow)
WindowPtr pWindow;
{
    return (TRUE);
}

/* (x, y) is the upper left corner of the window on the screen 
   do we really need to pass this?  (is it a;ready in pWin->absCorner?)
   we only do the rotation for pixmaps that are 32 bits wide (padded
or otherwise.)
   mfbChangeWindowAttributes() has already put a copy of the pixmap
in pPrivWin->pRotated*
*/

Bool 
mfbPositionWindow(pWin, x, y)
register WindowPtr pWin;
int x, y;
{
    register mfbPrivWin *pPrivWin;

    pPrivWin = (mfbPrivWin *)(pWin->devPrivate);
    if (IS_VALID_PIXMAP(pWin->backgroundTile) &&
	(pPrivWin->fastBackground != 0))
    {
	mfbXRotatePixmap(pPrivWin->pRotatedBackground,
			 pWin->absCorner.x - pPrivWin->oldRotate.x);
	mfbYRotatePixmap(pPrivWin->pRotatedBackground,
			 pWin->absCorner.y - pPrivWin->oldRotate.y);
    }

    if (IS_VALID_PIXMAP(pWin->borderTile) &&
	(pPrivWin->fastBorder != 0))
    {
	mfbXRotatePixmap(pPrivWin->pRotatedBorder,
			 pWin->absCorner.x - pPrivWin->oldRotate.x);
	mfbYRotatePixmap(pPrivWin->pRotatedBorder,
			 pWin->absCorner.y - pPrivWin->oldRotate.y);
    }
    if ( (IS_VALID_PIXMAP(pWin->borderTile) &&
	  (pPrivWin->fastBorder != 0))
	||
	 (IS_VALID_PIXMAP(pWin->backgroundTile) &&
	  (pPrivWin->fastBackground != 0)))
    {
	pPrivWin->oldRotate.x = pWin->absCorner.x;
	pPrivWin->oldRotate.y = pWin->absCorner.y;
    }
    /* Again, we have no failure modes indicated by any of the routines
     * we've called, so we have to assume it worked */
    return (TRUE);
}

Bool 
mfbUnmapWindow(pWindow, x, y)
WindowPtr pWindow;
int x, y;
{
    return (TRUE);
}

/* UNCLEAN!
   this code calls the bitblt helper code directly.

   mfbCopyWindow copies only the parts of the destination that are
visible in the source.
*/


void 
mfbCopyWindow(pWin, ptOldOrg, prgnSrc)
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

    mfbDoBitblt(pwinRoot, pwinRoot, GXcopy, prgnDst, pptSrc);
    DEALLOCATE_LOCAL(pptSrc);
    (* pWin->drawable.pScreen->RegionDestroy)(prgnDst);
}



/* swap in correct PaintWindow* routine.  If we can use a fast output
routine (i.e. the pixmap is paddable to 32 bits), also pre-rotate a copy
of it in devPrivate.
*/
Bool
mfbChangeWindowAttributes(pWin, mask)
    register WindowPtr pWin;
    register int mask;
{
    register int index;
    register mfbPrivWin *pPrivWin;

    pPrivWin = (mfbPrivWin *)(pWin->devPrivate);
    while(mask)
    {
	index = ffs(mask) -1;
	mask &= ~(index = 1 << index);
	switch(index)
	{
	  case CWBackingStore:
/*
	    if ((pWin->backingStore == WhenMapped) ||
	 	(pWin->backingStore == Always))
	    {
	    }
	    else
	    {
	    }
*/
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
		case USE_BACKGROUND_PIXEL:
                  pWin->PaintWindowBackground = mfbPaintWindowSolid;
	          pPrivWin->fastBackground = 0;
	          break;
		default:
		  if(mfbPadPixmap(pWin->backgroundTile))
		  {
		      pPrivWin->fastBackground = 1;
		      pPrivWin->oldRotate.x = pWin->absCorner.x;
		      pPrivWin->oldRotate.y = pWin->absCorner.y;
		      if (pPrivWin->pRotatedBackground)
			  mfbDestroyPixmap(pPrivWin->pRotatedBackground);
		      pPrivWin->pRotatedBackground =
			mfbCopyPixmap(pWin->backgroundTile);
		      mfbXRotatePixmap(pPrivWin->pRotatedBackground,
				       pWin->absCorner.x);
		      mfbYRotatePixmap(pPrivWin->pRotatedBackground,
				       pWin->absCorner.y);
		      pWin->PaintWindowBackground = mfbPaintWindow32;
		  }
		  else
		  {
		      pPrivWin->fastBackground = 0;
		      pWin->PaintWindowBackground = miPaintWindow;
		  }
		  break;
	      }
	      break;

	  case CWBackPixel:
              pWin->PaintWindowBackground = mfbPaintWindowSolid;
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
		case USE_BORDER_PIXEL:
	          pWin->PaintWindowBorder = mfbPaintWindowSolid;
	          pPrivWin->fastBorder = 0;
	          break;
		default:
		  if(mfbPadPixmap(pWin->borderTile))
		  {
		      pPrivWin->fastBorder = 1;
		      pPrivWin->oldRotate.x = pWin->absCorner.x;
		      pPrivWin->oldRotate.y = pWin->absCorner.y;
		      if (pPrivWin->pRotatedBorder)
			  mfbDestroyPixmap(pPrivWin->pRotatedBorder);
		      pPrivWin->pRotatedBorder =
			mfbCopyPixmap(pWin->borderTile);
		      mfbXRotatePixmap(pPrivWin->pRotatedBorder,
				       pWin->absCorner.x);
		      mfbYRotatePixmap(pPrivWin->pRotatedBorder,
				       pWin->absCorner.y);
		      pWin->PaintWindowBorder = mfbPaintWindow32;
		  }
		  else
		  {
		      pPrivWin->fastBorder = 0;
		      pWin->PaintWindowBorder = miPaintWindow;
		  }
		  break;
	      }
	      break;
	    case CWBorderPixel:
	      pWin->PaintWindowBorder = mfbPaintWindowSolid;
	      pPrivWin->fastBorder = 0;
	      break;

	}
    }
    /* Again, we have no failure modes indicated by any of the routines
     * we've called, so we have to assume it worked */
    return (TRUE);
}

