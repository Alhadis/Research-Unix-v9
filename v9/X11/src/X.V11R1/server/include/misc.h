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
/* $Header: misc.h,v 1.43 87/09/11 07:49:51 toddb Exp $ */
#ifndef MISC_H
#define MISC_H 1
/*
 *  X internal definitions 
 *
 */


extern unsigned long globalSerialNumber;

#ifndef NULL
#define NULL            0
#endif

#define MAXSCREENS	3
#define MAXCLIENTS	128
#define MAXFORMATS	8
#define MAXVISUALS_PER_SCREEN 50

typedef unsigned char *pointer;
typedef int Bool;
typedef long PIXEL;
typedef int ATOM;


#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#include "os.h" 	/* for ALLOCATE_LOCAL and DEALLOCATE_LOCAL */

#define NullBox ((BoxPtr)0)
#define MILLI_PER_MIN (1000 * 60)
#define MILLI_PER_SECOND (1000)
#define DEFAULT_SCREEN_SAVER_TIME (10 * MILLI_PER_MIN);

    /* this next is used with None and ParentRelative to tell
       PaintWin() what to use to paint the background. Also used
       in the macro IS_VALID_PIXMAP */

#define USE_BACKGROUND_PIXEL 3
#define USE_BORDER_PIXEL 3


/* byte swap a long literal */
#define lswapl(x) ((((x) & 0xff) << 24) |\
		   (((x) & 0xff00) << 8) |\
		   (((x) & 0xff000) >> 8) |\
		   (((x) >> 24) & 0xff))

/* byte swap a short literal */
#define lswaps(x) ((((x) & 0xff) << 8) | (((x) >> 8) & 0xff))

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define abs(a) ((a) > 0 ? (a) : -(a))
#define fabs(a) ((a) > 0.0 ? (a) : -(a))	/* floating absolute value */
#define sign(x) ((x) < 0 ? -1 : ((x) > 0 ? 1 : 0))

#define MAXSHORT 32767
#define MINSHORT -MAXSHORT 


/* byte swap a long */
#define swapl(x, n) n = ((char *) (x))[0];\
		 ((char *) (x))[0] = ((char *) (x))[3];\
		 ((char *) (x))[3] = n;\
		 n = ((char *) (x))[1];\
		 ((char *) (x))[1] = ((char *) (x))[2];\
		 ((char *) (x))[2] = n;

/* byte swap a short */
#define swaps(x, n) n = ((char *) (x))[0];\
		 ((char *) (x))[0] = ((char *) (x))[1];\
		 ((char *) (x))[1] = n



typedef struct _DDXPoint *DDXPointPtr;
typedef struct _Box *BoxPtr;
typedef struct _Rectangle *RectanglePtr;

#endif /* MISC_H */
