#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <ipc.h>
#include "defs.h"

/* exported */
char *errstr;

/* imports */
extern int atoi();
extern char *memchr();

/* dial a name and return an fd to the connection */
int
ipcopen(name, param)
	char *name;	/* name being dialed */
	char *param;	/* parameters for dialer */
{
	ipcinfo info;

	info.rfd = info.cfd = -1;
	info.myname = info.user = info.machine = NULL;
	info.uid = info.gid = -1;
	info.name = name;
	info.param = param;
	info.flags = IPC_OPEN;
	return ipcdial(&info);
}

/*
 *  Open a mounted steam.  ip->name points to a string of the form
 *  path!address.  Path must be a file system path to a mounted stream.
 */ 
_ipcopen(ip)
	ipcinfo *ip;
{
	static stretch path;
	char *np;
	int fd;
	struct stat sbuf;

	/* split into path and network address */
	_strcat(&path, ip->name, (char *)NULL, (char *)NULL);
	np = strchr(path.ptr, '!');
	if (np) {
		*np = '\0';
		ip->name += np - path.ptr + 1;
	}
	if (ip->flags&IPC_CAREFUL) {
		if (stat(path.ptr, &sbuf)<0)
			return ABORT(ENOENT, "destination nonexistent", NULLINFO);
		if (sbuf.st_mode&6 != 6)
			return ABORT(EACCES, "permission denied", NULLINFO);
	}
	if ((fd = open(path.ptr, 2))<0) {
		if (errno==EACCES){
			if (*(ip->name))
			    return ABORT(EACCES, "can't access dialer", NULLINFO);
			else
			    return ABORT(EACCES, "permission denied", NULLINFO);
		} else
			return ABORT(ENOENT, "destination nonexistent", NULLINFO);
	}

	/* update the translated part of the name */
	if(ip->myname)
		_strcat(&path, ip->myname, "!", path.ptr);
	ip->myname = path.ptr;
	return fd;
}

/* dial a name and return an fd to the connection */
int
ipcdial(ip)
	ipcinfo *ip;
{
	struct passfd pass;
	int fd;

	if ((fd = _ipcopen(ip))<0)
		return -1;

	/* pass reply channel */
	if (ip->rfd >= 0) {
		if (_fd_write(fd, ip->rfd)<0) {
			close(fd);
			return ABORT(EIO, "protocol botch", ip);
		}
		close(ip->rfd);
		ip->rfd = -1;
	}

	/* pass communications channel (if not same as reply channel) */
	if (ip->cfd >= 0) {
		if (_fd_write(fd, ip->cfd)<0) {
			close(fd);
			return ABORT(EIO, "protocol botch", ip);
		}
		close(ip->cfd);
		ip->cfd = -1;
	}

	/* pass the info on */
	if (_info_write(fd, ip) < 0) {
		close(fd);
		return ABORT(errno, errstr, ip);
	}
	if (ip->flags&IPC_HANDOFF)
		return fd;

	/* get the reply  */
	if (_fd_read(fd, &pass)>=0) {
		_reply_read(fd);
		if (errno != 0) {
			close(fd);
			close(pass.fd);
			return ABORT(errno, errstr, NULLINFO);
		}
		return pass.fd;
	} else {
		_reply_read(fd);
		if (errno != 0) {
			close(fd);
			return ABORT(errno, errstr, NULLINFO);
		}
		return fd;
	}
}

/*
 *  Pass a request to someone else to handle.
 */
ipcpass(ip)
	ipcinfo *ip;
{
	ip->flags |= IPC_HANDOFF;
	return ipcdial(ip);
}

/* set error number and string and return -1 */
int
_ipcabort(no, err, ip)
	int no;
	char *err;
	ipcinfo *ip;
{
	if (ip!=NULLINFO) {
		if (ip->cfd>0) {
			close(ip->cfd);
			ip->cfd = -1;
		}
		if (ip->rfd>0) {
			close(ip->rfd);
			ip->rfd = -1;
		}
	}
	errstr = err;
	errno = no;
	return -1;
}
