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
extern int errno;
#include    <sys/types.h>
#include    <sys/timeb.h>
#include    <signal.h>
#include    <sun/fbio.h>

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

/*
 * MAXEVENTS is the maximum number of events the mouse and keyboard functions
 * will read on a given call to their GetEvents vectors.
 */
#define MAXEVENTS 	32

/*
 * Data private to any sun pointer device.
 *	GetEvents, ProcessEvent and DoneEvents have uses similar to the
 *	    keyboard fields of the same name.
 *	pScreen is the screen the pointer is on (only valid if it is the
 *	    main pointer device).
 *	x and y are absolute coordinates on that screen (they may be negative)
 */
typedef struct ptrPrivate {
    short   	  x,	    	    	/* Current X coordinate of pointer */
		  y;	    	    	/* Current Y coordinate */
    ScreenPtr	  pScreen;  	    	/* Screen pointer is on */
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

#endif _SUN_H_
