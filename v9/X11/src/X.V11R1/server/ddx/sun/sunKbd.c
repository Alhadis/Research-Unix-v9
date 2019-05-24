/*-
 * sunKbd.c --
 *	Functions for retrieving data from a keyboard.
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
#include "sun.h"
#include <stdio.h>
#include "Xproto.h"
#include "keysym.h"

typedef struct {
    int	    	  trans;          	/* Original translation form */
} SunKbPrivRec, *SunKbPrivPtr;

extern CARD8 *sunModMap[];
extern KeySymsRec sunKeySyms[];

static void 	  sunBell();
static void 	  sunKbdCtrl();
static Firm_event *sunKbdGetEvents();
static void 	  sunKbdProcessEvent();
static void 	  sunKbdDoneEvents();
int	  	  autoRepeatKeyDown = 0;
int	  	  autoRepeatDebug = 0;
int	  	  autoRepeatReady;
static int	  autoRepeatFirst;
static struct timeval autoRepeatLastKeyDownTv;
static struct timeval autoRepeatDeltaTv;
#define	tvminus(tv, tv1, tv2) 	/* tv = tv1 - tv2 */ \
		if ((tv1).tv_usec < (tv2).tv_usec) { \
		    (tv1).tv_usec += 1000000; \
		    (tv1).tv_sec -= 1; \
		} \
		(tv).tv_usec = (tv1).tv_usec - (tv2).tv_usec; \
		(tv).tv_sec = (tv1).tv_sec - (tv2).tv_sec;
#define tvplus(tv, tv1, tv2) 	/* tv = tv1 + tv2 */ \
		(tv).tv_sec = (tv1).tv_sec + (tv2).tv_sec; \
		(tv).tv_usec = (tv1).tv_usec + (tv2).tv_usec; \
		if ((tv).tv_usec > 1000000) { \
			(tv).tv_usec -= 1000000; \
			(tv).tv_sec += 1; \
		}


static SunKbPrivRec	sunKbPriv;  
static KbPrivRec  	sysKbPriv = {
    -1,				/* Type of keyboard */
    -1,				/* Descriptor open to device */
    sunKbdGetEvents,		/* Function to read events */
    sunKbdProcessEvent,		/* Function to process an event */
    sunKbdDoneEvents,		/* Function called when all events */
				/* have been handled. */
    (pointer)&sunKbPriv,	/* Private to keyboard device */
    (Bool)0,			/* Mapped queue */
    0,				/* offset for device keycodes */
};

/*-
 *-----------------------------------------------------------------------
 * sunKbdProc --
 *	Handle the initialization, etc. of a keyboard.
 *
 * Results:
 *	None.
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
sunKbdProc (pKeyboard, what)
    DevicePtr	  pKeyboard;	/* Keyboard to manipulate */
    int	    	  what;	    	/* What to do to it */
{
    KbPrivPtr	  pPriv;
    register int  kbdFd;

    switch (what) {
	case DEVICE_INIT:
	    if (pKeyboard != LookupKeyboardDevice()) {
		ErrorF ("Cannot open non-system keyboard");
		return (!Success);
	    }
	    
	    /*
	     * First open and find the current state of the keyboard.
	     */
	    if (sysKbPriv.fd >= 0) {
		kbdFd = sysKbPriv.fd;
	    } else {
		kbdFd = open ("/dev/kbd", O_RDWR, 0);
		if (kbdFd < 0) {
		    Error ("Opening /dev/kbd");
		    return (!Success);
		}
		sysKbPriv.fd = kbdFd;
		(void) ioctl (kbdFd, KIOCTYPE, &sysKbPriv.type);
		(void) ioctl (kbdFd, KIOCGTRANS, &sunKbPriv.trans);
		if (sysKbPriv.type < 0 || sysKbPriv.type > KB_SUN3
		    || sunKeySyms[sysKbPriv.type].map == NULL)
		    FatalError("Unsupported keyboard type %d\n", 
			sysKbPriv.type);
		if (sunUseSunWindows()) {
		    (void) close( kbdFd );
		    sysKbPriv.fd = -1;
		} else {
		    if (fcntl (kbdFd, F_SETFL, (FNDELAY|FASYNC)) < 0
			|| fcntl(kbdFd, F_SETOWN, getpid()) < 0) {
			perror("sunKbdProc");
			FatalError("Can't set up kbd on fd %d\n", kbdFd);
		    }
		}
	    }

	    /*
	     * Perform final initialization of the system private keyboard
	     * structure and fill in various slots in the device record
	     * itself which couldn't be filled in before.
	     */
	    pKeyboard->devicePrivate = (pointer)&sysKbPriv;

	    pKeyboard->on = FALSE;
	    /*
	     * ensure that the keycodes on the wire are >= MIN_KEYCODE
	     */
	    if (sunKeySyms[sysKbPriv.type].minKeyCode < MIN_KEYCODE) {
		int offset = MIN_KEYCODE -sunKeySyms[sysKbPriv.type].minKeyCode;

		sunKeySyms[sysKbPriv.type].minKeyCode += offset;
		sunKeySyms[sysKbPriv.type].maxKeyCode += offset;
		sysKbPriv.offset = offset;
	    }
	    InitKeyboardDeviceStruct(
		    pKeyboard,
		    &(sunKeySyms[sysKbPriv.type]),
		    (sunModMap[sysKbPriv.type]),
		    sunBell,
		    sunKbdCtrl);
	    break;

	case DEVICE_ON:
	    if (sunUseSunWindows()) {
#ifdef SUN_WINDOWS
		if (! sunSetUpKbdSunWin(windowFd, TRUE, pKeyboard)) {
		    FatalError("Can't set up keyboard\n");
		}
		AddEnabledDevice(windowFd);
#endif SUN_WINDOWS
	    }
	    else {
		pPriv = (KbPrivPtr)pKeyboard->devicePrivate;
		kbdFd = pPriv->fd;

	        /*
	         * Set the keyboard into "direct" mode and turn on
	         * event translation.
	         */
		if (sunChangeKbdTranslation(pKeyboard,TRUE) < 0) {
		    FatalError("Can't set keyboard translation\n");
		}

		AddEnabledDevice(kbdFd);
	    }
	    pKeyboard->on = TRUE;
	    break;

	case DEVICE_CLOSE:
	case DEVICE_OFF:
	    if (sunUseSunWindows()) {
#ifdef SUN_WINDOWS
		if (! sunSetUpKbdSunWin(windowFd, FALSE, pKeyboard)) {
		    FatalError("Can't close keyboard\n");
		}
		RemoveEnabledDevice(windowFd);
#endif SUN_WINDOWS
	    }
	    else {
		pPriv = (KbPrivPtr)pKeyboard->devicePrivate;
		kbdFd = pPriv->fd;
	    
	        /*
	         * Restore original keyboard directness and translation.
	         */
		if (sunChangeKbdTranslation(pKeyboard,FALSE) < 0) {
		    FatalError("Can't reset keyboard translation\n");
		}

		RemoveEnabledDevice(kbdFd);
	    }
	    pKeyboard->on = FALSE;
	    break;
    }
    return (Success);
}

/*-
 *-----------------------------------------------------------------------
 * sunBell --
 *	Ring the terminal/keyboard bell
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	None, really...
 *
 *-----------------------------------------------------------------------
 */
static void
sunBell (loudness, pKeyboard)
    int	    	  loudness;	    /* Percentage of full volume */
    DevicePtr	  pKeyboard;	    /* Keyboard to ring */
{
    /* no can do, for now */
}

/*-
 *-----------------------------------------------------------------------
 * sunKbdCtrl --
 *	Alter some of the keyboard control parameters
 *
 * Results:
 *	None.
 *
 * Side Effects:
 *	Some...
 *
 *-----------------------------------------------------------------------
 */
static void
sunKbdCtrl (pKeyboard)
    DevicePtr	  pKeyboard;	    /* Keyboard to alter */
{
    /* can only change key click on sun 3 keyboards, so what's the use? */
}

/*-
 *-----------------------------------------------------------------------
 * sunKbdGetEvents --
 *	Return the events waiting in the wings for the given keyboard.
 *
 * Results:
 *	A pointer to an array of Firm_events or (Firm_event *)0 if no events
 *	The number of events contained in the array.
 *	If there are no keyboard events ready and autoRepeatKeyDown > 0,
 *	then *pNumEvents is set to 1 and Firm_event id is set to
 *	AUTOREPEAT_EVENTID.  In sunKbdProcessEvent, if autoRepeatKeyDown > 0
 *	and Firm_event id == AUTOREPEAT_EVENTID, then the event buffer is
 *	ignored and the	event is generated from the last KeyPress event.
 *
 * Side Effects:
 *	None.
 *-----------------------------------------------------------------------
 */
static Firm_event *
sunKbdGetEvents (pKeyboard, pNumEvents)
    DevicePtr	  pKeyboard;	    /* Keyboard to read */
    int	    	  *pNumEvents;	    /* Place to return number of events */
{
    int	    	  nBytes;	    /* number of bytes of events available. */
    KbPrivPtr	  pPriv;
    static Firm_event	evBuf[MAXEVENTS];   /* Buffer for Firm_events */

    pPriv = (KbPrivPtr) pKeyboard->devicePrivate;
    nBytes = read (pPriv->fd, evBuf, sizeof(evBuf));

    if (nBytes < 0) {
	if (errno == EWOULDBLOCK) {
	    *pNumEvents = 0;
	} else {
	    Error ("Reading keyboard");
	    FatalError ("Could not read the keyboard");
	}
    } else {
	*pNumEvents = nBytes / sizeof (Firm_event);
    }

    if (autoRepeatKeyDown && autoRepeatReady && *pNumEvents == 0) {
	*pNumEvents = 1;			/* Fake the event */
	evBuf[0].id = AUTOREPEAT_EVENTID;	/* Flags autoRepeat event */
	if (autoRepeatDebug)
	    ErrorF("sunKbdGetEvents: autoRepeatKeyDown = %d event\n",
				autoRepeatKeyDown);
    }

    return (evBuf);
}

/*-
 *-----------------------------------------------------------------------
 * sunKbdProcessEvent --
 *
 * Results:
 *
 * Side Effects:
 *
 * Caveat:
 *      To reduce duplication of code and logic (and therefore bugs), the
 *      sunwindows version of kbd processing (sunKbdProcessEventSunWin())
 *      counterfeits a firm event and calls this routine.  This
 *      couunterfeiting relies on the fact this this routine only looks at the
 *      id, time, and value fields of the firm event which it is passed.  If
 *      this ever changes, the sunKbdProcessEventSunWin will also have to
 *      change.
 *
 *-----------------------------------------------------------------------
 */
static void
sunKbdProcessEvent (pKeyboard, fe)
    DevicePtr	  pKeyboard;
    Firm_event	  *fe;
{
    xEvent		xE;
    PtrPrivPtr	  	ptrPriv;
    int			delta;
    static xEvent	autoRepeatEvent;

    ptrPriv = (PtrPrivPtr) LookupPointerDevice()->devicePrivate;

    if (autoRepeatKeyDown && fe->id == AUTOREPEAT_EVENTID) {
	/*
	 * Generate auto repeat event.	XXX one for now.
	 * Update time & pointer location of saved KeyPress event.
	 */
	if (autoRepeatDebug)
	    ErrorF("sunKbdProcessEvent: autoRepeatKeyDown = %d\n",
			autoRepeatKeyDown);

	delta = TVTOMILLI(autoRepeatDeltaTv) / 2;
	if (autoRepeatFirst == TRUE)
		autoRepeatFirst = FALSE;

	/*
	 * Fake a key up event and a key down event
	 * for the last key pressed.
	 */
	autoRepeatEvent.u.keyButtonPointer.time += delta;
	autoRepeatEvent.u.keyButtonPointer.rootX = ptrPriv->x;
	autoRepeatEvent.u.keyButtonPointer.rootY = ptrPriv->y;
	autoRepeatEvent.u.u.type = KeyRelease;
	(* pKeyboard->processInputProc) (&autoRepeatEvent, pKeyboard);

	autoRepeatEvent.u.keyButtonPointer.time += delta;
	autoRepeatEvent.u.u.type = KeyPress;
	(* pKeyboard->processInputProc) (&autoRepeatEvent, pKeyboard);

	/* Update time of last key down */
	tvplus(autoRepeatLastKeyDownTv, autoRepeatLastKeyDownTv, 
			autoRepeatDeltaTv);

	return;
    }

    /*
     * Kill AutoRepeater on any real Kbd event.
     */
    autoRepeatKeyDown = 0;
    if (autoRepeatDebug)
	ErrorF("sunKbdProcessEvent: autoRepeat off\n");

    xE.u.keyButtonPointer.time = TVTOMILLI(fe->time);
    xE.u.keyButtonPointer.rootX = ptrPriv->x;
    xE.u.keyButtonPointer.rootY = ptrPriv->y;
    xE.u.u.type = ((fe->value == VKEY_UP) ? KeyRelease : KeyPress);
    xE.u.u.detail = (fe->id & 0x7F) + sysKbPriv.offset;

    if (fe->value == VKEY_DOWN) {	/* turn on AutoRepeater */
	if (autoRepeatDebug)
            ErrorF("sunKbdProcessEvent: VKEY_DOWN\n");
	autoRepeatEvent = xE;
	autoRepeatFirst = TRUE;
	autoRepeatKeyDown++;
	autoRepeatLastKeyDownTv = fe->time;
    }

    (* pKeyboard->processInputProc) (&xE, pKeyboard);
}

/*-
 *-----------------------------------------------------------------------
 * sunDoneEvents --
 *	Nothing to do, here...
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */
static void
sunKbdDoneEvents (pKeyboard)
    DevicePtr	  pKeyboard;
{
}

/*-
 *-----------------------------------------------------------------------
 * sunChangeKbdTranslation
 *	Makes operating system calls to set keyboard translation 
 *	and direction on or off.
 *
 * Results:
 *	-1 if failure, else 0.
 *
 * Side Effects:
 * 	Changes kernel management of keyboard.
 *
 *-----------------------------------------------------------------------
 */
int
sunChangeKbdTranslation(pKeyboard,makeTranslated)
    DevicePtr pKeyboard;
    Bool makeTranslated;
{   
    KbPrivPtr	pPriv;
    int 	kbdFd;
    int 	tmp;
    int		KbdOpenedHere;

    pPriv = (KbPrivPtr)pKeyboard->devicePrivate;
    kbdFd = pPriv->fd;

    KbdOpenedHere = ( kbdFd < 0 );
    if ( KbdOpenedHere ) {
	kbdFd = open("/dev/kbd", O_RDONLY, 0);
	if ( kbdFd < 0 ) {
	    Error( "sunChangeKbdTranslation: Can't open keyboard" );
	    goto bad;
	}
    }
	
    if (makeTranslated) {
        /*
         * Next set the keyboard into "direct" mode and turn on
         * event translation. If either of these fails, we can't go
         * on.
         */
	if ( ! sunUseSunWindows() ) {
	    tmp = 1;
	    if (ioctl (kbdFd, KIOCSDIRECT, &tmp) < 0) {
		Error ("Setting keyboard direct mode");
		goto bad;
	    }
	}
	tmp = TR_UNTRANS_EVENT;
	if (ioctl (kbdFd, KIOCTRANS, &tmp) < 0) {
	    Error ("Setting keyboard translation");
	    goto bad;
	}
    }
    else {
        /*
         * Next set the keyboard into "indirect" mode and turn off
         * event translation.
         */
	if ( ! sunUseSunWindows() ) {
	    tmp = 0;
	    (void)ioctl (kbdFd, KIOCSDIRECT, &tmp);
	}
	tmp = ((SunKbPrivPtr)pPriv->devPrivate)->trans;
	(void)ioctl (kbdFd, KIOCTRANS, &tmp);
    }

    if ( KbdOpenedHere )
	(void) close( kbdFd );
    return(0);

bad:
    if ( KbdOpenedHere )
	(void) close( kbdFd );
    return( -1 );
}


#ifdef SUN_WINDOWS

/*-
 *-----------------------------------------------------------------------
 * sunSetUpKbdSunWin
 *	Change which events the kernel will pass through as keyboard
 * 	events.
 *
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */

Bool
sunSetUpKbdSunWin(windowFd, onoff, pKeyboard)
    int windowFd;
    Bool onoff;
    DeviceRec *pKeyboard;
{
    struct inputmask inputMask;
    static struct inputmask oldInputMask;

    if (onoff) {
        register int i;

	win_get_kbd_mask(windowFd, &oldInputMask);
	input_imnull(&inputMask);
	inputMask.im_flags |= 
		IM_ASCII | IM_NEGASCII | 
		IM_META | IM_NEGMETA | 
		IM_NEGEVENT | IM_INTRANSIT;
	win_setinputcodebit(&inputMask, KBD_USE);
	win_setinputcodebit(&inputMask, KBD_DONE);
	win_setinputcodebit(&inputMask, SHIFT_CAPSLOCK);
	win_setinputcodebit(&inputMask, SHIFT_LOCK);
	win_setinputcodebit(&inputMask, SHIFT_LEFT);
	win_setinputcodebit(&inputMask, SHIFT_RIGHT);
	win_setinputcodebit(&inputMask, SHIFT_LEFTCTRL);
	win_setinputcodebit(&inputMask, SHIFT_RIGHTCTRL);
	win_setinputcodebit(&inputMask, SHIFT_META);
	win_setinputcodebit(&inputMask, WIN_STOP);

        for (i=KEY_LEFTFIRST; i<=KEY_LEFTLAST; i++) {
            win_setinputcodebit(&inputMask, i);
        }
        for (i=KEY_TOPFIRST; i<=KEY_TOPLAST; i++) {
            win_setinputcodebit(&inputMask, i);
        }
        for (i=KEY_RIGHTFIRST; i<=KEY_RIGHTLAST; i++) {
            win_setinputcodebit(&inputMask, i);
        }

	win_set_kbd_mask(windowFd, &inputMask);

        /*
         * Set the keyboard into "direct" mode and turn on
         * event translation.
         */
#ifdef notdef
	if (sunChangeKbdTranslation(pKeyboard,TRUE) < 0) {
	    FatalError("Can't set keyboard translation\n");
	}
#endif notdef
    }
    else {
	win_set_kbd_mask(windowFd, &oldInputMask);

        /*
         * Restore original keyboard directness and translation.
         */
	if (sunChangeKbdTranslation(pKeyboard,FALSE) < 0) {
	    FatalError("Can't reset keyboard translation\n");
	}

    }
    return (TRUE);
}

#endif SUN_WINDOWS


#ifdef SUN_WINDOWS

/*-
 *-----------------------------------------------------------------------
 * sunKbdProcessEventSunWin
 *	Process sunwindows event destined for the keyboard.
 *      Rather than replicate the logic (and therefore replicate
 * 	bug fixes, etc), this code counterfeits a vuid 
 *	Firm_event and then uses the non-sunwindows code.
 * 	
 * Results:
 *
 * Side Effects:
 *
 *-----------------------------------------------------------------------
 */

void
sunKbdProcessEventSunWin(pKeyboard,se)
    DeviceRec *pKeyboard;
    register struct inputevent *se;
{   
    Firm_event	fe;

    fe.time = event_time(se);
    fe.id = event_id(se);
    fe.value = (event_is_up(se) ? VKEY_UP : VKEY_DOWN);

    sunKbdProcessEvent (pKeyboard, &fe);
}
#endif SUN_WINDOWS


Bool
LegalModifier(key)
{
    return (TRUE);
}

/*ARGSUSED*/
void
sunBlockHandler(nscreen, pbdata, pptv, pReadmask)
    int nscreen;
    pointer pbdata;
    struct timeval **pptv;
    pointer pReadmask;
{
    static struct timeval artv;	/* autorepeat timeval */
    static sec1 = 0;			/* tmp for patching */
    static sec2 = AUTOREPEAT_INITIATE;
    static sec3 = AUTOREPEAT_DELAY;

    if (!autoRepeatKeyDown)
	return;

    artv.tv_sec = sec1;
    if (autoRepeatFirst == TRUE)
	artv.tv_usec = 1000 * sec2;
    else
	artv.tv_usec = 1000 * sec3;
    *pptv = &artv;
    if (autoRepeatDebug)
	ErrorF("sunBlockHandler(%d,%d): \n", artv.tv_sec, artv.tv_usec);
}

/*ARGSUSED*/
void
sunWakeupHandler(nscreen, pbdata, err, pReadmask)
    int nscreen;
    pointer pbdata;
    unsigned long err;
    pointer pReadmask;
{
    struct timeval tv;

    if (autoRepeatDebug)
	ErrorF("sunWakeupHandler(ar=%d, err=%d):\n", autoRepeatKeyDown, err);

    if (autoRepeatKeyDown) {
	gettimeofday(&tv, (struct timezone *) NULL);
	tvminus(autoRepeatDeltaTv, tv, autoRepeatLastKeyDownTv);
	if (autoRepeatDeltaTv.tv_sec > 0 ||
			(!autoRepeatFirst && autoRepeatDeltaTv.tv_usec >
				1000 * AUTOREPEAT_DELAY) ||
			(autoRepeatDeltaTv.tv_usec >
				1000 * AUTOREPEAT_INITIATE))
		autoRepeatReady++;
    }
    
    if (autoRepeatReady)
	ProcessInputEvents();
    autoRepeatReady = 0;
}

