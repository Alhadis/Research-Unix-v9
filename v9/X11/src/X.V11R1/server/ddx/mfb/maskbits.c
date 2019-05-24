/* $Header: maskbits.c,v 1.2 87/09/07 19:04:44 rws Exp $ */
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
#include "maskbits.h"
#include "servermd.h"

/*
   these tables are used by several macros in the mfb code.

   the vax numbers everything left to right, so bit indices on the
screen match bit indices in longwords.  the pc-rt and Sun number
bits on the screen the way they would be written on paper,
(i.e. msb to the left), and so a bit index n on the screen is
bit index 32-n in a longword

   see also maskbits.h
*/

#if (BITMAP_BIT_ORDER == MSBFirst)
/* NOTE:
the first element in starttab could be 0xffffffff.  making it 0
lets us deal with a full first word in the middle loop, rather
than having to do the multiple reads and masks that we'd
have to do if we thought it was partial.
*/
int starttab[32] =
    {
	0x00000000,
	0x7FFFFFFF,
	0x3FFFFFFF,
	0x1FFFFFFF,
	0x0FFFFFFF,
	0x07FFFFFF,
	0x03FFFFFF,
	0x01FFFFFF,
	0x00FFFFFF,
	0x007FFFFF,
	0x003FFFFF,
	0x001FFFFF,
	0x000FFFFF,
	0x0007FFFF,
	0x0003FFFF,
	0x0001FFFF,
	0x0000FFFF,
	0x00007FFF,
	0x00003FFF,
	0x00001FFF,
	0x00000FFF,
	0x000007FF,
	0x000003FF,
	0x000001FF,
	0x000000FF,
	0x0000007F,
	0x0000003F,
	0x0000001F,
	0x0000000F,
	0x00000007,
	0x00000003,
	0x00000001
    };

int endtab[32] =
    {
	0x00000000,
	0x80000000,
	0xC0000000,
	0xE0000000,
	0xF0000000,
	0xF8000000,
	0xFC000000,
	0xFE000000,
	0xFF000000,
	0xFF800000,
	0xFFC00000,
	0xFFE00000,
	0xFFF00000,
	0xFFF80000,
	0xFFFC0000,
	0xFFFE0000,
	0xFFFF0000,
	0xFFFF8000,
	0xFFFFC000,
	0xFFFFE000,
	0xFFFFF000,
	0xFFFFF800,
	0xFFFFFC00,
	0xFFFFFE00,
	0xFFFFFF00,
	0xFFFFFF80,
	0xFFFFFFC0,
	0xFFFFFFE0,
	0xFFFFFFF0,
	0xFFFFFFF8,
	0xFFFFFFFC,
	0xFFFFFFFE
    };

/* a hack, for now, since the entries for 0 need to be all
   1 bits, not all zeros.
   this means the code DOES NOT WORK for segments of length
   0 (which is only a problem in the horizontal line code.)
*/
int startpartial[32] =
    {
	0xFFFFFFFF,
	0x7FFFFFFF,
	0x3FFFFFFF,
	0x1FFFFFFF,
	0x0FFFFFFF,
	0x07FFFFFF,
	0x03FFFFFF,
	0x01FFFFFF,
	0x00FFFFFF,
	0x007FFFFF,
	0x003FFFFF,
	0x001FFFFF,
	0x000FFFFF,
	0x0007FFFF,
	0x0003FFFF,
	0x0001FFFF,
	0x0000FFFF,
	0x00007FFF,
	0x00003FFF,
	0x00001FFF,
	0x00000FFF,
	0x000007FF,
	0x000003FF,
	0x000001FF,
	0x000000FF,
	0x0000007F,
	0x0000003F,
	0x0000001F,
	0x0000000F,
	0x00000007,
	0x00000003,
	0x00000001
    };

int endpartial[32] =
    {
	0xFFFFFFFF,
	0x80000000,
	0xC0000000,
	0xE0000000,
	0xF0000000,
	0xF8000000,
	0xFC000000,
	0xFE000000,
	0xFF000000,
	0xFF800000,
	0xFFC00000,
	0xFFE00000,
	0xFFF00000,
	0xFFF80000,
	0xFFFC0000,
	0xFFFE0000,
	0xFFFF0000,
	0xFFFF8000,
	0xFFFFC000,
	0xFFFFE000,
	0xFFFFF000,
	0xFFFFF800,
	0xFFFFFC00,
	0xFFFFFE00,
	0xFFFFFF00,
	0xFFFFFF80,
	0xFFFFFFC0,
	0xFFFFFFE0,
	0xFFFFFFF0,
	0xFFFFFFF8,
	0xFFFFFFFC,
	0xFFFFFFFE
    };
#else		/* LSBFirst */
/* NOTE:
the first element in starttab could be 0xffffffff.  making it 0
lets us deal with a full first word in the middle loop, rather
than having to do the multiple reads and masks that we'd
have to do if we thought it was partial.
*/
int starttab[32] = 
	{
	0x00000000,
	0xFFFFFFFE,
	0xFFFFFFFC,
	0xFFFFFFF8,
	0xFFFFFFF0,
	0xFFFFFFE0,
	0xFFFFFFC0,
	0xFFFFFF80,
	0xFFFFFF00,
	0xFFFFFE00,
	0xFFFFFC00,
	0xFFFFF800,
	0xFFFFF000,
	0xFFFFE000,
	0xFFFFC000,
	0xFFFF8000,
	0xFFFF0000,
	0xFFFE0000,
	0xFFFC0000,
	0xFFF80000,
	0xFFF00000,
	0xFFE00000,
	0xFFC00000,
	0xFF800000,
	0xFF000000,
	0xFE000000,
	0xFC000000,
	0xF8000000,
	0xF0000000,
	0xE0000000,
	0xC0000000,
	0x80000000
	};

int endtab[32] = 
	{
	0x00000000,
	0x00000001,
	0x00000003,
	0x00000007,
	0x0000000F,
	0x0000001F,
	0x0000003F,
	0x0000007F,
	0x000000FF,
	0x000001FF,
	0x000003FF,
	0x000007FF,
	0x00000FFF,
	0x00001FFF,
	0x00003FFF,
	0x00007FFF,
	0x0000FFFF,
	0x0001FFFF,
	0x0003FFFF,
	0x0007FFFF,
	0x000FFFFF,
	0x001FFFFF,
	0x003FFFFF,
	0x007FFFFF,
	0x00FFFFFF,
	0x01FFFFFF,
	0x03FFFFFF,
	0x07FFFFFF,
	0x0FFFFFFF,
	0x1FFFFFFF,
	0x3FFFFFFF,
	0x7FFFFFFF
	};

/* a hack, for now, since the entries for 0 need to be all
   1 bits, not all zeros.
   this means the code DOES NOT WORK for segments of length
   0 (which is only a problem in the horizontal line code.)
*/
int startpartial[32] = 
	{
	0xFFFFFFFF,
	0xFFFFFFFE,
	0xFFFFFFFC,
	0xFFFFFFF8,
	0xFFFFFFF0,
	0xFFFFFFE0,
	0xFFFFFFC0,
	0xFFFFFF80,
	0xFFFFFF00,
	0xFFFFFE00,
	0xFFFFFC00,
	0xFFFFF800,
	0xFFFFF000,
	0xFFFFE000,
	0xFFFFC000,
	0xFFFF8000,
	0xFFFF0000,
	0xFFFE0000,
	0xFFFC0000,
	0xFFF80000,
	0xFFF00000,
	0xFFE00000,
	0xFFC00000,
	0xFF800000,
	0xFF000000,
	0xFE000000,
	0xFC000000,
	0xF8000000,
	0xF0000000,
	0xE0000000,
	0xC0000000,
	0x80000000
	};

int endpartial[32] = 
	{
	0xFFFFFFFF,
	0x00000001,
	0x00000003,
	0x00000007,
	0x0000000F,
	0x0000001F,
	0x0000003F,
	0x0000007F,
	0x000000FF,
	0x000001FF,
	0x000003FF,
	0x000007FF,
	0x00000FFF,
	0x00001FFF,
	0x00003FFF,
	0x00007FFF,
	0x0000FFFF,
	0x0001FFFF,
	0x0003FFFF,
	0x0007FFFF,
	0x000FFFFF,
	0x001FFFFF,
	0x003FFFFF,
	0x007FFFFF,
	0x00FFFFFF,
	0x01FFFFFF,
	0x03FFFFFF,
	0x07FFFFFF,
	0x0FFFFFFF,
	0x1FFFFFFF,
	0x3FFFFFFF,
	0x7FFFFFFF
	};
#endif


/* used for masking bits in bresenham lines
   mask[n] is used to mask out all but bit n in a longword (n is a
screen position).
   rmask[n] is used to mask out the single bit at position n (n
is a screen posiotion.)
*/

#if (BITMAP_BIT_ORDER == MSBFirst)
int mask[] =
    {
    1<<31, 1<<30, 1<<29, 1<<28, 1<<27, 1<<26, 1<<25, 1<<24,
    1<<23, 1<<22, 1<<21, 1<<20, 1<<19, 1<<18, 1<<17, 1<<16,
    1<<15, 1<<14, 1<<13, 1<<12, 1<<11, 1<<10, 1<<9, 1<<8,
    1<<7, 1<<6, 1<<5, 1<<4, 1<<3, 1<<2, 1<<1, 1<<0
    }; 
int rmask[] = 
    {
    0xffffffff ^ (1<<31), 0xffffffff ^ (1<<30), 0xffffffff ^ (1<<29),
    0xffffffff ^ (1<<28), 0xffffffff ^ (1<<27), 0xffffffff ^ (1<<26),
    0xffffffff ^ (1<<25), 0xffffffff ^ (1<<24), 0xffffffff ^ (1<<23),
    0xffffffff ^ (1<<22), 0xffffffff ^ (1<<21), 0xffffffff ^ (1<<20),
    0xffffffff ^ (1<<19), 0xffffffff ^ (1<<18), 0xffffffff ^ (1<<17),
    0xffffffff ^ (1<<16), 0xffffffff ^ (1<<15), 0xffffffff ^ (1<<14),
    0xffffffff ^ (1<<13), 0xffffffff ^ (1<<12), 0xffffffff ^ (1<<11),
    0xffffffff ^ (1<<10), 0xffffffff ^ (1<<9),  0xffffffff ^ (1<<8),
    0xffffffff ^ (1<<7),  0xffffffff ^ (1<<6),  0xffffffff ^ (1<<5),
    0xffffffff ^ (1<<4),  0xffffffff ^ (1<<3),  0xffffffff ^ (1<<2),
    0xffffffff ^ (1<<1),  0xffffffff ^ (1<<0)
    };
#else	/* LSBFirst */
int mask[] =
    {
    1<<0, 1<<1, 1<<2, 1<<3, 1<<4, 1<<5, 1<<6, 1<<7,
    1<<8, 1<<9, 1<<10, 1<<11, 1<<12, 1<<13, 1<<14, 1<<15,
    1<<16, 1<<17, 1<<18, 1<<19, 1<<20, 1<<21, 1<<22, 1<<23,
    1<<24, 1<<25, 1<<26, 1<<27, 1<<28, 1<<29, 1<<30, 1<<31
    }; 
int rmask[] = 
    {
    0xffffffff ^ (1<<0), 0xffffffff ^ (1<<1), 0xffffffff ^ (1<<2),
    0xffffffff ^ (1<<3), 0xffffffff ^ (1<<4), 0xffffffff ^ (1<<5),
    0xffffffff ^ (1<<6), 0xffffffff ^ (1<<7), 0xffffffff ^ (1<<8),
    0xffffffff ^ (1<<9), 0xffffffff ^ (1<<10), 0xffffffff ^ (1<<11),
    0xffffffff ^ (1<<12), 0xffffffff ^ (1<<13), 0xffffffff ^ (1<<14),
    0xffffffff ^ (1<<15), 0xffffffff ^ (1<<16), 0xffffffff ^ (1<<17),
    0xffffffff ^ (1<<18), 0xffffffff ^ (1<<19), 0xffffffff ^ (1<<20),
    0xffffffff ^ (1<<21), 0xffffffff ^ (1<<22), 0xffffffff ^ (1<<23),
    0xffffffff ^ (1<<24), 0xffffffff ^ (1<<25), 0xffffffff ^ (1<<26),
    0xffffffff ^ (1<<27), 0xffffffff ^ (1<<28), 0xffffffff ^ (1<<29),
    0xffffffff ^ (1<<30), 0xffffffff ^ (1<<31)
    };
#endif

