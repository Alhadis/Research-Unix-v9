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
/* $Header: rtkeyboard.c,v 5.4 87/09/13 03:30:17 erik Exp $ */
/* $Source: /u1/X11/server/ddx/ibm/rt/RCS/rtkeyboard.c,v $ */

#ifndef lint
static char *rcsid = "$Header: rtkeyboard.c,v 5.4 87/09/13 03:30:17 erik Exp $";
#endif

#include <stdio.h>
#include <sys/file.h>
#include <sys/types.h>
#include <machinecons/xio.h>
#include <machineio/speakerio.h>

#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "keysym.h"

#include "rtio.h"
#include "rtkeymap.h"
#include "rtkeyboard.h"
#include "rtutils.h"

static	int		rtSpeakerFD= -1;
	DevicePtr	rtKeybd;
static	struct spk_blk	rtBellSetting= { 0,0,0,0 };  /* Be vewwwy quiet... */

/***============================================================***/

rtChangeLEDs(leds)
unsigned	leds;
{
    ioctl(rtScreenFD,((leds&RT_LED_NUMLOCK)?QIOCSETNUML:QIOCCLRNUML));
    ioctl(rtScreenFD,((leds&RT_LED_CAPSLOCK)?QIOCSETCAPSL:QIOCCLRCAPSL));
    ioctl(rtScreenFD,((leds&RT_LED_SCROLLOCK)?QIOCSETSCROLLL:QIOCCLRSCROLLL));
}

/***============================================================***/

	/*
	 * Magic code lifted from speaker(4) man page.
	 * I *couldn't* have made this up.
	 */

static void
rtSetBellPitch(freq)
int	freq;
{

    if (freq < 23) {
	rtBellSetting.freqhigh=0;
	rtBellSetting.freqlow=SPKOLOMIN;
    } else if (freq < 46) {
	rtBellSetting.freqhigh=64;
	rtBellSetting.freqlow = (char) ((6000.0 /(float) freq) - 9.31);
    } else if (freq < 91) {
	rtBellSetting.freqhigh=32;
	rtBellSetting.freqlow = (char) ((12000.0 /(float) freq) - 9.37);
    } else if (freq < 182) {
	rtBellSetting.freqhigh=16;
	rtBellSetting.freqlow = (char) ((24000.0 /(float) freq) - 9.48);
    } else if (freq < 363) {
	rtBellSetting.freqhigh=8;
	rtBellSetting.freqlow = (char) ((48000.0 /(float) freq) - 9.71);
    } else if (freq < 725) {
	rtBellSetting.freqhigh=4;
	rtBellSetting.freqlow = (char) ((96000.0 /(float) freq) - 10.18);
    } else if (freq < 1433) {
	rtBellSetting.freqhigh=2;
	rtBellSetting.freqlow = (char) ((192000.0 /(float) freq) - 11.10);
    } else if (freq < 12020) {
	rtBellSetting.freqhigh=1;
	rtBellSetting.freqlow = (char) ((384000.0 /(float) freq) - 12.95);
    } else {
	rtBellSetting.freqhigh=0;
	rtBellSetting.freqlow=SPKOLOMIN;
    }
}

/***============================================================***/

static void
rtChangeKeyboardControl(pDevice,ctrl)
    DevicePtr pDevice;
    KeybdCtrl *ctrl;
{
    int i,volume;

    TRACE(("rtChangeKeyboardControl(pDev=0x%x,ctrl=0x%x)\n",pDevice,ctrl));

    volume = (ctrl->click==0?-1:((ctrl->click / 14) & 7));
    ioctl(rtScreenFD, QIOCCLICK, volume);

    rtSetBellPitch(ctrl->bell_pitch);
    /* X specifies duration in milliseconds, RT in 1/128th's of a second */
    rtBellSetting.duration= ((double)ctrl->bell_duration)*(128.0/1000.0);

    rtChangeLEDs(ctrl->leds);

    ioctl(rtScreenFD, QIOCAUTOREP, ctrl->autoRepeat);
}

/***============================================================***/

static void
rtBell(loud, pDevice)
    int loud;
    DevicePtr pDevice;
{

    TRACE(("rtBell(loud= %d, pDev= 0x%x)\n",loud,pDevice));

    /* RT speaker volume is between 0 (off) and 3 (loud) */
    if (loud!=0 && (rtSpeakerFD!=-1)) {
	loud = (loud / 34)+1;
	rtBellSetting.volume= loud;
	write(rtSpeakerFD,&rtBellSetting,sizeof(rtBellSetting));
    }
}

/***============================================================***/

Bool
LegalModifier(key)
BYTE	key;
{
    TRACE(("LegalModifier(key= 0x%x)\n",key));
    if ((key==RT_CONTROL)||(key==RT_LEFT_SHIFT)||(key==RT_RIGHT_SHIFT)||
	(key==RT_LOCK)||(key==RT_ALT_L)||(key==RT_ALT_R)||(key==RT_ACTION)||
	(key==RT_NUM_LOCK)) {
	return TRUE;
    }
    return FALSE;
}

/***============================================================***/

static	int	rt_pckeys= FALSE;

void
rtUsePCKeyboard()
{
    TRACE(("rtUsePCKeyboard()\n"));

    rt_pckeys= TRUE;
    ErrorF("Using PC keyboard layout...\n");
}

/***============================================================***/

void
rtUseRTKeyboard()
{
    TRACE(("rtUseRTKeyboard()\n"));

    rt_pckeys= FALSE;
    ErrorF("Using RT keyboard layout...\n");
}

/***============================================================***/

rtGetKbdMappings( pKeySyms, pModMap )
KeySymsPtr	pKeySyms;
CARD8 *pModMap;
{
    register int i;
    TRACE(("rtGetKbdMappings( pKeySyms= 0x%x, pModMap= 0x%x )\n",
							pKeySyms,pModMap));
    for (i = 0; i < MAP_LENGTH; i++)
	pModMap[i] = NoSymbol;	/* make sure it is restored */

    if (rt_pckeys) {
	pModMap[ RT_CONTROL ] = LockMask;
	pModMap[ RT_LOCK ] = ControlMask;
    }
    else {
	pModMap[ RT_LOCK ] = LockMask;
	pModMap[ RT_CONTROL ] = ControlMask;
    }
    pModMap[ RT_LEFT_SHIFT ] = ShiftMask;
    pModMap[ RT_RIGHT_SHIFT ] = ShiftMask;
    pModMap[ RT_ALT_L ] = Mod1Mask;
    pModMap[ RT_ALT_R ] = Mod1Mask;
    pModMap[ RT_NUM_LOCK ] = LockMask;

    pKeySyms->minKeyCode=	RT_MIN_KEY;
    pKeySyms->maxKeyCode=	RT_MAX_KEY;
    pKeySyms->mapWidth=		RT_GLYPHS_PER_KEY;
    pKeySyms->map=		rtmap;
}

/***============================================================***/

int
rtKeybdProc(pDev, onoff, argc, argv)
    DevicePtr 	 pDev;
    int 	 onoff;
    int		 argc;
    char	*argv[];
{
    KeySymsRec		keySyms;
    CARD8 		modMap[MAP_LENGTH];

    TRACE(("rtKeybdProc( pDev= 0x%x, onoff= 0x%x )\n",pDev,onoff));

    switch (onoff)
    {
	case DEVICE_INIT: 
	    rtKeybd = pDev;
	    pDev->devicePrivate = (pointer) & rtQueue;
	    rtGetKbdMappings( &keySyms, &modMap );
	    InitKeyboardDeviceStruct(
			rtKeybd, &keySyms, &modMap, rtBell,
			rtChangeKeyboardControl);
	    if (rtSpeakerFD==-1) 
		rtSpeakerFD= open("/dev/speaker",O_WRONLY);
	    if (rtSpeakerFD==-1) 
		ErrorF("Couldn't open /dev/speaker\n");
	    break;
	case DEVICE_ON: 
	    pDev->on = TRUE;
	    AddEnabledDevice(rtScreenFD);
	    break;
	case DEVICE_OFF: 
	    pDev->on = FALSE;
/*	    RemoveEnabledDevice(rtScreenFD);*/
	    break;
	case DEVICE_CLOSE:
	    if (rtSpeakerFD!=-1)
		close(rtSpeakerFD);
	    rtSpeakerFD= -1;
	    break;
    }
    return Success;
}


