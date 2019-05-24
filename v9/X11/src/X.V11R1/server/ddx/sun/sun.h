/*-
 * sun.h --
 *	Internal declarations for the sun ddx interface
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
 *	"$Header: sun.h,v 4.3 87/09/12 02:29:58 sun Exp $ SPRITE (Berkeley)"
 */
#ifndef _SUN_H_
#define _SUN_H_

#include    <errno.h>
#include    <sys/param.h>
#include    <sys/types.h>
#include    <sys/time.h>
#include    <sys/file.h>
#include    <sys/fcntl.h>
#include    <sys/signal.h>
#include    <sundev/kbd.h>
#include    <sundev/kbio.h>
#include    <sundev/msio.h>
#include    <sun/fbio.h>

/*
 * SUN_WINDOWS is now defined (or not) by the Makefile
 * variable $(SUNWINDOWSFLAGS) in server/Makefile.
 */

#ifdef SUN_WINDOWS
#include    <varargs.h>
#include    <sys/ioctl.h>
#include    <stdio.h>
#include    <pixrect/pixrect_hs.h>
#include    <sunwindow/rect.h>
#include    <sunwindow/rectlist.h>
#include    <sunwindow/pixwin.h>
#include    <sunwindow/win_screen.h>
#include    <sunwindow/win_input.h>
#include    <sunwindow/cms.h>
#include    <sunwindow/win_struct.h>
#else 
/* already included by sunwindow/win_input.h */
#include    <sundev/vuid_event.h>
#endif SUN_WINDOWS

#include    "X.h"
#include    "Xproto.h"
#include    "scrnintstr.h"
#include    "screenint.h"
#ifdef NEED_EVENTS
#include    "inputstr.h"
#endif NEED_EVENTS
#include    "input.h"
#include    "cursorstr.h"
#include    "cursor.h"
#include    "pixmapstr.h"
#include    "pixmap.h"
#include    "windowstr.h"
#include    "gc.h"
#include    "gcstruct.h"
#include    "regionstr.h"
#include    "colormap.h"
#include    "miscstruct.h"
#include    "dix.h"
#include    "mfb.h"
#include    "mi.h"
#ifdef ZOIDS
#include    "zoid.h"
#endif ZOIDS

/*
 * MAXEVENTS is the maximum number of events the mouse and keyboard functions
 * will read on a given call to their GetEvents vectors.
 */
#define MAXEVENTS 	32

/*
 * Data private to any sun keyboard.
 *	GetEvents reads any events which are available for the keyboard
 *	ProcessEvent processes a single event and gives it to DIX
 *	DoneEvents is called when done handling a string of keyboard
 *	    events or done handling all events.
 *	devPrivate is private to the specific keyboard.
 *	map_q is TRUE if the event queue for the keyboard is memory mapped.
 */
typedef struct kbPrivate {
    int	    	  type;           	/* Type of keyboard */
    int	    	  fd;	    	    	/* Descriptor open to device */
    Firm_event	  *(*GetEvents)();  	/* Function to read events */
    void    	  (*ProcessEvent)();	/* Function to process an event */
    void    	  (*DoneEvents)();  	/* Function called when all events */
					/* have been handled. */
    pointer 	  devPrivate;	    	/* Private to keyboard device */
    Bool	  map_q;		/* TRUE if fd has a mapped event queue */
    int		  offset;		/* to be added to device keycodes */
} KbPrivRec, *KbPrivPtr;

#define	MIN_KEYCODE	8	/* necessary to avoid the mouse buttons */

/*
 * Data private to any sun pointer device.
 *	GetEvents, ProcessEvent and DoneEvents have uses similar to the
 *	    keyboard fields of the same name.
 *	pScreen is the screen the pointer is on (only valid if it is the
 *	    main pointer device).
 *	x and y are absolute coordinates on that screen (they may be negative)
 */
typedef struct ptrPrivate {
    int	    	  fd;	    	    	/* Descriptor to device */
    Firm_event 	  *(*GetEvents)(); 	/* Function to read events */
    void    	  (*ProcessEvent)();	/* Function to process an event */
    void    	  (*DoneEvents)();  	/* When all the events have been */
					/* handled, this function will be */
					/* called. */
    short   	  x,	    	    	/* Current X coordinate of pointer */
		  y;	    	    	/* Current Y coordinate */
    ScreenPtr	  pScreen;  	    	/* Screen pointer is on */
    pointer    	  devPrivate;	    	/* Field private to device */
} PtrPrivRec, *PtrPrivPtr;

/*
 * Cursor-private data
 *	screenBits	saves the contents of the screen before the cursor
 *	    	  	was placed in the frame buffer.
 *	source	  	a bitmap for placing the foreground pixels down
 *	srcGC	  	a GC for placing the foreground pixels down.
 *	    	  	Prevalidated for the cursor's screen.
 *	invSource 	a bitmap for placing the background pixels down.
 *	invSrcGC  	a GC for placing the background pixels down.
 *	    	  	Also prevalidated for the cursor's screen Pixmap.
 *	temp	  	a temporary pixmap for low-flicker cursor motion --
 *	    	  	exists to avoid the overhead of creating a pixmap
 *	    	  	whenever the cursor must be moved.
 *	fg, bg	  	foreground and background pixels. For a color display,
 *	    	  	these are allocated once and the rgb values changed
 *	    	  	when the cursor is recolored.
 *	scrX, scrY	the coordinate on the screen of the upper-left corner
 *	    	  	of screenBits.
 *	state	  	one of CR_IN, CR_OUT and CR_XING to track whether the
 *	    	  	cursor is in or out of the frame buffer or is in the
 *	    	  	process of going from one state to the other.
 */
typedef enum {
    CR_IN,		/* Cursor in frame buffer */
    CR_OUT,		/* Cursor out of frame buffer */
    CR_XING	  	/* Cursor in flux */
} CrState;

typedef struct crPrivate {
    PixmapPtr  	        screenBits; /* Screen before cursor put down */
    PixmapPtr  	        source;     /* Cursor source (foreground bits) */
    GCPtr   	  	srcGC;	    /* Foreground GC */
    PixmapPtr  	        invSource;  /* Cursor source inverted (background) */
    GCPtr   	  	invSrcGC;   /* Background GC */
    PixmapPtr  	        temp;	    /* Temporary pixmap for merging screenBits
				     * and the sources. Saves creation time */
    Pixel   	  	fg; 	    /* Foreground color */
    Pixel   	  	bg; 	    /* Background color */
    int	    	  	scrX,	    /* Screen X coordinate of screenBits */
			scrY;	    /* Screen Y coordinate of screenBits */
    CrState		state;      /* Current state of the cursor */
} CrPrivRec, *CrPrivPtr;

/*
 * Frame-buffer-private info.
 *	fd  	  	file opened to the frame buffer device.
 *	info	  	description of the frame buffer -- type, height, depth,
 *	    	  	width, etc.
 *	fb  	  	pointer to the mapped image of the frame buffer. Used
 *	    	  	by the driving routines for the specific frame buffer
 *	    	  	type.
 *	pGC 	  	A GC for realizing cursors.
 *	GetImage  	Original GetImage function for this screen.
 *	CreateGC  	Original CreateGC function
 *	CreateWindow	Original CreateWindow function
 *	ChangeWindowAttributes	Original function
 *	GetSpans  	GC function which needs to be here b/c GetSpans isn't
 *	    	  	called with the GC as an argument...
 *	mapped	  	flag set true by the driver when the frame buffer has
 *	    	  	been mapped in.
 *	parent	  	set true if the frame buffer is actually a SunWindows
 *	    	  	window.
 *	fbPriv	  	Data private to the frame buffer type.
 */
typedef struct {
    pointer 	  	fb; 	    /* Frame buffer itself */
    GCPtr   	  	pGC;	    /* GC for realizing cursors */

    void    	  	(*GetImage)();
    Bool	      	(*CreateGC)();/* GC Creation function previously in the
				       * Screen structure */
    Bool	      	(*CreateWindow)();
    Bool		(*ChangeWindowAttributes)();
    unsigned int  	*(*GetSpans)();
    void		(*EnterLeave)();
    Bool    	  	mapped;	    /* TRUE if frame buffer already mapped */
    Bool		parent;	    /* TRUE if fd is a SunWindows window */
    int	    	  	fd; 	    /* Descriptor open to frame buffer */
    struct fbtype 	info;	    /* Frame buffer characteristics */
    pointer 	  	fbPriv;	    /* Frame-buffer-dependent data */
} fbFd;

/*
 * Data describing each type of frame buffer. The probeProc is called to
 * see if such a device exists and to do what needs doing if it does. devName
 * is the expected name of the device in the file system. Note that this only
 * allows one of each type of frame buffer. This may need changing later.
 */
typedef enum {
	neverProbed, probedAndSucceeded, probedAndFailed
} SunProbeStatus;

/*
 * ZOIDS should only ever be defined if SUN_WINDOWS is defined.
 */
typedef struct _sunFbDataRec {
    Bool    (*probeProc)();	/* probe procedure for this fb */
    char    *devName;		/* device filename */
    SunProbeStatus probeStatus;	/* TRUE if fb has been probed successfully */
#ifdef ZOIDS
    Pixrect *pr;		/* set for bwtwo's only */
    Pixrect *scratch_pr;	/* set for bwtwo's only */
#endif ZOIDS
} sunFbDataRec;

extern sunFbDataRec sunFbData[];
/*
 * Cursor functions
 */
extern void 	  sunInitCursor();
extern Bool 	  sunRealizeCursor();
extern Bool 	  sunUnrealizeCursor();
extern Bool 	  sunDisplayCursor();
extern Bool 	  sunSetCursorPosition();
extern void 	  sunCursorLimits();
extern void 	  sunPointerNonInterestBox();
extern void 	  sunConstrainCursor();
extern void 	  sunRecolorCursor();
extern Bool	  sunCursorLoc();
extern void 	  sunRemoveCursor();
extern void	  sunRestoreCursor();

/*
 * Initialization
 */
extern void 	  sunScreenInit();
extern int  	  sunOpenFrameBuffer();

/*
 * GC Interceptions
 */
extern GCPtr	  sunCreatePrivGC();
extern Bool	  sunCreateGC();
extern Bool	  sunCreateWindow();
extern Bool	  sunChangeWindowAttributes();

extern void 	  sunGetImage();
extern unsigned int *sunGetSpans();

extern int	  isItTimeToYield;
extern int  	  sunCheckInput;    /* Non-zero if input is available */

extern fbFd 	  sunFbs[];
extern Bool	  screenSaved;		/* True is screen is being saved */

extern int  	  lastEventTime;    /* Time (in ms.) of last event */
extern void 	  SetTimeSinceLastInputEvent();
extern void	ErrorF();

#define AUTOREPEAT_INITIATE     (300)           /* milliseconds */
#define AUTOREPEAT_DELAY        (100)           /* milliseconds */
/*
 * We signal autorepeat events with the unique Firm_event
 * id AUTOREPEAT_EVENTID.
 * Because inputevent ie_code is set to Firm_event ids in
 * sunKbdProcessEventSunWin, and ie_code is short whereas
 * Firm_event id is u_short, we use 0x7fff.
 */
#define AUTOREPEAT_EVENTID      (0x7fff)        /* AutoRepeat Firm_event id */

extern int	autoRepeatKeyDown;		/* TRUE if key down */
extern int	autoRepeatReady;		/* TRUE if time out */
extern int	autoRepeatDebug;		/* TRUE if debugging */

/*
 * Sun specific extensions:
 *	trapezoids
 */
#ifdef ZOIDS
extern void	  sunBW2SolidXZoids();
extern void	  sunBW2SolidYZoids();
extern void	  sunBW2TiledXZoids();
extern void	  sunBW2TiledYZoids();
extern void	  sunBW2StipXZoids();
extern void	  sunBW2StipYZoids();
#endif ZOIDS

/*-
 * TVTOMILLI(tv)
 *	Given a struct timeval, convert its time into milliseconds...
 */
#define TVTOMILLI(tv)	(((tv).tv_usec/1000)+((tv).tv_sec*1000))

#ifdef SUN_WINDOWS
extern int windowFd;
#endif SUN_WINDOWS

#endif _SUN_H_
