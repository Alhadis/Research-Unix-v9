#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dk.h>
#include <dkerr.h>
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <ipc.h>
#include <libc.h>
#include "defs.h"
#include <dkwindow.h>
#include <sys/dkio.h>

/* imported */
extern	int	rdk_ld;
extern	int	dkp_ld;
extern	int	dt_ld;
extern 	int	errno;
extern int unit;


int
net_dial(ip)
	ipcinfo *ip;
{
	return dkdial(ip, 3);
}

int
net_announce(ip)
	ipcinfo *ip;
{
	return dkdial(ip, 2);
}

int
dkdial(ip, serv)
	ipcinfo *ip;
	int serv;
{
	int fd;
	int listener;
	int i;
	struct listenin d;
	char dialstr[BUFSIZE];
	char dialtone[2];
	register char *dp, *cp;
	struct stat st;
	struct dialout reply;
	int traffic=0;

	/* get type of request and properties */
	for(dp = ip->param; *dp; dp++) {
		for(;*dp && *dp==' '; dp++);
		if (strncmp(dp, "heavy", 5)==0 && (dp[5]=='\0'||dp[5]==' '))
			traffic = 2;
		for(;*dp && *dp!=' '; dp++);
	}

	/* establish the initial connection */
	fd = dkchan(traffic, unit);
	if (fd < 0)
		return ABORT(EBUSY, "out of output channels", NULLINFO);
	dkproto(fd, dkp_ld);
	ioctl(fd, FIOPUSHLD, &rdk_ld);

	/* set up the parameter block for the initial request */
	fstat(fd, &st);
	d.l_type = T_SRV;
	d.l_lchan = minor(st.st_rdev);
	d.l_srv = 256 - serv;
	d.l_param0 = traffic;
	d.l_param1 = 0;
	d.l_param2 = 0;
	d.l_param3 = 0;
	d.l_param4 = 0;
	d.l_param5 = 0;

	/*
	 * convert '!'s to '.'s .  escape all '.'s after the first '!'.
	 */
	for(cp=ip->name, dp=dialstr; *cp; cp++)
		if (*cp=='!'){
			*dp++ = '.';
			cp++;
			break;
		} else
			*dp++ = *cp;
	for(; *cp; cp++)
		if (*cp=='!'){
			*dp++ = '.';
		} else if (*cp=='.'){
			*dp++ = '\\';
			*dp++ = '.';
		} else
			*dp++ = *cp;
	*dp = '\0';
	strcat(dialstr, "\n");
	strcat(dialstr, ip->user);
	strcat(dialstr, "\n");
	listener = dkctlchan();
	if (listener < 0)
		return ABORT(errno,"can't get dk control channel",NULLINFO);
	i = write(listener, (char *)&d, sizeof(d));
	close(listener);
	if (i != sizeof(d)) {
		close(fd);
		return ABORT(EIO, "can't dial out on dk", NULLINFO);
	}
	i = read(fd, dialtone, 1);
	if (i < 0) {
		close(fd);
		return ABORT(EIO, "no dial tone from dk", NULLINFO);
	}
	if (dialtone[0] != 'O') {
		close(fd);
		return ABORT(EIO, "wrong dial tone from dk", NULLINFO);
	}
	ioctl(fd, DIOCSTOP, 0);
	write(fd, dialstr, strlen(dialstr));
	i = read(fd, (char *)&reply, sizeof reply);
	if (i <= 0) {
		close(fd);
		return ABORT(EIO,"remote system doesn't respond",
				(ipcinfo *)NULL);
	}

	/* successful reply, analyze it */
	if (reply.srv!=D_OPEN) {
		close(fd);
		dktouerr(reply.param1&0x7f);
		return -1;
	}
	if (dkproto(fd, dkp_ld) < 0) {
		close(fd);
		return ABORT(EIO, "can't push line discipline",NULLINFO);
	}
	if (W_VALID(reply.param4)) {	/* check window sizes */
		char ws[5];
		long wins = W_VALUE(reply.param4)>>2;
		/* use 3 X wins/4 */
		ws[0] = wins;
		ws[1] = wins>>8;
		ws[2] = wins>>16;
		ws[3] = wins>>24;
		ws[4] = 3;
		ioctl(fd, DIOCXWIN, ws);
	}
	ioctl(fd, FIOPOPLD, 0);		/* dump rdk */
	ip->myname = "who_knows";
	return fd;
}
