#define NEED_EVENTS
#include    "sun.h"
#include <sys/ioctl.h>

extern int	lastEventTime;
static void	sunMouseCtrl();
static int	sunMouseGetMotionEvents();

static PtrPrivRec sysMousePriv = {
    0,				/* Current X coordinate of pointer */
    0,				/* Current Y coordinate */
    NULL,			/* Screen pointer is on */
};
int sunMousefd = -1;

int
sunMouseProc (pMouse, what)
    DevicePtr	  pMouse;   	/* Mouse to play with */
    int	    	  what;	    	/* What to do with it */
{
    register int  fd;
    int	    	  format;
    static int	  oformat;
    BYTE    	  map[4];
    struct ttydevb tspeed;

    switch (what) {
	case DEVICE_INIT:
		if (pMouse != LookupPointerDevice()) {
			ErrorF ("Cannot open non-system mouse");	
			return (!Success);
	 	}

		if (sunMousefd >= 0) {
		    fd = sunMousefd;
		} else {
		    fd = open ("/dev/mouse", 0);
		    if (fd < 0) {
			Error ("Opening /dev/mouse");
			return (!Success);
		    }
		    sunMousefd = fd;
		    ioctl(sunMousefd, TIOCGDEV, &tspeed);
		    tspeed.ispeed = tspeed.ospeed = B1200;
		    ioctl(sunMousefd, TIOCSDEV, &tspeed);
		}

	    sysMousePriv.pScreen = &screenInfo.screen[0];
	    sysMousePriv.x = sysMousePriv.pScreen->width / 2;
	    sysMousePriv.y = sysMousePriv.pScreen->height / 2;

	    pMouse->devicePrivate = (pointer) &sysMousePriv;
	    pMouse->on = FALSE;
	    map[1] = 1;
	    map[2] = 2;
	    map[3] = 3;
	    InitPointerDeviceStruct(
		pMouse, map, 3, sunMouseGetMotionEvents, sunMouseCtrl);
	    break;

	case DEVICE_ON:
		AddEnabledDevice (sunMousefd);
		pMouse->on = TRUE;
		break;

	case DEVICE_CLOSE:
	    break;

	case DEVICE_OFF:
	 	pMouse->on = FALSE;
		RemoveEnabledDevice (sunMousefd);
	 	break;
	}
    return (Success);
}
	    
static void
sunMouseCtrl (pMouse)
    DevicePtr	  pMouse;
{
}

static int
sunMouseGetMotionEvents (buff, start, stop)
    CARD32 start, stop;
    xTimecoord *buff;
{
    return 0;
}

static short
MouseAccelerate (pMouse, delta)
    DevicePtr	  pMouse;
    int	    	  delta;
{
    register int  sgn = sign(delta);
    register PtrCtrl *pCtrl;

    delta = abs(delta);
    pCtrl = &((DeviceIntPtr) pMouse)->u.ptr.ctrl;

    if (delta > pCtrl->threshold) {
	return ((short) (sgn * (pCtrl->threshold +
				((delta - pCtrl->threshold) * pCtrl->num) /
				pCtrl->den)));
    } else {
	return ((short) (sgn * delta));
    }
}

void
sunMouseEvent(pMouse)
DevicePtr pMouse;
{
	register i;
	static char rbuf[512];
	static mstate, buttons, changebuttons;
	static deltax, deltay;
	int mousemoved = 0;
	char *tail;
	register char c, *cp;

	if ((i = read (sunMousefd, rbuf, sizeof(rbuf))) < 0)
		FatalError ("Could not read from mouse");

	tail = &rbuf[i];
	for (cp = rbuf; cp != tail; mstate++) {
		c = *cp++;
	
		/*
		 * State Machine - corresponds to the 5 bytes of the
		 * Microport mouse.
		 */
		switch(mstate) {
		case 0:
			if ((c & 0xf0) == 0x80) {
				i = ~c & 0x7;
				changebuttons = buttons ^ i;
				buttons = i;
			} else
				mstate--;
			break;
		case 1:
			deltax = c;
			break;
		case 2:
			deltay = c;
			break;
		case 3:
			deltax += c;
			break;
		case 4:
			deltay += c;
			/* The mouse moved */
			if (deltax || deltay) {
			    if (deltax)
				sysMousePriv.x += MouseAccelerate (pMouse, deltax);
			    if (deltay)
				sysMousePriv.y -= MouseAccelerate (pMouse, deltay);
			    if (sunConstrainXY (&sysMousePriv.x, &sysMousePriv.y))
			        mousemoved++;
			}
			/* Buttons changed states */
			if (changebuttons) {
			    if (mousemoved) {
				sunmkX(pMouse, MotionNotify, 0);
				mousemoved = 0;
			    }
			    for (i = 0; i < 3; i++) {
				if (changebuttons & (1 << i)) {
				 int type;
				 if (buttons & (1 << i))
				   type = ButtonPress;
				 else
				   type = ButtonRelease;
				 sunmkX(pMouse, type, 3 - i);
				}
			    }
			}
			mstate = -1;
			break;
		}
	}
	if (mousemoved)
		sunmkX(pMouse, MotionNotify, 0);
}

/*
 * Send a mouse of keyboard X event to dix
 */
sunmkX(pDev, type, detail)
DevicePtr pDev;
{
	xEvent xE;

	if (type == MotionNotify)
		sunMoveCursor (sysMousePriv.pScreen,
				sysMousePriv.x, sysMousePriv.y);
	xE.u.u.type = type;
	xE.u.u.detail = detail;
	xE.u.keyButtonPointer.rootX = sysMousePriv.x;
	xE.u.keyButtonPointer.rootY = sysMousePriv.y;
	xE.u.keyButtonPointer.time = ++lastEventTime;
	(*pDev->processInputProc) (&xE, pDev);
}
