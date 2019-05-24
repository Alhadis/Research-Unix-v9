/*
 *	Generic dialer routine.  Usage:
 *		dialer mount-pt [unit]
 */
#include <stdio.h>
#include <ipc.h>
#include <libc.h>
#include <sys/utsname.h>
#include "defs.h"

/* export */
char *av0;
int unit='a';		/* a(ny), 0, 2, or b(oth) */
char *param;
int debug;

/* cheap foiegn imports */
extern void dodialout(), dodialin();

usage(name)
	char *name;
{
	fprintf(stderr, "usage: %s [-m mount-pt] [-n netname] [-u [02ab]] [-t [02]]\n", name);
	exit(1);
}

main(ac, av)
	int ac;
	char *av[];
{
	char *netname=NULL, *mtpt="dk", *cp;
	int ai;

	av0 = av[0];
	chdir("/cs");

	/* get args */
	for (ai=1; ai<ac; ai++) {
		if (av[ai][0] == '-')
			for (cp=&av[ai][1]; *cp; cp++) {
				switch(*cp) {
				case 'm':
					if (ai+1>=ac)
						usage(av[0]);
					mtpt = av[++ai];
					break;
				case 'n':
					if (ai+1>=ac)
						usage(av[0]);
					netname = av[++ai];
					break;
				case '0':
				case '2':
				case 'b':
				case 'a':
					unit = *cp;
					break;
				case 'd':
					debug = 1;
					break;
				case 't':
					if (ai+1>=ac)
						usage(av[0]);
					switch(*av[++ai]) {
					case '0':
						param = "light";
						break;
					case '2':
						param = "heavy";
						break;
					default:
						usage(av[0]);
					}
					break;
				case 'u':
					if (ai+1>=ac)
						usage(av[0]);
					switch(*av[++ai]) {
					case 'a':
					case 'b':
					case '0':
					case '2':
						unit = *av[ai];
						break;
					default:
						usage(av[0]);
					}
					break;
				default:
					usage(av0);
				}
			}
		else
			usage(av0);
	}
	if (!debug)
		detach(mtpt);

	/* get defaults */
	if (netname==NULL) {
		struct utsname name;

		uname(&name);
		netname = name.nodename;
	}

	/* create dialer and listener */
	switch (fork()) {
	case -1:
		perror(av0);
		exit(1);
	case 0:
		for(;;)
			dodialout(mtpt);
	default:
		if (unit == 'b')
			switch(fork()) {
			case -1:
				perror("dkdialer");
				exit(1);
			case 0:
				if (param==NULL)
					param = "heavy";
				unit = '2';
				for(;;)
					dodialin(mtpt, netname, param);
			default:
				if (param==NULL)
					param = "light";
				unit = '0';
				break;
			}
		if (param==NULL)
			param = "heavy";
		for(;;)
			dodialin(mtpt, netname, param);
	}
}
