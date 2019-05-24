/*
 * machine-dependent code for
 * looking in stack frames
 * vax version
 */

#include "defs.h"
#include <sys/param.h>
#include "regs.h"
#include "sym.h"
#include "space.h"

int maxargs = 20;

/*
 * VAX stack frame
 */

#define	F_PSW	4
#define	F_FLAGS	6
#define	F_AP	8	/* saved ap */
#define	F_FP	12	/* saved fp */
#define	F_PC	16	/* return pc */
#define	F_REGS	20	/* saved regs, if any */

/*
 * flags
 */

#define	FFREGS	0x1fff	/* saved register flags; 01 is r0 */
#define	FFCALLS	0x2000	/* called by calls, not callg */
#define	FFOFF	0xc000	/* offset added to align stack */
#define	SALIGN(f)	(((f)>>14) & 03)

/*
 * is the saved psw reasonable?
 * really just a last resort to find the end of the stack
 */

#define	BADPSW(p)	(((p)&0xff00) != 0)

/*
 * is the pc probably in signal trampoline code?
 * == it's in the user block
 */

#define	sigtramp(pc)	(0x80000000 > (pc) && (pc) > 0x80000000 - ctob(UPAGES))

#define	F_HACK	64	/* where to find the saved pc in a trampoline frame */
/*
 * return an address for a local variable
 * no register vars, unfortunately, as we can't provide an address
 * gn is the procedure; ln the local name
 *
 * this is vax dependent because symbol tables vary
 */

localaddr(gn, ln)
char *gn, *ln;
{
	WORD fp, ap;
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
	if (findframe(&fp, &ap) == 0)
		error("stack frame not found");
	if (ln == NULL) {
		expsp = 0;
		expv = fp;
		return;
	}
	while (localsym()) {
		if (strcmp(ln, cursym->y_name) != 0)
			continue;
		expv = laddr(cursym, fp, ap);
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
	ADDR oap;
	register int fl;
	int tramp;

	if (adrflg) {
		fp = adrval;
		fl = stow(sget(fp + F_FLAGS, CORF|DATASP));
		if ((fl & FFCALLS) == 0)
			ap = fp;	/* callg, can't figure out ap */
		else {
			ap = adrval + F_REGS + SALIGN(fl);
			fl &= FFREGS;
			while (fl) {
				if (fl & 1)
					ap += SZREG;
				fl >>= 1;
			}
		}
		callpc = atow(aget(fp + F_PC, CORF|DATASP));
	} else {
		ap = (ADDR)rtow(rget(AP));
		fp = (ADDR)rtow(rget(FP));
		callpc = (ADDR)rtow(rget(PC));
	}
	clrraddr();
	while (cntval--) {
		chkerr();
		tramp = 0;
		if (sigtramp(callpc)) {
			printf("sigtramp(");
			tramp++;
		} else {
			findsym(callpc, INSTSP);
			if (cursym == NULL)
				printf("?(");
			else if (strcmp("start", cursym->y_name) == 0)
				break;
			else
				printf("%s(", cursym->y_name);
		}
		fl = ctow(cget(ap, CORF|DATASP));
		if ((narg = fl) > maxargs)
			narg = maxargs;
		oap = ap;
		while (--fl, --narg >= 0) {
			printf("%R", ltow(lget(ap += SZREG, CORF|DATASP)));
			if (narg != 0)
				printc(',');
		}
		if (fl >= 0)
			printf(",...");
		printf(") from %R\n", callpc);
		if (modif == 'C')
			locals(fp, oap);
		if (tramp)	/* hack */
			callpc = atow(aget(fp + F_HACK, CORF|DATASP));
		else
			callpc = atow(aget(fp + F_PC, CORF|DATASP));
		fl = stow(sget(fp + F_FLAGS, CORF|DATASP));
		setraddr(fl, fp);
		ap = atow(aget(fp + F_AP, CORF|DATASP));
		fp = atow(aget(fp + F_FP + SALIGN(fl), CORF|DATASP));
		if (fp == 0)
			break;
		fl = stow(sget(fp + F_PSW, CORF|DATASP));
		if (BADPSW(fl))
			break;
	}
	clrraddr();
}

static
locals(fp, ap)
ADDR fp, ap;
{
	WORD val;
	register int sp;
	ADDR laddr();

	while (localsym()) {
		sp = CORF | DATASP;
		if (cursym->y_ltype == S_RSYM)
			sp = CORF | REGSP;
		val = ltow(lget(laddr(cursym, fp, ap), sp));
		if (errflg == 0)
			printf("%8t%s/%10t%R\n", cursym->y_name, val);
		else {
			printf("%8t%s/%10t?\n", cursym->y_name);
			errflg = 0;
		}
	}
}

static ADDR
laddr(sp, fp, ap)
struct sym *sp;
ADDR fp, ap;
{

	switch (sp->y_ltype) {
	case S_STSYM:
		return (sp->y_value);

	case S_LSYM:
		return (fp - sp->y_value);

	case S_PSYM:
		return (ap + sp->y_value);

	case S_RSYM:
		return (sp->y_value * SZREG);
	}
	error("bad local symbol");
	/* NOTREACHED */
}

static int
findframe(fpp, app)
ADDR *fpp, *app;
{
	register ADDR fp, ap, pc;
	register int fl;
	struct sym *svcur;

	svcur = cursym;
	fp = rtow(rget(FP));
	ap = rtow(rget(AP));
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
		fl = stow(sget((ADDR)fp + F_FLAGS, CORF|DATASP));
		setraddr(fl, fp);
		pc = atow(aget(fp + F_PC, CORF|DATASP));
		ap = atow(aget(fp + F_AP, CORF|DATASP));
		fp = atow(aget(fp + F_FP + SALIGN(fl), CORF|DATASP));
		/* sigtramp? */
		if (errflg) {
			clrraddr();
			return (0);
		}
	}
	*fpp = fp;
	*app = ap;
	return (1);
}

/*
 * set addresses for saved registers for this frame
 */

static
setraddr(mask, fp)
register int mask;
register ADDR fp;
{
	register int r;
	register int i;
	extern ADDR raddr[];

	mask &= FFREGS;
	for (r = 0, i = 0; mask; r++)
		if (mask & (1 << r)) {
			if (MINREG <= r && r <= MAXREG)
				raddr[r - MINREG] = fp + F_REGS +
					i * SZREG;
			i++;
			mask &=~ (1 << r);
		}
}

static
clrraddr()
{
	register int i;
	extern ADDR raddr[];

	for (i = 0; i <= MAXREG - MINREG; i++)
		raddr[i] = 0;
}
