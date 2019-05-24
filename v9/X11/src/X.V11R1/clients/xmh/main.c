#ifndef lint
static char rcs_id[] = "$Header: main.c,v 1.13 87/09/11 08:18:11 toddb Exp $";
#endif lint
/*
 *			  COPYRIGHT 1987
 *		   DIGITAL EQUIPMENT CORPORATION
 *		       MAYNARD, MASSACHUSETTS
 *			ALL RIGHTS RESERVED.
 *
 * THE INFORMATION IN THIS SOFTWARE IS SUBJECT TO CHANGE WITHOUT NOTICE AND
 * SHOULD NOT BE CONSTRUED AS A COMMITMENT BY DIGITAL EQUIPMENT CORPORATION.
 * DIGITAL MAKES NO REPRESENTATIONS ABOUT THE SUITABILITY OF THIS SOFTWARE FOR
 * ANY PURPOSE.  IT IS SUPPLIED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY.
 *
 * IF THE SOFTWARE IS MODIFIED IN A MANNER CREATING DERIVATIVE COPYRIGHT RIGHTS,
 * APPROPRIATE LEGENDS MAY BE PLACED ON THE DERIVATIVE WORK IN ADDITION TO THAT
 * SET FORTH ABOVE.
 *
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting documentation,
 * and that the name of Digital Equipment Corporation not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.
 */

/* main.c */

#define MAIN 1			/* Makes global.h actually declare vars */
#include "xmh.h"
#include <signal.h>

Boolean shouldcheckscans;	/* If it's time to check for rescanning */
Boolean emptying = FALSE;	/* Whether we're currently in EmptyEventQueue. */


/* Handle one event.  Will block if there are no events to handle. */

HandleEvent()
{
    XEvent event;
    int     i;
    static int lasttime, lastx, lasty;
    static Window lastwin;
    QXNextEvent(theDisplay, &event);
    if (shouldcheckscans && !emptying) {
	if (debug) {
	    (void) fprintf(stderr, "[magic toc check ...");
	    (void) fflush(stderr);
	}
	QXPutBackEvent(theDisplay, &event);
	shouldcheckscans = FALSE;
	for (i = 0; i < numScrns; i++) {
	    if (scrnList[i]->toc)
		TocRecheckValidity(scrnList[i]->toc);
	    if (scrnList[i]->msg)
		TocRecheckValidity(MsgGetToc(scrnList[i]->msg));
	}
	(void) alarm(5 * 60);
	if (debug) {(void)fprintf(stderr, "done]\n");(void)fflush(stderr);}
    }
    else {
	(void)XtDispatchEvent(&event);
#ifdef X11
	if (defDoubleClick) {
	    if (event.xany.type == ButtonPress && DoubleClickProc
		&& event.xbutton.window == lastwin
		&& event.xbutton.x == lastx && event.xbutton.y == lasty
		&& event.xbutton.time - lasttime <= 500) {
		(*DoubleClickProc)(DoubleClickParam);
		DoubleClickProc = NULL;
	    }
	    if (event.xany.type == ButtonRelease) {
		lasttime = event.xbutton.time;
		lastx = event.xbutton.x;
		lasty = event.xbutton.y;
		lastwin = event.xbutton.window;
	    } else DoubleClickProc = NULL;
	}
#endif X11
	HandleConfirmEvent(&event);
    }
}

#ifdef X10
EmptyEventQueue()
{
    XEvent event;
    emptying = TRUE;
    do {
	while (XPending()) {
	    XPeekEvent(&event);
	    if (event.type & (KeyPressed | KeyReleased | MouseMoved |
			      ButtonPressed | ButtonReleased))
		XNextEvent(&event);
	    else
		HandleEvent();
	}
	XSync(0);
    } while (XPending());
    emptying = FALSE;
}
#endif


/* This gets called by the alarm clock every five minutes. */

NeedToCheckScans()
{
    shouldcheckscans = TRUE;
}



#ifdef X11
CheckMail(event)
XEvent *event;
{
    static int count = 0;
    int i;
    if (event->type == ClientMessage &&
	    ((int)event->xclient.message_type) == XtTimerExpired) {
	if (defNewMailCheck) {
if (debug) {(void)fprintf(stderr, "(Checking for new mail..."); (void)fflush(stderr);}
	    TocCheckForNewMail();
if (debug) (void)fprintf(stderr, "done)\n");
	}
	if (defMakeCheckpoints && count++ % 5 == 0) {
if (debug) {(void)fprintf(stderr, "(Checkpointing..."); (void)fflush(stderr);}
	    for (i=0 ; i<numScrns ; i++)
		if (scrnList[i]->msg) 
		    MsgCheckPoint(scrnList[i]->msg);
if (debug) (void)fprintf(stderr, "done)\n");
	}
    }
}
#endif X11

/* Main loop. */

main(argc, argv)
int argc;
char **argv;
{
    InitializeWorld(argc, argv);
    shouldcheckscans = TRUE;
    (void) signal(SIGALRM, NeedToCheckScans);
#ifdef X11
    if (defNewMailCheck)
	TocCheckForNewMail();
    if (defNewMailCheck || defMakeCheckpoints) {
	XtSetEventHandler(theDisplay, DefaultRootWindow(theDisplay), CheckMail,
			  StructureNotifyMask, (caddr_t)NULL);
	XtSetTimeOut(DefaultRootWindow(theDisplay), (caddr_t)NULL, (int)60000);
    }
#endif X11
    for (;;) {
	HandleEvent();
    }
}
