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

/* $Header: aedio.c,v 1.1 87/09/13 03:34:58 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/aed/RCS/aedio.c,v $ */

#ifndef lint
static char *rcsid = "$Header: aedio.c,v 1.1 87/09/13 03:34:58 erik Exp $";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <machinecons/xio.h>

#include "X.h"
#include "Xproto.h"
#include "screenint.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "pixmapstr.h"
#include "miscstruct.h"
#include "input.h"
#include "colormapst.h"
#include "resource.h"

#include "mfb.h"

#include "rtcolor.h"
#include "rtcursor.h"
#include "rtinit.h"
#include "rtio.h"
#include "rtutils.h"
#include "xaed.h"

extern	void	miRecolorCursor();
void aedResolveColor();
extern ColormapPtr pColormap;

xColorItem		screenWhite, screenBlack;

Bool	(*screenInitProcs[NUMSCREENS])() = {
	{aedScreenInit}
};

Bool	(*screenCloseProcs[NUMSCREENS])() = {
	{aedScreenClose}
};

char	*mouse_device= MOUSE_DEVICE;



static Bool
aedSaveScreen(pScreen, on)
    ScreenPtr pScreen;
    int on;
{

    TRACE(("aedSaveScreen( pScreen= 0x%x, on= %d )\n",pScreen,on));

    if (on == SCREEN_SAVER_FORCER)
    {
        lastEventTime = GetTimeInMillis();	
	return TRUE;
    }
    else
        return FALSE;
    return TRUE;
}

Bool
aedScreenInit(index, pScreen, argc, argv)
    int index;
    ScreenPtr pScreen;
    int argc;		/* these two may NOT be changed */
    char **argv;
{
    Bool retval;
    static int been_here;
    int	 in_emulator= E_XINPUT;
    int	 out_emulator= E_AED;
    int  code;
    int  i, len;
    xColorItem def;
    ColormapPtr cmap;

    TRACE(("aedScreenInit( index= %d, pScreen= 0x%x, argc= %d, argv= 0x%x )\n",index,pScreen,argc,argv));

    if (!been_here) {
	if ((code = open(MICROCODE, O_RDONLY)) < 0)
	{
	ErrorF("aedScreenInit: unable to open microcode file:%s\n",MICROCODE);
	return FALSE;
	}
	
	if ((rtScreenFD= open(AED_SCREEN_DEVICE, O_RDWR | O_NDELAY, 0)) <  0)
	{
            ErrorF(  "couldn't open aed\n");
            return FALSE; 
	} 
	if ( ioctl(rtScreenFD,EISETD,&in_emulator) < 0 )
	{
	    ErrorF("aedScreenInit: error setting input emulator");
	    return FALSE;
	}
	if ( ioctl(rtScreenFD,EOSETD,&out_emulator) < 0 )
	{
	    ErrorF("aedScreenInit: error setting output emulator");
	    return FALSE;
	}
	ioctl(rtScreenFD,QIOCADDR,&rtXaddr);
	rtQueue= (XEventQueue *)(&rtXaddr->ibuff);

	if (open ("/dev/bus", O_RDONLY|O_NDELAY) < 0) {
	    ErrorF("Unable to open /dev/bus\n");
	    return FALSE;
	}
	

	/* ok, get going already */
	i = *((short *) VIKROOT);  /* reset viking */

	do { len = read(code,vikint,4096);	/* read from code file */
	     write(rtScreenFD,vikint,4096);		/* load microcode */
	   } while (len == 4096);
	close(code);

	i = *((short *) VIKROOT);  /* reset viking again */

	*((short *)VIKROOT + 0x4002) = 0; 	/* reset command */
	*semaphore = 0xffff;	/* set to go */
	i = *semaphore;		/* start running viking */
	out_emulator = E_XOUTPUT;
	if ( ioctl(rtScreenFD,EOSETD,&out_emulator) < 0 )
	{
	    ErrorF("aedScreenInit: error setting output emulator");
	    return FALSE;
	}
	been_here= TRUE;
    }

    retval = aedInitScreen(index, pScreen, 0x80000000, 
				AED_SCREEN_WIDTH, AED_SCREEN_HEIGHT, 80, 80);
    aedCursorInit();
    pScreen->CloseScreen=	aedScreenClose;
    pScreen->SaveScreen=	aedSaveScreen;
    pScreen->RealizeCursor=	aedRealizeCursor;
    pScreen->UnrealizeCursor=	aedUnrealizeCursor;
    pScreen->DisplayCursor=	aedDisplayCursor;
    pScreen->SetCursorPosition=	rtSetCursorPosition;
    pScreen->CursorLimits=	rtCursorLimits;
    pScreen->PointerNonInterestBox= rtPointerNonInterestBox;
    pScreen->ConstrainCursor=	rtConstrainCursor;
    pScreen->RecolorCursor=	miRecolorCursor;
    pScreen->QueryBestSize=	rtQueryBestSize;
    pScreen->CreateGC=		aedCreateGC;
    pScreen->CreateWindow=	aedCreateWindow;
    pScreen->ChangeWindowAttributes=	aedChangeWindowAttributes;
    pScreen->ResolveColor = 	rtResolveColorMono;
    pScreen->CreateColormap = 	rtCreateColormapMono;
    pScreen->DestroyColormap = 	rtDestroyColormapMono;

    CreateColormap(pScreen->defColormap, pScreen,
		   LookupID(pScreen->rootVisual, RT_VISUALID, RC_CORE),
		   &cmap, AllocNone, 0);
    mfbInstallColormap(cmap);

    return(retval);
}

aedScreenClose(index, pScreen)
int	index;
ScreenPtr	pScreen;
{
extern	int	errno;

    TRACE(("aedScreenClose( index= %d, pScreen= 0x%x )\n",index,pScreen));

/*    if (close(rtScreenFD)) {
	ErrorF("Closing aed yielded %d\n",errno);
	return(FALSE);
    }*/
    return(TRUE);
}


