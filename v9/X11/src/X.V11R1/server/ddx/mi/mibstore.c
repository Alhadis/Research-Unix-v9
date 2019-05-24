/* $Header: mibstore.c,v 1.1 87/09/11 07:20:42 toddb Exp $ */
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
#include "Xprotostructs.h"

#include "misc.h"
#include "region.h"
#include "screenint.h"
#include "gc.h"
#include "window.h"
#include "pixmap.h"
#include "mi.h"

PixmapPtr miResizeBackingStore();

/* machine independent backing store

   window.devBackingStore has to point to an miBackingStore structure,
allocated and initialized by miInitBackingStore, and freed by
miFreeBackingStore.
   screen.validateGC needs to update the GC kept in the miBackingStore
structure by calling miValidateBackingStore()
   GC.clipType == CT_REGION or CT_NONE
   ValidateGC needs to keep a cumulative list of changes to the
GC's procedure vector.  miValidateBackingStore will zero the
flags when it is called.
   if a GC has been used with a window with backing store, and is
then used with one that isn't, ValidateGC must call
miUnhookBackingStore, which puts the real graphics calls into
the GC after saving any new stuff into miBSProcs.  

   this crufts ups ClearToBackground a little, but not a whole
lot.

NOTES
   we never have to initialize the backing store because we
never draw into an area that hasn't first been copied from the screen.
   all calculations are window-relative.
   NEVER assume pgcBlt is already validated!

   once it is created, the BackingStuff vector in the GC has the
most recent procs put in the GC, minus what's indicated in the 
procChanges flags.

JUSTIFICATION
   this is a cross between saving everything and just saving the
obscued areas (as in Pike's layers.)  this method has the advantage
of only doing each output operation once per pixel, visible or
invisible, and avoids having to do all the crufty storage
management of keeping several separate rectangles.  since the
ddx layer ouput primitives are required to draw through clipping
rectangles anyway, sending multiple drawing requests for each of
several rectangles isn't necessary.  (of course, it could be argued
that the ddx routines should just take one rectangle each and
get called multiple times, but that would make taking advantage of
smart hardware harder, and probably be slower as well.
*/

/* this returns a pointer to an miBackingStore structure.  it does not
put it into pWin->devBackingStore
*/

miBackingStore *
miInitBackingStore(pWin)
WindowPtr pWin;
{
    miBackingStore *pBack = (miBackingStore *)NULL;
    ScreenPtr pScreen;
    int status;

/* this is always called with backing store turned on ???
    if ((pWin->backingStore == WhenMapped) ||
	(pWin->backingStore == Always))
*/
    {
	pScreen = pWin->drawable.pScreen;
	pBack = (miBackingStore *)Xalloc(sizeof(miBackingStore));
	pBack->pGC = CreateGC(pWin, 0, 0, &status, 0);
	pBack->pLastGC = (GC *)NULL;
	pBack->pBackingPixmap =
	    (PixmapPtr)(*pScreen->CreatePixmap)(pScreen,
					      pWin->clientWinSize.width, 
					      pWin->clientWinSize.height,
					      1, XYBitmap);
	pBack->pSavedRegion = (* pScreen->RegionCreate)(NullBox, 1);
	pBack->pOrigClientClip = (* pScreen->RegionCreate)(NullBox, 1);
	pBack->clientClipOrg.x = 0;
	pBack->clientClipOrg.y = 0;
	pBack->pgcBlt = CreateGC(pWin, 0, 0, &status, 0);
    }
    return pBack;
}


/* turn off backing store.  all we need to do
   is invalidate the savedAreas.
*/
void
miRemoveBackingStore(pWin)
WindowPtr pWin;
{
    miBackingStore *pbs;
    BoxRec box;

    pbs = (miBackingStore *)(pWin->devBackingStore);
    box.x1 = box.y1 = box.x2 = box.y2 = 0;
    (* pWin->drawable.pScreen->RegionReset)(pbs->pSavedRegion, &box);
}



/* frees the miBackingStore pointed to by pWin->devBackingStore */
Bool miFreeBackingStore(pWin)
WindowPtr pWin;
{
    miBackingStore *pBack;
    pBack = ((miBackingStore *)(pWin->devBackingStore));
/* quid rides, berbex?  of COURSE we'll free it... */
}


void miSaveAreas(pWin)
register WindowPtr pWin;
{
    GC *pGC;
    PixmapPtr pBackingPixmap;
    RegionPtr prgnDoomed;
    register BoxPtr pbox;
    register int nbox;
    

    pBackingPixmap = 
	((miBackingStore *)(pWin->devBackingStore))->pBackingPixmap;
    pGC = ((miBackingStore *)(pWin->devBackingStore))->pgcBlt;
    ValidateGC(pBackingPixmap, pGC);

    prgnDoomed = pWin->backStorage->obscured;
    (* pWin->drawable.pScreen->TranslateRegion)(prgnDoomed, 
			     -pWin->absCorner.x, -pWin->absCorner.y);
    pbox = prgnDoomed->rects;
    nbox = prgnDoomed->numRects;
    while(nbox--)
    {
	(*pGC->CopyArea)(pWin, pBackingPixmap, pGC,
			 pbox->x1, pbox->y1,
			 pbox->x2-pbox->x1, pbox->y2-pbox->y1,
			 pbox->x1, pbox->y1);
	pbox++;
    }

    (* pWin->drawable.pScreen->Union)
	 (((miBackingStore *)(pWin->devBackingStore))->pSavedRegion,
	  ((miBackingStore *)(pWin->devBackingStore))->pSavedRegion,
	  prgnDoomed);
}


/* this is called before sending any exposure events to the client,
and so might be called if the window has grown.  changing the backing
pixmap doesn't require revalidating the backingGC because since
the window clip region has changed, the client's next output
request will result in a call to ValidateGC, which will in turn call
ValidateBackingStore.
   it replaces pWin->exposed with the region that could not be
restored from backing store.
   NOTE: this must be called with pWin->exposed window-relative.

*/
void
miRestoreAreas(pWin)
register WindowPtr pWin;
{
/* NOTE
   i think prgRestored and prgnFailed can be the same thing....
*/
    PixmapPtr pBackingPixmap;
    GC *pGC;
    RegionPtr prgnSaved;
    RegionPtr prgnExposed;
    RegionPtr prgnRestored;
    register BoxPtr pbox;
    register int nbox;
    register ScreenPtr pScreen;

    pScreen = pWin->drawable.pScreen;
    pGC = ((miBackingStore *)(pWin->devBackingStore))->pgcBlt;
    pBackingPixmap = 
	((miBackingStore *)(pWin->devBackingStore))->pBackingPixmap;
    prgnSaved = ((miBackingStore *)(pWin->devBackingStore))->pSavedRegion;
    prgnExposed = pWin->exposed;
    prgnRestored = (* pScreen->RegionCreate)((BoxPtr)NULL, 1);

    if ((pWin->clientWinSize.width != pBackingPixmap->width) ||
	(pWin->clientWinSize.height != pBackingPixmap->height))
    {
	pBackingPixmap = miResizeBackingStore(pWin);
    }

    (* pScreen->Intersect)(prgnRestored, prgnSaved, prgnExposed);
    pbox = prgnRestored->rects;
    nbox = prgnRestored->numRects;

    ValidateGC(pWin, pGC);
    while(nbox--)
    {
	(*pGC->CopyArea)(pBackingPixmap, pWin, pGC,
			 pbox->x1, pbox->y1,
			 pbox->x2-pbox->x1, pbox->y2-pbox->y1,
			 pbox->x1, pbox->y1);
	pbox++;
    }

    /* since pWin->exposed is no longer obscured, we no longer
       will have a valid copy of it in backing store
    */
    (* pScreen->Subtract)(prgnSaved, prgnSaved, prgnExposed);

    /* change pWin->exposed to exclude anything we've restored */
    (* pScreen->Subtract)(prgnExposed, prgnExposed, prgnRestored);

    (* pScreen->RegionDestroy)(prgnRestored);
}


/* bit gravity with backing store may require that the backing store
   be translated.  we may need to grow or shrink the backing store.
*/

void
miTranslateBackingStore(pWin, dx, dy)
WindowPtr pWin;
int dx;			/* translation distance */
int dy;
{
    register miBackingStore *pbs;
    register PixmapPtr pBackingPixmap;
    register RegionPtr prgnSaved;
    register GC *pgc;

    pbs = (miBackingStore *)(pWin->devBackingStore);
    prgnSaved = pbs->pSavedRegion;
    pBackingPixmap = pbs->pBackingPixmap;
    pgc = pbs->pgcBlt;

    if ((pWin->clientWinSize.width != pBackingPixmap->width) ||
	(pWin->clientWinSize.height != pBackingPixmap->height))
    {
	pBackingPixmap = miResizeBackingStore(pWin);
    }

    if (prgnSaved->numRects)
    {
	ValidateGC(pBackingPixmap, pgc);
	(pgc->CopyArea)(pBackingPixmap, pBackingPixmap, pgc,
			prgnSaved->extents.x1, prgnSaved->extents.x2,
			prgnSaved->extents.x2 - prgnSaved->extents.x1,
			prgnSaved->extents.y2 - prgnSaved->extents.y1,
			prgnSaved->extents.x1 + dx,
			prgnSaved->extents.y1 + dy);
	(* pWin->drawable.pScreen->TranslateRegion)(prgnSaved, dx, dy);
    }
}


/*  miValidateBackingStore must be called before pWin->drawable.clipListChanged
and pGC->stateChanges are reset.

    if this GC is the one we've been using, copy any fields
	that have changed to the backing GC
    else
	 copy the whole thing.

    if this GC has never been used with backing store before
	allocate miBSProcs

    for each function in the GC that has been changed since last time
	copy it to miBSProcs
	replace it with mibs function.

now set up clipping to draw only through client clip into obscured areas
    if win clip changed or client clip changed or client clip moved
        if clip origin is changed
	    save it in miBackingStore.clientClipOrg
	    set clipOrg.x = clipOrg.y = 0

	if clientClipType is a region
	    if clientClip has changed
	        copy clientClip to pOrigClientClip
	    else
	        copy pOrigClientClip to clientClip
	    translate clientClip by miBackingStore.clientClipOrigin
        else (client clip is none)
	    clientClip = pixmap bounds

        intersect pClientClip with (window rect - clipList)

make the GC/pixmap pair usable
    if stateChanges or window clip changed
        ValidateGC(pBackingPixmap, pBackingGC)

NOTE:
    we always exit this routine with BackingGC.clientClipType == CT_REGION
*/
miValidateBackingStore(pWin, pGC)
register WindowPtr pWin;
register GC *pGC;
{
    register miBackingStore *pBackingStore;
    PixmapPtr pBackingPixmap;
    GC *pBackingGC;
    int stateChanges;
    RegionPtr prgnTmp;			/* for intermediate computation */
    BoxRec	winbounds;		/* window on screen */

    pBackingStore = (miBackingStore *)(pWin->devBackingStore);
    pBackingGC = pBackingStore->pGC;
    pBackingPixmap = pBackingStore->pBackingPixmap;

    if (pBackingStore->pLastGC != pGC)
	stateChanges = (1 << 23) - 1;
    else
    {
	stateChanges = pGC->stateChanges;
	if (pWin->drawable.clipListChanged)
	    stateChanges |= GCClipMask;
    }

    CopyGC(pGC, pBackingGC, stateChanges);

    if (!pGC->pBackingStuff)
    {
	pGC->pBackingStuff = (pointer)Xalloc(sizeof(miBSProcs));
	pGC->procChanges = (1 << NUM_OUTPUT_FNS) - 1;
    }

    CopyGCProcsToBS(pGC);
    InstallBSHooksInGC(pGC, pGC->procChanges);

    pGC->procChanges = 0;
    pGC->usedWithBackingStore = 1;


    if ((pWin->drawable.clipListChanged) ||
	(stateChanges & (GCClipXOrigin | GCClipYOrigin | GCClipMask)))
    {
        if ((stateChanges & GCClipXOrigin) ||
	    (stateChanges & GCClipYOrigin))
        {
	    pBackingStore->clientClipOrg = pBackingGC->clipOrg;
	    pBackingGC->clipOrg.x = 0;
	    pBackingGC->clipOrg.y = 0;
        }

	/* use the original GC's clip type because the backing GC's
	   clip type may be a CT_REGION we put in earlier.
	*/
        if (pGC->clientClipType == CT_REGION)
        {
	    if (stateChanges & GCClipMask)
	    {
	        (* pGC->pScreen->RegionCopy)(pBackingStore->pOrigClientClip,
		           pBackingGC->clientClip.pRegion);
	    }
	    else
	    {
	        (* pGC->pScreen->RegionCopy)(pBackingGC->clientClip.pRegion,
			   pBackingStore->pOrigClientClip);
	    }
	    (* pGC->pScreen->TranslateRegion)(pBackingGC->clientClip.pRegion,
			    pBackingStore->clientClipOrg.x,
			    pBackingStore->clientClipOrg.y);
	}
	else
	{
	    /* arrive here only if client has CT_NONE in original GC */
	    BoxRec pixbounds;

	    pixbounds.x1 = 0;
	    pixbounds.y1 = 0;
	    pixbounds.x2 = pBackingPixmap->width;
	    pixbounds.y2 = pBackingPixmap->height;
	    /* if clipType in backing GC is a region, it's something
	       we put there before; otherwise, it's a CT_NONE we
	       got from CopyGC after the client changed his clip
	       region.
	    */
	    if (pBackingGC->clientClipType == CT_REGION)
		(* pBackingGC->pScreen->RegionReset)(pBackingGC->clientClip.pRegion,
						   &pixbounds);
	    else
	        pBackingGC->clientClip.pRegion = 
			   (* pBackingGC->pScreen->RegionCreate)(&pixbounds, 1);
	}
	pBackingGC->clientClipType = CT_REGION;

	/* get inverse of window clip list.
	   we have to translate because everything we use from
	   the window structure is screen-relative, and we need
	   it window-relative
	*/
	winbounds.x1 = pWin->absCorner.x;
	winbounds.y1 = pWin->absCorner.y;
	winbounds.x2 = winbounds.x1 + pWin->clientWinSize.width;
	winbounds.y2 = winbounds.y1 + pWin->clientWinSize.height;

	prgnTmp = (* pBackingGC->pScreen->RegionCreate)(&winbounds, 1);
	(* pBackingGC->pScreen->Subtract)(prgnTmp, prgnTmp, pWin->clipList);
	(* pBackingGC->pScreen->TranslateRegion)(prgnTmp, 
			       -pWin->absCorner.x, -pWin->absCorner.y);
	(* pBackingGC->pScreen->Intersect)(pBackingGC->clientClip.pRegion,
		  pBackingGC->clientClip.pRegion,
		  prgnTmp);
	(* pBackingGC->pScreen->RegionDestroy)(prgnTmp);
	pBackingGC->stateChanges |= GCClipMask;
    }

    ValidateGC(pBackingPixmap, pBackingGC);
    pBackingStore->pLastGC = pGC;
}


/* this grows or shrinks the backing pixmap, and copies the
   old contents into it
   copy the whole bounding box of the the SavedRegion, on
   the assumption that that's easier than copying the
   component boxes one at a time
   returns pointer to new backng pixmap
*/
PixmapPtr 
miResizeBackingStore(pWin)
WindowPtr pWin;
{
    miBackingStore *pbs;
    PixmapPtr pNewPixmap;
    PixmapPtr pBackingPixmap;
    ScreenPtr pScreen;
    GC	   *pGC;

    pbs = (miBackingStore *)(pWin->devBackingStore);
    pGC = pbs->pgcBlt;
    pScreen = pWin->drawable.pScreen;
    pBackingPixmap = pbs->pBackingPixmap;
    pNewPixmap =
        (PixmapPtr)(*pScreen->CreatePixmap)(pScreen, 
					   pWin->clientWinSize.width, 
					   pWin->clientWinSize.height, 
					   1, XYBitmap);

    ValidateGC(pNewPixmap, pGC);
    (*pGC->CopyArea)(pBackingPixmap, pNewPixmap, pGC,
		     0, 0, pBackingPixmap->width, pBackingPixmap->height,
		     0, 0);
    pbs->pBackingPixmap = pNewPixmap;

    /* clip the window's SavedRegion to the new window size */
    if ((pBackingPixmap->width < pWin->clientWinSize.width) ||
        (pBackingPixmap->height < pWin->clientWinSize.height))
    {
        BoxRec pixbounds;
        RegionPtr prgnTmp;

        pixbounds.x1 = 0;
        pixbounds.x2 = pNewPixmap->width;
        pixbounds.x1 = 0;
        pixbounds.x2 = pNewPixmap->width;
        prgnTmp = (* pScreen->RegionCreate)(&pixbounds, 1);
        (* pScreen->Intersect)(pbs->pSavedRegion, pbs->pSavedRegion, prgnTmp);
        (* pScreen->RegionDestroy)(prgnTmp);
    }
    (*pScreen->DestroyPixmap)(pBackingPixmap);
}



/*
    update the BSProcs, so if this GC is ever used with backing
store again we'll have something to start from.
    restore the REAL graphics routines to the GC
    mark the GC as not being in use by backing store
*/
void miUnhookBackingStore(pGC)
GC *pGC;
{
    miBSProcs *pBSProcs;

    if (!pGC->usedWithBackingStore)
	return;

    pBSProcs = (miBSProcs *)(pGC->pBackingStuff);
    if (!pBSProcs)
	return;

    CopyGCProcsToBS(pGC);
    pGC->procChanges = 0;

    pGC->FillSpans = pBSProcs->FillSpans;
    pGC->SetSpans = pBSProcs->SetSpans;
    pGC->GetSpans = pBSProcs->GetSpans;
    pGC->PutImage = pBSProcs->PutImage;
    pGC->CopyArea = pBSProcs->CopyArea;
    pGC->CopyPlane = pBSProcs->CopyPlane;
    pGC->PolyPoint = pBSProcs->PolyPoint;
    pGC->Polylines = pBSProcs->Polylines;
    pGC->PolySegment = pBSProcs->PolySegment;
    pGC->PolyRectangle = pBSProcs->PolyRectangle;
    pGC->PolyArc = pBSProcs->PolyArc;
    pGC->FillPolygon = pBSProcs->FillPolygon;
    pGC->PolyFillRect = pBSProcs->PolyFillRect;
    pGC->PolyFillArc = pBSProcs->PolyFillArc;
/*
    pGC->PolyText8 = pBSProcs->PolyText8;
    pGC->ImageText8 = pBSProcs->ImageText8;
*/

    pGC->usedWithBackingStore = 0;
}


static
CopyGCProcsToBS(pGC)
register GC *pGC;
{
    register miBSProcs *pBSProcs;

    register int mask;
    register int index;


    pBSProcs = (miBSProcs *)(pGC->pBackingStuff);
    mask = pGC->procChanges;
    while (mask)
    {
	index = ffs(mask) - 1;
	mask &= ~(index = (1 << index));
	switch (index)
	{

	  case GCFN_FILLSPANS:
	    pBSProcs->FillSpans = pGC->FillSpans;
	    break;
	  case GCFN_SETSPANS:
	    pBSProcs->SetSpans = pGC->SetSpans;
	    break;
	  case GCFN_GETSPANS:
	    pBSProcs->GetSpans = pGC->GetSpans;
	    break;
	  case GCFN_PUTIMAGE:
	    pBSProcs->PutImage = pGC->PutImage;
	    break;
	  case GCFN_COPYAREA:
	    pBSProcs->CopyArea = pGC->CopyArea;
	    break;
	  case GCFN_COPYPLANE:
	    pBSProcs->CopyPlane = pGC->CopyPlane;
	    break;
	  case GCFN_POLYPOINT:
	    pBSProcs->PolyPoint = pGC->PolyPoint;
	    break;
	  case GCFN_POLYLINES:
	    pBSProcs->Polylines = pGC->Polylines;
	    break;
	  case GCFN_POLYSEGMENT:
	    pBSProcs->PolySegment = pGC->PolySegment;
	    break;
	  case GCFN_POLYRECTANGLE:
	    pBSProcs->PolyRectangle = pGC->PolyRectangle;
	    break;
	  case GCFN_POLYARC:
	    pBSProcs->PolyArc = pGC->PolyArc;
	    break;
	  case GCFN_FILLPOLYGON:
	    pBSProcs->FillPolygon = pGC->FillPolygon;
	    break;
	  case GCFN_POLYFILLRECT:
	    pBSProcs->PolyFillRect = pGC->PolyFillRect;
	    break;
	  case GCFN_POLYFILLARC:
	    pBSProcs->PolyFillArc = pGC->PolyFillArc;
	    break;
/*
	  case GCFN_POLYTEXT:
	    pBSProcs->PolyText = pGC->PolyText;
	    break;
	  case GCFN_IMAGETEXT:
	    pBSProcs->ImageText = pGC->ImageText;
	    break;
*/
	  default:
	    break;

	}
    }
}

/* puts mibsWhatever in the GC for each routine in the
GC  that doesn't have a hook in it yet
*/
static
InstallBSHooksInGC(pGC, installMask)
register GC *pGC;
register int installMask;
{
    register int index;

    while(installMask)
    {
	index = ffs(installMask) - 1;
	installMask &= ~(index = (1 << index));
	switch (index)
	{

	  case GCFN_FILLSPANS:
	    pGC->FillSpans = mibsFillSpans;
	    break;
	  case GCFN_SETSPANS:
	    pGC->SetSpans = mibsSetSpans;
	    break;
	  case GCFN_GETSPANS:
	    pGC->GetSpans = mibsGetSpans;
	    break;
	  case GCFN_PUTIMAGE:
	    pGC->PutImage = mibsPutImage;
	    break;
	  case GCFN_COPYAREA:
	    pGC->CopyArea = mibsCopyArea;
	    break;
	  case GCFN_COPYPLANE:
	    pGC->CopyPlane = mibsCopyPlane;
	    break;
	  case GCFN_POLYPOINT:
	    pGC->PolyPoint = mibsPolyPoint;
	    break;
	  case GCFN_POLYLINES:
	    pGC->Polylines = mibsPolylines;
	    break;
	  case GCFN_POLYSEGMENT:
	    pGC->PolySegment = mibsPolySegment;
	    break;
	  case GCFN_POLYRECTANGLE:
	    pGC->PolyRectangle = mibsPolyRectangle;
	    break;
	  case GCFN_POLYARC:
	    pGC->PolyArc = mibsPolyArc;
	    break;
	  case GCFN_FILLPOLYGON:
	    pGC->FillPolygon = mibsFillPolygon;
	    break;
	  case GCFN_POLYFILLRECT:
	    pGC->PolyFillRect = mibsPolyFillRect;
	    break;
	  case GCFN_POLYFILLARC:
	    pGC->PolyFillArc = mibsPolyFillArc;
	    break;
/*
	  case GCFN_POLYTEXT:
	    pGC->PolyText = mibsPolyText;
	    break;
	  case GCFN_IMAGETEXT:
	    pGC->ImageText = mibsImageText;
	    break;
*/
	  default:
	    break;
	}
    }
}



/*
    mibs* generally call the saved proc vector in the GC,
    then call the one in the backing store's GC
*/

#define REALOUTPUT(pGC, name) \
    (((miBSProcs *)(pGC->pBackingStuff))->name)

#define BACKINGGC(pWin) \
    (((miBackingStore *)(pWin->devBackingStore))->pGC)

#define BACKINGPIXMAP(pWin) \
    ((miBackingStore *)(pWin->devBackingStore))->pBackingPixmap

#define BACKINGOUTPUT(pWin,name) \
    (BACKINGGC(pWin)->name)


void  mibsFillSpans()
{
}

void  mibsSetSpans()
{
}

int *
mibsGetSpans()
{
}


void  mibsPutImage()
{
}

void  mibsCopyArea()
{
}

void  mibsCopyPlane()
{
}

void  mibsPolyPoint()
{
}

void  mibsPolylines()
{
}

void  mibsPolySegment()
{
}

void  mibsPolyRectangle()
{
}

void  mibsPolyArc()
{
}

void  mibsFillPolygon()
{
}

void mibsPolyFillRect(pWin, pGC, nrectFill, prectInit)
WindowPtr pWin;
GC *pGC;
int nrectFill; 			/* number of rectangles to fill */
xRectangle *prectInit;  	/* Pointer to first rectangle to fill */
{
    xRectangle *prect;

/* NOTE
   should only copy if backing GC's composite clip has > 0 rectangles
*/
    prect = (xRectangle *)ALLOCATE_LOCAL(nrectFill * sizeof(xRectangle));
    bcopy(prectInit, prect, nrectFill * sizeof(xRectangle));

    (*REALOUTPUT(pGC, PolyFillRect))(pWin, pGC, nrectFill, prectInit);
    (*BACKINGOUTPUT(pWin, PolyFillRect))(BACKINGPIXMAP(pWin), 
					 BACKINGGC(pWin),
					 nrectFill, prect);
    DEALLOCATE_LOCAL(prect);
}

void  mibsPolyFillArc()
{
}

int mibsPolyText8()
{
}

int mibsPolyText16()
{
}

void  mibsImageText8()
{
}

void  mibsImageText16()
{
}



