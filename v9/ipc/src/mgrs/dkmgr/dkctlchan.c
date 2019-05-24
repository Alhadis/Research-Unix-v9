#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
#include <sys/dkio.h>

extern int errno;
extern int rdk_ld;
extern int dkp_ld;

static char *cnames[] = {
	"/dev/dk/dkctl0",
	"/dev/dk/dkctl2",
	"/dev/dk/dkctl",
	0
};
static char *names[] = {
	"/dev/dk/dk0%c%d",
	"/dev/dk/dk2%c%d",
	"/dev/dk/dk%c%d",
	0
};

static	int namefmt=-1;
static	char alph[] = "0123456789abcdefghijklmnopqrstuvwxyz";

static int high[] = {256, 256};
static int low[] = {3, 3 };

/*
 *	Open a free dk channel and remember the `type' in namefmt.
 */
int
dkchan(traffic, unit)
	int traffic;
	int unit;
{
	int missing=0;
	char outname[64];
	register i,j;
	int ai;
	int fd;

	switch(unit) {
	case '0':
		ai = 0;
		break;
	case '2':
		ai = 2;
		break;
	case 'b':
		ai = traffic;
		break;
	default:
		ai = -1;
		break;
	}
	for (j=0; j<2; j++) {
		for (i=low[j]; i<high[j]; i+=2) {
			if (ai>=0) {
				sprintf(outname, "/dev/dk/dk%d%c%d", ai,
					alph[i/10], i%10);
				if ((fd = open(outname, 2)) >= 0) {
					namefmt = ai ? 1 : 0;
					break;
				}
			} else {
				sprintf(outname, "/dev/dk/dk%c%d",
					alph[i/10], i%10);
				if ((fd = open(outname, 2)) >= 0) {
					namefmt = 2;
					break;
				}
			}
			if (errno==ENOENT) 
				if (++missing>5)
					break;
		}
		if (fd>=0)
			break;
	}
	if (fd < 0)
		return -1;
	low[0] = high[1] = i+2;
	return fd;
}

/*
 * open common control channel
 */
dkctlchan()
{
	register i ;
	char dkctlfile[32];

	sprintf(dkctlfile, cnames[namefmt]);
	i = open(dkctlfile, 2);
	if (i < 0) { 
		sprintf(dkctlfile, names[namefmt], alph[0], 1);
		i = open(dkctlfile, 2);
		if (i < 0) 
			return -1;
	}
	ioctl(i, DIOCNXCL, 0);
	if (ioctl(i, FIOLOOKLD, 0) != rdk_ld) {
		if (dkproto(i, dkp_ld) < 0) {
			fprintf(stderr, "can't push dkp_ld on %s\n", dkctlfile) ;
			close(i) ;
			return -1;
		}
		if (ioctl(i, FIOPUSHLD, &rdk_ld) < 0) {
			fprintf(stderr, "can't push rdk_ld on %s\n", dkctlfile) ;
			close(i) ;
			return -1;
		}
		if (ioctl(i, DIOCLHN, (char *)0) < 0) {
			fprintf(stderr, "can't be manager on %s\n", dkctlfile) ;
			close(i);
			return -1;
		}
	}
	return i;
}

/*
 * find the file name corresponding to a given channel.
 * assumes dkctlchan has been called.
 * Used for incoming calls.
 */

char *
dkfilename(chan)
{
	static char name[64];

	if (namefmt==-1)
		return(NULL);
	sprintf(name, names[namefmt], alph[chan/10], chan%10);
	return(name);
}

/*
 * Make sure that URP protocol is enabled on a datakit file.
 */
int
dkproto(file, linedis)
{
	if (ioctl(file, KIOCISURP, (char *)0) < 0)
		return(ioctl(file, FIOPUSHLD, &linedis));
	ioctl(file, KIOCINIT, (char *)0);
	return(0);
}
