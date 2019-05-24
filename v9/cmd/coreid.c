/*
 *
 * coreid [corefile] - display arguments of program that dumped core
 *
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <machine/reg.h>
#include <sys/vm.h>
#define HSIZE	ctob(UPAGES)
struct user u;
int coref;
char *core="core";
int value;
char cvalue;
unsigned enddata, startdata;
unsigned startstack;
int nchars;
long lseek();
main(argc, argv)
	char *argv[];
{
	register fp, av, ap, env;
	int *r;
	char *cp;
	int envflg = 0;

	while (argc > 1 && argv[1][0] == '-') {
		if (argv[1][1] == 'e')
			envflg++;
		else {
			fprintf(stderr, "usage: coreid [-e] [corefile]\n", core);
			exit(1);
		}
		argc--;
		argv++;
	}
	if(argc > 1)
		core=argv[1];
	coref=open(core, 0);
	if(coref < 0){
		fprintf(stderr, "coreid: can't open %s\n", core);
		exit(1);
	}
	if (read(coref, (char *)&u, sizeof(u)) != sizeof(u)){
		fprintf(stderr, "coreid: read U area %s\n", core);
		exit(1);
	}
	startdata=ctob(stoc(ctos(u.u_tsize + 1)));	/* address after text */
	enddata=startdata+ctob(u.u_dsize);		/* after data & bss */
	startstack = KERNELBASE - ctob(u.u_ssize);
	cp = (char *)&u;
	r = (int *)&cp[((int)u.u_ar0) & 0x1fff];
	fp= r[AR6];
	for(;;){
		Seek(fp);
		Read();
		if(value<fp)
			break;
		fp=value;
	}
	av=fp+20; /* Don't use arg, becuase of argv++ possibility */
	for(;;){
		Seek(av);
		Read();
		if(value==0)
			break;
		string();
		av+=sizeof(char *);
	}
	if(nchars==0)
		badcore();
	if (envflg) {
		env=fp+16;		/* It's the third argument */
		Seek(env);
		Read();
		env=value;
		for(;;){
			Seek(env);
			Read();
			if(value==0)
				break;
			printf("\n\t");
			string();
			env+=sizeof(char *);
		}
	}
	putchar('\n');
}

Seek(loc)
	register loc;
{
	if(loc > enddata){
		if(lseek(coref, (loc - startstack) + HSIZE + ctob(u.u_dsize), 0)==-1L)
			badcore();
	}else if(lseek(coref, (unsigned)loc-startdata+HSIZE, 0)==-1L)
		badcore();
}
Read()
{
	if(read(coref, &value, sizeof (int))!=sizeof (int))
		badcore();
}
Readc()
{
	if(read(coref, &cvalue, 1)!=1)
		badcore();
}
string()
{
	Seek(value);
	do{
		Readc();
		if(cvalue){
			putchar(cvalue);
			nchars++;
		}
	}while(cvalue);
	putchar(' ');
}
badcore()
{
	u.u_comm[DIRSIZ+1]=0;
	printf("(%s)\n", u.u_comm);
	exit(0);
}
