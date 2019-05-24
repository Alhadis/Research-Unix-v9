/*
 * machine-dependent code for
 * looking in stack frames
 * vax version
 */

#include "defs.h"
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/user.h>
#include "regs.h"
#include "sym.h"
#include "space.h"

int maxargs = 20;

/*
 * mc68000 stack frame
 */

#define	F_FP	0	/* saved fp */
#define	F_PC	4	/* return pc */
#define	F_ARG	8	/* first argument */
#define	FP	AR6

/*
 * is the saved psw reasonable?
 * really just a last resort to find the end of the stack
 */

#define	BADPSW(p)	(((p)&0xff00) != 0)

/*
 * return an address for a local variable
 * no register vars, unfortunately, as we can't provide an address
 * gn is the procedure; ln the local name
 */

localaddr(gn, ln)
char *gn, *ln;
{
	WORD fp;
	extern WORD expv;
	extern int expsp;
	ADDR laddr();

	if (gn) {
		if (findrtn(gn) == 0)
			error("function not found");
	}
	else {
		findsym((WORD)atow(rget(PC)), INSTSP);
		if (cursym == NULL)
			error("function not found");
	}
	if (findframe(&fp) == 0)
		error("stack frame not found");
	if (ln == NULL) {
		expsp = 0;
		expv = fp;
		return;
	}
	while (localsym()) {
		if (strcmp(ln, cursym->y_name) != 0)
			continue;
		expv = laddr(cursym, fp);
		if (cursym->y_ltype == S_RSYM)
			expsp = REGSP;
		else
			expsp = NOSP;
		return;
	}
	error("bad local variable");
	/* NOTREACHED */
}

/*
 * print a stack traceback
 * give locals if possible
 */

ctrace(modif)
char modif;
{
	register ADDR fp, ap, callpc;
	register int narg;
	register int fl;

	if (adrflg) {
		fp = adrval;
		callpc = atow(aget(fp + F_PC, CORF|DATASP));
	} else {
		fp = (ADDR)rtow(rget(FP));
		callpc = (ADDR)rtow(rget(PC));
	}
	clrraddr();
	while (cntval--) {
		chkerr();
		findsym(callpc, INSTSP);
		if (cursym == NULL)
			printf("?(");
		else if (strcmp("start", cursym->y_name) == 0)
			break;
		else
			printf("%s(", cursym->y_name);
		fl = getnargs(fp);
		if ((narg = fl) > maxargs)
			narg = maxargs;
		ap = fp + F_ARG;
		while (--fl, --narg >= 0) {
			printf("%R", ltow(lget(ap, CORF|DATASP)));
			ap += SZREG;
			if (narg != 0)
				printc(',');
		}
		if (fl >= 0)
			printf(",...");
		printf(") from %R\n", callpc);
		if (modif == 'C')
			locals(fp);
		callpc = atow(aget(fp + F_PC, CORF|DATASP));
		setraddr(fp);
		fp = atow(aget(fp + F_FP, CORF|DATASP));
		if (fp == 0)
			break;
	}
	clrraddr();
}

/*
 * Given a frame pointer determine the number of arguments.
 * Unlike the vax, there is no argument pointer so we look at
 * the instruction in the return pc and try to determine how
 * may arguments are there by the resetting of the stack pointer.
 */
#define	LEASPSP		0x4fef		/* Lea sp@(xx),sp instruction */
#define	ADDQWL		0x500f		/* Addqw #xx,sp instruction */

getnargs(fp)
ADDR fp;
{
	ADDR callpc;
	WORD instr, nargs;

	callpc = atow(aget(fp + F_PC, CORF|DATASP));
	instr = stow(sget(callpc, SYMF|INSTSP));
	if (instr == LEASPSP)
		nargs = stow(sget(callpc + 2, SYMF|INSTSP));
	else if ((instr & 0xf12f) == ADDQWL) {
		nargs = (instr >> 9) & 7;
		if (nargs == 0)
			nargs = 8;
	}
	else
		nargs = 0;
	return (nargs / SZREG);
}

static
locals(fp)
ADDR fp;
{
	WORD val;
	register int sp;
	ADDR laddr();

	while (localsym()) {
		sp = CORF | DATASP;
		if (cursym->y_ltype == S_RSYM)
			sp = CORF | REGSP;
		val = ltow(lget(laddr(cursym, fp), sp));
		if (errflg == 0)
			printf("%8t%s/%10t%R\n", cursym->y_name, val);
		else {
			printf("%8t%s/%10t?\n", cursym->y_name);
			errflg = 0;
		}
	}
}

static ADDR
laddr(sp, fp)
struct sym *sp;
ADDR fp;
{

	switch (sp->y_ltype) {
	case S_STSYM:
		return (sp->y_value);

	case S_LSYM:
		return (fp - sp->y_value);

	case S_PSYM:
		return (fp + sp->y_value);

	case S_RSYM:
		return (sp->y_value);
	}
	error("bad local symbol");
	/* NOTREACHED */
}

static int
findframe(fpp)
ADDR *fpp;
{
	register ADDR fp, pc;
	struct sym *svcur;

	svcur = cursym;
	fp = rtow(rget(FP));
	pc = rtow(rget(PC));
	if (errflg)
		return (0);
	clrraddr();
	for (;;) {
		findsym(pc, INSTSP);
		if (cursym == svcur)
			break;
		if (cursym && strcmp(cursym->y_name, "start") == 0) {
			clrraddr();
			return (0);
		}
		setraddr(fp);
		pc = atow(aget(fp + F_PC, CORF|DATASP));
		fp = atow(aget(fp + F_FP, CORF|DATASP));
		if (errflg) {
			clrraddr();
			return (0);
		}
	}
	*fpp = fp;
	return (1);
}

/*
 * set addresses for saved registers for this frame
 */

static
setraddr(fp)
register ADDR fp;
{
	register int r;
	register int i;
	extern ADDR raddr[];

	/* all wrong */
}

static
clrraddr()
{
	register int i;
	extern ADDR raddr[];

	for (i = 0; i <= MAXREG - MINREG; i++)
		raddr[i] = 0;
}
