/*-
 * sunMouse.c --
 *	Functions for playing cat and mouse... sorry.
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

#define NEED_EVENTS
#include    "sun.h"

typedef struct {
    int	    bmask;	    /* Current button state */
    Bool    mouseMoved;	    /* Mouse has moved */
} SunMsPrivRec, *SunMsPrivPtr;

static void 	  	sunMouseCtrl();
static int 	  	sunMouseGetMotionEvents();
static Firm_event 	*sunMouseGetEvents();
static void 	  	sunMouseProcessEvent();
static void 	  	sunMouseDoneEvents();

static SunMsPrivRec	sunMousePriv;
static PtrPrivRec 	sysMousePriv = {
    -1,				/* Descriptor to device */
    sunMouseGetEvents,		/* Function to read events */
    sunMouseProcessEvent,	/* Function to process an event */
    sunMouseDoneEvents,		/* When all the events have been */
				/* handled, this function will be */
				/* called. */
    0,				/* Current X coordinate of pointer */
    0,				/* Current Y coordinate */
    NULL,			/* Screen pointer is on */
    (pointer)&sunMousePriv,	/* Field private to device */
};

/*-
 *-----------------------------------------------------------------------
 * sunMouseProc --
 *	Handle the initialization, etc. of a mouse
 *
 * Results:
 *	none.
 *
 * Side Effects:
 *
 * Note:
 *	When using sunwindows, all input comes off a single fd, stored in the
 *	global windowFd.  Therefore, only one device should be enabled and
 *	disabled, even though the application still sees both mouse and
 *	keyboard.  We have arbitrarily chosen to enable and disable windowFd
 *	in the keyboard routine sunKbdProc rather than in sunMouseProc.
 *
 *-----------------------------------------------------------------------
 */
int
sunMouseProc (pMouse, what)
    DevicePtr	  pMouse;   	/* Mouse to play with */
    int	    	  what;	    	/* What to do with it */
{
    register int  fd;
    int	    	  format;
    static int	  oformat;
    BYTE    	  map[4];

    switch (what) {
	case DEVICE_INIT:
	    if (pMouse != LookupPointerDevice()) {
		ErrorF ("Cannot open non-system mouse");	
		return (!Success);
	    }

	    if (! sunUseSunWindows()) {
		if (sysMousePriv.fd >= 0) {
		    fd = sysMousePriv.fd;
		} else {
		    fd = open ("/dev/mouse", O_RDWR, 0);
		    if (fd < 0) {
			Error ("Opening /dev/mouse");
			return (!Success);
		    }
		    if (fcntl (fd, F_SETFL, (FNDELAY|FASYNC)) < 0
			|| fcntl(fd, F_SETOWN, getpid()) < 0) {
			    perror("sunMouseProc");
			    ErrorF("Can't set up mouse on fd %d\n", fd);
			}
		    
		    sysMousePriv.fd = fd;
		}
	    }

	    sysMousePriv.pScreen = &screenInfo.screen[0];
	    sysMousePriv.x = sysMousePriv.pScreen->width / 2;
	    sysMousePriv.y = sysMousePriv.pScreen->height / 2;

	    sunMousePriv.bmask = 0;
	    sunMousePriv.mouseMoved = FALSE;

	    pMouse->devicePrivate = (pointer) &sysMousePriv;
	    pMouse->on = FALSE;
	    map[1] = 1;
	    map[2] = 2;
	    map[3] = 3;
	    InitPointerDeviceStruct(
		pMouse, map, 3, sunMouseGetMotionEvents, sunMouseCtrl);
	    break;

	case DEVICE_ON:
	    if (! sunUseSunWindows()) {
		if (ioctl (((PtrPrivPtr)pMouse->devicePrivate)->fd,
			VUIDGFORMAT, &oformat) < 0) {
		    Error ("VUIDGFORMAT");
		    return(!Success);
		}
		format = VUID_FIRM_EVENT;
		if (ioctl (((PtrPrivPtr)pMouse->devicePrivate)->fd,
			VUIDSFORMAT, &format) < 0) {
		    Error ("VUIDSFORMAT");
		    return(!Success);
		}
		AddEnabledDevice (((PtrPrivPtr)pMouse->devicePrivate)->fd);
	    }

	    pMouse->on = TRUE;
	    break;

	case DEVICE_CLOSE:
	    if (! sunUseSunWindows()) {
		if (ioctl (((PtrPrivPtr)pMouse->devicePrivate)->fd,
			VUIDSFORMAT, &oformat) < 0) {
		    Error ("VUIDSFORMAT");
		}
	    }
	    break;

	case DEVICE_OFF:
	    pMouse->on = FALSE;
	    if (! sunUseSunWindows()) {
		RemoveEnabledDevice (((PtrPrivPtr)pMouse->devicePrivate)->fd);
	    }
	    break;
    }
    return (Success);
}
	    
/*-
 *-----------------------------------------------------------------------
 * sunMouseCtrl --
 *	Alter the control parameters for the mouse. Since acceleration
 *	etc. is done from the PtrCtrl record in the mouse's device record,
 *	there's nothing to do here.
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static void
sunMouseCtrl (pMouse)
    DevicePtr	  pMouse;
{
}

/*-
 *-----------------------------------------------------------------------
 * sunMouseGetMotionEvents --
 *	Return the (number of) motion events in the "motion history
 *	buffer" (snicker) between the given times.
 *
 * Results:
 *	The number of events stuffed.
 *
 * Side Effects:
 *	The relevant xTimecoord's are stuffed in the passed memory.
 *
 *-----------------------------------------------------------------------
 */
static int
sunMouseGetMotionEvents (buff, start, stop)
    CARD32 start, stop;
    xTimecoord *buff;
{
    return 0;
}

/*-
 *-----------------------------------------------------------------------
 * sunMouseGetEvents --
 *	Return the events waiting in the wings for the given mouse.
 *
 * Results:
 *	A pointer to an array of Firm_events or (Firm_event *)0 if no events
 *	The number of events contained in the array.
 *
 * Side Effects:
 *	None.
 *-----------------------------------------------------------------------
 */
static Firm_event *
sunMouseGetEvents (pMouse, pNumEvents)
    DevicePtr	  pMouse;	    /* Mouse to read */
    int	    	  *pNumEvents;	    /* Place to return number of events */
{
    int	    	  nBytes;	    /* number of bytes of events available. */
    register PtrPrivPtr	  pPriv;
    static Firm_event	evBuf[MAXEVENTS];   /* Buffer for Firm_events */

    pPriv = (PtrPrivPtr) pMouse->devicePrivate;

    nBytes = read (pPriv->fd, evBuf, sizeof(evBuf));

    if (nBytes < 0) {
	if (errno == EWOULDBLOCK) {
	    *pNumEvents = 0;
	} else {
	    Error ("Reading mouse");
	    FatalError ("Could not read from mouse");
	}
    } else {
	*pNumEvents = nBytes / sizeof (Firm_event);
    }
    return (evBuf);
}


/*-
 *-----------------------------------------------------------------------
 * MouseAccelerate --
 *	Given a delta and a mouse, return the acceleration of the delta.
 *
 * Results:
 *	The corrected delta
 *
 * Side Effects:
 *	None.
 *
 *-----------------------------------------------------------------------
 */
static short
MouseAccelerate (pMouse, delta)
    DevicePtr	  pMouse;
    int	    	  delta;
{
    register int  sgn = sign(delta);
    register PtrCtrl *pCtrl;

    delta = abs(delta);
    pCtrl = &((DeviceIntPtr) pMouse)->u.ptr.ctrl;

    if (delta > pCtrl->threshold) {
	return ((short) (sgn * (pCtrl->threshold +
				((delta - pCtrl->threshold) * pCtrl->num) /
				pCtrl->den)));
    } else {
	return ((short) (sgn * delta));
    }
}

/*-
 *-----------------------------------------------------------------------
 * sunMouseProcessEvent --
 *	Given a Firm_event for a mouse, pass it off the the dix layer
 *	properly converted...
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	The cursor may be redrawn...? devPrivate/x/y will be altered.
 *
 *-----------------------------------------------------------------------
 */
static void
sunMouseProcessEvent (pMouse, fe)
    DevicePtr	  pMouse;   	/* Mouse from which the event came */
    Firm_event	  *fe;	    	/* Event to process */
{
    int      index;		/* screen index */
    xEvent		xE;
    register PtrPrivPtr	pPriv;	/* Private data for pointer */
    register SunMsPrivPtr pSunPriv; /* Private data for mouse */
    register int  	bmask;	/* Temporary button mask */

    pPriv = (PtrPrivPtr)pMouse->devicePrivate;
    pSunPriv = (SunMsPrivPtr) pPriv->devPrivate;

    xE.u.keyButtonPointer.time = TVTOMILLI(fe->time);

    switch (fe->id) {
	case MS_LEFT:
	case MS_MIDDLE:
	case MS_RIGHT:
	    /*
	     * A button changed state. Sometimes we will get two events
	     * for a single state change. Should we get a button event which
	     * reflects the current state of affairs, that event is discarded.
	     *
	     * Mouse buttons start at 1.
	     */
	    xE.u.u.detail = (fe->id - MS_LEFT) + 1;
	    bmask = 1 << xE.u.u.detail;
	    if (fe->value == VKEY_UP) {
		if (pSunPriv->bmask & bmask) {
		    xE.u.u.type = ButtonRelease;
		    pSunPriv->bmask &= ~bmask;
		} else {
		    return;
		}
	    } else {
		if ((pSunPriv->bmask & bmask) == 0) {
		    xE.u.u.type = ButtonPress;
		    pSunPriv->bmask |= bmask;
		} else {
		    return;
		}
	    }
	    /*
	     * If the mouse has moved, we must update any interested client
	     * as well as DIX before sending a button event along.
	     */
	    if (pSunPriv->mouseMoved) {
		sunMouseDoneEvents (pMouse, FALSE);
	    }
	
	    break;
	case LOC_X_DELTA:
	    /*
	     * When we detect a change in the mouse coordinates, we call
	     * the cursor module to move the cursor. It has the option of
	     * simply removing the cursor or just shifting it a bit.
	     * If it is removed, DIX will restore it before we goes to sleep...
	     *
	     * What should be done if it goes off the screen? Move to another
	     * screen? For now, we just force the pointer to stay on the
	     * screen...
	     */
	    pPriv->x += MouseAccelerate (pMouse, fe->value);

            /*
             * Active Zaphod implementation:
             *    increment or decrement the current screen
             *    if the x is to the right or the left of
             *    the current screen.
             */
            if (screenInfo.numScreens > 1 &&
                (pPriv->x > pPriv->pScreen->width ||
                 pPriv->x < 0)) {
                sunRemoveCursor();
                /* disable color plane if it's current */
                index = pPriv->pScreen->myNum;
                (*sunFbs[index].EnterLeave) (pPriv->pScreen, 1);
                if (pPriv->x < 0) { 
                     if (pPriv->pScreen->myNum != 0)
                        (pPriv->pScreen)--;
                     else
                         pPriv->pScreen = &screenInfo.screen[screenInfo.numScreens -1];
 
                     pPriv->x += pPriv->pScreen->width;
                }
                else {
                    pPriv->x -= pPriv->pScreen->width;

                    if (pPriv->pScreen->myNum != screenInfo.numScreens -1)
                        (pPriv->pScreen)++;
                    else
                         pPriv->pScreen = &screenInfo.screen[0];
                }

                index = pPriv->pScreen->myNum;
                /* enable color plane if new current screen */
                (*sunFbs[index].EnterLeave) (pPriv->pScreen, 0);
            }

	    if (!sunConstrainXY (&pPriv->x, &pPriv->y)) {
		return;
	    }

            NewCurrentScreen (pPriv->pScreen, pPriv->x, pPriv->y);

#ifdef	SUN_ALL_MOTION
	    xE.u.u.type = MotionNotify;
	    sunMoveCursor (pPriv->pScreen, pPriv->x, pPriv->y);
	    break;
#else
	    ((SunMsPrivPtr)pPriv->devPrivate)->mouseMoved = TRUE;
	    return;
#endif
	case LOC_Y_DELTA:
	    /*
	     * For some reason, motion up generates a positive y delta
	     * and motion down a negative delta, so we must subtract
	     * here instead of add...
	     */
	    pPriv->y -= MouseAccelerate (pMouse, fe->value);
	    if (!sunConstrainXY (&pPriv->x, &pPriv->y)) {
		return;
	    }
#ifdef SUN_ALL_MOTION
	    xE.u.u.type = MotionNotify;
	    sunMoveCursor (pPriv->pScreen, pPriv->x, pPriv->y);
	    break;
#else
	    ((SunMsPrivPtr)pPriv->devPrivate)->mouseMoved = TRUE;
	    return;
#endif SUN_ALL_MOTION
	default:
	    FatalError ("sunMouseProcessEvent: unrecognized id\n");
	    break;
    }

    xE.u.keyButtonPointer.rootX = pPriv->x;
    xE.u.keyButtonPointer.rootY = pPriv->y;

    (* pMouse->processInputProc) (&xE, pMouse);
}

/*-
 *-----------------------------------------------------------------------
 * sunMouseDoneEvents --
 *	Finish off any mouse motions we haven't done yet. (At the moment
 *	this code is unused since we never save mouse motions as I'm
 *	unsure of the effect of getting a keystroke at a given [x,y] w/o
 *	having gotten a motion event to that [x,y])
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	A MotionNotify event may be generated.
 *
 *-----------------------------------------------------------------------
 */
/*ARGSUSED*/
static void
sunMouseDoneEvents (pMouse,final)
    DevicePtr	  pMouse;
    Bool	  final;
{
    PtrPrivPtr	  pPriv;
    SunMsPrivPtr  pSunPriv;
    xEvent	  xE;

    pPriv = (PtrPrivPtr) pMouse->devicePrivate;
    pSunPriv = (SunMsPrivPtr) pPriv->devPrivate;

    if (pSunPriv->mouseMoved) {
	sunMoveCursor (pPriv->pScreen, pPriv->x, pPriv->y);
	xE.u.keyButtonPointer.rootX = pPriv->x;
	xE.u.keyButtonPointer.rootY = pPriv->y;
	xE.u.keyButtonPointer.time = lastEventTime;
	xE.u.u.type = MotionNotify;
	(* pMouse->processInputProc) (&xE, pMouse);
	pSunPriv->mouseMoved = FALSE;
    }
}

#ifdef SUN_WINDOWS

/*
 * Process a sunwindows mouse event.  The possible events are
 *   LOC_MOVE
 *   MS_LEFT
 *   MS_MIDDLE
 *   MS_RIGHT
 */

void
sunMouseProcessEventSunWin(pMouse,se)
    DeviceRec *pMouse;
    register struct inputevent *se;
{   
    xEvent			xE;
    register int	  	bmask;	/* Temporary button mask */
    register PtrPrivPtr		pPriv;	/* Private data for pointer */
    register SunMsPrivPtr	pSunPriv; /* Private data for mouse */

    pPriv = (PtrPrivPtr)pMouse->devicePrivate;
    pSunPriv = (SunMsPrivPtr) pPriv->devPrivate;

    xE.u.keyButtonPointer.time = TVTOMILLI(event_time(se));

    switch (event_id(se)) {
        case MS_LEFT:
        case MS_MIDDLE:
        case MS_RIGHT:
	    /*
	     * A button changed state. Sometimes we will get two events
	     * for a single state change. Should we get a button event which
	     * reflects the current state of affairs, that event is discarded.
	     *
	     * Mouse buttons start at 1.
	     */
	    xE.u.u.detail = (event_id(se) - MS_LEFT) + 1;
	    bmask = 1 << xE.u.u.detail;
	    if (win_inputnegevent(se)) {
		if (pSunPriv->bmask & bmask) {
		    xE.u.u.type = ButtonRelease;
		    pSunPriv->bmask &= ~bmask;
		} else {
		    return;
		}
	    } else {
		if ((pSunPriv->bmask & bmask) == 0) {
		    xE.u.u.type = ButtonPress;
		    pSunPriv->bmask |= bmask;
		} else {
		    return;
		}
	    }
    	    break;
        case LOC_MOVE:
	    xE.u.u.type = MotionNotify;
	    xE.u.u.detail = 0;
	    pPriv->x = event_x(se);
	    pPriv->y = event_y(se);
	    if (!sunConstrainXY (&pPriv->x, &pPriv->y)) {
		return;
	    }
	    sunMoveCursor (pPriv->pScreen, pPriv->x, pPriv->y);
	    if ((pPriv->x != event_x(se)) || (pPriv->y != event_y(se))) {
		/*
		 * We constrained the pointer motion.  Tell the pointer
		 * where it really needs to be.
		 */
		win_setmouseposition(windowFd, pPriv->x, pPriv->y);
	    }
	    break;
	default:
	    FatalError ("sunMouseProcessEventSunWin: unrecognized id\n");
	    break;
    }

    xE.u.keyButtonPointer.rootX = event_x(se);
    xE.u.keyButtonPointer.rootY = event_y(se);

    (* pMouse->processInputProc) (&xE, pMouse);

}
#endif SUN_WINDOWS
