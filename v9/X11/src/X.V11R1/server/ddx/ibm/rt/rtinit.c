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
/* $Header: rtinit.c,v 1.7 87/09/13 03:29:26 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/rt/RCS/rtinit.c,v $ */

#ifndef lint
static char *rcsid = "$Header: rtinit.c,v 1.7 87/09/13 03:29:26 erik Exp $";
#endif


/* (ef) 4/11/87 -- to get definition of xEvent from Xproto.h */
#define NEED_EVENTS

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "input.h"
#include "scrnintstr.h"

#include "rtinit.h"
#include "rtkeyboard.h"
#include "rtmouse.h"
#include "rtutils.h"

#include "servermd.h"

#define MOTION_BUFFER_SIZE 0
#define NUMFORMATS 1
#define NUMDEVICES 2

extern	apa16ScreenInit();

static PixmapFormatRec	formats[] = {{1, 1, BITMAP_SCANLINE_PAD}};

int
InitOutput(screenInfo, argc, argv)
    ScreenInfo	*screenInfo;
    int		 argc;
    char	*argv[];
{
    int i;

    TRACE(("InitOutput( screenInfo= 0x%x)\n",screenInfo));

    rtProcessCommandLine(argc,argv);
    screenInfo->imageByteOrder = IMAGE_BYTE_ORDER;
    screenInfo->bitmapScanlineUnit = BITMAP_SCANLINE_UNIT;
    screenInfo->bitmapScanlinePad = BITMAP_SCANLINE_PAD;
    screenInfo->bitmapBitOrder = BITMAP_BIT_ORDER;

    screenInfo->numPixmapFormats = NUMFORMATS;
    for (i=0;i<NUMFORMATS;i++) {
	screenInfo->formats[i].depth = formats[i].depth;
	screenInfo->formats[i].bitsPerPixel = formats[i].bitsPerPixel;
	screenInfo->formats[i].scanlinePad = formats[i].scanlinePad;
    }

    AddScreen(apa16ScreenInit, argc,argv);
}

static DevicePtr keyboard;
static DevicePtr mouse;

InitInput()
{
    TRACE(("InitInput()\n"));

    mouse=	AddInputDevice(rtMouseProc,	TRUE);
    keyboard=	AddInputDevice(rtKeybdProc,	TRUE);

    RegisterPointerDevice( mouse, MOTION_BUFFER_SIZE );
    RegisterKeyboardDevice( keyboard );
}


int
rtProcessCommandLine(argc,argv)
int	argc;
char	*argv[];
{
int	i;
char	*keybd;
extern	char *getenv();
extern	char *rtArenaFile;

    TRACE(("rtProcessCommandLine( argc= %d, argv= 0x%x )\n",argc,argv));

    keybd= getenv("X11_KEYBOARD");

    for (i=1;i<argc;i++) {
	if	( strcmp( argv[i], "-pckeys" ) == 0 )	keybd= "pckeys";
	else if ( strcmp( argv[i], "-rtkeys" ) == 0 )	keybd= "rtkeys"; 
	else if ( strcmp( argv[i], "-trace"  ) == 0 )	rtTrace= TRUE;
#ifdef RT_SPECIAL_MALLOC
	else if ( strcmp( argv[i], "-malloc" ) == 0 )	{
		int lvl= atoi(argv[++i]);
		SetMallocCheckLevel(lvl);
		ErrorF("allocator check level set to %d...\n",lvl);
	}
	else if ( strcmp( argv[i], "-plumber" ) == 0 ) {
		rtSetupPlumber(argv[++i]);
	}
#endif RT_SPECIAL_MALLOC
    }

    if (keybd) {
	if	(strcmp(keybd,"pckeys")==0)	rtUsePCKeyboard();
	else if	(strcmp(keybd,"rtkeys")==0)	rtUseRTKeyboard();
    }
}



