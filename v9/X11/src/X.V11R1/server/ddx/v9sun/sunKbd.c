#define NEED_EVENTS
#include "sun.h"
#include <stdio.h>
#include <sys/ioctl.h>
#include "Xproto.h"
#include "keysym.h"

extern CARD8 *sunModMap[];
extern KeySymsRec sunKeySyms[];

static void 	  sunBell();
static void 	  sunKbdCtrl();

int sunKbdfd = -1;
static struct sgttyb	sttymodes;
static struct ttydevb	devmodes;
static struct tchars	tcharmodes;
#define	MAXNLINED	10
static int sunnld, linedstack[MAXNLINED];

static void
sunsavemodes()
{
	char filename[256], *p;
	char *getenv();
	int fd;

	if((p=getenv("HOME")) == 0)
		return;
	strcpy(filename, p);
	strcat(filename, "/.sttymodes");
	if ((fd = creat(filename, 0644)) < 0)
		return;
	ioctl(sunKbdfd, TIOCGETP, &sttymodes);
	ioctl(sunKbdfd, TIOCGDEV, &devmodes);
	ioctl(sunKbdfd, TIOCGETC, &tcharmodes);
	write(fd, &sttymodes, sizeof(struct sgttyb));
	write(fd, &devmodes, sizeof(struct ttydevb));
	write(fd, &tcharmodes, sizeof(struct tchars));
	close(fd);
	sunnld = 0;
	while ((linedstack[sunnld] = ioctl(sunKbdfd, FIOLOOKLD, 0)) >= 0) {
		ioctl(sunKbdfd, FIOPOPLD, 0);
		sunnld++;
	}
}

static void
sunresetmodes()
{
	 for (sunnld--; sunnld >= 0; sunnld--)
		ioctl(sunKbdfd, FIOPUSHLD, &linedstack[sunnld]);
	ioctl(sunKbdfd, TIOCSETP, &sttymodes);
	ioctl(sunKbdfd, TIOCSDEV, &devmodes);
	ioctl(sunKbdfd, TIOCSETC, &tcharmodes);
}

int
sunKbdProc (pKeyboard, what)
    DevicePtr	  pKeyboard;	/* Keyboard to manipulate */
    int	    	  what;	    	/* What to do to it */
{
    register int  kbdFd;

 	switch (what) {
	case DEVICE_INIT:
		if (pKeyboard != LookupKeyboardDevice()) {
			ErrorF ("Cannot open non-system keyboard");
			return (!Success);
		}
		if (sunKbdfd >= 0)
			kbdFd = sunKbdfd;
		else {
			kbdFd = open ("/dev/console", 2);
			if (kbdFd < 0) {
				Error ("Opening /dev/console");
				return (!Success);
			}
			sunKbdfd = kbdFd;
			sunsavemodes();
		}
	    
		pKeyboard->on = FALSE;
		InitKeyboardDeviceStruct(
			pKeyboard, sunKeySyms, sunModMap[0], sunBell, sunKbdCtrl);
		break;

	case DEVICE_ON:
		AddEnabledDevice(sunKbdfd);
		pKeyboard->on = TRUE;
		break;

	case DEVICE_CLOSE:
	case DEVICE_OFF:
		sunresetmodes();
		RemoveEnabledDevice(sunKbdfd);
		pKeyboard->on = FALSE;
		break;
    }
    return (Success);
}

static void
sunBell (loudness, pKeyboard)
    int	    	  loudness;	    /* Percentage of full volume */
    DevicePtr	  pKeyboard;	    /* Keyboard to ring */
{
#define	KBD_CMD_BELL	0x02		/* Turn on the bell */
#define	KBD_CMD_NOBELL	0x03		/* Turn off the bell */
	static char c[2] = { KBD_CMD_BELL, KBD_CMD_NOBELL};

	if (loudness)
		write(sunKbdfd, c, 2);
}

void
sunKbdEvent(pKeyboard)
DevicePtr pKeyboard;
{
	register char c, *cp, *cp2;
	register i;
	char *tail;
	int up;
	static char rbuf[512];
	static char keystate[128];
	static int ndown;

	if ((i = read (sunKbdfd, rbuf, sizeof(rbuf))) < 0)
		FatalError ("Could not read from keyboard");

	tail = &rbuf[i];
	for (cp = rbuf; cp != tail; ) {
		c = *cp++;
		/*
		 * If idle, all keys should be up
		 */
		if (c == 0x7f) {
			if (ndown) {
				for(cp2 = keystate; cp2 < &keystate[128]; cp2++)
					if (*cp2) {
						sunmkX(pKeyboard, KeyRelease, cp2 - keystate);
						*cp2 = 0;
					}
				ndown = 0;
			}
			continue;
		}
		i = c & 0x7f;
		up = c & 0x80;
		if (up) {
			if (keystate[i]) {
				sunmkX(pKeyboard, KeyRelease, i);
				keystate[i] = 0;
				ndown--;
			} else {
			 	sunmkX(pKeyboard, KeyPress, i);
				sunmkX(pKeyboard, KeyRelease, i);
			}
		} else {
			if (keystate[i]) {
				sunmkX(pKeyboard, KeyRelease, i);
			 	sunmkX(pKeyboard, KeyPress, i);
			} else {
			 	sunmkX(pKeyboard, KeyPress, i);
				keystate[i] = 1;
				ndown++;
			}
		}
	}

}

static void
sunKbdCtrl (pKeyboard, ctrl)
    DevicePtr	pKeyboard;	    /* Keyboard to alter */
    KeybdCtrl	*ctrl;
{
    char c;

#define	KBD_CMD_CLICK	0x0A		/* Turn on the click annunciator */
#define	KBD_CMD_NOCLICK	0x0B		/* Turn off the click annunciator */
    if (ctrl->click == 0) {   /* turn click off */
	c = KBD_CMD_NOCLICK;
	write(sunKbdfd, &c, 1);
    } else {
	c = KBD_CMD_CLICK;
	write(sunKbdfd, &c, 1);
    }
}

Bool
LegalModifier(key)
{
    return (TRUE);
}
