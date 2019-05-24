/*
 * mivaltree.c --
 *	Functions for recalculating window clip lists. Main function
 *	is miValidateTree.
 *
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
 * and the Massachusetts Institute of Technology, Cambridge, Massachusetts.
 * 
 *                         All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in 
 * supporting documentation, and that the names of Digital or MIT not be
 * used in advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.  
 * 
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 * 
 ******************************************************************/

 /* 
  * Aug '86: Susan Angebranndt -- original code
  * July '87: Adam de Boor -- substantially modified and commented
  */

#ifndef lint
static char rcsid[] =
"$Header: mivaltree.c,v 1.36 87/09/11 07:20:34 toddb Exp $ SPRITE (Berkeley)";
#endif lint

#include    "X.h"
#include    "scrnintstr.h"
#include    "windowstr.h"
#include    "mi.h"
#include    "region.h"

static RegionPtr	exposed = NullRegion;

/*-
 *-----------------------------------------------------------------------
 * miComputeClips --
 *	Recompute the clipList, borderClip, exposed and borderExposed
 *	regions for pParent and its children. Only viewable windows are
 *	taken into account.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	clipList, borderClip, exposed and borderExposed are altered.
 *	A VisibilityNotify event may be generated on the parent window.
 *
 *-----------------------------------------------------------------------
 */
static void
miComputeClips (pParent, pScreen, universe)
    register WindowPtr	pParent;
    register ScreenPtr	pScreen;
    register RegionPtr	universe;
{
    int			dx,
			dy;
    RegionPtr		childUniverse;
    register WindowPtr	pChild;
    int     	  	oldVis;
    BoxPtr  	  	borderSize;
    
    
    childUniverse = (RegionPtr)NULL;

    /*
     * Figure out the new visibility of this window.
     * The extent of the universe should be the same as the extent of
     * the borderSize region. If the window is unobscured, this rectangle
     * will be completely inside the universe (the universe will cover it
     * completely). If the window is completely obscured, none of the
     * universe will cover the rectangle.
     */
    borderSize = (* pScreen->RegionExtents) (pParent->borderSize);
    
    oldVis = pParent->visibility;
    switch ((* pScreen->RectIn) (universe, borderSize)) 
    {
	case rgnIN:
	    pParent->visibility = VisibilityUnobscured;
	    break;
	case rgnPART:
	    pParent->visibility = VisibilityPartiallyObscured;
	    break;
	default:
	    pParent->visibility = VisibilityFullyObscured;
	    break;
    }
    if (oldVis != pParent->visibility)
	SendVisibilityNotify(pParent);

    /*
     * To calculate exposures correctly, we have to translate the old
     * borderClip and clipList regions to the window's new location so there
     * is a correspondence between pieces of the new and old clipping regions.
     */
    dx = pParent->absCorner.x - pParent->oldAbsCorner.x;
    dy = pParent->absCorner.y - pParent->oldAbsCorner.y;

    if (dx || dy) 
    {
	/*
	 * If the window has moved, the border will *not* be copied, so we
	 * empty the old border clip to force exposure of the whole thing.
	 * We translate the old clipList because that will be exposed or copied
	 * if gravity is right.
	 */
	(* pScreen->RegionEmpty) (pParent->borderClip);
	(* pScreen->TranslateRegion) (pParent->clipList, dx, dy);
	pParent->oldAbsCorner = pParent->absCorner;
    } 
    else if (pParent->borderWidth) 
    {
	/*
	 * If the window has shrunk, we have to be careful about figuring the
	 * exposure of the right and bottom borders -- they should be exposed
	 * if the window shrank on that side and they aren't being obscured
	 * by a sibling -- so we add those edges to the borderExposed region.
	 * This is all necessary because sometimes what was the internal window
	 * in the old borderClip will overlap the new border and cause the
	 * right and bottom edges of the new border not to appear in the
	 * borderExposed region and it's a royal pain to figure out what to
	 * remove from the old borderClip.
	 * XXX: Isn't there a nicer way to do this?
	 */
	BoxPtr	oldExtents;
	BoxPtr	newExtents;
	BoxPtr	windowExtents;
	
	oldExtents = (* pScreen->RegionExtents) (pParent->borderClip);
	newExtents = (* pScreen->RegionExtents) (universe);
	windowExtents = (* pScreen->RegionExtents) (pParent->borderSize);

	if ((* pScreen->RegionNotEmpty) (universe) &&
	    (* pScreen->RegionNotEmpty) (pParent->borderClip) &&
	    ((newExtents->x2 < oldExtents->x2) ||
	     (newExtents->y2 < oldExtents->y2))) 
        {
	    BoxRec 	borderBox;
	    RegionPtr 	borderRegion;

	    borderRegion = (* pScreen->RegionCreate) (NULL, 1);

	    if (newExtents->x2 < oldExtents->x2) 
            {
		/*
		 * Add the right edge.
		 */
		borderBox.x1 = windowExtents->x2 - pParent->borderWidth;
		borderBox.y1 = windowExtents->y1;
		borderBox.x2 = windowExtents->x2;
		borderBox.y2 = windowExtents->y2;
		(* pScreen->RegionReset) (borderRegion, &borderBox);
		(* pScreen->Union) (pParent->borderExposed,
				    pParent->borderExposed,
				    borderRegion);
	    }
	    if (newExtents->y2 < oldExtents->y2) 
            {
		/*
		 * Add the bottom edge.
		 */
		borderBox.x1 = windowExtents->x1;
		borderBox.y1 = windowExtents->y2 - pParent->borderWidth;
		borderBox.x2 = windowExtents->x2;
		borderBox.y2 = windowExtents->y2;
		(* pScreen->RegionReset) (borderRegion, &borderBox);
		(* pScreen->Union) (pParent->borderExposed,
				    pParent->borderExposed,
				    borderRegion);
	    }
	    (* pScreen->RegionDestroy) (borderRegion);

	    /*
	     * To make sure we don't expose a border that's supposed to
	     * be clipped, clip the regions we just added to borderExposed...
	     */
	    (* pScreen->Intersect) (pParent->borderExposed, universe,
				    pParent->borderExposed);
	}
    }

    /*
     * Since the borderClip must not be clipped by the children, we do
     * the border exposure first...
     *
     * 'universe' is the window's borderClip. To figure the exposures, remove
     * the area that used to be exposed (the old borderClip) from the new.
     * This leaves a region of pieces that weren't exposed before. Border
     * exposures accumulate until they're taken care of.
     */
    (* pScreen->Subtract) (exposed, universe, pParent->borderClip);
    (* pScreen->Subtract) (exposed, exposed, pParent->winSize);

    (* pScreen->Union) (pParent->borderExposed, pParent->borderExposed,
			exposed);

    (* pScreen->RegionCopy) (pParent->borderClip, universe);

    /*
     * To get the right clipList for the parent, and to make doubly sure
     * that no child overlaps the parent's border, we remove the parent's
     * border from the universe before proceeding.
     */
    (* pScreen->Intersect) (universe, universe, pParent->winSize);
    
    for (pChild = pParent->firstChild; pChild; pChild = pChild->nextSib) 
    {
	if (pChild->viewable)
        {
	    /*
	     * If the child is viewable, we want to remove its extents from
	     * the current universe, but we only re-clip it if it's been
	     * marked.
	     */
	    if (pChild->marked) 
            {
		/*
		 * Figure out the new universe from the child's perspective and
		 * recurse.
		 */
		if (childUniverse == NullRegion) 
                {
		    childUniverse = (* pScreen->RegionCreate) (NULL, 1);
		}
		(* pScreen->Intersect) (childUniverse, universe,
					pChild->borderSize);
		miComputeClips (pChild, pScreen, childUniverse);
	    }
	    /*
	     * Once the child has been processed, we remove its extents from
	     * the current universe, thus denying its space to any other
	     * sibling.
	     */
	    (* pScreen->Subtract) (universe, universe, pChild->borderSize);
	} 
        else 
        {
	    pChild->marked = 0;
	    if ((* pScreen->RegionNotEmpty) (pChild->borderClip)) 
            {
		/*
		 * If the child isn't viewable, it has no business taking up
		 * clipping space, so we nuke all its clipping regions. There
		 * was a problem with UnmapGravity where the clipList wouldn't
		 * get nuked, so drawing operations succeeded while the window
		 * was unmapped...
		 */
		(* pScreen->RegionEmpty) (pChild->borderClip);
		(* pScreen->RegionEmpty) (pChild->clipList);
		(* pScreen->RegionEmpty) (pChild->exposed);
		(* pScreen->RegionEmpty) (pChild->borderExposed);
		pChild->drawable.serialNumber = NEXT_SERIAL_NUMBER;
	    }
	}
    }

    /*
     * 'universe' now contains the new clipList for the parent window.
     *
     * To figure the exposure of the window we subtract the old clip from the
     * new, just as for the border. Again, exposures accumulate.
     */
    (* pScreen->Subtract) (exposed, universe, pParent->clipList);
    
    (* pScreen->Union) (pParent->exposed, pParent->exposed, exposed);

    /*
     * One last thing: backing storage. We have to find out what parts of
     * the window are about to be obscured. We can just subtract the universe
     * from the old clipList and get the areas that were in the old but aren't
     * in the new and, hence, are about to be obscured.
     */
    if (pParent->backStorage && (pParent->backingStore != NotUseful)) 
    {
	(* pScreen->Subtract) (exposed, pParent->clipList, universe);
	(* pScreen->Union) (pParent->backStorage->obscured,
			    pParent->backStorage->obscured,
			    exposed);
    }
    
    (* pScreen->RegionCopy) (pParent->clipList, universe);

    pParent->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    pParent->marked = 0;

    if (childUniverse) 
	(* pScreen->RegionDestroy) (childUniverse);
}

/*-
 *-----------------------------------------------------------------------
 * miValidateTree --
 *	Recomputes the clip list for pParent and all its inferiors.
 *
 * Results:
 *	Always returns 1.
 *
 * Side Effects:
 *	The clipList, borderClip, exposed, and borderExposed regions for
 *	each marked window are altered.
 *
 * Notes:
 *	This routine assumes that all affected windows have had their
 *	marked fields set true and their winSize and borderSize regions
 *	adjusted to correspond to their new positions. The borderClip and
 *	clipList regions should not have been touched.
 *
 *	The top-most level is treated differently from all lower levels
 *	because pParent is unchanged. For the top level, we merge the
 *	regions taken up by the marked children back into the clipList
 *	for pParent, thus forming a region from which the marked children
 *	can claim their areas. For lower levels, where the old clipList
 *	and borderClip are invalid, we can't do this and have to do the
 *	extra operations done in miComputeClips, but this is much faster
 *	e.g. when only one child has moved...
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
int
miValidateTree (pParent, pChild, top, anyMarked)
    WindowPtr	  	pParent;    /* Parent to validate */
    WindowPtr	  	pChild;     /* First child of pParent that was
				     * affected */
    Bool    	  	top;	    /* TRUE if at top level. UNUSED */
    Bool    	  	anyMarked;  /* TRUE if any windows were marked. */
{
    RegionPtr	  	totalClip;  /* Total clipping region available to
				     * the marked children. pParent's clipList
				     * merged with the borderClips of all
				     * the marked children. */
    RegionPtr	  	childClip;  /* The new borderClip for the current
				     * child */
    register ScreenPtr	pScreen;
    register WindowPtr	pWin;

    if (!anyMarked) 
    {
	return (1);
    }
    pScreen = pParent->drawable.pScreen;

    if (exposed == NullRegion) 
    {
	exposed = (* pScreen->RegionCreate) (NULL, 1);
    }

    totalClip = (* pScreen->RegionCreate) (NULL, 1);
    childClip = (* pScreen->RegionCreate) (NULL, 1);

    /*
     * Preserve the parent's old clipList to make calculating exposures
     * easier.
     */
    (* pScreen->RegionCopy) (totalClip, pParent->clipList);

    if (pChild == NullWindow) 
    {
	pChild = pParent->firstChild;
    }
			       
    /*
     * First merge in the child windows at their old places.
     */
    for (pWin = pChild; pWin != NullWindow; pWin = pWin->nextSib) 
    {
	if (pWin->marked) 
        {
	    (* pScreen->Union) (totalClip, totalClip, pWin->borderClip);
	}
    }

    /*
     * Now go through the children of the root and figure their new
     * borderClips from the totalClip, passing that off to miComputeClips
     * to handle recursively. Once that's done, we remove the child
     * from the totalClip to clip any siblings below it.
     */
    for (pWin = pChild; pWin != NullWindow; pWin = pWin->nextSib) 
    {
	if (pWin->viewable) 
        {
	    if (pWin->marked) 
            {
		(* pScreen->Intersect) (childClip, totalClip, pWin->borderSize);
		miComputeClips (pWin, pScreen, childClip);
		(* pScreen->Subtract) (totalClip, totalClip, pWin->borderSize);
	    }
	} 
        else 
        {
	    /*
	     * Make sure the child is no longer marked (Windows being unmapped
	     * are marked but unviewable...)
	     */
	    pWin->marked = 0;
	    if ((* pScreen->RegionNotEmpty) (pWin->borderClip)) 
            {
		/*
		 * If the child isn't viewable, it has no business taking up
		 * clipping space, so we nuke all its clipping regions. There
		 * was a problem with UnmapGravity where the clipList wouldn't
		 * get nuked, so drawing operations succeeded while the window
		 * was unmapped...
		 */
		(* pScreen->RegionEmpty) (pChild->borderClip);
		(* pScreen->RegionEmpty) (pChild->clipList);
		(* pScreen->RegionEmpty) (pChild->exposed);
		(* pScreen->RegionEmpty) (pChild->borderExposed);
		pChild->drawable.serialNumber = NEXT_SERIAL_NUMBER;
	    }
	}
    }

    /*
     * totalClip contains the new clipList for the parent. Figure out
     * exposures and obscures as per miComputeClips and reset the parent's
     * clipList.
     */
    (* pScreen->Subtract) (exposed, totalClip, pParent->clipList);
    (* pScreen->Union) (pParent->exposed, pParent->exposed, exposed);

    if (pParent->backStorage && (pParent->backingStore != NotUseful)) 
    {
	(* pScreen->Subtract) (exposed, pParent->clipList, totalClip);
	(* pScreen->Union) (pParent->backStorage->obscured,
			    pParent->backStorage->obscured,
			    exposed);
    }
    
    (* pScreen->RegionCopy) (pParent->clipList, totalClip);
    pParent->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    pParent->marked = 0;

    (* pScreen->RegionDestroy) (childClip);

    WindowsRestructured();

    (* pScreen->RegionDestroy) (totalClip);

    return (1);
}
