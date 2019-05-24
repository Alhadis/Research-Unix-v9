#include <stdio.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

/*
 * bsd semi-equivalent of Ninth Edition hang
 *
 * D. A. Kapilow		12/18/87
 */

main(argc,argv)
int argc; char **argv;
{
	int pid, wpid;
	union wait status;

	if (argc <= 1) {
		fprintf(stderr,"Usage: %s cmd [args...]\n",*argv);
		exit(1);
	}
	if ((pid = fork()) == 0) {
		ptrace(PTRACE_TRACEME, 0, 0, 0, 0);
		execvp(argv[1], argv+1);
		perror(argv[1]);
		exit(1);
	}
	if (pid < 0) {
		fprintf(stderr,"%s: can't fork\n",*argv);
		exit(1);
	}
	while ((wpid = wait(&status)) != pid)
		;
	if (!WIFSTOPPED(status) || status.w_stopsig != SIGTRAP) {
		fprintf(stderr,"%s: child died\n",*argv);
		exit(1);
	}
	kill(pid, SIGSTOP);
	ptrace(PTRACE_DETACH, pid, (int *)1, 0, 0);
	fprintf(stderr, "%d %s\n", pid, argv[1]);
	while ((wpid = wait(&status)) != pid)
		if (wpid < 0)
			exit(1);
}
