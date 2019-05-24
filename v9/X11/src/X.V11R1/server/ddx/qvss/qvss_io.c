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
/* $Header: qvss_io.c,v 1.68 87/09/10 16:23:01 swick Exp $ */

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/time.h>
#include <vaxuba/qvioctl.h>
#include <errno.h>

#include "X.h"
#define  NEED_EVENTS
#include "Xproto.h"
#include "scrnintstr.h"
#include "cursorstr.h"
#include "pixmap.h"
#include "input.h"
#include "windowstr.h"
#include "regionstr.h"
#include "resource.h"

#include "mfb.h"
#include "mi.h"

Bool qvRealizeCursor(), qvUnrealizeCursor(), qvDisplayCursor();
Bool qvSetCursorPosition();
void qvCursorLimits();
void qvPointerNonInterestBox();
void qvConstrainCursor();
void qvQueryBestSize();
void qvResolveColor();
void qvCreateColormap();
void qvDestroyColormap();

extern void miRecolorCursor();
extern int  qvGetMotionEvents();
extern void qvChangePointerControl(), qvChangeKeyboardControl(), qvBell();

extern int errno;

static struct qv_info	*qvInfo;
static vsEventQueue	*queue;
static vsBox		*mbox;
static vsCursor		*mouse;
static int		fdQVSS;
static int		qLimit;
static int		lastEventTime;
static DevicePtr	qvKeyboard;
static DevicePtr	qvPointer;
static int              hotX, hotY;
static BoxRec           constraintBox;

#define MAX_LED 4

/* ARGSUSED */
static Bool
qvssSaveScreen(pScreen, on)
    ScreenPtr pScreen;
    int on;
{
    if (on == SCREEN_SAVER_FORCER)
    {
        lastEventTime = GetTimeInMillis();	
	return TRUE;
    }
    else
        return FALSE;
}

/* ARGSUSED */
static Bool
qvssScreenClose(index, pScreen)
    int index;
    ScreenPtr pScreen;
{
    /* This routine frees all of the dynamically allocated space associate
	with a screen. */

    mfbScreenClose(pScreen);

    if(close(fdQVSS))
    {
	ErrorF("Closing QVSS yielded %d\n", errno);
	return (FALSE);
    }
    return (TRUE);
}


static
qvssPixelError(index)
    int	index;
{
    ErrorF("Only 0 or 1 are acceptable pixels for device %d\n", index);
}


Bool
qvssScreenInit(index, pScreen, argc, argv)
    int index;
    ScreenPtr pScreen;
    int argc;		/* these two may NOT be changed */
    char **argv;
{
    Bool		retval, SpecificB, SpecificW;
    ColormapPtr		pColormap;
    int			i;
    char		*blackValue, *whiteValue;

    if ((fdQVSS = open("/dev/mouse", O_RDWR | O_NDELAY, 0)) <  0)
    {
	ErrorF(  "couldn't open qvss \n");
	return FALSE; 
    }
    ioctl(fdQVSS, QIOCINIT, 0); /* force an init to get the scrollmap right */
    if (ioctl(fdQVSS, QIOCADDR, &qvInfo) < 0)
    {
	ErrorF(  "error getting address of QVSS \n");
	close(fdQVSS);
	return FALSE;
	}
    mouse = (vsCursor *) &qvInfo->mouse;
    mbox = (vsBox *) &qvInfo->mbox;
	/*
	   the following hack is really pretending a part of one record
	   is really a record of another type!
        */
    queue = (vsEventQueue *) &qvInfo->ibuff;
    qLimit = queue->size - 1;

    retval = mfbScreenInit(index, pScreen, qvInfo->bitmap, 
			   1024, 864, 80, 80);
    pScreen->CloseScreen = qvssScreenClose;
    pScreen->SaveScreen = qvssSaveScreen;
    pScreen->RealizeCursor = qvRealizeCursor;
    pScreen->UnrealizeCursor = qvUnrealizeCursor;
    pScreen->DisplayCursor = qvDisplayCursor;
    pScreen->SetCursorPosition = qvSetCursorPosition;
    pScreen->CursorLimits = qvCursorLimits;
    pScreen->PointerNonInterestBox = qvPointerNonInterestBox;
    pScreen->ConstrainCursor = qvConstrainCursor;
    pScreen->RecolorCursor = miRecolorCursor;
    pScreen->QueryBestSize = qvQueryBestSize;
    pScreen->ResolveColor = qvResolveColor;
    pScreen->CreateColormap = qvCreateColormap;
    pScreen->DestroyColormap = qvDestroyColormap;
    CreateColormap(pScreen->defColormap, pScreen,
                   LookupID(pScreen->rootVisual, RT_VISUALID, RC_CORE),
                   &pColormap, AllocNone, 0);

    mfbInstallColormap(pColormap);
    blackValue = NULL;
    SpecificB = FALSE;
    whiteValue = NULL;
    SpecificW = FALSE;
    for(i = 1; i < argc; i++)
    {
	if(strncmp(argv[i], "-bp:", 4) == 0 && atoi(argv[i] + 4) == index)
	{
	    if(++i < argc)
	    {
		blackValue = argv[i];
		SpecificB = TRUE;
	    }
	    else
		UseMsg();
	}
	if(strncmp(argv[i], "-wp:", 4) == 0 && atoi(argv[i] + 4) == index)
	{
	    if(++i < argc)
	    {
		whiteValue = argv[i];
		SpecificW = TRUE;
	    }
	    else
		UseMsg();
	}
	if(strcmp(argv[i], "-bp") == 0 && !SpecificB)
	{
	    if(++i < argc)
	    {
		blackValue = argv[i];
	    }
	    else
		UseMsg();
	}
	if(strcmp(argv[i], "-wp") == 0 && !SpecificW)
	{
	    if(++i < argc)
	    {
		whiteValue = argv[i];
	    }
	    else
		UseMsg();
	}
	    
    }
    if(blackValue)
    {
	if((i = atoi(blackValue)) == 0 || i == 1)
	    pScreen->blackPixel = i;
	else  
	    qvssPixelError(index);
    }
    if(whiteValue)
    {
        if((i = atoi(whiteValue)) == 0 || i == 1)
    	    pScreen->whitePixel = i;
        else  
	    qvssPixelError(index);
    }
    return(retval);
}

/* ARGSUSED */
int
qvssMouseProc(pDev, onoff, argc, argv)
    DevicePtr pDev;
    int onoff, argc;
    char *argv[];
{
    BYTE map[4];

    switch (onoff)
    {
	case DEVICE_INIT: 
	    qvPointer = pDev;
	    pDev->devicePrivate = (pointer) &queue;
	    map[1] = 1;
	    map[2] = 2;
	    map[3] = 3;
	    InitPointerDeviceStruct(
		qvPointer, map, 3, qvGetMotionEvents, qvChangePointerControl);
	    SetInputCheck(&queue->head, &queue->tail);
	    break;
	case DEVICE_ON: 
	    pDev->on = TRUE;
            hotX = hotY = 0;
	    AddEnabledDevice(fdQVSS);
	    break;
	case DEVICE_OFF: 
	    pDev->on = FALSE;
/*	RemoveEnabledDevice(fdQVSS);   */
	    break;
	case DEVICE_CLOSE: 
	    break;
    }
    return Success;

}

#define LK_REPEAT_ON  0xe3
#define LK_REPEAT_OFF 0xe1

int
SetLKAutoRepeat (onoff)
    Bool onoff;
{
    extern char *AutoRepeatLKMode();
    extern char *UpDownLKMode();
    
    struct qv_kpcmd ioc; 
    register char  *divsets;
    divsets = onoff ? (char *) AutoRepeatLKMode() : (char *) UpDownLKMode();

    ioc.nbytes = 0;
    while (ioc.cmd = *divsets++)
	ioctl(fdQVSS, QIOCKPCMD, &ioc);
    ioc.cmd = ((onoff > 0) ? LK_REPEAT_ON : LK_REPEAT_OFF);
    return(ioctl(fdQVSS, QIOCKPCMD, &ioc));

}

/* ARGSUSED */
int
qvssKeybdProc(pDev, onoff, argc, argv)
    DevicePtr pDev;
    int onoff, argc;
    char *argv[];
{
    KeySymsRec keySyms;
    CARD8 modMap[MAP_LENGTH];

    switch (onoff)
    {
	case DEVICE_INIT: 
	    qvKeyboard = pDev;
	    pDev->devicePrivate = (pointer) & queue;
	    GetLK201Mappings( &keySyms, modMap);
	    InitKeyboardDeviceStruct(
		    qvKeyboard, &keySyms, modMap, qvBell,
		    qvChangeKeyboardControl);
            Xfree((pointer)keySyms.map);
/*	    SetLKAutoRepeat(FALSE); */
	    break;
	case DEVICE_ON: 
	    pDev->on = TRUE;
	    AddEnabledDevice(fdQVSS);
	    break;
	case DEVICE_OFF: 
	    pDev->on = FALSE;
/*	    RemoveEnabledDevice(fdQVSS);  */
	    break;
	case DEVICE_CLOSE: 
	    break;
    }
    return Success;
}


/*****************
 * ProcessInputEvents:
 *    processes all the pending input events
 *****************/

extern int screenIsSaved;

void
ProcessInputEvents()
{
#define DEVICE_KEYBOARD 2
    register int    i;
    register    vsEvent * pE;
    xEvent	x;
    int     nowInCentiSecs, nowInMilliSecs, adjustCentiSecs;
    struct timeval  tp;
    int     needTime = 1;

    i = queue->head;
    while (i != queue->tail)
    {
	if (screenIsSaved == SCREEN_SAVER_ON)
	    SaveScreens(SCREEN_SAVER_OFF, ScreenSaverReset);
	pE = &queue->events[i];
	x.u.keyButtonPointer.rootX = pE->vse_x + hotX;
	x.u.keyButtonPointer.rootY = pE->vse_y + hotY;
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
	if ((adjustCentiSecs = nowInCentiSecs - pE->vse_time) < -20000)
	    adjustCentiSecs += 0x10000;
	else
	    if (adjustCentiSecs > 20000)
		adjustCentiSecs -= 0x10000;
	x.u.keyButtonPointer.time = lastEventTime =
	    nowInMilliSecs - adjustCentiSecs * 10;

	if ((pE->vse_type == VSE_BUTTON) &&
	    (pE->vse_device == DEVICE_KEYBOARD))
	{					/* better be a button */
	    x.u.u.detail = pE->vse_key;
	    switch (pE->vse_direction)
	    {
		case VSE_KBTDOWN: 
		    x.u.u.type = KeyPress;
		    (qvKeyboard->processInputProc)(&x, qvKeyboard);
		    break;
		case VSE_KBTUP: 
		    x.u.u.type = KeyRelease;
		    (qvKeyboard->processInputProc)(&x, qvKeyboard);
		    break;
		default: 	       /* hopefully BUTTON_RAW_TYPE */
		    ProcessLK201Input(&x, qvKeyboard);
	    }
	}
	else
	{
	    if (pE->vse_type == VSE_BUTTON)
	    {
		if (pE->vse_direction == VSE_KBTDOWN)
		    x.u.u.type = ButtonPress;
		else
		    x.u.u.type = ButtonRelease;
		/* mouse buttons numbered from one */
		x.u.u.detail = pE->vse_key + 1;
	    }
	    else
		x.u.u.type = MotionNotify;
	(*qvPointer->processInputProc)(&x, qvPointer);
	}

	if (i == qLimit)
	    i = queue->head = 0;
	else
	    i = ++queue->head;
    }
#undef DEVICE_KEYBOARD
}

TimeSinceLastInputEvent()
{
    if (lastEventTime == 0)
	lastEventTime = GetTimeInMillis();
    return GetTimeInMillis() - lastEventTime;
}

/* ARGSUSED */
static void
qvBell(loud, pDevice)
    int loud;
    DevicePtr pDevice;
{
#define LK_ENABLE_BELL 0x23	/* enable bell / set volume 	*/
    struct qv_kpcmd ioc;

/* the lk201 volume is between 7 (quiet but audible) and 0 (loud) */
    loud = 7 - ((loud / 14) & 7);
    ioc.nbytes = 1;
    ioc.cmd = LK_ENABLE_BELL;
    ioc.par[0] = loud;
    ioctl(fdQVSS, QIOCKPCMD, &ioc);

    ioc.nbytes = 0;
    ioc.cmd = LK_RING_BELL;
    ioctl(fdQVSS, QIOCKPCMD, &ioc);
}

static void
ChangeLED(led, on)
    int led;
    Bool on ;
{
    struct qv_kpcmd ioc;

    if (led > MAX_LED)    
        return;
    if (on)
        ioc.cmd = LK_LED_ENABLE;
    else
        ioc.cmd = LK_LED_DISABLE;
    if (led == 1)
	ioc.par[0] = LED_1;
    else if (led == 2)
        ioc.par[0] = LED_2;
    else if (led == 3)
        ioc.par[0] = LED_3;
    else if (led == 4)
        ioc.par[0] = LED_4;
    ioc.par[1]  = 0;
    ioc.nbytes = 1;
    ioctl(fdQVSS, QIOCKPCMD, &ioc);
}


/* ARGSUSED */
static void
qvChangeKeyboardControl(pDevice, ctrl)
    DevicePtr pDevice;
    KeybdCtrl *ctrl;
{
#define LK_ENABLE_CLICK 0x1b	/* enable keyclick / set volume	*/
#define LK_DISABLE_CLICK 0x99	/* disable keyclick entirely	*/
#define LK_ENABLE_BELL 0x23	/* enable bell / set volume 	*/

    struct qv_kpcmd ioc;
    int i;

    if (ctrl->click == 0)    /* turn click off */
    {
	ioc.nbytes = 0;
	ioc.cmd = LK_DISABLE_CLICK;
	ioctl(fdQVSS, QIOCKPCMD, &ioc);
    }
    else 
    {
        int volume;

        volume = 7 - ((ctrl->click / 14) & 7);
	ioc.nbytes = 1;
	ioc.cmd = LK_ENABLE_CLICK;
	ioc.par[0] = volume;
	ioctl(fdQVSS, QIOCKPCMD, &ioc);
    }

    /* ctrl->bell: the DIX layer handles the base volume for the bell */
    
    /* ctrl->bell_pitch: as far as I can tell, you can't set this on lk201 */

    /* ctrl->bell_duration: as far as I can tell, you can't set this  */

    /* LEDs */
    for (i=1; i<=MAX_LED; i++)
        ChangeLED(i, (ctrl->leds & (1 << (i-1))));

    /* ctrl->autoRepeat: I'm turning it all on or all off.  */

    SetLKAutoRepeat(ctrl->autoRepeat);
}

/* ARGSUSED */
static void
qvChangePointerControl(pDevice, ctrl)
    DevicePtr pDevice;
    PtrCtrl   *ctrl;
{
    qvInfo->mthreshold = ctrl->threshold;
    if (!(qvInfo->mscale = ctrl->num / ctrl->den))
	qvInfo->mscale = 1;	/* watch for den > num */
}

/* ARGSUSED */
static int
qvGetMotionEvents(buff, start, stop)
    CARD32 start, stop;
    xTimecoord *buff;
{
    return 0;
}

/* ARGSUSED */
static Bool
qvSetCursorPosition( pScr, newx, newy, generateEvent)
    ScreenPtr	pScr;
    int	newx;
    int	newy;
    Bool		generateEvent;
{
    vsCursor	cursor;
    xEvent	motion;
    
    cursor.x = newx - hotX;
    cursor.y = newy - hotY;
    if ( ioctl(fdQVSS, QIOCSMSTATE, &cursor) < 0)
    {
	ErrorF( "error warping cursor\n");
	return FALSE;
    }
    if (generateEvent)
    {
	if (queue->head != queue->tail)
	    ProcessInputEvents();
	motion.u.keyButtonPointer.rootX = newx;
	motion.u.keyButtonPointer.rootY = newy;
	motion.u.keyButtonPointer.time = lastEventTime;
	motion.u.u.type = MotionNotify;
	(*qvPointer->processInputProc) (&motion, qvPointer);
    }
    return TRUE;
}

static Bool
qvDisplayCursor( pScr, pCurs)
    ScreenPtr	pScr;
    CursorPtr	pCurs;
{
    int i, x, y;

    /*
     * load the cursor
     */
    if ((hotX != pCurs->xhot) || (hotY != pCurs->yhot))
    {
	x = mouse->x + hotX;
	y = mouse->y + hotY;
	hotX = pCurs->xhot;
	hotY = pCurs->yhot;
	qvSetCursorPosition(pScr, x, y, FALSE);
	qvConstrainCursor(pScr, &constraintBox);
    }
    for ( i=0; i<16; i++)
	qvInfo->cursorbits[i] = ((short *)pCurs->devPriv[ pScr->myNum])[i];
    return TRUE;
}


/* ARGSUSED */
static void
qvPointerNonInterestBox( pScr, pBox)
    ScreenPtr	pScr;
    BoxPtr	pBox;
{
    qvInfo->mbox.bottom = pBox->y2;
    qvInfo->mbox.top = pBox->y1;
    qvInfo->mbox.left = pBox->x1;
    qvInfo->mbox.right = pBox->x2;
}

/* note that this driver does not support dealing with the minimums */
/* ARGSUSED */
static void
qvConstrainCursor( pScr, pBox)
    ScreenPtr	pScr;
    BoxPtr	pBox;
{
    constraintBox = *pBox;
    if (qvInfo)
    {
	qvInfo->max_cur_x = pBox->x2 - hotX - 1;
	qvInfo->max_cur_y = pBox->y2 - hotY - 1;
    }
    else
	ErrorF( "qvConstrainCursor: info = %x\n", qvInfo);
}

/*
 * qv cursor top-left corner cannot go to negative coordinates
 */
/* ARGSUSED */static void
qvCursorLimits( pScr, pCurs, pHotBox, pPhysBox)
    ScreenPtr	pScr;
    CursorPtr	pCurs;
    BoxPtr	pHotBox;
    BoxPtr	pPhysBox;	/* return value */
{
    pPhysBox->x1 = max( pHotBox->x1, pCurs->xhot);
    pPhysBox->y1 = max( pHotBox->y1, pCurs->yhot);
    pPhysBox->x2 = min( pHotBox->x2, 1024);
    pPhysBox->y2 = min( pHotBox->y2, 864);
}

static Bool
qvRealizeCursor( pScr, pCurs)
    ScreenPtr	pScr;
    CursorPtr	pCurs;	/* a SERVER-DEPENDENT cursor */
{
    int	forecolor =
	 ( pCurs->foreRed || pCurs->foreGreen || pCurs->foreBlue)?0:~0;
    int	backcolor = ~forecolor;
    register short	*a, *b;	/* hardware-defined */
    register int *	mask;	/* server-defined */
    register int *	src;	/* server-defined */
    register int	i;
    int		cursorBytes = 32*sizeof(short);
    int		lastRow = ((pCurs->height < 16) ? pCurs->height : 16);
    register unsigned short widthmask = (1<<pCurs->width)-1;
				/* used to mask off beyond the edge of the
				   real mask and source bits
				*/

    pCurs->devPriv[ pScr->myNum] = (pointer)Xalloc(cursorBytes);
    bzero((char *)pCurs->devPriv[ pScr->myNum], cursorBytes);

    /*
     * munge the SERVER-DEPENDENT, device-independent cursor bits into
     * what the device wants, which is 32 contiguous shorts.
     *
     * cursor hardware has "A" and "B" bitmaps
     * logic table is:
     *
     *		A	B	cursor
     *
     *		0	0	transparent
     *		1	0	xor (not used)
     *		0	1	black
     *		1	1	white
     */

    /*
     * "a" bitmap = image 
     *
     * "b" bitmap can be same as "mask", providing "a" is never on when
     *  "b" is off.
     */
    for ( i=0,
	  a = (short *)pCurs->devPriv[pScr->myNum],
	  b = ((short *)pCurs->devPriv[pScr->myNum]) + 16,
	/* XXX assumes DIX pixmap pad is size of int, 
	   and cursor is < 32 bits wide
	*/
	  src = (int *)pCurs->source,
	  mask = (int *)pCurs->mask;

	  i < lastRow;

	  i++, a++, b++, src++, mask++)
    {
	*a = ((*src & forecolor) | (~*src & backcolor)) & *mask;
	*b = *mask;
	*a &= widthmask;
	*b &= widthmask;
    }
    return TRUE;
}

static Bool
qvUnrealizeCursor( pScr, pCurs)
    ScreenPtr	pScr;
    CursorPtr	pCurs;
{
    Xfree((pointer) pCurs->devPriv[ pScr->myNum]);
    return TRUE;
}

static void
qvQueryBestSize(class, pwidth, pheight)
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
	  if (width != 0) {
	      /* Return the closest power of two not less than width */
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

/* ARGSUSED */
void
qvResolveColor(pred, pgreen, pblue, pVisual)
    unsigned short	*pred, *pgreen, *pblue;
    VisualPtr		pVisual;
{
    /* Gets intensity from RGB.  If intensity is >= half, pick white, else
     * pick black.  This may well be more trouble than it's worth. */
    *pred = *pgreen = *pblue = 
        (((39L * *pred +
           50L * *pgreen +
           11L * *pblue) >> 8) >= (((1<<8)-1)*50)) ? ~0 : 0;
}

void
qvCreateColormap(pmap)
    ColormapPtr	pmap;
{
    int	red, green, blue, pix;

    /* this is a monochrome colormap, it only has two entries, just fill
     * them in by hand.  If it were a more complex static map, it would be
     * worth writing a for loop or three to initialize it */
    pix = 0;
    red = green = blue = 0;
    AllocColor(pmap, &red, &green, &blue, &pix, 0);
    pix = 0;
    red = green = blue = ~0;
    AllocColor(pmap, &red, &green, &blue, &pix, 0);

}

/* ARGSUSED */
void
qvDestroyColormap(pmap)
    ColormapPtr	pmap;
{
}
