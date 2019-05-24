#include <sys/types.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <ipc.h>
#include <libc.h>
#include "defs.h"

/* preeclared */
void dodialin();

/* global */
int pid;

/* imported */
extern char *av0;
extern int net_announce();
extern void net_accept();
extern void net_reject();
extern ipcinfo *net_listen();

/* take alarm */
static int
ding() {
	signal(SIGALRM, ding);
	logevent("time out\n");
	return;
}

/* loop on calls into the CPU */
void
dodialin(mtpt, netname, param)
	char *mtpt;
	char *netname;
	char *param;
{
	int fd;
	ipcinfo info;

	pid = getpid();

	/* plug into network name space */
	for(;;){
		info.name = netname;
		info.user = getlogin();
		info.param = param;
		info.rfd = info.cfd = -1;
		signal(SIGALRM, ding);
		alarm(60);
		fd = net_announce(&info);
		alarm(0);
		if (fd>=0)
			break;
		logconsole("%s: can't announce to network as %s (%s)\n",
			av0, netname, errstr);
		sleep(60);
	}
	logconsole("%s: announced to network as %s\n", av0, netname);

	/* loop on requests */
	for(;;) {
		ipcinfo *ip;
		int rv;
		static stretch newsrc;
		static stretch newmyname;

		ip = net_listen(fd);
		if (ip == NULL) {
			logevent("broken listen (%s)\n", errstr);
			break;
		}
		ip->flags |= IPC_CAREFUL;
		if (ip->name==NULL || *ip->name=='\0')
			ip->name = "login";
		_strcat(&newsrc, mtpt, "!", ip->machine);
		ip->machine = newsrc.ptr;
		_strcat(&newmyname, mtpt, "!", ip->myname);
		ip->myname = newmyname.ptr;
		logcall("callin", ip);
		if (ip->flags & IPC_CREAT) {
			net_reject(ip, EINVAL, "bad request");
			logstatus(ip);
			continue;
		}
		signal(SIGALRM, ding);
		alarm(30);	/* avoid single-thread deadlock */
		rv = ipcdial(ip);
		alarm(0);
		if (rv<0) {
			net_reject(ip, errno, errstr);
			logstatus(ip);
			continue;
		} else
			close(rv);
		errno = 0; errstr = "";
		logstatus(ip);
		net_accept(ip);
	}
	close(fd);
}

