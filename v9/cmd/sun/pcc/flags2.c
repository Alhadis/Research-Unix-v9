#ifndef lint
static	char sccsid[] = "@(#)flags2.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "cpass2.h"

/*
 * process pass 2 flags from ccom/f1 command line
 */

int use68020 = 0;
int use68881 = 0;
int usesky = 0;
int usesoft = 0;
int useswitch = 0;
int usefpa = 0;
int stmtprofflag = 0;
FILE *dotd_fp;

struct machopts {
    char *optname;
    int *useflag;
} machopts[] = {
    "m68020",	&use68020,
    "fpa",	&usefpa,
    "f68881",	&use68881,
    "fsky",	&usesky,
    "fsoft",	&usesoft,
    "fswitch",	&useswitch,
};

int chk_ovfl = 0;

myflags( c, cpp )
    char c; 
    register char **cpp;
{
	register struct machopts *fp;
	register char *cp;

	switch( c ) {
	case 'A':
		cp = *cpp;
		stmtprofflag = 1;
		if((dotd_fp = fopen(cp+1, "w")) == NULL){
			perror(cp+1);
			cerror( "can't open statement profiling statistics file");
		}
		goto endswitch;

	case 'f':	
	case 'm':
		cp = *cpp;
		for (fp = machopts; fp->optname; fp++) {
			if (!strcmp(fp->optname, cp)) {
				*(fp->useflag) = 1;
				goto endswitch;
			}
		}
#ifndef  FORT
#ifdef FLOATMATH
		if (!strcmp("fsingle", cp)) {
			FLOATMATH = 1;
			goto endswitch;
		}
		if (!strcmp("fsingle2", cp)) {
			FLOATMATH = 2;
			goto endswitch;
		}
#endif
#endif

	endswitch:
		/* recognized an option string; skip to its end */
		while (cp[1]) cp++;
		*cpp = cp;
		return;

	case 'F': 
		usesky = 1; 
		use68881 = 0;
		return;
	case 'V':
		chk_ovfl++;
		return;
	default: 
		cerror( "Bad flag %c", c );
	}
}
