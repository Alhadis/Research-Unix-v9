/*	clock.c	4.23	81/07/09	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dk.h"
#include "../h/callout.h"
#include "../h/seg.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/lnode.h"
#include "../h/proc.h"
#include "../h/reg.h"
#include "../h/psl.h"
#include "../h/vm.h"
#include "../h/buf.h"
#include "../h/text.h"
#include "../h/vlimit.h"
#include "../h/mtpr.h"
#include "../h/clock.h"
#include "../h/cpu.h"
#include "../h/share.h"

int	queueflag;
int	queuetime;
int	intrtime;

/*
 * Hardclock is called straight from
 * the real time clock interrupt.
 * We limit the work we do at real clock interrupt time to:
 *	reloading clock
 *	decrementing time to callouts
 *	recording cpu time usage
 *	modifying priority of current process
 *	arrange for soft clock interrupt
 *	kernel pc profiling
 *
 * At software (softclock) interrupt time we:
 *	implement callouts
 *	maintain date
 *	lightning bolt wakeup (every second)
 *	alarm clock signals
 *	jab the scheduler
 *
 * On the vax softclock interrupts are implemented by
 * software interrupts.  Note that we may have multiple softclock
 * interrupts compressed into one (due to excessive interrupt load),
 * but that hardclock interrupts should never be lost.
 */
int	pshi;
int	pswaslo;
int	ps_hist[32];
caddr_t ps_pc[256], *pspcp = ps_pc;
/*ARGSUSED*/
hardclock(pc, ps)
	caddr_t pc;
{
	register struct callout *p1;
	register struct proc *pp;
	register int s, cpstate;

	/*
	 * reprime clock
	 */
	clkreld();

	/*
	 * priority statistics
	 */
	if ((ps & PSL_IPL) >= 0x140000) {
		pshi++;
		if (pshi > 20) {
			*pspcp++ = pc;
			if (pspcp >= &ps_pc[256])
				pspcp = ps_pc;
		}
	} else if (pshi) {
		if (pshi >= 32)
			pshi = 31;
		ps_hist[pshi]++;
		pshi = 0;
	}
	if (pswaslo) {
		pswaslo = 0;
		pshi = 0;
	}

	/*
	 * update callout times
	 */
	for (p1 = calltodo.c_next; p1 && p1->c_time <= 0; p1 = p1->c_next)
		;
	if (p1)
		p1->c_time--;

	/*
	 * Update iostat information.
	 */
	for (s = 0; s < DK_NDRIVE; s++)
		if (dk_busy&(1<<s))
			dk_time[s]++;

	if (USERMODE(ps)) {
		u.u_vm.vm_utime++;
		if (u.u_procp->p_nice > NZERO)
			cpstate = CP_NICE;
		else
			cpstate = CP_USER;
		s = 1;
	} else {
		cpstate = CP_SYS;
		if ((ps & PSL_IPL) >= 0x140000) {
			intrtime++;
			s = 0;
		} else if (queueflag) {
			cpstate = CP_QUEUE;
			s = 0;
		} else if (noproc) {
			if ((ps & PSL_IPL) == 0)
				cpstate = CP_IDLE;
			s = 0;
		} else {
			u.u_vm.vm_stime++;
			s = 1;
		}
	}
	cp_time[cpstate]++;

	/*
	 * Adjust priority of current process.
	 */
	if (s) {
		register KL_p lp;

		pp = u.u_procp;
		pp->p_cpticks++;
		lp = pp->p_lnode;
		if ( (s = pp->p_nice) >= (2*NZERO) || s < 0 )
			s = 2*NZERO - 1;
		lp->kl_cost += NiceTicks[s];
		shconsts.sc_tickc++;
		if ((pp->p_sharepri += lp->kl_usage * lp->kl_rate) > MAXUPRI) {
			if (lp->kl_norms)			/* root has shares */
				pp->p_sharepri = MAXUPRI;	/* Ceiling for real procs */
			else
				pp->p_sharepri = MAXPRI;	/* Idle procs */
		}
		if (pp->p_sharepri > MaxSharePri && lp->kl_usage <= MaxUsage)
			MaxSharePri = pp->p_sharepri;
	}

	/*
	 * Time moves on.
	 */
	++lbolt;

#if VAX780
	/*
	 * On 780's, implement a fast UBA watcher,
	 * to make sure uba's don't get stuck.
	 */
	if (cpu == VAX_780 && panicstr == 0 && !BASEPRI(ps))
		unhang();
#endif

	/*
	 * Schedule a software interrupt for the rest
	 * of clock activities.
	 */
	setsoftclock();
}

/*
 * Constant for decay filter for cpu usage field
 * in process table (used by ps au).
 */
float	ccpu = 0.9512294245;		/* exp(-1/20) */
float	omccpu = 0.0;

/*
 * Software clock interrupt.
 * This routine runs at lower priority than device interrupts.
 *
 * Processes have their (32-bit) priority depend on their owner's ``normalised
 * usage'' of resources. However, V9's low-level scheduler only has 127 priorities,
 * (in fact, only 32, so that someone could use an 'ffs' instruction),
 * so we normalise this ``sharepri'' into the 7-bit ``usrpri''. Note that
 * the 'ffs' hack means that (after PUSER) there are only 20 real priorities
 * for processes to run in, and that defined kernel priorities should differ
 * by more than 4 to be meaningful.
 */
/*ARGSUSED*/
softclock(pc, ps)
	caddr_t pc;
{
	register struct callout *p1;
	register struct proc *pp;
	register int s;
	register int a;
	static meterclock = 0;
	static sharesecs = 0;

	pswaslo = 1;
	/*
	 * Perform callouts (but not after panic's!)
	 */
	if (panicstr == 0) {
		for (;;) {
			register caddr_t arg;
			register int (*func)();

			s = spl7();
			if ((p1 = calltodo.c_next) == 0 || p1->c_time > 0) {
				splx(s);
				break;
			}
			calltodo.c_next = p1->c_next;
			arg = p1->c_arg;
			func = p1->c_func;
			p1->c_next = callfree;
			callfree = p1;
			(void) splx(s);
			(*func)(arg);
		}
	}

	/*
	 * If idling and processes are waiting to swap in,
	 * check on them.
	 */
	if (noproc && runin) {
		runin = 0;
		wakeup((caddr_t)&runin);
	}

	/*
	 * Run paging daemon every 1/4 sec.
	 */
	if (lbolt % (hz/4) == 0) {
		vmpago();
	}

	/*
	 * Reschedule every 1/10 sec.
	 */
	if (lbolt % (hz/10) == 0) {
		runrun++;
		aston();
	}

	/*
	 * Lightning bolt every second:
	 *	sleep timeouts
	 *	process priority recomputation
	 *	process %cpu averaging
	 *	virtual memory metering
	 *	kick swapper if processes want in
	 */
	if (lbolt >= hz) {
		/*
		 * This doesn't mean much on VAX since we run at
		 * software interrupt time... if hardclock()
		 * calls softclock() directly, it prevents
		 * this code from running when the priority
		 * was raised when the clock interrupt occurred.
		 */
		if (BASEPRI(ps))
			return;

		/*
		 * If we didn't run a few times because of
		 * long blockage at high ipl, we don't
		 * really want to run this code several times,
		 * so squish out all multiples of hz here.
		 * Then trim the software clock.
		 */
		time += lbolt / hz;
		lbolt %= hz;
		clktrim();

		/*
		 * Run share every Delta seconds.
		 */
		if (++sharesecs >= Delta) {
			sharesecs -= Delta;
			setshsched();
		}

		/*
		 * Decay per user active process count.
		 */
		decayrate();

		/*
		 * Wakeup lightning bolt sleepers.
		 * Processes sleep on lbolt to wait
		 * for short amounts of time (e.g. 1 second).
		 */
		wakeup((caddr_t)&lbolt);

		/*
		 * Recompute process priority and process
		 * sleep() system calls as well as internal
		 * sleeps with timeouts (tsleep() kernel routine).
		 */
		if (omccpu == zerof)
			omccpu = (onef - ccpu) / (float)hz;
		MaxSharePri = onef;
		for (pp = proc; pp < procNPROC; pp++)
		if ((a = pp->p_stat) && a!=SZOMB) {
			register KL_p lp;

			/*
			 * Increase resident time, to max of 127 seconds
			 * (it is kept in a character.)  For
			 * loaded processes this is time in core; for
			 * swapped processes, this is time on drum.
			 */
			if (pp->p_time != 127)
				pp->p_time++;
			/*
			 * If process has clock counting down, and it
			 * expires, set it running (if this is a tsleep()),
			 * or give it an SIGALRM (if the user process
			 * is using alarm signals.
			 */
			if (pp->p_clktim && --pp->p_clktim == 0)
				psignal(pp, SIGALRM);
			if (pp->p_tsleep && --pp->p_tsleep == 0) {
				s = spl6();
				switch (a) {

				case SSLEEP:
					setrun(pp);
					break;

				case SSTOP:
					unsleep(pp);
					break;
				}
				pp->p_flag |= STIMO;
				splx(s);
			}
			/*
			 * If process is blocked, increment computed
			 * time blocked.  This is used in swap scheduling.
			 */
			if (a==SSLEEP || a==SSTOP)
				if (pp->p_slptime != 127)
					pp->p_slptime++;
			/*
			 * Update estimations of process cpu utilization.
			 */
			pp->p_pctcpu *= ccpu;
			if (s = pp->p_cpu) {
				pp->p_cpu = s >> 1;
			}
			if (s = pp->p_cpticks) {
				pp->p_pctcpu += omccpu * (float)s;
				pp->p_cpu += s;
				pp->p_cpticks = 0;
			}
			/*
			 * Decay priority based on nice.
			 */
			if ((s = pp->p_nice) >= (2*NZERO) || s < 0)
				s = 2*NZERO - 1;
			lp = pp->p_lnode;
			if (lp->kl_norms) {
				pp->p_sharepri *= NiceDecays[s];
				if (pp->p_sharepri > MaxSharePri && lp->kl_usage <= MaxUsage)
					MaxSharePri = pp->p_sharepri;
			} else
				pp->p_sharepri = MAXPRI;
			/*
			 * Update active process count.
			 */
			if (a == SRUN || a == SSLEEP && pp->p_pri <= PZERO)
				lp->kl_rate += NiceRates[s];
		}

		/*
		 * Now re-scan the procs, setting the normalised usrpri.
		 */

		for (pp = proc; pp < procNPROC; pp++)
		if ((a = pp->p_stat) && a!=SZOMB) {
			(void) setpri(pp);
			/*
			 * Now have computed new process priority
			 * in p->p_usrpri.  Carefully change p->p_pri.
			 * A process is on a run queue associated with
			 * this priority, so we must block out process
			 * state changes during the transition.
			 */
			s = spl6();
			if (pp->p_pri >= PUSER && pp->p_pri != pp->p_usrpri) {
				if ((pp != u.u_procp || noproc) && a == SRUN &&
				    (pp->p_flag & SLOAD)) {
					remrq(pp);
					pp->p_pri = pp->p_usrpri;
					setrq(pp);
				} else
					pp->p_pri = pp->p_usrpri;
			}
			splx(s);
		}

		/*
		 * If the swap process is trying to bring
		 * a process in, have it look again to see
		 * if it is possible now.
		 */
		if (runin!=0) {
			runin = 0;
			wakeup((caddr_t)&runin);
		}

		/*
		 * If there are pages that have been cleaned, 
		 * jolt the pageout daemon to process them.
		 * We do this here so that these pages will be
		 * freed if there is an abundance of memory and the
		 * daemon would not be awakened otherwise.
		 */
		if (bclnlist != NULL)
			wakeup((caddr_t)&proc[2]);
	}

	/*
	 * Perform virtual memory metering, decoupled from 1-sec uproar
	 */
	meterclock = (meterclock+11) % (hz-1);	/* 11 is a prominent non-divisor */
	if (meterclock == lbolt)
		vmmeter();

	if (noproc)
		return;
	pp = u.u_procp;

	/*
	 * If trapped user-mode, give it a profiling tick.
	 */
	if (USERMODE(ps) && u.u_prof.pr_scale) {
		pp->p_flag |= SOWEUPC;
		aston();
	}

	/*
	 * Maintain per-process cpu statistics
	 */
	s = pp->p_rssize;
	u.u_vm.vm_idsrss += s;
	if (pp->p_textp) {
		register int xrss = pp->p_textp->x_rssize;

		s += xrss;
		u.u_vm.vm_ixrss += xrss;
	}
	if (s > u.u_vm.vm_maxrss)
		u.u_vm.vm_maxrss = s;
	if ((u.u_vm.vm_utime+u.u_vm.vm_stime+1)/hz > u.u_limit[LIM_CPU]) {
		psignal(pp, SIGXCPU);
		if (u.u_limit[LIM_CPU] < INFINITY - 5)
			u.u_limit[LIM_CPU] += 5;
	}
}

/*
 * Timeout is called to arrange that
 * fun(arg) is called in tim/hz seconds.
 * An entry is linked into the callout
 * structure.  The time in each structure
 * entry is the number of hz's more
 * than the previous entry.
 * In this way, decrementing the
 * first entry has the effect of
 * updating all entries.
 *
 * The panic is there because there is nothing
 * intelligent to be done if an entry won't fit.
 */
timeout(fun, arg, tim)
	int (*fun)();
	caddr_t arg;
{
	register struct callout *p1, *p2, *pnew;
	register int t;
	int s;

	t = tim;
	s = spl7();
	pnew = callfree;
	if (pnew == NULL)
		panic("timeout table overflow");
	callfree = pnew->c_next;
	pnew->c_arg = arg;
	pnew->c_func = fun;
	for (p1 = &calltodo; (p2 = p1->c_next) && p2->c_time < t; p1 = p2)
		t -= p2->c_time;
	p1->c_next = pnew;
	pnew->c_next = p2;
	pnew->c_time = t;
	if (p2)
		p2->c_time -= t;
	splx(s);
}

/*
 *      Delay goes to sleep on a unique address for a
 *      guaranteed minimum period ticks/HZ secs.
 *      Because a timeout() can't be cancelled, the process
 *      will be unkillable while asleep.  Beware of giving
 *      delay() an argument of more than a few hundred.
 */
delay(ticks)
{
        register int    x;
        extern wakeup();

        if (ticks<=0)
                return;
        x = spl7();
        timeout(wakeup, (caddr_t)u.u_procp+1, ticks);
        sleep((caddr_t)u.u_procp+1, PZERO-1);
        splx(x);
}
