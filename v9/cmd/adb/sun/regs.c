/*
 * code to keep track of registers
 */

#include "defs.h"
#include "regs.h"
#include "space.h"
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/user.h>

struct reglist {
	char	*rname;
	short	roffs;
	TREG	rval;
};

struct reglist reglist[] = {
	{"pc",	PC},
	{"ps",	PS},
	{"sp",	AR7},
	{"a6",	AR6},
	{"a5",	AR5},
	{"a4",	AR4},
	{"a3",	AR3},
	{"a2",	AR2},
	{"a1",	AR1},
	{"a0",	AR0},
	{"d7",	R7},
	{"d6",	R6},
	{"d5",	R5},
	{"d4",	R4},
	{"d3",	R3},
	{"d2",	R2},
	{"d1",	R1},
	{"d0",	R0},
	{NULL}
};

/*
 * the following are needed only to
 * make registers `addressable'
 * which is needed only so we can
 * examine register variables
 */

ADDR raddr[MAXREG - MINREG + 1];
int roffs[MAXREG - MINREG + 1] = {
	R0, R1, R2, R3, R4, R5, R6, R7, AR0, AR1, AR2, AR3, AR4, AR5, AR6,
	SP, PS, PC
};

/*
 * get/put registers
 * in our saved copies
 */

TREG
rget(r)
{
	register struct reglist *rp;

	for (rp = reglist; rp->rname; rp++)
		if (rp->roffs == r)
			return (rp->rval);
	error("panic: rget");
	/* NOTREACHED */
}

rput(r, v)
TREG v;
{
	register struct reglist *rp;

	for (rp = reglist; rp->rname; rp++)
		if (rp->roffs == r) {
			rp->rval = v;
			return;
		}
	error("panic: rput");
	/* NOTREACHED */
}

/*
 * grab registers into saved copy
 * should be called before looking at the process
 */

rsnarf()
{
	register struct reglist *rp;
	struct user u;
	int *ip;

	fget((ADDR)0, CORF|UBLKSP, (char *)&u, sizeof(u));
	ip = (int *)(((int)u.u_ar0) & 0x1fff);
	for (rp = reglist; rp->rname; rp++) {
		rp->rval = 0;
		fget((ADDR)&ip[rp->roffs], CORF|UBLKSP,
			(char *)&rp->rval, SZREG);
	}
}

/*
 * put registers back
 */

rrest()
{
	register struct reglist *rp;
	struct user u;
	int *ip;

	if (pid == 0)
		return;
	fget((ADDR)0, CORF|UBLKSP, (char *)&u, sizeof(u));
	ip = (int *)(((int)u.u_ar0) & 0x1fff);
	for (rp = reglist; rp->rname; rp++)
		fput((ADDR)&ip[rp->roffs], CORF|UBLKSP,
			(char *)&rp->rval, SZREG);
}

/*
 * print the registers
 */

printregs(c)
char c;
{
	register struct reglist *rp;
	int tab = 0;

	for (rp = reglist; rp->rname; rp++) {
		printf("%-8R >%s", rtow(rp->rval), rp->rname);
		tab = !tab;
		if (tab)
			printf("\t");
		else
			printf("\n");
	}
	if (tab)
		printf("\n");
	printpc();
}

/*
 * translate a name to a magic register offset
 * the latter useful in rget/rput
 */

int
rname(n)
char *n;
{
	register struct reglist *rp;

	for (rp = reglist; rp->rname; rp++)
		if (strcmp(n, rp->rname) == 0)
			return (rp->roffs);
	return (BADREG);
}
