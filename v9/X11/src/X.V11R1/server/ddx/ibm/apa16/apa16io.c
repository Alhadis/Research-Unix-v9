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

/* $Header: apa16io.c,v 5.6 87/09/13 03:18:24 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/apa16/RCS/apa16io.c,v $ */

#ifndef lint
static char *rcsid = "$Header: apa16io.c,v 5.6 87/09/13 03:18:24 erik Exp $";
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
#include "apa16decls.h"
#include "apa16hdwr.h"

extern	void	miRecolorCursor();

xColorItem		screenWhite, screenBlack;

Bool	(*screenInitProcs[NUMSCREENS])() = {
	{apa16ScreenInit}
};

Bool	(*screenCloseProcs[NUMSCREENS])() = {
	{apa16ScreenClose}
};

char	*mouse_device= "/dev/msapa16";

int	use_hardware= TRUE;

static Bool
apa16SaveScreen(pScreen, on)
    ScreenPtr pScreen;
    int on;
{

    TRACE(("apa16SaveScreen( pScreen= 0x%x, on= %d )\n",pScreen,on));

    if (on == SCREEN_SAVER_FORCER)
    {
        lastEventTime = GetTimeInMillis();	
	return TRUE;
    }
    else
        return FALSE;
}

Bool
apa16ScreenInit(index, pScreen, argc, argv)
    int index;
    ScreenPtr pScreen;
    int argc;		/* these two may NOT be changed */
    char **argv;
{
    Bool retval;
    static int been_here;
    int	 emulator= E_XINPUT;
    ColormapPtr cmap;

    TRACE(("apa16ScreenInit( index= %d, pScreen= 0x%x, argc= %d, argv= 0x%x )\n",index,pScreen,argc,argv));

    if (!been_here) {
	if ((rtScreenFD= open("/dev/apa16", O_RDWR | O_NDELAY, 0)) <  0)
	{
            ErrorF(  "couldn't open apa16\n");
            return FALSE; 
	} 
	ioctl(rtScreenFD,EISETD,&emulator);
	ioctl(rtScreenFD,QIOCADDR,&rtXaddr);
	rtQueue= (XEventQueue *)(&rtXaddr->ibuff);

	if (open ("/dev/bus", O_RDONLY|O_NDELAY) < 0) {
	    ErrorF("Unable to open /dev/bus\n");
	    return FALSE;
	}
	been_here= TRUE;
    }

    retval = mfbScreenInit(index, pScreen, APA16_BASE, 
				APA16_WIDTH, APA16_HEIGHT, 80, 80);
    apa16CursorInit();
    pScreen->CloseScreen=	apa16ScreenClose;
    pScreen->SaveScreen=	apa16SaveScreen;
    pScreen->RealizeCursor=	apa16RealizeCursor;
    pScreen->UnrealizeCursor=	apa16UnrealizeCursor;
    pScreen->DisplayCursor=	apa16DisplayCursor;
    pScreen->SetCursorPosition=	rtSetCursorPosition;
    pScreen->CursorLimits=	rtCursorLimits;
    pScreen->PointerNonInterestBox= rtPointerNonInterestBox;
    pScreen->ConstrainCursor=	rtConstrainCursor;
    pScreen->RecolorCursor=	miRecolorCursor;
    pScreen->QueryBestSize=	rtQueryBestSize;
    pScreen->ResolveColor=	rtResolveColorMono;
    pScreen->CreateColormap=	rtCreateColormapMono;
    pScreen->DestroyColormap=	rtDestroyColormapMono;

    if (getenv("APA16_IGN_HDWR")) {
	ErrorF("Ignoring apa16 hardware...\n");
    } else {
	ErrorF("Using apa16 hardware...\n");
	pScreen->CreateGC=			apa16CreateGC;
    	pScreen->CreateWindow=			apa16CreateWindow;
    	pScreen->ChangeWindowAttributes=	apa16ChangeWindowAttributes;
    }

    CreateColormap(pScreen->defColormap, pScreen,
		   LookupID(pScreen->rootVisual, RT_VISUALID, RC_CORE),
		   &cmap, AllocNone, 0);
    mfbInstallColormap(cmap);

    QUEUE_INIT();
    WHITE_ON_BLACK();
    return(retval);
}

apa16ScreenClose(index, pScreen)
int	index;
ScreenPtr	pScreen;
{
extern	int	errno;

    TRACE(("apa16ScreenClose( index= %d, pScreen= 0x%x )\n",index,pScreen));

/*    if (close(rtScreenFD)) {
	ErrorF("Closing apa16 yielded %d\n",errno);
	return(FALSE);
    }*/
    return(TRUE);
}
