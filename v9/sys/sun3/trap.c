#ifndef lint
static	char sccsid[] = "@(#)trap.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/lnode.h"
#include "../h/proc.h"
#include "../h/seg.h"
#include "../h/vm.h"
#include "../h/share.h"

#include "../machine/fault.h"
#include "../machine/frame.h"
#include "../machine/buserr.h"
#include "../machine/memerr.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/psl.h"
#include "../machine/pte.h"
#include "../machine/reg.h"
#include "../machine/trap.h"


#define	USER	0x400		/* user-mode flag added to type */

struct	sysent	sysent[128];
int		syscnt[128];

char	*trap_type[] = {
	"Vector address 0x0",
	"Vector address 0x4",
	"Bus error",
	"Address error",
	"Illegal instruction",
	"Divide by zero",
	"CHK, CHK2 instruction",
	"TRAPV, cpTRAPcc, cpTRAPcc instruction",
	"Priviledge violation",
	"Trace",
	"1010 emulator trap",
	"1111 emulator trap",
	"Vector address 0x30",
	"Coprocessor protocol error",
	"Stack format error",
	"Unitialized interrupt",
	"Vector address 0x40",
	"Vector address 0x44",
	"Vector address 0x48",
	"Vector address 0x4c",
	"Vector address 0x50",
	"Vector address 0x54",
	"Vector address 0x58",
	"Vector address 0x5c",
	"Spurious interrupt",
};
#define	TRAP_TYPES	(sizeof trap_type / sizeof trap_type[0])

#if defined(DEBUG) || defined(lint)
int tdebug  = 0;
int tudebug = 0;
int lodebug = 0;
int bedebug = 0;
#else
#define	tdebug	0
#define	tudebug	0
#define	lodebug	0
#define	bedebug	0
#endif defined(DEBUG) || defined(lint)

u_char	getsegmap();
long	getpgmap();

/*
 * Called from the trap handler when a processor trap occurs.
 * Returns amount to adjust the stack:  > 0 removes bus error
 * info, == 0 does nothing.
 */
int
trap(type, regs, fmt)
	int type;
	struct regs regs;
	struct stkfmt fmt;
{
	register struct regs *locregs = &regs;
	register int i = 0;
	register struct proc *p = u.u_procp;
	time_t syst;
	int nosig = 0;
	int besize = 0;
	int be = (type == T_BUSERR)? getbuserr() : 0;

	cnt.v_trap++;
	syst = u.u_vm.vm_stime;
	if (tdebug) {
		i = type/sizeof (int);
		if ((unsigned)i < TRAP_TYPES)
			printf("trap: %s\n", trap_type[i]);
		showregs("trap", type, locregs, &fmt, be);
	}
	if (USERMODE(locregs->r_sr)) {
		type |= USER;
		u.u_ar0 = &locregs->r_dreg[0];
	}

	switch (type) {

	default:
	die:
		(void) spl7();
		showregs((char *)0, fmt.f_vector, locregs, &fmt, be);
		traceback((long)locregs->r_areg[6], (long)locregs->r_sp);
		i = fmt.f_vector/sizeof (int);
		if (i < TRAP_TYPES)
			panic(trap_type[i]);
		panic("trap");
		/*NOTREACHED*/

	case T_BUSERR:
		if (be & BE_TIMEOUT)
			DELAY(2000);	/* allow for refresh recovery time */

		/* may have been expected by C (e.g., Multibus probe) */
		if (nofault) {
			label_t *ftmp;

			ftmp = nofault;
			nofault = 0;
			longjmp(ftmp);
		}
		/* may be fault caused by transfer to/from user space */
		if (u.u_lofault == 0)
			goto die;

		switch (fmt.f_stkfmt) {
		case SF_MEDIUM: {
			struct bei_medium *beip =
			    (struct bei_medium *)&fmt.f_beibase;

			besize = sizeof (struct bei_medium);
			if (beip->bei_faultc || beip->bei_faultb)
				break;
			if (beip->bei_dfault && pagefault(beip->bei_fault))
				return (0);
			break;
			}
		case SF_LONGB: {
			struct bei_longb *beip =
			    (struct bei_longb *)&fmt.f_beibase;

			besize = sizeof (struct bei_longb);
			if (beip->bei_faultc || beip->bei_faultb)
				break;
			if (beip->bei_dfault && pagefault(beip->bei_fault))
				return (0);
			break;
			}
		default:
			panic("bad bus error stack format");
		}

		if (lodebug) {
			showregs("lofault", type, locregs, &fmt, be);
			traceback((long)locregs->r_areg[6],
			    (long)locregs->r_sp);
		}
		locregs->r_pc = u.u_lofault;
		return (besize);

	case T_ADDRERR:			/* address error */
		/* may be fault caused by transfer to/from user space */
		if (u.u_lofault == 0)
			goto die;
		switch (fmt.f_stkfmt) {
		case SF_MEDIUM: {
			struct bei_medium *beip =
			    (struct bei_medium *)&fmt.f_beibase;

			if (beip->bei_fcode != FC_UD)
				goto die;
			besize = sizeof (struct bei_medium);
			break;
			}
		case SF_LONGB: {
			struct bei_longb *beip =
			    (struct bei_longb *)&fmt.f_beibase;

			if (beip->bei_fcode != FC_UD)
				goto die;
			besize = sizeof (struct bei_longb);
			break;
			}
		default:
			panic("bad address error stack format");
		}
		if (lodebug) {
			showregs("lofault", type, locregs, &fmt, be);
			traceback((long)locregs->r_areg[6],
			    (long)locregs->r_sp);
		}
		locregs->r_pc = u.u_lofault;
		return (besize);

	case T_ADDRERR + USER:		/* user address error */
		if (tudebug)
			showregs("USER ADDRESS ERROR", type, locregs, &fmt, be);
		i = SIGBUS;
		switch (fmt.f_stkfmt) {
		case SF_MEDIUM:
			besize = sizeof (struct bei_medium);
			break;
		case SF_LONGB:
			besize = sizeof (struct bei_longb);
			break;
		default:
			panic("bad address error stack format");
		}
		break;

	case T_SPURIOUS:
	case T_SPURIOUS + USER:		/* spurious interrupt */
		i = spl7();
		printf("spurious level %d interrupt\n", (i & SR_INTPRI) >> 8);
		(void) splx(i);
		return (0);

	case T_PRIVVIO + USER:		/* privileged instruction fault */
		if (tudebug)
			showregs("USER PRIVILEGED INSTRUCTION", type, locregs,
			    &fmt, be);
		u.u_code = fmt.f_vector;
		i = SIGILL;
		break;

	case T_COPROCERR + USER:	/* coprocessor protocol error */
		/*
		 * Dump out obnoxious info to warn user
		 * that something isn't right w/ the 68881
		 */
		showregs("USER COPROCESSOR PROTOCOL ERROR", type, locregs,
		    &fmt, be);
		u.u_code = fmt.f_vector;
		i = SIGILL;
		break;

	case T_M_BADTRAP + USER:	/* (some) undefined trap */
	case T_ILLINST + USER:		/* illegal instruction fault */
		if (tudebug)
			showregs("USER ILLEGAL INSTRUCTION", type, locregs,
			    &fmt, be);
		u.u_code = fmt.f_vector;
		i = SIGILL;
		break;

	case T_M_FLOATERR + USER:	/* (some) floating error trap */
	case T_ZERODIV + USER:		/* divide by zero */
	case T_CHKINST + USER:		/* CHK [CHK2] instruction */
	case T_TRAPV + USER:		/* TRAPV [cpTRAPcc TRAPcc] instr */
		u.u_code = fmt.f_vector;
		i = SIGFPE;
		break;

	/*
	 * If the user SP is above the stack segment,
	 * grow the stack automatically.
	 */
	case T_BUSERR + USER: {

		if (be & BE_TIMEOUT)
			DELAY(2000);	/* allow for refresh recovery time */

		switch (fmt.f_stkfmt) {
		case SF_MEDIUM: {
			struct bei_medium *beip =
			    (struct bei_medium *)&fmt.f_beibase;

			besize = sizeof (struct bei_medium);
			/*
			 * check for any errors in buserr
			 * register besides invalid
			 */
			if (be & ~BE_INVALID)
				goto pferr;
			if ((bedebug && (beip->bei_faultb || beip->bei_faultc))
			    || (bedebug > 1 && beip->bei_fault))
				printf("medium fault b %d %x, c %d %x, d %d %x\n",
				    beip->bei_faultb, locregs->r_pc+4,
				    beip->bei_faultc, locregs->r_pc+2,
				    beip->bei_dfault, beip->bei_fault);

			if (beip->bei_dfault && ((beip->bei_fcode == FC_UD)
			    || (beip->bei_fcode == FC_UP))) {
				if (pagefault(beip->bei_fault))
					return (0);
				if (grow((unsigned)beip->bei_fault)) {
					nosig = 1;
					besize = 0;
					goto out;
				}
				goto pferr;
			}
			if (beip->bei_faultc) {
				if (pagefault(locregs->r_pc+2))
					return (0);
				goto pferr;
			}
			if (beip->bei_faultb) {
				if (pagefault(locregs->r_pc+4))
					return (0);
				goto pferr;
			}
			goto pferr;
		}
		case SF_LONGB: {
			struct bei_longb *beip =
			    (struct bei_longb *)&fmt.f_beibase;

			besize = sizeof (struct bei_longb);
			/*
			 * check for any errors in buserr
			 * register besides invalid
			 */
			if (be & ~BE_INVALID)
				goto pferr;
			if ((bedebug && (beip->bei_faultb || beip->bei_faultc))
			    || (bedebug > 1 && beip->bei_fault))
				printf("long fault b %d %x, c %d %x, d %d %x\n",
				    beip->bei_faultb, beip->bei_stageb,
				    beip->bei_faultc, beip->bei_stageb-2,
				    beip->bei_dfault, beip->bei_fault);

			if (beip->bei_dfault && ((beip->bei_fcode == FC_UD)
			    || (beip->bei_fcode == FC_UP))) {
				if (pagefault(beip->bei_fault))
					return (0);
				if (grow((unsigned)beip->bei_fault)) {
					nosig = 1;
					besize = 0;
					goto out;
				}
				goto pferr;
			}
			if (beip->bei_faultc) {
				if (pagefault(beip->bei_stageb-2))
					return (0);
				goto pferr;
			}
			if (beip->bei_faultb) {
				if (pagefault(beip->bei_stageb))
					return (0);
				goto pferr;
			}
			goto pferr;
		}
		default:
			panic("bad bus error stack format");
		}
	pferr:
		if (tudebug)
			showregs("USER BUS ERROR", type, locregs, &fmt, be);
		i = SIGSEGV;
		if (besize == 0)
			panic("besize");
		break;
		}

	case T_TRACE:			/* caused by tracing trap instr */
		u.u_pcb.pcb_p0lr |= TRACE_PENDING;
		return (0);

	case T_TRACE + USER:		/* trace trap */
		dotrace(locregs);
		goto out;

	case T_BRKPT + USER:		/* bpt instruction (trap #15) fault */
		locregs->r_pc = locregs->r_pc - 2;
		u.u_code = TRAP_BKPT;
		i = SIGTRAP;
		break;

	case T_EMU1010 + USER:		/* 1010 emulator trap */
	case T_EMU1111 + USER:		/* 1111 emulator trap */
		u.u_code = fmt.f_vector;
		i = SIGEMT;
		break;
	}

	psignal(u.u_procp, i);
out:
	if (u.u_pcb.pcb_p0lr & TRACE_PENDING)
		dotrace(locregs);
	p = u.u_procp;
	if (p->p_cursig || (p->p_sig && issig(p)))
		psig();
	p->p_pri = p->p_usrpri;
	if (runrun) {
		/*
		 * Since we are u.u_procp, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()'ed, we might not be on the queue indicated by
		 * our priority.
		 */
		(void) spl6();
		setrq(p);
		swtch();
		(void) spl0();
	}
	if (u.u_prof.pr_scale && (syst -= u.u_vm.vm_stime))
		addupc(locregs->r_pc, &u.u_prof, (int)-syst);
	curpri = p->p_pri;
	return (besize);
}

/*
 * Called from the trap handler when a system call occurs
 */
long syscount[0200];	/* temp */
syscall(code, regs)
	int code;
	struct regs regs;
{
	time_t syst;
	caddr_t params;
	int i;

	syst = u.u_vm.vm_stime;
	if (!USERMODE(regs.r_sr))
		panic("syscall");
	{
	/*
	 * At this point we declare a number of register variables.
	 * syscall_setjmp (called below) does not preserve the values
	 * of register variables, so we limit their scope to this block.
	 */
	register struct regs *locregs;
	register struct sysent *callp;
	register struct proc *p;

	p = u.u_procp;
	p->p_lnode->kl_cost += shconsts.sc_syscall;
	shconsts.sc_syscallc++;
	syscount[code&0177]++;
	locregs = &regs;
	u.u_ar0 = &locregs->r_dreg[0];
	params = (caddr_t)locregs->r_sp + 2 * NBPW;
	u.u_error = 0;
	callp = &sysent[code&0177];
	if (callp == sysent) {
		i = fuword(params);
		params += NBPW;
		callp = &sysent[i&0177];
	}
	if (callp->sy_narg) {
		if (fulwds((caddr_t)params, (caddr_t)u.u_arg,
		    callp->sy_narg)) {
			u.u_error = EFAULT;
			goto bad;
		}
	}
	u.u_ap = u.u_arg;
	u.u_dirp = (caddr_t)u.u_arg[0];
	u.u_r.r_val1 = 0;
	u.u_r.r_val2 = regs.r_dreg[1];
	syscnt[callp - sysent]++;
	/*
	 * Syscall_setjmp is a special setjmp that only saves a6 and sp.
	 * The result is a significant speedup of this critical path,
	 * but meanwhile all the register variables have the wrong
	 * values after a longjmp returns here.
	 * This is the reason for the limited scope of the register
	 * variables in this routine - the values may go away here.
	 */
	if (syscall_setjmp(u.u_qsav)) {
		if (u.u_error == 0 && u.u_eosys == JUSTRETURN)
			u.u_error = EINTR;
	} else {
		u.u_eosys = JUSTRETURN;
		(*(callp->sy_call))(u.u_ap);
	}
	/* end of scope of register variables above */
	}
	if (u.u_eosys != JUSTRETURN) {
		if (u.u_eosys == RESTARTSYS)
			regs.r_pc -= 2;
	} else {
		regs.r_sp += sizeof (int);	/* pop syscall # */
		if (u.u_error) {
bad:
			regs.r_dreg[0] = u.u_error;
			regs.r_sr |= SR_CC;	/* carry bit */
		} else {
			regs.r_sr &= ~SR_CC;
			regs.r_dreg[0] = u.u_r.r_val1;
			regs.r_dreg[1] = u.u_r.r_val2;
		}
	}
	if (u.u_pcb.pcb_p0lr & TRACE_PENDING)
		dotrace(&regs);
	{
	/* scope for use of register variable p */
	register struct proc *p;

	p = u.u_procp;
	if (p->p_cursig || (p->p_sig && issig(p)))
		psig();
	p->p_pri = p->p_usrpri;
	if (runrun) {
		/*
		 * Since we are u.u_procp, clock will normally just change
		 * our priority without moving us from one queue to another
		 * (since the running process is not on a queue.)
		 * If that happened after we setrq ourselves but before we
		 * swtch()'ed, we might not be on the queue indicated by
		 * our priority.
		 */
		(void) spl6();
		setrq(p);
		swtch();
		(void) spl0();
	}
	if (u.u_prof.pr_scale && (syst -= u.u_vm.vm_stime))
		addupc(regs.r_pc, &u.u_prof, (int)-syst);
	curpri = p->p_pri;
	}
}

/*
 * nonexistent system call-- set fatal error code.
 */
nosys()
{

	u.u_error = 100;
}

/*
 * Ignored system call
 */
nullsys()
{

}

/*
 * Handle trace traps, both real and delayed.
 */
dotrace(locregs)
	struct regs *locregs;
{
	register int r, s;
	struct proc *p = u.u_procp;

	s = spl6();
	r = u.u_pcb.pcb_p0lr&AST_CLR;
	u.u_pcb.pcb_p0lr &= ~AST_CLR;
	u.u_ar0[PS] &= ~PSL_T;
	(void) splx(s);
	if (r & TRACE_AST) {
		if ((p->p_flag&SOWEUPC) && u.u_prof.pr_scale) {
			addupc(locregs->r_pc, &u.u_prof, 1);
			p->p_flag &= ~SOWEUPC;
		}
		if ((r & TRACE_USER) == 0)
			return;
	}
	u.u_code = TRAP_TRACE;
	psignal(p, SIGTRAP);
}

/*
 * Print out a traceback for kernel traps
 */
traceback(afp, sp)
	long afp, sp;
{
	struct frame *tospage = (struct frame *)btoc(sp);
	struct frame *fp = (struct frame *)afp;
	static int done = 0;

	if (panicstr && done++ > 0)
		return;

	printf("Begin traceback...fp = %x, sp = %x\n", fp, sp);
	while (btoc(((int)fp)) == (int)tospage) {
		if (fp == fp->fr_savfp) {
			printf("FP loop at %x", fp);
			break;
		}
		printf("Called from %x, fp=%x, args=%x %x %x %x\n",
		    fp->fr_savpc, fp->fr_savfp,
		    fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2], fp->fr_arg[3]);
		fp = fp->fr_savfp;
	}
	printf("End traceback...\n");
	DELAY(2000000);
}

showregs(str, type, locregs, fmtp, be)
	char *str;
	int type;
	struct regs *locregs;
	struct stkfmt *fmtp;
{
	int *r, s;
	int fcode, accaddr;
	char *why;

	s = spl7();
	printf("%s: %s\n", u.u_comm, str ? str : "");
	printf(
	"trap address 0x%x, pid %d, pc = %x, sr = %x, stkfmt %x, context %x\n",
	    fmtp->f_vector, u.u_procp->p_pid, locregs->r_pc, locregs->r_sr,
	    fmtp->f_stkfmt, getcontext());
	type &= ~USER;
	if (type == T_BUSERR)
		printf("Bus Error Reg %b\n", be, BUSERR_BITS);
	if (type == T_BUSERR || type == T_ADDRERR) {
		switch (fmtp->f_stkfmt) {
		case SF_MEDIUM: {
			struct bei_medium *beip =
			    (struct bei_medium *)&fmtp->f_beibase;

			fcode = beip->bei_fcode;
			if (beip->bei_dfault) {
				why = "data";
				accaddr = beip->bei_fault;
			} else if (beip->bei_faultc) {
				why = "stage c";
				accaddr = locregs->r_pc+2;
			} else if (beip->bei_faultb) {
				why = "stage b";
				accaddr = locregs->r_pc+4;
			} else {
				why = "unknown";
				accaddr = 0;
			}
			printf("%s fault address %x faultc %d faultb %d ",
			    why, accaddr, beip->bei_faultc, beip->bei_faultb);
			printf("dfault %d rw %d size %d fcode %d\n",
			    beip->bei_dfault, beip->bei_rw,
			    beip->bei_size, fcode);
			break;
			}
		case SF_LONGB: {
			struct bei_longb *beip =
			    (struct bei_longb *)&fmtp->f_beibase;

			fcode = beip->bei_fcode;
			if (beip->bei_dfault) {
				why = "data";
				accaddr = beip->bei_fault;
			} else if (beip->bei_faultc) {
				why = "stage c";
				accaddr = beip->bei_stageb-2;
			} else if (beip->bei_faultb) {
				why = "stage b";
				accaddr = beip->bei_stageb;
			} else {
				why = "unknown";
				accaddr = 0;
			}
			printf("%s fault address %x faultc %d faultb %d ",
			    why, accaddr, beip->bei_faultc, beip->bei_faultb);
			printf("dfault %d rw %d size %d fcode %d\n",
			    beip->bei_dfault, beip->bei_rw,
			    beip->bei_size, fcode);
			break;
			}
		default:
			panic("bad bus error stack format");
		}
		if (fcode == FC_SD || fcode == FC_SP) {
			printf("KERNEL MODE\n");
			printf("page map %x\n", getpgmap((caddr_t)accaddr));
		} else {
			int tss, dss, sss, v;
			struct pmeg *pmp;
			struct proc *p = u.u_procp;
			struct pte *pte;

			v = btop(accaddr);
			tss = tptov(p, 0);
			dss = dptov(p, 0);
			sss = sptov(p, p->p_ssize - 1);
			if (v >= tss && v < tss + p->p_tsize ||
			    v >= dss && v < dss + p->p_dsize ||
			    v >= sss && v < sss + p->p_ssize) {
				pmp = &pmeg[p->p_ctx->ctx_pmeg[v/NPAGSEG]];
				pte = vtopte(p, (unsigned)v);
				printf("pagefault, pmp %x, pte %x %x\n",
				    pmp, pte, *pte);
				printf("pme %x\n", getpgmap((caddr_t)accaddr));
			} else {
				printf("bad addr, v %d tss %d dss %d sss %d\n",
					v, tss, dss, sss);
			}
		}
	}
	r = &locregs->r_dreg[0];
	printf("D0-D7  %x %x %x %x %x %x %x %x\n",
	    r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
	r = &locregs->r_areg[0];
	printf("A0-A7  %x %x %x %x %x %x %x %x\n",
	    r[0], r[1], r[2], r[3], r[4], r[5], r[6], r[7]);
	DELAY(2000000);
	(void) splx(s);
}
