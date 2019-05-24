#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <ipc.h>
#include "defs.h"
#include <sys/inet/tcp_user.h>
#include <libc.h>
#include <sys/param.h>
#include <sys/ioctl.h>

#define CSPORT 1

/* imported */
extern char *in_ntoa();
extern int tcp_connect();
extern int tcp_accept();
extern int tcp_listen();
extern char *in_host();
extern in_addr in_address();
extern int rmesg_ld;

/* dial a service on the internet - try first via cs, then direct */
int
net_dial(ip)
	ipcinfo *ip;
{
	char *service;
	struct tcpuser tu;
	in_addr faddr;
	int fd;

	/* parse the name */
	service = strchr(ip->name, '!');
	if (service != NULL)
		*service++ = '\0';
	else
		return ABORT(ENOENT, "unassigned destination", NULLINFO);
	faddr = in_address(ip->name);
	if (faddr==INADDR_ANY)
		return ABORT(ENOENT, "unassigned destination", NULLINFO);

	/* get a channel for the connection */
	fd = tcp_sock();
	if (fd < 0)
		return ABORT(EBUSY, "out of output channels", NULLINFO);

	/* connect to the remote connection server */
	tu.laddr = INADDR_ANY;
	tu.lport = 0;
	tu.faddr = faddr;
	tu.fport = CSPORT;
	tu.param = 0;
	errstr = "destination not answering";
	if (tcp_connect(fd, &tu)>=0) {
		/* send the request */
		ip->name = service;
		if (_info_write(fd, ip)==0) {
			condition(fd, ip);
			if (_reply_read(fd)==0 && errno==0) {
				ip->myname = in_ntoa(tu.laddr);
				return fd;
			}
		}
	}
	close(fd);

	/* no connection server, try the port directly */
	fd = tcp_sock();
	if (fd < 0)
		return ABORT(EBUSY, "out of output channels", NULLINFO);
	tu.laddr = INADDR_ANY;
	tu.lport = 0;
	tu.faddr = faddr;
	tu.fport = fstotcp(service);
	if (tu.fport<=0) {
		close(fd);
		return ABORT(ENOENT, errstr, NULLINFO);
	}
	tu.param = 0;
	if (tcp_connect(fd, &tu)<0) {
		close(fd);
		return ABORT(ENOENT, errstr, NULLINFO);
	}
	condition(fd, ip);
	ip->myname = in_ntoa(tu.laddr);
	return fd;
}

/* condition the connection as requested */
condition(fd, ip)
	int fd;
	ipcinfo *ip;
{
	char *fields[10];
	char param[128];
	int i;
	int dohup=0;
	int domesg=0;

	strcpy(param, ip->param);
	setfields(" \t");
	getmfields(param, fields, 10);
	for (i=0; i<10 && fields[i]; i++) {
		if (strcmp(fields[i], "hup")==0)
			dohup = 1;
		if (strcmp(fields[i], "delim")==0)
			domesg = 1;
	}
	if (dohup)
		ioctl(fd, TCPIOHUP, 0);
	if (domesg)
		ioctl(fd, FIOPUSHLD, &rmesg_ld);
}

/* announce onto the internet - only the first one gets there */
int
net_announce(ip)
	ipcinfo *ip;
{
	int fd;
	struct tcpuser tu;

	USE(ip);
	fd = tcp_sock();
	if (fd < 0)
		return ABORT(EBUSY, "no more output channels", NULLINFO);
	tu.lport = TCPPORT_ANY;
	tu.laddr = 0;
	tu.fport = 0;
	tu.faddr = 0;
	tu.param = 0;
	if (tcp_listen(fd, &tu)<0) {
		close(fd);
		return ABORT(EEXIST, "server already exists", NULLINFO);
	}
	return fd;
}

/* listen for a call in */
ipcinfo *
net_listen(fd)
	int fd;
{
	static ipcinfo info;
	static char myname[PATHLEN];
	static char name[PATHLEN];
	static char machine[PATHLEN];
	struct tcpuser tu;
	extern char *tcptofs();

	info.flags = 0;
	for(;;) {
		tu.param = 0;
		if ((fd = tcp_accept(fd, &tu))<0)
			return NULL;
		if (tu.lport == CSPORT) {
			if (tu.fport > 1023)
				info.uid = info.gid = -1;
			else
				info.uid = info.gid = 0;
			info.user = "_unknown_";
			if (_info_read(fd, &info)<0) {
				close(fd);
				return (ipcinfo *)NULL;
			}
			info.flags |= IPC_HANDOFF;
			condition(fd, &info);
		} else {
			info.user = "_unknown_";
			info.param = "";
			strcpy(name, tcptofs(tu.lport));
			info.name = name;
		}
		strcpy(machine, in_host(tu.faddr));
		info.machine = machine;
		strcpy(myname, in_ntoa(tu.laddr));
		info.myname = myname;
		info.flags |= IPC_OPEN;
		info.rfd = fd;
		info.cfd = -1;
		return &info;
	}
}

/* accept a call */
void
net_accept(ip)
	ipcinfo *ip;
{
	USE(ip);
}

/* reject a call - null if a non-cs call */
void
net_reject(ip, no, str)
	ipcinfo *ip;
	int no;
	char *str;
{
	if (ip->flags&IPC_HANDOFF && ip->rfd>=0)
		_reply_write(ip->rfd, no, str);
	if (ip->rfd>=0) {
		close(ip->rfd);
		ip->rfd = -1;
	}
}
