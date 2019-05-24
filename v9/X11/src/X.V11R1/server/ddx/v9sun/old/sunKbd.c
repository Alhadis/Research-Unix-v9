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
	register char c, *cp;
	register i;
	char *tail;
	static char rbuf[512];

	if ((i = read (sunKbdfd, rbuf, sizeof(rbuf))) < 0)
		FatalError ("Could not read from keyboard");

	tail = &rbuf[i];
	for (cp = rbuf; cp != tail; ) {
		c = *cp++;
		/*
		 * Eat the idle key messages
		 */
		if (c == 0x7f)
			continue;
		if (c & 0x80)
			sunmkX(pKeyboard, KeyRelease, c & 0x7F);
		else
		 	sunmkX(pKeyboard, KeyPress, c & 0x7F);
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
