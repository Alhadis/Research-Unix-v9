/* $Header: pixmapstr.h,v 1.1 87/09/11 07:50:20 toddb Exp $ */
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

#ifndef PIXMAPSTRUCT_H
#define PIXMAPSTRUCT_H
#include "pixmap.h"
#include "screenint.h"
#include "misc.h"

/* first element of any drawable */
typedef struct _DrawInfo {
    short	type;
    ScreenPtr	pScreen;     
    int         depth;          /* alias bitsPerPixel, numPlanes */
    unsigned long        serialNumber;
} DrawableRec;
/*
 * PIXMAP -- device dependent 
 */

typedef struct _Pixmap {
    DrawableRec drawable;
    int width, height;
    int refcnt;
    int devKind;		/* probably flags or something */
    pointer devPrivate;		/* probably a pointer */
    } PixmapRec;
#endif /* PIXMAPSTRUCT_H */
