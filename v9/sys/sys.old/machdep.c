/*	machdep.c	4.36	81/05/09	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/reg.h"
#include "../h/mtpr.h"
#include "../h/clock.h"
#include "../h/pte.h"
#include "../h/vm.h"
#include "../h/lnode.h"
#include "../h/proc.h"
#include "../h/psl.h"
#include "../h/buf.h"
#include "../h/nexus.h"
#include "../h/ubavar.h"
#include "../h/ubareg.h"
#include "../h/cons.h"
#include "../h/reboot.h"
#include "../h/conf.h"
#include "../h/mem.h"
#include "../h/cpu.h"
#include "../h/inode.h"
#include "../h/file.h"
#include "../h/text.h"
#include "../h/callout.h"
#include "../h/cmap.h"
#include <frame.h>
#include "../h/rpb.h"
#include <time.h>

int	icode[] =
{
	0x9f19af9f,	/* pushab [&"init",0]; pushab */
	0x02dd09af,	/* "/etc/init"; pushl $2 */
	0xbc5c5ed0,	/* movl sp,ap; chmk */
	0x2ffe110b,	/* $exec; brb .; "/ */
	0x2f637465,	/* etc/ */
	0x74696e69,	/* init" */
	0x00000000,	/* \0\0\0";  0 */
	0x00000014,	/* [&"init", */
	0x00000000,	/* 0] */
};
int	szicode = sizeof(icode);
 
/*
 * Declare these as initialized data so we can patch them.
 */
int	nbuf = 0;
int	nswbuf = 0;

/*
 * Machine-dependent startup code
 */
startup(firstaddr)
	int firstaddr;
{
	register int unixsize;
	register unsigned i;
	register struct pte *pte;
	register caddr_t v;

	/*
	 * Initialize error message buffer (at end of core).
	 */
	maxmem -= CLSIZE;
	pte = msgbufmap;
	for (i = 0; i < CLSIZE; i++)
		*(int *)pte++ = PG_V | PG_KW | (maxmem + i);
	mtpr(TBIA, 1);

	/*
	 * Good {morning,afternoon,evening,night}.
	 */
	printf(version);
	
	/*
	 * First determine how many buffers are reasonable.
	 * Current alg is 32 per megabyte, with min of 32.
	 * We allocate 1/2 as many swap buffer headers as file i/o buffers.
	 */
	if (nbuf == 0) {
		nbuf = (32 * physmem) / btoc(1024*1024);
		if (nbuf < 32)
			nbuf = 32;
	}
	if (nswbuf == 0) {
		nswbuf = (nbuf / 2) &~ 1;	/* force even */
		if (nswbuf > 256)
			nswbuf = 256;		/* sanity */
	}
	printf("real mem = %d nbuf = %d nswbuf = %d\n", ctob(maxmem),
		nbuf, nswbuf);

	/*
	 * Allocate space for system data structures.
	 */
	v = (caddr_t)(0x80000000 | (firstaddr * NBPG));
#define	valloc(name, type, num) \
	    (name) = (type *)(v); (v) = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)(v); (v) = (caddr_t)((lim) = ((name)+(num)))
	valloc(buffers, char, BUFSIZE*nbuf);
	valloc(buf, struct buf, nbuf);
	valloc(swbuf, struct buf, nswbuf);
	valloc(swsize, short, nswbuf);	/* note: nswbuf is even */
	valloc(swpf, int, nswbuf);
	valloclim(inode, struct inode, ninode, inodeNINODE);
	valloclim(file, struct file, nfile, fileNFILE);
	valloclim(proc, struct proc, nproc, procNPROC);
	valloclim(text, struct text, ntext, textNTEXT);
	valloclim(lnodes, struct kern_lnode, maxusers, lnodesMAXUSERS);
	valloc(callout, struct callout, ncallout);
	valloc(swapmap, struct map, nswapmap = nproc * 2);
	valloc(argmap, struct map, ARGMAPSIZE);
	valloc(kernelmap, struct map, nproc);

	/*
	 * Now allocate space for core map
	 */
	ncmap = (physmem*NBPG - ((int)v &~ 0x80000000)) /
		    (NBPG*CLSIZE + sizeof (struct cmap));
	valloclim(cmap, struct cmap, ncmap, ecmap);
	if ((((int)(ecmap+1))&~0x80000000) > SYSPTSIZE*NBPG)
		panic("sys pt too small");

	/*
	 * Clear allocated space, and make r/w entries
	 * for the space in the kernel map.
	 */
	unixsize = btoc((int)(ecmap+1) &~ 0x80000000);
	if (unixsize >= physmem - 8*UPAGES)
		panic("no memory");
	for (i = firstaddr; i < unixsize; i++) {
		*(int *)(&Sysmap[i]) = PG_V | PG_KW | i;
		clearseg(i);
	}
	mtpr(TBIA, 1);

	/*
	 * Initialize callouts
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

	/*
	 * Initialize memory allocator and swap
	 * and user page table maps.
	 *
	 * THE USER PAGE TABLE MAP IS CALLED ``kernelmap''
	 * WHICH IS A VERY UNDESCRIPTIVE AND INCONSISTENT NAME.
	 */
	meminit(unixsize, maxmem);
	maxmem = freemem;
	printf("avail mem = %d\n", ctob(maxmem));
	rminit(kernelmap, USRPTSIZE, 1, "usrpt", nproc);

	/*
	 * Configure the system.
	 */
	configure();

	/*
	 * Clear restart inhibit flags.
	 */
	tocons(TXDB_CWSI);
	tocons(TXDB_CCSI);
}

/*
 * set up a physical address
 * into users virtual address space.
 */
sysphys()
{

	if(!suser())
		return;
	u.u_error = EINVAL;
}

/*
 * Initialze the clock, based on the time base which is, e.g.
 * from a filesystem.  Base provides the time to within six months,
 * and the time of year clock provides the rest.
 */
clkinit(base)
	time_t base;
{
	register unsigned todr = mfpr(TODR);
	long deltat;
	int year = YRREF;
	unsigned secyr;

#ifdef MVAX
	/*
	 * Get the MicroVAX-II's toy register.
	 * Convert it to the time format that the todr is
	 * normally kept in.
	 */
	if( cpu == M_VAX ) {
		todr = gettoy();
		for (;;) {
			secyr = SECYR;
			if (LEAPYEAR(year))
				secyr += SECDAY;
			if (todr < secyr)
				break;
			todr -= secyr;
			year++;
			}
		year = YRREF;
		todr = TODRZERO + todr*100;
	}
#endif MVAX
	if (base < 5*SECYR) {
		printf("WARNING: preposterous time in file system");
		time = 6*SECYR + 186*SECDAY + SECDAY/2;
		clkset();
		goto check;
	}
	/*
	 * Have been told that VMS keeps time internally with base TODRZERO.
	 * If this is correct, then this routine and VMS should maintain
	 * the same date, and switching shouldn't be painful.
	 * (Unfortunately, VMS keeps local time, so when you run UNIX
	 * and VMS, VMS runs on GMT...).
	 */
	if (todr < TODRZERO) {
		printf("WARNING: todr too small");
		time = base;
		/*
		 * Believe the time in the file system for lack of
		 * anything better, resetting the TODR.
		 */
		clkset();
		goto check;
	}
	/*
	 * Sneak to within 6 month of the time in the filesystem,
	 * by starting with the time of the year suggested by the TODR,
	 * and advancing through succesive years.  Adding the number of
	 * seconds in the current year takes us to the end of the current year
	 * and then around into the next year to the same position.
	 */
	for (time = (todr-TODRZERO)/100; time < base-SECYR/2; time += SECYR) {
		if (LEAPYEAR(year))
			time += SECDAY;
		year++;
	}

	/*
	 * The hardware and software clocks are now in sync, so it is
	 * safe to call clkset() to get its side effect, which is to
	 * initialize yearbase.
	 */
	clkset();

	/*
	 * See if we gained/lost two or more days;
	 * if so, assume something is amiss.
	 */
	deltat = time - base;
	if (deltat < 0)
		deltat = -deltat;
	if (deltat < 2*SECDAY)
		return;
	printf("WARNING: clock %s %d days",
	    time < base ? "lost" : "gained", deltat / SECDAY);
check:
	printf(" -- CHECK AND RESET THE DATE!\n");
}

/* Time in seconds from the epoch to the start of the current year */
time_t	yearbase;

/*
 * Reset the TODR based on the time value; used when the TODR
 * has a preposterous value and also when the time is reset
 * by the stime system call.  Also called when the TODR goes past
 * TODRZERO + 100*(SECYEAR+2*SECDAY) (e.g. on Jan 2 just after midnight)
 * to wrap the TODR around.
 *
 * Side effect: yearbase is set appropriately.
 */

clkset()
{
	int year = YRREF;
	unsigned secyr;
	unsigned yrtime = time;

	yearbase = 0;

	/*
	 * Whittle the time down to an offset in the current year,
	 * by subtracting off whole years as long as possible.
	 */
#ifdef MVAX
	if( cpu == M_VAX ) {
		settoy( yrtime );
		return;
	}
#endif MVAX
	for (;;) {
		secyr = SECYR;
		if (LEAPYEAR(year))
			secyr += SECDAY;
		if (yrtime < secyr)
			break;
		yrtime -= secyr;
		yearbase += secyr;
		year++;
	}
	mtpr(TODR, TODRZERO + yrtime*100);
}

#ifdef MVAX
/*
 * This routine sets the time of year clock on the MicroVAX-II.
 */
static	int	dmsize[12] =
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

#define	dysize(A) (((A)%4)? 365: 366)

gettoy()
{
	int i, sec=0;
	struct tm tm;
	int s;
	register struct qb_regs *qb_regs = (struct qb_regs *)nexus;

	/*
	 * Copy the toy register contents into tm so that we can
	 * work with. The toy must be completely read in 2.5 millisecs.
	 *
	 *
	 * Wait for update in progress to be done.
	 */
	while( qb_regs->qb_toycsra & QBT_UIP )
		;
	s = spl7();
	tm.tm_sec = qb_regs->qb_toysecs;
	tm.tm_min = qb_regs->qb_toymins;
	tm.tm_hour = qb_regs->qb_toyhours;
	tm.tm_mday = qb_regs->qb_toyday;
	tm.tm_mon = qb_regs->qb_toymonth;
	tm.tm_year = qb_regs->qb_toyyear;
	splx( s );
	/*
	 * sanity check the clock
	 */
	if( tm.tm_sec < 0 || tm.tm_sec >59  ||
		tm.tm_min < 0 || tm.tm_min > 59 ||
		tm.tm_hour < 0 || tm.tm_hour > 23  ||
		tm.tm_mday < 1 || tm.tm_mday > 31 ||
		tm.tm_mon < 1 || tm.tm_mon >12 ||
		tm.tm_year < 70 || tm.tm_year > 99 )
			return 0;
	/*
	 * Added up the seconds since the epoch
	 */
	tm.tm_year += 1900;
	for (i = 1970; i < tm.tm_year; i++)
		sec += dysize(i);
	/* 
	 * Leap tm.tm_year 
	 */
	if (dysize(tm.tm_year) == 366 && tm.tm_mon >= 3)
		sec++;
	/*
	 * Do the current tm.tm_year
	 */
	for( i=0 ; i < tm.tm_mon-1 ; i++ )
		sec += dmsize[i];
	sec += tm.tm_mday-1;
	sec = 24*sec + tm.tm_hour;
	sec = 60*sec + tm.tm_min;
	sec = 60*sec + tm.tm_sec;
	return (sec);
}

/*
 * This routine is used to set the MicroVAX-II toy register.
 */
settoy( tim )
{
	register int d0, d1;
	long hms, day;
	register int *tp;
	struct tm xtime;
	int s;
	register struct qb_regs *qb_regs = (struct qb_regs *)nexus;

	/*
	 * break initial number into days
	 */
	hms = tim % 86400;
	day = tim / 86400;
	if (hms<0) {
		hms += 86400;
		day -= 1;
	}
	tp = (int *)&xtime;

	/*
	 * generate hours:minutes:seconds
	 */
	*tp++ = hms%60;
	d1 = hms/60;
	*tp++ = d1%60;
	d1 /= 60;
	*tp++ = d1;

	/*
	 * year number
	 */
	if (day>=0) for(d1=70; day >= dysize(d1); d1++)
		day -= dysize(d1);
	else for (d1=70; day<0; d1--)
		day += dysize(d1-1);
	xtime.tm_year = d1;
	xtime.tm_yday = d0 = day;

	/*
	 * generate month
	 */

	if (dysize(d1)==366)
		dmsize[1] = 29;
	for(d1=0; d0 >= dmsize[d1]; d1++)
		d0 -= dmsize[d1];
	dmsize[1] = 28;
	*tp++ = d0+1;
	*tp++ = d1+1;
	xtime.tm_isdst = 0;
	/*
	 * Copy the time into the toy.
	 */
	qb_regs->qb_toycsrb = QBT_SETUP;
	s = spl7();
	qb_regs->qb_toysecs = xtime.tm_sec;
	qb_regs->qb_toymins = xtime.tm_min;
	qb_regs->qb_toyhours = xtime.tm_hour;
	qb_regs->qb_toyday = xtime.tm_mday;
	qb_regs->qb_toymonth = xtime.tm_mon;
	qb_regs->qb_toyyear = xtime.tm_year;
	splx( s );
	/*
	 * Start the clock again.
	 */
	qb_regs->qb_toycsra = QBT_SETA;
	qb_regs->qb_toycsrb = QBT_SETB;
}
#endif MVAX


unsigned trimprof[3];

/*
 * Tweak the rate of the software clock
 * to track the hardware clock accurately.
 * This routine is called once a second,
 * immediately after 'time' has been incremented.
 */
clktrim()
{
	register long delta;
	register long incr;
	register unsigned todr = mfpr(TODR);

#ifdef MVAX
	/*
	 * Get the MicroVAX-II's toy register.
	 * Convert it to the time format that the todr is
	 * normally kept in.
	 */
	if( cpu == M_VAX ) {
		int year = YRREF;
		unsigned secyr;

		todr = gettoy();
		for (;;) {
			secyr = SECYR;
			if (LEAPYEAR(year))
				secyr += SECDAY;
			if (todr < secyr)
				break;
			todr -= secyr;
			year++;
			}
		todr = TODRZERO + todr*100;
	}
#endif MVAX

	delta = 100 * (time - yearbase) - todr + TODRZERO;

	/*
	 * If delta > 0, the software clock is running fast.
	 * It is measured in centiseconds, the units of the
	 * hardware clock.  If delta is non-zero, we will
	 * stretch or shrink each tick by 111 microseconds
	 * from the nominal, for a drift of about 2/3 of a
	 * percent.  This should result by changing delta by
	 * about 2/3 each second, thus making it very unlikely
	 * that we will overshoot delta == 0.  On the other
	 * hand, we will be able to absorb 16 minutes of drift
	 * each day.  We also count each kind of tweak.
	 */
	
	incr = -16667;		/* -1/60 of a second, in microseconds */
	if (delta > 0) {
			incr -= 111;
			trimprof[2]++;
	} else if (delta < 0) {
			incr += 111;
			trimprof[0]++;
	} else {
		trimprof[1]++;
	}

	/* tell the hardware about it */
	/* unfortunately this does nothing on the MVAX */
	mtpr (NICR, incr);
}

/*
 * This system call sets the time of year clock without touching
 * the software clock.  It returns the previous clock value.  If
 * the argument is zero or the caller is not the super-user, it
 * does not change the clock.
 */
settod()
{
	register struct a {
		long unsigned tod;
	} *uap;
	register unsigned todr = mfpr(TODR);

#ifdef MVAX
	/*
	 * Get the MicroVAX-II's toy register.
	 * Convert it to the time format that the todr is
	 * normally kept in.
	 */
	if( cpu == M_VAX ) {
		int year = YRREF;
		unsigned secyr;

		todr = gettoy();
		for (;;) {
			secyr = SECYR;
			if (LEAPYEAR(year))
				secyr += SECDAY;
			if (todr < secyr)
				break;
			todr -= secyr;
			year++;
			}
		todr = TODRZERO + todr*100;
	}
#endif MVAX

	uap = (struct a *) u.u_ap;
	u.u_r.r_val1 = todr;
#ifdef MVAX
	if (uap->tod != 0 && suser() && cpu == M_VAX) {
		settoy(uap->tod);
		return;
	}
#endif MVAX
	if (uap->tod != 0 && suser())
		mtpr (TODR, uap->tod);
}

/*
 * Return the difference (in microseconds)
 * between the  current time and a previous
 * time as represented  by the arguments.
 * If there is a pending clock interrupt
 * which has not been serviced due to high
 * ipl, return error code.
 */
vmtime(otime, olbolt, oicr)
	register int otime, olbolt, oicr;
{

	if (mfpr(ICCS)&ICCS_INT)
		return(-1);
	else
		return(((time-otime)*60 + lbolt-olbolt)*16667 + mfpr(ICR)-oicr);
}

/*
 * Send an interrupt to process
 *
 * SHOULD CHANGE THIS TO PASS ONE MORE WORK SO THAT ALL INFORMATION
 * PROVIDED BY HARDWARE IS AVAILABLE TO THE USER PROCESS.
 */
sendsig(p, n)
	int (*p)();
{
	register int *usp, *regs;

	regs = u.u_ar0;
	usp = (int *)regs[SP];
	usp -= 5;
	if ((int)usp <= USRSTACK - ctob(u.u_ssize))
		(void) grow((unsigned)usp);
	;			/* Avoid asm() label botch */
#ifndef lint
	asm("probew $3,$20,(r11)");
	asm("beql bad");
#else
	if (useracc((caddr_t)usp, 0x20, 1))
		goto bad;
#endif
	*usp++ = n;
	if (n == SIGILL || n == SIGFPE) {
		*usp++ = u.u_code;
		u.u_code = 0;
	} else
		*usp++ = 0;
	*usp++ = (int)p;
	*usp++ = regs[PC];
	*usp++ = regs[PS];
	regs[SP] = (int)(usp - 5);
	regs[PS] &= ~(PSL_CM|PSL_FPD);
	regs[PC] = (int)u.u_pcb.pcb_sigc;
	return;

asm("bad:");
bad:
	/*
	 * Process has trashed its stack; give it an illegal
	 * instruction to halt it in its tracks.
	 */
	u.u_signal[SIGILL] = SIG_DFL;
	u.u_procp->p_siga0 &= ~(1<<(SIGILL-1));
	u.u_procp->p_siga1 &= ~(1<<(SIGILL-1));
	psignal(u.u_procp, SIGILL);
}

int userreg[] = {R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, AP, FP, SP, PC};

dorti()
{
	struct frame frame;
	register int sp;
	register int reg, mask;

	(void) copyin((caddr_t)u.u_ar0[FP], (caddr_t)&frame, sizeof (frame));
	sp = u.u_ar0[FP] + sizeof (frame);
	u.u_ar0[PC] = frame.fr_savpc;
	u.u_ar0[FP] = frame.fr_savfp;
	u.u_ar0[AP] = frame.fr_savap;
	mask = frame.fr_mask;
	for (reg = 0; reg <= 11; reg++) {
		if (mask&1) {
			u.u_ar0[userreg[reg]] = fuword((caddr_t)sp);
			sp += 4;
		}
		mask >>= 1;
	}
	sp += frame.fr_spa;
	u.u_ar0[PS] = (u.u_ar0[PS] & 0xffff0000) | frame.fr_psw;
	if (frame.fr_s)
		sp += 4 + 4 * (fuword((caddr_t)sp) & 0xff);
	/* phew, now the rei */
	u.u_ar0[PC] = fuword((caddr_t)sp);
	sp += 4;
	u.u_ar0[PS] = fuword((caddr_t)sp);
	sp += 4;
	u.u_ar0[PS] |= PSL_USERSET;
	u.u_ar0[PS] &= ~PSL_USERCLR;
	u.u_ar0[SP] = (int)sp;
}

/*
 * Invalidate single all pte's in a cluster
 */
tbiscl(v)
	unsigned v;
{
	register caddr_t addr;		/* must be first reg var */
	register int i;

	asm(".set TBIS,58");
	addr = ptob(v);
	for (i = 0; i < CLSIZE; i++) {
#ifdef lint
		mtpr(TBIS, addr);
#else
		asm("mtpr r11,$TBIS");
#endif
		addr += NBPG;
	}
}
  
int	waittime = -1;

boot(arghowto)
	int arghowto;
{
	register int howto;		/* r11 == how to boot */
	register int devtype;		/* r10 == major of root dev */

	howto = arghowto;
	if ((howto&RB_NOSYNC)==0 && waittime < 0 && bfreelist[0].b_forw) {
		waittime = 0;
		update();
		printf("syncing disks... ");
		while (++waittime <= 5)
			sleep((caddr_t)&lbolt, PZERO);
		printf("done\n");
	}
	splx(0x1f);			/* extreme priority */
	devtype = major(rootdev);
	if (howto&RB_HALT) {
#ifdef MVAX
		if( cpu == M_VAX ) {
			((struct qb_regs *)nexus)->qb_cpmbx = RB_HALTMD;
			for (;;)
 				asm ("halt");
		}
#endif MVAX
		printf("halting (in tight loop); hit\n\t^P\n\tHALT\n\n");
		mtpr(IPL, 0x1f);
		for (;;)
			;
	} else {
		if ((howto & RB_PANIC) != 0)
			doadump();
		tocons(TXDB_BOOT);
	}
#if defined(VAX750) || defined(VAX7ZZ) || defined(MVAX)
	if (cpu != VAX_780)
		{ asm("movl r11,r5"); }		/* boot flags go in r5 */
#endif
#ifdef MVAX
	if( cpu == M_VAX ) 
		((struct qb_regs *)nexus)->qb_cpmbx = RB_REBOOT;
#endif
	for (;;)
		asm("halt");
	/*NOTREACHED*/
}

tocons(c)
{

	while ((mfpr(TXCS)&TXCS_RDY) == 0)
		continue;
	mtpr(TXDB, c);
}

/*
 * Doadump comes here after turning off memory management and
 * getting on the dump stack, either when called above, or by
 * the auto-restart code.
 */
dumpsys()
{
	int dstat;
	rpb.rp_flag = 1;
	if ((minor(dumpdev)&07) != 1)
		return;
	printf("\ndumping to dev %x, offset %d\n", dumpdev, dumplo);
	printf("dump ");
	switch (dstat = (*bdevsw[major(dumpdev)].d_dump)(dumpdev)) {

	case ENXIO:
		printf("device bad\n");
		break;

	case EFAULT:
		printf("device not ready\n");
		break;

	case EINVAL:
		printf("area improper\n");
		break;

	case EIO:
		printf("i/o error\n");
		break;

	default:
		printf("unexpected error %d\n", dstat);
		break;

	case 0:
		printf("succeeded\n");
		break;
	}
}
