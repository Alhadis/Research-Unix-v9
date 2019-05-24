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
/* $Header: apa16hdwr.c,v 5.2 87/09/13 03:16:52 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/apa16/RCS/apa16hdwr.c,v $ */

#ifndef lint
static char *rcsid = "$Header: apa16hdwr.c,v 5.2 87/09/13 03:16:52 erik Exp $";
#endif

#include "Xmd.h"

#include "apa16hdwr.h"
	/*
	 * variables and functions to manipulate the apa16
	 * rasterop hardware
	 */

int	apa16_qoffset;

#ifdef NOT_X
#define ErrorF printf
#endif

/***============================================================***/

unsigned short apa16_rop2stype[16] = {
	/* GXclear */		0,
	/* GXand */		1,
	/* GXandReverse */	2,
	/* GXcopy */		9,
	/* GXandInverted */	8,
	/* GXnoop */		0,
	/* GXxor */		0xa,
	/* GXor */		0xb,
	/* GXnor */		0,
	/* GXequiv */		0,
	/* GXinvert */		0,
	/* GXorReverse */	0,
	/* GXcopyInverted */	0,
	/* GXorInverted */	0xc,
	/* GXnand */		0xe,
	/* GXSet */		0};

/***============================================================***/

   /*
    * Prints "knt" instructions on the apa16 rasterop processing queue.
    */

apa16_queue_check(knt)
unsigned knt;
{
CARD16 *tmp= QUEUE_BASE+apa16_qoffset;
CARD16 *qtmp= QPTR_TO_SPTR(QUEUE_PTR);

ErrorF("qtmp= 0x%x\n",qtmp);
   ErrorF("hardware queue pointer= 0x%04x\n",QUEUE_PTR);
   ErrorF("software queue pointer= 0x%04x\n",SPTR_TO_QPTR(tmp));
   ErrorF("base= 0x%04x, top= 0x%04x\n",SPTR_TO_QPTR(QUEUE_BASE),
					SPTR_TO_QPTR(QUEUE_TOP));
   ErrorF("queue counter= 0x%x\n",QUEUE_CNTR);
   ErrorF("software queue	hardware queue\n");
   if (tmp!=QUEUE_TOP) tmp+= knt;
   if (qtmp!=QUEUE_TOP) qtmp+= knt;
   while (knt--) {
      ErrorF("0x%04x= 0x%04x	",SPTR_TO_QPTR(tmp),*tmp);
      ErrorF("0x%04x= 0x%04x\n",SPTR_TO_QPTR(qtmp),*qtmp);
      tmp--;
      qtmp--;
   }
}

