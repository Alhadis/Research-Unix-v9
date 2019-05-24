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

#include "X.h"
#include "misc.h"
#include "cursor.h"
#include "screenint.h"

#include "cfb.h"

void cfbQueryBestSize(class, pwidth, pheight)
int class;
short *pwidth;
short *pheight;
{
    unsigned width, test;

    switch(class)
    {
      case CursorShape:
	  *pwidth = 16;
	  *pheight = 16;
	  break;
      case TileShape:
      case StippleShape:
	  width = *pwidth;
	  /* Return the closes power of two not less than what they gave me */
	  test = 0x80000000;
	  /* Find the highest 1 bit in the width given */
	  while(!(test & width))
	     test >>= 1;
	  /* If their number is greater than that, bump up to the next
	   *  power of two */
	  if((test - 1) & width)
	     test <<= 1;
	  *pwidth = test;
	  /* We don't care what height they use */
	  break;
    }
}

