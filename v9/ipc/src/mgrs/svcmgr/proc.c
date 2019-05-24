/*
 * stuff concerned with the table
 * of processes we've spawned
 * almost everything in this file should go away;
 * it's needed only to keep /etc/utmp up to date
 */

#include "mgr.h"
#include <utmp.h>
#include <wait.h>
#include <libc.h>

typedef struct {
	int pid;
	char *tty;
} Proc;

#define	MAXPROC	200
Proc proc[MAXPROC];
Proc *maxproc;

/*
 * make ourselves into a new process
 * returns 0 if we're the parent, or we can't
 * returns 1 in the child
 */
newproc(rp)
	Request *rp;
{
	int pid;
	register Proc *p;
	char *ttyname(), *tty;

	if ((pid = fork()) < 0) {
		logevent("can't fork; gave up\n");
		close(rp->i->cfd);
		return (0);
	}
	if (pid == 0)
		return (1);
	tty = ttyname(rp->i->cfd);
	close(rp->i->cfd);
	if (tty==NULL)
		return (0);
	logevent("newproc(%d, %s)\n", pid, tty);
	for (p = proc; p < &proc[MAXPROC]; p++)
		if (p->pid == 0) {
			if ((p->tty = strdup(tty)) != NULL)
				p->pid = pid;
			return (0);
		}
	/* too bad */
	logevent("no internal proc; didn't bother\n");
	return (0);
}

/*
 * clean up after any dead children
 */
checkkids()
{
	int pid;

	while ((pid = wait3(NULL, WNOHANG, NULL)) > 0)
		deadproc(pid);
}

deadproc(pid)
int pid;
{
	register Proc *p;

	logevent("deadproc(%d)\n", pid);
	for (p = proc; p < &proc[MAXPROC]; p++)
		if (p->pid == pid) {
			rmutmp(p->tty);
			p->pid = 0;
			free(p->tty);
			break;
		}
}

/*
 * this should really be somewhere else ...
 */

rmutmp(tty)
char *tty;
{
	static struct utmp ut, vt;
	int fd;
	long time();

	if (strncmp(tty, "/dev/", 5) == 0)
		tty += 5;
	strncpy(vt.ut_line, tty, sizeof(vt.ut_line));
	vt.ut_time = time((long *)0);
	if ((fd = open("/etc/utmp", 2)) >= 0) {
		while (read(fd, (char *)&ut, sizeof(ut)) == sizeof(ut))
			if (strncmp(tty, ut.ut_line, sizeof(ut.ut_line)) == 0) {
				lseek(fd, (long)-sizeof(ut), 1);
				write(fd, (char *)&vt, sizeof(vt));
				break;
			}
		close(fd);
	}
	if ((fd = open("/usr/adm/wtmp", 1)) >= 0) {
		lseek(fd, 0L, 2);
		write(fd, (char *)&vt, sizeof(vt));
		close(fd);
	}
}
