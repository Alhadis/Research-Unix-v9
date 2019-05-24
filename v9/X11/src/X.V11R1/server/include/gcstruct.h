/* $Header: gcstruct.h,v 1.1 87/09/11 07:49:45 toddb Exp $ */
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

#ifndef GCSTRUCT_H
#define GCSTRUCT_H

#include "gc.h"

#include "miscstruct.h"
#include "region.h"
#include "pixmap.h"
#include "screenint.h"
#include "dixfont.h"

typedef struct _GCInterest {
    struct _GCInterest	*pNextGCInterest;
    struct _GCInterest	*pLastGCInterest;
    int			length;		
    ATOM		owner;		/* extension id of owning extension */
    unsigned long	ValInterestMask;
    void		(* ValidateGC) ();
    unsigned long	ChangeInterestMask;
    int			(* ChangeGC) ();
    void		(* CopyGCSource) ();
    void		(* CopyGCDest) ();
    void		(* DestroyGC) ();
    pointer		extPriv;	/* pointer extension private data */
} GCInterestRec;

typedef struct _GC{
    ScreenPtr	pScreen;		
    pointer	devPriv;		/* private to the device */
    int         depth;    
    unsigned long        serialNumber;
    GCInterestPtr	pNextGCInterest;
    GCInterestPtr	pLastGCInterest;
    int		alu;
    unsigned long	planemask;
    unsigned long	fgPixel, bgPixel;
    int		lineWidth;          
    int		lineStyle;
    int		capStyle;
    int		joinStyle;
    int		fillStyle;
    int		fillRule;
    int		arcMode;
    PixmapPtr	tile;
    PixmapPtr	stipple;
    DDXPointRec	patOrg;			/* origin for (tile, stipple) */
    FontPtr	font;
    int		subWindowMode;
    Bool	graphicsExposures;
    DDXPointRec	clipOrg;
    pointer	clientClip;
    int		clientClipType;		/* pixmap, region, or none */
    int		dashOffset;
    int		numInDashList;		/* num elements in dash linst */
    unsigned char *dash;		/* dash pattern */

    unsigned long	stateChanges;	/* masked with GC_* */
    DDXPointRec	lastWinOrg;		/* origin of last window */
    int		miTranslate:1;		/* should mi things translate? */

    void (* FillSpans)();
    void (* SetSpans)();

    void (* PutImage)();
    void (* CopyArea)();
    void (* CopyPlane)();
    void (* PolyPoint)();
    void (* Polylines)();
    void (* PolySegment)();
    void (* PolyRectangle)();
    void (* PolyArc)();
    void (* FillPolygon)();
    void (* PolyFillRect)();
    void (* PolyFillArc)();
    int (* PolyText8)();
    int (* PolyText16)();
    void (* ImageText8)();
    void (* ImageText16)();
    void (* ImageGlyphBlt)();
    void (* PolyGlyphBlt)();
    void (* PushPixels)();
    void (* LineHelper)();
    void (* ChangeClip) ();
    void (* DestroyClip) ();
    void (* CopyClip)();
} GC;

extern GC *CreateGC();
extern GC BltGCs[];
extern void FreeGC();

#endif /* GCSTRUCT_H */
