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
/* $Header: mfbscrinit.c,v 1.52 87/09/07 19:08:52 toddb Exp $ */

#include "X.h"
#include "Xproto.h"	/* for xColorItem */
#include "Xmd.h"
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "resource.h"
#include "colormap.h"
#include "mfb.h"
#include "mistruct.h"
#include "dix.h"

#include "servermd.h"

static void
mfbFreeVisual(p, id)
    pointer p;
    int id;
{
    Xfree(p);
}

/* dts * (inch/dot) * (25.4 mm / inch) = mm */
Bool
mfbScreenInit(index, pScreen, pbits, xsize, ysize, dpix, dpiy)
    int index;
    register ScreenPtr pScreen;
    pointer pbits;		/* pointer to screen bitmap */
    int xsize, ysize;		/* in pixels */
    int dpix, dpiy;		/* dots per inch */
{
    DepthPtr	pDepth;
    VisualPtr	pVisual;
    long	*pVids;
    register PixmapPtr pPixmap;

    pScreen->myNum = index;
    pScreen->width = xsize;
    pScreen->height = ysize;
    pScreen->mmWidth = (xsize * 254) / (dpix * 10);
    pScreen->mmHeight = (ysize * 254) / (dpiy * 10);
    pScreen->numDepths = 1;
    pScreen->allowedDepths = pDepth = (DepthPtr) Xalloc(sizeof(DepthRec));

    pScreen->rootDepth = 1;
    pScreen->rootVisual = FakeClientID(0);
    pScreen->defColormap = (Colormap) FakeClientID(0);
    pScreen->minInstalledCmaps = 1;
    pScreen->maxInstalledCmaps = 1;
    pScreen->whitePixel = 1;
    pScreen->blackPixel = 0;
    pScreen->backingStoreSupport = NotUseful;
    pScreen->saveUnderSupport = NotUseful;

    /* cursmin and cursmax are device specific */

    pScreen->numVisuals = 1;
    pScreen->visuals = pVisual = (VisualPtr) Xalloc(sizeof (VisualRec));

    pPixmap = (PixmapPtr )Xalloc(sizeof(PixmapRec));
    pPixmap->drawable.type = DRAWABLE_PIXMAP;
    pPixmap->drawable.depth = 1;
    pPixmap->drawable.pScreen = pScreen;
    pPixmap->drawable.serialNumber = 0;
    pPixmap->width = xsize;
    pPixmap->height = ysize;
    pPixmap->refcnt = 1;
    pPixmap->devPrivate = pbits;
    pPixmap->devKind = PixmapBytePad(xsize, 1);
    pScreen->devPrivate = (pointer)pPixmap;

    /* anything that mfb doesn't know about is assumed to be done
       elsewhere.  (we put in no-op only for things that we KNOW
       are really no-op.
    */
    pScreen->CreateWindow = mfbCreateWindow;
    pScreen->DestroyWindow = mfbDestroyWindow;
    pScreen->PositionWindow = mfbPositionWindow;
    pScreen->ChangeWindowAttributes = mfbChangeWindowAttributes;
    pScreen->RealizeWindow = mfbMapWindow;
    pScreen->UnrealizeWindow = mfbUnmapWindow;

    pScreen->RealizeFont = mfbRealizeFont;
    pScreen->UnrealizeFont = mfbUnrealizeFont;
    pScreen->GetImage = mfbGetImage;
    pScreen->GetSpans = mfbGetSpans;
    pScreen->CreateGC = mfbCreateGC;
    pScreen->CreatePixmap = mfbCreatePixmap;
    pScreen->DestroyPixmap = mfbDestroyPixmap;
    pScreen->ValidateTree = miValidateTree;

    pScreen->InstallColormap = mfbInstallColormap;
    pScreen->UninstallColormap = mfbUninstallColormap;
    pScreen->ListInstalledColormaps = mfbListInstalledColormaps;
    pScreen->StoreColors = NoopDDA;

    pScreen->RegionCreate = miRegionCreate;
    pScreen->RegionCopy = miRegionCopy;
    pScreen->RegionDestroy = miRegionDestroy;
    pScreen->Intersect = miIntersect;
    pScreen->Inverse = miInverse;
    pScreen->Union = miUnion;
    pScreen->Subtract = miSubtract;
    pScreen->RegionReset = miRegionReset;
    pScreen->TranslateRegion = miTranslateRegion;
    pScreen->RectIn = miRectIn;
    pScreen->PointInRegion = miPointInRegion;
    pScreen->WindowExposures = miWindowExposures;
    pScreen->RegionNotEmpty = miRegionNotEmpty;
    pScreen->RegionEmpty = miRegionEmpty;
    pScreen->RegionExtents = miRegionExtents;

    pScreen->BlockHandler = NoopDDA;
    pScreen->WakeupHandler = NoopDDA;
    pScreen->blockData = (pointer)0;
    pScreen->wakeupData = (pointer)0;

    pVisual->vid = pScreen->rootVisual;
    pVisual->screen = index;
    pVisual->class = StaticGray;
    pVisual->redMask = 0;
    pVisual->greenMask = 0;
    pVisual->blueMask = 0;
    pVisual->bitsPerRGBValue = 1;
    pVisual->ColormapEntries = 2;

    pDepth->depth = 1;
    pDepth->numVids = 1;
    pDepth->vids = pVids = (long *) Xalloc(sizeof (long));
    pVids[0] = pScreen->rootVisual;	/* our one and only visual */
    AddResource(
	pScreen->rootVisual, RT_VISUALID, pVisual, mfbFreeVisual, RC_CORE);

    /* we MIGHT return 0 if we had been keeping track of potential
       allocation failures.  one day someone will clean this up.
    */
    return 1;
}

