#include <sys/types.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <ipc.h>
#include <libc.h>
#include <wait.h>
#include "defs.h"

/* preeclared */
void dodialout();
void dodialin();
void doopen();
void docreat();

/* global */
int pid;

/* imported */
extern char *av0;
extern int net_dial();
extern int net_announce();
extern void net_accept();
extern void net_reject();
extern ipcinfo *net_listen();
extern char *getlogin();

static
deadbaby()
{
	while(wait3(NULL, WNOHANG, NULL)>0)
		;
	signal(SIGCHLD, deadbaby);
}

/* loop on calls out of the CPU */
void
dodialout(mtpt)
	char *mtpt;
{
	int fd;
	int st;

	pid = getpid();

	/* plug into local name space */
	for(st=1;;st=st>60?st:(st<<1)){
		fd = ipccreat(mtpt, "");
		if (fd>=0)
			break;
		logconsole("%s: can't announce as %s (%s)\n", av0, mtpt, errstr);
		sleep(st);
	}
	chmod(mtpt, 0666);
	logconsole("%s: announced to fs as %s\n", av0, mtpt);
	fflush(stdout);

	/* loop on requests */
	for(;;) {
		ipcinfo *ip;

		ip = ipclisten(fd);
		if (ip == NULL) {
			fprintf(stderr,"out broken listen\n");
			break;
		}

		/* run request as separate process */
		signal(SIGCHLD, deadbaby);
		switch(fork()) {
		case -1:		/* whoops */
			ipcreject(ip, errno, "no more processes");
			logstatus(ip);
			continue;
		case 0:
			close(fd);
			break;
		default:
			(void)ABORT(0, "", ip);
			continue;
		}

		logcall("callout", ip);
		if (ip->flags & IPC_CREAT)
			docreat(ip);
		else
			doopen(ip, mtpt);
		exit(0);
	}
	close(fd);
}

/* establish a connection to a net name */
void
doopen(ip, mtpt)
	ipcinfo *ip;
	char *mtpt;
{
	int fd;
	static stretch myname;

	*av0 = 'D';

	fd = net_dial(ip);
	if (fd < 0) {
		ipcreject(ip, errno, errstr);
		logstatus(ip);
		return;
	}
	errno = 0; errstr = "";
	_strcat(&myname, mtpt, "!", ip->myname);
	ipcdaccept(ip, fd, myname.ptr);
	close(fd);
	logstatus(ip);
}

/* announce a new netname */
void
docreat(ip)
	ipcinfo *ip;
{
	ipcinfo *netip;
	int listenfd;
	int toclient;
	int pfd[2];
	fd_set fds;

	if (ip->cfd >= 0) {
		ipcreject(ip, EIO, "can't do remote ipccreat");
		logstatus(ip);
		return;
	}

	*av0 = 'L';

	/* for communications with requestor */
	if (pipe(pfd) < 0) {
		ipcreject(ip, errno, "can't create local channel");
		logstatus(ip);
		return;
	}
	toclient = pfd[1];

	/* dial out on device */
	listenfd = net_announce(ip);
	if (listenfd < 0) {
		close(pfd[0]);
		close(pfd[1]);
		ipcreject(ip, errno, errstr);
		logstatus(ip);
		return;
	}

	/* accept the announce request */
	if (ipcdaccept(ip, pfd[0], "who_knows") < 0)
		return;
	close(pfd[0]);
	errno = 0; errstr = "";
	logstatus(ip);

	/* loop waiting for in calls */
	FD_ZERO(fds);
	FD_SET(listenfd, fds);
	FD_SET(toclient, fds);
	while(1) {
		fd_set rfds;
		int rv;

		/* check for input or hang-up */
		rfds = fds;
		rv = select(NOFILE, &rfds, (fd_set*)0, 10000);
		if (rv == 0)
			continue;
		else if (rv < 0)
			break;
		else if (FD_ISSET(toclient, rfds))
			break;

		/* get request */
		netip = net_listen(listenfd);
		if (netip == NULL) {
			ABORT(errno, errstr, ip);
			break;
		}
		logcall(ip->name, netip);

		/* make a new channel to the listener */
		if (pipe(pfd)<0) {
			net_reject(netip, errno, "no more pipes");
			logstatus(ip);
			continue;
		}
		if (ioctl(toclient, FIOSNDFD, &(pfd[0]))<0) {
			net_reject(netip, errno, "protocol botch");
			logstatus(ip);
			break;
		}
		close(pfd[0]);

		/* pass the request over the new channel */
		if (ioctl(pfd[1], FIOSNDFD, &(netip->rfd))<0) {
			net_reject(netip, errno, "protocol botch");
			close(pfd[1]);
			logstatus(ip);
			continue;
		}
		close(netip->rfd);
		netip->rfd = -1;
		if (_info_write(pfd[1], netip) < 0) {
			net_reject(netip, errno, "protocol botch");
			close(pfd[1]);
			logstatus(ip);
			continue;
		}
		if (_reply_read(pfd[1]) < 0)
			net_reject(netip, errno, "protocol botch");
		else
			net_accept(netip);
		logstatus(ip);
		close(pfd[1]);
	}
	return;
}
