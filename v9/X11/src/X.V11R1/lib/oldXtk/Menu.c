/* $Header: Menu.c,v 1.1 87/09/11 07:58:12 toddb Exp $ */
#ifndef lint
static char *sccsid = "@(#)Menu.c	1.6	2/26/87";
#endif lint

/*
 * Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts.
 * 
 *                         All Rights Reserved
 * 
 * Permission to use, copy, modify, and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, 
 * provided that the above copyright notice appear in all copies and that
 * both that copyright notice and this permission notice appear in 
 * supporting documentation, and that the name of Digital Equipment
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.  
 * 
 * 
 * DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
 * DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
 * ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/* 
 * Menu.c - Basic menu widget
 * 
 * Author:	M. Gancarz, based on ButtonBox.c by haynes
 */

#include	"Xlib.h"
#include	"Intrinsic.h"
#include        "Menu.h"
#include        "Atoms.h"

#define X10

#define MAXHEIGHT	((1 << 31)-1)
#define MAXWIDTH	((1 << 31)-1)

#define max(x, y)	(((x) > (y)) ? (x) : (y))
#define min(x, y)	(((x) < (y)) ? (x) : (y))
#define assignmax(x, y)	if ((y) > (x)) x = (y)
#define assignmin(x, y)	if ((y) < (x)) x = (y)

#define all_items	i = 0, item = data->entries;\
                        i < data->numentries; i++, item = item->next
#define item_w		(item->lug.w)
#define item_x		(item->lug.wb.x)
#define item_y		(item->lug.wb.y)
#define item_width	(item->lug.wb.width)
#define item_height	(item->lug.wb.height)
#define item_borderWidth	(item->lug.wb.borderWidth)

/****************************************************************
 *
 * Private Types
 *
 ****************************************************************/

typedef void (*MenuProc)();

typedef struct _WidgetEntryRec {
    struct _WidgetEntryRec	*next;		/* ptr to next entry */
    struct _WidgetEntryRec	*prev;		/* ptr to previous entry */
    WindowLug			lug;		/* data for this entry */
} WidgetEntryRec, *WidgetEntry;

typedef	struct _WidgetDataRec {
    Display		*dpy;		/* widget display connection */
    Window		w;		/* widget window */
    Position		x,y;		/* widget location */
    Dimension		width, height;	/* widget window width and height */
    Dimension		borderWidth;	/* widget window border width */
    XtOrientation	orient;		/* menu orientation */
    Pixel		bp;		/* widget window border pixmap */
    Pixel		bg;		/* widget window background pixmap */
    Dimension		space;		/* inter-entry spacing */
    Dimension		hpad, vpad;	/* pad between entries and parent */
    unsigned long	notify;		/* events which cause a callback */
    MenuProc		proc;		/* procedure to invoke for callback */
    caddr_t		tag;		/* widget client data */
    int			numentries;	/* number of entries */
    WidgetEntry		entries;	/* list of entries */
} WidgetDataRec, *WidgetData;

typedef struct _ParameterDataRec {
    XtMenuEntry	*entry;		/* arg MenuEntry */
    int		i;		/* arg index */
} ParameterDataRec, *ParameterData;


/****************************************************************
 *
 * Private Data
 *
 ****************************************************************/

static int	BlPixel, WhPixel;
static XContext  widgetContext;

extern void Dummy();

static WidgetDataRec globaldata;
static WidgetDataRec globalinit = {
    NULL,		/* Display dpy; */
    NULL,		/* Window w; */
    0,0,		/* x,y */
    1, 1,		/* int width, height; */
    1,			/* int borderWidth; */
    XtorientVertical,	/* XtOrientation orient; */
    0,			/* Border pixel */
    1,			/* Background pixel */
    1,			/* int space; */
    2, 2,		/* int hpad, vpad; */
    0,			/* int notify; */
    Dummy,		/* void (*Proc)(); */
    NULL,		/* caddr_t tag; */
    0,			/* int numentries; */
    NULL		/* WidgetEntry entries; */
};

static Resource resources[] = {
    {XtNwindow, XtCWindow, XrmRWindow,
	sizeof(Window), (caddr_t)&globaldata.w, (caddr_t)NULL},
    {XtNx, XtCX, XrmRInt,
	sizeof(int),(caddr_t)&globaldata.x,(caddr_t)NULL},
    {XtNy, XtCY, XrmRInt,
	sizeof(int),(caddr_t)&globaldata.y,(caddr_t)NULL},
    {XtNwidth, XtCWidth, XrmRInt,
        sizeof(int), (caddr_t)&globaldata.width, (caddr_t)NULL},
    {XtNheight, XtCHeight, XrmRInt,
        sizeof(int), (caddr_t)&globaldata.height, (caddr_t)NULL},
    {XtNorientation, XtCOrientation, XtROrientation,
        sizeof(XtOrientation), (caddr_t)&globaldata.orient, (caddr_t)NULL},
    {XtNborderWidth, XtCBorderWidth, XrmRInt,
        sizeof(int), (caddr_t)&globaldata.borderWidth, (caddr_t)NULL},
    {XtNborder, XtCColor, XrmRPixmap,
        sizeof(Pixmap), (caddr_t)&globaldata.bp, (caddr_t)NULL},
    {XtNbackground, XtCColor, XrmRPixmap,
        sizeof(Pixmap), (caddr_t)&globaldata.bg, (caddr_t)NULL},
    {XtNinternalWidth, XtCWidth, XrmRInt,
        sizeof(int), (caddr_t)&globaldata.hpad, (caddr_t)NULL},
    {XtNinternalHeight, XtCHeight, XrmRInt,
        sizeof(int), (caddr_t)&globaldata.vpad, (caddr_t)NULL},
    {XtNspace, XtCSpace, XrmRInt,
        sizeof(int), (caddr_t)&globaldata.space, (caddr_t)NULL},
    {XtNnotify, XtCNotify, XrmRInt,
        sizeof(int), (caddr_t)&globaldata.notify, (caddr_t)NULL},
    {XtNfunction, XtCFunction, XtRFunction,
        sizeof(MenuProc), (caddr_t)&globaldata.proc, (caddr_t)NULL},
    {XtNparameter, XtCParameter, XrmRPointer,
        sizeof(caddr_t), (caddr_t)&globaldata.tag, (caddr_t)NULL},
};


static ParameterDataRec parms;
static ParameterDataRec parminit = {
    NULL,	/* MenuEntry entry */
    -1,		/* int i; */
};

static Resource parmResources[] = {
    {XtNmenuEntry, XtCMenuEntry, XrmRPointer,
	sizeof(XtMenuEntry), (caddr_t)&parms.entry, (caddr_t)NULL},
};

/****************************************************************
 *
 * Private Routines
 *
 ****************************************************************/

static Boolean initialized = FALSE;

static void MenuInitialize(dpy)
Display *dpy;
{
    if (initialized)
    	return;
    initialized = TRUE;

    BlPixel = BlackPixel(dpy, DefaultScreen(dpy));
    WhPixel = WhitePixel(dpy, DefaultScreen(dpy));

    globalinit.bp = BlPixel;
    globalinit.bg = WhPixel;

    globalinit.orient = XtorientVertical;
    widgetContext = XUniqueContext();
}

/*
 * Given a display and window, get the widget data.
 */

static WidgetData DataFromWindow(dpy, window)
Display *dpy;
Window window;
{
    WidgetData result;
    if (XFindContext(dpy, window, widgetContext, (caddr_t *)&result))
        return NULL;
    return result;
}

/*
 * Default callback procedure
 */
/*ARGSUSED*/
static void Dummy(tag)
caddr_t tag;
{
    (void) puts("Menu: dummy callback");
}

extern void Destroy();

/*
 *
 * Do a layout, either actually assigning positions, or just calculating size
 *
 */

static int DoLayout(data)
WidgetData	data;
{
    int	i;
    int	vpad, hpad, sp;
    Dimension maxheight = 0, maxwidth = 0, maxborderWidth = 0;
    Dimension menuwidth, menuheight, dmaxborderWidth, borderWidth;
    Position newx, newy, xoffset, yoffset;
    Dimension newwidth, newheight;
    WidgetEntry item;

    if (data->numentries == 0) return(0);

    sp = data->space;
    hpad = data->hpad;
    vpad = data->vpad;

    for (all_items) {
        assignmax(maxheight, item->lug.wb.height);
        assignmax(maxwidth, item_width);
        assignmax(maxborderWidth, item_borderWidth);
    }

    dmaxborderWidth = maxborderWidth << 1;
    if (data->orient == XtorientVertical) {
        menuwidth = (hpad << 1) + maxwidth + dmaxborderWidth;
        menuheight = (data->numentries * (maxheight + dmaxborderWidth)) +
                     ((data->numentries - 1) * sp) + (vpad << 1);
    } else {
        menuwidth = (data->numentries * (maxwidth + dmaxborderWidth)) +
                     ((data->numentries - 1) * sp) + (hpad << 1);
        menuheight = (vpad << 1) + maxheight + dmaxborderWidth;
    }
    (void) TryLayout(data, menuwidth, menuheight);
   
    newx = hpad;
    newy = vpad;
    xoffset = maxwidth + dmaxborderWidth;
    yoffset = maxheight + dmaxborderWidth;
    for (all_items) {
        borderWidth = item_borderWidth << 1;
        newwidth = xoffset - borderWidth;
        newheight = yoffset - borderWidth;
        if (newwidth != item_width || newheight != item_height
         || newx != item_x || newy != item_y) {
            XMoveResizeWindow(data->dpy, item_w, 
	    	newx, newy, newwidth, newheight);
	    XtSendConfigureNotify(item_w,&(item->lug.wb));
	}
        item_x = newx;
        item_y = newy;
        item_width = newwidth;
        item_height = newheight;
        if (data->orient == XtorientVertical)
            newy += sp + yoffset;
        else newx += sp + xoffset;
    }

    return (1);
}

/*
 *
 * Compute the layout of the menu
 *
 */

/* ||| Why Layout calls DoLayout I don't know... */

static void Layout(data)
WidgetData	data;
{
    (void) DoLayout(data);
}

/*
 *
 * Generic widget event handler
 *
 */

static XtEventReturnCode EventHandler(event, eventdata)
XEvent *event;
caddr_t eventdata;
{
    WidgetData	data = (WidgetData) eventdata;

    switch (event->type) {
	case ConfigureNotify:
	    /* we expect to get an expose window on resize */
            if ((data->height != event->xconfigure.height) ||
               (data->width != event->xconfigure.width)) {
	        data->height = event->xconfigure.height;
	        data->width = event->xconfigure.width;
	        Layout(data);
            }
	    break;

        case DestroyNotify: Destroy(data); break;


        case Expose:
            break;
    }

    return (XteventHandled);
}

/*
 * Widget notify event handler
 */
/*ARGSUSED*/
static XtEventReturnCode Notify(event, eventdata)
XEvent *event;
caddr_t eventdata;
{
    WidgetData data = (WidgetData)eventdata;

    data->proc(data->tag);
    return(XteventHandled);
}

/*
 *
 * Destroy the widget
 *
 */

static void Destroy(data)
WidgetData	data;
{
    WidgetEntry item;
    int	i;
    /* send destroy messages to all my subwindows */
    for (all_items) {
	(void) XtSendDestroyNotify(data->dpy, item_w); }
	
    XtFree((char *) data->entries);

    XtClearEventHandlers(data->dpy, data->w);
    (void) XDeleteContext(data->dpy, data->w, widgetContext);
    XDestroyWindow (data->dpy,data->w);
    XtFree ((char *) data);
}

/*
 *
 * Find Entry
 *
 */

static WidgetEntry FindEntry(data, w)
    WidgetData	data;
    Window	w;
{
    int	i;
    WidgetEntry item;

    for (all_items)
	if (item_w == w) return (item);

    return NULL;
}

/*
 *
 * Resize Entry
 *
 */

static void ResizeEntry(data, b, reqBox, replBox)
WidgetData	data;
WindowLugPtr	b;
WindowBox	*reqBox;
WindowBox	*replBox;	/* RETURN */
{
    b->wb = *reqBox;
    XResizeWindow(data->dpy,b->w, b->wb.width, b->wb.height);
    Layout(data);
    *replBox = b->wb;
}

/*
 *
 * Try to do a new layout within a particular width and height
 *
 */

static int TryLayout(data, width, height)
WidgetData	data;
Dimension	width, height;
{
    WindowBox	box, rbox;

    /* let's see if our parent will go for it. */
    box.width = width;
    box.height = height;
    switch (XtMakeGeometryRequest(data->dpy, data->w, XtgeometryResize, &box, &rbox)) {

	case XtgeometryNoManager:
	    XResizeWindow(data->dpy,data->w, box.width, box.height);
	    /* fall through to "yes" */

	case XtgeometryYes:
	    data->width = box.width;
	    data->height = box.height;
	    return (1);


	case XtgeometryNo:
	    return (0);


	case XtgeometryAlmost:
	    box = rbox;
	    (void) XtMakeGeometryRequest(data->dpy, data->w, XtgeometryResize, &box, &rbox);
	    data->width = box.width;
	    data->height = box.height;
	    return (1);

    }
    return (0);
}

/*
 *
 * Try to do a new layout
 *
 */

static int TryNewLayout(data)
WidgetData	data;
{
    if (TryLayout(data, data->width, data->height)) return (1);
    if (TryLayout(data, data->width, MAXHEIGHT)) return (1);
    if (TryLayout(data, MAXWIDTH, MAXHEIGHT)) return(1);
    return (0);
}

/*
 *
 * Entry Resize Request
 *
 */

static XtGeometryReturnCode ResizeEntryRequest(data, w, reqBox, replBox)
WidgetData	data;
Window		w;
WindowBox	*reqBox;
WindowBox	*replBox;	/* RETURN */

{
    WidgetEntry item;
    WindowLug oldb;

    item = FindEntry(data, w);
    if (item == NULL) return (XtgeometryNo);
    oldb = item->lug;
    item->lug.wb = *reqBox;

    if ((reqBox->width <= item_width) && (reqBox->height <= item_height)) {
        /* making the button smaller always works */
        ResizeEntry(data, &(item->lug), reqBox, replBox);
	return (XtgeometryYes);
    }

    if (TryNewLayout(data)) {
	ResizeEntry(data, &(item->lug), reqBox, replBox);
	return (XtgeometryYes);
    }

    item->lug = oldb;
    return (XtgeometryNo);
}

/*
 *
 * Menu Entry Geometry Manager
 *
 */

static XtGeometryReturnCode EntryGeometryManager(
	dpy, w, req, reqBox, replBox)
Display		*dpy;
Window		w;
XtGeometryRequest  req;
WindowBox	*reqBox;
WindowBox	*replBox;	/* RETURN */
{
    WidgetData	data;

    if (XFindContext(dpy, w, widgetContext, (caddr_t *)&data) == XCNOENT)
	return (XtgeometryYes);
    /* requests: move, resize, top, bottom */
    switch (req) {
    case XtgeometryTop    : return (XtgeometryYes);
    case XtgeometryBottom : return (XtgeometryYes);
    case XtgeometryMove   : return (XtgeometryNo);
    case XtgeometryResize :
        return (ResizeEntryRequest(data, w, reqBox, replBox));
    }
    return (XtgeometryNo);
}

static XtStatus AddEntry(data, w, index)
WidgetData data;
Window w, index;
{
    int i;
    Position x, y;
    Dimension width, height, borderWidth;
    unsigned depth;
    Drawable root;
    WidgetEntry newentry;
    WidgetEntry item;

    /*
     * Return an error if one of the following conditions is true:
     * 1. The specified window doesn't exist
     * 2. The specified window is already an entry in this widget
     * 3. An index was specified, even though no entries exist
     */
    (void) XGetGeometry(data->dpy, w, &root, &x, &y, &width, &height,
			&borderWidth, &depth);
    if (FindEntry(data, w) != NULL) return (0);
    if ((data->numentries == 0) && index) return(0);

    newentry = (WidgetEntry) XtCalloc(1, sizeof(WidgetEntryRec));
    newentry->lug.w = w;
    newentry->lug.wb.x = x;
    newentry->lug.wb.y = y;
    newentry->lug.wb.width = width;
    newentry->lug.wb.height = height;
    newentry->lug.wb.borderWidth = borderWidth;

    if (data->numentries == 0)
	data->entries = newentry;
    else {
        if (FindEntry(data, index)) {
            for (all_items) {
                if (item_w == index) {
                    newentry->next = item;
                    if (item->prev == NULL)
                        data->entries = item->prev = newentry;
                    else {
                        (item->prev)->next = newentry;
                        item->prev = newentry;
                    }
                    break;
                }
            }
        } else {
            for (all_items) /* NULL */ ;
            for (i = 0, item = data->entries; i < data->numentries - 1;
                 i++, item = item->next) /* NULL */;
            item->next = newentry;
            newentry->prev = item;
        }

    }

    data->numentries++;
    (void) XSaveContext(data->dpy, newentry->lug.w, widgetContext, (caddr_t)data);
    (void) XtSetGeometryHandler(data->dpy, newentry->lug.w, EntryGeometryManager);

    return(1);
}

static void BatchAddEntries(data, args, argCount)
WidgetData data;
ArgList args;
int argCount;
{
    if (argCount) {
	for ( ; --argCount >= 0; args++) {
            if (strcmp(args->name , XtNmenuEntry)==0) {
		(void) AddEntry(data, (*((XtMenuEntry *)args->value)).w,
                                (*((XtMenuEntry *)args->value)).index);
	    }
	}
/*	(void) TryNewLayout(data); */
	Layout(data);
    }
}


/****************************************************************
 *
 * Public Routines
 *
 ****************************************************************/

Window XtMenuCreate(dpy, parent, args, argCount)
    Display  *dpy;
    Window   parent;
    ArgList  args;
    int      argCount;
{
    WidgetData	data;
    XrmNameList	names;
    XrmClassList classes;
    XSetWindowAttributes wvals;
    int valuemask;

    if (!initialized) MenuInitialize(dpy);

    data = (WidgetData) XtMalloc(sizeof(WidgetDataRec));

    globaldata = globalinit;
    globaldata.dpy = dpy;
    XtGetResources(dpy, resources, XtNumber(resources), args, argCount, parent,
	"menu", "Menu", &names, &classes);
    *data = globaldata;
    if (data->width == 0)
        data->width = ((data->hpad != 0) ? data->hpad : 10);
    if (data->height == 0)
	data->height = ((data->vpad != 0) ? data->vpad : 10);

    if (data->w != NULL) {
	XWindowChanges wc;
	unsigned int valuemask;
	valuemask = CWX | CWY | CWWidth | CWHeight | CWBorderWidth;
	wc.x = data->x; wc.y = data->y; wc.width = data->width;
	wc.height = data->height; wc.border_width = data->borderWidth;
	XConfigureWindow(data->dpy,data->w,valuemask, &wc);
	XReparentWindow(data->dpy,data->w,parent,data->x,data->y);
     }
    if (data->w == NULL) {   
     wvals.background_pixel = data->bg;
     wvals.border_pixel = data->bp;
     wvals.bit_gravity = CenterGravity;
    
     valuemask = CWBackPixel | CWBorderPixel | CWBitGravity;
	data->w = XCreateWindow(data->dpy, parent, data->x, data->y,
			 data->width, data->height, data->borderWidth,
			 0, InputOutput, (Visual *)CopyFromParent,
			 valuemask, &wvals);

    }

    XtSetNameAndClass(data->dpy, data->w, names, classes);
    XrmFreeNameList(names);
    XrmFreeClassList(classes);

    /* set handler for message and destroy events */

    XtSetEventHandler(data->dpy, data->w, (XtEventHandler)EventHandler,
                         StructureNotifyMask, (caddr_t) data);

    /* set handler for notify events, if any */
    if (data->notify)
        XtSetEventHandler(data->dpy, data->w, (XtEventHandler)Notify, data->notify,
                            (caddr_t) data);

    (void) XSaveContext(data->dpy, data->w, widgetContext, (caddr_t)data);
    (void) XtSetGeometryHandler(data->dpy, data->w, (XtGeometryHandler) XtMenuGeometryManager);

    /*
     * Batch add initial entries
     */
    BatchAddEntries(data, args, argCount);

    return (data->w);
}

XtStatus XtMenuAddEntry(dpy, parent, args, argCount)
    Display  *dpy;
    Window   parent;
    ArgList  args;
    int      argCount;
{
    WidgetData	data;

    if (XFindContext(dpy, parent, widgetContext, (caddr_t *)&data) == XCNOENT)
        return(0);

    /*
     * Batch add all entries
     */
    BatchAddEntries(data, args, argCount);

    return (1);
}

XtStatus XtMenuDeleteEntry(dpy, parent, args, argCount)
    Display  *dpy;
    Window   parent;
    ArgList  args;
    int      argCount;
{
    WidgetData	data;
    int		i;
    WidgetEntry item;

    if (XFindContext(dpy, parent, widgetContext, (caddr_t *)&data) == XCNOENT)
        return(0);

    parms = parminit;
    XtSetValues(parmResources, XtNumber(parmResources), args, argCount);
    if ((parms.entry == NULL) && (parms.i == -1)) return (0);

    if (parms.i >= 0) i = parms.i;
    else {
        for (all_items)
            if (item_w == parent) break;
    }

    if (i >= data->numentries) return (0);
    if (data->numentries == 1) {
        data->entries = NULL;
    } else if (i == data->numentries - 1) {
        (item->prev)->next = NULL;
    } else {
        (item->prev)->next = item->next;
        (item->next)->prev = item->prev;
    }

    (void) XDeleteContext(dpy, item_w, widgetContext);
    (void) XtClearGeometryHandler(dpy, item_w);
    data->numentries--;
    XtFree((char *) item);
    Layout(data);
    return (1);
}

/*
 * Menu Geometry Manager
 *
 * Note: The menu widget client typically should write its own.
 * However, this one simply says "yes" to any request to aid in
 * menu prototyping because many clients simply don't care if they're
 * using the menu as a pop-up.
 */
/*ARGSUSED*/
XtGeometryReturnCode XtMenuGeometryManager(dpy, window, req, reqBox, repBox)
    Display *dpy;
    Window window;
    XtGeometryRequest req;
    WindowBox *reqBox, *repBox;
{
    XResizeWindow(dpy, window, reqBox->width, reqBox->height);
    *repBox = *reqBox;
    return(XtgeometryYes);
}

/*
 *
 * Get Attributes
 *
 */

void XtMenuGetValues (dpy, window, args, argCount)
Display *dpy;
Window window;
ArgList args;
int argCount;
{
    WidgetData  data;
    data = DataFromWindow(dpy, window);
    if (data == NULL) return;
    globaldata = *data;
    XtGetValues(resources, XtNumber(resources), args, argCount);
}

/*
 *
 * Set Attributes
 *
 */

void XtMenuSetValues (dpy, window, args, argCount)
Display *dpy;
Window window;
ArgList args;
int argCount;
{
    WidgetData  data;
    data = DataFromWindow(dpy, window);
    if (data == NULL) return;
    globaldata = *data;
    XtSetValues(resources, XtNumber(resources), args, argCount);
    *data = globaldata;
}

