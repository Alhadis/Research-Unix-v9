/************************************************************
Copyright 1987 by Sun Microsystems, Inc. Mountain View, CA.

                    All Rights Reserved

Permission  to  use,  copy,  modify,  and  distribute   this
software  and  its documentation for any purpose and without
fee is hereby granted, provided that the above copyright no-
tice  appear  in all copies and that both that copyright no-
tice and this permission notice appear in  supporting  docu-
mentation,  and  that the names of Sun or MIT not be used in
advertising or publicity pertaining to distribution  of  the
software  without specific prior written permission. Sun and
M.I.T. make no representations about the suitability of this
software for any purpose. It is provided "as is" without any
express or implied warranty.

SUN DISCLAIMS ALL WARRANTIES WITH REGARD TO  THIS  SOFTWARE,
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FIT-
NESS FOR A PARTICULAR PURPOSE. IN NO EVENT SHALL SUN BE  LI-
ABLE  FOR  ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,  DATA  OR
PROFITS,  WHETHER  IN  AN  ACTION OF CONTRACT, NEGLIGENCE OR
OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION  WITH
THE USE OR PERFORMANCE OF THIS SOFTWARE.

********************************************************/

#include "X.h"
#include "Xmd.h"
#include	<servermd.h>
#include "scrnintstr.h"
#include "pixmapstr.h"
#include "resource.h"
#include "colormap.h"
#include "colormapst.h"
#include "cfb.h"
#include "mi.h"
#include "mistruct.h"
#include "dix.h"

extern void miGetImage();	/* XXX should not be needed */
extern ColormapPtr CreateStaticColormap();	/* XXX is this needed? */

static VisualRec visuals[] = {
/* vid screen class rMask gMask bMask oRed oGreen oBlue bpRGB cmpE nplan */
#ifdef	notdef
    /*  Eventually,  we would like to offer this visual too */
    0,	0, StaticGray, 0,   0,    0,   0,    0,	  0,    1,   2,    1,
#endif
#ifdef	STATIC_COLOR
    0,  0, StaticColor,0,   0,    0,   0,    0,   0,    8,  256,   8,
#else
    0,  0, PseudoColor,0,   0,    0,   0,    0,   0,    8,  256,   8,
#endif
};

#define	NUMVISUALS	((sizeof visuals)/(sizeof visuals[0]))
#define	ROOTVISUAL	(NUMVISUALS-1)

static DepthRec depths[] = {
/* depth	numVid		vids */
    1,		0,		NULL,
    8,		1,		NULL,
};

static ColormapPtr cfbColorMaps[NUMVISUALS];	/* assume one per visual */
#define NUMDEPTHS	((sizeof depths)/(sizeof depths[0]))

/* dts * (inch/dot) * (25.4 mm / inch) = mm */
Bool
cfbScreenInit(index, pScreen, pbits, xsize, ysize, dpi)
    int index;
    register ScreenPtr pScreen;
    pointer pbits;		/* pointer to screen bitmap */
    int xsize, ysize;		/* in pixels */
    int dpi;			/* dots per inch */
{
    long	*pVids;
    register PixmapPtr pPixmap;
    int	i;
    void cfbInitialize332Colormap();

    pScreen->myNum = index;
    pScreen->width = xsize;
    pScreen->height = ysize;
    pScreen->mmWidth = (xsize * 254) / (dpi * 10);
    pScreen->mmHeight = (ysize * 254) / (dpi * 10);
    pScreen->numDepths = NUMDEPTHS;
    pScreen->allowedDepths = depths;

    pScreen->rootDepth = 8;
    pScreen->minInstalledCmaps = 1;
    pScreen->maxInstalledCmaps = 1;
    pScreen->backingStoreSupport = NotUseful;
    pScreen->saveUnderSupport = NotUseful;

    /* cursmin and cursmax are device specific */

    pScreen->numVisuals = NUMVISUALS;
    pScreen->visuals = visuals;

    pPixmap = (PixmapPtr )Xalloc(sizeof(PixmapRec));
    pPixmap->drawable.type = DRAWABLE_PIXMAP;
    pPixmap->drawable.depth = 8;
    pPixmap->drawable.pScreen = pScreen;
    pPixmap->drawable.serialNumber = 0;
    pPixmap->width = xsize;
    pPixmap->height = ysize;
    pPixmap->refcnt = 1;
    pPixmap->devPrivate = pbits;
    pPixmap->devKind = PixmapBytePad(xsize, 8);
    pScreen->devPrivate = (pointer)pPixmap;

    /* anything that cfb doesn't know about is assumed to be done
       elsewhere.  (we put in no-op only for things that we KNOW
       are really no-op.
    */
    pScreen->QueryBestSize = cfbQueryBestSize;
    pScreen->CreateWindow = cfbCreateWindow;
    pScreen->DestroyWindow = cfbDestroyWindow;
    pScreen->PositionWindow = cfbPositionWindow;
    pScreen->ChangeWindowAttributes = cfbChangeWindowAttributes;
    pScreen->RealizeWindow = cfbMapWindow;
    pScreen->UnrealizeWindow = cfbUnmapWindow;

    pScreen->RealizeFont = mfbRealizeFont;
    pScreen->UnrealizeFont = mfbUnrealizeFont;
    pScreen->GetImage = miGetImage;
    pScreen->GetSpans = cfbGetSpans;	/* XXX */
    pScreen->CreateGC = cfbCreateGC;
    pScreen->CreatePixmap = cfbCreatePixmap;
    pScreen->DestroyPixmap = cfbDestroyPixmap;
    pScreen->ValidateTree = miValidateTree;

#ifdef	STATIC_COLOR
    pScreen->InstallColormap = NoopDDA;
    pScreen->UninstallColormap = NoopDDA;
    pScreen->ListInstalledColormaps = cfbListInstalledColormaps;
#endif
#ifdef	STATIC_COLOR
    pScreen->StoreColors = NoopDDA;
    pScreen->ResolveColor = cfbResolveStaticColor;
#endif

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

    pScreen->CreateColormap = cfbInitialize332Colormap;
    pScreen->DestroyColormap = NoopDDA;

    /*  Set up the remaining fields in the visuals[] array & make a RT_VISUALID */
    for (i = 0; i < NUMVISUALS; i++) {
	visuals[i].vid = FakeClientID(0);
	visuals[i].screen = index;
	AddResource(visuals[i].vid, RT_VISUALID, &visuals[i], NoopDDA, RC_CORE);
	switch (visuals[i].class) {
	case StaticGray:
	case StaticColor:
	    CreateColormap(FakeClientID(0), pScreen, &visuals[i], 
		&cfbColorMaps[i], AllocAll, 0);
	    break;
	case PseudoColor:
	case GrayScale:
	    CreateColormap(FakeClientID(0), pScreen, &visuals[i], 
		&cfbColorMaps[i], AllocNone, 0);
	    break;
	case TrueColor:
	case DirectColor:
	    FatalError("Bad visual in cfbScreenInit\n");
	}
	if (!cfbColorMaps[i])
	    FatalError("Can't create colormap in cfbScreenInit\n");
    }
    pScreen->defColormap = cfbColorMaps[ROOTVISUAL]->mid;
    pScreen->rootVisual = visuals[ROOTVISUAL].vid;

    /*  Set up the remaining fields in the depths[] array */
    for (i = 0; i < NUMDEPTHS; i++) {
	if (depths[i].numVids > 0) {
	    depths[i].vids = pVids = (long *) Xalloc(sizeof (long) * depths[i].numVids);
	    /* XXX - here we offer only the 8-bit visual */
	    pVids[0] = visuals[ROOTVISUAL].vid;
	}
    }
    return( TRUE );
}

