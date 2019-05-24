#ifndef lint
static	char sccsid[] = "@(#)cc.c 1.1 86/02/03 SMI"; /* from UCB 4.7 83/07/01 */
#endif
/*
 * cc - front end for C compiler
 */
#include <sys/param.h>
#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include <sys/dir.h>

char	*cpp = "/lib/cpp";
char	*count = "/usr/lib/bb_count";
char	*ccom = "/lib/ccom";
char	*c2 = "/lib/c2";
char	*as = "/bin/as";
char	*ld = "/bin/ld";
char	*libc = "-lc";
char	*crt0 = "/lib/crt0.o";
char	*gcrt0 = "/lib/gcrt0.o";
char	*mcrt0 = "/lib/mcrt0.o";

char	tmp0[30];		/* big enough for /tmp/ctm%05.5d */
char	*tmp1, *tmp2, *tmp3, *tmp4, *tmp5, *tmp6;
char	*outfile;
char	*savestr(), *strspl(), *setsuf();
int	idexit();
char	**av, **clist, **llist, **plist;
int	cflag, eflag, oflag, pflag, sflag, wflag, Rflag, exflag, proflag;
int	aflag;
int	gproflag, gflag, Gflag;
int	vflag;

/*
 * Added to support NRTX
 */
char	*libdir;
char	*expandlib();
int nrtxflag;
int nofloatcrtflag;
#ifdef mc68000

/*
 * starting in release 3.0, we must figure out what kind of processor
 * we are running on, and generate code accordingly.  This requires
 * some magic routines from libc.a
 */
int	is68020();	/* returns 1 if the host is a 68020 */

struct mach_info { 
	char *optname; 
	int  found; 
	int  isatype;
	char *crt1;
};

struct mach_info machopts[] = {
	"-m68010",   0,	1,  (char*)0,	/* use 68010 subset */
	"-m68020",   0, 1,  (char*)0,	/* use 68020 extensions */
	(char*)0
};

struct mach_info floatopts[] = {
	"", /*fpa*/  0, 1,  "Wcrt1.o",	/* sun fpa */
	"-f68881",   0, 1,  "Mcrt1.o",	/* 68881 */
	"-fsky",     0, 1,  "Scrt1.o",	/* sky board */
	"-fsoft",    0, 1,  "Fcrt1.o",	/* software floating point */
	"-fswitch",  0, 1,  (char*)0,	/* switched floating point */
	"-fsingle",  0, 0,  (char*)0,	/* single precision float */
	"-fsingle2", 0, 0,  (char*)0,	/* pass float args as floats */
	(char *)0 ,
};

extern char *getenv();
char *FLOAT_OPTION = "FLOAT_OPTION";
struct mach_info *machtype=NULL;	/* selected target machine type */
struct mach_info *fptype=NULL;		/* selected floating pt machine type */
struct mach_info *default_machtype();	/* default target machine type */
struct mach_info *default_fptype();	/* default floating point machine */

#define M_68010  &machopts[0]
#define M_68020  &machopts[1]
#define F_fpa    &floatopts[0]
#define F_68881  &floatopts[1]
#define F_sky    &floatopts[2]
#define F_soft   &floatopts[3]
#define F_switch &floatopts[4]

#define use68010  (machtype == M_68010)
#define use68020  (machtype == M_68020)

#define unsupported(machtype, fptype) \
	( machtype == M_68010 && fptype == F_fpa \
	|| machtype == M_68020 && fptype == F_sky )

#endif
char	*dflag;
int	exfail;
char	*chpass;
char	*npassname;
char	*ccname;

int	nc, nl, np, nxo, na;

#define	cunlink(s)	if (s) unlink(s)

main(argc, argv)
	register int argc;
	register char **argv;
{
	char *t;
	char *assource;
	register int i, j, c, tmpi;
	register struct mach_info *mp;
	int optfound;

	/* ld currently adds upto 5 args; 10 is room to spare */
	av = (char **)calloc(argc+20, sizeof (char **));
	clist = (char **)calloc(argc, sizeof (char **));
	llist = (char **)calloc(argc, sizeof (char **));
	plist = (char **)calloc(argc, sizeof (char **));
	ccname = argv[0];
	for (i = 1; i < argc; i++) {
		if (*argv[i] == '-') switch (argv[i][1]) {

		case 'S':
			sflag++;
			cflag++;
			continue;
		case 'o':
			if (++i < argc) {
				outfile = argv[i];
				switch (getsuf(outfile)) {

				case 'c':
				case 'o':
					error("-o would overwrite %s",
					    outfile);
					exit(8);
				}
			}
			continue;
		case 'R':
			Rflag++;
			continue;
		case 'O':
			/*
			 * There might be further chars after -O; we just
			 * pass them on to c2 as an extra argument -- later.
			 */
			oflag++;
			continue;
		case 'p':
			proflag++;
			if (argv[i][2] == 'g'){
				crt0 = gcrt0;
				gproflag++;
			} else {
				crt0 = mcrt0;
			}
			continue;
		case 'g':
			if (argv[i][2] == 'o') {
			    Gflag++;	/* old format for -go */
			} else {
			    gflag++;	/* new format for -g */
			}
			continue;
		case 'w':
			wflag++;
			continue;
		case 'E':
			exflag++;
		case 'P':
			pflag++;
			if (argv[i][1]=='P')
				fprintf(stderr,
	"%s: warning: -P option obsolete; you should use -E instead\n", ccname);
			plist[np++] = argv[i];
		case 'c':
			cflag++;
			continue;
		case 'D':
		case 'I':
		case 'U':
		case 'C':
			plist[np++] = argv[i];
			continue;
		case 't':
			if (chpass)
				error("-t overwrites earlier option", 0);
			chpass = argv[i]+2;
			if (chpass[0]==0)
				chpass = "012palc";
			continue;
		case 'f':
#ifdef mc68000
			/*
			 * floating point option switches
			 */
			optfound = 0;
			for (mp = floatopts; mp->optname; mp++) {
			    if (!strcmp(argv[i], mp->optname)){
				if (mp->isatype) {
				    if (fptype != NULL && fptype != mp) {
					error("%s overwrites earlier option",
					    mp->optname);
				    }
				    fptype = mp;
				} else {
				    mp->found = 1;
				}
				optfound = 1;
				break;
			    }
			}
			if (!optfound) {
			    if (argv[i][2] == '\0') {
				fprintf(stderr,
				    "%s: warning: -f option is obsolete\n",
				    ccname);
			    } else {
				fprintf(stderr,
				    "%s: warning: %s option not recognized\n",
				    ccname, argv[i]);
			    }
			}
			continue;
#else !mc68000
			fprintf(stderr,
			    "%s: warning: -f option is obsolete\n",
			    ccname);
			continue;
#endif !mc68000

#ifdef	mc68000
		case 'm':
			optfound = 0;
			for (mp = machopts; mp->optname; mp++) {
			    if (!strcmp(argv[i], mp->optname)) {
				if (mp->isatype) {
				    if (machtype != NULL && machtype != mp) {
					error("%s overwrites earlier option",
					    mp->optname);
				    }
				    machtype = mp;
				} else {
				    mp->found = 1;
				}
				optfound = 1;
				break;
			    }
			}
			if (!optfound) {
			    fprintf(stderr,
				"%s: warning: %s option not recognized\n",
				ccname, argv[i]);
			}
			continue;
#endif	mc68000

		case 'B':
			if (npassname)
				error("-B overwrites earlier option", 0);
			npassname = argv[i]+2;
			if (npassname[0]==0)
				npassname = "/usr/c/o";
			continue;
		case 'd':
			dflag = argv[i];
			continue;
		case 'a':
			aflag++;
			continue;
		case 'A':
			/*
			 * There might be further chars after -A; we just
			 * pass them on to c2 as an extra argument -- later.
			 */
			continue;
		case 'v':
			vflag++;
			continue;
		/*
		 * Local Options
		 * added by D.A. Kapilow  12/3/86 to support NRTX
		 * libraries and startoff files
		 */
		case 'L':
			/*
			 * Change path for -l
			 */
			if (argv[i][2] == 'p') {
				libdir = &argv[i][3];
			}
			/*
			 * Change default crt0.o
			 */
			else if (argv[i][2] == 's') {
				crt0 = &argv[i][3];
			}
			/*
			 * Set nrtxflag to avoid pulling in crt1.o with -f
			 */
			else if (argv[i][2] == 'n') {
				nrtxflag++;
			}
			/*
			 * Set nofloatcrtflag to avoid pulling in crt1.o
			 * when not using NRTX.  Kludge used for cross
			 * development of version 9.
			 */
			else if (argv[i][2] == 'f') {
				nofloatcrtflag++;
			}
			continue;
		}
		t = argv[i];
		c = getsuf(t);
		if (c=='c' || c=='s' || c=='i' || exflag) {
			clist[nc++] = t;
			t = setsuf(t, 'o');
		}
		if (libdir && t[0] == '-' && t[1] == 'l') {
			t = expandlib(libdir, &t[2]);
		}
		if (nodup(llist, t)) {
			llist[nl++] = t;
			if (getsuf(t)=='o')
				nxo++;
		}
	}
	if (gflag || Gflag) {
		if (oflag)
			fprintf(stderr, "%s: warning: -g disables -O\n", ccname);
		oflag = 0;
	}
#ifdef mc68000
	/*
	 * if no machine type specified, use the default
	 */
	if (machtype == NULL) {
		machtype = default_machtype();
	}
	/*
	 * if no floating point machine type specified, use the default
	 */
	if (fptype == NULL) {
		fptype = default_fptype(machtype);
	} else if (unsupported(machtype, fptype)) {
		t = fptype->optname;
		fptype = default_fptype(machtype);
		fprintf(stderr,
		    "%s: warning: %s option not supported with %s; %s used\n",
		    ccname, t, machtype->optname, fptype->optname);
	}
	machtype->found = 1;
	fptype->found = 1;
#endif mc68000

	if (npassname && chpass ==0)
		chpass = "012palc";
	if (chpass && npassname==0)
		npassname = "/usr/new/";
	if (chpass)
	for (t=chpass; *t; t++) {
		switch (*t) {

		case '0':
		case '1':
			ccom = strspl(npassname, "ccom");
			continue;
		case '2':
			c2 = strspl(npassname, "c2");
			continue;
		case 'p':
			cpp = strspl(npassname, "cpp");
			continue;
		case 'a':
			as = strspl(npassname, "as");
			continue;
		case 'l':
			ld = strspl(npassname, "ld");
			continue;
		case 'c':
			libc = strspl(npassname, "libc.a");
			continue;
		}
	}
	if (nc==0)
		goto nocom;
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		signal(SIGINT, idexit);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, idexit);
	if (pflag==0)
		sprintf(tmp0, "/tmp/ctm%05.5d", getpid());
	tmp1 = strspl(tmp0, "1");
	tmp2 = strspl(tmp0, "2");
	tmp3 = strspl(tmp0, "3");
	if (pflag==0)
		tmp4 = strspl(tmp0, "4");
	if (oflag)
		tmp5 = strspl(tmp0, "5");
	for (i=0; i<nc; i++) {
		int suffix = getsuf(clist[i]);
		if (nc > 1) {
			printf("%s:\n", clist[i]);
			fflush(stdout);
		}
		if (suffix == 's') {
			assource = clist[i];
			goto assemble;
		} else
			assource = tmp3;
		if (suffix == 'i')
			goto compile;
		if (pflag)
			tmp4 = setsuf(clist[i], 'i');
		av[0] = "cpp"; av[1] = clist[i]; av[2] = exflag ? "-" : tmp4;
		na = 3;
		for (j = 0; j < np; j++)
			av[na++] = plist[j];
		av[na++] = 0;
		if (callsys(cpp, av)) {
			exfail++;
			eflag++;
		}
		if (pflag || exfail) {
			cflag++;
			continue;
		}
compile:

		/*
		 * Call the bb_count preprocessor
		 */
		if (aflag) {
			tmp6 = strspl(tmp0, "6");
			av[0] = "bb_count";
			av[1] = tmp4;
			av[2] = clist[i];
			av[3] = tmp6;
			av[4] = 0;
			if (callsys(count, av)) {
				exfail++;
				eflag++;
			}
			if (pflag || exfail) {
				cflag++;
				continue;
			}
		}
		if (sflag)
			assource = tmp3 = setsuf(clist[i], 's');
		av[0] = "ccom"; 
		av[1] = aflag? tmp6: (suffix=='i'? clist[i] : tmp4); 
		av[2] = oflag? tmp5: tmp3; 
		na = 3;
		if (proflag)
			av[na++] = "-XP";
		if (gflag) {
			av[na++] = "-Xg";
		} else if (Gflag) { 
			av[na++] = "-XG"; 
		} if (wflag)
			av[na++] = "-w";
#ifdef mc68000
		/* pass code gen options to ccom */
		for (mp = machopts; mp->optname; mp++) {
		    if (mp->found) {
			av[na++] = mp->optname;
		    }
		}
		for (mp = floatopts; mp->optname; mp++) {
		    if (mp->found) {
			av[na++] = mp->optname;
		    }
		}
#endif
		av[na] = 0;
		if (callsys(ccom, av)) {
			cflag++;
			eflag++;
			continue;
		}
		if (oflag) {
			av[0] = "c2"; av[1] = tmp5; av[2] = tmp3; na = 3;
			av[na++] = (use68020 ? "-20" : "-10");
			/* Pass -Oxxx arguments to optimizer */
			for (tmpi = 1; tmpi < argc; tmpi++) {
				if (argv[tmpi][0] == '-'
				 && argv[tmpi][1] == 'O'
				 && argv[tmpi][2] != '\0') {
					av[na++] = argv[tmpi]+2;
				}
			}
			av[na] = 0;
			if (callsys(c2, av)) {
				unlink(tmp3);
				tmp3 = assource = tmp5;
			} else
				unlink(tmp5);
		}
		if (sflag)
			continue;
	assemble:
		cunlink(tmp1); cunlink(tmp2); cunlink(tmp4);
		av[0] = "as"; av[1] = "-o"; av[2] = setsuf(clist[i], 'o');
		na = 3;
		av[na++] = (use68020 ? "-20" : "-10");
		if (Rflag)
			av[na++] = "-R";
		if (dflag)
			av[na++] = dflag;
		/* Pass -Axxx arguments to assembler */
		for (tmpi = 1; tmpi < argc; tmpi++) {
			if (argv[tmpi][0] == '-'
			 && argv[tmpi][1] == 'A'
			 && argv[tmpi][2] != '\0') {
				av[na++] = argv[tmpi]+2;
			}
		}
		av[na++] = assource;
		av[na] = 0;
		if (callsys(as, av) > 1) {
			cflag++;
			eflag++;
			continue;
		}
	}
nocom:
	if (cflag==0 && nl!=0) {
		i = 0;
		na = 0;
		av[na++] = "ld";
		av[na++] = "-X";
		if (nrtxflag) {
			av[na++] = "-r";
			av[na++] = "-d";
		}
		av[na++] = crt0;
/*
		if (fptype->crt1 != NULL && !nrtxflag && !nofloatcrtflag) {
			av[na++] = strspl("/lib/", fptype->crt1);
		}
*/
		if (outfile) {
			av[na++] = "-o";
			av[na++] = outfile;
		}
		while (i < nl)
			av[na++] = llist[i++];
		if (aflag) 
			av[na++] = "/usr/lib/bb_link.o";
		if (gflag || Gflag) {
			if (libdir)
				av[na++] = expandlib(libdir, "g");
			else
				av[na++] = "-lg";
		}
		if (proflag) {
			if (libdir)
				av[na++] = expandlib(libdir, "c_p");
			else
				av[na++] = "-lc_p";
		}
		else {
			if (libdir && libc[0] == '-' && libc[1] == 'l')
				av[na++] = expandlib(libdir, &libc[2]);
			else
				av[na++] = libc;
		}
		av[na++] = 0;
		eflag |= callsys(ld, av);
		if (nc==1 && nxo==1 && eflag==0)
			unlink(setsuf(clist[0], 'o'));
	}
	dexit();
}

#ifdef mc68000

/*
 * default target machine type is the same as host
 */
struct mach_info *
default_machtype()
{
    return (is68020()? M_68020 : M_68010);
}

/*
 *  Floating point is such a zoo on this machine that
 *  nobody agrees what the default should be.  So let
 *  the user decide, and to hell with it.
 */
struct mach_info *
default_fptype(mtp)
    struct mach_info *mtp;
{
    register char *env_string;
    register struct mach_info *ftp;

    env_string = getenv(FLOAT_OPTION);
    if (env_string == NULL) {
	return (F_soft);
    }
    for(ftp = floatopts; ftp->isatype; ftp++) {
	if (!strcmp(ftp->optname+1, env_string)) {
	    if (unsupported(mtp, ftp)) {
		ftp = F_soft;
		fprintf(stderr,
		"%s: warning: FLOAT_OPTION=%s not supported with %s; %s used\n",
		    ccname, env_string, mtp->optname+1, ftp->optname+1);
	    }
	    return(ftp);
	}
    }
    ftp = F_soft;
    fprintf(stderr,
	"%s: warning: FLOAT_OPTION=%s not recognized; %s used\n",
	ccname, env_string, ftp->optname+1);
    return(ftp);
}

#endif mc68000

idexit()
{

	eflag = 100;
	dexit();
}

dexit()
{

	if (!pflag) {
		cunlink(tmp1);
		cunlink(tmp2);
		if (sflag==0)
			cunlink(tmp3);
		cunlink(tmp4);
		cunlink(tmp5);
		cunlink(tmp6);
	}
	exit(eflag);
}

/*VARARGS1*/
error(s, x1, x2, x3, x4)
	char *s;
{
	FILE *diag = exflag ? stderr : stdout;

	fprintf(diag, "%s: ", ccname);
	fprintf(diag, s, x1, x2, x3, x4);
	putc('\n', diag);
	exfail++;
	cflag++;
	eflag++;
}

getsuf(as)
char as[];
{
	register int c;
	register char *s;
	register int t;

	s = as;
	c = 0;
	while (t = *s++)
		if (t=='/')
			c = 0;
		else
			c++;
	s -= 3;
	if (c <= DIRSIZ && c > 2 && *s++ == '.')
		return (*s);
	return (0);
}

char *
setsuf(as, ch)
	char *as;
{
	register char *s, *s1;

	s = s1 = savestr(as);
	while (*s)
		if (*s++ == '/')
			s1 = s;
	s[-1] = ch;
	return (s1);
}

callsys(f, v)
	char *f, **v;
{
	int t, status;

#ifdef DEBUG
	printf("fork %s:", f);
	for (t = 0; v[t]; t++) printf(" %s", v[t][0]? v[t]: "(empty)");
	printf("\n");
	return 0;
#else  DEBUG
	if (vflag){
		fprintf(stderr,"%s: ",f);
		for (t = 0; v[t]; t++) fprintf(stderr, " %s", v[t][0]? v[t]: "(empty)");
		fprintf(stderr, "\n");
	}
	fflush(stderr);	/* purge any junk before the vfork */
	t = vfork();
	if (t == -1) {
		fprintf( stderr, "%s: No more processes\n", ccname);
		return (100);
	}
	if (t == 0) {
		execv(f, v);
		/*
		 * We are now in The Vfork Zone, and can't use "fprintf".
		 * We use "write" and "_perror" instead.
		 */
		write(2, ccname, strlen(ccname));
		write(2, ": Can't execute ", 16);
		perror(f);
		_exit(100);
	}
	while (t != wait(&status))
		;
	if ((t=(status&0377)) != 0 && t!=14) {
		if (t!=2) {
			fprintf( stderr, "%s: Fatal error in %s\n", ccname, f);
			eflag = 8;
		}
		dexit();
	}
	return ((status>>8) & 0377);
#endif DEBUG
}

nodup(l, os)
	char **l, *os;
{
	register char *t, *s;
	register int c;

	s = os;
	if (getsuf(s) != 'o')
		return (1);
	while (t = *l++) {
		while (c = *s++)
			if (c != *t++)
				break;
		if (*t==0 && c==0)
			return (0);
		s = os;
	}
	return (1);
}

#define	NSAVETAB	1024
char	*savetab;
int	saveleft;

char *
savestr(cp)
	register char *cp;
{
	register int len;

	len = strlen(cp) + 1;
	if (len > saveleft) {
		saveleft = NSAVETAB;
		if (len > saveleft)
			saveleft = len;
		savetab = (char *)malloc(saveleft);
		if (savetab == 0) {
			fprintf(stderr, "%s: ran out of memory (savestr)\n", ccname);
			exit(1);
		}
	}
	strncpy(savetab, cp, len);
	cp = savetab;
	savetab += len;
	saveleft -= len;
	return (cp);
}

char *
strspl(left, right)
	char *left, *right;
{
	char buf[BUFSIZ];

	strcpy(buf, left);
	strcat(buf, right);
	return (savestr(buf));
}

char *
expandlib(dir, lib)
	char *dir, *lib;
{
	char buf[BUFSIZ];

	strcpy(buf, dir);
	strcat(buf, "/lib");
	strcat(buf, lib);
	strcat(buf, ".a");
	return (savestr(buf));
}
