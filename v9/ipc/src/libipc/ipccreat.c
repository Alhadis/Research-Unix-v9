#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <ipc.h>
#include "defs.h"

/* imports */
extern int conn_ld;
extern struct passwd *pwsearch();
extern char *strncpy();

/* global to this file */
#define ROOTUID 0

/* plug into the name space */
int
ipccreat(name, param)
	char *name;	/* name being dialed */
	char *param;	/* parameters for creation */
{
	int pfd[2];
	char *path=name;
	ipcinfo info;

	if (path==NULL)
		return ABORT(EINVAL, "name too long", (ipcinfo*)NULL);

	/* make a node to mount onto */
	if (strchr(path, '!')!=NULL || (access(path,0)<0 && creat(path, 0666)<0)) {
		/* remote creat */
		info.rfd = info.cfd = -1;
		info.myname = info.user = info.machine = NULL;
		info.uid = info.gid = -1;
		info.name = name;
		info.param = param;
		info.flags = IPC_CREAT;
		return ipcdial(&info);
	}

	/* get the stream to mount */
	if (pipe(pfd) < 0)
		return ABORT(errno, "out of pipes", (ipcinfo*)NULL);
	if (ioctl(pfd[1], FIOPUSHLD, &conn_ld) < 0) {
		close(pfd[0]);	
		close(pfd[1]);
		return ABORT(errno, "pushing line discipline", (ipcinfo*)NULL);
	}

	/* mount */
	if (fmount(3, pfd[1], path, 0) < 0) {
		close(pfd[0]);
		close(pfd[1]);
		return ABORT(errno, "can't mount", (ipcinfo*)NULL);
	}
	close(pfd[1]);
	return pfd[0];
}

/* listen for a connection */
ipcinfo *
ipclisten(fd)
	int fd;
{
	struct passfd pass;
	int pfd[2];
	static ipcinfo info;
	static char buf[BUFSIZE];
	static char user[32];
	int fd1= -1, fd2= -1;

	/* get a unique stream to the caller */
restart:
	if (_fd_read(fd, &pass)<0) {
		close(fd);
		return NULL;	/* nothing there */
	}
	info.uid = pass.uid;
	info.gid = pass.gid;
	info.rfd = pass.fd;
	strncpy(user, pass.logname, sizeof(pass.logname));
	user[sizeof(pass.logname)] = '\0';
	info.user = user;
	(void)ioctl(info.rfd, FIOACCEPT, &pass);

	/* get possible passed fds */
	if (_fd_read(info.rfd, &pass)>=0) {
		fd1 = pass.fd;
		if (_fd_read(info.rfd, &pass)>=0)
			fd2 = pass.fd;
	}

	/* get the request */
	if (_info_read(info.rfd, &info)<0) {
		/* requestor gave up */
		close(info.rfd);
		if(fd1>=0)
			close(fd1);
		if(fd2>=0)
			close(fd2);
		goto restart;
	}

	/* decode the request */
	if (info.flags & IPC_HANDOFF) {
		close(info.rfd);
		info.rfd = fd1;
		info.cfd = fd2;
		info.flags &= ~IPC_HANDOFF;
	} else {
		info.cfd = fd1;
		if (fd2>=0)
			close(fd2);
	}
	return &info;
}

/*
 *  Accept a connection.  Close all except ip->cfd.
 */
int
ipcaccept(ip)
	ipcinfo *ip;
{
	ipcdaccept(ip, -1, "who_cares");
}
/*
 *  Accept a connection, and supply a source address and communications fd
 */
int
ipcdaccept(ip, commfd, source)
	ipcinfo *ip;
	int commfd;
	char *source;
{
	if (commfd >= 0) {

		/* supply our own channel for communications */
		if (_fd_write(ip->rfd, commfd) < 0) {
			close(commfd);
			return ABORT(errno, "can't pass conection", ip);
		}
		_reply_write(ip->rfd, 0, source);
		ABORT(0, "", ip);
		ip->cfd = commfd;
	} else if (ip->cfd >= 0) {

		/* use client supplied channel for communications */
		_reply_write(ip->rfd, 0, "");
		close(ip->rfd);
		ip->rfd = -1;
	} else {

		/* use reply channel for communications */
		_reply_write(ip->rfd, 0, "");
		ip->cfd = ip->rfd;
		ip->rfd = -1;
	}
	return(ip->cfd);
}

/*  Reject a connection.
 */
int
ipcreject(ip, no, str)
	ipcinfo *ip;
	int no;		/* error number */
	char *str;	/* error string */
{
	_reply_write(ip->rfd, no, str);
	ABORT(no, str, ip);
	return 0;
}
