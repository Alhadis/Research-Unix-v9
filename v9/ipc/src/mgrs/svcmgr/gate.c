#include "mgr.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/filio.h>

/*
 *  Dial out to a gateway.  Return -1 if no more gates are to be tried.
 *  Return 0 if more gates are to be tried.
 */
gateout(rp, ap)
	Request *rp;
	Action *ap;
{
	int fd;
	extern int rmesg_ld;

	if (rp->i->cfd>=0) {
		ipcreject(rp->i, EINVAL, "gate-through disallowed");
		return -1;
	}

	/* call `gateway' */
	fd = ipcopen(ap->arg, rp->i->param);

	/* send original request */
 	if (fd<0 || _info_write(fd, rp->i)<0) {
		/* if there are any more gateout's, keep trying */
		if (ap->next==NULL)
			ipcreject(rp->i, errno, errstr);
		close(fd);
		return 0;
	}

	/* see if the gateway could place the call */
	if (_reply_read(fd)<0 || errno!=0 || ioctl(fd, FIOPUSHLD, &rmesg_ld)<0) {
		/* call was rejected, don't try any more gateouts */
		ipcreject(rp->i, errno, errstr);
		close(fd);
		return -1;
	}

	/* gateway and call were accepted -- go for it */
	ipcdaccept(rp->i, fd, "who_cares");
	return -1;
}

/*
 *  Acccept a gateway call
 */
gateway(rp, ap)
	Request *rp;
	Action *ap;
{
	int caller, callee;
	ipcinfo info;
	fd_set fds;
	char newname[ARB];
	extern int mesg_ld;
	char *mapuser();

	/* see if we gateway for this requestor */
	if (mapuser(rp->s->name, rp->i->machine, rp->i->user)==NULL) {
		ipcreject(rp->i, EACCES, "gateway disallowed");
		return -1;
	}
	if ((caller=ipcaccept(rp->i))<0)
		return -1;

	/* get the original request */
	info.uid = info.gid = 0;
	if (_info_read(caller, &info)<0)
		return -1;

	/* dial the number */
	sprintf(newname, "%s!%s", ap->arg, info.name);
	info.name = newname;
	info.rfd = info.cfd = -1;
	info.flags = IPC_OPEN;
	callee = ipcdial(&info);
	if (callee<0 || ioctl(callee, FIOPUSHLD, &rmesg_ld)<0) {
		_reply_write(caller, errno, errstr);
		close(caller);
		return -1;
	}

	/* tell gateout that it worked */
	if (_reply_write(caller, 0, "")<0) {
		close(callee);
		return -1;
	}

	/* shuttle bytes back and forth */
	FD_ZERO(fds);
	for(;;) {
		FD_SET(caller, fds);
		FD_SET(callee, fds);
		switch(select(NOFILE, &fds, (struct fd_set *)0, 1000)) {
		case -1:
			return -1;
		case 0:
			continue;
		}
		if (FD_ISSET(caller, fds))
			if (pass(caller, callee)<0)
				return -1;
		if (FD_ISSET(callee, fds))
			if (pass(callee, caller)<0)
				return -1;
	}
}

pass(from, to)
	int from, to;
{
	char buf[ARB];
	int n;

	if ((n=read(from, buf, ARB))<=0)
		return -1;
	if (write(to, buf, n)!=n)
		return -1;
	return 0;
}
