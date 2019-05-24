#include <dk.h>
#include <dkmgr.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
#include <ipc.h>
#include <dkwindow.h>
#include "defs.h"

/* imported */
extern int dkp_ld;
extern int unit;
extern char *dkfilename();

/* global to this module */
static int chan;
static long tstamp;
static int traffic;
static int newfd;

ipcinfo *
net_listen(fd)
	int fd;
{
	static char buf[256];
	static ipcinfo info;
	static char secid[128];
	char *fp, *tp;
	char *line[12];
	char *field[12];
	int n;

	/* get the request */
	if ((n = read(fd, buf, sizeof(buf)-1)) <= 0) {
		ABORT(errno, "bad read in listen", NULLINFO);
		return NULL;
	}
	buf[n] = '\0';
	logevent("<DS>%s\n", buf);

	/* initialize the info struct */
	info.rfd = info.cfd = -1;
	info.user = info.machine = NULL;
	info.uid = info.gid = -1;
	info.name = NULL;
	info.param = NULL;
	info.flags = IPC_OPEN;

	/* break into lines */
	setfields("\n");
	n = getfields(buf, line, 12);
	if (n < 2) {
		ABORT(EIO, "bad message from dk (1 line)", NULLINFO);
		return NULL;
	}

	/* line 0 is `chan.tstamp.traffic' */
	setfields(".");
	getfields(line[0], field, 3);
	chan = atoo(field[0]);
	tstamp = atoo(field[1]);
	traffic = atoo(field[2]);
	info.param = W_TRAF(traffic)==0 ? "light" : "heavy";

	/* line 1 is `dialstring.service', remember escapes */
	info.myname = line[1];
	for(tp=fp=line[1]; *fp; fp++)
		if(*fp=='.') {
			if (info.name==NULL){
				*tp++ = '\0';
				info.name = tp;
			} else
				*tp++ = '!';
		} else if(*fp=='\\' && *(fp+1)=='.') {
			*tp++ = '.';
			fp++;
		} else
			*tp++ = *fp;

	/* the rest is variable length */
	switch(n) {
	case 2:
		/* no more lines */
		info.machine = NULL;
		info.user = NULL;
		info.param = NULL;
		break;
	case 3:
		/* line 2 is `source.user.param1.param2' */
		logevent("source = %s\n", line[2]);
		getfields(line[2], field, 3);
		info.machine = field[0];
		info.user = field[1];
		info.param = field[2];
		break;
	case 4:
		/* line 2 is `user.param1.param2' */
		getfields(line[2], field, 2);
		info.user = field[0];
		info.param = field[1];

		/* line 3 is `source.node.mod.chan' */
		logevent("source = %s\n", line[3]);
		getfields(line[3], field, 2);
		info.machine = field[0];
		break;
	default:
		ABORT(ENXIO, "bad message from dk(>4 line)", NULLINFO);
		return NULL;
	}

	/* open the channel */
	if ((info.rfd = open(dkfilename(chan), 2))<0) {
		ABORT(EIO, "can't open channel", NULLINFO);
		return NULL;
	}
	if (dkproto(info.rfd, dkp_ld) < 0) {
		close(info.rfd);
		ABORT(EIO, "can't push line discipline",NULLINFO);
		return NULL;
	}
	setwins(info.rfd, traffic);
	ioctl(info.rfd, DIOCNXCL, 0);
	newfd = dup(info.rfd);

	if (info.user==NULL || info.user[0]=='\0')
		info.user = "_unknown_";
	if (info.machine==NULL || info.machine[0]=='\0')
		info.machine = "_unknown_";
	if (info.name==NULL)
		info.name = "";
	return (&info);
}

atoo(cp)
	register char *cp;
{
	register int x;

	for(x=0; *cp>='0' && *cp<='7'; cp++)
		x = (x<<3) | (*cp-'0');
	return x;
}

setwins(f, traffic)
{
	char ws[5];
	long wins;
	if (W_VALID(traffic) && W_TRAF(traffic)) {
		wins = W_VALUE(W_ORIG(traffic));
		/* try 3 X 1/4 */
		wins >>= 2;
		ws[0] = wins;
		ws[1] = wins>>8;
		ws[2] = 0;
		ws[3] = 0;
		ws[4] = 3;
		ioctl(f, DIOCXWIN, ws);
	}
}

void
net_accept(ip)
	ipcinfo *ip;
{
	struct listenin d;
	int fd;

	USE(ip);
	if ((fd=dkctlchan())<0) {
		close(newfd);
		return;
	}
	d.l_lchan = 1;
	d.l_type = T_REPLY;
	d.l_srv = D_OPEN;
	d.l_param0 = chan;
	d.l_param1 = 0;
	d.l_param2 = chan;
	d.l_param3 = tstamp;
	d.l_param4 = 0;
	d.l_param5 = 0;
	write(fd, (char *)&d, sizeof(d));
	close(fd);
	close(newfd);
}

void
net_reject(ip, error, msg)
	ipcinfo *ip;
	int error;
	char *msg;
{
	struct listenin d ;
	int fd;

	if ((fd=dkctlchan())<0) {
		close(newfd);
		return;
	}
	d.l_lchan = 1;
	d.l_type = T_REPLY;
	d.l_srv = D_FAIL;
	d.l_param0 = chan;
	d.l_param1 = utodkerr(error);
	d.l_param2 = chan;
	d.l_param3 = tstamp ;
	d.l_param4 = 0;
	d.l_param5 = 0;
	write(fd, (char *)&d, sizeof(d));
	close(fd);
	close(ip->rfd);
	ip->rfd = -1;
	close(newfd);
	errno = error;
	errstr = msg;
	return;
}
