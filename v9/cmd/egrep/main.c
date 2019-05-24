#include	"hdr.h"

State states[NSTATES];
State *nxtst();
int state[NSTATES];
int line = 1;
int name[MAXLIN];
int left[MAXLIN];
int right[MAXLIN];
int parent[MAXLIN];
int foll[MAXLIN];
int positions[MAXPOS];
char chars[MAXLIN];
int nxtpos = 0;
int inxtpos;
int nxtchar = 0;
int tmpstat[MAXLIN];
int begstat[MAXLIN];
int colpos[MAXLIN];
State *istat;
int nstate = 1;
int xstate;
int count;
int icount;
char *input;

char reinit = 0;

long	lnum;
int	bflag;
int	cflag;
int	fflag;
int	hflag = 1;
int	iflag;
int	lflag;
int	nflag;
int	sflag;
int	vflag;
int	nfile;
long	tln;
int	nsucc;
int	badbotch;

int	expfile;
int 	bmegrep = 0;

main(argc, argv)
char **argv;
{
	int (*fn)(), execute(), bmexecute();
	int etext();
	char buf[2048];


	while (--argc > 0 && (++argv)[0][0]=='-')
		switch (argv[0][1]) {

		case 'b':
			bflag++;
			continue;

		case 'c':
			cflag++;
			continue;

		case 'e':
			argc--;
			argv++;
			goto cut;

		case 'f':
			fflag++;
			continue;

		case 'h':
			hflag = 0;
			continue;

		case 'i':
			iflag++;
			continue;

		case 'l':
			lflag++;
			continue;

		case 'n':
			nflag++;
			continue;

		case 's':
			sflag++;
			continue;

		case 'v':
			vflag++;
			continue;

		default:
			fprint(2, "egrep: unknown flag\n");
			continue;
		}
cut:
	if (argc<=0)
		exit(2);
	if (fflag) {
		if ((expfile = open(*argv, 0)) < 0) {
			fprint(2, "egrep: can't open %s\n", *argv);
			exit(2);
		}
	}
	else input = *argv;
	Finit(1, (char *)0);
	argc--;
	argv++;

	/*mailprep();/**/
	yyparse();
maildone();
	if(!vflag && islit(buf)){
		bmprep(buf);
		fn = bmexecute;
	} else
		fn = execute;

	cgotofn();
	nfile = argc;
	if (argc<=0) {
		if (lflag) exit(1);
		(*fn)((char *)0);
	}
	else while (--argc >= 0) {
		if (reinit == 1) clearg();
		(*fn)(*argv++);
	}
	exit(badbotch ? 2 : nsucc==0);
}
