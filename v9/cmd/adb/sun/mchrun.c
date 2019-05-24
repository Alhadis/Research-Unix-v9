/*
 * machine-dependent code for controlling a process
 * this for vax
 */

#include "defs.h"
#include "bkpt.h"
#include "regs.h"
#include "space.h"
#include <machine/reg.h>
#include <machine/psl.h>


/*
 * install (f != 0) or remove (f == 0) a breakpoint
 */

#define	BPT	0x4e4f		/* Trap #15 */

extern ADDR txtsize;
bkput(bk, f)
register BKPT *bk;
{
	register int sp;

	if (bk->loc < txtsize)
		sp = CORF | INSTSP;
	else
		sp = CORF | DATASP;
	if (f == 0)
		sput(bk->loc, sp, wtos(bk->ins));
	else {
		bk->ins = stow(sget(bk->loc, sp));
		sput(bk->loc, sp, wtos(BPT));
		if (errflg) {
			printf("cannot set breakpoint: ");
			/* stuff */
			prints(errflg);
		}
	}
}

/*
 * set psl to cause a trap after one instruction
 * needed for v8; no ioctl to step pc
 */

setstep()
{

	rput(PS, rget(PS) | PSL_T);
}
