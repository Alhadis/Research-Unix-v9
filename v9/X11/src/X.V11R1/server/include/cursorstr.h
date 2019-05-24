/* $Header: cursorstr.h,v 1.1 87/09/11 07:49:40 toddb Exp $ */
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
#ifndef CURSORSTRUCT_H
#define CURSORSTRUCT_H 

#include "cursor.h"
#include "pixmap.h"
#include "misc.h"
/* 
 * device-independent cursor storage
 *
 * source and mask point directly to the bits, which are in the server-defined
 * bitmap format.
 */
typedef struct _Cursor {
    unsigned char *source;			/* points to bits */
    unsigned char *mask;			/* points to bits */
    int width;
    int height;
    int xhot;					/* must be within bitmap */
    int yhot;					/* must be within bitmap */
    unsigned foreRed, foreGreen, foreBlue;	/* device-independent color */
    unsigned backRed, backGreen, backBlue;	/* device-independent color */
    int refcnt;
    pointer devPriv[MAXSCREENS];		/* set by pScr->RealizeCursor*/
} CursorRec;

typedef struct _CursorMetric {
    int width, height, xhot, yhot;
} CursorMetricRec;

extern void		FreeCursor();
extern CursorPtr	AllocCursor();		/* also realizes it */
				/* created from default cursor font */
extern CursorPtr	CreateRootCursor();

#endif /* CURSORSTRUCT_H */
