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

/* $Header: colormap.c,v 1.52 87/09/11 07:18:27 toddb Exp $ */

#include "X.h"
#define NEED_EVENTS
#include "Xproto.h"
#include "misc.h"
#include "dix.h"
#include "colormapst.h"
#include "os.h"
#include "scrnintstr.h"
#include "resource.h"
#include "windowstr.h"

Pixel	FindBestPixel();

/* GetNextBitsOrBreak(bits, mask, base)  -- 
 * (Suggestion: First read the macro, then read this explanation.
 *
 * Either generate the next value to OR in to a pixel or break out of this
 * while loop 
 *
 * This macro is used when we're trying to generate all 2^n combinations of
 * bits in mask.  What we're doing here is counting in binary, except that
 * the bits we use to count may not be contiguous.  This macro will be
 * called 2^n times, returning a different value in bits each time. Then
 * it will cause us to break out of a surrounding loop. (It will always be
 * called from within a while loop.)
 * On call: mask is the value we want to find all the combinations for
 * base has 1 bit set where the least significant bit of mask is set
 *
 * For example,if mask is 01010, base should be 0010 and we count like this:
 * 00010 (see this isn't so hard), 
 *     then we add base to bits and get 0100. (bits & ~mask) is (0100 & 0100) so
 *      we add that to bits getting (0100 + 0100) =
 * 01000 for our next value.
 *      then we add 0010 to get 
 * 01010 and we're done (easy as 1, 2, 3)
 */
#define GetNextBitsOrBreak(bits, mask, base)	\
	    if((bits) == (mask)) 		\
		break;		 		\
	    (bits) += (base);		 	\
	    while((bits) & ~(mask))		\
		(bits) += ((bits) & ~(mask));	
/* ID of server as client */
#define SERVER_ID	0
#define NoneFound	-1

typedef struct 
{
	Colormap	mid;
	int		client;
	} colorResource;

/* Invariants:
 * refcnt == 0 means entry is empty
 * refcnt > 0 means entry is useable by many clients, so it can't be changed
 * refcnt == AllocPrivate means entry owned by one client only
 * fShared should only be set if refcnt == AllocPrivate
 */


/* Create and initialize the color map */
int 
CreateColormap (mid, pScreen, pVisual, ppcmap, alloc, client)
    int		mid;		/* resource to use for this colormap */
    ScreenPtr	pScreen;
    VisualPtr	pVisual;
    ColormapPtr	*ppcmap;	
    int		alloc;		/* 1 iff all entries are allocated writeable */
    unsigned int client;
{
    int		class, size, sizebytes;
    ColormapPtr	pmap;
    EntryPtr	pent;
    int		i, j;
    Pixel	*ppix;

    class = pVisual->class;
    if(!(class & DynamicClass) && alloc != AllocNone && client != SERVER_ID)
	return (BadMatch);

    if((pmap = (ColormapPtr) Xalloc(sizeof(ColormapRec))) == (ColormapPtr)NULL)
	goto NoMap;
    AddResource(mid, RT_COLORMAP, pmap, FreeColormap, RC_CORE);
    pmap->mid = mid;
    pmap->flags = 0; 	/* start out with all flags clear */
    pmap->pScreen = pScreen;
    pmap->pVisual = pVisual;
    pmap->class = class;

    /* allocate first (red) map */
    size = pVisual->ColormapEntries;
    pmap->freeRed = size;
    sizebytes = size * sizeof (Entry);
    if( (pmap->red = (EntryPtr) Xalloc(sizebytes)) == (EntryPtr) NULL)
	goto NoRedMap;
    bzero ((char *) pmap->red, sizebytes);

    if((pmap->clientPixelsRed =
        (Pixel **) Xalloc(MAXCLIENTS * sizeof(Pixel *))) == (Pixel **) NULL)
	goto NoClientPixelsRed;
    if((pmap->numPixelsRed =
        (int *) Xalloc(MAXCLIENTS * sizeof(int))) == (int *) NULL)
	goto NoNumPixelsRed;
    for(j = 0; j < MAXCLIENTS; j++)
    {
	(pmap->numPixelsRed)[j] = 0;
	(pmap->clientPixelsRed)[j] = (Pixel *) NULL;
    }
    if (alloc == AllocAll)
    {
	pmap->flags |= AllAllocated;
	for(pent = &(pmap->red[0]); pent < &pmap->red[pmap->freeRed]; pent++)
	{
	    pent->refcnt = AllocPrivate;
	}
	pmap->freeRed = 0;
	if((ppix = (Pixel *)Xalloc(size * sizeof(Pixel))) == (Pixel *) NULL)
	    goto NoAllocAll;
	(pmap->clientPixelsRed)[client] = ppix;
	for(i = 0; i < size; i++)
	    ppix[i] = i;
	(pmap->numPixelsRed)[client] = size;
    }

    if ((class | DynamicClass) == DirectColor)
    {
	pmap->freeGreen = size;
	pmap->freeBlue = size;
	

	if((pmap->green = (EntryPtr) Xalloc(sizebytes)) == (EntryPtr)NULL)
	    goto NoGreenMap;
	if((pmap->blue = (EntryPtr) Xalloc(sizebytes)) == (EntryPtr) NULL)
	    goto NoBlueMap;

	bzero ((char *) pmap->green, sizebytes);
	bzero ((char *) pmap->blue, sizebytes);

	if((pmap->clientPixelsGreen =
	    (Pixel **)Xalloc(MAXCLIENTS * sizeof(Pixel *))) == (Pixel **) NULL)
	    goto NoClientPixelsGreen;
	if((pmap->clientPixelsBlue =
	    (Pixel **)Xalloc(MAXCLIENTS * sizeof(Pixel *))) == (Pixel **)NULL)
	    goto NoClientPixelsBlue;
	if((pmap->numPixelsGreen = (int *) Xalloc(MAXCLIENTS * sizeof(int))) ==
		(int *) NULL)
	    goto NoNumPixelsGreen;
	if((pmap->numPixelsBlue = (int *) Xalloc(MAXCLIENTS * sizeof(int))) ==
		(int *) NULL)
	    goto NoNumPixelsBlue;

	for(j = 0; j < MAXCLIENTS; j++)
	{
	    (pmap->clientPixelsGreen)[j] = (Pixel *) NULL;
	    (pmap->clientPixelsBlue)[j] = (Pixel *) NULL;
	    (pmap->numPixelsGreen)[j] = 0;
	    (pmap->numPixelsBlue)[j] = 0;
	}
	/* If every cell is allocated, mark its refcnt */
	if (alloc == AllocAll)
	{
	    for(pent = &pmap->green[0]; pent < &pmap->green[size]; pent++)
	    {
		pent->refcnt = AllocPrivate;
	    }
	    for(pent = &pmap->blue[0]; pent < &pmap->blue[size]; pent++)
	    {
		pent->refcnt = AllocPrivate;
	    }
	    pmap->freeGreen = 0;
	    pmap->freeBlue = 0;

	    if((ppix = (Pixel *) Xalloc(size * sizeof(Pixel))) ==
	        (Pixel *) NULL)
		goto NoDirectAllocAll;
	    (pmap->clientPixelsGreen)[client] = ppix;
	    for(i = 0; i < size; i++)
		ppix[i] = i;
	    (pmap->numPixelsGreen)[client] = size;

	    if((ppix = (Pixel *) Xalloc(size * sizeof(Pixel))) ==
		(Pixel *) NULL)
		goto NoDirectAllocAll2;
	    (pmap->clientPixelsBlue)[client] = ppix;

	    for(i = 0; i < size; i++)
		ppix[i] = i;
	    (pmap->numPixelsBlue)[client] = size;
	}
    }
    /* If the device wants a chance to initialize the colormap in any way,
     * this is it.  In specific, if this is a Static colormap, this is the
     * time to fill in the colormap's values */
    pmap->flags |= BeingCreated;
    (*pScreen->CreateColormap)(pmap);
    pmap->flags &= ~BeingCreated;
    *ppcmap = pmap;
    return (Success);

    /* There are many different places where Xalloc might run out of memory.
     * Once we hit one of them, we need to free everything we had previously
     * alllocated.  Rather than repeat the code and make CreateColormap 
     * completely unreadable, I've collected it below.  On an error, controls
     * jumps into one of these entry points and then falls on down to free
     * everything previously allocated.
     */
NoDirectAllocAll2:
    Xfree((pmap->clientPixelsGreen)[client]);
NoDirectAllocAll:
NoNumPixelsBlue:
    Xfree(pmap->numPixelsGreen);
NoNumPixelsGreen:
    Xfree(pmap->clientPixelsBlue);
NoClientPixelsBlue:
    Xfree(pmap->clientPixelsGreen);
NoClientPixelsGreen:
    Xfree(pmap->blue);
NoAllocAll:
NoBlueMap:
    Xfree(pmap->green);
NoGreenMap:
    Xfree(pmap->numPixelsRed);
NoNumPixelsRed:
    Xfree(pmap->clientPixelsRed);
NoClientPixelsRed:
    Xfree(pmap->red);
NoRedMap:
    FreeResource(mid, RC_NONE);
NoMap:
    return (BadAlloc);
}

void
FreeColormap (pmap, client)
    ColormapPtr	pmap;
    int		client;
{
    unsigned long mid;
    int		i, size;

    mid = pmap->mid;

    /* Uninstalling the default colormap is a no-op */
    if (CLIENT_ID(client) != SERVER_ID && mid == pmap->pScreen->defColormap)
	return;

    if(CLIENT_ID(client) != SERVER_ID)
    {
        (*(pmap->pScreen->UninstallColormap)) (pmap);
        WalkTree(pmap->pScreen, TellNoMap, (char *) &mid);
    }

    /* This is the device's chance to undo anything it needs to, especially
     * to free any storage it allocated */
    (*pmap->pScreen->DestroyColormap)(pmap);

    if(pmap->clientPixelsRed)
    {
	for(i = 0; i < MAXCLIENTS; i++)
	{
	    Xfree((pmap->clientPixelsRed)[i]);
	}
	Xfree(pmap->clientPixelsRed);
	Xfree(pmap->numPixelsRed);
    }
    size = pmap->pVisual->ColormapEntries;
    for(i = 0; i < size; i++)
    {
	if(pmap->red[i].fShared)
	{
	    if(pmap->red[i].co.shco.red)
	    {
	        if(--pmap->red[i].co.shco.red->refcnt == 0)
		    Xfree(pmap->red[i].co.shco.red);
	    }
	    if(pmap->red[i].co.shco.green)
	    {
	        if(--pmap->red[i].co.shco.green->refcnt == 0)
	            Xfree(pmap->red[i].co.shco.green);
	    }
	    if(pmap->red[i].co.shco.blue)
	    {
	        if(--pmap->red[i].co.shco.blue->refcnt == 0)
	            Xfree(pmap->red[i].co.shco.blue);
	    }
	}
    }
    Xfree(pmap->red);
    if((pmap->class | DynamicClass) == DirectColor)
    {
        for(i = 0; i < MAXCLIENTS; i++)
	{
            Xfree((pmap->clientPixelsGreen)[i]);
            Xfree((pmap->clientPixelsBlue)[i]);
        }
	Xfree(pmap->clientPixelsGreen);
	Xfree(pmap->clientPixelsBlue);
	Xfree(pmap->numPixelsGreen);
	Xfree(pmap->numPixelsBlue);
	for(i = 0; i < size; i++)
	{
	    if(pmap->green[i].fShared)
	    {
		if(--pmap->green[i].co.shco.green->refcnt == 0)
		    Xfree(pmap->green[i].co.shco.green);
	    }
	 
	    if(pmap->blue[i].fShared)
	    {
		if(--pmap->blue[i].co.shco.blue->refcnt == 0)
		    Xfree(pmap->blue[i].co.shco.blue);
	    }
	}
        Xfree(pmap->green);
        Xfree(pmap->blue);
    }
    Xfree(pmap);
}

/* Tell window that pmid has disappeared */
int
TellNoMap (pwin, pmid)
    WindowPtr	pwin;
    int 	*pmid;
{
    xEvent 	xE;
    if (pwin->colormap == *( (int *) pmid))
    {
	/* This should be call to DeliverEvent */
	xE.u.u.type = ColormapNotify;
	xE.u.colormap.window = pwin->wid;
	xE.u.colormap.colormap = *pmid;
	xE.u.colormap.new = TRUE;
	xE.u.colormap.state = ColormapUninstalled;
	DeliverEvents(pwin, &xE, 1);
        pwin->colormap = None;
    }

    return (WT_WALKCHILDREN);
}

/* Tell window that it has inherited a new colormap */
int
TellNewMap (pwin, pxE)
    WindowPtr	pwin;
    xEvent 	*pxE;
{
    if (pwin->colormap == CopyFromParent)
    {
	/* This should be call to DeliverEvent */
	pxE->u.colormap.window = pwin->wid;
	DeliverEvents(pwin, pxE, 1);
    }

    return (WT_WALKCHILDREN);
}


  
/* Tell window that pmid got uninstalled */
int
TellLostMap (pwin, pmid)
    WindowPtr	pwin;
    int 	*pmid;
{
    xEvent 	xE;
    if (pwin->colormap == *( (int *) pmid))
    {
	/* This should be call to DeliverEvent */
	xE.u.u.type = ColormapNotify;
	xE.u.colormap.window = pwin->wid;
	xE.u.colormap.colormap = *pmid;
	xE.u.colormap.new = FALSE;
	xE.u.colormap.state = ColormapUninstalled;
	DeliverEvents(pwin, &xE, 1);
    }

    return (WT_WALKCHILDREN);
}

/* Tell window that pmid got installed */
int
TellGainedMap (pwin, pmid)
    WindowPtr	pwin;
    int 	*pmid;
{
    xEvent 	xE;
    if (pwin->colormap == *( (int *) pmid))
    {
	/* This should be call to DeliverEvent */
	xE.u.u.type = ColormapNotify;
	xE.u.colormap.window = pwin->wid;
	xE.u.colormap.colormap = *pmid;
	xE.u.colormap.new = FALSE;
	xE.u.colormap.state = ColormapInstalled;
	DeliverEvents(pwin, &xE, 1);
    }

    return (WT_WALKCHILDREN);
}

  
int
CopyColormapAndFree (mid, pSrc, client)
    int		mid;
    ColormapPtr	pSrc;
    int		client;
{
    ColormapPtr	pmap = (ColormapPtr) NULL;
    int		result, alloc, size, midSrc;
    EntryPtr	pent, pentSrc;
    ScreenPtr	pScreen;
    VisualPtr	pVisual;

    pScreen = pSrc->pScreen;
    pVisual = pSrc->pVisual;
    midSrc = pSrc->mid;
    alloc = ((pSrc->flags & AllAllocated) && CLIENT_ID(midSrc) == client) ?
            AllocAll : AllocNone;
    size = pVisual->ColormapEntries;

    /* If the create returns non-0, it failed */
    result =
      CreateColormap (mid, pScreen, pVisual, &pmap, alloc, client);
    if(pmap == (ColormapPtr) NULL)
        return(result);
    if(alloc == AllocAll)
    {
	/* In "The Tao of X", the master says:
	* What does the server [ . . .] do if the colormap passed in is one
	* that was created with alloc=AllocAll in CreateColormap?  The
	* semantics I think I want is that the new map is also AllocAll, and
	* all RGB contents are copied over, and the original map is changed
	* back to AllocNone with no entries allocated.
	*/
	pentSrc = &pSrc->red[0];
        for(pent = &(pmap->red[0]); pent < &pmap->red[size]; pent++)
	{
	    *pent = *pentSrc++;
	    if(pent->fShared)
	    {
		if(pent->co.shco.red)
		    pent->co.shco.red->refcnt++;
		if(pent->co.shco.green)
		    pent->co.shco.green->refcnt++;
		if(pent->co.shco.blue)
		    pent->co.shco.blue->refcnt++;
	    }
	}
	if((pmap->class | DynamicClass) == DirectColor)
	{
	    pentSrc = &pSrc->green[0];
	    for(pent = &(pmap->green[0]); pent < &pmap->green[size]; pent++)
	    {
		*pent = *pentSrc++;
	        if(pent->fShared)
		{
		    if(pent->co.shco.green)
			pent->co.shco.green->refcnt++;
		}
	    }
	    pentSrc = &pSrc->blue[0];
	    for(pent = &(pmap->blue[0]); pent < &pmap->blue[size]; pent++)
	    {
		*pent = *pentSrc++;
	        if(pent->fShared)
		{
		    if(pent->co.shco.blue)
			pent->co.shco.blue->refcnt++;
		}
	    }
	}
	/* We're going to "change" the original map back to AllocNone. The
	 * easiest way to do this is to delete the map and create a new one
	 * with the same id */
	FreeResource(midSrc, RC_NONE);
	CreateColormap(midSrc, pScreen, pVisual, &pmap, AllocNone, client);
	return(result);
    }

    CopyFree (REDMAP, client, pSrc, pmap);


    if (pmap->class == DirectColor)
    {
        CopyFree (GREENMAP, client, pSrc, pmap);
        CopyFree (BLUEMAP, client, pSrc, pmap);
    }
    return(result);
}

/* Helper routine for freeing large numbers of cells from a map */
void
CopyFree (channel, client, pmapSrc, pmapDst)
    int		channel, client;
    ColormapPtr	pmapSrc, pmapDst;
{
    int		z, npix;
    EntryPtr	pentSrcFirst, pentDstFirst;
    EntryPtr	pentSrc, pentDst;
    Pixel	*ppix;

    switch(channel)
    {
      case PSEUDOMAP:
      case REDMAP:
	ppix = (pmapSrc->clientPixelsRed)[client];
	npix = (pmapSrc->numPixelsRed)[client];
	pentSrcFirst = pmapSrc->red;
	pentDstFirst = pmapDst->red;
	break;
      case GREENMAP:
	ppix = (pmapSrc->clientPixelsGreen)[client];
	npix = (pmapSrc->numPixelsGreen)[client];
	pentSrcFirst = pmapSrc->green;
	pentDstFirst = pmapDst->green;
	break;
      case BLUEMAP:
	ppix = (pmapSrc->clientPixelsBlue)[client];
	npix = (pmapSrc->numPixelsBlue)[client];
	pentSrcFirst = pmapSrc->blue;
	pentDstFirst = pmapDst->blue;
	break;
    }
    for(z = npix; z-- > 0; ppix++)
    {
        /* Copy pixels */
        pentSrc = pentSrcFirst + *ppix;
        pentDst = pentDstFirst + *ppix;
        *pentDst = *pentSrc;

        if(pentSrc->refcnt ==  AllocPrivate)
	{
	    if(pentSrc->fShared)
	    {
		pentSrc->co.shco.red->refcnt++;
		if(pentSrc->co.shco.green)
		    pentSrc->co.shco.green->refcnt++;
		if(pentSrc->co.shco.blue)
		    pentSrc->co.shco.blue->refcnt++;
	    }
            pentSrc->refcnt = 0;
        }
    	FreeCell(pmapSrc, *ppix, channel);
    }

    /* Note that FreeCell has already fixed pmapSrc->free{Color} */
    switch(channel)
    {
      case PSEUDOMAP:
      case REDMAP:
        pmapDst->freeRed -= (pmapSrc->numPixelsRed)[client];
        (pmapDst->clientPixelsRed)[client] =
	    (pmapSrc->clientPixelsRed)[client];
        (pmapSrc->clientPixelsRed)[client] = (Pixel *) NULL;
        (pmapDst->numPixelsRed)[client] = (pmapSrc->numPixelsRed)[client];
        (pmapSrc->numPixelsRed)[client] = 0;
	break;
      case GREENMAP:
        pmapDst->freeGreen -= (pmapSrc->numPixelsGreen)[client];
        (pmapDst->clientPixelsGreen)[client] =
	    (pmapSrc->clientPixelsGreen)[client];
        (pmapSrc->clientPixelsGreen)[client] = (Pixel *) NULL;
        (pmapDst->numPixelsGreen)[client] = (pmapSrc->numPixelsGreen)[client];
        (pmapSrc->numPixelsGreen)[client] = 0;
	break;
      case BLUEMAP:
        pmapDst->freeBlue -= pmapSrc->numPixelsBlue[client];
        pmapDst->clientPixelsBlue[client] = pmapSrc->clientPixelsBlue[client];
        pmapSrc->clientPixelsBlue[client] = (Pixel *) NULL;
        pmapDst->numPixelsBlue[client] = pmapSrc->numPixelsBlue[client];
        pmapSrc->numPixelsBlue[client] = 0;
	break;
    }
}

/* Free the ith entry in a color map.  Must handle freeing of
 * colors allocated through AllocColorPlanes */
void
FreeCell (pmap, i, channel)
    ColormapPtr pmap;
    int	i, channel;
{
    EntryPtr pent;
    int	*pCount;


    switch (channel)
    {
      case PSEUDOMAP:
      case REDMAP:
	  {
          pent = (EntryPtr) &pmap->red[i];
	  pCount = &pmap->freeRed;
	  break;
	  }
      case GREENMAP:
	  {
          pent = (EntryPtr) &pmap->green[i];
	  pCount = &pmap->freeGreen;
	  break;
	  }
      case BLUEMAP:
	  {
          pent = (EntryPtr) &pmap->blue[i];
	  pCount = &pmap->freeBlue;
	  break;
	  }

    }
    /* If it's not privately allocated and it's not time to free it, just
     * decrement the count */
    if (pent->refcnt > 1)
	pent->refcnt--;
    else
    {
        /* If the color type is shared, find the sharedcolor. If decremented
         * refcnt would be 0, free the shared cell. */
        if (pent->fShared)
	{
	    if(pent->co.shco.red->refcnt == 1)
	    {
		Xfree(pent->co.shco.red);
	    }
	    else
		pent->co.shco.red->refcnt--;

	    if(pent->co.shco.green)
	    {
		if(pent->co.shco.green->refcnt == 1)
		{
		    Xfree(pent->co.shco.green);
		}
		else
		    pent->co.shco.green->refcnt--;
	    }

	    if(pent->co.shco.blue)
	    {
		if(pent->co.shco.blue->refcnt == 1)
		{
		    Xfree(pent->co.shco.blue);
		}
		else
		    pent->co.shco.blue->refcnt--;
	    }
	    
	    pent->fShared = 0;
	}
	pent->refcnt = 0;
	*pCount += 1;
    }
}


/* Get a read-only color from a ColorMap (probably slow for large maps)
 * Returns by changing the value in pred, pgreen, pblue and pPix
 * On Error sets Alloc
 */
int
AllocColor (pmap, pred, pgreen, pblue, pPix, client)
    ColormapPtr		pmap;
    unsigned short 	*pred, *pgreen, *pblue;
    Pixel		*pPix;
    int			client;
{
    Pixel	pixel, pixT;
    int		entries;
    xrgb	rgb;
    int		class;
    VisualPtr	pVisual;


    pVisual = pmap->pVisual;
    (*pmap->pScreen->ResolveColor) (pred, pgreen, pblue, pVisual);
    rgb.red = *pred;
    rgb.green = *pgreen;
    rgb.blue = *pblue;
    class = pmap->class;
    entries = pVisual->ColormapEntries;

    /* If the colormap is being created, then we want to be able to change
     * the colormap, even if it's a static type. Otherwise, we'd never be
     * able to initialize static colormaps
     */
    if(client == SERVER_ID && (pmap->flags & BeingCreated))
	class |= DynamicClass;

    /* If this is one of the static storage classes, and we're not initializing
     * it, the best we can do is to find the closest color entry to the
     * requested one and return that.
     */
    switch (class) {
    /* If this is StaticColor or StaticGray, look up all three components
     * in the same pmap */
    case StaticColor:
    case StaticGray:
	*pPix = FindBestPixel(pmap->red, entries, &rgb, PSEUDOMAP);
	return(Success);

    case TrueColor:
	/* Look up each component in its own map, then OR them together */
	pixel = FindBestPixel(pmap->red, entries, &rgb, REDMAP);
	*pPix |= pixel << pVisual->offsetRed;
	pixel = FindBestPixel(pmap->green, entries, &rgb, GREENMAP);
	*pPix |= pixel << pVisual->offsetGreen;
	pixel = FindBestPixel(pmap->blue, entries, &rgb, BLUEMAP);
	*pPix |= pixel << pVisual->offsetBlue;
	return(Success);

    case GrayScale:
    case PseudoColor:
        FindColor(pmap, pmap->red, entries, &rgb, pPix, PSEUDOMAP,
          client, AllComp);
	if(*pPix  == NoneFound)
	{
	    return (BadAlloc);
	}
        break;

    case DirectColor:
	pixT = 0;
	pixel = (*pPix & pVisual->redMask) >> pVisual->offsetRed; 
	FindColor(pmap, pmap->red, entries, &rgb, &pixel, REDMAP,
	  client, RedComp);
	if(pixel == NoneFound)
	{
	    return (BadAlloc);
	}
	else
	    pixT |= pixel;
	pixel = (*pPix & pVisual->greenMask) >> pVisual->offsetGreen; 
	FindColor(pmap, pmap->green, entries, &rgb, &pixel, GREENMAP,
	  client, GreenComp);
	if(pixel == NoneFound)
	{
	    return (BadAlloc);
	}
	else
	    pixT |= pixel;
	pixel = (*pPix & pVisual->blueMask) >> pVisual->offsetBlue; 
	FindColor(pmap, pmap->blue, entries, &rgb, &pixel, BLUEMAP,
	  client, BlueComp);
	if(pixel == NoneFound)
	{
	    return (BadAlloc);
	}
	else
	    pixT |= pixel;
	*pPix = pixT;
	break;
    }

    /* if this is the client's first pixel in this colormap, tell the
     * resource manager that the client has pixels in this colormap which
     * should be freed when the client dies */
    if ((pmap->numPixelsRed)[client] == 1)
    {
	colorResource	*pcr;

	pcr = (colorResource *) Xalloc(sizeof(colorResource));
	pcr->mid = pmap->mid;
	pcr->client = client;
	AddResource(
	   FakeClientID(client), RT_CMAPENTRY, pcr, FreeClientPixels, RC_CORE);
    }
    return (Success);
}


Pixel
FindBestPixel(pentFirst, size, prgb, channel)
    EntryPtr	pentFirst;
    int		size;
    xrgb	*prgb;
    int		channel;
{
    EntryPtr	pent;
    Pixel	pixel, final, minval, dr, dg, db, diff;

    final = 0;
    minval = ~0;
    /* look for the minimal difference */
    for (pent = pentFirst, pixel = 0; pixel < size; pent++, pixel++)
    {
	dr = dg = db = 0;
	switch(channel)
	{
	  case PSEUDOMAP:
	      dg = pent->co.local.green - prgb->green;
	      db = pent->co.local.blue - prgb->blue;
	  case REDMAP:
	      dr = pent->co.local.red - prgb->red;
	      break;
	  case GREENMAP:
	      dg = pent->co.local.green - prgb->green;
	      break;
	  case BLUEMAP:
	      db = pent->co.local.blue - prgb->blue;
	      break;
	}
	diff = dr * dr + dg * dg + db * db;
	if(diff < minval)
	{
	    final = pixel;
	    minval = diff;
	}
    }
    return(final);
}

/* Tries to find a color in pmap that exactly matches the one requested in prgb 
 * if it can't it allocates one.
 * Starts looking at pentFirst + *pPixel, so if you want a specific pixel,
 * load *pPixel with that value, otherwise set it to 0
 * Returns -1 on error, assuming that no one has a map THAT big */
static
Pixel
FindColor (pmap, pentFirst, size, prgb, pPixel, channel, client, comp)
    ColormapPtr	pmap;
    EntryPtr	pentFirst;
    int		size;
    xrgb	*prgb;
    Pixel	*pPixel;
    int		channel;
    int		client;
    int		(*comp) ();
{
    EntryPtr	pent;
    Pixel	pixel, free, freeShift;
    int		npix, count;
    Pixel	*ppix;
    xColorItem	def;

    free = NoneFound;

    if((pixel = *pPixel) > size)
	pixel = 0;
    /* see if there is a match, and also look for a free entry */
    for (pent = pentFirst + pixel, count = 0; count < size; count++)
    {
        if (pent->refcnt > 0)
	{
    	    if ((*comp) (pent, prgb))
	    {
    	        pent->refcnt++;
		switch(channel)
		{
		  case REDMAP:
		    pixel  <<= pmap->pVisual->offsetRed;
		  case PSEUDOMAP:
		    *pPixel = pixel;
		    break;
		  case GREENMAP:
		    pixel  <<= pmap->pVisual->offsetGreen;
		    *pPixel = pixel;
		    break;
		  case BLUEMAP:
		    pixel  <<= pmap->pVisual->offsetBlue;
		    *pPixel = pixel;
		    break;
		}
    	        return(Success);
    	    }
        }
	else if (free == NoneFound && pent->refcnt == 0)
	{
	    free = pixel;
	    /* If we're initializing the colormap, then we are looking for
	     * the first free cell we can find, not to minimize the number
	     * of entries we use.  So don't look any further. */
	    if(pmap->flags & BeingCreated)
		break;
	}
	if(++pixel > size)
	{
	    pent = pentFirst;
	    pixel = 0;
	}
	else
	    pent++;
    }

    /* If we got here, we didn't find a match.  If we also didn't find
     * a free entry, we're out of luck.  Otherwise, we'll usurp a free
     * entry and fill it in */
    if (free == NoneFound)
    {
	return (NoneFound);
    }
    pent = pentFirst + free;
    pent->fShared = FALSE;
    pent->refcnt = 1;

    def.flags = 0;
    switch (channel)
    {
      case PSEUDOMAP:
        pent->co.local.green = prgb->green;
        pent->co.local.blue = prgb->blue;
	def.green = prgb->green;
	def.blue = prgb->blue;
	def.flags |= DoGreen;
	def.flags |= DoBlue;
	/* For PseudoColor we load all three values for the pixel,
	 * but only put it in 1 map, the red one */

	/* So Fall through */
      case REDMAP:
        pent->co.local.red = prgb->red;
        def.red = prgb->red;
	def.flags |= DoRed;
	pmap->freeRed--;
	if(pmap->numPixelsRed)
	{
	    npix = (pmap->numPixelsRed)[client];
	    ppix = (Pixel *) Xrealloc ((char *)
		(pmap->clientPixelsRed)[client], (npix + 1) * sizeof(Pixel));
	    (pmap->clientPixelsRed)[client] = ppix;
	    (pmap->numPixelsRed)[client]++;
	}
	freeShift = free << pmap->pVisual->offsetRed;
	break;

      case GREENMAP:
	pent->co.local.green = prgb->green;
        def.green = prgb->green;
	def.flags |= DoGreen;
	pmap->freeGreen--;
	if(pmap->numPixelsGreen)
	{
	    npix = (pmap->numPixelsGreen)[client];
	    ppix = (Pixel *) Xrealloc ((char *)
		(pmap->clientPixelsGreen)[client], (npix + 1) * sizeof(Pixel));
	    (pmap->clientPixelsGreen)[client] = ppix;
	    (pmap->numPixelsGreen)[client]++;
	}
	freeShift = free << pmap->pVisual->offsetGreen;
	break;

      case BLUEMAP:
	pent->co.local.blue = prgb->blue;
	def.blue = prgb->blue;
	def.flags |= DoBlue;
	pmap->freeBlue--;
	if(pmap->numPixelsBlue)
	{
	    npix = (pmap->numPixelsBlue)[client];
	    ppix = (Pixel *) Xrealloc ((char *)
		(pmap->clientPixelsBlue)[client], (npix + 1) * sizeof(Pixel));
	    (pmap->clientPixelsBlue)[client] = ppix;
	    (pmap->numPixelsBlue)[client]++;
	}
	freeShift = free << pmap->pVisual->offsetBlue;
	break;
    }
    def.pixel = (channel == PSEUDOMAP) ? free : freeShift;
    ppix[npix] = free;
    (*pmap->pScreen->StoreColors) (pmap, 1, &def);
	
    *pPixel = def.pixel;
    return(Success);
}

/* Comparison functions -- passed to FindColor to determine if an
 * entry is already the color we're looking for or not */
int
AllComp (pent, prgb)
    EntryPtr	pent;
    xrgb	*prgb;
{
    if((pent->co.local.red == prgb->red) &&
       (pent->co.local.green == prgb->green) &&
       (pent->co.local.blue == prgb->blue) )
       return (1);
    return (0);
}

int
RedComp (pent, prgb)
    EntryPtr	pent;
    xrgb	*prgb;
{
    if (pent->co.local.red == prgb->red) 
	return (1);
    return (0);
}
int
GreenComp (pent, prgb)
    EntryPtr	pent;
    xrgb	*prgb;
{
    if (pent->co.local.green == prgb->green) 
	return (1);
    return (0);
}
int
BlueComp (pent, prgb)
    EntryPtr	pent;
    xrgb	*prgb;
{
    if (pent->co.local.blue == prgb->blue) 
	return (1);
    return (0);
}


/* Read the color value of a cell */

int
QueryColors (pmap, count, ppixIn, prgbList)
    ColormapPtr	pmap;
    int		count;
    Pixel	*ppixIn;
    xrgb	*prgbList;
{
    Pixel	*ppix, pixel;
    xrgb	*prgb;
    VisualPtr	pVisual;
    EntryPtr	pent;
    unsigned	i;
    int		errVal = Success;

    pVisual = pmap->pVisual;
    if ((pmap->class | DynamicClass) == DirectColor)
    {

	for( ppix = ppixIn, prgb = prgbList; count-- > 0; ppix++, prgb++)
	{
	    pixel = *ppix;
	    i  = (pixel & pVisual->redMask) >> pVisual->offsetRed;
	    if (i >= pVisual->ColormapEntries)
		errVal =  BadValue;
	    else
	    {
		prgb->red = RRED(&pmap->red[i]);


		i  = (pixel & pVisual->greenMask) >> pVisual->offsetGreen;
		if (i >= pVisual->ColormapEntries)
		    errVal =  BadValue;
		else
		{
		    prgb->green = RGREEN(&(pmap->green[i]));

		    i  = (pixel & pVisual->blueMask) >> pVisual->offsetBlue;
		    if (i >= pVisual->ColormapEntries)
			errVal =  BadValue;
		    else
			prgb->blue = RBLUE(&(pmap->blue[i]));
		}
	    }
	}
    }
    else
    {
	for( ppix = ppixIn, prgb = prgbList; count-- > 0; ppix++, prgb++)
	{
	    pixel = *ppix;
	    if (pixel >= pVisual->ColormapEntries)
		errVal = BadValue;
	    else
	    {
		pent = (EntryPtr)&pmap->red[pixel];
		prgb->red = RRED(pent);
		prgb->green = RGREEN(pent);
		prgb->blue = RBLUE(pent);
	    }
	}
    }
    return (errVal);
}

/* Free all of a client's colors and cells */
void
FreeClientPixels (pcr)
    colorResource *pcr;
{
    Pixel  			*ppix, *ppixStart;
    register ColormapPtr	pmap;
    register int 		client;
    register int 		n;
    int				class;

    /* if mid is no longer a resource, the colormap has already been freed
     * and we can all go home.
     */
    if((pmap = (ColormapPtr) LookupID(pcr->mid, RT_COLORMAP, RC_CORE)) ==
        (ColormapPtr) NULL)
    {
        Xfree((char *) pcr);
	return;
    }
    client = pcr->client;
    Xfree((char *) pcr);

    class = pmap->class;
    ppix = (pmap->clientPixelsRed)[client];
    ppixStart = ppix;
    n = (pmap->numPixelsRed)[client];
    while (n-- > 0)
    {
	FreeCell(pmap, *ppix++, PSEUDOMAP);
    }
    Xfree ((pointer) ppixStart);
    (pmap->clientPixelsRed)[client] = (Pixel *) NULL;
    (pmap->numPixelsRed)[client] = 0;
 
    if ((class | DynamicClass) == DirectColor) 
    {
        ppix = (pmap->clientPixelsGreen)[client];
	ppixStart = ppix;
	n = (pmap->numPixelsGreen)[client];
	while (n-- > 0)
	{
	    FreeCell(pmap, *ppix++, GREENMAP);
	}
	Xfree ((pointer) ppixStart);
	(pmap->clientPixelsGreen)[client] = (Pixel *) NULL;
	(pmap->numPixelsGreen)[client] = 0;

        ppix = (pmap->clientPixelsBlue)[client];
	ppixStart = ppix;
	n = (pmap->numPixelsBlue)[client];
	while (n-- > 0)
	{
	    FreeCell(pmap, *ppix++, BLUEMAP);
	}
	Xfree ((pointer) ppixStart);
	(pmap->clientPixelsBlue)[client] = (Pixel *) NULL;
	(pmap->numPixelsBlue)[client] = 0;
    }
}

int
AllocColorCells (client, pmap, colors, planes, contig, ppix, masks)
    int		client;
    ColormapPtr	pmap;
    int		colors, planes;
    Bool	contig;
    Pixel	*ppix;
    Pixel	*masks;
{
    Pixel	rmask, gmask, bmask, *ppixFirst;
    int		r, g, b, n, class;
    int		ok;

    class = pmap->class;
    if (class == StaticColor)
    {
	return (BadMatch); /* Shouldn't try on this type */
    }
    if (pmap->class == DirectColor)
    {
        ok = AllocDirect (client, pmap, colors, planes, planes, planes,
	     contig, ppix, &rmask, &gmask, &bmask);
        r = g = b = 0;
        for (n = 0; n < planes; n++)
	{
	    while(!(rmask & 1 << r))
		r++;
	    while(!(gmask & 1 << g))
		g++;
	    while(!(bmask & 1 << b))
		b++;
	    *masks++ = ((1 << r++) << pmap->pVisual->offsetRed) |
	               ((1 << g++) << pmap->pVisual->offsetGreen) |
		       ((1 << b++) << pmap->pVisual->offsetBlue);
	}
    }
    else
    {
        ok = AllocPseudo (client, pmap, colors, planes, contig, ppix, &rmask,
	 &ppixFirst);
        r = 0;
        for (n = 0; n < planes; n++)
	{
	    while(!(rmask & 1 << r))
		r++;
	    *masks++ = (1 << r++);
	}
    }
    return (ok);

}


int
AllocColorPlanes (client, pmap, colors, r, g, b, contig, pixels,
                 prmask, pgmask, pbmask)
    int		client;
    ColormapPtr	pmap;
    int		colors, r, g, b;
    Bool	contig;
    Pixel	*pixels;
    Pixel	*prmask, *pgmask, *pbmask;
{
    Bool	ok;
    Pixel	mask, *ppixFirst;
    register int		shift, i;
    int		class;

    class = pmap->class;
    if (class == StaticColor)
    {
	return (BadMatch); /* Shouldn't try on this type */
    }
    if (class == DirectColor)
        ok = AllocDirect (client, pmap, colors, r, g, b, contig, pixels,
	  prmask, pgmask, pbmask);
    else
    {
	/* Allocate the proper pixels */
	/* XXX This is sort of bad, because of contig is set, we force all
	 * r + g + b bits to be contiguous.  Should only force contiguity
	 * per mask 
	 */
        ok = AllocPseudo (client, pmap, colors, r + g + b, contig, pixels,
	  &mask, &ppixFirst);

	if(ok == Success)
	{
	    /* now split that mask into three */
	    *prmask = *pgmask = *pbmask = 0;
	    shift = 0;
	    for(i = 0; i < r; i++)
	    {
		while (!(mask & (1 << shift)))
		    shift++;
		*prmask |= (1 << shift++);
	    }
	    for(i = 0; i < g; i++)
	    {
		while (!(mask & (1 << shift)))
		    shift++;
		*pgmask |= (1 << shift++);
	    }
	    for(i = 0; i < b; i++)
	    {
		while (!(mask & (1 << shift)))
		    shift++;
		*pbmask |= (1 << shift++);
	    }

	    /* set up the shared color cells */
	    AllocShared(pmap, client, pixels, colors, r, g, b,
	                *prmask, *pgmask, *pbmask, ppixFirst);
	}
    }
    return (ok);
	

}

int
AllocDirect (client, pmap, c, r, g, b, contig, pixels, prmask, pgmask, pbmask)
    int		client;
    ColormapPtr	pmap;
    int		c, r, g, b;
    Bool	contig;
    Pixel	*pixels;
    Pixel	*prmask, *pgmask, *pbmask;
{
    Pixel	*ppixRed, *ppixGreen, *ppixBlue;
    Pixel	*ppix, *pDst, *p;
    int		npix;
    Bool	ok;

    /* start out with empty pixels */
    for(p = pixels; p < pixels + c; p++)
	*p = 0;

    if(!(ppixRed = (Pixel *)ALLOCATE_LOCAL((c << r) * sizeof(Pixel))))
	return(BadAlloc);
    ok = AllocCP(pmap, pmap->red, client, c, pmap->freeRed, r, contig,
                 ppixRed, prmask);
    *prmask <<= pmap->pVisual->offsetRed;

    if(!(ppixGreen = (Pixel *)ALLOCATE_LOCAL((c << g) * sizeof(Pixel))))
    {
	DEALLOCATE_LOCAL(ppixRed);
	return(BadAlloc);
    }
    ok = ok & AllocCP(pmap, pmap->green, client, c, pmap->freeGreen, g, contig,
                      ppixGreen, pgmask);
    *pgmask <<= pmap->pVisual->offsetGreen;

    if(!ok)
    {
	for(ppix = ppixRed, npix = 0; npix < c << r; npix++)
	    pmap->red[*ppix++].refcnt = 0;
	DEALLOCATE_LOCAL(ppixGreen);
	DEALLOCATE_LOCAL(ppixRed);
	return(BadAlloc);
    }

    if(!(ppixBlue = (Pixel *)ALLOCATE_LOCAL((c << b) * sizeof(Pixel))))
    {
	DEALLOCATE_LOCAL(ppixGreen);
	DEALLOCATE_LOCAL(ppixRed);
	return(BadAlloc);
    }
    ok = ok & AllocCP(pmap, pmap->blue, client, c, pmap->freeBlue, b, contig,
                      ppixBlue, pbmask);
    *pbmask <<= pmap->pVisual->offsetBlue;

    if (!ok)
    {
	for(ppix = ppixRed, npix = 0; npix < c << r; npix++)
	    pmap->red[*ppix++].refcnt = 0;
	for(ppix = ppixGreen, npix = 0; npix < c << g; npix++)
	    pmap->green[*ppix++].refcnt = 0;
	DEALLOCATE_LOCAL(ppixBlue);
	DEALLOCATE_LOCAL(ppixGreen);
	DEALLOCATE_LOCAL(ppixRed);
	return(BadAlloc);
    }
    else
    {
	npix = c << r;
	ppix = (Pixel *) Xrealloc(
	    (pointer)(pmap->clientPixelsRed)[client],
	    ((pmap->numPixelsRed)[client] + npix) * sizeof(Pixel));
	(pmap->clientPixelsRed)[client] = ppix;
	ppix += (pmap->numPixelsRed)[client];
	for (pDst = pixels, p = ppixRed; p < ppixRed + npix; p++)
	{
	    *ppix++ = *p;
	    if(p < ppixRed + c)
	        *pDst++ |= *p << pmap->pVisual->offsetRed;
	}
	(pmap->numPixelsRed)[client] += npix;
	pmap->freeRed -= npix;

	npix = c << g;
	ppix = (Pixel *) Xrealloc(
	    (pointer)(pmap->clientPixelsGreen)[client],
	    ((pmap->numPixelsGreen)[client] + npix) * sizeof(Pixel));
	(pmap->clientPixelsGreen)[client] = ppix;
	ppix += (pmap->numPixelsGreen)[client];
	for (pDst = pixels, p = ppixGreen; p < ppixGreen + npix; p++)
	{
	    *ppix++ = *p;
	    if(p < ppixGreen + c)
	        *pDst++ |= *p << pmap->pVisual->offsetGreen;
	}
	(pmap->numPixelsGreen)[client] += npix;
	pmap->freeGreen -= npix;

	npix = c << b;
	ppix = (Pixel *) Xrealloc(
	    (pointer)(pmap->clientPixelsBlue)[client],
	    ((pmap->numPixelsBlue)[client] + npix) * sizeof(Pixel));
	(pmap->clientPixelsBlue)[client] = ppix;
	ppix += (pmap->numPixelsBlue)[client];
	for (pDst = pixels, p = ppixBlue; p < ppixBlue + npix; p++)
	{
	    *ppix++ = *p;
	    if(p < ppixBlue + c)
	        *pDst++ |= *p << pmap->pVisual->offsetBlue;
	}
	(pmap->numPixelsBlue)[client] += npix;
	pmap->freeBlue -= npix;

    }
    DEALLOCATE_LOCAL(ppixBlue);
    DEALLOCATE_LOCAL(ppixGreen);
    DEALLOCATE_LOCAL(ppixRed);
    return (Success);
}

int
AllocPseudo (client, pmap, c, r, contig, pixels, pmask, pppixFirst)
    int		client;
    ColormapPtr	pmap;
    int		c, r;
    Bool	contig;
    Pixel	*pixels;
    Pixel	*pmask;
    Pixel	**pppixFirst;
{
    Pixel	*ppix, *p, *pDst, *ppixTemp;
    int		npix;
    int	result;

    npix = c << r;
    if(!(ppixTemp = (Pixel *)ALLOCATE_LOCAL(npix * sizeof(Pixel))))
	return(BadAlloc);
    result = AllocCP(pmap, pmap->red, client, c, pmap->freeRed, r, contig,
      ppixTemp, pmask);

    if (result)
    {

	/* all the allocated pixels are added to the client pixel list,
	 * but only the unique ones are returned to the client */
	ppix = (Pixel *) Xrealloc(
	    (pointer)(pmap->clientPixelsRed)[client],
	    ((pmap->numPixelsRed)[client] + npix) * sizeof(Pixel));
	(pmap->clientPixelsRed)[client] = ppix;
	ppix += (pmap->numPixelsRed)[client];
	*pppixFirst = ppix;
	pDst = pixels;
	for (p = ppixTemp; p < ppixTemp + npix; p++)
	{
	    *ppix++ = *p;
	    if(p < ppixTemp + c)
	        *pDst++ = *p;
	}
	(pmap->numPixelsRed)[client] += npix;
	pmap->freeRed -= npix;
    }
    DEALLOCATE_LOCAL(ppixTemp);
    return (result ? Success : BadAlloc);
}

/* Allocates count << planes pixels from colormap pmap for client. If
 * contig, then the plane mask is made of consecutive bits.  Returns
 * all count << pixels in the array pixels. The first count of those
 * pixels are the unique pixels.  *pMask has the mask to Or with the
 * unique pixels to get the rest of them.
 *
 * Returns True iff all pixels could be allocated 
 * All cells allocated will have refcnt set to AllocPrivate and shared to FALSE
 * (see AllocShared for why we care)
 */
int
AllocCP (pmap, pentFirst, client, count, free, planes, contig, pixels, pMask)
    ColormapPtr	pmap;
    EntryPtr	pentFirst;
    int		client, count, free, planes;
    Bool	contig;
    Pixel	*pixels, *pMask;
    
{
    EntryPtr	ent;
    long	pixel;
    int		dplanes, base, found, entries, maxp, save;
    Pixel	*ppix;
    Pixel	mask;
    Pixel	finalmask;

    dplanes = pmap->pVisual->nplanes;

    /* Easy case.  Allocate pixels only */
    if (planes == 0)
    {
        if (count == 0 || count > free)
    	    return (FALSE);

        /* allocate writable entries */
	ppix = pixels;
        ent = pentFirst;
        pixel = 0;
        while (--count >= 0)
	{
            /* Just find count unallocated cells */
    	    while (ent->refcnt)
	    {
    	        ent++;
    	        pixel++;
    	    }
    	    ent->refcnt = AllocPrivate;
    	    *ppix++ = pixel;
	    ent->fShared = FALSE;
        }
        *pMask = 0;
        return (TRUE);
    }
    else if ( count <= 0  || planes >= dplanes ||
      (count << planes) > free)
    {
	return (FALSE);
    }

    /* General case count pixels * 2 ^ planes cells to be allocated */

    /* make room for new pixels */
    ent = pentFirst;

    /* first try for contiguous planes, since it's fastest */
    for (mask = (1 << planes) - 1, base = 1, dplanes -= (planes - 1);
         --dplanes >= 0;
         mask += mask, base += base)
    {
        ppix = pixels;
        found = 0;
        pixel = 0;
        entries = pmap->pVisual->ColormapEntries - mask;
        while (pixel < entries)
	{
    	    save = pixel;
    	    maxp = pixel + mask + base;
    	    /* check if all are free */
    	    while (pixel != maxp && ent[pixel].refcnt == 0)
    	        pixel += base;
	    if (pixel == maxp)
		{
		    /* this one works */
		    *ppix++ = save;
		    found++;
		    if (found == count)
		    {
			/* found enough, allocate them all */
			while (--count >= 0)
			{
			    pixel = pixels[count];
			    maxp = pixel + mask;
			    while (1)
			    {
				ent[pixel].refcnt = AllocPrivate;
				ent[pixel].fShared = FALSE;
		    		ent[pixel].co.shco.red = (SHAREDCOLOR *) NULL;
		    		ent[pixel].co.shco.green =(SHAREDCOLOR *)NULL;
		    		ent[pixel].co.shco.blue = (SHAREDCOLOR *)NULL;
				if (pixel == maxp)
				    break;
				pixel += base;
				*ppix++ = pixel;
			    }
			}
			*pMask = mask;
			return (TRUE);
		    }
		}
    	    pixel = save + 1;
    	    if (pixel & mask)
    	        pixel += mask;
        }
    }

    dplanes = pmap->pVisual->nplanes;
    if (contig || planes == 1 || dplanes < 3)
        {
	return (FALSE);
	}

    /* this will be very slow for large maps, need a better algorithm */

    /*
       we can generate the smallest and largest numbers that fits in dplanes
       bits and contain exactly planes bits set as follows. First, we need to
       check that it is possible to generate such a mask at all.
       (Non-contiguous masks need one more bit than contiguous masks). Then
       the smallest such mask consists of the rightmost planes-1 bits set, then
       a zero, then a one in position planes + 1. The formula is
         (0x11 << (planes-1)) -1
       The largest such masks consists of the leftmost planes-1 bits set, then
       a zero, then a one bit in position dplanes-planes-1. If dplanes is
       smaller than 32 (the number of bits in a word) then the formula is:
         (1<<dplanes) - (1<<(dplanes-planes+1) + (1<<dplanes-planes-1)
       If dplanes = 32, then we can't calculate (1<<dplanes) and we have
       to use:
         ( (1<<(planes-1)) - 1) << (dplanes-planes+1) + (1<<(dplanes-planes-1))
	  
	  << Thank you, Loretta>>>

    */

    finalmask =
        (1<<((planes-1) - 1) << (dplanes-planes+1)) + (1<<(dplanes-planes-1));
    for (mask = (0x11 << (planes -1) - 1); mask <= finalmask; mask++)
    {
        /* next 3 magic statements count number of ones (HAKMEM #169) */
        pixel = (mask >> 1) & 033333333333;
        pixel = mask - pixel - ((pixel >> 1) & 033333333333);
        if ((((pixel + (pixel >> 3)) & 030707070707) % 077) != planes)
    	    continue;
        ppix = pixels;
        found = 0;
        entries = pmap->pVisual->ColormapEntries - mask;
        base = 1 << (ffs(mask) - 1);
        for (pixel = 0; pixel < entries; pixel++)
	{
	    if (pixel & mask)
	        continue;
	    maxp = 0;
	    /* check if all are free */
	    while (ent[pixel + maxp].refcnt == 0)
	    {
		GetNextBitsOrBreak(maxp, mask, base);
	    }
	    if (maxp <= mask)
		continue;
	    /* this one works */
	    *ppix++ = pixel;
	    found++;
	    if (found < count)
		continue;
	    /* found enough, allocate them all */
	    while (--count >= 0)
	    {
		pixel = (pixels)[count];
		maxp = 0;
		while (1)
		{
		    ent[pixel + maxp].refcnt = AllocPrivate;
		    ent[pixel + maxp].fShared = FALSE;
		    ent[pixel + maxp].co.shco.red = (SHAREDCOLOR *) NULL;
		    ent[pixel + maxp].co.shco.green = (SHAREDCOLOR *) NULL;
		    ent[pixel + maxp].co.shco.blue = (SHAREDCOLOR *) NULL;
		    GetNextBitsOrBreak(maxp, mask, base);
		    *ppix++ = pixel + maxp;
		}
	    }

	    *pMask = mask;
	    return (TRUE);
	}
    }
    pixels = pMask = (Pixel *)NULL;
    return (BadAlloc);
}

/* find last set bit */
int
fls (l)
    CARD32	l;
{
    int		shft;

    shft = 32 - 1;		/* 32 = bits per CARD32 */
    while(!(l & (1L << shft)))
	shft--;
    return(shft);
    
}

void
AllocShared (pmap, client, ppix, c, r, g, b, rmask, gmask, bmask, ppixFirst)
    ColormapPtr	pmap;
    int		client;
    Pixel	*ppix;
    int		c, r, g, b;
    Pixel	rmask, gmask, bmask;
    Pixel	*ppixFirst;	/* First of the client's new pixels */
{
    Pixel	*pptr, *cptr;
    Pixel	basemask;	/* bits not used in any mask */
    int		npix, z, npixClientNew;
    Pixel	base, bits;
    SHAREDCOLOR *pshared;

    basemask = ~(rmask | gmask | bmask);
    npixClientNew = c << (r + g + b);

    for(pptr = ppix, npix = 0; npix < c ; npix++, pptr++)
    {
	bits = 0;
	base = 1 << (ffs(rmask) - 1);
	while(1)
	{
	    pshared = (SHAREDCOLOR *) Xalloc (sizeof(SHAREDCOLOR));
	    pshared->refcnt = 1 << (g + b);
	    for (cptr = ppixFirst, z = npixClientNew; z-- > 0; cptr++)
	    {
		if( ((*cptr & basemask) == ((*pptr | bits) & basemask)) &&
		    ((*cptr & rmask) == ((*pptr | bits) & rmask)) &&
		    pmap->red[*cptr].co.shco.red == (SHAREDCOLOR *) NULL) 
		{
		    pmap->red[*cptr].fShared = TRUE;
		    pmap->red[*cptr].co.shco.red = pshared;
		}
	    }
	    GetNextBitsOrBreak(bits, rmask, base);
	}

	bits = 0;
	base = 1 << (ffs(gmask) - 1);
	while(1)
	{
	    pshared = (SHAREDCOLOR *) Xalloc (sizeof(SHAREDCOLOR));
	    pshared->refcnt = 1 << (r + b);
	    for (cptr = ppixFirst, z = npixClientNew; z-- > 0; cptr++)
	    {
		if( ((*cptr & basemask) == ((*pptr | bits) & basemask)) &&
		    ((*cptr & gmask) == ((*pptr | bits) & gmask)) &&
		    pmap->red[*cptr].co.shco.green ==(SHAREDCOLOR *)NULL) 
		{
		    pmap->red[*cptr].co.shco.green = pshared;
		}
	    }
	    GetNextBitsOrBreak(bits, gmask, base);

	}

	bits = 0;
	base = 1 << (ffs(bmask) - 1);
	while(1)
	{
	    pshared = (SHAREDCOLOR *) Xalloc (sizeof(SHAREDCOLOR));
	    pshared->refcnt = 1 << (r + g);
	    for (cptr = ppixFirst, z = npixClientNew; z-- > 0; cptr++)
	    {
		if( ((*cptr & basemask) == ((*pptr | bits) & basemask)) &&
		    ((*cptr & bmask) == ((*pptr | bits) & bmask)) &&
		    pmap->red[*cptr].co.shco.blue ==(SHAREDCOLOR *) NULL) 
		{
		    pmap->red[*cptr].co.shco.blue = pshared;
		}
	    }
	    GetNextBitsOrBreak(bits, bmask, base);
	}

    }
}


/* Free colors and/or cells (probably slow for large numbers) */

FreeColors (pmap, client, count, pixels, mask)
    ColormapPtr	pmap;
    int		client, count;
    Pixel	*pixels;
    Pixel	mask;
{
    int		rval, result, class;


    class = pmap->class;
    if((pmap->flags & AllocAll) || !(class & DynamicClass))
    {
	return(BadAccess);
    }
    if (class == DirectColor)
    {
        result = FreeCo(pmap, client, REDMAP, count, pixels, mask);
	/* If any of the three calls fails, we must report that, if more
	 * than one fails, it's ok that we report the last one */
        rval = FreeCo(pmap, client, GREENMAP, count, pixels, mask);
	if(rval != Success)
	    result = rval;
	rval = FreeCo(pmap, client, BLUEMAP, count, pixels, mask);
	if(rval != Success)
	    result = rval;
    }
    else
        result = FreeCo(pmap, client, PSEUDOMAP, count, pixels, mask);

    return (result);
}

/* Helper for FreeColors -- frees all combinations of *newpixels and mask bits
 * which the client has allocated in channel colormap cells of pmap.
 * doesn't change newpixels if it doesn't need to */
int
FreeCo (pmap, client, color, npixIn, ppixIn, mask)
    ColormapPtr	pmap;		/* which colormap head */
    int		client;		
    int		color;		/* which sub-map, eg RED, BLUE, PSEUDO */
    int		npixIn;		/* number of pixels passed in */
    Pixel	*ppixIn;	/* list of base pixels */
    Pixel	mask;		/* mask client gave us */ 
{

    Pixel	*ppixClient, pixTest;
    int		npixClient, npixNew, npix;
    unsigned	bits, base;
    Pixel	*pptr, *cptr;
    int 	n, zapped;
    int		errVal = Success;
    int		cmask, offset;

    if (npixIn == 0)
        return (errVal);
    bits = 0;
    zapped = 0;
    base = 1 << (ffs(mask) - 1);

    switch(color)
    {
      case REDMAP:
	cmask = pmap->pVisual->redMask;
	offset = pmap->pVisual->offsetRed;
	ppixClient = (pmap->clientPixelsRed)[client];
	npixClient = (pmap->numPixelsRed)[client];
	break;
      case GREENMAP:
	cmask = pmap->pVisual->greenMask;
	offset = pmap->pVisual->offsetGreen;
	ppixClient = (pmap->clientPixelsGreen)[client];
	npixClient = (pmap->numPixelsGreen)[client];
	break;
      case BLUEMAP:
	cmask = pmap->pVisual->blueMask;
	offset = pmap->pVisual->offsetBlue;
	ppixClient = (pmap->clientPixelsBlue)[client];
	npixClient = (pmap->numPixelsBlue)[client];
	break;
      case PSEUDOMAP:
	cmask = ~0;
	offset = 0;
	ppixClient = (pmap->clientPixelsRed)[client];
	npixClient = (pmap->numPixelsRed)[client];
	break;
    }

    /* zap all pixels which match */
    while (1)
    {
        /* go through pixel list */
        for (pptr = ppixIn, n = npixIn; --n >= 0; pptr++)
	{
	    pixTest = ((*pptr | bits) & cmask) >> offset;
	    if (pixTest >= pmap->pVisual->ColormapEntries)
	    {
		errVal = BadValue;
		continue;
	    }

	    /* find match in client list */
	    for (cptr = ppixClient, npix = npixClient;
	         --npix >= 0 && *cptr != pixTest;
		 cptr++) ;

	    if (npix >= 0)
	    {
		FreeCell(pmap, pixTest, color);
		*cptr = -1;
		zapped++;
	    }
	    else
		errVal = BadAccess;
	}
        /* generate next bits value */
	GetNextBitsOrBreak(bits, mask, base);
    }


    /* delete freed pixels from client pixel list */
    if (zapped)
    {
        npixNew = npixClient - zapped;
        if (npixNew)
	{
	    /* Since the list can only get smaller, we can do a copy in
	     * place and then realloc to a smaller size */
    	    pptr = cptr = ppixClient;

	    /* If we have all the new pixels, we don't have to examine the
	     * rest of the old ones */
	    for(npix = 0; npix < npixNew; cptr++)
	    {
    	        if (*cptr != -1)
		{
    		    *pptr++ = *cptr;
		    npix++;
    	        }
    	    }
	    ppixClient = (Pixel *)Xrealloc(ppixClient, npixNew * sizeof(Pixel));
	    npixClient = npixNew;
        }
	else
	{
	    npixClient = 0;
    	    ppixClient = (Pixel *)NULL;
	}
	switch(color)
	{
	  case PSEUDOMAP:
	  case REDMAP:
	    (pmap->clientPixelsRed)[client] = ppixClient;
	    (pmap->numPixelsRed)[client] = npixClient;
	    break;
	  case GREENMAP:
	    (pmap->clientPixelsGreen)[client] = ppixClient;
	    (pmap->numPixelsGreen)[client] = npixClient;
	    break;
	  case BLUEMAP:
	    (pmap->clientPixelsBlue)[client] = ppixClient;
	    (pmap->numPixelsBlue)[client] = npixClient;
	    break;
	}
    }
    return (errVal);
}



/* Redefine color values */
int
StoreColors (pmap, count, defs)
    ColormapPtr	pmap;
    int		count;
    xColorItem	*defs;
{
    register int 	pix;
    register xColorItem *pdef;
    register EntryPtr 	pent, pentT, pentLast;
    register VisualPtr	pVisual;
    SHAREDCOLOR		*pred, *pgreen, *pblue;
    int			n, ChgRed, ChgGreen, ChgBlue, idef;
    int			class, errVal = Success;
    int			ok;


    class = pmap->class;
    pVisual = pmap->pVisual;

    idef = 0;
    if(class == DirectColor)
    {
        for (pdef = defs, n = 0; n < count; pdef++, n++)
	{
	    ok = TRUE;
            (*pmap->pScreen->ResolveColor)
	        (&pdef->red, &pdef->green, &pdef->blue, pmap->pVisual);

	    pix = (pdef->pixel & pVisual->redMask) >> pVisual->offsetRed;
	    if (pix >= pVisual->ColormapEntries )
	    {
		errVal = BadValue;
		ok = FALSE;
	    }
	    else if (pmap->red[pix].refcnt != AllocPrivate &&
		     pmap->red[pix].refcnt != 1)
	    {
		errVal = BadAccess;
		ok = FALSE;
	    }
	    else
	    {
	        pent = &pmap->red[pix];
		if(pdef->flags & DoRed)
		{
		    if(pent->fShared)
		    {
			if(pent->co.shco.red)
			    pent->co.shco.red->color = pdef->red;
		    }
		    else
			pent->co.local.red = pdef->red;
		}
	    }

	    pix = (pdef->pixel & pVisual->greenMask) >> pVisual->offsetGreen;
	    if (pix >= pVisual->ColormapEntries )
	    {
		errVal = BadValue;
		ok = FALSE;
	    }
	    else if (pmap->green[pix].refcnt != AllocPrivate &&
	             pmap->green[pix].refcnt != 1)
	    {
		errVal = BadAccess;
		ok = FALSE;
	    }
	    else
	    {
	        pent = &pmap->green[pix];
		if(pdef->flags & DoGreen)
		{
		    if(pent->fShared)
		    {
			if(pent->co.shco.green)
			    pent->co.shco.green->color = pdef->green;
		    }
		    else
			pent->co.local.green = pdef->green;
		}
	    }

	    pix = (pdef->pixel & pVisual->blueMask) >> pVisual->offsetBlue;
	    if (pix >= pVisual->ColormapEntries )
	    {
		errVal = BadValue;
		ok = FALSE;
	    }
	    else if (pmap->red[pix].refcnt != AllocPrivate &&
	             pmap->red[pix].refcnt != 1)
	    {
		errVal = BadAccess;
		ok = FALSE;
	    }
	    else
	    {
	        pent = &pmap->blue[pix];
		if(pdef->flags & DoBlue)
		{
		    if(pent->fShared)
		    {
			if(pent->co.shco.blue)
			    pent->co.shco.blue->color = pdef->blue;
		    }
		    else
			pent->co.local.blue = pdef->blue;
		}
	    }
	    /* If this is an o.k. entry, then it gets added to the list
	     * to be sent to the hardware.  If not, skip it.  Once we've
	     * skipped one, we have to copy all the others.
	     */
	    if(ok)
	    {
		if(idef != n)
		    defs[idef] = defs[n];
		idef++;
	    }
	}
    }
    else
    {
        for (pdef = defs, n = 0; n < count; pdef++, n++)
	{

	    ok = TRUE;
	    if (pdef->pixel >= pVisual->ColormapEntries)
	    {
	        errVal = BadValue;
		ok = FALSE;
	    }
	    else if (pmap->red[pdef->pixel].refcnt != AllocPrivate &&
	             pmap->red[pdef->pixel].refcnt != 1)
	    {
		errVal = BadAccess;
		ok = FALSE;
	    }

	    /* If this is an o.k. entry, then it gets added to the list
	     * to be sent to the hardware.  If not, skip it.  Once we've
	     * skipped one, we have to copy all the others.
	     */
	    if(ok)
	    {
		if(idef != n)
		    defs[idef] = defs[n];
		idef++;
	    }
	    else
		continue;

            (*pmap->pScreen->ResolveColor)
	        (&pdef->red, &pdef->green, &pdef->blue, pmap->pVisual);

	    pent = &pmap->red[pdef->pixel];

	    if(pdef->flags & DoRed)
	    {
		if(pent->fShared)
		{
		    if(pent->co.shco.red)
			pent->co.shco.red->color = pdef->red;
		}
		else
		    pent->co.local.red = pdef->red;
	    }
	    if(pdef->flags & DoGreen)
	    {
		if(pent->fShared)
		{
		    if(pent->co.shco.green)
			pent->co.shco.green->color = pdef->green;
		}
		else
		    pent->co.local.green = pdef->green;
	    }
	    if(pdef->flags & DoBlue)
	    {
		if(pent->fShared)
		{
		    if(pent->co.shco.blue)
			pent->co.shco.blue->color = pdef->blue;
		}
		else
		    pent->co.local.blue = pdef->blue;
	    }

	    if(pent->fShared == TRUE)
	    {
                /* have to run through the colormap and change anybody who
		 * shares this value */
	        pred = pent->co.shco.red;
	        pgreen = pent->co.shco.green;
	        pblue = pent->co.shco.blue;
	        ChgRed = pdef->flags & DoRed;
	        ChgGreen = pdef->flags & DoGreen;
	        ChgBlue = pdef->flags & DoBlue;
	        pentLast = pmap->red + pVisual->ColormapEntries;

	        for(pentT = pmap->red; pentT < pentLast; pentT++)
		{
		    if(pentT->fShared == TRUE && pentT != pent)
		    {
			xColorItem	defChg;

			/* There are, alas, devices in this world too dumb
			 * to read their own hardware colormaps.  Sick, but
			 * true.  So we're going to be really nice and load
			 * the xColorItem with the proper value for all the
			 * fields.  We will only set the flags for those
			 * fields that actually change.  Smart devices can
			 * arrange to change only those fields.  Dumb devices
			 * can rest assured that we have provided for them,
			 * and can change all three fields */

			defChg.flags = 0;
			if(ChgRed && pentT->co.shco.red == pred)
			{
			    defChg.flags |= DoRed;
			}
			if(ChgGreen && pentT->co.shco.green == pgreen)
			{
			    defChg.flags |= DoGreen;
			}
			if(ChgBlue && pentT->co.shco.blue == pblue)
			{
			    defChg.flags |= DoBlue;
			}
			if(defChg.flags != 0)
			{
			    defChg.pixel = pentT - pmap->red;
			    defChg.red = pentT->co.shco.red->color;
			    defChg.green = pentT->co.shco.green->color;
			    defChg.blue = pentT->co.shco.blue->color;
			    (*(pmap->pScreen->StoreColors)) (pmap, 1, &defChg);
			}
		    }
		}

	    }
	}
    }
    /* Note that we use idef, the count of acceptable entries, and not
     * count, the count of proposed entries */
    ( *(pmap->pScreen->StoreColors)) (pmap, idef, defs);
    return (errVal);
}

int
IsMapInstalled(map, pWin)
    Colormap	map;
    WindowPtr	pWin;
{
    WindowPtr	pWinT;
    Colormap	*pmaps;
    int		imap, nummaps, found;

    if(pWin->drawable.pScreen->maxInstalledCmaps == 0)
	return(FALSE);

    /*  Find a real map id */
    pWinT = pWin;
    while(map == CopyFromParent)
    {
	if(pWinT->parent)
	{
	    if(pWinT->parent->colormap != CopyFromParent)
	    {
		map = pWinT->parent->colormap;
	    }
	    else
		pWinT = pWinT->parent;
	}
	else
	    return(FALSE);
    }
    pmaps = (Colormap *) ALLOCATE_LOCAL( 
             pWin->drawable.pScreen->maxInstalledCmaps * sizeof(Colormap));
    if(!pmaps)
	return(FALSE);
    nummaps = (*pWin->drawable.pScreen->ListInstalledColormaps)
        (pWin->drawable.pScreen, pmaps);
    found = FALSE;
    for(imap = 0; imap < nummaps; imap++)
    {
	if(pmaps[imap] == map)
	{
	    found = TRUE;
	    break;
	}
    }
    DEALLOCATE_LOCAL(pmaps);
    return (found);
}
