#include "mgr.h"
#include <stdio.h>
#include <signal.h>
#include <libc.h>

main(ac, av)
	int ac;
	char *av[];
{
	chdir("/cs");
	if (ac==1 || strcmp(av[1], "-d")!=0)
		init(0);
	else
		init(1);
	logevent("started\n");
	calls();
}

deadbabies()
{
	checkkids();
	signal(SIGCHLD, deadbabies);
}

init(debug)
{
	register char **e;
	extern char **environ;
	extern char **newep;
	extern char *newenv[];

	if (!debug)
		detach("svc");
	statfiles();
	readfiles();
	signal(SIGCHLD, deadbabies);
	e = environ;
	newep = newenv;
	for (; *e; e++)
		if (strncmp(*e, "TZ=", 3) == 0)
			*newep++ = *e;
	*newep = NULL;
}

calls()
{
	register Request *rp;
	Request *listen();

	for (;;) {
		rp = listen();
		if (newproc(rp) == 0)
			continue;
		doreq(rp);
		exit(1);	/* we shouldn't get here if the request works */
	}
}
