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
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <machinecons/xio.h>

#define NEED_EVENTS

#include "X.h"
#include "Xproto.h"
#include "input.h"
#include "miscstruct.h"

#include "rtcursor.h"
#include "rtkeyboard.h"
#include "rtmouse.h"
#include "rtutils.h"

XIoAddr		*rtXaddr;
XEventQueue	*rtQueue;
int		lastEventTime;
int		 rtScreenFD;

extern	int	screenIsSaved;

/*****************
 * ProcessInputEvents:
 *    processes all the pending input events
 *****************/

void
ProcessInputEvents()
{
    register int    i;
    register    XEvent * pE;
    xEvent	e;
    int     nowInCentiSecs, nowInMilliSecs, adjustCentiSecs;
    struct timeval  tp;
    int     needTime = 1;
#ifdef RT_SPECIAL_MALLOC
    extern int rtShouldDumpArena;
#endif /* RT_SPECIAL_MALLOC */

/*    TRACE(("ProcessInputEvents()\n"));*/

#ifdef RT_SPECIAL_MALLOC
    if (rtShouldDumpArena) {
       rtDumpArena();
    }
#endif /* RT_SPECIAL_MALLOC */

    i = rtQueue->head;
    while (i != rtQueue->tail)
    {
	if (screenIsSaved == SCREEN_SAVER_ON )
	    SaveScreens(SCREEN_SAVER_OFF, ScreenSaverReset);
	pE = &rtQueue->events[i];
	e.u.keyButtonPointer.rootX = pE->xe_x;
	e.u.keyButtonPointer.rootY = pE->xe_y;

	if (rtSoftCursor&&pE->xe_device==XE_MOUSE) {
	    if ((pE->xe_x<rtCursorBounds.x1)||(pE->xe_x>rtCursorBounds.x2)||
		 (pE->xe_y<rtCursorBounds.y1)||(pE->xe_y>rtCursorBounds.y2)) {
		(*rtCursorShow)(&pE->xe_x,&pE->xe_y);
	    }
	    else {
		(*rtCursorX)= (pE->xe_x-rtCursorHotX);
		(*rtCursorY)= (pE->xe_y-rtCursorHotY);
	    }
	}
    /* 
     * The following silly looking code is because the old version of the
     * driver only delivers 16 bits worth of centiseconds. We are supposed
     * to be keeping time in terms of 32 bits of milliseconds.
     */
	if (needTime)
	{
	    needTime = 0;
	    gettimeofday(&tp, 0);
	    nowInCentiSecs = ((tp.tv_sec * 100) + (tp.tv_usec / 10000)) & 0xFFFF;
	/* same as driver */
	    nowInMilliSecs = (tp.tv_sec * 1000) + (tp.tv_usec / 1000);
	/* beware overflow */
	}
	if ((adjustCentiSecs = nowInCentiSecs - pE->xe_time) < -20000)
	    adjustCentiSecs += 0x10000;
	else
	    if (adjustCentiSecs > 20000)
		adjustCentiSecs -= 0x10000;
	e.u.keyButtonPointer.time = lastEventTime = 
	    nowInMilliSecs - adjustCentiSecs * 10;

	if ((pE->xe_type == XE_BUTTON)&&(pE->xe_device==XE_DKB))
	{
	    e.u.u.detail= pE->xe_key;
	    switch (pE->xe_direction) {
		case XE_KBTDOWN:
			e.u.u.type= KeyPress;
			(rtKeybd->processInputProc)(&e,rtKeybd);
			break;
		case XE_KBTUP:
			e.u.u.type= KeyRelease;
			(rtKeybd->processInputProc)(&e,rtKeybd);
			break;
		default:	/* hopefully BUTTON_RAW_TYPE */
			ErrorF("got a raw button, what do I do?\n");
			break;
	    }
	}
	else if ((pE->xe_device==XE_MOUSE)||(pE->xe_device==XE_TABLET))
	{
	    if (pE->xe_type == XE_BUTTON )
	    {
		if (pE->xe_direction == XE_KBTDOWN)
		    e.u.u.type= ButtonPress;
		else
		    e.u.u.type= ButtonRelease;
		/* mouse buttons numbered from one */
		e.u.u.detail = pE->xe_key+1;
	    }
	    else
		e.u.u.type = MotionNotify;
	    (*rtPtr->processInputProc)(&e,rtPtr);
	}
	if (i == rtQueue->size)
	    i = rtQueue->head = 0;
	else
	    i = ++rtQueue->head;
    }
}

TimeSinceLastInputEvent()
{
/*    TRACE(("TimeSinceLastInputEvent()\n"));*/

    if (lastEventTime == 0)
	lastEventTime = GetTimeInMillis();
    return GetTimeInMillis() - lastEventTime;
}

void
rtQueryBestSize(class, pwidth, pheight)
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
	  if (width > 0) {
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
	  }
	  /* We don't care what height they use */
	  break;
    }
}

