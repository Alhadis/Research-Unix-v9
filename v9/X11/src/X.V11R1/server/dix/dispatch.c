/* $Header: dispatch.c,v 1.16 87/09/12 21:40:28 sun Exp $ */
/************************************************************
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

********************************************************/

#include "X.h"
#define NEED_REPLIES
#define NEED_EVENTS
#include "Xproto.h"
#include "windowstr.h"
#include "fontstruct.h"
#include "dixfontstr.h"
#include "gcstruct.h"
#include "osstruct.h"
#include "selection.h"
#include "colormapst.h"
#include "cursorstr.h"
#include "scrnintstr.h"
#include "opaque.h"
#include "input.h"
#include "servermd.h"

extern WindowRec WindowTable[];
extern xConnSetupPrefix connSetupPrefix;
extern char *ConnectionInfo;
extern void ProcessInputEvents();
extern void ValidateGC();

Selection *CurrentSelections = (Selection *)NULL;
int NumCurrentSelections = 0;

extern long ScreenSaverTime;
extern long ScreenSaverInterval;
extern int  ScreenSaverBlanking;
extern int  ScreenSaverAllowExposures;
static ClientPtr onlyClient;
static Bool grabbingClient = FALSE;
static long *checkForInput[2];
extern Bool clientsDoomed;
extern int connBlockScreenStart;

extern int (* ProcVector[256]) ();
extern int (* SwappedProcVector[256]) ();
extern void (* EventSwapVector[128]) ();
extern void (* ReplySwapVector[256]) ();
extern void Swap32Write(), SLHostsExtend(), SQColorsExtend(), WriteSConnectionInfo();
void KillAllClients();

/* buffers for clients. legal values below */
static int nextFreeClientID=1;	   /* 0 is for the server */

static int	nClients = 0;	/* number active clients */

#define SAME_SCREENS(a, b) (\
    (a.pScreen == b.pScreen))

#define VALIDATE(pGC, pDraw, rt) {\
    if (pGC->serialNumber != pDraw->serialNumber)\
    {\
	ValidateGC(pDraw, pGC);\
    } \
}

#define LEGAL_NEW_RESOURCE(id)\
    if ((LookupID(id, RT_ANY, RC_CORE) != 0) || (id & SERVER_BIT) \
	|| (client->clientAsMask != CLIENT_BITS(id)))\
        return(BadIDChoice)


#define LOOKUP_DRAWABLE(did, client)\
    ((client->lastDrawableID == did) ? \
     (DrawablePtr)client->lastDrawable : (DrawablePtr)LookupDrawable(did, client))

#define VERIFY_GC(pGC, rid, client)\
    if (client->lastGCID == rid)\
    {\
        pGC = (GC *) client->lastGC;\
    }\
    else\
    {\
	pGC = (GC *)LookupID(rid, RT_GC, RC_CORE);\
        if (!pGC)\
        {\
	    client->errorValue = rid;\
	    return (BadGC);\
        }\
    }

#define VALIDATE_DRAWABLE_AND_GC(drawID, pDraw, pGC, client)\
    if ((client->lastDrawableID != drawID) || (client->lastGCID != stuff->gc))\
    {\
        if (client->lastDrawableID != drawID)\
	{\
    	    pDraw = (DrawablePtr)LookupID(drawID, RT_DRAWABLE, RC_CORE);\
    	    if (!pDraw)\
	    {\
	        client->errorValue = drawID; \
                return (BadDrawable);\
	    }\
	    if ((pDraw->type == DRAWABLE_WINDOW) || \
		(pDraw->type == DRAWABLE_PIXMAP))\
    	    {\
	        client->lastDrawable = (DrawablePtr)pDraw;\
	        client->lastDrawableID = drawID;\
	    }\
            else\
	    {\
	        client->errorValue = drawID;\
                return (BadDrawable);\
	    }\
        }\
        else\
	    pDraw = (DrawablePtr)client->lastDrawable;\
        if (client->lastGCID != stuff->gc)\
	{\
	    pGC = (GC *)LookupID(stuff->gc, RT_GC, RC_CORE);\
            if (!pGC)\
            {\
	        client->errorValue = stuff->gc;\
	        return (BadGC);\
            }\
            client->lastGC = (GCPtr)pGC;\
            client->lastGCID = stuff->gc;\
        }\
        else\
            pGC = (GC *) client->lastGC;\
        if ((pGC->depth != pDraw->depth) || (pGC->pScreen != pDraw->pScreen))\
	{\
            client->errorValue = stuff->gc;\
	    client->lastGCID = -1;\
	    return (BadMatch);\
         }\
    }\
    else\
    {\
        pGC = (GC *) client->lastGC;\
        pDraw = (DrawablePtr)client->lastDrawable;\
    }\
    if (pGC->serialNumber != pDraw->serialNumber)\
    { \
	ValidateGC(pDraw, pGC);\
    }

void
SetInputCheck(c0, c1)
    long *c0, *c1;
{
    checkForInput[0] = c0;
    checkForInput[1] = c1;
}

void
InitSelections()
{
    int i;

    if (NumCurrentSelections == 0)
    {    
	CurrentSelections = (Selection *)Xalloc(sizeof(Selection));
	NumCurrentSelections = 1;
    }
    for (i = 0; i< NumCurrentSelections; i++)
	CurrentSelections[i].window = None;
}

void 
FlushClientCaches(id)
    int id;
{
    int i;
    register ClientPtr client;

    client = clients[CLIENT_ID(id)];
    if (client == NullClient)
        return ;
    for (i=0; i<currentMaxClients; i++)
    {
        if (client == clients[i])
	{
            if (client->lastDrawableID == id)
                client->lastDrawableID = INVALID;
            else if (client->lastGCID == id)
                client->lastGCID = -1;
	}
    }
}

Dispatch()
{
    ClientPtr	        *clientReady;     /* mask of request ready clients */
    ClientPtr	        *newClients;      /* mask of new clients */ 
    int			result;
    xReq		*request;
    int			ErrorStatus;
    ClientPtr		client;
    int			nready, nnew;

    nextFreeClientID = 1;
    InitSelections();
    nClients = 0;
    clientsDoomed = FALSE;

    clientReady = (ClientPtr *) ALLOCATE_LOCAL(sizeof(ClientPtr) * MaxClients);
    newClients = (ClientPtr *)ALLOCATE_LOCAL(sizeof(ClientPtr) * MaxClients);

    while (1) 
    {
StartOver:
        if (*checkForInput[0] != *checkForInput[1])
	    ProcessInputEvents();

	WaitForSomething(clientReady, &nready, newClients, &nnew);

	/*****************
	 *  Establish any new connections
	 *****************/

	while (nnew--)
        {
	    client = newClients[nnew];
	    client->requestLogIndex = 0;
	    InitClientResources(client);
	    SendConnectionSetupInfo(client);
	    nClients++;
	}

       /***************** 
	*  Handle events in round robin fashion, doing input between 
	*  each round 
	*****************/

	while ((nready--) > 0)
	{
	    client = clientReady[nready];
	    if (! client)
	    {
		ErrorF( "HORRIBLE ERROR, unused client %d\n", nready);
		continue;
	    }
	    isItTimeToYield = FALSE;
 
            requestingClient = client;
	    while (! isItTimeToYield)
	    {
	        if (*checkForInput[0] != *checkForInput[1])
		    ProcessInputEvents();
	   
		/* now, finally, deal with client requests */

	        request = (xReq *)ReadRequestFromClient(
				      client, &result, request);
	        if (result < 0) 
	        {
		    CloseDownClient(client);
		    isItTimeToYield = TRUE;
		    continue;
	        }
	        else if (result == 0)
	        {
#ifdef notdef
		    ErrorF(  "Blocked read in dispatcher\n");
		    ErrorF(  "reqType %d %d\n", 
			     (request ? request->reqType : -1),
			       nready);
#endif
		    if (nready > 0)
			continue;
		    else
		        goto StartOver;
		}

		client->sequence++;
		client->requestBuffer = (pointer)request;
		if (client->requestLogIndex == MAX_REQUEST_LOG)
		    client->requestLogIndex = 0;
		client->requestLog[client->requestLogIndex] = request->reqType;
		client->requestLogIndex++;
		ErrorStatus = (* (client->swapped ?
		    SwappedProcVector : ProcVector)[request->reqType])(client);
	    
		if (ErrorStatus != Success) 
		{
		    if (client->noClientException != Success)
                        CloseDownClient(client);
                    else
		        Oops(client, request->reqType, 0, ErrorStatus);
		    isItTimeToYield = TRUE;
		    continue;
	        }
	    }
	}
	/* Not an error, we just need to know to restart */
	if((nClients == -1) || clientsDoomed)
	    break;         /* so that DEALLOCATE_LOCALs happen */
    }
    if (clientsDoomed)
        KillAllClients();
    DEALLOCATE_LOCAL(newClients);
    DEALLOCATE_LOCAL(clientReady);
}

int
ProcBadRequest(client)
    ClientPtr client;
{
    return (BadRequest);
}

extern int Ones();

int
ProcCreateWindow(client)
    register ClientPtr client;
{
    register WindowPtr pParent, pWin;
    REQUEST(xCreateWindowReq);
    int result;
    int len;

    REQUEST_AT_LEAST_SIZE(xCreateWindowReq);
    
    LEGAL_NEW_RESOURCE(stuff->wid); 
    if (!(pParent = (WindowPtr)LookupWindow(stuff->parent, client)))
        return BadWindow;
    len = stuff->length -  (sizeof(xCreateWindowReq) >> 2);
    if (Ones(stuff->mask) != len)
        return BadLength;
    if (!stuff->width || !stuff->height)
        return BadValue;
    pWin = CreateWindow(stuff->wid, pParent, stuff->x,
			      stuff->y, stuff->width, stuff->height, 
			      stuff->borderWidth, stuff->class,
			      stuff->mask, (long *) &stuff[1], 
			      stuff->depth, 
			      client, stuff->visual, &result);
    if (pWin)
        AddResource(stuff->wid, RT_WINDOW, pWin, DeleteWindow, RC_CORE);
    if (client->noClientException != Success)
        return(client->noClientException);
    else
        return(result);
}

int
ProcChangeWindowAttributes(client)
    register ClientPtr client;
{
    register WindowPtr pWin;
    REQUEST(xChangeWindowAttributesReq);
    register int result;
    int len;

    REQUEST_AT_LEAST_SIZE(xChangeWindowAttributesReq);
    pWin = (WindowPtr)LookupWindow(stuff->window, client);
    if (!pWin)
        return(BadWindow);
    len = stuff->length - (sizeof(xChangeWindowAttributesReq) >> 2);
    if (len != Ones(stuff->valueMask))
        return BadLength;
    client->lastDrawableID = INVALID;   
    result =  ChangeWindowAttributes(pWin, 
				  stuff->valueMask, 
				  (long *) &stuff[1], 
				  client);
    if (client->noClientException != Success)
        return(client->noClientException);
    else
        return(result);
}

int
ProcGetWindowAttributes(client)
    register ClientPtr client;
{
    register WindowPtr pWin;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pWin = (WindowPtr)LookupWindow(stuff->id, client);
    if (!pWin)
        return(BadWindow);
    GetWindowAttributes(pWin, client);
    return(client->noClientException);
}

int
ProcDestroyWindow(client)
    register ClientPtr client;
{
    register WindowPtr pWin;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pWin = (WindowPtr)LookupWindow(stuff->id, client);
    if (!pWin)
        return(BadWindow);
    FreeResource(stuff->id, RC_NONE);
    return(client->noClientException);
}

int
ProcDestroySubwindows(client)
    register ClientPtr client;
{
    register WindowPtr pWin;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pWin = (WindowPtr)LookupWindow(stuff->id, client);
    if (!pWin)
        return(BadWindow);
    DestroySubwindows(pWin, client);
    return(client->noClientException);
}

int
ProcChangeSaveSet(client)
    register ClientPtr client;
{
    register WindowPtr pWin;
    REQUEST(xChangeSaveSetReq);
    register result;
		  
    REQUEST_SIZE_MATCH(xChangeSaveSetReq);
    pWin = (WindowPtr)LookupWindow(stuff->window, client);
    if (!pWin)
        return(BadWindow);
    if (client->clientAsMask == (CLIENT_ID(pWin->wid)))
        return BadMatch;
    if ((stuff->mode == SetModeInsert) || (stuff->mode == SetModeDelete))
    {
        result = AlterSaveSetForClient(client, pWin, stuff->mode);
	if (client->noClientException != Success)
	    return(client->noClientException);
	else
            return(result);
    }
    else
	return( BadValue );
}

int
ProcReparentWindow(client)
    register ClientPtr client;
{
    register WindowPtr pWin, pParent;
    REQUEST(xReparentWindowReq);
    register int result;

    REQUEST_SIZE_MATCH(xReparentWindowReq);
    pWin = (WindowPtr)LookupWindow(stuff->window, client);
    if (!pWin)
        return(BadWindow);
    pParent = (WindowPtr)LookupWindow(stuff->parent, client);
    if (!pParent)
        return(BadWindow);
    if (SAME_SCREENS(pWin->drawable, pParent->drawable))
    {
        if ((pWin->backgroundTile == (PixmapPtr)ParentRelative) &&
            (pParent->drawable.depth != pWin->drawable.depth))
            return BadMatch;
        result =  ReparentWindow(pWin, pParent, 
			 (short)stuff->x, (short)stuff->y, client);
	if (client->noClientException != Success)
            return(client->noClientException);
	else
            return(result);
    }
    else 
        return (BadMatch);
}

int
ProcMapWindow(client)
    register ClientPtr client;
{
    register WindowPtr pWin;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pWin = (WindowPtr)LookupWindow(stuff->id, client);
    if (!pWin)
        return(BadWindow);
    MapWindow(pWin, HANDLE_EXPOSURES, BITS_DISCARDED,
		  SEND_NOTIFICATION, client);
           /* update cache to say it is mapped */
    return(client->noClientException);
}

int
ProcMapSubwindows(client)
    register ClientPtr client;
{
    register WindowPtr pWin;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pWin = (WindowPtr)LookupWindow( stuff->id, client);
    if (!pWin)
        return(BadWindow);
    MapSubwindows(pWin, HANDLE_EXPOSURES, client);
           /* update cache to say it is mapped */
    return(client->noClientException);
}

int
ProcUnmapWindow(client)
    register ClientPtr client;
{
    register WindowPtr pWin;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pWin = (WindowPtr)LookupWindow( stuff->id, client);
    if (!pWin)
        return(BadWindow);
    UnmapWindow(pWin, HANDLE_EXPOSURES, SEND_NOTIFICATION, FALSE);
           /* update cache to say it is mapped */
    return(client->noClientException);
}

int
ProcUnmapSubwindows(client)
    register ClientPtr client;
{
    register WindowPtr pWin;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pWin = (WindowPtr)LookupWindow( stuff->id, client);
    if (!pWin)
        return(BadWindow);
    UnmapSubwindows(pWin, HANDLE_EXPOSURES, FALSE);
    return(client->noClientException);
}

int
ProcConfigureWindow(client)
    register ClientPtr client;
{
    register WindowPtr pWin;
    REQUEST(xConfigureWindowReq);
    register int result;
    int len;

    REQUEST_AT_LEAST_SIZE(xConfigureWindowReq);
    pWin = (WindowPtr)LookupWindow( stuff->window, client);
    if (!pWin)
        return(BadWindow);
    len = stuff->length - (sizeof(xConfigureWindowReq) >> 2);
    if (Ones(stuff->mask) != len)
        return BadLength;
    result =  ConfigureWindow(pWin, stuff->mask, (char *) &stuff[1], 
			      client);
    if (client->noClientException != Success)
        return(client->noClientException);
    else
        return(result);
}

int
ProcCirculateWindow(client)
    register ClientPtr client;
{
    register WindowPtr pWin;
    REQUEST(xCirculateWindowReq);

    REQUEST_SIZE_MATCH(xCirculateWindowReq);
    if ((stuff->direction != RaiseLowest) &&
	(stuff->direction != LowerHighest))
        return BadValue;
    pWin = (WindowPtr)LookupWindow(stuff->window, client);
    if (!pWin)
        return(BadWindow);
    CirculateWindow(pWin, stuff->direction, client);
    return(client->noClientException);
}

int
ProcGetGeometry(client)
    register ClientPtr client;
{
    xGetGeometryReply rep;
    register DrawablePtr pDraw;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    if (!(pDraw = LOOKUP_DRAWABLE(stuff->id, client)))
    {                /* can be inputonly */
        if (!(pDraw = (DrawablePtr)LookupWindow(stuff->id, client))) 
            return (BadDrawable);
    }
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.root = WindowTable[pDraw->pScreen->myNum].wid;
    rep.depth = pDraw->depth;

    if (pDraw->type == DRAWABLE_PIXMAP)
    {
	PixmapPtr pPixmap = (PixmapPtr)pDraw;

	rep.x = rep.y = rep.borderWidth = 0;
	rep.width = pPixmap->width;
	rep.height = pPixmap->height;
    }
    else
    {
        register WindowPtr pWin = (WindowPtr)pDraw;
	rep.x = pWin->clientWinSize.x - pWin->borderWidth;
	rep.y = pWin->clientWinSize.y - pWin->borderWidth;
	rep.borderWidth = pWin->borderWidth;
	rep.width = pWin->clientWinSize.width;
	rep.height = pWin->clientWinSize.height;
    }
    WriteReplyToClient(client, sizeof(xGetGeometryReply), &rep);
    return(client->noClientException);
}

int
ProcQueryTree(client)
    register ClientPtr client;
{

    xQueryTreeReply reply;
    int numChildren = 0;
    register WindowPtr pChild, pWin;
    Window  *childIDs = (Window *)NULL;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pWin = (WindowPtr)LookupWindow(stuff->id, client);
    if (!pWin)
        return(BadWindow);
    reply.type = X_Reply;
    reply.root = WindowTable[pWin->drawable.pScreen->myNum].wid;
    reply.sequenceNumber = client->sequence;
    if (pWin->parent)
	reply.parent = pWin->parent->wid;
    else
        reply.parent = (Window)None;

    for (pChild = pWin->lastChild; pChild; pChild = pChild->prevSib)
	numChildren++;
    if (numChildren)
    {
	int curChild = 0;

	childIDs = (Window *) Xalloc(numChildren * sizeof(Window));
	for (pChild = pWin->lastChild; pChild; pChild = pChild->prevSib)
	    childIDs[curChild++] = pChild->wid;
    }
    
    reply.nChildren = numChildren;
    reply.length = (numChildren * sizeof(Window)) >> 2;
    
    WriteReplyToClient(client, sizeof(xQueryTreeReply), &reply);
    if (numChildren)
    {
    	client->pSwapReplyFunc = Swap32Write;
	WriteSwappedDataToClient(client, numChildren * sizeof(Window), childIDs);
	Xfree(childIDs);
    }

    return(client->noClientException);
}

int
ProcInternAtom(client)
    register ClientPtr client;
{
    Atom atom;
    char *tchar;
    REQUEST(xInternAtomReq);

    REQUEST_AT_LEAST_SIZE(xInternAtomReq);
    tchar = (char *) &stuff[1];
    atom = MakeAtom(tchar, stuff->nbytes, !stuff->onlyIfExists);
    if (atom || stuff->onlyIfExists)
    {
	xInternAtomReply reply;
	reply.type = X_Reply;
	reply.length = 0;
	reply.sequenceNumber = client->sequence;
	reply.atom = (atom ? atom : None);
	WriteReplyToClient(client, sizeof(xInternAtomReply), &reply);
	return(client->noClientException);
    }
    else
	return (BadAlloc);
}

int
ProcGetAtomName(client)
    register ClientPtr client;
{
    char *str;
    xGetAtomNameReply reply;
    int len;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    if (str = (char *)NameForAtom(stuff->id)) 
    {
	len = strlen(str);
	reply.type = X_Reply;
	reply.length = (len + 3) >> 2;
	reply.sequenceNumber = client->sequence;
	reply.nameLength = len;
	WriteReplyToClient(client, sizeof(xGetAtomNameReply), &reply);
	WriteToClient(client, len, str);
	return(client->noClientException);
    }
    else 
    { 
	client->errorValue = stuff->id;
	return (BadAtom);
    }
}

int 
ProcDeleteProperty(client)
    register ClientPtr client;
{
    WindowPtr pWin;
    REQUEST(xDeletePropertyReq);
    int result;
              
    REQUEST_SIZE_MATCH(xDeletePropertyReq);
    pWin = (WindowPtr)LookupWindow(stuff->window, client);
    if (!pWin)
        return(BadWindow);
    if (ValidAtom(stuff->property))
    {
	result = DeleteProperty(pWin, stuff->property);
        if (client->noClientException != Success)
            return(client->noClientException);
	else
	    return(result);
    }
    else 
	return (BadAtom);
}


int
ProcSetSelectionOwner(client)
    register ClientPtr client;
{
    WindowPtr pWin;
    TimeStamp time;
    REQUEST(xSetSelectionOwnerReq);

    REQUEST_SIZE_MATCH(xSetSelectionOwnerReq);
    time = ClientTimeToServerTime(stuff->time);

    /* If the client's time stamp is in the future relative to the server's
	time stamp, do not set the selection, just return success. */
    if (CompareTimeStamps(time, currentTime) == LATER)
    	return Success;
    if (stuff->window != None)
    {
        pWin = (WindowPtr)LookupWindow(stuff->window, client);
        if (!pWin)
            return(BadWindow);
    }
    else
        pWin = (WindowPtr)None;
    if (ValidAtom(stuff->selection))
    {
	int i = 0;

	/*
	 * First, see if the selection is already set... 
	 */
	while ((i < NumCurrentSelections) && 
	       CurrentSelections[i].selection != stuff->selection) 
            i++;
        if (i < NumCurrentSelections)
        {        
	    xEvent event;

	    /* If the timestamp in client's request is in the past relative
		to the time stamp indicating the last time the owner of the
		selection was set, do not set the selection, just return 
		success. */
            if (CompareTimeStamps(time, CurrentSelections[i].lastTimeChanged)
		== EARLIER)
		return Success;
            if (CurrentSelections[i].pWin != (WindowPtr)None)
	    {
		event.u.u.type = SelectionClear;
		event.u.selectionClear.time = time.milliseconds;
		event.u.selectionClear.window = CurrentSelections[i].window;
		event.u.selectionClear.atom = CurrentSelections[i].selection;
		DeliverEvents(CurrentSelections[i].pWin, &event, 1);
	    }
	    CurrentSelections[i].selection = stuff->selection;
	    CurrentSelections[i].lastTimeChanged = time;
	    CurrentSelections[i].window = stuff->window;
	    CurrentSelections[i].pWin = pWin;
	    CurrentSelections[i].client = client;
	    return (client->noClientException);
	}
	/*
	 * It doesn't exist, so add it...
	 */
            NumCurrentSelections++;
	    CurrentSelections = 
			(Selection *)Xrealloc(CurrentSelections, 
			NumCurrentSelections * sizeof(Selection));

	CurrentSelections[i].selection = stuff->selection;
        CurrentSelections[i].lastTimeChanged = time;
	CurrentSelections[i].window = stuff->window;
	CurrentSelections[i].pWin = pWin;
	CurrentSelections[i].client = client;
	return (client->noClientException);
    }
    else 
        return (BadAtom);
}

int
ProcGetSelectionOwner(client)
    register ClientPtr client;
{
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    if (ValidAtom(stuff->id))
    {
	int i;
        xGetSelectionOwnerReply reply;

	i = 0;
        while ((i < NumCurrentSelections) && 
	       CurrentSelections[i].selection != stuff->id) i++;
        reply.type = X_Reply;
	reply.length = 0;
	reply.sequenceNumber = client->sequence;
        if (i < NumCurrentSelections)
            reply.owner = CurrentSelections[i].window;
        else
            reply.owner = None;
        WriteReplyToClient(client, sizeof(xGetSelectionOwnerReply), &reply);
        return(client->noClientException);
    }
    else            
        return (BadAtom); 
}

int
ProcConvertSelection(client)
    register ClientPtr client;
{
    Bool paramsOkay = TRUE;
    xEvent event;
    WindowPtr pWin;
    REQUEST(xConvertSelectionReq);

    REQUEST_SIZE_MATCH(xConvertSelectionReq);
    pWin = (WindowPtr)LookupWindow(stuff->requestor, client);
    if (!pWin)
        return(BadWindow);

    paramsOkay = (ValidAtom(stuff->selection) && ValidAtom(stuff->target));
    if (stuff->property != None)
	paramsOkay &= ValidAtom(stuff->property);
    if (paramsOkay)
    {
	int i;

	i = 0;
	while ((i < NumCurrentSelections) && 
	       CurrentSelections[i].selection != stuff->selection) i++;
	if ((i < NumCurrentSelections) && 
	    (CurrentSelections[i].window != None))
	{        
	    event.u.u.type = SelectionRequest;
	    event.u.selectionRequest.time = stuff->time;
	    event.u.selectionRequest.owner = 
			CurrentSelections[i].window;
	    event.u.selectionRequest.requestor = stuff->requestor;
	    event.u.selectionRequest.selection = stuff->selection;
	    event.u.selectionRequest.target = stuff->target;
	    event.u.selectionRequest.property = stuff->property;
	    if (TryClientEvents(
		CurrentSelections[i].client, &event, 1, NoEventMask,
		NoEventMask, NullGrab))
		return (client->noClientException);
	}
	event.u.u.type = SelectionNotify;
	event.u.selectionNotify.time = stuff->time;
	event.u.selectionNotify.requestor = stuff->requestor;
	event.u.selectionNotify.selection = stuff->selection;
	event.u.selectionNotify.target = stuff->target;
	event.u.selectionNotify.property = None;
	DeliverEvents(pWin, &event, 1);
	return (client->noClientException);
    }
    else 
        return (BadAtom);
}

int
ProcGrabServer(client)
    register ClientPtr client;
{
    OnlyListenToOneClient(client);
    grabbingClient = TRUE;
    onlyClient = client;
    return(client->noClientException);
}

int
ProcUngrabServer(client)
    register ClientPtr client;
{
    REQUEST(xReq);
    REQUEST_SIZE_MATCH(xReq);
    grabbingClient = FALSE;
    ListenToAllClients();
    return(client->noClientException);
}

int
ProcTranslateCoords(client)
    register ClientPtr client;
{
    REQUEST(xTranslateCoordsReq);

    register WindowPtr pWin, pDst;
    xTranslateCoordsReply rep;

    REQUEST_SIZE_MATCH(xTranslateCoordsReq);
    pWin = (WindowPtr)LookupWindow(stuff->srcWid, client);
    if (!pWin)
        return(BadWindow);
    pDst = (WindowPtr)LookupWindow(stuff->dstWid, client);
    if (!pDst)
        return(BadWindow);
    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    if (!SAME_SCREENS(pWin->drawable, pDst->drawable))
    {
	rep.sameScreen = xFalse;
        rep.child = None;
	rep.dstX = rep.dstY = 0;
    }
    else
    {
	INT16 x, y;
	rep.sameScreen = xTrue;
	rep.child = None;
	/* computing absolute coordinates -- adjust to destination later */
	x = pWin->absCorner.x + stuff->srcX;
	y = pWin->absCorner.y + stuff->srcY;
	pWin = pDst->firstChild;
	while (pWin)
	{
	    if ((pWin->mapped) &&
		(x >= pWin->absCorner.x - pWin->borderWidth) &&
		(x < pWin->absCorner.x + pWin->clientWinSize.width +
		 pWin->borderWidth) &&
		(y >= pWin->absCorner.y - pWin->borderWidth) &&
		(y < pWin->absCorner.y + pWin->clientWinSize.height
		 + pWin->borderWidth))
            {
		rep.child = pWin->wid;
		pWin = (WindowPtr) NULL;
	    }
	    else
		pWin = pWin->nextSib;
	}
	/* adjust to destination coordinates */
	rep.dstX = x - pDst->absCorner.x;
	rep.dstY = y - pDst->absCorner.y;
    }
    WriteReplyToClient(client, sizeof(xTranslateCoordsReply), &rep);
    return(client->noClientException);
}

int
ProcOpenFont(client)
    register ClientPtr client;
{
    FontPtr pFont;
    REQUEST(xOpenFontReq);

    REQUEST_AT_LEAST_SIZE(xOpenFontReq);
    client->errorValue = stuff->fid;
    LEGAL_NEW_RESOURCE(stuff->fid);
    if ( pFont = OpenFont( stuff->nbytes, (char *)&stuff[1]))
    {
	AddResource( stuff->fid, RT_FONT, pFont, CloseFont,RC_CORE);
	return(client->noClientException);
    }
    else
	return (BadName);
}

int
ProcCloseFont(client)
    register ClientPtr client;
{
    FontPtr pFont;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pFont = (FontPtr)LookupID(stuff->id, RT_FONT, RC_CORE);
    if ( pFont != (FontPtr)NULL)	/* id was valid */
    {
        FreeResource( stuff->id, RC_NONE);
	return(client->noClientException);
    }
    else
        return (BadFont);
}

int
ProcQueryFont(client)
    register ClientPtr client;
{
    xQueryFontReply	*reply;
    FontPtr pFont;
    register GC *pGC;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    client->errorValue = stuff->id;		/* EITHER font or gc */
    pFont = (FontPtr)LookupID(stuff->id, RT_FONT, RC_CORE);
    if (!pFont)
    {
	  /* can't use VERIFY_GC because it might return BadGC */
	pGC = (GC *) LookupID(stuff->id, RT_GC, RC_CORE);
        if (!pGC)
            return(BadFont);     /* procotol spec says only error is BadFont */
	pFont = pGC->font;
    }

    {
	CharInfoPtr	pmax = &pFont->pFI->maxbounds;
	CharInfoPtr	pmin = &pFont->pFI->minbounds;
	int		nprotoxcistructs;
	int		rlength;

	nprotoxcistructs = (
	   pmax->metrics.rightSideBearing == pmin->metrics.rightSideBearing &&
	   pmax->metrics.leftSideBearing == pmin->metrics.leftSideBearing &&
	   pmax->metrics.descent == pmin->metrics.descent &&
	   pmax->metrics.ascent == pmin->metrics.ascent &&
	   pmax->metrics.characterWidth == pmin->metrics.characterWidth) ?
		0 : n2dChars(pFont->pFI);

	rlength = sizeof(xQueryFontReply) +
	             pFont->pFI->nProps * sizeof(xFontProp)  +
		     nprotoxcistructs * sizeof(xCharInfo);
	reply = (xQueryFontReply *)ALLOCATE_LOCAL(rlength);
	if(!reply)
	{
	    return(client->noClientException = BadAlloc);
	}

	reply->type = X_Reply;
	reply->length = (rlength - sizeof(xGenericReply)) >> 2;
	reply->sequenceNumber = client->sequence;
	QueryFont( pFont, reply, nprotoxcistructs);

        WriteReplyToClient(client, rlength, reply);
	DEALLOCATE_LOCAL(reply);
	return(client->noClientException);
    }
}

int
ProcQueryTextExtents(client)
    register ClientPtr client;
{
    REQUEST(xQueryTextExtentsReq);
    xQueryTextExtentsReply reply;
    FontPtr pFont;
    GC *pGC;
    ExtentInfoRec info;
    short length;

    REQUEST_AT_LEAST_SIZE(xQueryTextExtentsReq);
        
    pFont = (FontPtr)LookupID( stuff->fid, RT_FONT, RC_CORE);
    if (!pFont)
    {
        pGC = (GC *)LookupID( stuff->fid, RT_GC, RC_CORE);
        if (!pGC)
            return(BadFont);
	pFont = pGC->font;
    }
    length = stuff->length - (sizeof(xQueryTextExtentsReq) >> 2);
    length = length << 1;
    if (stuff->oddLength)
        length--;
    QueryTextExtents(pFont, length, &stuff[1], &info);   
    reply.type = X_Reply;
    reply.length = 0;
    reply.sequenceNumber = client->sequence;
    reply.drawDirection = info.drawDirection;
    reply.fontAscent = info.fontAscent;
    reply.fontDescent = info.fontDescent;
    reply.overallAscent = info.overallAscent;
    reply.overallDescent = info.overallDescent;
    reply.overallWidth = info.overallWidth;
    reply.overallLeft = info.overallLeft;
    reply.overallRight = info.overallRight;
    WriteReplyToClient(client, sizeof(xQueryTextExtentsReply), &reply);
    return(client->noClientException);
}

int
ProcListFonts(client)
    register ClientPtr client;
{
    xListFontsReply reply; 
    FontPathPtr fpr;
    int stringLens, i;
    char *bufptr, *bufferStart;
    REQUEST(xListFontsReq);

    REQUEST_AT_LEAST_SIZE(xListFontsReq);

    fpr = ExpandFontNamePattern( stuff->nbytes, 
					      &stuff[1], stuff->maxNames);
    stringLens = 0;
    for (i=0; i<fpr->npaths; i++)
        stringLens += fpr->length[i];

    reply.type = X_Reply;
    reply.length = (stringLens + fpr->npaths + 3) >> 2;
    reply.nFonts = fpr->npaths;
    reply.sequenceNumber = client->sequence;

    bufptr = bufferStart = (char *)ALLOCATE_LOCAL(reply.length << 2);
    if(!bufptr)
        return(client->noClientException = BadAlloc);

            /* since WriteToClient long word aligns things, 
	       copy to temp buffer and write all at once */
    for (i=0; i<fpr->npaths; i++)
    {
        *bufptr++ = fpr->length[i];
        bcopy(fpr->paths[i], bufptr,  fpr->length[i]);
        bufptr += fpr->length[i];
    }
    WriteReplyToClient(client, sizeof(xListFontsReply), &reply);
    WriteToClient(client, stringLens + fpr->npaths, bufferStart);
    FreeFontRecord(fpr);
    DEALLOCATE_LOCAL(bufferStart);
    
    return(client->noClientException);
}

int
ProcListFontsWithInfo(client)
    register ClientPtr client;
{
    register xListFontsWithInfoReply *reply;
    xListFontsWithInfoReply last_reply;
    FontRec font;
    FontInfoRec finfo;
    register FontPathPtr fpaths;
    register char **path;
    register int n, *length;
    int rlength;
    REQUEST(xListFontsWithInfoReq);

    REQUEST_AT_LEAST_SIZE(xListFontsWithInfoReq);

    fpaths = ExpandFontNamePattern( stuff->nbytes, &stuff[1], stuff->maxNames);
    font.pFI = &finfo;
    for (n = fpaths->npaths, path = fpaths->paths, length = fpaths->length;
	 --n >= 0;
	 path++, length++)
    {
	if (!(DescribeFont(*path, *length, &finfo, &font.pFP)))
	   continue;
	rlength = sizeof(xListFontsWithInfoReply)
		    + finfo.nProps * sizeof(xFontProp);
	if (reply = (xListFontsWithInfoReply *)ALLOCATE_LOCAL(rlength))
	{
		reply->type = X_Reply;
		reply->sequenceNumber = client->sequence;
		reply->length = (rlength - sizeof(xGenericReply)
				 + *length + 3) >> 2;
		QueryFont(&font, (xQueryFontReply *) reply, 0);
		reply->nameLength = *length;
		reply->nReplies = n;
		WriteReplyToClient(client, rlength, reply);
		WriteToClient(client, *length, *path);
		DEALLOCATE_LOCAL(reply);
	}
	Xfree((char *)font.pFP);
    }
    FreeFontRecord(fpaths);
    bzero((char *)&last_reply, sizeof(xListFontsWithInfoReply));
    last_reply.type = X_Reply;
    last_reply.sequenceNumber = client->sequence;
    last_reply.length = (sizeof(xListFontsWithInfoReply)
			  - sizeof(xGenericReply)) >> 2;
    WriteReplyToClient(client, sizeof(xListFontsWithInfoReply), &last_reply);
    return(client->noClientException);
}

int
ProcCreatePixmap(client)
    register ClientPtr client;
{
    PixmapPtr pMap;
    register DrawablePtr pDraw;
    REQUEST(xCreatePixmapReq);
    DepthPtr pDepth;
    register int i;

    REQUEST_AT_LEAST_SIZE(xCreatePixmapReq);
    client->errorValue = stuff->pid;
    LEGAL_NEW_RESOURCE(stuff->pid);
    if (!(pDraw = LOOKUP_DRAWABLE(stuff->drawable, client)))
    {        /* can be inputonly */
        if (!(pDraw = (DrawablePtr)LookupWindow(stuff->drawable, client))) 
            return (BadDrawable);
    }

    if (!stuff->width || !stuff->height)
        return BadValue;
    if (stuff->depth != 1)
    {
        pDepth = pDraw->pScreen->allowedDepths;
        for (i=0; i<pDraw->pScreen->numDepths; i++, pDepth++)
	   if (pDepth->depth == stuff->depth)
               goto CreatePmap;
        return BadValue;
    }
CreatePmap:
    pMap = (PixmapPtr)(*pDraw->pScreen->CreatePixmap)
		(pDraw->pScreen, stuff->width,
		 stuff->height, stuff->depth);
    if (pMap)
    {
	pMap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
	AddResource(
	    stuff->pid, RT_PIXMAP, pMap, pDraw->pScreen->DestroyPixmap,
	    RC_CORE);
	return(client->noClientException);
    }
    else
	return (BadAlloc);
}

int
ProcFreePixmap(client)
    register ClientPtr client;
{
    PixmapPtr pMap;

    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pMap = (PixmapPtr)LookupID(stuff->id, RT_PIXMAP, RC_CORE);
    if (pMap) 
    {
	FreeResource(stuff->id, RC_NONE);
	return(client->noClientException);
    }
    else 
    {
	client->errorValue = stuff->id;
	return (BadPixmap);
    }
}

int
ProcCreateGC(client)
    register ClientPtr client;
{
    int error;
    GC *pGC;
    register DrawablePtr pDraw;
    int len;
    REQUEST(xCreateGCReq);

    REQUEST_AT_LEAST_SIZE(xCreateGCReq);
    client->errorValue = stuff->gc;
    LEGAL_NEW_RESOURCE(stuff->gc);
    if (!(pDraw = LOOKUP_DRAWABLE( stuff->drawable, client) ))
        return (BadDrawable);
    len = stuff->length -  (sizeof(xCreateGCReq) >> 2);
    if (len != Ones(stuff->mask))
        return BadLength;
    pGC = (GC *)CreateGC(pDraw, stuff->mask, 
			 (char *) &stuff[1], &error);
    if (error != Success)
        return error;
    if (pGC)
    {
	AddResource(stuff->gc, RT_GC, pGC, FreeGC, RC_CORE);
	return(client->noClientException);
    }
    else 
	return (BadAlloc);
}

int
ProcChangeGC(client)
    register ClientPtr client;
{
    GC *pGC;
    REQUEST(xChangeGCReq);
    int result, len;
		
    REQUEST_AT_LEAST_SIZE(xChangeGCReq);
    VERIFY_GC(pGC, stuff->gc, client);
    len = stuff->length -  (sizeof(xChangeGCReq) >> 2);
    if (len != Ones(stuff->mask))
        return BadLength;
    result = DoChangeGC(pGC, stuff->mask, (int *) &stuff[1], 0);
    if (client->noClientException != Success)
        return(client->noClientException);
    else
        return(result);
}

int
ProcCopyGC(client)
    register ClientPtr client;
{
    register GC *dstGC;
    register GC *pGC;
    REQUEST(xCopyGCReq);

    REQUEST_SIZE_MATCH(xCopyGCReq);
    VERIFY_GC( pGC, stuff->srcGC, client);
    VERIFY_GC( dstGC, stuff->dstGC, client);
    if ((dstGC->pScreen != pGC->pScreen) || (dstGC->depth != pGC->depth))
        return (BadMatch);    
    CopyGC(pGC, dstGC, stuff->mask);
    return (client->noClientException);
}

int
ProcSetDashes(client)
    register ClientPtr client;
{
    register GC *pGC;
    int result;
    REQUEST(xSetDashesReq);

    REQUEST_AT_LEAST_SIZE(xSetDashesReq);
    if ((sizeof(xSetDashesReq) >> 2) == stuff->length)
         return BadValue;

    VERIFY_GC(pGC,stuff->gc, client);

    result = SetDashes(pGC, stuff->dashOffset, stuff->nDashes, &stuff[1]);
    if (client->noClientException != Success)
        return(client->noClientException);
    else
        return(result);
}

int
ProcSetClipRectangles(client)
    register ClientPtr client;
{
    int	nr;
    register GC *pGC;
    REQUEST(xSetClipRectanglesReq);

    REQUEST_AT_LEAST_SIZE(xSetClipRectanglesReq);
    if ((stuff->ordering != Unsorted) && (stuff->ordering != YSorted) &&
	(stuff->ordering != YXSorted) && (stuff->ordering != YXBanded))
        return BadValue;
    VERIFY_GC(pGC,stuff->gc, client);
    pGC->clipOrg.x = stuff->xOrigin;
    pGC->stateChanges |= GCClipXOrigin;
		 
    pGC->clipOrg.y = stuff->yOrigin;
    pGC->stateChanges |= GCClipYOrigin;
		 
    nr = ((stuff->length  << 2) - sizeof(xSetClipRectanglesReq)) >> 3;
    SetClipRects(pGC, nr, &stuff[1], stuff->ordering);
    pGC->stateChanges |= GCClipMask;
    pGC->serialNumber = 0;
    return(client->noClientException);
}

int
ProcFreeGC(client)
    register ClientPtr client;
{
    register GC *pGC;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    VERIFY_GC(pGC,stuff->id,client);
    FreeResource(stuff->id, RC_NONE);
    return(client->noClientException);
}

int
ProcClearToBackground(client)
    register ClientPtr client;
{
    REQUEST(xClearAreaReq);
    register WindowPtr pWin;

    REQUEST_SIZE_MATCH(xClearAreaReq);
    pWin = (WindowPtr)LookupWindow( stuff->window, client);
    if (!pWin)
        return(BadWindow);
    if (pWin->class == InputOnly)
    {
	client->errorValue = stuff->window;
	return (BadWindow);
    }		    
    if ((stuff->exposures != xTrue) && (stuff->exposures != xFalse))
        return(BadValue);
    (*pWin->ClearToBackground)(pWin, stuff->x, stuff->y,
			       stuff->width, stuff->height,
			       (Bool)stuff->exposures);
    return(client->noClientException);
}

int
ProcCopyArea(client)
    register ClientPtr client;
{
    register DrawablePtr pDst;
    register DrawablePtr pSrc;
    register GC *pGC;
    REQUEST(xCopyAreaReq);

    REQUEST_SIZE_MATCH(xCopyAreaReq);

    VALIDATE_DRAWABLE_AND_GC(stuff->dstDrawable, pDst, pGC, client); 
    if (stuff->dstDrawable != stuff->srcDrawable)
    {
        if (!(pSrc = LOOKUP_DRAWABLE(stuff->srcDrawable, client)))
            return(BadDrawable);
	if ((pDst->pScreen != pSrc->pScreen) || (pDst->depth != pSrc->depth))
	{
	    client->errorValue = stuff->dstDrawable;
	    return (BadMatch);
	}
    }
    else
        pSrc = pDst;
    (*pGC->CopyArea)(pSrc, pDst, pGC, stuff->srcX, stuff->srcY,
				 stuff->width, stuff->height, 
				 stuff->dstX, stuff->dstY);
				 
    return(client->noClientException);
}

int
ProcCopyPlane(client)
    register ClientPtr client;
{
    register DrawablePtr psrcDraw, pdstDraw;
    register GC *pGC;
    REQUEST(xCopyPlaneReq);

    REQUEST_SIZE_MATCH(xCopyPlaneReq);

   /* Check to see if stuff->bitPlane has exactly ONE bit set */
   if(stuff->bitPlane == 0 || stuff->bitPlane & (stuff->bitPlane - 1)) 
       return(BadValue);

    VALIDATE_DRAWABLE_AND_GC(stuff->dstDrawable, pdstDraw, pGC, client);
    if (stuff->dstDrawable != stuff->srcDrawable)
    {
        if (!(psrcDraw = LOOKUP_DRAWABLE(stuff->srcDrawable, client)))
            return(BadDrawable);
	if (pdstDraw->pScreen != psrcDraw->pScreen)
	{
	    client->errorValue = stuff->dstDrawable;
	    return (BadMatch);
	}
    }
    else
        psrcDraw = pdstDraw;
    (*pGC->CopyPlane)(psrcDraw, pdstDraw, pGC, stuff->srcX, stuff->srcY,
				 stuff->width, stuff->height, 
				 stuff->dstX, stuff->dstY, stuff->bitPlane);
    return(client->noClientException);
}

int
ProcPolyPoint(client)
    register ClientPtr client;
{
    int npoint;
    register GC *pGC;
    register DrawablePtr pDraw;
    REQUEST(xPolyPointReq);

    REQUEST_AT_LEAST_SIZE(xPolyPointReq);
    if ((stuff->coordMode != CoordModeOrigin) && 
	(stuff->coordMode != CoordModePrevious))
        return BadValue;
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client); 
    npoint = ((stuff->length << 2) - sizeof(xPolyPointReq)) >> 2;
    if (npoint)
        (*pGC->PolyPoint)(pDraw, pGC, stuff->coordMode, npoint,
			  (xPoint *) &stuff[1]);
    return (client->noClientException);
}

int
ProcPolyLine(client)
    register ClientPtr client;
{
    int npoint;
    register GC *pGC;
    register DrawablePtr pDraw;
    REQUEST(xPolyLineReq);

    REQUEST_AT_LEAST_SIZE(xPolyLineReq);
    if ((stuff->coordMode != CoordModeOrigin) && 
	(stuff->coordMode != CoordModePrevious))
        return BadValue;
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);
    npoint = ((stuff->length << 2) - sizeof(xPolyLineReq));
    if(npoint % sizeof(xPoint) != 0)
	return(BadLength);
    npoint >>= 2;
    if (npoint < 1)
	return(BadLength);

    (*pGC->Polylines)(pDraw, pGC, stuff->coordMode, npoint, 
			  (xPoint *) &stuff[1]);
    return(client->noClientException);
}

int
ProcPolySegment(client)
    register ClientPtr client;
{
    int nsegs;
    register GC *pGC;
    register DrawablePtr pDraw;
    REQUEST(xPolySegmentReq);

    REQUEST_AT_LEAST_SIZE(xPolySegmentReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);
    nsegs = (stuff->length << 2) - sizeof(xPolySegmentReq);
    if(nsegs % sizeof(xSegment) != 0)
	return(BadLength);
    nsegs >>= 3;
    if (nsegs)
        (*pGC->PolySegment)(pDraw, pGC, nsegs, (xSegment *) &stuff[1]);
    return (client->noClientException);
}

int
ProcPolyRectangle (client)
    register ClientPtr client;
{
    int nrects;
    register GC *pGC;
    register DrawablePtr pDraw;
    REQUEST(xPolyRectangleReq);

    REQUEST_AT_LEAST_SIZE(xPolyRectangleReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);
    nrects = ((stuff->length << 2) - sizeof(xPolyRectangleReq)) >> 3;
    if (nrects)
        (*pGC->PolyRectangle)(pDraw, pGC, 
		    nrects, (xRectangle *) &stuff[1]);
    return(client->noClientException);
}

int
ProcPolyArc(client)
    register ClientPtr client;
{
    int		narcs;
    register GC *pGC;
    register DrawablePtr pDraw;
    REQUEST(xPolyArcReq);

    REQUEST_AT_LEAST_SIZE(xPolyArcReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);
    narcs = ((stuff->length << 2) - sizeof(xPolyArcReq)) / 
	    sizeof(xArc);
    if (narcs)
        (*pGC->PolyArc)(pDraw, pGC, narcs, (xArc *) &stuff[1]);
    return (client->noClientException);
}

int
ProcFillPoly(client)
    register ClientPtr client;
{
    int          things;
    register GC *pGC;
    register DrawablePtr pDraw;
    REQUEST(xFillPolyReq);

    REQUEST_AT_LEAST_SIZE(xFillPolyReq);
    if ((stuff->shape != Complex) && (stuff->shape != Nonconvex) &&  
	(stuff->shape != Convex) && (stuff->coordMode != CoordModeOrigin) && 
	(stuff->coordMode != CoordModePrevious))
        return BadValue;

    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);
    things = ((stuff->length << 2) - sizeof(xFillPolyReq)) >> 2;
    if (things)
        (*pGC->FillPolygon) (pDraw, pGC, stuff->shape,
			 stuff->coordMode, things,
			 (DDXPointPtr) &stuff[1]);
    return(client->noClientException);
}

int
ProcPolyFillRectangle(client)
    register ClientPtr client;
{
    int             things;
    register GC *pGC;
    register DrawablePtr pDraw;
    REQUEST(xPolyFillRectangleReq);

    REQUEST_AT_LEAST_SIZE(xPolyFillRectangleReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);
    things = ((stuff->length << 2) - 
	      sizeof(xPolyFillRectangleReq)) >> 3;
    if (things)
        (*pGC->PolyFillRect) (pDraw, pGC, things,
		      (xRectangle *) &stuff[1]);
    return (client->noClientException);
}

int
ProcPolyFillArc               (client)
    register ClientPtr client;
{
    int		narcs;
    register GC *pGC;
    register DrawablePtr pDraw;
    REQUEST(xPolyFillArcReq);

    REQUEST_AT_LEAST_SIZE(xPolyFillArcReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);
    narcs = ((stuff->length << 2) - 
	     sizeof(xPolyFillArcReq)) / sizeof(xArc);
    if (narcs)
        (*pGC->PolyFillArc) (pDraw, pGC, narcs, (xArc *) &stuff[1]);
    return (client->noClientException);
}

int
ProcPutImage(client)
    register ClientPtr client;
{
    register GC *pGC;
    register DrawablePtr pDraw;
    REQUEST(xPutImageReq);

    REQUEST_AT_LEAST_SIZE(xPutImageReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);
    if (stuff->format == XYBitmap)
    {
        if ((stuff->depth != 1) || (stuff->leftPad > screenInfo.bitmapScanlineUnit))
            return BadMatch;
    }
    else if (stuff->format == XYPixmap)
    {
        if ((pDraw->depth != stuff->depth) || 
	    (stuff->leftPad > screenInfo.bitmapScanlineUnit))
            return BadMatch;
    }
    else if (stuff->format == ZPixmap)
    {
        if ((pDraw->depth != stuff->depth) || (stuff->leftPad != 0))
            return BadMatch;
    }
    else
        return BadValue;
    (*pGC->PutImage) (pDraw, pGC, stuff->depth, stuff->dstX, stuff->dstY,
		  stuff->width, stuff->height, 
		  stuff->leftPad, stuff->format, 
		  (char *) &stuff[1]);
     return (client->noClientException);
}

int
ProcGetImage(client)
    register ClientPtr	client;
{
    register DrawablePtr pDraw;
    int			nlines, linesPerBuf, widthBytesLine;
    register int	height, linesDone;
    int plane;
    char		*pBuf;
    xGetImageReply	xgi;

    REQUEST(xGetImageReq);

    height = stuff->height;
    REQUEST_SIZE_MATCH(xGetImageReq);
    if ((stuff->format != XYPixmap) && (stuff->format != ZPixmap))
        return(BadValue);
    if(!(pDraw = LOOKUP_DRAWABLE(stuff->drawable, client) ))
	return (BadDrawable);
    if(pDraw->type == DRAWABLE_WINDOW)
    {
      if( /* check for being on screen */
         ((WindowPtr) pDraw)->absCorner.x + stuff->x < 0 ||
         ((WindowPtr) pDraw)->absCorner.x + stuff->x + stuff->width >
             pDraw->pScreen->width ||
         ((WindowPtr) pDraw)->absCorner.y + stuff->y < 0 ||
         ((WindowPtr) pDraw)->absCorner.y + stuff->y + height >
             pDraw->pScreen->height ||
          /* check for being inside of border */
         stuff->x < -((WindowPtr)pDraw)->borderWidth ||
         stuff->x + stuff->width >
              ((WindowPtr)pDraw)->borderWidth +
              ((WindowPtr)pDraw)->clientWinSize.width ||
         stuff->y < -((WindowPtr)pDraw)->borderWidth ||
         stuff->y + stuff->height >
              ((WindowPtr)pDraw)->borderWidth +
              ((WindowPtr)pDraw)->clientWinSize.height
        )
	    return(BadMatch);
	xgi.visual = ((WindowPtr) pDraw)->visual;
    }
    else
    {
      if((stuff->x < 0) ||
         (stuff->x+stuff->width > ((PixmapPtr) pDraw)->width) ||
         (stuff->y < 0) ||
         (stuff->y+stuff->height > ((PixmapPtr) pDraw)->height)
        )
	    return(BadMatch);
	xgi.visual = None;
    }
    xgi.type = X_Reply;
    /* should this be set??? */
    xgi.sequenceNumber = client->sequence;
    xgi.depth = pDraw->depth;
    if(stuff->format == ZPixmap)
    {
	widthBytesLine = PixmapBytePad(stuff->width, pDraw->depth);
	xgi.length = (widthBytesLine >> 2) * stuff->height;
    }
    else 
    {
	widthBytesLine = PixmapBytePad(stuff->width, 1);
	xgi.length = (widthBytesLine >> 2) * stuff->height *
		     /* only planes asked for */
		     Ones(stuff->planeMask & ((1 << pDraw->depth) - 1));
    }
    linesPerBuf = IMAGE_BUFSIZE / widthBytesLine;
    if(!(pBuf = (char *) ALLOCATE_LOCAL(IMAGE_BUFSIZE)))
        return (client->noClientException = BadAlloc);

    WriteReplyToClient(client, sizeof (xGetImageReply), &xgi);

    if (stuff->format == ZPixmap)
    {
        linesDone = 0;
        while (height - linesDone > 0)
        {
	    nlines = min(linesPerBuf, height - linesDone);
	    (*pDraw->pScreen->GetImage) (pDraw,
	                                 stuff->x,
				         stuff->y + linesDone,
				         stuff->width, 
				         nlines,
				         stuff->format,
				         stuff->planeMask,
				         pBuf);
	    /* Note that this is NOT a call to WriteSwappedDataToClient,
               as we do NOT byte swap */
	    WriteToClient(client, nlines * widthBytesLine, pBuf);
	    linesDone += nlines;
        }
    }
    else
    {
        for (plane = 1 << (pDraw->depth - 1); plane; plane >>= 1)
	{
	    if (stuff->planeMask & plane)
	    {
	        linesDone = 0;
	        while (height - linesDone > 0)
	        {
		    nlines = min(linesPerBuf, height - linesDone);
	            (*pDraw->pScreen->GetImage) (pDraw,
	                                         stuff->x,
				                 stuff->y + linesDone,
				                 stuff->width, 
				                 nlines,
				                 stuff->format,
				                 plane,
				                 pBuf);
		    /* Note: NOT a call to WriteSwappedDataToClient,
		       as we do NOT byte swap */
		    WriteToClient(client, nlines * widthBytesLine, pBuf);
		    linesDone += nlines;
		}
            }
	}
    }
    DEALLOCATE_LOCAL(pBuf);
    return (client->noClientException);
}


int
ProcPolyText(client)
    register ClientPtr client;
{
    int		xorg;
    REQUEST(xPolyTextReq);
    register DrawablePtr pDraw;
    register GC *pGC;
    register FontPtr pFont;

    int (* polyText)();
    register unsigned char *pElt;
    unsigned char *pNextElt;
    unsigned char *endReq;
    int		itemSize;
    
#define TextEltHeader 2
#define FontShiftSize 5

    REQUEST_AT_LEAST_SIZE(xPolyTextReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);

    pElt = (unsigned char *)&stuff[1];
    endReq = ((unsigned char *) stuff) + (stuff->length <<2);
    xorg = stuff->x;
    if (stuff->reqType == X_PolyText8)
    {
	polyText = pGC->PolyText8;
	itemSize = 1;
    }
    else
    {
	polyText =  pGC->PolyText16;
	itemSize = 2;
    }

    while (endReq - pElt > TextEltHeader)
    {
	if (*pElt == FontChange)
        {
	    Font	fid;

	    if (endReq - pElt < FontShiftSize)
		 return (BadLength);
	    fid =  *(pElt+4)		/* big-endian */
		 | *(pElt+3) << 8
		 | *(pElt+2) << 16
		 | *(pElt+1) << 24;
	    pFont = (FontPtr)LookupID(fid, RT_FONT, RC_CORE);
	    if (!pFont)
	    {
		client->errorValue = fid;
		return (BadFont);
	    }
	    if (pFont != pGC->font)
	    {
		DoChangeGC( pGC, GCFont, &fid, 0);
		ValidateGC(pDraw, pGC);
	    }
	    pElt += FontShiftSize;
	}
	else	/* print a string */
	{
	    pNextElt = pElt + TextEltHeader + (*pElt)*itemSize;
	    if ( pNextElt > endReq)
		return( BadLength);
	    xorg += *((char *)(pElt + 1));	/* must be signed */
	    xorg = (* polyText)(pDraw, pGC, xorg, stuff->y, *pElt,
		pElt + TextEltHeader);
	    pElt = pNextElt;
	}
    }
    return (client->noClientException);
#undef TextEltHeader
#undef FontShiftSize
}

int
ProcImageText(client)
    register ClientPtr client;
{
    register DrawablePtr pDraw;
    register GC *pGC;

    REQUEST(xImageTextReq);

    REQUEST_AT_LEAST_SIZE(xImageTextReq);
    VALIDATE_DRAWABLE_AND_GC(stuff->drawable, pDraw, pGC, client);

    (*((stuff->reqType == X_ImageText8) ? pGC->ImageText8 : pGC->ImageText16))
	(pDraw, pGC, stuff->x, stuff->y, stuff->nChars, &stuff[1]);
    return (client->noClientException);
}


int
ProcCreateColormap(client)
    register ClientPtr client;
{
    VisualPtr	pVisual;
    ColormapPtr	pmap;
    int		mid;
    register WindowPtr   pWin;
    REQUEST(xCreateColormapReq);
    int result;

    REQUEST_SIZE_MATCH(xCreateColormapReq);

    if ((stuff->alloc != AllocNone) && (stuff->alloc != AllocAll))
        return(BadValue);
    mid = stuff->mid;
    LEGAL_NEW_RESOURCE(mid);    
    pWin = (WindowPtr)LookupWindow(stuff->window, client);
    if (!pWin)
        return(BadWindow);

    pVisual = (VisualPtr)LookupID(stuff->visual, RT_VISUALID, RC_CORE);
    if ((!pVisual) || pVisual->screen != pWin->drawable.pScreen->myNum)
    {
	client->errorValue = stuff->visual;
	return(BadValue);
    }
    result =  CreateColormap(mid, pWin->drawable.pScreen,
        pVisual, &pmap, stuff->alloc, client->index);
    if (client->noClientException != Success)
        return(client->noClientException);
    else
        return(result);
}

int
ProcFreeColormap(client)
    register ClientPtr client;
{
    ColormapPtr pmap;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pmap = (ColormapPtr )LookupID(stuff->id, RT_COLORMAP, RC_CORE);
    if (pmap) 
    {
	FreeColormap(pmap, (client->index << CLIENTOFFSET));
	FreeResource(stuff->id, RC_NONE);
	return (client->noClientException);
    }
    else 
    {
	client->errorValue = stuff->id;
	return (BadColor);
    }
}


int
ProcCopyColormapAndFree(client)
    register ClientPtr client;
{
    int		mid;
    ColormapPtr	pSrcMap;
    REQUEST(xCopyColormapAndFreeReq);
    int result;

    REQUEST_SIZE_MATCH(xCopyColormapAndFreeReq);
    mid = stuff->mid;
    LEGAL_NEW_RESOURCE(mid);
    if(pSrcMap = (ColormapPtr )LookupID(stuff->srcCmap, RT_COLORMAP, RC_CORE))
    {
	result = CopyColormapAndFree(mid, pSrcMap, client->index);
	if (client->noClientException != Success)
            return(client->noClientException);
	else
            return(result);
    }
    else
    {
	client->errorValue = stuff->srcCmap;
	return(BadColor);
    }
}

int
ProcInstallColormap(client)
    register ClientPtr client;
{
    ColormapPtr pcmp;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pcmp = (ColormapPtr  )LookupID(stuff->id, RT_COLORMAP, RC_CORE);
    if (pcmp)
    {
        (*(pcmp->pScreen->InstallColormap)) (pcmp);
        return (client->noClientException);        
    }
    else
    {
        client->errorValue = stuff->id;
        return (BadColor);
    }
}

int
ProcUninstallColormap(client)
    register ClientPtr client;
{
    ColormapPtr pcmp;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pcmp = (ColormapPtr )LookupID(stuff->id, RT_COLORMAP, RC_CORE);
    if (pcmp)
    {
	if(pcmp->mid != pcmp->pScreen->defColormap)
            (*(pcmp->pScreen->UninstallColormap)) (pcmp);
        return (client->noClientException);        
    }
    else
    {
        client->errorValue = stuff->id;
        return (BadColor);
    }
}

int
ProcListInstalledColormaps(client)
    register ClientPtr client;
{
    xListInstalledColormapsReply *preply; 
    int nummaps;
    WindowPtr pWin;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pWin = (WindowPtr)LookupWindow(stuff->id, client);

    if (!pWin)
        return(BadWindow);

    preply = (xListInstalledColormapsReply *) 
		ALLOCATE_LOCAL(sizeof(xListInstalledColormapsReply) +
		     pWin->drawable.pScreen->maxInstalledCmaps *
		     sizeof(Colormap));
    if(!preply)
        return(client->noClientException = BadAlloc);

    preply->type = X_Reply;
    preply->sequenceNumber = client->sequence;
    nummaps = (*pWin->drawable.pScreen->ListInstalledColormaps)
        (pWin->drawable.pScreen, (Colormap *)&preply[1]);
    preply->nColormaps = nummaps;
    preply->length = nummaps;
    WriteReplyToClient(client, sizeof (xListInstalledColormapsReply), preply);
    client->pSwapReplyFunc = Swap32Write;
    WriteSwappedDataToClient(client, nummaps * sizeof(Colormap), &preply[1]);
    DEALLOCATE_LOCAL(preply);
    return(client->noClientException);
}

int
ProcAllocColor                (client)
    register ClientPtr client;
{
    ColormapPtr pmap;
    int	retval;
    xAllocColorReply acr;
    REQUEST(xAllocColorReq);

    REQUEST_SIZE_MATCH(xAllocColorReq);
    pmap = (ColormapPtr )LookupID(stuff->cmap, RT_COLORMAP, RC_CORE);
    if (pmap)
    {
	acr.type = X_Reply;
	acr.length = 0;
	acr.sequenceNumber = client->sequence;
	acr.red = stuff->red;
	acr.green = stuff->green;
	acr.blue = stuff->blue;
	acr.pixel = 0;
	if(retval = AllocColor(pmap, &acr.red, &acr.green, &acr.blue,
	                       &acr.pixel, client->index))
	{
            if (client->noClientException != Success)
                return(client->noClientException);
	    else
	        return (retval);
	}
        WriteReplyToClient(client, sizeof(xAllocColorReply), &acr);
	return (client->noClientException);

    }
    else
    {
        client->errorValue = stuff->cmap;
        return (BadColor);
    }
}

int
ProcAllocNamedColor           (client)
    register ClientPtr client;
{
    ColormapPtr pcmp;
    REQUEST(xAllocNamedColorReq);

    REQUEST_AT_LEAST_SIZE(xAllocNamedColorReq);
    pcmp = (ColormapPtr )LookupID(stuff->cmap, RT_COLORMAP, RC_CORE);
    if (pcmp)
    {
	int		retval;

	xAllocNamedColorReply ancr;

	ancr.type = X_Reply;
	ancr.length = 0;
	ancr.sequenceNumber = client->sequence;

	if(OsLookupColor(pcmp->pScreen->myNum, (char *)&stuff[1], stuff->nbytes,
	                 &ancr.exactRed, &ancr.exactGreen, &ancr.exactBlue))
	{
	    ancr.screenRed = ancr.exactRed;
	    ancr.screenGreen = ancr.exactGreen;
	    ancr.screenBlue = ancr.exactBlue;
	    ancr.pixel = 0;
	    if(retval = AllocColor(pcmp,
	                 &ancr.screenRed, &ancr.screenGreen, &ancr.screenBlue,
			 &ancr.pixel, client->index))
	    {
                if (client->noClientException != Success)
                    return(client->noClientException);
                else
    	            return(retval);
	    }
            WriteReplyToClient(client, sizeof (xAllocNamedColorReply), &ancr);
	    return (client->noClientException);
	}
	else
	    return(BadName);
	
    }
    else
    {
        client->errorValue = stuff->cmap;
        return (BadColor);
    }
}

int
ProcAllocColorCells           (client)
    register ClientPtr client;
{
    ColormapPtr pcmp;
    REQUEST(xAllocColorCellsReq);

    REQUEST_SIZE_MATCH(xAllocColorCellsReq);
    pcmp = (ColormapPtr )LookupID(stuff->cmap, RT_COLORMAP, RC_CORE);
    if (pcmp)
    {
	xAllocColorCellsReply	accr;
	int			npixels, nmasks, retval;
	unsigned long		*ppixels, *pmasks;

	npixels = stuff->colors;
	nmasks = stuff->planes;
	ppixels = (unsigned long *)ALLOCATE_LOCAL(npixels * sizeof(long) + 
						  nmasks * sizeof(long));
	if(!ppixels)
            return(client->noClientException = BadAlloc);
	pmasks = ppixels + npixels;

	if(retval = AllocColorCells(client->index, pcmp, npixels, nmasks, 
	                            stuff->contiguous, ppixels, pmasks))
	{
	    DEALLOCATE_LOCAL(ppixels);
            if (client->noClientException != Success)
                return(client->noClientException);
	    else
	        return(retval);
	}
	accr.type = X_Reply;
	accr.length = ( (npixels + nmasks) * sizeof(long)) >> 2;
	accr.sequenceNumber = client->sequence;
	accr.nPixels = npixels;
	accr.nMasks = nmasks;
        WriteReplyToClient(client, sizeof (xAllocColorCellsReply), &accr);
	client->pSwapReplyFunc = Swap32Write;
        WriteSwappedDataToClient(client, (npixels + nmasks) * sizeof (long), ppixels);
	DEALLOCATE_LOCAL(ppixels);
        return (client->noClientException);        
    }
    else
    {
        client->errorValue = stuff->cmap;
        return (BadColor);
    }
}

int
ProcAllocColorPlanes(client)
    register ClientPtr client;
{
    ColormapPtr pcmp;
    REQUEST(xAllocColorPlanesReq);

    REQUEST_SIZE_MATCH(xAllocColorPlanesReq);
    pcmp = (ColormapPtr )LookupID(stuff->cmap, RT_COLORMAP, RC_CORE);
    if (pcmp)
    {
	xAllocColorPlanesReply	acpr;
	int			npixels, retval;
	unsigned long		*ppixels;

	npixels = stuff->colors;
	acpr.type = X_Reply;
	acpr.sequenceNumber = client->sequence;
	acpr.nPixels = npixels;
	npixels *= sizeof(long);
	ppixels = (unsigned long *)ALLOCATE_LOCAL(npixels);
	if(!ppixels)
            return(client->noClientException = BadAlloc);
	if(retval = AllocColorPlanes(client->index, pcmp, stuff->colors,
	    stuff->red, stuff->green, stuff->blue, stuff->contiguous, ppixels,
	    &acpr.redMask, &acpr.greenMask, &acpr.blueMask))
	{
            DEALLOCATE_LOCAL(ppixels);
            if (client->noClientException != Success)
                return(client->noClientException);
	    else
	        return(retval);
	}
	acpr.length = npixels >> 2;
	WriteReplyToClient(client, sizeof(xAllocColorPlanesReply), &acpr);
	client->pSwapReplyFunc = Swap32Write;
	WriteSwappedDataToClient(client, npixels, ppixels);
	DEALLOCATE_LOCAL(ppixels);
        return (client->noClientException);        
    }
    else
    {
        client->errorValue = stuff->cmap;
        return (BadColor);
    }
}

int
ProcFreeColors          (client)
    register ClientPtr client;
{
    ColormapPtr pcmp;
    REQUEST(xFreeColorsReq);

    REQUEST_AT_LEAST_SIZE(xFreeColorsReq);
    pcmp = (ColormapPtr )LookupID(stuff->cmap, RT_COLORMAP, RC_CORE);
    if (pcmp)
    {
	int	count;
        int     retval;

	if(pcmp->flags & AllAllocated)
	    return(BadAccess);
	count = ((stuff->length << 2)- sizeof(xFreeColorsReq)) >> 2;
	retval =  FreeColors(pcmp, client->index, count,
	    (unsigned long *)&stuff[1], stuff->planeMask);
        if (client->noClientException != Success)
            return(client->noClientException);
        else
            return(retval);

    }
    else
    {
        client->errorValue = stuff->cmap;
        return (BadColor);
    }
}

int
ProcStoreColors               (client)
    register ClientPtr client;
{
    ColormapPtr pcmp;
    REQUEST(xStoreColorsReq);

    REQUEST_AT_LEAST_SIZE(xStoreColorsReq);
    pcmp = (ColormapPtr )LookupID(stuff->cmap, RT_COLORMAP, RC_CORE);
    if (pcmp)
    {
	int	count;
        int     retval;

        if(pcmp->flags & AllAllocated)
	    if(CLIENT_ID(stuff->cmap) != client->index)
	        return(BadAccess);
        count =
	  ((stuff->length << 2) - sizeof(xStoreColorsReq)) / sizeof(xColorItem);
	retval = StoreColors(pcmp, count, (xColorItem *)&stuff[1]);
        if (client->noClientException != Success)
            return(client->noClientException);
        else
            return(retval);
    }
    else
    {
        client->errorValue = stuff->cmap;
        return (BadColor);
    }
}

int
ProcStoreNamedColor           (client)
    register ClientPtr client;
{
    ColormapPtr pcmp;
    REQUEST(xStoreNamedColorReq);

    REQUEST_AT_LEAST_SIZE(xStoreNamedColorReq);
    pcmp = (ColormapPtr )LookupID(stuff->cmap, RT_COLORMAP, RC_CORE);
    if (pcmp)
    {
	xColorItem	def;
        int             retval;

        if(pcmp->flags & AllAllocated)
	    if(CLIENT_ID(stuff->cmap) != client->index)
	        return(BadAccess);

	if(OsLookupColor(pcmp->pScreen->myNum, (char *)&stuff[1],
	                 stuff->nbytes, &def.red, &def.green, &def.blue))
	{
	    def.flags = stuff->flags;
	    retval = StoreColors(pcmp, 1, &def);
            if (client->noClientException != Success)
                return(client->noClientException);
	    else
		return(retval);
	}
        return (BadName);        
    }
    else
    {
        client->errorValue = stuff->cmap;
        return (BadColor);
    }
}

int
ProcQueryColors(client)
    register ClientPtr client;
{
    ColormapPtr pcmp;
    REQUEST(xQueryColorsReq);

    REQUEST_AT_LEAST_SIZE(xQueryColorsReq);
    pcmp = (ColormapPtr )LookupID(stuff->cmap, RT_COLORMAP, RC_CORE);
    if (pcmp)
    {
	int			count, retval;
	xrgb 			*prgbs;
	xQueryColorsReply	qcr;

	count = ((stuff->length << 2) - sizeof(xQueryColorsReq)) >> 2;
	if(!(prgbs = (xrgb *)ALLOCATE_LOCAL(count * sizeof(xrgb))))
            return(client->noClientException = BadAlloc);
	if(retval = QueryColors(pcmp, count, (unsigned long *)&stuff[1], prgbs))
	{
   	    DEALLOCATE_LOCAL(prgbs);
	    if (client->noClientException != Success)
                return(client->noClientException);
	    else
	        return (retval);
	}
	qcr.type = X_Reply;
	qcr.length = (count * sizeof(xrgb)) >> 2;
	qcr.sequenceNumber = client->sequence;
	qcr.nColors = count;
	WriteReplyToClient(client, sizeof(xQueryColorsReply), &qcr);
	client->pSwapReplyFunc = SQColorsExtend;
	WriteSwappedDataToClient(client, count * sizeof(xrgb), prgbs);
	DEALLOCATE_LOCAL(prgbs);
	return(client->noClientException);
	
    }
    else
    {
        client->errorValue = stuff->cmap;
        return (BadColor);
    }
} 

int
ProcLookupColor(client)
    register ClientPtr client;
{
    ColormapPtr pcmp;
    REQUEST(xLookupColorReq);

    REQUEST_AT_LEAST_SIZE(xLookupColorReq);
    pcmp = (ColormapPtr )LookupID(stuff->cmap, RT_COLORMAP, RC_CORE);
    if (pcmp)
    {
	xLookupColorReply lcr;

	if(OsLookupColor(pcmp->pScreen->myNum, (char *)&stuff[1], stuff->nbytes,
	                 &lcr.exactRed, &lcr.exactGreen, &lcr.exactBlue))
	{
	    lcr.type = X_Reply;
	    lcr.length = 0;
	    lcr.sequenceNumber = client->sequence;
	    lcr.screenRed = lcr.exactRed;
	    lcr.screenGreen = lcr.exactGreen;
	    lcr.screenBlue = lcr.exactBlue;
	    (*pcmp->pScreen->ResolveColor)(&lcr.screenRed,
	                                   &lcr.screenGreen,
					   &lcr.screenBlue);
	    WriteReplyToClient(client, sizeof(xLookupColorReply), &lcr);
	    return(client->noClientException);
	}
        return (BadName);        
    }
    else
    {
        client->errorValue = stuff->cmap;
        return (BadColor);
    }
}

int
ProcCreateCursor( client)
    register ClientPtr client;
{
    CursorPtr	pCursor;

    register PixmapPtr 	src;
    register PixmapPtr 	msk;
    unsigned int *	srcbits;
    unsigned int *	mskbits;
    int		width, height;
    CursorMetricRec cm;


    REQUEST(xCreateCursorReq);

    REQUEST_SIZE_MATCH(xCreateCursorReq);
    LEGAL_NEW_RESOURCE(stuff->cid);

    src = (PixmapPtr)LookupID( stuff->source, RT_PIXMAP, RC_CORE);
    msk = (PixmapPtr)LookupID( stuff->mask, RT_PIXMAP, RC_CORE);
    if (   src == (PixmapPtr)NULL)
	return (BadPixmap);
    if ( msk == (PixmapPtr)NULL)
	msk = src;

    if (  src->width != msk->width
       || src->height != msk->height
       || src->drawable.depth != 1
       || msk->drawable.depth != 1)
	return (BadMatch);

    width = src->width;
    height = src->height;

    if ( stuff->x > width 
      || stuff->y > height )
	return (BadMatch);

    srcbits = (unsigned int *)Xalloc( PixmapBytePad(width, 1)*height); 
    mskbits = (unsigned int *)Xalloc( PixmapBytePad(width, 1)*height); 

    (* src->drawable.pScreen->GetImage)( src, 0, 0, width, height,
					 XYBitmap, 0xffffffff, srcbits);
    (* msk->drawable.pScreen->GetImage)( msk, 0, 0, width, height,
					 XYBitmap, 0xffffffff, mskbits);
    cm.width = width;
    cm.height = height;
    cm.xhot = stuff->x;
    cm.yhot = stuff->y;
    pCursor = AllocCursor( srcbits, mskbits, &cm,
	    stuff->foreRed, stuff->foreGreen, stuff->foreBlue,
	    stuff->backRed, stuff->backGreen, stuff->backBlue);

    AddResource( stuff->cid, RT_CURSOR, pCursor, FreeCursor, RC_CORE);
    return (client->noClientException);
}

/*
 * protocol requires positioning of glyphs so hot-spots are coincident	XXX
 */
int
ProcCreateGlyphCursor( client)
    register ClientPtr client;
{
    FontPtr  sourcefont;
    FontPtr  maskfont;
    char   *srcbits;
    char   *mskbits;
    CursorPtr pCursor;
    CursorMetricRec cm;
    int res;

    REQUEST(xCreateGlyphCursorReq);

    REQUEST_SIZE_MATCH(xCreateGlyphCursorReq);
    LEGAL_NEW_RESOURCE(stuff->cid);

    sourcefont = (FontPtr) LookupID(stuff->source, RT_FONT, RC_CORE);
    maskfont = (FontPtr) LookupID(stuff->mask, RT_FONT, RC_CORE);

    if (sourcefont == (FontPtr) NULL)
    {
	client->errorValue = stuff->source;
	return(BadFont);
    }

    if (maskfont == (FontPtr) NULL)
    {
	client->errorValue = stuff->mask;
	return(BadFont);
    }

    if (!CursorMetricsFromGlyph(maskfont, stuff->maskChar, &cm))
    {
	client->errorValue = stuff->mask;
	return BadValue;
    }

    if (res = ServerBitsFromGlyph(stuff->source, 
				  sourcefont, stuff->sourceChar,
				  &cm, &srcbits))
	return res;
    if (res = ServerBitsFromGlyph(stuff->mask, 
				  maskfont, stuff->maskChar,
				  &cm, &mskbits))
	return res;

    pCursor = AllocCursor(srcbits, mskbits, &cm,
	    stuff->foreRed, stuff->foreGreen, stuff->foreBlue,
	    stuff->backRed, stuff->backGreen, stuff->backBlue);

    AddResource(stuff->cid, RT_CURSOR, pCursor, FreeCursor, RC_CORE);
    return client->noClientException;
}


int
ProcFreeCursor(client)
    register ClientPtr client;
{
    CursorPtr pCursor;
    REQUEST(xResourceReq);

    REQUEST_SIZE_MATCH(xResourceReq);
    pCursor = (CursorPtr)LookupID(stuff->id, RT_CURSOR, RC_CORE);
    if (pCursor) 
    {
	FreeResource( stuff->id, RC_NONE);
	return (client->noClientException);
    }
    else 
    {
	return (BadCursor);
    }
}

int
ProcQueryBestSize   (client)
    register ClientPtr client;
{
    xQueryBestSizeReply	reply;
    register DrawablePtr pDraw;
    ScreenPtr pScreen;
    REQUEST(xQueryBestSizeReq);

    REQUEST_SIZE_MATCH(xQueryBestSizeReq);
    if ((stuff->class != CursorShape) && 
	(stuff->class != TileShape) && 
	(stuff->class != StippleShape))
        return(BadValue);
    if (!(pDraw = LOOKUP_DRAWABLE(stuff->drawable, client)))
    {
	client->errorValue = stuff->drawable;
	return (BadDrawable);
    }
    pScreen = pDraw->pScreen;
    (* pScreen->QueryBestSize)(stuff->class, &stuff->width,
			       &stuff->height);
    reply.type = X_Reply;
    reply.length = 0;
    reply.sequenceNumber = client->sequence;
    reply.width = stuff->width;
    reply.height = stuff->height;
    WriteReplyToClient(client, sizeof(xQueryBestSizeReply), &reply);
    return (client->noClientException);
}


int
ProcSetScreenSaver            (client)
    register ClientPtr client;
{
    int blankingOption, exposureOption;
    REQUEST(xSetScreenSaverReq);

    REQUEST_SIZE_MATCH(xSetScreenSaverReq);
    blankingOption = stuff->preferBlank;
    if ((blankingOption != DontPreferBlanking) &&
        (blankingOption != PreferBlanking) &&
        (blankingOption != DefaultBlanking))
        return BadMatch;

    exposureOption = stuff->allowExpose;
    if ((exposureOption != DontAllowExposures) &&
        (exposureOption != AllowExposures) &&
        (exposureOption != DefaultExposures))
        return BadMatch;

    if ((stuff->timeout < -1) || (stuff->interval < -1))
        return BadMatch;

    ScreenSaverBlanking = blankingOption; 
    ScreenSaverAllowExposures = exposureOption;

    if (stuff->timeout >= 0)
	ScreenSaverTime = stuff->timeout * MILLI_PER_SECOND;
    else 
	ScreenSaverTime = DEFAULT_SCREEN_SAVER_TIME;
    if (stuff->interval > 0)
	ScreenSaverInterval = stuff->interval * MILLI_PER_SECOND;
    else
	ScreenSaverInterval = DEFAULT_SCREEN_SAVER_TIME;
    return (client->noClientException);
}

int
ProcGetScreenSaver(client)
    register ClientPtr client;
{
    xGetScreenSaverReply rep;

    rep.type = X_Reply;
    rep.length = 0;
    rep.sequenceNumber = client->sequence;
    rep.timeout = ScreenSaverTime / MILLI_PER_SECOND;
    rep.interval = ScreenSaverInterval / MILLI_PER_SECOND;
    rep.preferBlanking = ScreenSaverBlanking;
    rep.allowExposures = ScreenSaverAllowExposures;
    WriteReplyToClient(client, sizeof(xGetScreenSaverReply), &rep);
    return (client->noClientException);
}

int
ProcChangeHosts(client)
    register ClientPtr client;
{
    REQUEST(xChangeHostsReq);
    int result;

    REQUEST_AT_LEAST_SIZE(xChangeHostsReq);

    if(stuff->mode == HostInsert)
	result = AddHost(client, stuff->hostFamily, stuff->hostLength, 
		&stuff[1]);
    else if (stuff->mode == HostDelete)
	result = RemoveHost(client, stuff->hostFamily, 
			    stuff->hostLength,  &stuff[1]);  
    else
        return BadValue;
    return (result || client->noClientException);
}

int
ProcListHosts(client)
    register ClientPtr client;
{
extern int GetHosts();
    xListHostsReply reply;
    int	len, nHosts;
    pointer	pdata;
    REQUEST(xListHostsReq);

    REQUEST_SIZE_MATCH(xListHostsReq);
    if((len = GetHosts(&pdata, &nHosts, &reply.enabled)) < 0)
	return(BadImplementation);
    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;
    reply.nHosts = nHosts;
    reply.length = len >> 2;
    WriteReplyToClient(client, sizeof(xListHostsReply), &reply);
    client->pSwapReplyFunc = SLHostsExtend;
    WriteSwappedDataToClient(client, len, pdata);
    Xfree(pdata);
    return (client->noClientException);
}

int
ProcChangeAccessControl(client)
    register ClientPtr client;
{
    REQUEST(xSetAccessControlReq);

    REQUEST_SIZE_MATCH(xSetAccessControlReq);
    if ((stuff->mode != EnableAccess) && (stuff->mode != DisableAccess))
        return BadValue;
    ChangeAccessControl(client, stuff->mode == EnableAccess);
    return (client->noClientException);
}

int
ProcKillClient(client)
    register ClientPtr client;
{
    REQUEST(xResourceReq);

    pointer *pResource;
    int clientIndex;

    REQUEST_SIZE_MATCH(xResourceReq);
    if (stuff->id == AllTemporary)
    {
	CloseDownRetainedResources();
        return (client->noClientException);
    }
    pResource = (pointer *)LookupID(stuff->id, RT_ANY, RC_CORE);
  
    clientIndex = CLIENT_ID(stuff->id);

    if (clientIndex && pResource)
    {
    	if (clients[clientIndex] && !clients[clientIndex]->clientGone)
 	{
	    CloseDownClient(clients[clientIndex]);
            return (client->noClientException);
	}
    }
    else   /* can't kill client 0, which is server */
    {
	client->errorValue = stuff->id;
	return (BadValue);
    }
}

int
ProcSetFontPath(client)
    register ClientPtr client;
{
    REQUEST(xSetFontPathReq);
    
    REQUEST_AT_LEAST_SIZE(xSetFontPathReq);
    
    SetFontPath(stuff->nFonts, stuff->length, &stuff[1]);
    return (client->noClientException);
}

int
ProcGetFontPath(client)
    register ClientPtr client;
{
    FontPathPtr pFP;
    xGetFontPathReply reply;
    int stringLens, i;
    char *bufferStart;
    register char  *bufptr;
    REQUEST (xReq);

    REQUEST_SIZE_MATCH(xReq);
    pFP = GetFontPath();
    stringLens = 0;
    for (i=0; i<pFP->npaths; i++)
        stringLens += pFP->length[i];

    reply.type = X_Reply;
    reply.sequenceNumber = client->sequence;
    reply.length = (stringLens + pFP->npaths + 3) >> 2;
    reply.nPaths = pFP->npaths;

    bufptr = bufferStart = (char *)ALLOCATE_LOCAL(reply.length << 2);
    if(!bufptr)
        return(client->noClientException = BadAlloc);
            /* since WriteToClient long word aligns things, 
	       copy to temp buffer and write all at once */
    for (i=0; i<pFP->npaths; i++)
    {
        *bufptr++ = pFP->length[i];
        bcopy(pFP->paths[i], bufptr,  pFP->length[i]);
        bufptr += pFP->length[i];
    }
    WriteReplyToClient(client, sizeof(xGetFontPathReply), &reply);
    WriteToClient(client, stringLens + pFP->npaths, bufferStart);
    DEALLOCATE_LOCAL(bufferStart);
    return(client->noClientException);
}

int
ProcChangeCloseDownMode(client)
    register ClientPtr client;
{
    REQUEST(xSetCloseDownModeReq);

    REQUEST_SIZE_MATCH(xSetCloseDownModeReq);
    if ((stuff->mode == AllTemporary) ||
	(stuff->mode == RetainPermanent) ||
	(stuff->mode == RetainTemporary))
    {
	client->closeDownMode = stuff->mode;
	return (client->noClientException);
    }
    else   
    {
	client->errorValue = stuff->mode;
	return (BadValue);
    }
}

int ProcForceScreenSaver(client)
    register ClientPtr client;
{    
    REQUEST(xForceScreenSaverReq);

    REQUEST_SIZE_MATCH(xForceScreenSaverReq);
    
    if ((stuff->mode != ScreenSaverReset) && 
	(stuff->mode != ScreenSaverActive))
        return BadValue;
    SaveScreens(SCREEN_SAVER_FORCER, stuff->mode);
    return client->noClientException;
}

int ProcNoOperation(client)
    register ClientPtr client;
{
    REQUEST(xReq);

    REQUEST_AT_LEAST_SIZE(xReq);
    
    /* noop -- don't do anything */
    return(client->noClientException);
}

extern void NotImplemented();

void
InitProcVectors()
{
    int i;
    for (i = 0; i<256; i++)
    {
	if(!ProcVector[i])
	{
            ProcVector[i] = SwappedProcVector[i] = ProcBadRequest;
	    ReplySwapVector[i] = NotImplemented;
	}
    }
    for(i = LASTEvent; i < 128; i++)
    {
	EventSwapVector[i] = NotImplemented;
    }
    
}

/**********************
 * CloseDownClient
 *
 *  Client can either mark his resources destroy or retain.  If retained and
 *  then killed again, the client is really destroyed.
 *********************/

void
CloseDownClient(client)
    register ClientPtr client;
{
    extern void DeleteClientFromAnySelections();
    register int i;
      /* ungrab server if grabbing client dies */
    if (grabbingClient &&  (onlyClient == client))
    {
	grabbingClient = FALSE;
	ListenToAllClients();
    }
    DeleteClientFromAnySelections(client);
    ReleaseActiveGrabs(client);
    
    if (client->closeDownMode == DestroyAll)
    {
        client->clientGone = TRUE;  /* so events aren't sent to client */
        CloseDownConnection(client);
        FreeClientResources(client);
        for (i=0; i<currentMaxClients; i++)
            if (clients[i] == client)
	    {
		nextFreeClientID = i;
                clients[nextFreeClientID] = NullClient;
		break;
	    }
        Xfree(client);
	if(--nClients == 0)
	    nClients = -1;
    }
            /* really kill resources this time */
    else if (client->clientGone)
    {
        FreeClientResources(client);
        for (i=0; i<currentMaxClients; i++)
            if (clients[i] == client)
	    {
		nextFreeClientID = i;
                clients[nextFreeClientID] = NullClient;
		break;
	    }
        Xfree(client);
	--nClients;
    }
    else
    {
        client->clientGone = TRUE;
        CloseDownConnection(client);
    }
}

static void
KillAllClients()
{
    int i;
    for (i=1; i<currentMaxClients; i++)
        if (clients[i] && !clients[i]->clientGone)
            CloseDownClient(clients[i]);     
}

void
KillServerResources()
{
    int i;

    KillAllClients();
    /* Good thing we stashed these two in globals so we could get at them
     * here. */
    CloseDownDevices(argcGlobal, argvGlobal);
    for (i = 0; i < screenInfo.numScreens; i++)
	(*screenInfo.screen[i].CloseScreen)(i, &screenInfo.screen[i]);
}


/*********************
 * CloseDownRetainedResources
 *
 *    Find all clients that are gone and have terminated in RetainTemporary 
 *    and  destroy their resources.
 *********************/

CloseDownRetainedResources()
{
    register int i;
    register ClientPtr client;

    for (i=1; i<currentMaxClients; i++)
    {
        client = clients[i];
        if (client && (client->closeDownMode == RetainTemporary)
	    && (client->clientGone))
	{
            FreeClientResources(client);
            nextFreeClientID = i;
	    Xfree(client);
	    clients[i] = NullClient;
	}
    }
}

/************************
 * int NextAvailableClientID()
 *
 * OS depedent portion can't assign client id's because of CloseDownModes.
 * Returns -1 if the there are no free clients.
 *************************/

ClientPtr
NextAvailableClient()
{
    int i;
    ClientPtr client;

    if (nextFreeClientID >= currentMaxClients)
        nextFreeClientID = 1;
    if (!clients[nextFreeClientID])
    {
	i = nextFreeClientID;
	nextFreeClientID++;
    }
    else
    {
	i = 1;
	while ((i<currentMaxClients) && (clients[i]))
            i++;
        if (i < currentMaxClients)
	    nextFreeClientID = i;
	else
        {
	    clients = (ClientPtr *)Xrealloc(clients, i * sizeof(ClientRec));
	    currentMaxClients++;
	}
    }
    clients[i] = client =  (ClientPtr)Xalloc(sizeof(ClientRec));
    client->index = i;
    client->sequence = 0; 
    client->clientAsMask = i << CLIENTOFFSET;
    client->closeDownMode = DestroyAll;
    client->clientGone = FALSE;
    client->lastDrawable = (DrawablePtr) NULL;
    client->lastDrawableID = INVALID;
    client->lastGC = (GCPtr) NULL;
    client->lastGCID = -1;
    client->numSaved = 0;
    client->saveSet = (pointer *)NULL;
    client->noClientException = Success;

    return(client);
}

SendConnectionSetupInfo(client)
    ClientPtr client;
{
    xWindowRoot *root;
    int i;

    ((xConnSetup *)ConnectionInfo)->ridBase = client->clientAsMask;
    ((xConnSetup *)ConnectionInfo)->ridMask = 0xfffff;
        /* fill in the "currentInputMask" */
    root = (xWindowRoot *)(ConnectionInfo + connBlockScreenStart);
    for (i=0; i<screenInfo.numScreens; root += sizeof(xWindowRoot), i++) 
        root->currentInputMask = WindowTable[i].allEventMasks;

    if (client->swapped) {
	WriteSConnSetupPrefix(client, &connSetupPrefix);
        WriteSConnectionInfo(client, connSetupPrefix.length << 2, ConnectionInfo);
	}
    else {
        WriteToClient(client, sizeof(xConnSetupPrefix), (char *) &connSetupPrefix);
        WriteToClient(client, connSetupPrefix.length << 2, ConnectionInfo);
	}
}

/*****************
 * Oops
 *    Send an Error back to the client. 
 *****************/

Oops (client, reqCode, minorCode, status)
    ClientPtr client;
    char reqCode, minorCode, status;
{
    xError rep;
    register int i;

    rep.type = X_Error;
    rep.sequenceNumber = client->sequence;
    rep.errorCode = status;
    rep.majorCode = reqCode;
    rep.minorCode = minorCode;
    rep.resourceID = client->errorValue;

    for (i=0; i<currentMaxClients; i++)
        if (clients[i] == client) break;
#ifdef notdef
    ErrorF(  "OOPS! => client: %x, seq: %d, err: %d, maj:%d, min: %d resID: %x\n",
    	client->index, rep.sequenceNumber, rep.errorCode,
	rep.majorCode, rep.minorCode, rep.resourceID);
#endif

    WriteEventsToClient (client, 1, (xEvent *) &rep); 
}


void
DeleteWindowFromAnySelections(pWin)
    WindowPtr pWin;
{
    int i = 0;

    for (i = 0; i< NumCurrentSelections; i++)
        if (CurrentSelections[i].pWin == pWin)
        {
            CurrentSelections[i].pWin = (WindowPtr)NULL;
            CurrentSelections[i].window = None;
	}
}

static void
DeleteClientFromAnySelections(client)
    ClientPtr client;
{
    int i = 0;

    for (i = 0; i< NumCurrentSelections; i++)
        if (CurrentSelections[i].client == client)
        {
            CurrentSelections[i].pWin = (WindowPtr)NULL;
            CurrentSelections[i].window = None;
	}
}

void
MarkClientException(client)
    ClientPtr client;
{
    client->noClientException = -1;
}


/* Byte swap a list of longs */

SwapLongs (list, count)
	register long *list;
	register int count;
{
	register int n;

	while (count >= 8) {
	    swapl(list+0, n);
	    swapl(list+1, n);
	    swapl(list+2, n);
	    swapl(list+3, n);
	    swapl(list+4, n);
	    swapl(list+5, n);
	    swapl(list+6, n);
	    swapl(list+7, n);
	    list += 8;
	    count -= 8;
	}
	while (--count >= 0) {
	    swapl(list, n);
	    list++;
	}
}

/* Byte swap a list of shorts */

SwapShorts (list, count)
	register short *list;
	register int count;
{
	register int n;

	while (count >= 16) {
	    swaps(list+0, n);
	    swaps(list+1, n);
	    swaps(list+2, n);
	    swaps(list+3, n);
	    swaps(list+4, n);
	    swaps(list+5, n);
	    swaps(list+6, n);
	    swaps(list+7, n);
	    swaps(list+8, n);
	    swaps(list+9, n);
	    swaps(list+10, n);
	    swaps(list+11, n);
	    swaps(list+12, n);
	    swaps(list+13, n);
	    swaps(list+14, n);
	    swaps(list+15, n);
	    list += 16;
	    count -= 16;
	}
	while (--count >= 0) 
        {
	    swaps(list, n);
	    list++;
	}
}
