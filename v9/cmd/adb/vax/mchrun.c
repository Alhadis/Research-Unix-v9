/*
 * machine-dependent code for controlling a process
 * this for vax
 */

#include "defs.h"
#include "bkpt.h"
#include "regs.h"
#include "space.h"
#include <sys/psl.h>


/*
 * install (f != 0) or remove (f == 0) a breakpoint
 */

#define	BPT	03

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
		cput(bk->loc, sp, wtoc(bk->ins));
	else {
		bk->ins = ctow(cget(bk->loc, sp));
		cput(bk->loc, sp, wtoc(BPT));
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

	rput(PSL, rget(PSL) | PSL_T);
}
