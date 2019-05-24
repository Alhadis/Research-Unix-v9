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

/* $Header: WaitFor.c,v 1.25 87/09/13 20:31:42 sun Exp $ */

/*****************************************************************
 * OS Depedent input routines:
 *
 *  WaitForSomething,  GetEvent
 *
 *****************************************************************/

#include <errno.h>
#include <stdio.h>
#include "X.h"
#include "misc.h"
#include <sys/time.h>
#include <sys/param.h>
#include <signal.h>
#include "osdep.h"
#include "dixstruct.h"

extern long AllSockets[];
extern long AllClients[];
extern long LastSelectMask[];
extern long WellKnownConnections;
extern long EnabledDevices;
extern long ClientsWithInput[];

extern long ScreenSaverTime;               /* milliseconds */
extern long ScreenSaverInterval;               /* milliseconds */
extern ClientPtr ConnectionTranslation[];

extern Bool clientsDoomed;

extern void CheckConnections();
extern int FirstClient;
extern int MaxClients;

extern int errno;

int isItTimeToYield = 1;

/*****************
 * WaitForSomething:
 *     Make the server suspend until there is data from clients or
 *     input or ddx notices something of interest (graphics
 *     queue ready, etc.)  If the time between INPUT events is
 *     greater than ScreenSaverTime, the display is turned off (or
 *     saved, depending on the hardware).  So, WaitForSomething()
 *     has to handle this also (that's why the select() has a timeout.
 *     For more info on ClientsWithInput, see ReadRequestFromClient().
 *     pClientsReady is a mask, the bits set are 
 *     indices into the o.s. depedent table of available clients.
 *     (In this case, there is no table -- the index is the socket
 *     file descriptor.)  
 *****************/

static int intervalCount = 0;

WaitForSomething(pClientsReady, nready, pNewClients, nnew)
    ClientPtr *pClientsReady;
    int *nready;
    ClientPtr *pNewClients;
    int *nnew;
{
    int i;
    struct timeval waittime, *wt;
    long timeout;
    long readyClients[mskcnt];
    long curclient;
    int selecterr;

    *nready = 0;
    *nnew = 0;
    CLEARBITS(readyClients);
    if (! (ANYSET(ClientsWithInput)))
    {
	/* We need a while loop here to handle 
	   crashed connections and the screen saver timeout */
	while (1)
        {
            if (ScreenSaverTime)
	    {
                timeout = ScreenSaverTime - TimeSinceLastInputEvent();
	        if (timeout < 0) /* may be forced by AutoResetServer() */
	        {
		    if (clientsDoomed)
		    {
		        *nnew = *nready = 0;
			break;
		    }
	            if (timeout < intervalCount)
                    {
		        SaveScreens(SCREEN_SAVER_ON, ScreenSaverActive);
		        if (intervalCount)
    		            intervalCount -= ScreenSaverInterval;
                        else
                            intervalCount = 
					-(ScreenSaverInterval + ScreenSaverTime);
		    }
    	            timeout -= intervalCount;
    	        }
                else
	            intervalCount = 0;
                waittime.tv_sec = timeout / MILLI_PER_SECOND;
	        waittime.tv_usec = 0;
		wt = &waittime;
	    }
            else
                wt = NULL;
	    COPYBITS(AllSockets, LastSelectMask);
	    BlockHandler(&wt, LastSelectMask);
	    i = select (MAXSOCKS, LastSelectMask, 
			(int *) NULL, (int *) NULL, wt);
	    selecterr = errno;
	    WakeupHandler(i, LastSelectMask);
	    if (i <= 0) /* An error or timeout occurred */
            {
		if (i < 0) 
		    if (selecterr == EBADF)    /* Some client disconnected */
	            	CheckConnections ();
		    else if (selecterr != EINTR)
			ErrorF("WaitForSomething(): select: errno=%d\n",
			    selecterr);
    	    }
	    else
	    {
		MASKANDSETBITS(readyClients, LastSelectMask, AllClients); 
		if (LastSelectMask[0] & WellKnownConnections) 
		   EstablishNewConnections(pNewClients, nnew);
		if (*nnew || (LastSelectMask[0] & EnabledDevices) 
		    || (ANYSET (readyClients)))
			    break;
	    }
	}
    }
    else
    {
       COPYBITS(ClientsWithInput, readyClients);
    }

    if (ANYSET(readyClients))
    {
	for (i=0; i<mskcnt; i++)
	{
	    while (readyClients[i])
	    {
		curclient = ffs (readyClients[i]) - 1;
		pClientsReady[(*nready)++] = 
			ConnectionTranslation[curclient + (32 * i)];
		readyClients[i] &= ~(1 << curclient);
	    }
	}	
    }
}



