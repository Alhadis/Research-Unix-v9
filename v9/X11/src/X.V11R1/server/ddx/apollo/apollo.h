/******************************************************************
Copyright 1987 by Apollo Computer Inc., Chelmsford, Massachusetts.

                        All Rights Reserved

Permission to use, duplicate, change, and distribute this software and
its documentation for any purpose and without fee is granted, provided
that the above copyright notice appear in such copy and that this
copyright notice appear in all supporting documentation, and that the
names of Apollo Computer Inc. or MIT not be used in advertising or publicity
pertaining to distribution of the software without written prior permission.
******************************************************************/

#include "/sys/ins/base.ins.c"
#include "/sys/ins/ec2.ins.c"
#include "/sys/ins/gpr.ins.c"
#include "/sys/ins/ios.ins.c"
#include "/sys/ins/io_traits.ins.c"
#include "/sys/ins/kbd.ins.c"
#include "/sys/ins/smdu.ins.c"
#include "/sys/ins/tone.ins.c"
#include "/sys/ins/trait.ins.c"
   
#include <errno.h>
#include <stdio.h>
#include <sys/file.h>
#include <sys/time.h>
#include <sys/types.h>

#include "X.h"
#include "Xmd.h"
#define  NEED_EVENTS
#include "Xproto.h"
#include "misc.h"
#include "cursorstr.h"
#include "cursor.h"
#include "dixstruct.h"
#include "dixfontstr.h"
#include "extnsionst.h"
#include "fontstruct.h"
#include "gcstruct.h"
#include "input.h"
#include "keysym.h"
#include "mfb.h"
#include "mi.h"
#include "miscstruct.h"
#include "pixmapstr.h"
#include "pixmap.h"
#include "regionstr.h"
#include "resource.h"
#include "resourcest.h"
#include "screenint.h"
#include "scrnintstr.h"
#include "servermd.h"
#include "windowstr.h"

/* structure to hold window operation procedures for
   each window - a resource tied to the window. */
typedef struct {
    void	(*PaintWindowBackground)();
    void	(*PaintWindowBorder)();
    void	(*CopyWindow)();
} WinPrivRec, *WinPrivPtr;

int	wPrivClass;		/* Resource class for icky private
				 * window structure (WinPrivRec)
				 * needed to protect the cursor
				 * from background/border paintings */

/* key definition record */
typedef struct _key {
    short key_color;
    short key_mods;
    int base_key;
    } keyRec;

/* private field of Cursor */
typedef struct {
    Bool        cursorIsDown;           /* cursor is not now displayed */
    Bool        cursorLeftDown;         /* cursorIsDown AND has been left down, to be put back sometime later */
    unsigned long   bitsToSet[16];      /* bits of masked cursor image to set */
    unsigned long   bitsToClear[16];    /* bits of masked cursor image to clear */
    unsigned long   savedBits[16];      /* saved bits under cursor */
    int         alignment;              /* cursor x coordinate mod 16 */
    short       *pBitsScreen;           /* address of word where cursor origin is */
    } apPrivCurs;

/* proceudure pointers we stole from Screen and Window structures */
typedef struct {
    Bool	      	(*CreateGC)();/* GC Creation function previously in the
				       * Screen structure */
    Bool	      	(*CreateWindow)();
    Bool	    	(*ChangeWindowAttributes)();
    unsigned int  	*(*GetSpans)();
    void    	  	(*GetImage)();
} apProcPtrs;

extern Bool		apCreateGC();

extern void		miRecolorCursor();
extern Bool		apCreateWindow();
extern Bool		apChangeWindowAttributes();
extern void		apGetImage();
extern unsigned int	*apGetSpans();
extern int  		apGetMotionEvents();
extern void 		apChangePointerControl();
extern void 		apChangeKeyboardControl();
extern void		apBell();
extern Bool		apScreenInit();
extern int		apMouseProc();
extern int		apKeybdProc();

extern int		MakeGPRStream();
extern Bool		GetGPREvent ();

extern gpr_$event_t     apEventType;
extern unsigned char    apEventData[1];
extern gpr_$position_t  apEventPosition;

extern long		*apECV;
extern long		*apLastECV;

extern apProcPtrs	apProcs;
extern CursorPtr	pCurCursor;

extern int		wPrivClass;

