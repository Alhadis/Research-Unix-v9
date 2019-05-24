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
/* $Header: rtmouse.c,v 5.2 87/09/13 03:30:56 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/rt/RCS/rtmouse.c,v $ */

#ifndef lint
static char *rcsid = "$Header: rtmouse.c,v 5.2 87/09/13 03:30:56 erik Exp $";
#endif

#include <stdio.h>
#define IBMRTPC

#include <sys/types.h>
#include <sys/file.h>
#include <sys/tbioctl.h>
#include <machineio/mouseio.h>
#include <machinecons/xio.h>

#include "X.h"
#include "Xproto.h"
#include "input.h"

#include "rtio.h"
#include "rtmouse.h"
#include "rtutils.h"

extern	char	*getenv();

DevicePtr	rtPtr;

/*
 * Initialize mouse 
 * -- copied from X version 10 for the RT, /usr/src/X/libibm/libsrc/mouse.c
 *    Brown Copyright?
 */

static
rtMouseInit()
{
	char *MouseType = getenv("MOUSETYPE");
	char *MouseName = getenv("MOUSENAME");
	struct sgttyb MouseSG;
	int ldisc, ioarg;
	int mdev = -1;

	TRACE(( "rtMouseInit()\n"));

	/*
	 * Get mouse device name
	 */

	if (!MouseName)
		MouseName = mouse_device;

	/*
	 * Open mouse 
	 */

	if ((mdev = open (MouseName, O_RDWR|O_NDELAY)) < 0) {
		fprintf  (stderr, "Error in open of (%s)\n", MouseName);
		fflush(stderr);
		exit (2);
	}

	/*
	 * Switch to mouse line discipline
	 */

	ldisc = TABLDISC;
	ioctl(mdev, TIOCSETD, (caddr_t) &ldisc);

	/*
	 * Check for Mouse Systems Mouse
	 */

	if (MouseType && (strcmp (MouseType, "MSCMOUSE") == 0)) {
		MouseSG.sg_ispeed = 9;
		MouseSG.sg_ospeed = 9;
		MouseSG.sg_erase = -1;
		MouseSG.sg_kill = -1;
		MouseSG.sg_flags = RAW | ANYP;
		ioctl(mdev, TIOCSETP, (caddr_t) &MouseSG);

		/*
		 * Set to MSCmouse emulation
		 */

		ldisc = PCMS_DISC;
		if (ioctl (mdev, TBIOSETD, (caddr_t) &ldisc) < 0)
		    ErrorF("Error in setting PCMS_DISC Line Discipline");

	} else {	/* Assume the Planar Mouse */

		/*
		 * Use 3 button emulation
		 */

		ldisc = PLANMS_DISC3;
		if (ioctl (mdev, TBIOSETD, (caddr_t) &ldisc) < 0)
		   ErrorF("Error in setting PLANMS_DISC3 Line Discipline");

		/*
		 * Set default mouse sample rate
		 */

		ioarg = MS_RATE_40;
		if (ioctl (mdev, MSIC_SAMP, (caddr_t) &ioarg) < 0)
			ErrorF("Error in setting mouse sample rate");

		/*
		 * Set default mouse resolution
		 */

		ioarg = MS_RES_200;
		if (ioctl (mdev, MSIC_RESL, (caddr_t) &ioarg) < 0)
			ErrorF("Error in setting mouse resolution");
	}
	 
}

/***================================================================***/

static void
rtChangePointerControl(pDevice)
DevicePtr	pDevice;
{

    TRACE(("rtChangePointerControl( pDevice= 0x%x )\n",pDevice));
}

static int
rtGetMotionEvents(buff, start, stop)
    CARD32 start, stop;
    xTimecoord *buff;
{
    TRACE(("rtGetMotionEvents( buff= 0x%x, start= %d, stop= %d )\n",
							buff,start,stop));
    return 0;
}

/***================================================================***/

int
rtMouseProc(pDev, onoff)
    DevicePtr	pDev;
    int onoff;
{
    static int been_here= FALSE;
    BYTE map[4];

    TRACE(("rtMouseProc( pDev= 0x%x, onoff= 0x%x )\n",pDev, onoff ));

    switch (onoff)
    {
    case DEVICE_INIT: 
	    rtPtr = pDev;
	    pDev->devicePrivate = (pointer) &rtQueue;
	    map[1] = 1;
	    map[2] = 2;
	    map[3] = 3;
	    if (!been_here) {
	       rtMouseInit();
	       been_here= TRUE;
	    }
	    InitPointerDeviceStruct(
		rtPtr, map, 3, rtGetMotionEvents, rtChangePointerControl );
	    SetInputCheck( &rtQueue->head, rtQueue->tail);
	    break;
    case DEVICE_ON:
	    pDev->on = TRUE;
/* (ef) 2/8/87 -- rtScreenFD should really be a file descriptor hung */
/*		off of devPrivate, so we can have one server for */
/*	      	multiple screens.				*/
	    AddEnabledDevice(rtScreenFD);
	    break;
    case DEVICE_OFF:
	pDev->on = FALSE;
/* (ef) 2/8/87 -- see comment above */
/*	RemoveEnabledDevice(rtScreenFD);*/
	break;
    case DEVICE_CLOSE:
	break;
    }
    return Success;
}

