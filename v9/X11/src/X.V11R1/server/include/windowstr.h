/* $Header: windowstr.h,v 1.3 87/09/10 01:43:43 toddb Exp $ */
/***********************************************************
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

******************************************************************/

#ifndef WINDOWSTRUCT_H
#define WINDOWSTRUCT_H

#include "window.h"
#include "pixmapstr.h"
#include "region.h"
#include "cursor.h"
#include "property.h"
#include "resource.h"	/* for ROOT_WINDOW_ID_BASE */
#include "dix.h"
#include "miscstruct.h"
#include "Xprotostr.h"


typedef struct _BackingStore {
    RegionPtr obscured;
    void (* SaveDoomedAreas)();
    void (* RestoreAreas)();
    void (* TranslateBackingStore)(); /* to make bit gravity and backing
					store work together */
} BackingStoreRec;

/* 
 * A window -- device independent
 * 
 */

typedef struct _Window {

	DrawableRec drawable;		/* screen and type */

	VisualID visual;

	struct _Window *parent;	        /* Other windows it contains */
	struct _Window *nextSib;	        /* Other windows it contains */
	struct _Window *prevSib;	        /* (linked two ways) */
	struct _Window *firstChild;	/* top-most window this contains */
	struct _Window *lastChild;	/* bottom-most window it contains */

	CursorPtr cursor;                 /* cursor information */

	ClientPtr client;		/* client object for creator */
	long wid;                        /* client's name for this window */

	RegionPtr clipList;               /* clipping rectangle for output*/
	RegionPtr winSize;                /* inside window dimensions, 
					  clipped to parent */
	RegionPtr borderClip;             /* clipList + border */
	RegionPtr borderSize;             /* window + border, clip to parent */
        RegionPtr exposed;                /* list of exposed regions, 
					  translated.  After ValidateTree,
					  draw background in exposed and 
					  send translated regions to client */
	
	RegionPtr borderExposed;
	xRectangle clientWinSize;       /* x,y, w,h of unobscured window 
					  relative to parent */
	DDXPointRec  absCorner;
	DDXPointRec  oldAbsCorner;      /* used in ValidateTree */
	int class;                    /* InputOutput, InputOnly */
	Mask eventMask;
	Mask dontPropagateMask;
	Mask allEventMasks;
	Mask deliverableEvents;
	pointer otherClients;		/* defined in input.h */
	pointer passiveGrabs;		/* define in input.h */

	PropertyPtr userProps;            /* client's property list */

        XID backgroundPixmapID;        /* for screen saver, root only */
	PixmapPtr backgroundTile;
	unsigned long backgroundPixel;
	PixmapPtr borderTile;
	unsigned long borderPixel;
	int borderWidth;
        void (* PaintWindowBackground)();
        void (* PaintWindowBorder)();
	void (* CopyWindow)();
	void (* ClearToBackground)();

	unsigned long backingBitPlanes;
	unsigned long backingPixel;
	int  backingStore;           /* no, whenMapped, always */
	BackingStorePtr backStorage;

	char  bitGravity;
        char  winGravity;
	Colormap colormap;
		
            /* bits for accelerator information */            
                     
	Bool	saveUnder:1;
        unsigned  visibility:2;		      
	unsigned mapped:1;
	unsigned realized:1;            /* ancestors are all mapped */
	unsigned viewable:1;            /* realized && InputOutput */
	unsigned overrideRedirect:1;
	unsigned marked:1;

	pointer devBackingStore;		/* optional */
	pointer devPrivate;			/* dix never looks at this */
} WindowRec;

extern int DeleteWindow();
extern int ChangeWindowAttributes();
extern int WalkTree();
extern CreateRootWindow();
extern WindowPtr CreateWindow();
extern int DeleteWindow();
extern int DestroySubwindows();
extern int ChangeWindowAttributes();
extern int GetWindowAttributes();
extern int ConfigureWindow();
extern int ReparentWindow();
extern int MapWindow();
extern int MapSubwindow();
extern int UnmapWindow();
extern int UnmapSubwindow();
extern RegionPtr NotClippedByChildren();

#endif /* WINDOWSTRUCT_H */

