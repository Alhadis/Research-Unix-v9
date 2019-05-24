#include <sys/types.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <ipc.h>
#include "defs.h"

/* buffer definition */
#define BUFLEN 512

/* have to define this somewhere */
char *ipcname;

/*
 *  Recieve an fd
 */
_fd_read(onfd, pp)
	int onfd;
	struct passfd *pp;
{
	register int rv;

	while((rv=ioctl(onfd, FIORCVFD, pp))<0 && errno==EINTR)
		;
	return rv;
}

/*
 *  Send an fd
 */
_fd_write(onfd, tofd)
{
	return ioctl(onfd, FIOSNDFD, &tofd);
}

/*
 *  Send the connection info.
 */
int
_info_write(fd, ip)
	int fd;
	ipcinfo *ip;
{
	char b[BUFLEN];
	int n;

	if (ip->name==NULL)
		ip->name = "";
	if (ip->param==NULL)
		ip->param = "";
	if (ip->machine==NULL)
		ip->machine = "";
	if (ip->user==NULL)
		ip->user = "";
	if (ip->myname==NULL)
		ip->myname = "";
	sprintf(b, "%s\n%s\n%s\n%s\n%s\n%d\n%d\n%d\n", ip->myname, ip->name,
		ip->param, ip->machine, ip->user, ip->flags, ip->uid, ip->gid);
	n = strlen(b);
	if (write(fd, b, n)!=n)
		return ABORT(errno, "can't send request", NULLINFO);
	return 0;
}

/*
 *  Read the connection info.  If an error occurs, ip->reply and ip->conn are
 *  closed.
 */
int
_info_read(fd, ip)
	int fd;
	ipcinfo *ip;
{
	static char b[BUFLEN];
	char *f[8];
	int n;

	while((n=read(fd, b, sizeof(b)))<0 && errno==EINTR)
		;
	if (n <= 0) 
		return ABORT(errno, "error reading request", ip);
	b[n] = '\0';
	setfields("\n");
	if (getfields(b, f, 8)!=8)
		return ABORT(EINVAL, "protocol botch", ip);
	ip->myname = f[0];
	ip->name = f[1];
	ip->param = f[2];
	ip->machine = f[3];
	ip->flags = atoi(f[5]);

	/* supply a system name */
	if (ip->uid!=ROOTUID || ip->machine[0]=='\0')
		ip->machine = "";

	/* supply a user name */
	if (ip->uid==ROOTUID && *(f[4])!='\0')
		ip->user = f[4];

	/* supply uid/gid cruft */
	if (ip->uid==ROOTUID && atoi(f[6])!=-1) {
		ip->uid = atoi(f[6]);
		ip->gid = atoi(f[7]);
	}
	return 0;
}

/*
 *  Send a reply to a connection request
 */
_reply_write(fd, no, str)
	int fd;
	int no;
	char *str;
{
	char b[BUFLEN];
	int n;

	if (str==NULL)
		str = "";
	sprintf(b, "%d\n%s\n", no, str);
	n = strlen(b);
	if (write(fd, b, n)!=n)
		return -1;
	return 0;
}

/*
 *  Get a reply to a connection request.
 */
int
_reply_read(fd)
	int fd;
{
	static char b[BUFLEN];
	char *f[2];
	char *ptr;
	int n;

	while((n=read(fd, b, sizeof(b)))<0 && errno==EINTR)
		;
	if (n <= 0) 
		return ABORT(errno, "error reading request", NULLINFO);
	b[n] = '\0';
	setfields("\n");
	getfields(b, f, 2);
	errno = atoi(f[0]);
	if ((ptr=strchr(f[1], '\n'))!=NULL)
		*ptr = '\0';
	if (errno!=0)
		errstr = f[1];
	else
		ipcname = f[1];
	return 0;
}

