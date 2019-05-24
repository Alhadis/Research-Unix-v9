#include <sys/types.h>
#include <stdio.h>
#include <ipc.h>
#include <libc.h>
#include "defs.h"

static char buf[1024];
static char logname[128];

openlog(mtpt)
	char *mtpt;
{
	int fd;
	char *cp;

	/* set up log file */
	if ((cp=strrchr(mtpt, '/'))!=NULL)
		mtpt = cp+1;
	sprintf(logname, "/usr/ipc/log/%s", mtpt);
	fd = open(logname, 1);
	if (fd<0 && errno==ENOENT)
		fd = creat(logname, 0666);
	if (fd<0) {
		fprintf(stderr, "cannot open log file, %s\n", logname);
		exit(1);
	}
	dup2(fd, 2);
	close(fd);
}

/* logging events */
logevent(format, a1, a2, a3, a4, a5, a6)
	char *format;
{
	long now=time((long *)0);
	char msg[1024];
	int fd;

	sprintf(msg, format, a1, a2, a3, a4, a5, a6);
	sprintf(buf, "%.15s %s", ctime(&now)+4, msg);
	lseek(2, 0L, 2);
	write(2, buf, strlen(buf));
}

/* logging events on both log and console */
logconsole(format, a1, a2, a3, a4, a5, a6)
	char *format;
{
	long now=time((long *)0);
	char msg[1024];
	int len;

	sprintf(msg, format, a1, a2, a3, a4, a5, a6);
	sprintf(buf, "%.15s %s", ctime(&now)+4, msg);
	len = strlen(buf);
	lseek(2, 0L, 2);
	write(2, buf, len);
	write(3, buf, len);
}

/* logging calls */
logcall(msg, ip)
	char *msg;
	ipcinfo *ip;
{
	logevent("%s %s(%s!%s) from %s!%s\n", msg, 
		(ip->flags&IPC_CREAT)?"creat":"open", ip->myname, ip->name,
		ip->machine, ip->user);
}

/* log call status */
logstatus(ip)
	ipcinfo *ip;
{
	long now=time((long *)0);

	sprintf(buf, "%.15s (%d)%s %s(%s) from %s!%s\n", ctime(&now)+4, errno,
		errstr, (ip->flags&IPC_CREAT)?"creat":"open", ip->name,
		ip->machine, ip->user);
	lseek(2, 0L, 2);
	write(2, buf, strlen(buf));
}
