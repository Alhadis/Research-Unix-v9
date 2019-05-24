/***********************************************************
		Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
#define INT16 short	     /* 		non byte-swapping	 | */
#define INT32 int	     /* 		machines		 | */

#define VIKROOT 0xF40A0000
#define VWRITE(BUF,N,OFF)	\
	{ bcopy(BUF,(char *)(VIKROOT + OFF),(int) N+N);  }
#define VREAD(BUF,N,OFF)	\
	{ bcopy((char *)(VIKROOT + OFF),BUF,(int) N+N);  }

#define AED_SCREEN_HEIGHT 800
#define AED_SCREEN_WIDTH 1024
#define AED_SCREEN_DEVICE "/dev/aed"
#define MOUSE_DEVICE "/dev/msaed"

#define MICROCODE "/usr/lib/aed/whim.aed"

#define VIKSEMA		0
#define VIKCMD 		1
#define ORMERGE 	2
#define ORFONTID 	3
#define ORWIDTH 	4
#define ORDASHPAT 	5
#define ORDASHLEN 	6
#define ORXPOSN 	7
#define ORYPOSN 	8
#define ORXORG 		9
#define ORYORG 		10
#define ORCLIPLX 	11
#define ORCLIPLY 	12
#define ORCLIPHX 	13
#define ORCLIPHY 	14
#define ORCOLOR 	15
#define ORLEN 		16
#define ORDATA 		17
#define FFID		2
#define CFFID		2
#define CFCID		3

/* private field of GC */
typedef struct {
    short	rop;		/* reduction of rasterop to 1 of 3 */
    short	ropOpStip;	/* rop for opaque stipple */
    short	fExpose;	/* callexposure handling ? */
    short	freeCompClip;
    PixmapPtr	pRotatedTile; /* tile/stipple  rotated to align with window */
    PixmapPtr	pRotatedStipple;	/* and using offsets */
    RegionPtr	pAbsClientRegion; /* client region in screen coords */
    RegionPtr	pCompositeClip; /* FREE_CC or REPLACE_CC */
    void 	(* FillArea)();
    PixmapPtr   *ppPixmap;	/* points to the pixmapPtr to
				   use for tiles and stipples */
    short lastDrawableType;	/* was last drawable a window or a pixmap */
    } aedPrivGC;
typedef aedPrivGC	*aedPrivGCPtr;


extern unsigned short vikint[2048];
extern unsigned short *semaphore;
extern int vikoff;
extern int mergexlate[16];

extern aedCursorInit();
extern Bool aedRealizeCursor();
extern Bool aedUnrealizeCursor();
extern int aedDisplayCursor();
extern aedShowCursor();
extern void aedSolidFS();
extern void aedTileFS();
extern Bool aedCreateGC();
extern void aedDestroyGC();
extern void aedValidateGC();
extern unsigned int * aedGetSpans();
extern Bool aedScreenInit();
extern aedScreenClose();
extern void aedPaintWindowSolid();
extern Bool aedInitScreen();
extern void aedSetSpans();
extern Bool aedCreateWindow();
extern void aedCopyWindow();
extern Bool aedChangeWindowAttributes();
extern void aedSolidLine();
extern void aedDashLine();
extern void aedPaintWindowTile();
extern void aedImageGlyphBlt();
extern void aedCopyArea();
extern Bool aedPositionWindow();
extern void aedPolyFillRect();
extern void aedSolidFillArea();
extern void aedStippleFillArea();
extern void aedPolySegment();
extern int aedSetCursorPosition();
extern void aedPushPixSolid();
extern void aedDrawImage();
