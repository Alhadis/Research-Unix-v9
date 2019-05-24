/* $Header: Xprotostr.h,v 1.1 87/09/11 06:37:58 toddb Exp $ */
#ifndef XPROTOSTRUCTS_H
#define XPROTOSTRUCTS_H

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
#include "Xmd.h"

/* Used by PolySegment */

typedef struct _xSegment {
    INT16 x1, y1, x2, y2;
} xSegment;

/* POINT */

typedef struct _xPoint {
	INT16		x,y;
} xPoint;

typedef struct _xRectangle {
    INT16 x, y;
    CARD16  width, height;
} xRectangle;

/*  ARC  */

typedef struct _xArc {
    INT16 x, y;
    CARD16   width, height;
    INT16   angle1, angle2;
} xArc;

#endif /* XPROTOSTRUCTS_H */
