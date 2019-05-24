/* $Header: regionstr.h,v 1.1 87/09/11 07:50:10 toddb Exp $ */
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
#ifndef REGIONSTRUCT_H
#define REGIONSTRUCT_H

#include "region.h"
#include "miscstruct.h"
/* 
 *   clip region
 */

typedef struct _Region {
    short	size;
    short 	numRects;
    BoxPtr	rects;
    BoxRec 	extents;
} RegionRec;

/*
 *   Check to see if there is enough memory in the present region.
 */
#define MEMCHECK(reg, rect, firstrect){\
        if ((reg)->numRects >= ((reg)->size - 1)){\
          (firstrect) = (BoxPtr) Xrealloc \
          ((firstrect), (2 * (sizeof(BoxRec)) * ((reg)->size)));\
          (reg)->size *= 2;\
          (rect) = &(firstrect)[(reg)->numRects];\
         }\
       }

/*  this routine checks to see if the previous rectangle is the same
 *  or subsumes the new rectangle to add.
 */

#define CHECK_PREVIOUS(Reg, R, Rx1, Ry1, Rx2, Ry2)\
               (!(((Reg)->numRects > 0)&&\
                  ((R-1)->y1 == (Ry1)) &&\
                  ((R-1)->y2 == (Ry2)) &&\
                  ((R-1)->x1 <= (Rx1)) &&\
                  ((R-1)->x2 >= (Rx2))))

/*  add a rectangle to the given Region */
#define ADDRECT(reg, r, rx1, ry1, rx2, ry2){\
    if (((rx1) < (rx2)) && ((ry1) < (ry2)) &&\
        CHECK_PREVIOUS((reg), (r), (rx1), (ry1), (rx2), (ry2))){\
              (r)->x1 = (rx1);\
              (r)->y1 = (ry1);\
              (r)->x2 = (rx2);\
              (r)->y2 = (ry2);\
              EXTENTS((r), (reg));\
              (reg)->numRects++;\
              (r)++;\
            }\
        }


/*
 *  update region extents
 */
#define EXTENTS(r,idRect){\
            if((r)->x1 < (idRect)->extents.x1)\
              (idRect)->extents.x1 = (r)->x1;\
            if((r)->y1 < (idRect)->extents.y1)\
              (idRect)->extents.y1 = (r)->y1;\
            if((r)->x2 > (idRect)->extents.x2)\
              (idRect)->extents.x2 = (r)->x2;\
            if((r)->y2 > (idRect)->extents.y2)\
              (idRect)->extents.y2 = (r)->y2;\
        }


#define EMPTY_REGION(pReg) (pReg->numRects = 0)

#define REGION_NOT_EMPTY(pReg) (pReg->numRects)

extern RegionPtr miRegionCreate();
extern void miprintRects();
extern void miRegionCopy();
extern int miInverse();
extern int miIntersect();
extern int miUnion();
extern int miSubtract();
extern int miRectIn();
extern void miTranslateRegion();
extern void miRegionDestroy();
extern void miRegionReset();
extern Bool miPointInRegion();

#endif /* REGIONSTRUCT_H */
