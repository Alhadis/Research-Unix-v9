/* $Header: scrnintstr.h,v 1.1 87/09/11 07:50:28 toddb Exp $ */
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
#ifndef SCREENINTSTRUCT_H
#define SCREENINTSTRUCT_H

#include "screenint.h"
#include "misc.h"
#include "region.h"
#include "pixmap.h"
#include "gc.h"
#include "colormap.h"


typedef struct _PixmapFormat {
    unsigned char	depth;
    unsigned char	bitsPerPixel;
    unsigned char	scanlinePad;
    } PixmapFormatRec;
    
typedef struct _Visual {
    long	vid;
    short	screen;    
    short       class;
    unsigned long	redMask, greenMask, blueMask;
    int		offsetRed, offsetGreen, offsetBlue;
    short       bitsPerRGBValue;
    short	ColormapEntries;
    short	nplanes;	/* = log2 (ColormapEntries). This does not
				 * imply that the screen has this many planes.
				 * it may have more or fewer */
  } VisualRec;

typedef struct _Depth {
    int		depth;
    int		numVids;
    long	*vids;    /* block of visual ids for this depth */
  } DepthRec;

typedef struct _Screen {
    int			myNum;	/* index of this instance in Screens[] */
    ATOM id;
    short		width, height;
    short		mmWidth, mmHeight;
    short		numDepths;
    DepthPtr       	allowedDepths;
    short       	rootDepth;
    long        	rootVisual;
    long		defColormap;
    short		minInstalledCmaps, maxInstalledCmaps;
    char                backingStoreSupport, saveUnderSupport;
    unsigned long	whitePixel, blackPixel;
    unsigned long	rgf;	/* array of flags; she's -- HUNGARIAN */
    GCPtr		GCperDepth[MAXFORMATS+1];
			/* next field is a stipple to use as default in
			   a GC.  we don't build default tiles of all depths
			   because they are likely to be of a color
			   different from the default fg pixel, so
			   we don't win anything by building
			   a standard one.
			*/
    PixmapPtr		PixmapPerDepth[1];
    pointer		devPrivate;
    short       	numVisuals;
    VisualPtr		visuals;

    /* Random screen procedures */

    Bool (* CloseScreen)();		/* index, pScreen */
    void (* QueryBestSize)();		/* class, pwidth, pheight */
    Bool (* SaveScreen)();		/* pScreen, on */
    void (* GetImage)();		/* pDrawable, sx, sy, w, h, format, 
					 * planemask, pdestbits */
    unsigned int  *(* GetSpans)();	/* pDrawable, wMax, ppt, pwidth,
					 * nspans */
    void (* PointerNonInterestBox)();	/* pScr, BoxPtr */

    /* Window Procedures */

    Bool (* CreateWindow)();		/* pWin */
    Bool (* DestroyWindow)();		/* pWin */
    Bool (* PositionWindow)();		/* pWin, x, y */
    Bool (* ChangeWindowAttributes)();	/* pWin, mask */
    Bool (* RealizeWindow)();		/* pWin */
    Bool (* UnrealizeWindow)();		/* pWin */
    int  (* ValidateTree)();		/* pParent, pChild, top, anyMarked */
    void (* WindowExposures)();       /* pWin: WindowPtr, pRegion: RegionPtr */

    /* Pixmap procedures */

    PixmapPtr (* CreatePixmap)(); 	/* pScreen, width, height, depth */
    Bool (* DestroyPixmap)();		/* pPixmap */

    /* Font procedures */

    Bool (* RealizeFont)();		/* pScr, pFont */
    Bool (* UnrealizeFont)();		/* pScr, pFont */

    /* Cursor Procedures */
    void (* ConstrainCursor)();   	/* pScr, BoxPtr */
    void (* CursorLimits)();		/* pScr, pCurs, BoxPtr, BoxPtr */
    Bool (* DisplayCursor)();		/* pScr, pCurs */
    Bool (* RealizeCursor)();		/* pScr, pCurs */
    Bool (* UnrealizeCursor)();		/* pScr, pCurs */
    void (* RecolorCursor)();		/* pScr, pCurs, displayed */
    Bool (* SetCursorPosition)();	/* pScr, x, y */

    /* GC procedures */

    Bool (* CreateGC)();		/* pGC */

    /* Colormap procedures */

    void (* CreateColormap)();		/* pcmap */
    void (* DestroyColormap)();		/* pcmap */
    void (* InstallColormap)();		/* pcmap */
    void (* UninstallColormap)();	/* pcmap */
    int (* ListInstalledColormaps) (); 	/* pScreen, pmaps */
    void (* StoreColors)();		/* pmap, ndef, pdef */
    void (* ResolveColor)();		/* preg, pgreen, pblue */

    /* Region procedures */

    RegionPtr (* RegionCreate)(); 	/* rect, size */
    void (* RegionCopy)();		/* dstrgn, srcrgn */
    void (* RegionDestroy)();		/* pRegion */
    int (* Intersect)();		/* newReg, reg1, reg2 */
    int (* Union)();			/* newReg, reg1, reg2 */
    int (* Subtract)();			/* regD, regM, regS */
    int (* Inverse)();			/* newReg, reg1, invRect */
    void (* RegionReset)();		/* pRegion, pBox */
    void (* TranslateRegion)();		/* pRegion, x, y */
    int (* RectIn)();			/* pRegion, pRect */
    Bool (* PointInRegion)();		/* pRegion, x, y, pBox */
    Bool (* RegionNotEmpty)();      	/* pRegion: RegionPtr */
    void (* RegionEmpty)();        	/* pRegion: RegionPtr */
    BoxPtr (*RegionExtents)(); 		/* pRegion: RegionPtr */

    /* os layer procedures */
    void (* BlockHandler)();		/* data: pointer */
    void (* WakeupHandler)();		/* data: pointer */
    pointer blockData;
    pointer wakeupData;
} ScreenRec;

typedef struct _ScreenInfo {
    int		imageByteOrder;
    int		bitmapScanlineUnit;
    int		bitmapScanlinePad;
    int		bitmapBitOrder;
    int		numPixmapFormats;
    PixmapFormatRec
		formats[MAXFORMATS];
    int		arraySize;
    int		numScreens;
    ScreenPtr	screen;
} ScreenInfo;

extern ScreenInfo screenInfo;
#endif /* SCREENINTSTRUCT_H */
