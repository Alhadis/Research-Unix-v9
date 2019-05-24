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
#include "X.h"
#include "pixmapstr.h"
#include "regionstr.h"
#include "xaed.h"


unsigned short	vikint[2048];
unsigned short 	*semaphore = (unsigned short *) (VIKROOT + 0x4000);
int		vikoff = 0;     /* offset into viking buffer */
int 		mergexlate[16] = {0,8,4,12,2,10,6,14,1,9,5,13,3,11,7,15};


vikwait()
{
int i, poll;
i=0;
while(*semaphore)
	{
	for(poll = 1000; poll != 0; poll--); /* do nothing */
		/* we need a sleep of microseconds for this */
	if (i++ >= 1000)
		{
		ErrorF("vikwait: Semaphore apparently hung.\n");
		return(-1);
		}
	}
return(i);
}


command(nwords)
int nwords;		   /* number of words to transfer */
{
if ( nwords >= 2048 )
    ErrorF("WARNING: vikoff > 2048");
vikwait();
VWRITE(&vikint[1],nwords,0x4002); 
*semaphore = 0x0101;
}


vforce()
{
if (vikoff>ORDATA)
	{
	vikint[ORLEN] = vikoff-ORDATA;
	command(vikoff-1);
	vikoff = 0;
	}
vikoff = 0;
}


clear(n)
int n;
{
    /* if new order will not fit in buffer, transmit it */
    if ((vikoff+n) > 2047)  vforce();

    /* if there are no orders already in buffer, initialize it */
    if (vikoff == 0)
       {
	vikint[VIKCMD] = 1; /* process orders */

	/* set parameters */
	vikint[ORMERGE]		= 12;		    /* merge mode */
	vikint[ORFONTID] 	= 0;		    /* font id */
	vikint[ORWIDTH] 	= 1;		    /* line width */
	vikint[ORDASHPAT] 	= 0;		    /* line dash pattern */
	vikint[ORDASHLEN] 	= 0;		    /* line pattern length */
	vikint[ORXPOSN] 	= 0;		    /* current x position */
	vikint[ORYPOSN] 	= 0;		    /* current y position */
	vikint[ORXORG] 		= 0;		    /* window origin */
	vikint[ORYORG] 		= 0;		    /* window origin */
	vikint[ORCLIPLX] 	= 0;
	vikint[ORCLIPLY] 	= 0;
	vikint[ORCLIPHX] 	= 1023;	
	vikint[ORCLIPHY] 	= 799;		    /* clip high y */
	vikint[ORCOLOR] 	= 0;		    /* window color */

	vikint[ORLEN] 		= 0;   /* will get set to actual length */

	/* start placing orders */
	vikoff = ORDATA;
      }
  }
