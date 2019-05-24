/*
 *	Generic dialer routine.  Usage:
 *		dialer mount-pt [unit]
 */
#include <stdio.h>
#include <ipc.h>
#include <sys/filio.h>
#include <libc.h>

/* export */
char *av0;
int debug;

/* cheap foiegn imports */
extern void dodialout(), dodialin();

usage(name)
	char *name;
{
	fprintf(stderr, "usage: %s [-m mount-pt]\n", name);
	exit(1);
}

main(ac, av)
	int ac;
	char *av[];
{
	char *mtpt="tcp", *cp;
	int ai;

	av0 = av[0];
	chdir("/cs");
	for (ai=1; ai<ac; ai++) {
		if (av[ai][0] == '-')
			for (cp= &av[ai][1]; *cp; cp++) {
				switch(*cp) {
				case 'd':
					debug = 1;
					break;
				case 'm':
					if (ai+1>=ac || *cp!='m')
						usage(av0);
					mtpt = av[++ai];
					break;
				default:
					usage(av0);
				}
			}
		else
			usage(av[0]);
	}
	if (!debug)
		detach(mtpt);

	tcpconfig();

	/* create dialer and listener */
	switch (fork()) {
	case -1:
		perror(av0);
		exit(1);
	case 0:
		for(;;)
			dodialout(mtpt);
		break;
	default:
		for(;;)
			dodialin(mtpt, "seymour cray", "");
		break;
	}
}

#define IPDEVFORTCP "/dev/iptcp"
tcpconfig()
{
	int fd;
	extern int tcp_ld;

	fd = open(IPDEVFORTCP, 2);
	if(fd < 0)
		return -1;
	if(ioctl(fd, FIOPUSHLD, &tcp_ld) < 0) {
		close(fd);
		return -1;
	}
	logevent("tcp active\n");
	return 0;
}
