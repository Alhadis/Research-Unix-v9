/*-
 * sunIo.c --
 *	Functions to handle input from the keyboard and mouse.
 *
 * Copyright (c) 1987 by the Regents of the University of California
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies.  The University of California
 * makes no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 *
 *
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
static char sccsid[] = "%W %G Copyright 1987 Sun Micro";
#endif

#include    "sun.h"
#include    "opaque.h"

Bool	    	screenSaved = FALSE;
int	    	lastEventTime = 0;
extern int	sunSigIO;
extern void	SaveScreens();

#ifdef SUN_WINDOWS
int		windowFd = 0;
#define	INPBUFSIZE	128
#endif SUN_WINDOWS

/*-
 *-----------------------------------------------------------------------
 * TimeSinceLastInputEvent --
 *	Function used for screensaver purposes by the os module.
 *
 * Results:
 *	The time in milliseconds since there last was any
 *	input.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
int
TimeSinceLastInputEvent()
{
    struct timeval	now;

    gettimeofday (&now, (struct timezone *)0);

    if (lastEventTime == 0) {
	lastEventTime = TVTOMILLI(now);
    }
    return TVTOMILLI(now) - lastEventTime;
}

/*-
 *-----------------------------------------------------------------------
 * ProcessInputEvents --
 *	Retrieve all waiting input events and pass them to DIX in their
 *	correct chronological order. Only reads from the system pointer
 *	and keyboard.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Events are passed to the DIX layer.
 *
 *-----------------------------------------------------------------------
 */
void
ProcessInputEvents ()
{
    register Firm_event    *ptrEvents,    	/* Current pointer event */
			   *kbdEvents;	    	/* Current keyboard event */
    register int	    numPtrEvents, 	/* Number of remaining pointer
						 * events */
			    numKbdEvents;   	/* Number of remaining
						 * keyboard events */
    int	    	  	    nPE,    	    	/* Original number of pointer
						 * events */
			    nKE;    	    	/* Original number of
						 * keyboard events */
    DevicePtr		    pPointer;
    DevicePtr		    pKeyboard;
    register PtrPrivPtr     ptrPriv;
    register KbPrivPtr	    kbdPriv;
    Firm_event	  	    *lastEvent;	    	/* Last event processed */
    enum {
	NoneYet, Ptr, Kbd
    }			    lastType = NoneYet;	/* Type of last event */

#ifdef SUN_WINDOWS
    struct inputevent sunevents[INPBUFSIZE];
    register struct inputevent *se = sunevents, *seL;
    int         n;
    static int event_ignore = TRUE;
#endif SUN_WINDOWS

    /*
     *  Defensive programming - only reset sunIOPending (preventing
     *  further calls to ProcessInputEvents() until a future SIGIO)
     *  if we have actually received a SIGIO,  so we know it works.
     */
    if (sunSigIO) {
	isItTimeToYield = 0;
    }
    pPointer = LookupPointerDevice();
    pKeyboard = LookupKeyboardDevice();

    if ( sunUseSunWindows() ) {
#ifdef SUN_WINDOWS
	if ((n=read(windowFd,sunevents,INPBUFSIZE*sizeof sunevents[0])) < 0 
			    && errno != EWOULDBLOCK) {
	    /*
	     * Error reading events; should do something. XXX
	     */
	    return;
	}

	if (autoRepeatKeyDown && autoRepeatReady && n <= 0) {
		/* fake a sunwindows kbd event */
		n = sizeof(struct inputevent);
		se->ie_code = AUTOREPEAT_EVENTID;
		if (autoRepeatDebug)
		    ErrorF("ProcessInputEvents: sw auto event\n");
	}

	for (seL = sunevents + (n/(sizeof sunevents[0]));  se < seL; se++) {
	    if (screenSaved)
		SaveScreens(SCREEN_SAVER_FORCER, ScreenSaverReset);
	    lastEventTime = TVTOMILLI(event_time(se));

	    /*
	     * Decide whether or not to pay attention to events.
	     * Ignore the events if the locator has exited X Display.
	     */
	    switch (event_id(se)) {
		case KBD_DONE:
		    sunChangeKbdTranslation( pKeyboard, FALSE );
		    break;
		case KBD_USE:
		    sunChangeKbdTranslation( pKeyboard, TRUE );
		    break;
		case LOC_WINENTER:
		    event_ignore = FALSE;
		    break;
		case LOC_WINEXIT:
		    event_ignore = TRUE;
		    break;
	    }

	    if (event_ignore) {
		continue;
	    }

	    /*
	     * Figure out the X device this event should be reported on.
	     */
	    switch (event_id(se)) {
		case LOC_MOVE:
		case MS_LEFT:
		case MS_MIDDLE:
		case MS_RIGHT:
		    sunMouseProcessEventSunWin(pPointer,se);
		    break;
		case LOC_WINEXIT:
		case LOC_WINENTER:
		case KBD_DONE:
		case KBD_USE:
		    break;
		default:
		    sunKbdProcessEventSunWin(pKeyboard,se);
		    break;
	    }
	}
#endif SUN_WINDOWS
    } 
    else {
	ptrPriv = (PtrPrivPtr)pPointer->devicePrivate;
	kbdPriv = (KbPrivPtr)pKeyboard->devicePrivate;
	
	/*
	 * Get events from both the pointer and the keyboard, storing the number
	 * of events gotten in nPE and nKE and keeping the start of both arrays
	 * in pE and kE
	 */
	ptrEvents = (* ptrPriv->GetEvents) (pPointer, &nPE);
	kbdEvents = (* kbdPriv->GetEvents) (pKeyboard, &nKE);
	
	numPtrEvents = nPE;
	numKbdEvents = nKE;
	lastEvent = (Firm_event *)0;

	/*
	 * So long as one event from either device remains unprocess, we loop:
	 * Take the oldest remaining event and pass it to the proper module
	 * for processing. The DDXEvent will be sent to ProcessInput by the
	 * function called.
	 */
	while (numPtrEvents || numKbdEvents) {
	    if (numPtrEvents && numKbdEvents) {
		if (timercmp (&kbdEvents->time, &ptrEvents->time, <)) {
		    if (lastType == Ptr) {
			(* ptrPriv->DoneEvents) (pPointer, FALSE);
		    }
		    (* kbdPriv->ProcessEvent) (pKeyboard, kbdEvents);
		    numKbdEvents--;
		    lastEvent = kbdEvents++;
		    lastType = Kbd;
		} else {
		    if (lastType == Kbd) {
			(* kbdPriv->DoneEvents) (pKeyboard, FALSE);
		    }
		    (* ptrPriv->ProcessEvent) (pPointer, ptrEvents);
		    numPtrEvents--;
		    lastEvent = ptrEvents++;
		    lastType = Ptr;
		}
	    } else if (numKbdEvents) {
		if (lastType == Ptr) {
		    (* ptrPriv->DoneEvents) (pPointer, FALSE);
		}
		(* kbdPriv->ProcessEvent) (pKeyboard, kbdEvents);
		numKbdEvents--;
		lastEvent = kbdEvents++;
		lastType = Kbd;
	    } else {
		if (lastType == Kbd) {
		    (* kbdPriv->DoneEvents) (pKeyboard, FALSE);
		}
		(* ptrPriv->ProcessEvent) (pPointer, ptrEvents);
		numPtrEvents--;
		lastEvent = ptrEvents++;
		lastType = Ptr;
	    }
	}

	if (lastEvent) {
	    lastEventTime = TVTOMILLI(lastEvent->time);
	    if (screenSaved) {
		SaveScreens(SCREEN_SAVER_FORCER, ScreenSaverReset);
	    }
	}
	
	(* kbdPriv->DoneEvents) (pKeyboard);
	(* ptrPriv->DoneEvents) (pPointer);

    }

    sunRestoreCursor();

}


/*-
 *-----------------------------------------------------------------------
 * SetTimeSinceLastInputEvent --
 *	Set the lastEventTime to now.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	lastEventTime is altered.
 *
 *-----------------------------------------------------------------------
 */
void
SetTimeSinceLastInputEvent()
{
    struct timeval now;

    gettimeofday (&now, (struct timezone *)0);
    lastEventTime = TVTOMILLI(now);
}
