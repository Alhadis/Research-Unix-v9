#ifndef lint
static char *sccsid = "@(#)Event.c	1.7	2/25/87";
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

#include "Xlib.h"
#include "Intrinsic.h"
#include "Xutil.h"

/* Private Definitions */

typedef struct _ProcRec {
    XtEventHandler	     proc;
    unsigned long    mask;
    caddr_t 	     data;
    struct _ProcRec  *next;
} ProcRec, *ProcPtr;

typedef struct {
    Display	   *dpy;
    Window 	   window;
    ProcPtr 	   firstproc;
    unsigned long  totalmask;
    Boolean	    master;	/* True if this window is a child of root. */
} EventRec, *EventPtr;

static XContext eventContext;
static Boolean initialized = FALSE;

/* Private Routines */

void EventInitialize()
{
    if (initialized)
    	return;
    initialized = TRUE;

    eventContext = XUniqueContext();
}

static EventPtr EventPtrFromWindow(dpy, w)
Display	*dpy;
Window w;
{
    EventPtr result;
    if (XFindContext(dpy, w, eventContext, (caddr_t *)&result) != XCNOENT)
        return(result);
    return(0);
}

/*
 * Delete the event handler reference in the dispatcher for this window.
 */
static void ResetEventHandler(dpy, w)
Display *dpy;
Window w;
{
    register EventPtr ctx;
    register ProcPtr p, p2;

    ctx = EventPtrFromWindow(dpy, w);
    if (!ctx)
	  return;
    p = ctx->firstproc;
    while (p != NULL) {
	p2 = p->next;
	XtFree((char *) p);
	p = p2;
    }
    XtFree((char *)ctx);

    (void) XDeleteContext(dpy, w, eventContext);
    return;
}

static XtEventReturnCode CallProc(ctx, event)
    EventPtr ctx;
    XEvent *event;
{
    register EventPtr evp;
    EventPtr first;
    register ProcPtr p;
    register int nprocs, i;
    XtEventHandler procs[100];	/* We need to copy the procs into this */
				/* local storage before calling any of */
				/* them, because one of the calls */
				/* could cause the ctx record to be */
				/* destroyed! */
    caddr_t data[100];
    unsigned long kinds;
    XtEventReturnCode	result;
    XtEventReturnCode	finalresult = XteventNoHandler;

    static unsigned long masks[] = {
	0,			/* No such event. */
	StructureNotifyMask,	/* MessageEvent (Faked by toolkit). */
	KeyPressMask,		/* KeyPress */
	KeyReleaseMask,		/* KeyRelease */
	ButtonPressMask,	/* ButtonPress */
	ButtonReleaseMask,	/* ButtonRelease */
	PointerMotionMask | PointerMotionHintMask | Button1MotionMask |
	    Button2MotionMask | Button3MotionMask | Button4MotionMask |
		Button5MotionMask | ButtonMotionMask,
				/* MotionNotify */
	EnterWindowMask,	/* EnterNotify */
	LeaveWindowMask,	/* LeaveNotify */
	FocusChangeMask,	/* FocusIn */
	FocusChangeMask,	/* FocusOut */
	KeymapStateMask,	/* KeymapNotify */
	ExposureMask,		/* Expose */
	ExposureMask,		/* GraphicsExpose */
	ExposureMask,		/* NoExpose */
	VisibilityChangeMask,	/* VisibilityNotify */
	0,			/* CreateNotify */
	StructureNotifyMask,	/* DestroyNotify */
	StructureNotifyMask,	/* UnmapNotify */
	StructureNotifyMask,	/* MapNotify */
	0,			/* MapRequest */
	StructureNotifyMask,	/* ReparentNotify */
	StructureNotifyMask,	/* ConfigureNotify */
	0,			/* ConfigureRequest */
	StructureNotifyMask,	/* GravityNotify */
	0,			/* ResizeRequest */
	StructureNotifyMask,	/* CirculateNotify */
	0,			/* CirculateRequest */
	PropertyChangeMask,	/* PropertyNotify */
	StructureNotifyMask,	/* SelectionClear */
	StructureNotifyMask,	/* SelectionRequest */
	StructureNotifyMask,	/* SelectionNotify */
	ColormapChangeMask,	/* ColormapNotify */
	StructureNotifyMask,	/* ClientMessage */
	StructureNotifyMask	/* MappingNotify */
    };
	
    if (event->type >= LASTEvent) return(finalresult);

    kinds = masks[event->type];

    nprocs = 0;
    if (XFindContext(event->xany.display, (Window) 0,
		      eventContext, (caddr_t *)&first))
	first = ctx;
    for (evp = first; evp != NULL; evp = (evp == ctx) ? NULL : ctx) {
	for (p = evp->firstproc; p != NULL; p = p->next) {
	    if (kinds & p->mask) {
		procs[nprocs] = p->proc;
		data[nprocs++] = p->data;
	    }
	}
    }
    for (i=0 ; i<nprocs ; i++) {
	result = (*(procs[i]))(event, data[i]);
	if (finalresult != XteventHandled)
	    finalresult = result;
    }
    return(finalresult);
}


/* Public routines */

/*
 * Dispatch an event to the appropriate subwindow.
 * Return a value indicating how the event was handled.
 */
XtEventReturnCode XtDispatchEvent(event)
XEvent *event;
{
    EventPtr ctx;
 
    if (XFindContext(event->xany.display, event->xany.window,
		      eventContext, (caddr_t *)&ctx) != 0)
        return XteventNoHandler;
    else
	return CallProc(ctx, event);
}

/*
 * Register an event handler "proc" for window "w" with the
 * event dispatcher for "eventMask" events.
 * Note that re-registering the same window and handler with
 * a new event mask OVERWRITES the previous eventMask for that
 * window/handler combination.
 */
void XtSetEventHandler(dpy, w, proc, eventMask, data)
    Display	    *dpy;
    Window 	    w;
    XtEventHandler  proc;
    unsigned long   eventMask;
    caddr_t 	    data;
{
    register EventPtr ctx;
    register ProcPtr p, p2;
    register unsigned long totalmask;

    ctx = EventPtrFromWindow(dpy, w);
    if (ctx == NULL) {
        ctx = (EventPtr)XtMalloc(sizeof(EventRec));
	ctx->dpy = dpy;
        ctx->window = w;
        (void) XSaveContext(dpy, w, eventContext, (caddr_t)ctx);
        ctx->firstproc = NULL;
        ctx->totalmask = 0;
	ctx->master = FALSE;
    }
    totalmask = 0;
    p2 = NULL;
    p = ctx->firstproc;
    while (p != NULL && p->proc != proc) {
        totalmask |= p->mask;
        p2 = p;
        p = p->next;
    }
    if (p == NULL) {
        if (eventMask == 0)
            return;
        p = (ProcPtr)XtMalloc(sizeof(ProcRec));
        p->next = ctx->firstproc;
        ctx->firstproc = p;
        p->proc = proc;
    }
    if (eventMask) {
        p->mask = eventMask;
        p->data = data;
    }
    else {
        if (p2)
            p2->next = p->next;
        else
            ctx->firstproc = p->next;
        p2 = p;
        p = p->next;
        XtFree((char *)p2);
    }
    totalmask |= eventMask;
    for (; p != NULL; p = p->next)
        totalmask |= p->mask;
    if (totalmask != ctx->totalmask) {
        ctx->totalmask = totalmask;
        if (ctx->window) {
	    if (!ctx->master) totalmask &= ~StructureNotifyMask;
            XSelectInput(dpy, w, totalmask);
	}
    }
}

void XtDeleteEventHandler(dpy, w, proc)
    Display	   *dpy;
    Window         w;
    XtEventHandler proc;
{
    XtSetEventHandler(dpy, w, (XtEventHandler) proc, (unsigned long) 0, (caddr_t) NULL);
}

/* 
 * Make the given window be treated as a "master" window; that is, one that
 * is not a child of some other widget.
 */

void XtMakeMaster(dpy, w)
    Display *dpy;
    Window w;
{
    EventPtr ctx;
    XWMHints *hints;

    ctx = EventPtrFromWindow(dpy, w);
    if (ctx == NULL) {
        ctx = (EventPtr)XtMalloc(sizeof(EventRec));
        ctx->window = w;
        (void) XSaveContext(dpy, w, eventContext, (caddr_t)ctx);
        ctx->firstproc = NULL;
        ctx->totalmask = 0;
    } else XSelectInput(dpy, w, ctx->totalmask);
    ctx->master = TRUE;
    /* ||| Maybe the following stuff doesn't belong here.  It's just trying
       to tell the window manager that we handle our own focus.  -TW %%% */
    hints = XGetWMHints(dpy, w);
    if (hints == NULL) {
	hints = (XWMHints *) XtMalloc(sizeof(XWMHints));
	hints->flags = 0;
    }
    hints->flags |= InputHint;
    hints->input = FALSE;
    XSetWMHints(dpy, w, hints);
    XtFree(hints);
}

/*
 * Arrange to have the given procedure called whenever ANY event happens
 * that matches the given mask.
 */
void XtSetGlobalEventHandler(dpy, proc, eventMask, data)
    Display	    *dpy;
    XtEventHandler  proc;
    unsigned long   eventMask;
    caddr_t	    data;
{
    XtSetEventHandler(dpy, (Window) 0, (XtEventHandler) proc, eventMask, data);
}

void XtClearEventHandlers(dpy, window)
    Display *dpy;
    Window window;
{
    ResetEventHandler(dpy, window);
    (void) XtClearGeometryHandler(dpy, window);
}
