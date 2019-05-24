#include <stdio.h>
#include <ctype.h>
#include <sys/inet/tcp_user.h>

int debug;

main(argc, argv)
int argc;
char **argv;
{
	int myport;
	int mysock;
	struct in_service *sp;
	struct tcpuser tu;
	char *lname;
	char *cmd;
	char *catargs();

	if (argc < 3) {
		fprintf(stderr, "usage: tcpd service login [ cmd ]\n");
		exit(1);
	}
	if (isdigit(argv[1][0]))
		myport = atoi(argv[1]);
	else {
		sp = in_service(argv[1], "tcp", 0);
		if (sp == NULL) {
			fprintf(stderr, "tcpd: unlisted service %s\n", argv[1]);
			exit(1);
		}
		myport = sp->port;
	}
	tu.laddr = 0;
	tu.lport = myport;
	lname = argv[2];
	if (argc > 3 && strcmp(argv[3], "-x") == 0) {
		debug++;
		argv++;
		argc--;
	}
	cmd = catargs(&argv[3], argc - 3);
	if (debug)
		fprintf(stderr, "starting: %d: %s: %s\n", myport, lname, cmd);
	if ((mysock = tcp_sock()) < 0) {
		fprintf(stderr, "tcpd: no socket\n");
		exit(1);
	}
	tu.fport = 0;
	tu.faddr = 0;
	tu.param = 0;
	if (tcp_listen(mysock, &tu) < 0) {
		fprintf(stderr, "tcpd: can't listen\n");
		exit(1);
	}
	for (;;)
		daemon(mysock, lname, cmd);
}

char *
catargs(ap, n)
register char **ap;
register int n;
{
	static char buf[200];

	if (n <= 0)
		return (NULL);
	while (--n >= 0)
		strcat(buf, *ap++);
	return (buf);
}

daemon(s, lname, cmd)
int s;
char *lname;
char *cmd;
{
	int fd;
	int pid;
	struct tcpuser tu;

	tu.lport = 0;
	tu.laddr = 0;
	tu.fport = 0;
	tu.faddr = 0;
	tu.param = 0;
	if ((fd = tcp_accept(s, &tu)) < 0) {
		fprintf(stderr, "uucpd: can't accept\n");
		perror("accept");
		return;
	}
	if (debug)
		logconn(tu.faddr, tu.fport, fd);
	if ((pid = fork()) < 0) {
		fprintf(stderr, "uucpd: can't fork\n");
		close(fd);
		return;
	}
	else if (pid != 0) {
		close(fd);
		while (wait((int *)0) != pid)
			;
		return;
	}
	if ((pid = fork()) != 0)
		_exit(1);
	close(s);
	dup2(fd, 0);
	dup2(0, 1);
	dup2(0, 2);
	dup2(0, 3);
	execl("/etc/login", "login", "-f", lname, cmd ? cmd: NULL, NULL);
	perror("tcpd login");
	_exit(1);
}

logconn(fa, fp, fd)
in_addr fa;
tcp_port fp;
int fd;
{
	long t;

	time(&t);
	fprintf(stderr, "%.15s conn from %s port %d fd %d\n",
		ctime(&t)+4, in_ntoa(fa), fp, fd);
}
