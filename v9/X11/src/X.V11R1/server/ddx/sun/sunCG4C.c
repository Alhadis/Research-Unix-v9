/*-
 * sunCG4C.c --
 *	Functions to support the sun CG4 board as a memory frame buffer.
 */

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

#ifndef	lint
static char sccsid[] = "@(#)sunCG4C.c	1.4 6/1/87 Copyright 1987 Sun Micro";
#endif

#include    "sun.h"

#include    <sys/mman.h>
#include    <pixrect/memreg.h>
#include    <sundev/cg4reg.h>
#include    "colormap.h"
#include    "colormapst.h"
#include    "resource.h"
#include    <struct.h>

/*-
 * The cg4 frame buffer is divided into several pieces.
 *	1) an array of 8-bit pixels
 *	2) a one-bit deep overlay plane
 *	3) an enable plane
 *	4) a colormap and status register
 *
 * XXX - put the cursor in the overlay plane
 */
#define	CG4_HEIGHT	900
#define	CG4_WIDTH	1152

typedef struct cg4c {
	u_char mpixel[128*1024];		/* bit-per-pixel memory */
	u_char epixel[128*1024];		/* enable plane */
	u_char cpixel[CG4_HEIGHT][CG4_WIDTH];	/* byte-per-pixel memory */
} CG4C, CG4CRec, *CG4CPtr;

#define CG4C_IMAGE(fb)	    ((caddr_t)(&(fb)->cpixel))
#define CG4C_IMAGEOFF	    ((off_t)0x0)
#define CG4C_IMAGELEN	    (((CG4_HEIGHT*CG4_WIDTH + 8191)/8192)*8192)
#define	CG4C_MONO(fb)	    ((caddr_t)(&(fb)->mpixel))
#define	CG4C_MONOLEN	    (128*1024)
#define	CG4C_ENABLE(fb)	    ((caddr_t)(&(fb)->epixel))
#define	CG4C_ENBLEN	    CG4C_MONOLEN

static CG4CPtr CG4Cfb = NULL;

/* XXX - next line means only one CG4 - fix this */
static ColormapPtr sunCG4CInstalledMap;

extern int TellLostMap(), TellGainedMap();

static void
sunCG4CUpdateColormap(pScreen, index, count, rmap, gmap, bmap)
    ScreenPtr	pScreen;
    int		index, count;
    u_char	*rmap, *gmap, *bmap;
{
    struct fbcmap sunCmap;

    sunCmap.index = index;
    sunCmap.count = count;
    sunCmap.red = &rmap[index];
    sunCmap.green = &gmap[index];
    sunCmap.blue = &bmap[index];

#ifdef SUN_WINDOWS
    if (sunUseSunWindows()) {
	static Pixwin *pw = 0;

	if (! pw) {
	    if ( ! (pw = pw_open(windowFd)) )
		FatalError( "sunCG4CUpdateColormap: pw_open failed\n" );
	    pw_setcmsname(pw, "X.V11");
	}
	pw_putcolormap(
	    pw, index, count, &rmap[index], &gmap[index], &bmap[index]);
    }
#endif SUN_WINDOWS

    if (ioctl(sunFbs[pScreen->myNum].fd, FBIOPUTCMAP, &sunCmap) < 0) {
	perror("sunCG4CUpdateColormap");
	FatalError( "sunCG4CUpdateColormap: FBIOPUTCMAP failed\n" );
    }
}

/*-
 *-----------------------------------------------------------------------
 * sunCG4CSaveScreen --
 *	Preserve the color screen by turning on or off the video
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Video state is switched
 *
 *-----------------------------------------------------------------------
 */
static Bool
sunCG4CSaveScreen (pScreen, on)
    ScreenPtr	  pScreen;
    Bool    	  on;
{
    int		state = on;

    switch (on) {
    case SCREEN_SAVER_FORCER:
	SetTimeSinceLastInputEvent();
	screenSaved = FALSE;
	state = 1;
	break;
    case SCREEN_SAVER_OFF:
	screenSaved = FALSE;
	state = 1;
	break;
    case SCREEN_SAVER_ON:
    default:
	screenSaved = TRUE;
	state = 0;
	break;
    }
    (void) ioctl(sunFbs[pScreen->myNum].fd, FBIOSVIDEO, &state);
    return( TRUE );
}

/*-
 *-----------------------------------------------------------------------
 * sunCG4CCloseScreen --
 *	called to ensure video is enabled when server exits.
 *
 * Results:
 *	Screen is unsaved.
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static Bool
sunCG4CCloseScreen(i, pScreen)
    int		i;
    ScreenPtr	pScreen;
{
    sunCG4CInstalledMap = NULL;
    return (pScreen->SaveScreen(pScreen, SCREEN_SAVER_OFF));
}

/*-
 *-----------------------------------------------------------------------
 * sunCG4CInstallColormap --
 *	Install given colormap.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	Existing map is uninstalled.
 *	All clients requesting ColormapNotify are notified
 *
 *-----------------------------------------------------------------------
 */
static void
sunCG4CInstallColormap(cmap)
    ColormapPtr	cmap;
{
    register int i;
    register Entry *pent = cmap->red;
    u_char	  rmap[256], gmap[256], bmap[256];

    if (cmap == sunCG4CInstalledMap)
	return;
    if (sunCG4CInstalledMap)
	WalkTree(sunCG4CInstalledMap->pScreen, TellLostMap,
		 (char *) &(sunCG4CInstalledMap->mid));
    for (i = 0; i < cmap->pVisual->ColormapEntries; i++) {
	if (pent->fShared) {
	    rmap[i] = pent->co.shco.red->color >> 8;
	    gmap[i] = pent->co.shco.green->color >> 8;
	    bmap[i] = pent->co.shco.blue->color >> 8;
	}
	else {
	    rmap[i] = pent->co.local.red >> 8;
	    gmap[i] = pent->co.local.green >> 8;
	    bmap[i] = pent->co.local.blue >> 8;
	}
	pent++;
    }
    sunCG4CInstalledMap = cmap;
    sunCG4CUpdateColormap(cmap->pScreen, 0, 256, rmap, gmap, bmap);
    WalkTree(cmap->pScreen, TellGainedMap, (char *) &(cmap->mid));
}

/*-
 *-----------------------------------------------------------------------
 * sunCG4CUninstallColormap --
 *	Uninstall given colormap.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	default map is installed
 *	All clients requesting ColormapNotify are notified
 *
 *-----------------------------------------------------------------------
 */
static void
sunCG4CUninstallColormap(cmap)
    ColormapPtr	cmap;
{
    if (cmap == sunCG4CInstalledMap) {
	Colormap defMapID = cmap->pScreen->defColormap;

	if (cmap->mid != defMapID) {
	    ColormapPtr defMap = (ColormapPtr) LookupID(defMapID, RT_COLORMAP, RC_CORE);

	    if (defMap)
		sunCG4CInstallColormap(defMap);
	    else
	        ErrorF("sunCG4C: Can't find default colormap\n");
	}
    }
}

/*-
 *-----------------------------------------------------------------------
 * sunCG4CListInstalledColormaps --
 *	Fills in the list with the IDs of the installed maps
 *
 * Results:
 *	Returns the number of IDs in the list
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static int
sunCG4CListInstalledColormaps(pScreen, pCmapList)
    ScreenPtr	pScreen;
    Colormap	*pCmapList;
{
    *pCmapList = sunCG4CInstalledMap->mid;
    return (1);
}


/*-
 *-----------------------------------------------------------------------
 * sunCG4CStoreColors --
 *	Sets the pixels in pdefs into the specified map.
 *
 * Results:
 *	None
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */
static void
sunCG4CStoreColors(pmap, ndef, pdefs)
    ColormapPtr	pmap;
    int		ndef;
    xColorItem	*pdefs;
{
    switch (pmap->class) {
    case PseudoColor:
	if (pmap == sunCG4CInstalledMap) {
	    /* We only have a single colormap */
	    u_char	rmap[256], gmap[256], bmap[256];

	    while (ndef--) {
		register unsigned index = pdefs->pixel&0xff;

		/* PUTCMAP assumes colors to be assigned start at 0 */
		rmap[index] = (pdefs->red) >> 8;
		gmap[index] = (pdefs->green) >> 8;
		bmap[index] = (pdefs->blue) >> 8;
	 	sunCG4CUpdateColormap(pmap->pScreen,
				      index, 1, rmap, gmap, bmap);
		pdefs++;
	    }
	}
	break;
    case DirectColor:
    default:
	ErrorF("sunCG4CStoreColors: bad class %d\n", pmap->class);
	break;
    }
}

/*-
 *-----------------------------------------------------------------------
 * sunCG4CResolvePseudoColor --
 *	Adjust specified RGB values to closest values hardware can do.
 *
 * Results:
 *	Args are modified.
 *
 * Side Effects:
 *	None
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
sunCG4CResolvePseudoColor(pRed, pGreen, pBlue, pVisual)
    CARD16	*pRed, *pGreen, *pBlue;
    VisualPtr	pVisual;
{
    *pRed &= 0xff00;
    *pGreen &= 0xff00;
    *pBlue &= 0xff00;
}

/*-
 *-----------------------------------------------------------------------
 * sunCG4CInit --
 *	Attempt to find and initialize a cg4 framebuffer used as mono
 *
 * Results:
 *	TRUE if everything went ok. FALSE if not.
 *
 * Side Effects:
 *	Most of the elements of the ScreenRec are filled in. Memory is
 *	allocated for the frame buffer and the buffer is mapped. The
 *	video is enabled for the frame buffer...
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static Bool
sunCG4CInit (index, pScreen, argc, argv)
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    ScreenPtr	  pScreen;  	/* The Screen to initialize */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    CARD16	zero = 0, ones = ~0;
#ifdef	notdef
    u_char	  rmap[256], gmap[256], bmap[256];
#endif

    if (!cfbScreenInit (index, pScreen, CG4Cfb->cpixel,	
			    sunFbs[index].info.fb_width,
			    sunFbs[index].info.fb_height, 90))
	return (FALSE);

    pScreen->SaveScreen =   	    	sunCG4CSaveScreen;
    pScreen->RealizeCursor = 	    	sunRealizeCursor;
    pScreen->UnrealizeCursor =	    	sunUnrealizeCursor;
    pScreen->DisplayCursor = 	    	sunDisplayCursor;
    pScreen->SetCursorPosition =    	sunSetCursorPosition;
    pScreen->CursorLimits = 	    	sunCursorLimits;
    pScreen->PointerNonInterestBox = 	sunPointerNonInterestBox;
    pScreen->ConstrainCursor = 	    	sunConstrainCursor;
    pScreen->RecolorCursor = 	    	sunRecolorCursor;

#ifdef	STATIC_COLOR
    pScreen->InstallColormap = NoopDDA;
    pScreen->UninstallColormap = NoopDDA;
    pScreen->ListInstalledColormaps = (int (*)())NoopDDA;
    pScreen->StoreColors = NoopDDA;
    pScreen->ResolveColor = cfbResolveStaticColor;
#else STATIC_COLOR
    pScreen->InstallColormap = sunCG4CInstallColormap;
    pScreen->UninstallColormap = sunCG4CUninstallColormap;
    pScreen->ListInstalledColormaps = sunCG4CListInstalledColormaps;
    pScreen->StoreColors = sunCG4CStoreColors;
    pScreen->ResolveColor = sunCG4CResolvePseudoColor;
#endif

#ifdef	notdef
    /*
     * Fill the color map with a color cube 
     */
    for (i = 0; i < 256; i++) {
	rmap[i] = (i & 0x7) << 5;
	gmap[i] = (i & 0x38) << 2;
	bmap[i] = (i & 0xc0);
    }

    sunCG4CUpdateColormap(pScreenInfo->screen[index], 0, 256, rmap, gmap, bmap);
#endif

    {
	ColormapPtr cmap = (ColormapPtr)LookupID(pScreen->defColormap, RT_COLORMAP, RC_CORE);

	if (!cmap)
	    FatalError("Can't find default colormap\n");
	if (AllocColor(cmap, &ones, &ones, &ones, &(pScreen->whitePixel), 0)
	    || AllocColor(cmap, &zero, &zero, &zero, &(pScreen->blackPixel), 0))
		FatalError("Can't alloc black & white pixels in cfbScreeninit\n");
	sunCG4CInstallColormap(cmap);
    }


    sunCG4CSaveScreen( pScreen, SCREEN_SAVER_FORCER );
    sunScreenInit (pScreen);
    return (TRUE);
}

/*-
 *--------------------------------------------------------------
 * sunCG4CSwitch --
 *      Enable or disable color plane 
 *
 * Results:
 *      Color plane enabled for select =0, disabled otherwise.
 *
 *--------------------------------------------------------------
 */
static void
sunCG4CSwitch (pScreen, select)
   ScreenPtr  pScreen;
   u_char     select;
{
   int index;
   register int j;

   index = pScreen->myNum;
   CG4Cfb = (CG4CPtr) sunFbs[index].fb;

   for (j = 0; j < 128*1024; j++)
        CG4Cfb->epixel[j] = (!select) ? 0 : 0xff;
}

/*-
 *-----------------------------------------------------------------------
 * sunCG4CProbe --
 *	Attempt to find and initialize a cg4 framebuffer used as mono
 *
 * Results:
 *	TRUE if everything went ok. FALSE if not.
 *
 * Side Effects:
 *	Memory is allocated for the frame buffer and the buffer is mapped.
 *
 *-----------------------------------------------------------------------
 */
Bool
sunCG4CProbe (pScreenInfo, index, fbNum, argc, argv)
    ScreenInfo	  *pScreenInfo;	/* The screenInfo struct */
    int	    	  index;    	/* The index of pScreen in the ScreenInfo */
    int	    	  fbNum;    	/* Index into the sunFbData array */
    int	    	  argc;	    	/* The number of the Server's arguments. */
    char    	  **argv;   	/* The arguments themselves. Don't change! */
{
    int         i, oldNumScreens;

    if (sunFbData[fbNum].probeStatus == probedAndFailed) {
	return FALSE;
    }

    if (sunFbData[fbNum].probeStatus == neverProbed) {
	int         fd;
	struct fbtype fbType;

	if ((fd = sunOpenFrameBuffer(FBTYPE_SUN4COLOR, &fbType, index, fbNum,
				     argc, argv)) < 0) {
	    sunFbData[fbNum].probeStatus = probedAndFailed;
	    return FALSE;
	}

#ifdef	_MAP_NEW
	if ((int)(CG4Cfb = (CG4CPtr) mmap((caddr_t) 0,
		 CG4C_MONOLEN + CG4C_ENBLEN + CG4C_IMAGELEN,
		 PROT_READ | PROT_WRITE,
		 MAP_SHARED | _MAP_NEW, fd, 0)) == -1) {
	    Error("Mapping cg4c");
	    sunFbData[fbNum].probeStatus = probedAndFailed;
	    (void) close(fd);
	    return FALSE;
	}
#else	_MAP_NEW
	CG4Cfb = (CG4CPtr) valloc(CG4C_MONOLEN + CG4C_ENBLEN + CG4C_IMAGELEN);
	if (CG4Cfb == (CG4CPtr) NULL) {
	    ErrorF("Could not allocate room for frame buffer.\n");
	    sunFbData[fbNum].probeStatus = probedAndFailed;
	    return FALSE;
	}

	if (mmap((caddr_t) CG4Cfb, CG4C_MONOLEN + CG4C_ENBLEN + CG4C_IMAGELEN,
		 PROT_READ | PROT_WRITE,
		 MAP_SHARED, fd, 0) < 0) {
	    Error("Mapping cg4c");
	    sunFbData[fbNum].probeStatus = probedAndFailed;
	    (void) close(fd);
	    return FALSE;
	}
#endif	_MAP_NEW

	sunFbs[index].fd = fd;
	sunFbs[index].info = fbType;
	sunFbs[index].fb = (pointer) CG4Cfb;
        sunFbs[index].EnterLeave = sunCG4CSwitch;
	sunFbData[fbNum].probeStatus = probedAndSucceeded;

    }

    /*
     * If we've ever successfully probed this device, do the following. 
     */

    oldNumScreens = pScreenInfo->numScreens;
    i = AddScreen(sunCG4CInit, argc, argv);
    pScreenInfo->screen[index].CloseScreen = sunCG4CCloseScreen;
    /* Now set the enable plane for color */
    if (index == 0) sunCG4CSwitch (&(pScreenInfo->screen[0]), 0);

    return (i > oldNumScreens);
}
