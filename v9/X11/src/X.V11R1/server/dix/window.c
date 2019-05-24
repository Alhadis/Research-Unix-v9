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

/* $Header: window.c,v 1.170 87/09/07 18:56:00 rws Exp $ */

#include "X.h"
#define NEED_REPLIES
#define NEED_EVENTS
#include "Xproto.h"
#include "misc.h"
#include "scrnintstr.h"
#include "os.h"
#include "regionstr.h"
#include "windowstr.h"
#include "input.h"
#include "resource.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "dixstruct.h"
#include "gcstruct.h"
#include "servermd.h"

/******
 * Window stuff for server 
 *
 *    CreateRootWindow, CreateWindow, ChangeWindowAttributes,
 *    GetWindowAttributes, DeleteWindow, DestroySubWindows,
 *    HandleSaveSet, ReparentWindow, MapWindow, MapSubWindows,
 *    UnmapWindow, UnmapSubWindows, ConfigureWindow, CirculateWindow,
 *
 ******/

static long _back[16] = {0x88888888, 0x22222222, 0x44444444, 0x11111111,
			 0x88888888, 0x22222222, 0x44444444, 0x11111111,
			 0x88888888, 0x22222222, 0x44444444, 0x11111111,
			 0x88888888, 0x22222222, 0x44444444, 0x11111111};

typedef struct _ScreenSaverStuff {
    WindowPtr pWindow;
    XID       wid;
    XID       cid;
    BYTE      blanked;
} ScreenSaverStuffRec;

#define SCREEN_IS_BLANKED   0
#define SCREEN_IS_TILED     1
#define SCREEN_ISNT_SAVED   2

#define DONT_USE_GRAVITY 0
#define USE_GRAVITY   1

extern int ScreenSaverBlanking, ScreenSaverAllowExposures;
int screenIsSaved = FALSE;

static ScreenSaverStuffRec savedScreenInfo[MAXSCREENS];

extern WindowRec WindowTable[];
extern void (* ReplySwapVector[256]) ();

static void ResizeChildrenWinSize();

#define INPUTONLY_LEGAL_MASK (CWWinGravity | CWEventMask | \
			      CWDontPropagate | CWOverrideRedirect | CWCursor )

/******
 * PrintWindowTree
 *    For debugging only
 ******/

int
PrintChildren(p1, indent)
    WindowPtr p1;
    int indent;
{
    WindowPtr p2;
    int i;
 
    while (p1) 
    {
        p2 = p1->firstChild;
        for (i=0; i<indent; i++) ErrorF( " ");
	ErrorF( "%x\n", p1->wid);
        miprintRects(p1->clipList); 
	PrintChildren(p2, indent+4);
	p1 = p1->nextSib;
    }
}

PrintWindowTree()          
{
    int i;
    WindowPtr pWin, p1;

    for (i=0; i<screenInfo.numScreens; i++)
    {
	ErrorF( "WINDOW %d\n", i);
	pWin = &WindowTable[i];
        miprintRects(pWin->clipList); 
	p1 = pWin->firstChild;
	PrintChildren(p1, 4);
    }
}


/*****
 * WalkTree
 *   Walk the window tree, for SCREEN, preforming FUNC(pWin, data) on
 *   each window.  If FUNC returns WT_WALKCHILDREN, traverse the children,
 *   if it returns WT_DONTWALKCHILDREN, dont.  If it returns WT_STOPWALKING
 *   exit WalkTree.  Does depth-first traverse.
 *****/

int
TraverseTree(pWin, func, data)
    WindowPtr pWin;
    int (*func)();
    pointer data;
{
    int result;
    WindowPtr pChild;

    if (pWin == 0) 
       return(WT_NOMATCH);
    result = (* func)(pWin, data);

    if (result == WT_STOPWALKING) 
        return(WT_STOPWALKING);

    if (result == WT_WALKCHILDREN) 
        for (pChild = pWin->firstChild; pChild; pChild = pChild->nextSib)
            if (TraverseTree(pChild, func,data) ==  WT_STOPWALKING) 
                return(WT_STOPWALKING);

    return(WT_NOMATCH);
}

int
WalkTree(pScreen, func, data)
    ScreenPtr pScreen;
    int (* func)();
    char *data;
{
    WindowPtr pWin;
    
    pWin = &WindowTable[pScreen->myNum];
    return(TraverseTree(pWin, func, data));
}

/*****
 *  DoObscures(pWin)
 *    
 *****/

static void
DoObscures(pWin)
    WindowPtr pWin;
{
    WindowPtr pSib;

    if (pWin->backStorage  == 0 || (pWin->backingStore == NotUseful))
        return ;
    if ((* pWin->drawable.pScreen->RegionNotEmpty)(pWin->backStorage->obscured))
    {
        (*pWin->backStorage->SaveDoomedAreas)( pWin );
        (* pWin->drawable.pScreen->RegionEmpty)(pWin->backStorage->obscured);
    }
    pSib = pWin->firstChild;
    while (pSib)
    {
        DoObscures(pSib);
	pSib = pSib->nextSib;
    }
}

/*****
 *  HandleExposures(pWin)
 *    starting at pWin, draw background in any windows that have exposure
 *    regions, translate the regions, restore any backing store,
 *    and then send any regions stille xposed to the client
 *****/

/* NOTE
   the order of painting and restoration needs to be different,
to avoid an extra repaint of the background. --rgd
*/

void
HandleExposures(pWin)
    WindowPtr pWin;
{
    WindowPtr pSib;

    if ((* pWin->drawable.pScreen->RegionNotEmpty)(pWin->borderExposed))
    {
	(*pWin->PaintWindowBorder)(pWin, pWin->borderExposed, PW_BORDER);
	(* pWin->drawable.pScreen->RegionEmpty)(pWin->borderExposed);
    }
    (* pWin->drawable.pScreen->WindowExposures)(pWin);
    pSib = pWin->firstChild;
    while (pSib)
    {
        HandleExposures(pSib);
	pSib = pSib->nextSib;
    }
}

extern int NotImplemented();

static void
InitProcedures(pWin)
    WindowPtr pWin;
{
#ifdef DEBUG
    void (**j) ();
    for (j = &pWin->PaintWindowBackground;
         j < &pWin->ClearToBackground; j++ )
        *j = (void (*) ())NotImplemented;
#endif /* DEBUG */

}

static void
SetWindowToDefaults(pWin, pScreen)
    WindowPtr pWin;
    ScreenPtr pScreen;
{
    pWin->prevSib = NullWindow;
    pWin->firstChild = NullWindow;
    pWin->lastChild = NullWindow;

    pWin->userProps = (PropertyPtr)NULL;

    pWin->backingStore = NotUseful;
    pWin->backStorage = (BackingStorePtr) NULL;

    pWin->mapped = 0;           /* off */
    pWin->realized = 0;         /* off */
    pWin->viewable = 0;
    pWin->overrideRedirect = FALSE;
    pWin->saveUnder = FALSE;

    pWin->bitGravity = ForgetGravity; 
    pWin->winGravity = NorthWestGravity;
    pWin->backingBitPlanes = -1;
    pWin->backingPixel = 0;

    pWin->eventMask = 0;
    pWin->dontPropagateMask = 0;
    pWin->allEventMasks = 0;
    pWin->deliverableEvents = 0;

    pWin->otherClients = (pointer)NULL;
    pWin->passiveGrabs = (pointer)NULL;
    pWin->colormap = (Colormap)CopyFromParent;

    pWin->exposed = (* pScreen->RegionCreate)(NULL, 1);
    pWin->borderExposed = (* pScreen->RegionCreate)(NULL, 1);

}

static void
MakeRootCursor(pWin)
    WindowPtr pWin;
{
    unsigned char *srcbits, *mskbits;
    int i;
    if (rootCursor)
    {
	pWin->cursor = rootCursor;
	rootCursor->refcnt++;
    }
    else
    {
	CursorMetricRec cm;
	cm.width=32;
	cm.height=16;
	cm.xhot=8;
	cm.yhot=8;

        srcbits = (unsigned char *)Xalloc( PixmapBytePad(32, 1)*16); 
        mskbits = (unsigned char *)Xalloc( PixmapBytePad(32, 1)*16); 
        for (i=0; i<PixmapBytePad(32, 1)*16; i++)
	{
	    srcbits[i] = mskbits[i] = 0xff;
	}
	pWin->cursor = AllocCursor( srcbits, mskbits,	&cm,
		    ~0, ~0, ~0, 0, 0, 0);
    }
}

static void
MakeRootTile(pWin)
    WindowPtr pWin;
{
    ScreenPtr pScreen = pWin->drawable.pScreen;
    GCPtr pGC;

    pWin->backgroundTile = (*pScreen->CreatePixmap)(pScreen, 16, 16, 
			        pScreen->rootDepth);

    pGC = GetScratchGC(pScreen->rootDepth, pScreen);

    {
	CARD32 attributes[3];

	attributes[0] = pScreen->whitePixel;
	attributes[1] = pScreen->blackPixel;
	attributes[2] = FALSE;

	ChangeGC(pGC, GCForeground | GCBackground | GCGraphicsExposures,
	     attributes, 3);
   }

   ValidateGC(pWin->backgroundTile, pGC);

   (*pGC->PutImage)(pWin->backgroundTile, pGC, 1,
	            0, 0, 16, 16, 0, XYBitmap, _back);

   FreeScratchGC(pGC);

}

/*****
 * CreateRootWindow
 *    Makes a window at initialization time for specified screen
 *****/

int
CreateRootWindow(screen)
    int		screen;
{
    WindowPtr	pWin;
    BoxRec	box;
    ScreenPtr	pScreen;

    savedScreenInfo[screen].pWindow = NULL;
    savedScreenInfo[screen].wid = FakeClientID(0);
    savedScreenInfo[screen].cid = FakeClientID(0);
    screenIsSaved = SCREEN_SAVER_OFF;
    
    pWin = &WindowTable[screen];
    pScreen = &screenInfo.screen[screen];
    InitProcedures(pWin);

    pWin->drawable.pScreen = pScreen;
    pWin->drawable.type = DRAWABLE_WINDOW;

    pWin->drawable.depth = pScreen->rootDepth;

    pWin->parent = NullWindow;
    SetWindowToDefaults(pWin, pScreen);

    pWin->colormap = pScreen->defColormap;    

    pWin->nextSib = NullWindow;

    MakeRootCursor(pWin);

    pWin->client = serverClient;        /* since belongs to server */
    pWin->wid = FakeClientID(0);

    pWin->clientWinSize.x = pWin->clientWinSize.y = 0;
    pWin->clientWinSize.height = screenInfo.screen[screen].height;
    pWin->clientWinSize.width = screenInfo.screen[screen].width;
    pWin->absCorner.x = pWin->absCorner.y = 0;
    pWin->oldAbsCorner.x = pWin->oldAbsCorner.y = 0;

    box.x1 = 0;
    box.y1 = 0;
    box.x2 = screenInfo.screen[screen].width ;
    box.y2 = screenInfo.screen[screen].height;
    pWin->clipList = (* pScreen->RegionCreate)(&box, 1); 
    pWin->winSize = (* pScreen->RegionCreate)(&box, 1);
    pWin->borderSize = (* pScreen->RegionCreate)(&box, 1);
    pWin->borderClip = (* pScreen->RegionCreate)(&box, 1); 

    pWin->class = InputOutput;
    pWin->visual = pScreen->rootVisual;

    pWin->backgroundPixel = pScreen->whitePixel;

    pWin->borderPixel = pScreen->blackPixel;
    pWin->borderWidth = 0;

    AddResource(pWin->wid, RT_WINDOW, pWin, DeleteWindow, RC_CORE);

    MakeRootTile(pWin);
    pWin->borderTile = (PixmapPtr)USE_BORDER_PIXEL;

    /* re-validate GC for use with root Window */

    (*pScreen->CreateWindow)(pWin);
    MapWindow(pWin, DONT_HANDLE_EXPOSURES, BITS_DISCARDED, 
	      DONT_SEND_NOTIFICATION, serverClient);

    /* We SHOULD check for an error value here XXX */
    (*pScreen->ChangeWindowAttributes)(pWin, CWBackPixmap | CWBorderPixel);

    (*pWin->PaintWindowBackground)(pWin, pWin->clipList, PW_BACKGROUND);
    EventSelectForWindow(pWin, serverClient, 0);
    return(Success);
}

/*****
 * CreateWindow
 *    Makes a window in response to client request 
 *    XXX  What about depth of inputonly windows -- should be 0
 *****/

WindowPtr
CreateWindow(wid, pParent, x, y, w, h, bw, class, vmask, vlist, 
	     depth, client, visual, error)
    Window wid;
    WindowPtr pParent;    /* already looked up in table to do error checking*/
    short x,y;
    unsigned short w, h, bw;
    int class;
    int vmask;
    long *vlist;
    int depth;
    ClientPtr client;
    VisualID visual;
    int *error;
{
    WindowPtr pWin;
    ScreenPtr pScreen;
    BoxRec box;
    xEvent event;
    int idepth, ivisual;
    Bool fOK;
    DepthPtr pDepth;

    if ((class != InputOnly) && (pParent->class == InputOnly))
    {
        *error = BadMatch;
	return (WindowPtr)NULL;
    }

    if ((class == InputOnly) && ((bw != 0) || (depth != 0)))
    {
        *error = BadMatch;
	return (WindowPtr)NULL;
    }

    pScreen = pParent->drawable.pScreen;
    /* Find out if the depth and visual are acceptable for this Screen */
    fOK = FALSE;
    if ((class == InputOutput && depth == 0) || 
	(class == InputOnly) || (class == CopyFromParent))
        depth = pParent->drawable.depth;

    if (visual == CopyFromParent)
        visual = pParent->visual;
    else
    {
        for(idepth = 0; idepth < pScreen->numDepths; idepth++)
        {
	    pDepth = (DepthPtr) &pScreen->allowedDepths[idepth];
	    if (depth == pDepth->depth)
    	    {
		for (ivisual = 0; ivisual < pDepth->numVids; ivisual++)
	        {
		    if (visual == pDepth->vids[ivisual])
		        fOK = TRUE;
	        }
	    }
        }
        if (fOK == 0)
        {
            *error = BadMatch;
	    return (WindowPtr)NULL;
        }
    }

    pWin = (WindowPtr ) Xalloc( sizeof(WindowRec) );
    if (pWin == (WindowPtr) NULL) 
    {
	*error = BadAlloc;
	return (WindowPtr)NULL;
    }
    InitProcedures(pWin);
    pWin->drawable = pParent->drawable;
    if (class == InputOutput)
	pWin->drawable.depth = depth;
    else if (class == InputOnly)
        pWin->drawable.type = (short) UNDRAWABLE_WINDOW;

    pWin->wid = wid;
    pWin->client = client;
    pWin->visual = visual;

    SetWindowToDefaults(pWin, pScreen);

    pWin->cursor = (CursorPtr)None;

    if (class == CopyFromParent)
	pWin->class = pParent->class;
    else
	pWin->class = class;

    pWin->borderWidth = (int) bw;
    pWin->backgroundTile = (PixmapPtr)None;
    pWin->backgroundPixel = pScreen->whitePixel;

    if ((pWin->drawable.depth != pParent->drawable.depth) &&
	(((vmask & (CWBorderPixmap | CWBorderPixel)) == 0 )))
    {
        Xfree(pWin);
        *error = BadMatch;
        return (WindowPtr)NULL;
    }
    if ((vmask & (CWBorderPixmap | CWBorderPixel)) != 0)
		/* it will just get fixed in ChangeWindowAttributes */
        pWin->borderTile = (PixmapPtr)NULL;
    else
    {                              /* this is WRONG!!         XXX */
	                           /* should be actually copied */
        pWin->borderTile = pParent->borderTile;   
        if (IS_VALID_PIXMAP(pParent->borderTile))
            pParent->borderTile->refcnt++;
    }
    pWin->borderPixel = pParent->borderPixel;
    pWin->visibility = VisibilityFullyObscured;
		
    pWin->clientWinSize.x = x + bw;
    pWin->clientWinSize.y = y + bw;
    pWin->clientWinSize.height = h;
    pWin->clientWinSize.width = w;
    pWin->absCorner.x = pWin->oldAbsCorner.x = pParent->absCorner.x + x + bw;
    pWin->absCorner.y = pWin->oldAbsCorner.y = pParent->absCorner.y + y + bw;

        /* set up clip list correctly for unobscured WindowPtr */
    pWin->clipList = (* pScreen->RegionCreate)(NULL, 1);
    pWin->borderClip = (* pScreen->RegionCreate)(NULL, 1);
    box.x1 = pWin->absCorner.x;
    box.y1 = pWin->absCorner.y;
    box.x2 = box.x1 + w;
    box.y2 = box.y1 + h;
    pWin->winSize = (* pScreen->RegionCreate)(&box, 1);
    (* pScreen->Intersect)(pWin->winSize, pWin->winSize,  pParent->winSize);
    if (bw)
    {
	box.x1 -= bw;
	box.y1 -= bw;
	box.x2 += bw;
	box.y2 += bw;
        pWin->borderSize = (* pScreen->RegionCreate)(&box, 1);
        (* pScreen->Intersect)(pWin->borderSize, pWin->borderSize, 
			       pParent->winSize);
    }
    else
    {
        pWin->borderSize = (* pScreen->RegionCreate)(NULL, 1);
	(* pScreen->RegionCopy)(pWin->borderSize, pWin->winSize);
    }	

    pWin->parent = pParent;    
    if ((screenIsSaved == SCREEN_SAVER_ON)
	&& (pParent == &WindowTable[pScreen->myNum])
	&& (pParent->firstChild)
	&& (savedScreenInfo[pScreen->myNum].blanked == SCREEN_IS_TILED))
    {
	WindowPtr pFirst = pParent->firstChild;
	pWin->nextSib = pFirst->nextSib;
        if (pFirst->nextSib)
    	    pFirst->nextSib->prevSib = pWin;
	else
	    pParent->lastChild = pWin;
        pFirst->nextSib = pWin;
	pWin->prevSib = pFirst;
    }
    else
    {
        pWin->nextSib = pParent->firstChild;
        if (pParent->firstChild) 
	    pParent->firstChild->prevSib = pWin;
        else
            pParent->lastChild = pWin;
	pParent->firstChild = pWin;
    }

    /* We SHOULD check for an error value here XXX */
    (*pScreen->CreateWindow)(pWin);
    /* We SHOULD check for an error value here XXX */
    (*pScreen->PositionWindow)(pWin, pWin->absCorner.x, pWin->absCorner.y);
    if ((vmask & CWEventMask) == 0)
        EventSelectForWindow(pWin, client, 0);

    if (vmask)
        *error = ChangeWindowAttributes(pWin, vmask, vlist, pWin->client);
    else
        *error = Success;

    WindowHasNewCursor( pWin);

    event.u.u.type = CreateNotify;
    event.u.createNotify.window = wid;
    event.u.createNotify.parent = pParent->wid;
    event.u.createNotify.x = x;
    event.u.createNotify.y = y;
    event.u.createNotify.width = w;
    event.u.createNotify.height = h;
    event.u.createNotify.borderWidth = bw;
    event.u.createNotify.override = pWin->overrideRedirect;
    DeliverEvents(pWin->parent, &event, 1, NullWindow);		

    return pWin;
}

static void
FreeWindowResources(pWin)
    WindowPtr pWin;
{
    ScreenPtr pScreen;
    void (* proc)();

    pScreen = pWin->drawable.pScreen;

    DeleteWindowFromAnySaveSet(pWin);
    DeleteWindowFromAnySelections(pWin);
    DeleteWindowFromAnyEvents(pWin, TRUE);
    proc = pScreen->RegionDestroy;
    (* proc)(pWin->clipList);
    (* proc)(pWin->winSize);
    (* proc)(pWin->borderClip);
    (* proc)(pWin->borderSize);
    (* proc)(pWin->exposed);
    (* proc)(pWin->borderExposed);
    if (pWin->backStorage)
    {
        (* proc)(pWin->backStorage->obscured);
	Xfree(pWin->backStorage);
    }
    (* pScreen->DestroyPixmap)(pWin->borderTile);
    (* pScreen->DestroyPixmap)(pWin->backgroundTile);

    if (pWin->cursor != (CursorPtr)None)
        FreeCursor( pWin->cursor, 0);

    DeleteAllWindowProperties(pWin);
    /* We SHOULD check for an error value here XXX */
    (* pScreen->DestroyWindow)(pWin);
}

static void
CrushTree(pWin)
    WindowPtr pWin;
{
    WindowPtr pSib;
    xEvent event;

    if (pWin == (WindowPtr) NULL) 
        return;
    while (pWin) 
    {
	CrushTree(pWin->firstChild);

	event.u.u.type = DestroyNotify;
       	event.u.destroyNotify.window = pWin->wid;
	DeliverEvents(pWin, &event, 1, NullWindow);		

	FreeResource(pWin->wid, RC_CORE);
	pSib = pWin->nextSib;
	pWin->realized = FALSE;
	pWin->viewable = FALSE;
	(* pWin->drawable.pScreen->UnrealizeWindow)(pWin);
	FreeWindowResources(pWin);
	Xfree(pWin);
	pWin = pSib;
    }
}
	
/*****
 *  DeleteWindow
 *       Deletes child of window then window itself
 *****/

DeleteWindow(pWin, wid)
    WindowPtr pWin;
    int wid;
{
    WindowPtr pParent;
    xEvent event;

    UnmapWindow(pWin, HANDLE_EXPOSURES, SEND_NOTIFICATION, FALSE);

    CrushTree(pWin->firstChild);

    event.u.u.type = DestroyNotify;
    event.u.destroyNotify.window = pWin->wid;
    DeliverEvents(pWin, &event, 1, NullWindow);		

    pParent = pWin->parent;
    FreeWindowResources(pWin);
    if (pParent)
    {
	if (pParent->firstChild == pWin)
            pParent->firstChild = pWin->nextSib;
	if (pParent->lastChild == pWin)
            pParent->lastChild = pWin->prevSib;
        if (pWin->nextSib) 
            pWin->nextSib->prevSib = pWin->prevSib;
        if (pWin->prevSib) 
            pWin->prevSib->nextSib = pWin->nextSib;
	Xfree(pWin);
    }
}

DestroySubwindows(pWin, client)
    WindowPtr pWin;
    ClientPtr client;
{
    WindowPtr pChild, pSib;
    xEvent event;

    if ((pChild = pWin->lastChild) == (WindowPtr) NULL)
        return;
    while (pChild) 
    {
	pSib = pChild->prevSib;
	/* a little lazy evaluation, don't send exposes until all deleted */
	if (pSib != (WindowPtr) NULL)
	{
	    event.u.u.type = UnmapNotify;
	    event.u.unmapNotify.window = pWin->wid;
	    event.u.unmapNotify.fromConfigure = xFalse;
	    DeliverEvents(pWin, &event, 1, NullWindow);
	}
        else
        {
	    pChild->nextSib = (WindowPtr)NULL;
    	    UnmapWindow(pChild, HANDLE_EXPOSURES, SEND_NOTIFICATION, FALSE);
	}
	CrushTree(pChild->firstChild);

	event.u.u.type = DestroyNotify;
	event.u.destroyNotify.window = pChild->wid;
	DeliverEvents(pChild, &event, 1, NullWindow);		
    
	FreeResource(pChild->wid, RC_CORE);
	FreeWindowResources(pChild);
	Xfree(pChild);
	pChild = pSib;
    }
    pWin->firstChild = (WindowPtr )NULL;
}


/*****
 *  ChangeWindowAttributes
 *   
 *  The value-mask specifies which attributes are to be changed; the
 *  value-list contains one value for each one bit in the mask, from least
 *  to most significant bit in the mask.  
 *****/
 
int 
ChangeWindowAttributes(pWin, vmask, vlist, client)
    WindowPtr pWin;
    unsigned int vmask;
    long *vlist;
    ClientPtr client;
{
    int index;
    long *pVlist;
    PixmapPtr pPixmap;
    Pixmap pixID;
    CursorPtr pCursor;
    Cursor cursorID;
    int result;
    ScreenPtr pScreen;
    unsigned int vmaskCopy = 0;
    int error;

    if ((pWin->class == InputOnly) && (vmask & (~INPUTONLY_LEGAL_MASK)))
        return BadMatch;

    error = Success;
    pScreen = pWin->drawable.pScreen;
    pVlist = vlist;
    while (vmask) 
    {
	index = ffs(vmask) - 1;
	vmask &= ~(index = (1 << index));
	switch (index) 
        {
	  case CWBackPixmap: 
	    pixID = (Pixmap )*pVlist;
	    pVlist++;
	    if (pixID == None)
	    {
		(* pScreen->DestroyPixmap)(pWin->backgroundTile);
		if (pWin->parent == (WindowPtr) NULL)
                    MakeRootTile(pWin);
                else
                    pWin->backgroundTile = (PixmapPtr)NULL;
	    }
	    else if (pixID == ParentRelative)
	    {
		(* pScreen->DestroyPixmap)(pWin->backgroundTile);
		if (pWin->parent == (WindowPtr) NULL)
		    MakeRootTile(pWin);
		else
	            pWin->backgroundTile = (PixmapPtr)ParentRelative;
		/* Note that the parent's backgroundTile's refcnt is NOT
		 * incremented. */
	    }
            else
	    {	
                pPixmap = (PixmapPtr)LookupID(pixID, RT_PIXMAP, RC_CORE);
                if (pPixmap != (PixmapPtr) NULL)
		{
                    if  ((pPixmap->drawable.depth != pWin->drawable.depth) ||
			 (pPixmap->drawable.pScreen != pScreen))
		    {
                        error = BadMatch;
			goto PatchUp;
		    }
		    (* pScreen->DestroyPixmap)(pWin->backgroundTile); 
		    pWin->backgroundTile = pPixmap;
		    pPixmap->refcnt++;
		}
	        else 
		{
		    error = BadPixmap;
		    goto PatchUp;
		}
	    }
	    break;
	  case CWBackPixel: 
	    pWin->backgroundPixel = (CARD32 ) *pVlist;
	    (* pScreen->DestroyPixmap)(pWin->backgroundTile);
	    pWin->backgroundTile = (PixmapPtr)USE_BACKGROUND_PIXEL;
	           /* background pixel overrides background pixmap,
		      so don't let the ddx layer see both bits */
            vmaskCopy &= ~CWBackPixmap;
	    pVlist++;
	    break;
	  case CWBorderPixmap:
	    pixID = (Pixmap ) *pVlist;
	    pVlist++;
	    if (pixID == CopyFromParent)
	    {
		GCPtr pGC;
		PixmapPtr parentPixmap;
		if ((pWin->parent == (WindowPtr) NULL) || (pWin->parent && 
		        	       (pWin->drawable.depth != 
					pWin->parent->drawable.depth)))
		{
		    error = BadMatch;
		    goto PatchUp;
		}
		(* pScreen->DestroyPixmap)(pWin->borderTile);
		parentPixmap = pWin->parent->borderTile;
		if (parentPixmap == (PixmapPtr)USE_BORDER_PIXEL)
		{
		    pWin->borderTile = (PixmapPtr)USE_BORDER_PIXEL;
		    pWin->borderPixel = pWin->parent->borderPixel;
		}
                else
		{
		    CARD32 attribute;

		    pPixmap = (* pWin->drawable.pScreen->CreatePixmap)
				(pWin->drawable.pScreen, parentPixmap->width,
				 parentPixmap->height, pWin->drawable.depth);
		    pGC = GetScratchGC(pWin->drawable.depth, 
				   pWin->drawable.pScreen);
			
		    attribute = GXcopy;
		    ChangeGC(pGC, GCFunction, &attribute, 1);
		    ValidateGC(pPixmap, pGC);

		    (* pGC->CopyArea)(parentPixmap, pPixmap, pGC, 0, 0,
				       parentPixmap->width,
				       parentPixmap->height, 
				       pWin->drawable.depth,
				       0, 0);
		    pWin->borderTile = pPixmap;
		    FreeScratchGC(pGC);
		}
	    }
	    else
	    {	
		pPixmap = (PixmapPtr)LookupID(pixID, RT_PIXMAP, RC_CORE);
		if (pPixmap) 
		{
                    if  ((pPixmap->drawable.depth != pWin->drawable.depth) ||
			 (pPixmap->drawable.pScreen != pScreen))
		    {
			error = BadMatch;
			goto PatchUp;
		    }
		    (* pScreen->DestroyPixmap)(pWin->borderTile);
		    pWin->borderTile = pPixmap;
		    pPixmap->refcnt++;
		}
    	        else
		{
		    error = BadPixmap;
		    goto PatchUp;
		}
	    }
	    break;
	  case CWBorderPixel: 
            pWin->borderPixel = (CARD32) *pVlist;
	    (* pScreen->DestroyPixmap)(pWin->borderTile);	    
	    pWin->borderTile = (PixmapPtr)USE_BORDER_PIXEL;
		    /* border pixel overrides border pixmap,
		       so don't let the ddx layer see both bits */
	    vmaskCopy &= ~CWBorderPixmap;
	    pVlist++;
            break;
	  case CWBitGravity: 
            pWin->bitGravity = (CARD8 )*pVlist;
	    pVlist++;
	    break;
	  case CWWinGravity: 
            pWin->winGravity = (CARD8 )*pVlist;
	    pVlist++;
	    break;
	  case CWBackingStore: 
            pWin->backingStore = (CARD8 )*pVlist;
	    pVlist++;
	    break;
	  case CWBackingPlanes: 
	    pWin->backingBitPlanes = (CARD32) *pVlist;
	    pVlist++;
	    break;
	  case CWBackingPixel: 
            pWin->backingPixel = (CARD32)*pVlist;
	    pVlist++;
	    break;
	  case CWSaveUnder:
            pWin->saveUnder = (Bool) *pVlist;
	    pVlist++;
	    break;
	  case CWEventMask:
	    result = EventSelectForWindow(pWin, client, (long )*pVlist);
	    if (result)
	    {
		error = result;
		goto PatchUp;
	    }
	    pVlist++;
	    break;
	  case CWDontPropagate:
	    result =  EventSuppressForWindow(pWin, client, (long )*pVlist);
	    if (result)
	    {
		error = result;
		goto PatchUp;
	    }
	    pVlist++;
	    break;
	  case CWOverrideRedirect:
            pWin->overrideRedirect = (Bool ) *pVlist;
	    pVlist++;
	    break;
	  case CWColormap:
	    {
            Colormap	cmap;
	    ColormapPtr	pCmap;
	    xEvent	xE;
	    WindowPtr	pWinT;

	    cmap = (Colormap ) *pVlist;
	    pWinT = pWin;
	    while(cmap == CopyFromParent)
	    {
		if(pWinT->parent)
		{
		    if (pWinT->parent->colormap != CopyFromParent)
		    {
			cmap = pWinT->parent->colormap;
		    }
		    else
			pWinT = pWinT->parent;
		}
		else
		{
		    error = BadMatch;
		    goto PatchUp;
		}
	    }
	    pCmap = (ColormapPtr)LookupID(cmap, RT_COLORMAP, RC_CORE);
	    if (pCmap)
	    {
	        if (pCmap->pVisual->vid == pWin->visual)
	        { 
		    pWin->colormap = (Colormap ) cmap;
		    xE.u.u.type = ColormapNotify;
	            xE.u.colormap.new = TRUE;
	            xE.u.colormap.state = IsMapInstalled(cmap, pWin);
	            TraverseTree(pWin, TellNewMap, &xE);
		}
                else
		{
		    error = BadMatch;
		    goto PatchUp;
		}
	    }
            else
	    {
		error = BadColor;
		goto PatchUp;
	    }
	    pVlist++;
	    break;
	    }
	  case CWCursor:
	    cursorID = (Cursor ) *pVlist;
	    pVlist++;
	    /*
	     * install the new
	     */
	    if ( cursorID == None)
	    {
	        if ( pWin->cursor != None)
		    FreeCursor( pWin->cursor);
                if (pWin == &WindowTable[pWin->drawable.pScreen->myNum])
		   MakeRootCursor( pWin);
                else            
                    pWin->cursor = (CursorPtr)None;
	    }
            else
	    {
	        pCursor = (CursorPtr)LookupID(cursorID, RT_CURSOR, RC_CORE);
                if (pCursor) 
		{
    	            if ( pWin->cursor != None)
			FreeCursor( pWin->cursor);
                    pWin->cursor = pCursor;
                    pWin->cursor->refcnt++;
		}
	        else
		{
		    error = BadCursor;
		    goto PatchUp;
		}
	    }
	    WindowHasNewCursor( pWin);
	    break;
	 default: break;
      }
      vmaskCopy |= index;
    }
PatchUp:
    	/* We SHOULD check for an error value here XXX */
    (*pScreen->ChangeWindowAttributes)(pWin, vmaskCopy);

    /* 
        If the border pixel changed, redraw the border. 
	Note that this has to be done AFTER pScreen->ChangeWindowAttributes
        for the tile to be rotated, and the correct function selected.
    */
    if ((vmaskCopy & (CWBorderPixel | CWBorderPixmap)) 
	&& pWin->viewable && pWin->borderWidth)
    {
        (* pScreen->Subtract)(pWin->borderExposed, pWin->borderClip, 
			      pWin->winSize);
	(*pWin->PaintWindowBorder)(pWin, pWin->borderExposed, PW_BORDER);
        (* pScreen->RegionEmpty)(pWin->borderExposed);
    }
    return error;
}


/*****
 * GetWindowAttributes
 *    Notice that this is different than ChangeWindowAttributes
 *****/

GetWindowAttributes(pWin, client)
    WindowPtr pWin;
    ClientPtr client;
{
    xGetWindowAttributesReply wa;
    WindowPtr pWinT;

    wa.type = X_Reply;
    wa.bitGravity = pWin->bitGravity;
    wa.winGravity = pWin->winGravity;
    wa.backingStore  = pWin->backingStore;
    wa.length = (sizeof(xGetWindowAttributesReply) - 
		 sizeof(xGenericReply)) >> 2;
    wa.sequenceNumber = client->sequence;
    wa.backingBitPlanes =  pWin->backingBitPlanes;
    wa.backingPixel =  pWin->backingPixel;
    wa.saveUnder = (BOOL)pWin->saveUnder;
    wa.override = pWin->overrideRedirect;
    if (!pWin->mapped)
        wa.mapState = IsUnmapped;
    else if (pWin->realized)
        wa.mapState = IsViewable;
    else
        wa.mapState = IsUnviewable;

    pWinT = pWin;
    while (pWinT->colormap == (Colormap)CopyFromParent)
        pWinT = pWinT->parent;	
    wa.colormap =  pWinT->colormap;
    wa.mapInstalled = IsMapInstalled(wa.colormap, pWin);

    wa.yourEventMask = EventMaskForClient(pWin, client, &wa.allEventMasks);
    wa.doNotPropagateMask = pWin->dontPropagateMask ;
    wa.class = pWin->class;
    wa.visualID = pWin->visual;

    WriteReplyToClient(client, sizeof(xGetWindowAttributesReply), &wa);
}


static WindowPtr
MoveWindowInStack(pWin, pNextSib)
    WindowPtr pWin, pNextSib;
{
    WindowPtr pParent = pWin->parent;
    WindowPtr pFirstChange = pWin; /* highest window where list changes */

    if (pWin->nextSib != pNextSib)
    {
        if (!pNextSib)        /* move to bottom */
	{
            if (pParent->firstChild == pWin)
                pParent->firstChild = pWin->nextSib;
	    /* if (pWin->nextSib) */	 /* is always True: pNextSib == NULL
				          * and pWin->nextSib != pNextSib
					  * therefore pWin->nextSib != NULL */
	        pFirstChange = pWin->nextSib;
		pWin->nextSib->prevSib = pWin->prevSib;
	    /* else pFirstChange = pWin; */	
	    if (pWin->prevSib) 
                pWin->prevSib->nextSib = pWin->nextSib;
            pParent->lastChild->nextSib = pWin;
            pWin->prevSib = pParent->lastChild;
            pWin->nextSib = (WindowPtr )NULL;
            pParent->lastChild = pWin;
	}
        else if (pParent->firstChild == pNextSib) /* move to top */
        {        
	    pFirstChange = pWin;
	    if (pParent->lastChild == pWin)
    	       pParent->lastChild = pWin->prevSib;
	    if (pWin->nextSib) 
		pWin->nextSib->prevSib = pWin->prevSib;
	    if (pWin->prevSib) 
                pWin->prevSib->nextSib = pWin->nextSib;
	    pWin->nextSib = pParent->firstChild;
	    pWin->prevSib = (WindowPtr ) NULL;
	    pNextSib->prevSib = pWin;
	    pParent->firstChild = pWin;
	}
        else			/* move in middle of list */
        {
	    WindowPtr pOldNext = pWin->nextSib;

	    pFirstChange = (WindowPtr )NULL;
            if (pParent->firstChild == pWin)
                pFirstChange = pParent->firstChild = pWin->nextSib;
	    if (pParent->lastChild == pWin) {
	       pFirstChange = pWin;
    	       pParent->lastChild = pWin->prevSib;
	    }
	    if (pWin->nextSib) 
		pWin->nextSib->prevSib = pWin->prevSib;
	    if (pWin->prevSib) 
                pWin->prevSib->nextSib = pWin->nextSib;
            pWin->nextSib = pNextSib;
            pWin->prevSib = pNextSib->prevSib;
	    if (pNextSib->prevSib)
                pNextSib->prevSib->nextSib = pWin;
            pNextSib->prevSib = pWin;
	    if (!pFirstChange) {		     /* do we know it yet? */
	        pFirstChange = pParent->firstChild;  /* no, search from top */
	        while ((pFirstChange != pWin) && (pFirstChange != pOldNext))
		     pFirstChange = pFirstChange->nextSib;
	    }
	}
    }

    return( pFirstChange );
}

static void
MoveWindow(pWin, x, y, pNextSib)
    WindowPtr pWin;
    short x,y;
    WindowPtr pNextSib;
{
    WindowPtr pParent;
    Bool WasMapped = (Bool)(pWin->realized);
    BoxRec box;
    short oldx, oldy, bw;
    RegionPtr oldRegion;
    DDXPointRec oldpt;
    Bool anyMarked;
    register ScreenPtr pScreen;
    BoxPtr pBox;
    WindowPtr windowToValidate = pWin;

    /* if this is a root window, can't be moved */
    if (!(pParent = pWin->parent)) 
       return ;
    pScreen = pWin->drawable.pScreen;
    bw = pWin->borderWidth;

    oldx = pWin->absCorner.x;
    oldy = pWin->absCorner.y;
    oldpt.x = oldx;
    oldpt.y = oldy;
    if (WasMapped) 
    {
        oldRegion = (* pScreen->RegionCreate)(NULL, 1);
        (* pScreen->RegionCopy)(oldRegion, pWin->borderClip);
        pBox = (* pScreen->RegionExtents)(pWin->borderSize);
	anyMarked = MarkSiblingsBelowMe(pWin, pBox);
    }
    pWin->clientWinSize.x = x + bw;
    pWin->clientWinSize.y = y + bw;
    pWin->oldAbsCorner.x = oldx;
    pWin->oldAbsCorner.y = oldy;
    pWin->absCorner.x = pParent->absCorner.x + x +bw;
    pWin->absCorner.y = pParent->absCorner.y + y + bw;

    box.x1 = pWin->absCorner.x;
    box.y1 = pWin->absCorner.y;
    box.x2 = box.x1 + pWin->clientWinSize.width;
    box.y2 = box.y1+ pWin->clientWinSize.height;
    (* pScreen->RegionReset)(pWin->winSize, &box);
    (* pScreen->Intersect)(pWin->winSize, pWin->winSize, pParent->winSize); 

    if (bw)
    {
	box.x1 -= bw;
	box.y1 -= bw;
	box.x2 += bw;
	box.y2 += bw;
	(* pScreen->RegionReset)(pWin->borderSize, &box);
        (* pScreen->Intersect)(pWin->borderSize, pWin->borderSize, 
			       pParent->winSize);
    }
    else
        (* pScreen->RegionCopy)(pWin->borderSize, pWin->winSize);

    (* pScreen->PositionWindow)(pWin,pWin->absCorner.x, pWin->absCorner.y);

    windowToValidate = MoveWindowInStack(pWin, pNextSib);

    ResizeChildrenWinSize(pWin, FALSE, 0, 0, DONT_USE_GRAVITY);
    if (WasMapped) 
    {

        anyMarked = MarkSiblingsBelowMe(windowToValidate, pBox) || anyMarked;
            
        (* pScreen->ValidateTree)(pParent, (WindowPtr)NULL, TRUE, anyMarked);
	
	DoObscures(pParent); 
        if (pWin->backgroundTile == (PixmapPtr)ParentRelative)
            (* pScreen->RegionCopy)(pWin->exposed, pWin->clipList);
        else
        {
	    /*
	     * Why subtract borderSize from parent's exposures? parent
	     * exposures aren't supposed to extend into children!
	     */
       	    (* pWin->CopyWindow)(pWin, oldpt, oldRegion);
            (* pScreen->Subtract)(pParent->exposed, pParent->exposed,
                                  pWin->borderSize);
 
	}
	(* pScreen->RegionDestroy)(oldRegion);
	HandleExposures(pParent); 
    }    
}

static void
ResizeChildrenWinSize(pWin, XYSame, dw, dh, useGravity)
    WindowPtr pWin;
    Bool XYSame;
    int dw, dh;
    Bool useGravity;
{
    WindowPtr pSib;
    RegionPtr parentReg;
    BoxRec box;
    register short x, y, cwsx, cwsy;
    Bool unmap = FALSE;
    register ScreenPtr pScreen;
    xEvent event;

    pScreen = pWin->drawable.pScreen;
    parentReg = pWin->winSize;
    pSib = pWin->firstChild;
    x = pWin->absCorner.x;
    y = pWin->absCorner.y;

    while (pSib) 
    {
	cwsx = pSib->clientWinSize.x;
	cwsy = pSib->clientWinSize.y;
        if (useGravity == USE_GRAVITY)
        {
	    switch (pSib->winGravity)
	    {
	       case UnmapGravity: 
                    unmap = TRUE;
	       case NorthWestGravity: 
		    break;
               case NorthGravity:  
                   cwsx += dw/2;
		   break;
               case NorthEastGravity:    
		   cwsx += dw;	     
		   break;
               case WestGravity:         
                   cwsy += dh/2;
                   break;
               case CenterGravity:    
                   cwsx += dw/2;
		   cwsy += dh/2;
                   break;
               case EastGravity:         
                   cwsx += dw;
		   cwsy += dh/2;
                   break;
               case SouthWestGravity:    
		   cwsy += dh;
                   break;
               case SouthGravity:        
                   cwsx += dw/2;
		   cwsy += dh;
                   break;
               case SouthEastGravity:    
                   cwsx += dw;
		   cwsy += dh;
		   break;
               case StaticGravity:           /* XXX */
		   break;
	       default:
                   break;
	    }
	}

 	event.u.u.type = GravityNotify;
 	event.u.gravity.window = pSib->wid;
 	event.u.gravity.x = cwsx;
 	event.u.gravity.y = cwsy;
 	DeliverEvents (pSib, &event, 1, NullWindow);
 
	box.x1 = x + cwsx;
	box.y1 = y + cwsy;
	box.x2 = box.x1 + pSib->clientWinSize.width;
	box.y2 = box.y1 + pSib->clientWinSize.height;

	pSib->oldAbsCorner.x = pSib->absCorner.x;
	pSib->oldAbsCorner.y = pSib->absCorner.y;
	pSib->absCorner.x = x + cwsx;
	pSib->absCorner.y = y + cwsy;

	(* pScreen->RegionReset)(pSib->winSize, &box);
	(* pScreen->Intersect)(pSib->winSize, pSib->winSize, 
					      parentReg);

	if (pSib->borderWidth)
	{
	    box.x1 -= pSib->borderWidth;
	    box.y1 -= pSib->borderWidth;
	    box.x2 += pSib->borderWidth;
	    box.y2 += pSib->borderWidth;
	    (* pScreen->RegionReset)(pSib->borderSize, &box);
	    (* pScreen->Intersect)(pSib->borderSize, 
					 pSib->borderSize, parentReg);
	}
	else
	    (* pScreen->RegionCopy)(pSib->borderSize, pSib->winSize);
	(* pScreen->PositionWindow)(pSib, pSib->absCorner.x, pSib->absCorner.y);
	pSib->marked = 1;
	if (pSib->firstChild) 
            ResizeChildrenWinSize(pSib, XYSame, 0, 0, DONT_USE_GRAVITY);
        if (unmap)
	{
            UnmapWindow(pSib, DONT_HANDLE_EXPOSURES, SEND_NOTIFICATION,	TRUE);
	    unmap = FALSE;
	}
        pSib = pSib->nextSib;
    }
}

static int
ExposeAll(pWin, pScreen)
    WindowPtr pWin;
    ScreenPtr pScreen;
{
    if (!pWin)
        return(WT_NOMATCH);
    if (pWin->mapped)
    {
        (* pScreen->RegionCopy)(pWin->exposed, pWin->clipList);
        return (WT_WALKCHILDREN);
    }
    else
        return(WT_NOMATCH);
}



static void
SlideAndSizeWindow(pWin, x, y, w, h, pSib)
    WindowPtr pWin;
    short x,y;
    unsigned short w, h;
    WindowPtr pSib;
{
    WindowPtr pParent;
    Bool WasMapped = (Bool)(pWin->realized);
    BoxRec box;
    unsigned short width = pWin->clientWinSize.width,
                   height = pWin->clientWinSize.height;    
    short oldx = pWin->absCorner.x,
          oldy = pWin->absCorner.y,
          bw = pWin->borderWidth;
    short dw, dh;
    Bool XYSame = FALSE;
    DDXPointRec oldpt;
    RegionPtr oldRegion;
    Bool anyMarked;
    register ScreenPtr pScreen;
    BoxPtr pBox;
    WindowPtr pFirstChange;

    /* if this is a root window, can't be resized */
    if (!(pParent = pWin->parent)) 
        return ;

    pScreen = pWin->drawable.pScreen;
    if (WasMapped) 
    {
        if ((pWin->bitGravity != ForgetGravity) ||
            (pWin->backgroundTile == (PixmapPtr)ParentRelative))
            oldRegion = NotClippedByChildren(pWin);
        pBox = (* pScreen->RegionExtents)(pWin->borderSize);
	anyMarked = MarkSiblingsBelowMe(pWin, pBox);
    }
    pWin->clientWinSize.x = x + bw;
    pWin->clientWinSize.y = y + bw;
    pWin->clientWinSize.height = h;
    pWin->clientWinSize.width = w;
    XYSame = ((pParent->absCorner.x + x == pWin->absCorner.x) 
	      && (pParent->absCorner.y + y == pWin->absCorner.y));
    pWin->oldAbsCorner.x = oldx;
    pWin->oldAbsCorner.y = oldy;
    oldpt.x = oldx;
    oldpt.y = oldy;

    pWin->absCorner.x = pParent->absCorner.x + x + bw;
    pWin->absCorner.y = pParent->absCorner.y + y + bw;

    box.x1 = pWin->absCorner.x;
    box.y1 = pWin->absCorner.y;
    box.x2 = pWin->absCorner.x + w;
    box.y2 = pWin->absCorner.y + h;
    (* pScreen->RegionReset)(pWin->winSize, &box);
    (* pScreen->Intersect)(pWin->winSize, pWin->winSize, pParent->winSize);

    if (pWin->borderWidth)
    {
	box.x1 -= bw;
	box.y1 -= bw;
	box.x2 += bw;
	box.y2 += bw;
	(* pScreen->RegionReset)(pWin->borderSize, &box);
        (* pScreen->Intersect)(pWin->borderSize, pWin->borderSize, 
				     pParent->winSize);
    }
    else
        (* pScreen->RegionCopy)(pWin->borderSize, pWin->winSize);

    dw = w - width;
    dh = h - height;
    ResizeChildrenWinSize(pWin, XYSame, dw, dh, USE_GRAVITY);

    /* let the hardware adjust background and border pixmaps, if any */
    (* pScreen->PositionWindow)(pWin, pWin->absCorner.x, pWin->absCorner.y);

    pFirstChange = MoveWindowInStack(pWin, pSib);

    if (WasMapped) 
    {
        RegionPtr pRegion;

	anyMarked = MarkSiblingsBelowMe(pFirstChange, pBox) || anyMarked;

        if ((pWin->bitGravity == ForgetGravity) ||
            (pWin->backgroundTile == (PixmapPtr)ParentRelative))
	{
	    (* pScreen->ValidateTree)(pParent, pFirstChange, TRUE, anyMarked);
	    /* CopyWindow will step on borders, so re-paint them */
	    (* pScreen->Subtract)(pWin->borderExposed, 
			 pWin->borderClip, pWin->winSize);
	    TraverseTree(pWin, ExposeAll, pScreen); 
	    DoObscures(pParent); 
	    HandleExposures(pParent);
	}
	else
	{
	    pRegion = (* pScreen->RegionCreate)(NULL, 1);
	    (* pScreen->RegionCopy)(pRegion, pWin->clipList);

	    x = pWin->absCorner.x;
	    y = pWin->absCorner.y;
	    switch (pWin->bitGravity)
	    {
	      case NorthWestGravity: 
		    break;
              case NorthGravity:  
                   x += dw/2;
		   break;
              case NorthEastGravity:    
		   x += dw;	     
		   break;
              case WestGravity:         
                   y += dh/2;
                   break;
              case CenterGravity:    
                   x += dw/2;
		   y += dh/2;
                   break;
              case EastGravity:         
                   x += dw;
		   y += dh/2;
                   break;
              case SouthWestGravity:    
		   y += dh;
                   break;
              case SouthGravity:        
                   x += dw/2;
		   y += dh;
                   break;
              case SouthEastGravity:    
                   x += dw;
		   y += dh;
		   break;
	      case StaticGravity:
		   x = oldx;
		   y = oldy;
		   break;
	      default:
                   break;
	    }

	    (* pScreen->ValidateTree)(pParent, pFirstChange, TRUE, anyMarked);
	    DoObscures(pParent);
	    if (pWin->backStorage && (pWin->backingStore != NotUseful))
	    {
		ErrorF("Going to translate backing store %d %d\n",
		       oldx - x, oldy - y);
                (* pWin->backStorage->TranslateBackingStore) (pWin, 
							      oldx - x, oldy - y);
	    }
            oldpt.x = oldx - x + pWin->absCorner.x;
	    oldpt.y = oldy - y + pWin->absCorner.y;
	    (* pWin->CopyWindow)(pWin, oldpt, oldRegion);


	    /* Note that oldRegion is *translated* by CopyWindow */

	    /*
	     * We've taken care of those spots in oldRegion so they needn't
	     * be re-exposed...
	     */
	    (* pScreen->Subtract)(pWin->exposed, pWin->clipList, oldRegion);
	    (* pScreen->RegionDestroy)(pRegion);

	    /* CopyWindow will step on borders, so repaint them */
	    (* pScreen->Subtract)(pWin->borderExposed, 
				  pWin->borderClip, pWin->winSize);

	    HandleExposures(pParent);
	    (* pScreen->RegionDestroy)(oldRegion);
	}
    }
}

static void
ChangeBorderWidth(pWin, width)
    WindowPtr pWin;
    int width;
{
    WindowPtr pParent;
    BoxRec box;
    BoxPtr pBox;
    int oldwidth;
    Bool anyMarked;
    register ScreenPtr pScreen;
    RegionPtr oldRegion;
    DDXPointRec oldpt;
    Bool WasMapped = (Bool)(pWin->realized);

    oldwidth = pWin->borderWidth;
    if (oldwidth == width) 
        return ;
    pScreen = pWin->drawable.pScreen;
    pParent = pWin->parent;
    pWin->borderWidth = width;

    oldpt.x = pWin->absCorner.x;
    oldpt.y = pWin->absCorner.y;
    pWin->oldAbsCorner.x = oldpt.x;
    pWin->oldAbsCorner.y = oldpt.y;

    pWin->clientWinSize.x += width - oldwidth;
    pWin->clientWinSize.y += width - oldwidth;
    pWin->absCorner.x += width - oldwidth;
    pWin->absCorner.y += width - oldwidth;

    if (WasMapped)
    {
        oldRegion = (* pScreen->RegionCreate)(NULL, 1);
        (* pScreen->RegionCopy)(oldRegion, pWin->borderClip);
    }


    box.x1 = pWin->absCorner.x;
    box.y1 = pWin->absCorner.y;
    box.x2 = box.x1 + pWin->clientWinSize.width;
    box.y2 = box.y1 + pWin->clientWinSize.height;
    (* pScreen->RegionReset)(pWin->winSize, &box);
    (* pScreen->Intersect)(pWin->winSize, pWin->winSize, pParent->winSize);

    if (width)
    {
	box.x1 -= width;
	box.y1 -= width;
	box.x2 += width;
	box.y2 += width;
	(* pScreen->RegionReset)(pWin->borderSize, &box);
        (* pScreen->Intersect)(pWin->borderSize, pWin->borderSize, 
			       pParent->winSize);
    }
    else
        (* pScreen->RegionCopy)(pWin->borderSize, pWin->winSize);

    if (WasMapped)
    {
        if (width < oldwidth)
            pBox = (* pScreen->RegionExtents)(oldRegion);
        else        
            pBox = (* pScreen->RegionExtents)(pWin->borderSize);
        anyMarked = MarkSiblingsBelowMe(pWin, pBox);

        (* pScreen->ValidateTree)(pParent,(anyMarked ? pWin : (WindowPtr)NULL),
					     TRUE, anyMarked );  

        (* pWin->CopyWindow)(pWin, oldpt, oldRegion);

        if (width > oldwidth)
	{
            DoObscures(pParent);
	    (* pScreen->Subtract)(pWin->borderExposed,
				  pWin->borderClip, pWin->winSize);
	    (* pWin->PaintWindowBorder)(pWin, pWin->borderExposed, PW_BORDER);
	    (* pScreen->RegionEmpty)(pWin->borderExposed);
	}
	else if (oldwidth > width)
            HandleExposures(pParent);

	(* pScreen->RegionDestroy)(oldRegion);

    }
}


#define GET_INT16(m, f) \
  	if (m & mask) \
          { \
             f = (short) *pVlist;\
 	    pVlist++; \
         }
#define GET_CARD16(m, f) \
 	if (m & mask) \
         { \
            f = (CARD16) *pVlist;\
 	    pVlist++;\
         }

#define GET_CARD8(m, f) \
 	if (m & mask) \
         { \
            f = (CARD8) *pVlist;\
 	    pVlist++;\
         }

#define ChangeMask (CWX | CWY | CWWidth | CWHeight)

#define IllegalInputOnlyConfigureMask (CWBorderWidth)

/*
 * IsSiblingAboveMe
 *     returns Above if pSib above pMe in stack or Below otherwise 
 */

static int
IsSiblingAboveMe(pMe, pSib)
    WindowPtr pMe, pSib;
{
    WindowPtr pWin;

    pWin = pMe->parent->firstChild;
    while (pWin)
    {
        if (pWin == pSib)
            return(Above);
        else if (pWin == pMe)
            return(Below);
        pWin = pWin->nextSib;
    }
    return(Below);
}

static Bool
AnyWindowOverlapsMe(pWin, box)
    WindowPtr pWin;
    BoxPtr box;
{
    WindowPtr pSib;
    register ScreenPtr pScreen;

    pSib = pWin->parent->firstChild;
    pScreen = pWin->drawable.pScreen;
    while (pSib)
    {
        if (pSib == pWin)
            return(FALSE);
        else if ((pSib->mapped) && 
		 ((* pScreen->RectIn)(pSib->borderSize, box) != rgnOUT))
            return(TRUE);
        pSib = pSib->nextSib;
    }
    return(FALSE);
}

static WindowPtr
IOverlapAnyWindow(pWin, box)
    WindowPtr pWin;
    BoxPtr box;
{
    WindowPtr pSib;
    register ScreenPtr pScreen;

    pScreen = pWin->drawable.pScreen;
    pSib = pWin->nextSib;
    while (pSib)
    {
        if ((pSib->mapped) &&
	    ((* pScreen->RectIn)(pSib->borderSize, box) != rgnOUT))
            return(pSib);
        pSib = pSib->nextSib;
    }
    return((WindowPtr )NULL);
}

/*
 *   WhereDoIGoInTheStack() 
 *        Given pWin and pSib and the relationshipe smode, return
 *        the window that pWin should go ABOVE.
 *        If a pSib is specified:
 *            Above:  pWin is placed just above pSib
 *            Below:  pWin is placed just below pSib
 *            TopIf:  if pSib occludes pWin, then pWin is placed
 *                    at the top of the stack
 *            BottomIf:  if pWin occludes pSib, then pWin is 
 *                       placed at the bottom of the stack
 *            Opposite: if pSib occludes pWin, then pWin is placed at the
 *                      top of the stack, else if pWin occludes pSib, then
 *                      pWin is placed at the bottom of the stack
 *
 *        If pSib is NULL:
 *            Above:  pWin is placed at the top of the stack
 *            Below:  pWin is placed at the bottom of the stack
 *            TopIf:  if any sibling occludes pWin, then pWin is placed at
 *                    the top of the stack
 *            BottomIf: if pWin occludes any sibline, then pWin is placed at
 *                      the bottom of the stack
 *            Opposite: if any sibling occludes pWin, then pWin is placed at
 *                      the top of the stack, else if pWin occludes any
 *                      sibling, then pWin is placed at the bottom of the stack
 *
 */

static WindowPtr 
WhereDoIGoInTheStack(pWin, pSib, x, y, w, h, smode)
    WindowPtr pWin, pSib;
    short x, y, w, h;
    int smode;
{
    BoxRec box;
    register ScreenPtr pScreen;

    if ((pWin == pWin->parent->firstChild) && 
	(pWin == pWin->parent->lastChild))
        return((WindowPtr ) NULL);
    pScreen = pWin->drawable.pScreen;
    box.x1 = x;
    box.y1 = y;
    box.x2= x + w;
    box.y2 = y + h;
    switch (smode)
    {
      case Above:
        if (pSib)
           return(pSib);
        else if (pWin == pWin->parent->firstChild)
            return(pWin->nextSib);
        else
            return(pWin->parent->firstChild);
      case Below:
        if (pSib)
	    if (pSib->nextSib != pWin)
	        return(pSib->nextSib);
	    else
	        return(pWin->nextSib);
        else
            return((WindowPtr )NULL);
      case TopIf:
        if (pSib)
	{
            if ((IsSiblingAboveMe(pWin, pSib) == Above) &&
                ((* pScreen->RectIn)(pSib->borderSize, &box) != rgnOUT))
                return(pWin->parent->firstChild);
            else
                return(pWin->nextSib);
	}
        else if (AnyWindowOverlapsMe(pWin, &box))
            return(pWin->parent->firstChild);
        else
            return(pWin->nextSib);
      case BottomIf:
        if (pSib)
	{
            if ((IsSiblingAboveMe(pWin, pSib) == Below) &&
                ((* pScreen->RectIn)(pSib->borderSize, &box) != rgnOUT))
                return(WindowPtr)NULL;
            else
                return(pWin->nextSib);
	}
        else if (IOverlapAnyWindow(pWin, &box))
            return((WindowPtr)NULL);
        else
            return(pWin->nextSib);
      case Opposite:
        if (pSib)
	{
	    if ((* pScreen->RectIn)(pSib->borderSize, &box) != rgnOUT)
            {
                if (IsSiblingAboveMe(pWin, pSib) == Above)
                    return(pWin->parent->firstChild);
                else 
                    return((WindowPtr)NULL);
            }
            else
                return(pWin->nextSib);
	}
        else if (AnyWindowOverlapsMe(pWin, &box))
	{
	    /* If I'm occluded, I can't possibly be the first child
             * if (pWin == pWin->parent->firstChild)
             *    return pWin->nextSib;
	     */
            return(pWin->parent->firstChild);
	}
        else if (IOverlapAnyWindow(pWin, &box))
            return((WindowPtr)NULL);
        else
            return pWin->nextSib;
      default:
      {
        ErrorF("Internal error in ConfigureWindow, smode == %d\n",smode );
        return((WindowPtr)pWin->nextSib);
      }
    }
}

static void
ReflectStackChange(pWin, pSib)
    WindowPtr pWin, pSib;
{
/* Note that pSib might be NULL */

    Bool doValidation = (Bool)pWin->realized;
    WindowPtr pParent;
    int anyMarked;
    BoxPtr box;
    WindowPtr pFirstChange;

    /* if this is a root window, can't be restacked */
    if (!(pParent = pWin->parent))
        return ;

    pFirstChange = MoveWindowInStack(pWin, pSib);

    if (doValidation)
    {
        box = (* pWin->drawable.pScreen->RegionExtents)(pWin->borderSize);
        anyMarked = MarkSiblingsBelowMe(pFirstChange, box);
        (* pWin->drawable.pScreen->ValidateTree)(pParent, pFirstChange,
					 TRUE, anyMarked);
	DoObscures(pParent);
	HandleExposures(pParent);
    }
}

/*****
 * ConfigureWindow
 *****/


int 
ConfigureWindow(pWin, mask, vlist, client)
    WindowPtr pWin;
    unsigned long mask;
    long *vlist;
    ClientPtr client;
{
#define RESTACK_WIN    0
#define MOVE_WIN       1
#define RESIZE_WIN     2
    WindowPtr pSib = (WindowPtr )NULL;
    Window sibwid;
    int index, tmask;
    long *pVlist;
    short x,   y, beforeX, beforeY;
    unsigned short w = pWin->clientWinSize.width,
                   h = pWin->clientWinSize.height,
	           bw = pWin->borderWidth;
    int action, 
        smode = Above;
    xEvent event;

    if ((pWin->class == InputOnly) && (mask & IllegalInputOnlyConfigureMask))
        return(BadMatch);

    if ((mask & CWSibling) && !(mask & CWStackMode))
        return(BadMatch);

    pVlist = vlist;

    if (pWin->parent)
    {
        x = pWin->absCorner.x - pWin->parent->absCorner.x - bw;
        y = pWin->absCorner.y - pWin->parent->absCorner.y - bw;
    }
    else
    {
        x = pWin->absCorner.x;
        y = pWin->absCorner.y;
    }
    beforeX = x;
    beforeY = y;
    action = RESTACK_WIN;	
    if ((mask & (CWX | CWY)) && (!(mask & (CWHeight | CWWidth))))
    {
	GET_INT16(CWX, x);
 	GET_INT16(CWY, y);
	action = MOVE_WIN;
    }
	/* or should be resized */
    else if (mask & (CWX |  CWY | CWWidth | CWHeight))
    {
	GET_INT16(CWX, x);
	GET_INT16(CWY, y);
	GET_CARD16(CWWidth, w);
	GET_CARD16 (CWHeight, h);
	if (!w || !h)
            return BadValue;
        action = RESIZE_WIN;
    }
    tmask = mask & ~ChangeMask;
    while (tmask) 
    {
	index = ffs(tmask) - 1;
	tmask &= ~(index = (1 << index));
	switch (index) 
        {
          case CWBorderWidth:   
	    GET_CARD16(CWBorderWidth, bw);
	    break;
          case CWSibling: 
	    sibwid = (Window ) *pVlist;
	    pVlist++;
            pSib = (WindowPtr )LookupID(sibwid, RT_WINDOW, RC_CORE);
            if (!pSib)
                return(BadWindow);
            if (pSib->parent != pWin->parent)
		return(BadMatch);
	    if (pSib == pWin)
	        return(BadMatch);
	    break;
          case CWStackMode:
	    GET_CARD8(CWStackMode, smode);
	    if ((smode != TopIf) && (smode != BottomIf) &&
 		(smode != Opposite) && (smode != Above) && (smode != Below))
                   return(BadMatch);
	    break;
	  default: 
	    return(BadMatch);
	}
    }
	/* root really can't be reconfigured, so just return */
    if (!pWin->parent)    
	return Success;

        /* Figure out if the window should be moved.  Doesnt
           make the changes to the window if event sent */

    if (mask & CWStackMode)
        pSib = WhereDoIGoInTheStack(pWin, pSib, x, y, w, h, smode);
    else
        pSib = pWin->nextSib;

    if ((!pWin->overrideRedirect) && 
        (pWin->parent->allEventMasks & SubstructureRedirectMask))
    {
	event.u.u.type = ConfigureRequest;
	event.u.configureRequest.window = pWin->wid;
	event.u.configureRequest.parent = pWin->parent->wid;
        if (mask & CWSibling)
	   event.u.configureRequest.sibling = sibwid;
        else
       	    event.u.configureRequest.sibling = None;
        if (mask & CWStackMode)
	   event.u.u.detail = smode;
        else
       	    event.u.u.detail = Above;
	event.u.configureRequest.x = x;
	event.u.configureRequest.y = y;
	event.u.configureRequest.width = w;
	event.u.configureRequest.height = h;
	event.u.configureRequest.borderWidth = bw; 
	event.u.configureRequest.valueMask = mask;
	if (MaybeDeliverEventsToClient(pWin->parent, &event, 1, 
	        SubstructureRedirectMask, client) == 1)
    	    return(Success);            
    }
    if (action == RESIZE_WIN) {
        Bool size_change = (w != pWin->clientWinSize.width)
                        || (h != pWin->clientWinSize.height);
	if (size_change && (pWin->allEventMasks & ResizeRedirectMask)) {
	    xEvent eventT;
    	    eventT.u.u.type = ResizeRequest;
    	    eventT.u.resizeRequest.window = pWin->wid;
	    eventT.u.resizeRequest.width = w;
	    eventT.u.resizeRequest.height = h;
	    if (MaybeDeliverEventsToClient(pWin, &eventT, 1, 
				       ResizeRedirectMask, client) == 1) {
                /* if event is delivered, leave the actual size alone. */
	        w = pWin->clientWinSize.width;
	        h = pWin->clientWinSize.height;
                size_change = FALSE;
		}
	    }
        if (!size_change) {
	    if (mask & (CWX | CWY))
    	        action = MOVE_WIN;
	    else if (mask & (CWStackMode | CWSibling | CWBorderWidth))
	        action = RESTACK_WIN;
            else   /* really nothing to do */
                return(Success) ;        
	    }
        }

    if (action == RESIZE_WIN)
            /* we've already checked whether there's really a size change */
            goto ActuallyDoSomething;
    if ((mask & CWX) && (x != beforeX))
            goto ActuallyDoSomething;
    if ((mask & CWY) && (y != beforeY))
            goto ActuallyDoSomething;
    if ((mask & CWBorderWidth) && (bw != pWin->borderWidth))
            goto ActuallyDoSomething;
    if (mask & CWStackMode) 
    {
        if (pWin->nextSib != pSib)
            goto ActuallyDoSomething;
    }
    return(Success);

ActuallyDoSomething:
    event.u.u.type = ConfigureNotify;
    event.u.configureNotify.window = pWin->wid;
    if (pSib)
        event.u.configureNotify.aboveSibling = pSib->wid;
    else
        event.u.configureNotify.aboveSibling = None;
    event.u.configureNotify.x = x;
    event.u.configureNotify.y = y;
    event.u.configureNotify.width = w;
    event.u.configureNotify.height = h;
    event.u.configureNotify.borderWidth = bw;
    event.u.configureNotify.override = pWin->overrideRedirect;
    DeliverEvents(pWin, &event, 1, NullWindow);

    if (mask & CWBorderWidth) 
    {
        if (action == RESTACK_WIN)
            ChangeBorderWidth(pWin, bw);
        else
            pWin->borderWidth = bw;
    }
    if (action == MOVE_WIN)
        MoveWindow(pWin, x, y, pSib);
    else if (action == RESIZE_WIN)
        SlideAndSizeWindow(pWin, x, y, w, h, pSib);
    else if (mask & CWStackMode)
        ReflectStackChange(pWin, pSib);

    return(Success);
#undef RESTACK_WIN    
#undef MOVE_WIN   
#undef RESIZE_WIN  
}


/******
 *
 * CirculateWindow
 *    For RaiseLowest, raises the lowest mapped child (if any) that is
 *    obscured by another child to the top of the stack.  For LowerHighest,
 *    lowers the highest mapped child (if any) that is obscuring another
 *    child to the bottom of the stack.  Exposure processing is performed 
 *
 ******/

/* XXX shouldn't this be a case for the stack mode stuff?? */

int
CirculateWindow(pParent, direction, client)
    WindowPtr pParent;
    int direction;
    ClientPtr client;
{
    WindowPtr pChild, pSib;
    xEvent event;
    register ScreenPtr pScreen;

    if (pParent->firstChild == pParent->lastChild) 
        return(Success) ;

    pScreen = pParent->drawable.pScreen;
    if (direction == RaiseLowest)
    {
        pChild = pParent->lastChild;
    	while (pChild)
	{
	    if (pChild->mapped && (pChild->visibility != VisibilityUnobscured))
	    {
                if (pParent->firstChild != pChild)
                {        
		    if (pParent->lastChild == pChild)
			pParent->lastChild = pChild->prevSib;
                    if (pChild->nextSib) 
                        pChild->nextSib->prevSib = pChild->prevSib;
                    if (pChild->prevSib) 
                        pChild->prevSib->nextSib = pChild->nextSib;
                    pChild->nextSib = pParent->firstChild;
                    pChild->prevSib = (WindowPtr ) NULL;
		    pParent->firstChild->prevSib = pChild;
                    pParent->firstChild = pChild;
		    pChild->mapped = 0;   /* to fool MapWindow */
		    pSib = pChild;
		    break;
		}
		return Success;
	    }
	    pChild = pChild->prevSib;
	}
    }
    else if (direction == LowerHighest)
    {
        BoxPtr pBox;

        pChild = pParent->firstChild;
    	while (pChild)
	{
	    if (pChild->mapped && (pChild->visibility == VisibilityUnobscured))
	    {
		int result;

                   /* HACK -- this search is n squared */

		pSib = pChild->nextSib;

                   /* find a sibling that pChild overlaps */

                pBox = (* pScreen->RegionExtents)(pChild->borderSize);
		while (pSib)
	        {
		    if (pSib->realized && (pSib->visibility != VisibilityUnobscured))
		    {
			result = (* pScreen->RectIn)(pSib->borderSize, 
					&pBox);
			if (result != rgnOUT) 
			    break;
		    }
		    pSib = pSib->nextSib;
		}
                if (pSib)
		{
		    WindowPtr pNext;
		    pNext = pChild->nextSib;
		    if (pParent->firstChild == pChild)
			pParent->firstChild = pNext;
                    pNext->prevSib = pChild->prevSib;
                    if (pChild->prevSib) 
                        pChild->prevSib->nextSib = pNext;
                    pChild->nextSib = (WindowPtr ) NULL;
                    pChild->prevSib = pParent->lastChild;
		    pParent->lastChild->nextSib = pChild;
                    pParent->lastChild = pChild;
		    pSib->mapped = 0;  /* to fool mapWindow */
    		    break;
		}
	    }
	    pChild = pChild->nextSib;
	}
    }
    if (!pChild)
        return(Success) ;
    event.u.circulate.window = pChild->wid;
    event.u.circulate.parent = pParent->wid;
    event.u.circulate.event = pParent->wid;
    if (direction == RaiseLowest)
	event.u.circulate.place = PlaceOnTop;
    else
        event.u.circulate.place = PlaceOnBottom;

    if (pParent->allEventMasks & SubstructureRedirectMask)
    {
	event.u.u.type = CirculateRequest;
	if (MaybeDeliverEventsToClient(pParent, &event, 1, 
	        SubstructureRedirectMask, client) == 1)
    	    return(Success);            
    }
    if (pParent->realized)
        MapWindow(pSib, HANDLE_EXPOSURES, BITS_DISCARDED,
	      DONT_SEND_NOTIFICATION, client);
    event.u.u.type = CirculateNotify;
    DeliverEvents(pParent, &event, 1, NullWindow);
    return(Success);
}

/*****
 *  ReparentWindow
 *****/

static int
CompareWIDs(pWin, wid)
    WindowPtr pWin;
    int *wid;
{
    if (pWin->wid == *wid) 
       return(WT_STOPWALKING);
    else
       return(WT_WALKCHILDREN);
}

int 
ReparentWindow(pWin, pParent, x, y, client)
    WindowPtr pWin, pParent;
    short x,y;
    ClientPtr client;
{
    WindowPtr pPrev;
    Bool WasMapped = (Bool)(pWin->realized);
    BoxRec box;
    xEvent event;
    short oldx, oldy;
    int bw;
    register ScreenPtr pScreen;
    
    if (pWin == pParent)
        return(BadWindow);
    if (TraverseTree(pWin, CompareWIDs, &pParent->wid) == WT_STOPWALKING)
        return(BadWindow);		

    pScreen = pWin->drawable.pScreen;
    event.u.u.type = ReparentNotify;
    event.u.reparent.window = pWin->wid;
    event.u.reparent.parent = pParent->wid;
    event.u.reparent.x = x;
    event.u.reparent.y = y;
    event.u.reparent.override = pWin->overrideRedirect;
    DeliverEvents(pWin, &event, 1, pParent);

    oldx = pWin->absCorner.x;
    oldy = pWin->absCorner.y;
    if (WasMapped) 
       UnmapWindow(pWin, HANDLE_EXPOSURES, SEND_NOTIFICATION, FALSE);

    /* take out of sibling chain */

    pPrev = pWin->parent;
    if (pPrev->firstChild == pWin)
        pPrev->firstChild = pWin->nextSib;
    if (pPrev->lastChild == pWin)
        pPrev->lastChild = pWin->prevSib;

    if (pWin->nextSib) 
        pWin->nextSib->prevSib = pWin->prevSib;
    if (pWin->prevSib) 
        pWin->prevSib->nextSib = pWin->nextSib;

    /* insert at begining of pParent */
    pWin->parent = pParent;
    pWin->nextSib = pParent->firstChild;
    pWin->prevSib = (WindowPtr )NULL;
    if (pParent->firstChild) 
	pParent->firstChild->prevSib = pWin;
    else
        pParent->lastChild = pWin;
    pParent->firstChild = pWin;

    /* clip to parent */
    box.x1 = x + pParent->absCorner.x;
    box.y1 = y + pParent->absCorner.y;
    box.x2 = box.x1 + pWin->clientWinSize.width;
    box.y2 = box.y1+ pWin->clientWinSize.height;
    (* pScreen->RegionReset)(pWin->winSize, &box);
    (* pScreen->Intersect)(pWin->winSize, pWin->winSize, 
					  pParent->winSize); 

    pWin->clientWinSize.x = x;
    pWin->clientWinSize.y = y;
    pWin->oldAbsCorner.x = oldx;
    pWin->oldAbsCorner.y = oldy;
    pWin->absCorner.x = box.x1;
    pWin->absCorner.y = box.y1;

    if (bw = pWin->borderWidth)
    {
	box.x1 -= bw;
	box.y1 -= bw;
	box.x2 += bw;
	box.y2 += bw;
	(* pScreen->RegionReset)(pWin->borderSize, &box);
        (* pScreen->Intersect)(pWin->borderSize, pWin->borderSize, 
			       pParent->winSize);
    }
    else
        (* pScreen->RegionCopy)(pWin->borderSize, pWin->winSize);

    (* pScreen->PositionWindow)(pWin, pWin->absCorner.x, pWin->absCorner.y);
    ResizeChildrenWinSize(pWin, FALSE, 0, 0, DONT_USE_GRAVITY);

    if (WasMapped)    
    {
        MapWindow(pWin, HANDLE_EXPOSURES, BITS_DISCARDED, SEND_NOTIFICATION,
	                                    client);
    }
    RecalculateDeliverableEvents(pParent);
    return(Success);
}

static int
MarkChildren(pWin, box)
    WindowPtr pWin;
    BoxPtr box;
{
    WindowPtr pChild;
    int anyMarked=0;
    register ScreenPtr pScreen;

    pScreen = pWin->drawable.pScreen;
    pChild = pWin->firstChild;
    while (pChild) 
    {
        if (pChild->mapped && ((* pScreen->RectIn)(pChild->borderSize, box)))
        {
            anyMarked++;
	    pChild->marked = 1;
	    anyMarked += MarkChildren(pChild, box);
	}
	pChild = pChild->nextSib;
    }
    return(anyMarked);
}

static int
MarkSiblingsBelowMe(pWin, box)
    WindowPtr pWin;
    BoxPtr box;
{
    WindowPtr pSib;
    int anyMarked = 0;
    register ScreenPtr pScreen;

    pScreen = pWin->drawable.pScreen;

    pSib = pWin;
    while (pSib) 
    {
        if (pSib->mapped && ((* pScreen->RectIn)(pSib->borderSize, box)))
        {
	    pSib->marked = 1;
	    anyMarked++;
            if (pSib->firstChild)
    	        anyMarked += MarkChildren(pSib, box);
	}
	pSib = pSib->nextSib;
    }
    return(anyMarked);
}    


/*****
 * MapWindow
 *    If some other client has selected SubStructureReDirect on the parent
 *    and override-redirect is xFalse, then a MapRequest event is generated,
 *    but the window remains unmapped.  Otherwise, the window is mapped and a
 *    MapNotify event is generated.
 *****/

static void
RealizeChildren(pWin, client)
    WindowPtr pWin;
    ClientPtr client;
{
    WindowPtr pSib;
    Bool (* Realize)();

    pSib = pWin;
    if (pWin)
       Realize = pSib->drawable.pScreen->RealizeWindow;
    while (pSib)
    {
        if (pSib->mapped)
	{
	    pSib->realized = TRUE;
            pSib->viewable = pSib->class == InputOutput;
            (* Realize)(pSib);
	    FlushClientCaches(pSib->wid);
            if (pSib->firstChild) 
                RealizeChildren(pSib->firstChild, client);
	}
        pSib = pSib->nextSib;
    }
}

int
MapWindow(pWin, SendExposures, BitsAvailable, SendNotification, client)

    WindowPtr pWin;
    Bool SendExposures;
    Bool BitsAvailable;
    ClientPtr client;
{
    register ScreenPtr pScreen;

    WindowPtr pParent;
    Bool anyMarked;

    if (pWin->mapped) 
        return(Success);
    pScreen = pWin->drawable.pScreen;
    if (pParent = pWin->parent)
    {
        xEvent event;
        BoxPtr box;

        if (SendNotification && (!pWin->overrideRedirect) && 
	    (pParent->allEventMasks & SubstructureRedirectMask))
	{
	    event.u.u.type = MapRequest;
	    event.u.mapRequest.window = pWin->wid;
	    event.u.mapRequest.parent = pParent->wid;

	    if (MaybeDeliverEventsToClient(pParent, &event, 1, 
	        SubstructureRedirectMask, client) == 1)
    	        return(Success);            
	}

	pWin->mapped = 1;          
        if (SendNotification)
	{
	    event.u.u.type = MapNotify;
	    event.u.mapNotify.window = pWin->wid;
	    event.u.mapNotify.override = pWin->overrideRedirect;
	    DeliverEvents(pWin, &event, 1, NullWindow);
	}

	pWin->marked = 0;         /* so siblings get mapped correctly */
        if (!pParent->realized)
            return(Success);
        pWin->realized = TRUE;
        pWin->viewable = pWin->class == InputOutput;
    	/* We SHOULD check for an error value here XXX */
        (* pScreen->RealizeWindow)(pWin);
        if (pWin->firstChild)
            RealizeChildren(pWin->firstChild, client);    
        box = (* pScreen->RegionExtents)(pWin->borderSize);
        anyMarked = MarkSiblingsBelowMe(pWin, box);

	/* kludge; remove when miregion works! */
        if (!pParent->parent && 
	    (box->x1 == pParent->winSize->extents.x1) &&
	    (box->y1 == pParent->winSize->extents.y1) &&
	    (box->x2 == pParent->winSize->extents.x2) &&
	    (box->y2 == pParent->winSize->extents.y2) &&
	    (pParent->firstChild == pWin)) {
	  (*pWin->drawable.pScreen->RegionCopy)(pParent->clipList,
						pParent->winSize);
	}
	/* end of kludge */

	(* pScreen->ValidateTree)(pParent, pWin, TRUE, anyMarked);
        if (SendExposures) 
        {
	    if (!BitsAvailable)
	    {
    	        (* pScreen->RegionCopy)(pWin->exposed, pWin->clipList);  
		DoObscures(pParent);
		HandleExposures(pWin);
	    }
	    else
	    {
		/*
		 * Children shouldn't be in the parent's exposed region...
                (* pScreen->Subtract)(pParent->exposed, pParent->exposed, 
				      pWin->borderSize);
		 */
                (* pScreen->RegionEmpty)(pWin->exposed);
		DoObscures(pParent);
		HandleExposures(pWin);
	    }
	}
    }
    else
    {
	pWin->mapped = 1;
        pWin->realized = TRUE;     /* for roots */
        pWin->viewable = pWin->class == InputOutput;
    	/* We SHOULD check for an error value here XXX */
        (* pScreen->RealizeWindow)(pWin);
    }
    
    return(Success);
}

 
/*****
 * MapSubwindows
 *    Performs a MapWindow all unmapped children of the window, in top
 *    to bottom stacking order.
 *****/

MapSubwindows(pWin, SendExposures, client)
    WindowPtr pWin;
    Bool SendExposures;
    ClientPtr client;
{
    WindowPtr pChild;

    pChild = pWin->firstChild;
    while (pChild) 
    {
	if (!pChild->mapped) 
	    /* What about backing store? */
            MapWindow(pChild, SendExposures, BITS_DISCARDED, 
		      SEND_NOTIFICATION, client);
        pChild = pChild->nextSib;
    }
}

/*****
 * UnmapWindow
 *    If the window is already unmapped, this request has no effect.
 *    Otherwise, the window is unmapped and an UnMapNotify event is
 *    generated.  Cannot unmap a root window.
 *****/
 
static void
UnrealizeChildren(pWin)
    WindowPtr pWin;
{
    WindowPtr pSib;
    void (*RegionEmpty)();
    Bool (*Unrealize)();
    
    pSib = pWin;
    if (pWin)
    {
        RegionEmpty = pWin->drawable.pScreen->RegionEmpty;
        Unrealize = pWin->drawable.pScreen->UnrealizeWindow;
    }
    while (pSib)
    {
	pSib->realized = pSib->viewable = FALSE;
        (* Unrealize)(pSib);
	        /* to force exposures later */
        (* RegionEmpty)(pSib->clipList);    
        (* RegionEmpty)(pSib->borderClip);
	(* RegionEmpty)(pSib->exposed);
	pWin->drawable.serialNumber = NEXT_SERIAL_NUMBER;
        DeleteWindowFromAnyEvents(pSib, FALSE);
        if (pSib->firstChild) 
            UnrealizeChildren(pSib->firstChild);
        pSib = pSib->nextSib;
    }
}

UnmapWindow(pWin, SendExposures, SendNotification, fromConfigure)
    WindowPtr pWin;
    Bool SendExposures, fromConfigure;
{
    WindowPtr pParent;
    xEvent event;
    Bool anyMarked;
    Bool wasMapped = (Bool)pWin->realized;
    BoxPtr box;

    if ((!pWin->mapped) || (!(pParent = pWin->parent))) 
        return(Success);
    if (SendNotification)
    {
	event.u.u.type = UnmapNotify;
	event.u.unmapNotify.window = pWin->wid;
	event.u.unmapNotify.fromConfigure = fromConfigure;
	DeliverEvents(pWin, &event, 1, NullWindow);
    }
    if (wasMapped)
    {
        box = (* pWin->drawable.pScreen->RegionExtents)(pWin->borderSize);
        anyMarked = MarkSiblingsBelowMe(pWin, box);
    }
    pWin->mapped = 0;
    pWin->realized = pWin->viewable = FALSE;
    if (wasMapped)
    {
    	/* We SHOULD check for an error value here XXX */
        (* pWin->drawable.pScreen->UnrealizeWindow)(pWin);
        DeleteWindowFromAnyEvents(pWin, FALSE);
        if (pWin->firstChild)
            UnrealizeChildren(pWin->firstChild);
        (* pWin->drawable.pScreen->ValidateTree)(pParent, pWin, 
						 TRUE, anyMarked);
        if (SendExposures)
        {
	    HandleExposures(pParent);
	}
    }        
    return(Success);
}

/*****
 * UnmapSubwindows
 *    Performs an UnMapWindow request with the specified mode on all mapped
 *    children of the window, in bottom to top stacking order.
 *
 *    XXX: Should use the validation function, not mess with the clip lists
 *    directly, though this way is faster.
 *****/

UnmapSubwindows(pWin, sendExposures)
    WindowPtr pWin;
    Bool sendExposures;
{
    WindowPtr pChild;
    xEvent event;
    void (*RegionEmpty)();
    Bool (*UnrealizeWindow)();
    Bool wasMapped = (Bool)pWin->realized;

    pChild = pWin->lastChild;
    event.u.u.type = UnmapNotify;
    event.u.unmapNotify.fromConfigure = xFalse;
    RegionEmpty = pWin->drawable.pScreen->RegionEmpty;
    UnrealizeWindow = pWin->drawable.pScreen->UnrealizeWindow;    
    while (pChild) 
    {
	if (pChild->mapped) 
        {
	    event.u.unmapNotify.window = pChild->wid;
	    event.u.unmapNotify.fromConfigure = xFalse;
	    DeliverEvents(pWin, &event, 1, NullWindow);
	    pChild->mapped = 0;
            if (wasMapped)
	    {
    	        pChild->realized = pChild->viewable = FALSE;
    		/* We SHOULD check for an error value here XXX */
                (* UnrealizeWindow)(pChild);
	        if (pChild->firstChild)
                    UnrealizeChildren(pChild->firstChild);
                DeleteWindowFromAnyEvents(pChild, FALSE);
	        (* RegionEmpty)(pChild->clipList);/* to force expsoures later*/
    	        (* RegionEmpty)(pChild->borderClip);
	        (* RegionEmpty)(pChild->borderExposed);
	        (* RegionEmpty)(pChild->exposed);      
	        pChild->drawable.serialNumber = NEXT_SERIAL_NUMBER;
	    }
	}
        pChild = pChild->prevSib;
    }
    if ((pWin->lastChild) && wasMapped)
    {
        (* pWin->drawable.pScreen->Intersect)(pWin->clipList,
				      pWin->winSize, pWin->borderClip);
        (* pWin->drawable.pScreen->RegionCopy)(pWin->exposed, pWin->clipList);
        HandleExposures(pWin);    		
    }
}


void
HandleSaveSet(client)
    ClientPtr client;
{
    WindowPtr pParent, pWin;
    int j;

    for (j=0; j<client->numSaved; j++)
    {
        pWin = (WindowPtr)client->saveSet[j]; 
        pParent = pWin->parent;
        while (pParent && (pParent->client == client))
            pParent = pParent->parent;
        if (pParent)
	{
            ReparentWindow(pWin, pParent, pWin->absCorner.x, 
			   pWin->absCorner.y, client);
	    if(!pWin->realized && pWin->mapped)
		pWin->mapped = FALSE;
            MapWindow(pWin, HANDLE_EXPOSURES, BITS_DISCARDED,
	                    SEND_NOTIFICATION, client);
	}
    }
}
    
Bool
VisibleBoundingBoxFromPoint(pWin, x, y, box)
    WindowPtr pWin;
    int x, y;   /* in root */
    BoxPtr box;   /* "return" value */
{
    if (!pWin->realized)
	return (FALSE);
    if ((* pWin->drawable.pScreen->PointInRegion)(pWin->clipList, x, y, box))
        return(TRUE);
    return(FALSE);
}

Bool
PointInWindowIsVisible(pWin, x, y)
    WindowPtr pWin;
    int x, y;	/* in root */
{
    BoxRec box;

    if (!pWin->realized)
	return (FALSE);
    if ((* pWin->drawable.pScreen->PointInRegion)(pWin->clipList, x, y, &box))
        return(TRUE);
    return(FALSE);
}


RegionPtr 
NotClippedByChildren(pWin)
    WindowPtr pWin;
{
    register ScreenPtr pScreen;
    RegionPtr pReg;

    pScreen = pWin->drawable.pScreen;
    pReg = (* pScreen->RegionCreate)(NULL, 1);
    (* pScreen->Intersect) (pReg, pWin->borderClip, pWin->winSize);
    return(pReg);
}


void
SendVisibilityNotify(pWin)
    WindowPtr pWin;
{
    xEvent event;
    event.u.u.type = VisibilityNotify;
    event.u.visibility.window = pWin->wid;
    event.u.visibility.state = pWin->visibility;
    DeliverEvents(pWin, &event, 1, NullWindow);
}


#define RANDOM_WIDTH 32

void
SaveScreens(on, mode)
    int on;
    int mode;
{
    int i, j;
    int what;
    unsigned char *srcbits, *mskbits;

    if (on == SCREEN_SAVER_FORCER)
    {
        if (mode == ScreenSaverReset)
            what = SCREEN_SAVER_OFF;
        else               
           what = SCREEN_SAVER_ON;
	if (what == screenIsSaved)
            return ;
    }
    else
        what = on;
    for (i = 0; i < screenInfo.numScreens; i++)
    {
        if (on == SCREEN_SAVER_FORCER)
        {
           (* screenInfo.screen[i].SaveScreen) (&screenInfo.screen[i], on);
        }
        if (what == SCREEN_SAVER_OFF)
        {
	    if (savedScreenInfo[i].blanked == SCREEN_IS_BLANKED)
	    {
	       (* screenInfo.screen[i].SaveScreen) (&screenInfo.screen[i], on);
	    }
            else if (savedScreenInfo[i].blanked == SCREEN_IS_TILED)
	    {
    	        FreeResource(savedScreenInfo[i].wid, RC_NONE);
                savedScreenInfo[i].pWindow = (WindowPtr)NULL;
    	        FreeResource(savedScreenInfo[i].cid, RC_NONE);
	    }
	    continue;
        }
        else if (what == SCREEN_SAVER_ON) 
        {
            if (screenIsSaved == SCREEN_SAVER_ON)  /* rotate pattern */
            {
	        int new_x, new_y;

		if (savedScreenInfo[i].blanked == SCREEN_IS_TILED)
	        {
	            new_x = random() % RANDOM_WIDTH;
	            new_y = random() % RANDOM_WIDTH;
	            MoveWindow(savedScreenInfo[i].pWindow, -new_x, -new_y, 
		               savedScreenInfo[i].pWindow->nextSib);
		}
		continue;
	    }
            if (ScreenSaverBlanking != DontPreferBlanking) 
	    {
	       if ((* screenInfo.screen[i].SaveScreen)
		   (&screenInfo.screen[i], what))
	       {
	           savedScreenInfo[i].blanked = SCREEN_IS_BLANKED;
                   continue;
	       }
	    }
            if (ScreenSaverAllowExposures != DontAllowExposures)
            {
                int result;
                long attributes[1];
	        int mask = CWBackPixmap;
                WindowPtr pWin;		
		CursorMetricRec cm;
                
                if (WindowTable[i].backgroundTile == 
		    (PixmapPtr)USE_BACKGROUND_PIXEL)
		{
                    attributes[0] = WindowTable[i].backgroundPixel;
		    mask = CWBackPixel;
		}
                else
                    attributes[0] = None;

                pWin = savedScreenInfo[i].pWindow = 
    			/* We SHOULD check for an error value here XXX */
		     CreateWindow(savedScreenInfo[i].wid,
		     &WindowTable[i], 
		     -RANDOM_WIDTH, -RANDOM_WIDTH,
		     screenInfo.screen[i].width + RANDOM_WIDTH, 
		     screenInfo.screen[i].height + RANDOM_WIDTH,
		     0, InputOutput, mask, attributes, 0, 0, 
		     WindowTable[i].visual, &result);
                if (attributes[0] == None)
		{
		    
		    pWin->backgroundTile = pWin->parent->backgroundTile;
		    pWin->backgroundTile->refcnt++;
		    (* screenInfo.screen[i].ChangeWindowAttributes)
				(pWin, CWBackPixmap);
		}
	        AddResource(pWin->wid, RT_WINDOW, 
			savedScreenInfo[i].pWindow,
			DeleteWindow, RC_CORE);
		cm.width=32;
		cm.height=16;
		cm.xhot=8;
		cm.yhot=8;
                srcbits = (unsigned char *)Xalloc( PixmapBytePad(32, 1)*16); 
		mskbits = (unsigned char *)Xalloc( PixmapBytePad(32, 1)*16); 
                for (j=0; j<PixmapBytePad(32, 1)*16; j++)
    	            srcbits[j] = mskbits[j] = 0x0;
		pWin->cursor = AllocCursor( srcbits, mskbits, &cm,
		    ~0, ~0, ~0, 0, 0, 0);
		AddResource(savedScreenInfo[i].cid, RT_CURSOR,
			pWin->cursor,
			FreeCursor, RC_CORE);	
 		pWin->cursor->refcnt++; 
	        pWin->overrideRedirect = TRUE;
                MapWindow(pWin, TRUE, FALSE, FALSE, 0);
	        savedScreenInfo[i].blanked = SCREEN_IS_TILED;
	    }
            else
	        savedScreenInfo[i].blanked = SCREEN_ISNT_SAVED;
	}
    }
    screenIsSaved = what; 
}

