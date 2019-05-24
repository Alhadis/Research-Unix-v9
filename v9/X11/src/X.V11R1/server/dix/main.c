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
/* $Header: main.c,v 1.124 87/09/07 18:17:41 toddb Exp $ */

#include "X.h"
#include "Xproto.h"
#include "input.h"
#include "scrnintstr.h"
#include "misc.h"
#include "os.h"
#include "windowstr.h"
#include "resource.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "extension.h"
#include "colormap.h"
#include "cursorstr.h"
#include "opaque.h"
#include "servermd.h"

extern long defaultScreenSaverTime;
extern long defaultScreenSaverInterval;
extern int defaultScreenSaverBlanking;
extern int defaultScreenSaverAllowExposures;

extern char *display;
char *ConnectionInfo;
xConnSetupPrefix connSetupPrefix;

extern WindowRec WindowTable[];
extern xColorItem screenWhite, screenBlack;
extern FontPtr defaultFont;

extern void SetInputCheck();
extern void AbortServer();

PaddingInfo PixmapWidthPaddingInfo[33];
int connBlockScreenStart;

unsigned char *minfree;

static int restart = 0;

int
NotImplemented()
{
    FatalError("Not implemented");
}

/*
 * This array encodes the answer to the question "what is the log base 2
 * of the number of pixels that fit in a scanline pad unit?"
 * Note that ~0 is an invalid entry (mostly for the benefit of the reader).
 */
static int answer[6][3] = {
	/* pad   pad   pad */
	/*  8     16    32 */

	{   3,     4,    5 },	/* 1 bit per pixel */
	{   1,     2,    3 },	/* 4 bits per pixel */
	{   0,     1,    2 },	/* 8 bits per pixel */
	{   ~0,    0,    1 },	/* 16 bits per pixel */
	{   ~0,    ~0,   0 },	/* 24 bits per pixel */
	{   ~0,    ~0,   0 }	/* 32 bits per pixel */
};

/*
 * This array gives the answer to the question "what is the first index for
 * the answer array above given the number of bits per pixel?"
 * Note that ~0 is an invalid entry (mostly for the benefit of the reader).
 */
static int indexForBitsPerPixel[ 33 ] = {
	~0, 0, ~0, ~0,	/* 1 bit per pixel */
	1, ~0, ~0, ~0,	/* 4 bits per pixel */
	2, ~0, ~0, ~0,	/* 8 bits per pixel */
	~0,~0, ~0, ~0,
	3, ~0, ~0, ~0,	/* 16 bits per pixel */
	~0,~0, ~0, ~0,
	4, ~0, ~0, ~0,	/* 24 bits per pixel */
	~0,~0, ~0, ~0,
	5		/* 32 bits per pixel */
};

/*
 * This array gives the answer to the question "what is the second index for
 * the answer array above given the number of bits per scanline pad unit?"
 * Note that ~0 is an invalid entry (mostly for the benefit of the reader).
 */
static int indexForScanlinePad[ 33 ] = {
	~0, ~0, ~0, ~0,
	~0, ~0, ~0, ~0,
	0,  ~0, ~0, ~0,	/* 8 bits per scanline pad unit */
	~0, ~0, ~0, ~0,
	1,  ~0, ~0, ~0,	/* 16 bits per scanline pad unit */
	~0, ~0, ~0, ~0,
	~0, ~0, ~0, ~0,
	~0, ~0, ~0, ~0,
	2		/* 32 bits per scanline pad unit */
};

main(argc, argv)
    int		argc;
    char	*argv[];
{
    int		i, j, k, looping;
    int		alwaysCheckForInput[2];

    /* Notice if we're restart.  Probably this is because we jumped through
     * uninitialized pointer */
    minfree = (unsigned char *)sbrk(0);               /* FOR DEBUG       XXX */
    if (restart)
	FatalError("server restarted. Jumped through uninitialized pointer?\n");
    else
	restart = 1;
    /* These are needed by some routines which are called from interrupt
     * handlers, thus have no direct calling path back to main and thus
     * can't be passed argc, argv as parameters */
    argcGlobal = argc;
    argvGlobal = argv;
    display = "0";
    ProcessCommandLine(argc, argv);

    alwaysCheckForInput[0] = 0;
    alwaysCheckForInput[1] = 1;
    looping = 0;
    while(1)
    {
        ScreenSaverTime = defaultScreenSaverTime;
	ScreenSaverInterval = defaultScreenSaverInterval;
	ScreenSaverBlanking = defaultScreenSaverBlanking;
	ScreenSaverAllowExposures = defaultScreenSaverAllowExposures;

	/* Perform any operating system dependent initializations you'd like */
	OsInit();		
	if(!looping)
	{
	    CreateWellKnownSockets();
	    InitProcVectors();
	    serverClient = (ClientPtr)Xalloc(sizeof(ClientRec));
            serverClient->sequence = 0;
            serverClient->closeDownMode = RetainPermanent;
            serverClient->clientGone = FALSE;
            serverClient->lastDrawable = (DrawablePtr)NULL;
	    serverClient->lastDrawableID = INVALID;
            serverClient->lastGC = (GCPtr)NULL;
	    serverClient->lastGCID = None;
	    serverClient->numSaved = None;
	    serverClient->saveSet = (pointer *)NULL;
	    serverClient->index = 0;
	}
        currentMaxClients = 10;
        clients = (ClientPtr *)Xalloc(currentMaxClients * sizeof(ClientPtr));
        for (i=1; i<currentMaxClients; i++) 
            clients[i] = NullClient;
        clients[0] = serverClient;

	InitClientResources(serverClient);      /* for root resources */

	SetInputCheck(&alwaysCheckForInput[0], &alwaysCheckForInput[1]);
	screenInfo.arraySize = 0;
	screenInfo.numScreens = 0;
	screenInfo.screen = (ScreenPtr)NULL;
	/*
	 * Just in case the ddx doesnt supply a format for depth 1 (like qvss).
	 */
	j = indexForBitsPerPixel[ 1 ];
	k = indexForScanlinePad[ BITMAP_SCANLINE_PAD ];
	PixmapWidthPaddingInfo[1].scanlinePad = BITMAP_SCANLINE_PAD-1;
	PixmapWidthPaddingInfo[1].bitmapPadLog2 = answer[j][k];

	InitAtoms();
	InitExtensions(); 
	InitOutput(&screenInfo, argc, argv);
	if (screenInfo.numScreens < 1)
	    FatalError("no screens found\n");
	InitEvents();
	InitInput(argc, argv);
	InitAndStartDevices(argc, argv);

	SetDefaultFontPath(defaultFontPath);	/* default path has no nulls */
	if ( ! SetDefaultFont(defaultTextFont))
	    ErrorF( "main: Could not open default font '%s'\n",
	        defaultTextFont);
	if ( ! (rootCursor = CreateRootCursor(defaultCursorFont, 0)))
	    ErrorF( "main: Could not open default cursor font '%s'\n",
		defaultCursorFont);

	for (i=0; i<screenInfo.numScreens; i++) 
	{
	    CreateRootWindow(i);
	}
        DefineInitialRootWindow(&WindowTable[0]);
	if(!looping)
	{
	    CreateConnectionBlock();
	}

	Dispatch();

	/* Now free up whatever must be freed */
	CloseDownExtensions();
	FreeAllResources();
	for ( i = 0; i < screenInfo.numScreens; i++)
        {
	    FreeGCperDepth(i);
	    FreeDefaultStipple(i);
	}
	CloseDownDevices(argc, argv);
	for (i = 0; i < screenInfo.numScreens; i++)
	    (* screenInfo.screen[i].CloseScreen)(i, &screenInfo.screen[i]);
	Xfree(screenInfo.screen);

        CloseFont(defaultFont);
        defaultFont = (FontPtr)NULL;

	ResetHosts(display);
        Xfree(clients);

	looping = 1;
    }
}

static int padlength[4] = {0, 3, 2, 1};

CreateConnectionBlock()
{
    xConnSetup setup;
    xWindowRoot root;
    xDepth	depth;
    xVisualType visual;
    xPixmapFormat format;
    int i, j, k, vid, 
        lenofblock=0,
        sizesofar = 0;
    char *pBuf;

    
    /* Leave off the ridBase and ridMask, these must be sent with 
       connection */

    setup.release = VENDOR_RELEASE;
    /*
     * per-server image and bitmap parameters are defined in Xmd.h
     */
    setup.imageByteOrder = screenInfo.imageByteOrder;
    setup.bitmapScanlineUnit  = screenInfo.bitmapScanlineUnit;
    setup.bitmapScanlinePad = screenInfo.bitmapScanlinePad;
    setup.bitmapBitOrder = screenInfo.bitmapBitOrder;
    setup.motionBufferSize = NumMotionEvents();
    setup.numRoots = screenInfo.numScreens;
    setup.nbytesVendor = strlen(VENDOR_STRING); 
    setup.numFormats = screenInfo.numPixmapFormats;
    setup.maxRequestSize = MAX_REQUEST_SIZE;
    QueryMinMaxKeyCodes(&setup.minKeyCode, &setup.maxKeyCode);
    
    lenofblock = sizeof(xConnSetup) + 
            ((setup.nbytesVendor + 3) & ~3) +
	    (setup.numFormats * sizeof(xPixmapFormat)) +
            (setup.numRoots * sizeof(xWindowRoot));
    ConnectionInfo = (char *) Xalloc(lenofblock);

    bcopy((char *)&setup, ConnectionInfo, sizeof(xConnSetup));
    sizesofar = sizeof(xConnSetup);
    pBuf = ConnectionInfo + sizeof(xConnSetup);

    bcopy(VENDOR_STRING, pBuf, setup.nbytesVendor);
    sizesofar += setup.nbytesVendor;
    pBuf += setup.nbytesVendor;
    i = padlength[setup.nbytesVendor & 3];
    if (i)
    {
        char pad[4];
        bcopy(pad, pBuf, i);
        pBuf += i;
	sizesofar += i;
    }    
    
    for (i=0; i<screenInfo.numPixmapFormats; i++)
    {
	format.depth = screenInfo.formats[i].depth;
	format.bitsPerPixel = screenInfo.formats[i].bitsPerPixel;
	format.scanLinePad = screenInfo.formats[i].scanlinePad;;
	bcopy((char *)&format, pBuf, sizeof(xPixmapFormat));
	pBuf += sizeof(xPixmapFormat);
	sizesofar += sizeof(xPixmapFormat);
    }

    connBlockScreenStart = sizesofar;
    for (i=0; i<screenInfo.numScreens; i++) 
    {
	ScreenPtr	pScreen;
	DepthPtr	pDepth;
	VisualPtr	pVisual;

	pScreen = &(screenInfo.screen[i]);
        root.windowId = WindowTable[i].wid;
        root.defaultColormap = pScreen->defColormap;
        root.whitePixel = pScreen->whitePixel;
	root.blackPixel = pScreen->blackPixel;
        root.currentInputMask = 0;    /* filled in when sent */
        root.pixWidth = pScreen->width;
        root.pixHeight = pScreen->height;
        root.mmWidth = pScreen->mmWidth;
	root.mmHeight = pScreen->mmHeight;
        root.minInstalledMaps = pScreen->minInstalledCmaps;
        root.maxInstalledMaps = pScreen->maxInstalledCmaps; 
        root.rootVisualID = pScreen->rootVisual;		
        root.backingStore = pScreen->backingStoreSupport;
        root.saveUnders = pScreen->saveUnderSupport;
        root.rootDepth = pScreen->rootDepth;
	root.nDepths = pScreen->numDepths;
        bcopy((char *)&root, pBuf, sizeof(xWindowRoot));
	sizesofar += sizeof(xWindowRoot);
        pBuf += sizeof(xWindowRoot);

	pDepth = pScreen->allowedDepths;
	for(j = 0; j < pScreen->numDepths; j++, pDepth++)
	{
	    lenofblock += sizeof(xDepth) + 
		    (pDepth->numVids * sizeof(xVisualType));
            ConnectionInfo = (char *)Xrealloc(ConnectionInfo, lenofblock);
            pBuf = ConnectionInfo + sizesofar;            
	    depth.depth = pDepth->depth;
	    depth.nVisuals = pDepth->numVids;
	    bcopy((char *)&depth, pBuf, sizeof(xDepth));
	    pBuf += sizeof(xDepth);
	    sizesofar += sizeof(xDepth);
	    for(k = 0; k < pDepth->numVids; k++)
	    {
		vid = pDepth->vids[k];
		pVisual = (VisualPtr) LookupID(vid, RT_VISUALID, RC_CORE);
		visual.visualID = pVisual->vid;
		visual.class = pVisual->class;
		visual.bitsPerRGB = pVisual->bitsPerRGBValue;
		visual.colormapEntries = pVisual->ColormapEntries;
		visual.redMask = pVisual->redMask;
		visual.greenMask = pVisual->greenMask;
		visual.blueMask = pVisual->blueMask;
		bcopy((char *)&visual, pBuf, sizeof(xVisualType));
		pBuf += sizeof(xVisualType);
	        sizesofar += sizeof(xVisualType);
	    }
	}
    }
    connSetupPrefix.success = xTrue;
    connSetupPrefix.length = lenofblock/4;
    connSetupPrefix.majorVersion = X_PROTOCOL;
    connSetupPrefix.minorVersion = X_PROTOCOL_REVISION;
}


/* VARARGS */
FatalError (msg, v0, v1, v2, v3, v4, v5, v6, v7, v8)
    char *msg;
    int v0, v1, v2, v3, v4, v5, v6, v7, v8;
{
    ErrorF("\nFatal server bug!\n");
    ErrorF(msg, v0, v1, v2, v3, v4, v5, v6, v7, v8);
    ErrorF("\n");
    AbortServer();
}

/*
	grow the array of screenRecs if necessary.
	call the device-supplied initialization procedure 
with its screen number, a pointer to its ScreenRec, argc, and argv.
	return the number of successfully installed screens.

*/

AddScreen(pfnInit, argc, argv)
    Bool	(* pfnInit)();
    int argc;
    char **argv;
{

    int i = screenInfo.numScreens;
    int scanlinepad, format, bitsPerPixel, j, k;
#ifdef DEBUG
    void	(**jNI) ();
#endif /* DEBUG */

    if (screenInfo.numScreens == screenInfo.arraySize)
    {
	screenInfo.arraySize += 5;
	screenInfo.screen = (ScreenPtr)Xrealloc(
	    screenInfo.screen, 
	    screenInfo.arraySize * sizeof(ScreenRec));
    }

#ifdef DEBUG
	    for (jNI = &screenInfo.screen[i].QueryBestSize; 
		 jNI < (void (**) ()) &screenInfo.screen[i].RegionExtents; 
		 jNI++)
		*jNI = (void (*) ())NotImplemented;
#endif /* DEBUG */


    /*
     * This loop gets run once for every Screen that gets added,
     * but thats ok.  If the ddx layer initializes the formats
     * one at a time calling AddScreen() after each, then each
     * iteration will make it a little more accurate.  Worst case
     * we do this loop N * numPixmapFormats where N is # of screens.
     * Anyway, this must be called after InitOutput and before the
     * screen init routine is called.
     */
    for (format=0; format<screenInfo.numPixmapFormats; format++)
    {
 	bitsPerPixel = screenInfo.formats[format].bitsPerPixel;
  	scanlinepad = screenInfo.formats[format].scanlinePad;
 	j = indexForBitsPerPixel[ bitsPerPixel ];
  	k = indexForScanlinePad[ scanlinepad ];
 	PixmapWidthPaddingInfo[ bitsPerPixel ].bitmapPadLog2 = answer[j][k];
 	PixmapWidthPaddingInfo[ bitsPerPixel ].scanlinePad =
 	    (scanlinepad/bitsPerPixel) - 1;
    }
  
    /* This is where screen specific stuff gets initialized.  Load the
       screen structure, call the hardware, whatever.
       This is also where the default colormap should be allocated and
       also pixel values for blackPixel, whitePixel, and the cursor
       Note that InitScreen is NOT allowed to modify argc, argv, or
       any of the strings pointed to by argv.  They may be passed to
       multiple screens. 
    */ 
    screenInfo.screen[i].rgf = ~0;  /* there are no scratch GCs yet*/
    screenInfo.screen[i].myNum = i;
    if ((*pfnInit)(i, &screenInfo.screen[i], argc, argv))
    {
	screenInfo.numScreens++;
        CreateGCperDepthArray(i);
	CreateDefaultStipple(i);
    }
    else
	ErrorF("screen %d failed initialization\n", i);

    return screenInfo.numScreens;
}


