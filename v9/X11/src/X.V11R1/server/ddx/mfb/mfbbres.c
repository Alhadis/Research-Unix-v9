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
/* $Header: mfbbres.c,v 1.11 87/09/11 07:48:29 toddb Exp $ */
#include "X.h"
#include "misc.h"
#include "mfb.h"
#include "maskbits.h"

/* Solid bresenham line */
/* NOTES
   e2 is used less often than e1, so it's not in a register
*/

mfbBresS(rop, addrl, nlwidth, signdx, signdy, axis, x1, y1, e, e1, e2, len)
int rop;		/* a reduced rasterop */
register int *addrl;		/* pointer to base of bitmap */
int nlwidth;		/* width in longwords of bitmap */
int signdx, signdy;	/* signs of directions */
int axis;		/* major axis (Y_AXIS or X_AXIS) */
int x1, y1;		/* initial point */
register int e;		/* error accumulator */
register int e1;	/* bresenham increments */
int e2;
register int len;	/* length of line */
{

    register int yinc;	/* increment to next scanline */
    register int addrb;		/* bitmask */

    /* point to longword containing first point */
    addrl = addrl + (y1 * nlwidth) + (x1 >> 5);
    addrb = x1&0x1f;
    yinc = signdy * nlwidth;

    if (rop == RROP_BLACK)
    {
        if (axis == X_AXIS)
        {
	    if (signdx > 0)
	    {
	        while(len--)
	        {
		    *addrl &= rmask[addrb];
		    if (e < 0)
		        e += e1;
		    else
		    {
		        addrl += yinc;
		        e += e2;
		    }
		    if (addrb == 31)
		    {
		        addrb = -1;
		        addrl++;
		    }
		    addrb++;
	        }
	    }
	    else
	    {
	        while(len--)
	        {
		    *addrl &= rmask[addrb];
		    if (e <= 0)
		        e += e1;
		    else
		    {
		        addrl += yinc;
		        e += e2;
		    }
		    if (addrb == 0)
		    {
		        addrb = 32;
		        addrl--;
		    }
		    addrb--;
	        }
	    }
        } /* if X_AXIS */
        else
        {
	    if (signdx > 0)
	    {
	        while(len--)
	        {
		    *addrl &= rmask[addrb];
		    if (e < 0)
		        e += e1;
		    else
		    {
		        if (addrb == 31)
		        {
			    addrb = -1;
			    addrl++;
		        }
		        addrb++;
		        e += e2;
		    }
		    addrl += yinc;
	        }
	    }
	    else
	    {
	        while(len--)
	        {
		    *addrl &= rmask[addrb];
		    if (e <= 0)
		        e += e1;
		    else
		    {
		        if (addrb == 0)
		        {
			    addrb = 32;
			    addrl--;
		        }
		        addrb--;
		        e += e2;
		    }
		    addrl += yinc;
	        }
	    }
        } /* else Y_AXIS */
    } 
    else if (rop == RROP_WHITE)
    {
        if (axis == X_AXIS)
        {
	    if (signdx > 0)
	    {
	        while(len--)
	        {
		    *addrl |= mask[addrb];
		    if (e < 0)
		        e += e1;
		    else
		    {
		        addrl += yinc;
		        e += e2;
		    }
		    if (addrb == 31)
		    {
		        addrb = -1;
		        addrl++;
		    }
		    addrb++;
	        }
	    }
	    else
	    {
	        while(len--)
	        {
		    *addrl |= mask[addrb];
		    if (e <= 0)
		        e += e1;
		    else
		    {
		        addrl += yinc;
		        e += e2;
		    }
		    if (addrb == 0)
		    {
		        addrb = 32;
		        addrl--;
		    }
		    addrb--;
	        }
	    }
        } /* if X_AXIS */
        else
        {
	    if (signdx > 0)
	    {
	        while(len--)
	        {
		    *addrl |= mask[addrb];
		    if (e < 0)
		        e += e1;
		    else
		    {
		        if (addrb == 31)
		        {
			    addrb = -1;
			    addrl++;
		        }
		        addrb++;
		        e += e2;
		    }
		    addrl += yinc;
	        }
	    }
	    else
	    {
	        while(len--)
	        {
		    *addrl |= mask[addrb];
		    if (e <= 0)
		        e += e1;
		    else
		    {
		        if (addrb == 0)
		        {
			    addrb = 32;
			    addrl--;
		        }
		        addrb--;
		        e += e2;
		    }
		    addrl += yinc;
	        }
	    }
        } /* else Y_AXIS */
    }
    else if (rop == RROP_INVERT)
    {
        if (axis == X_AXIS)
        {
	    if (signdx > 0)
	    {
	        while(len--)
	        {
		    *addrl ^= mask[addrb];
		    if (e < 0)
		        e += e1;
		    else
		    {
		        addrl += yinc;
		        e += e2;
		    }
		    if (addrb == 31)
		    {
		        addrb = -1;
		        addrl++;
		    }
		    addrb++;
	        }
	    }
	    else
	    {
	        while(len--)
	        {
		    *addrl ^= mask[addrb];
		    if (e <= 0)
		        e += e1;
		    else
		    {
		        addrl += yinc;
		        e += e2;
		    }
		    if (addrb == 0)
		    {
		        addrb = 32;
		        addrl--;
		    }
		    addrb--;
	        }
	    }
        } /* if X_AXIS */
        else
        {
	    if (signdx > 0)
	    {
	        while(len--)
	        {
		    *addrl ^= mask[addrb];
		    if (e < 0)
		        e += e1;
		    else
		    {
		        if (addrb == 31)
		        {
			    addrb = -1;
			    addrl++;
		        }
		        addrb++;
		        e += e2;
		    }
		    addrl += yinc;
	        }
	    }
	    else
	    {
	        while(len--)
	        {
		    *addrl ^= mask[addrb];
		    if (e <= 0)
		        e += e1;
		    else
		    {
		        if (addrb == 0)
		        {
			    addrb = 32;
			    addrl--;
		        }
		        addrb--;
		        e += e2;
		    }
		    addrl += yinc;
	        }
	    }
        } /* else Y_AXIS */
    }
} 
