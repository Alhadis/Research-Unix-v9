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
/* $Header: rtutils.c,v 5.1 87/09/13 03:31:29 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/rt/RCS/rtutils.c,v $ */

#ifndef lint
static char *rcsid = "$Header: rtutils.c,v 5.1 87/09/13 03:31:29 erik Exp $";
#endif

#include <machinecons/qevent.h>

#include <rtutils.h>

int	rtTrace;

#ifdef NOT_X
#define ErrorF printf
#endif

#define PUT_BIT(d,b)   (((d)&(((unsigned)1)<<(b)))?ErrorF("*"):ErrorF("."))

print_pattern(width,height,data)
int	width,height;
char	*data;
{
char	*tmp=data;
int	row,bit;
unsigned data_byte;
int	bits_left;

    TRACE(("print_pattern( width= %d, height= %d, data= 0x%x )\n",
							width,height,data));

    for (row=0;row<height;row++) {
	ErrorF("0x");
	for (bit=0;bit<(width+7)/8;bit++) {
	   ErrorF("%02x",*tmp++);
	}
	ErrorF("\n");
    }
    for (row=0;row<height;row++) {
	for (bits_left=width;bits_left>0;bits_left-=8) {
	    data_byte= *data++;
	    for (bit=7;bit>=(bits_left>8?0:8-bits_left);bit--) {
		PUT_BIT(data_byte,bit);
	    }
	}
	ErrorF("\n");
    }
}

/***==================================================================***/

print_event(xE)
XEvent	*xE;
{

    TRACE(("print_event( xE= 0x%x )\n",xE));

    ErrorF("Event(%d,%d): ",xE->xe_x,xE->xe_y);
    switch (xE->xe_device) {
	case XE_MOUSE:		ErrorF("mouse "); break;
	case XE_DKB:		ErrorF("keyboard "); break;
	case XE_TABLET:		ErrorF("tablet "); break;
	case XE_AUX:		ErrorF("aux "); break;
	case XE_CONSOLE:	ErrorF("console "); break;
	default:		ErrorF("unknown(%d) ",xE->xe_device); break;
    }
    if (xE->xe_type==XE_BUTTON) {
	ErrorF("button ");
	if	(xE->xe_direction==XE_KBTUP)	ErrorF("up ");
	else if	(xE->xe_direction==XE_KBTDOWN)	ErrorF("down ");
	else if	(xE->xe_direction==XE_KBTRAW)	ErrorF("raw ");
	else 			ErrorF("unknown(%d) ",xE->xe_direction);
	ErrorF("(key= %d)",xE->xe_key);
    }
    else if (xE->xe_type==XE_MMOTION)	ErrorF("MMOTION");
    else if (xE->xe_type==XE_TMOTION)	ErrorF("TMOTION");
    ErrorF("\n");
}

#ifdef RT_SPECIAL_MALLOC
#include <stdio.h>
#include <signal.h>

int	rtShouldDumpArena= 0;
static	char	*rtArenaFile= 0;

static	
rtMarkDumpArena()
{
    rtShouldDumpArena= 1;
    return 0;
}


rtDumpArena()
{
FILE  *mfil;

   mfil= fopen(rtArenaFile,"a");
   if (!mfil) {
	ErrorF("Couldn't open %s to dump arena, ignored\n",rtArenaFile);
	return(0);
   }
   else {
	ErrorF("Dumping malloc arena to %s\n",rtArenaFile);
	plumber(mfil);
	fflush(mfil);
	fclose(mfil);
   }
   rtShouldDumpArena= 0;
   return(1);
}

rtNoteHit()
{
static int old= 4;

   ErrorF("received SIGTERM\n");
   old= SetMallocCheckLevel(old);
   return(1);
}

rtSetupPlumber(name)
char	*name;
{
struct sigvec sv;

    ErrorF("Setting up plumber to dump to %s\n",name);
    rtArenaFile= name;
    unlink(rtArenaFile);
    sv.sv_handler= rtMarkDumpArena;
    sv.sv_mask= 0;
    sv.sv_onstack= 0;
    sigvec(SIGBUS,&sv,NULL);
    sv.sv_handler= rtDumpArena;
    sigvec(SIGEMT,&sv,NULL);
    sv.sv_handler= rtNoteHit;
    sigvec(SIGTERM,&sv,NULL);
    return(1);
}
#endif /* RT_SPECIAL_MALLOC */
