#ifndef lint
static	char sccsid[] = "@(#)machdep.c 1.1 86/02/03 Copyr 1986 Sun Micro";
#endif lint

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/lnode.h"
#include "../h/proc.h"
#include "../h/msgbuf.h"
#include "../h/buf.h"
#include "../h/inode.h"
#include "../h/file.h"
#include "../h/text.h"
#include "../h/callout.h"
#include "../h/cmap.h"
#include "../h/reboot.h"

#include "../machine/mbvar.h"
#include "../machine/psl.h"
#include "../machine/reg.h"
#include "../machine/clock.h"
#include "../machine/pte.h"
#include "../machine/scb.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/eeprom.h"
#include "../machine/interreg.h"
#include "../machine/memerr.h"
#include "../machine/eccreg.h"
#include "../machine/frame.h"

/*
 * Declare these as initialized data so we can patch them.
 */
int nbuf = 0;
int nswbuf = 0;
int bufpages = 0;
int physmem = 0;	/* memory size in pages, patch if you want less */
int kernprot = 1;	/* write protect kernel text */
int msgbufinit = 0;	/* message buffer has been initialized, ok to printf */
dev_t consdev = 0;

int (*exit_vector)() = (int (*)())0;	/* Where to go when halting UNIX */

#define TESTVAL	0xA55A	/* memory test value */

u_char	getsegmap(), pmegallocres();
long	getpgmap();

#ifdef SUN3_260
/*
 * Since there is no implied ordering of the memory cards, we store
 * a zero terminated list of pointers to eccreg's that are active so
 * that we only look at existent memory cards during softecc() handling.
 */
struct eccreg *ecc_alive[MAX_ECC+1];
#endif SUN3_260

/*
 * We make use of CMAPn (the pte address)
 * and CADDRn (the virtual address)
 * which are both temporaries defined in locore.s,
 * not preserved across context switches,
 * and not to be used in interrupt routines
 */

/*
 * Machine-dependent startup code
 */
startup()
{
	register int unixsize, dvmapage;
	register unsigned i;
	register int c;
	register struct pte *pte;
	register caddr_t v;
	u_int firstaddr;		/* next free physical page number */
	extern char start[], etext[], end[], CADDR1[], Syslimit[];
	u_char oc, u_pmeg;
	u_int mapaddr;
	caddr_t zmemall();
	void v_handler();
	int mon_mem;

	initscb();			/* set trap vectors */
	*INTERREG |= IR_ENA_INT;	/* make sure interrupts can occur */
	firstaddr = btoc((int)end - KERNELBASE) + UPAGES;

	/* 
	 * Initialize map of allocated page map groups.
 	 * Must be done before mapin of unallocated segments.
	 */
	pmeginit();			/* init list of pmeg data structures */
	ctxinit();			/* init context data structures */

	/*
	 * Reserve necessary pmegs and set segment mapping.
	 * It is assumed here that the pmegs for low
	 * memory have already been duplicated for the
	 * segments up in the kernel virtual address space.
	 */

	/*
	 * invalidate to start of high mapping
	 */
	for (i = 0; i < KERNELBASE >> SGSHIFT; i++)
		setsegmap(i, (u_char)SEGINV);

	/* reserve kernel pmegs */
	for (; i < ptos(NPAGSEG - 1 + btoc(end)); i++)
		pmegreserve(getsegmap(i));

	for (; i < (MONSTART >> SGSHIFT); i++)	/* invalidate to mon start */
		setsegmap(i, (u_char)SEGINV);

	for (; i < (MONEND >> SGSHIFT); i++) 	/* reserve monitor pmegs */
		if ((oc = getsegmap(i)) != (u_char)SEGINV)	
			pmegreserve(oc);

	for (; i < NSEGMAP - 1; i++)		/* invalid until last seg */
		setsegmap(i, (u_char)SEGINV);

	/*
	 * Last segment contains the u area itself, 
	 * the pmeg here is reserved for all contexts.
	 * We also reserve the invalid pmeg itself.
	 */
	u_pmeg = getsegmap(NSEGMAP - 1);
	pmegreserve(u_pmeg);
	pmegreserve((u_char)SEGINV);

	setcputype();			/* sets cpu and dvmasize variables */

	/*
	 * Make sure the memory error register is
	 * set up to generate interrupts on error.
	 */
#if defined(SUN3_160) || defined(SUN3_50)
	if (cpu == CPU_SUN3_160 || cpu == CPU_SUN3_50)
		MEMREG->mr_per = PER_INTENA | PER_CHECK;
#endif defined(SUN3_160) || defined(SUN3_50)

#ifdef SUN3_260
	if (cpu == CPU_SUN3_260) {
		register struct eccreg **ecc_nxt = ecc_alive;
		register struct eccreg *ecc;

		/*
		 * Go probe for all memory cards and perform initialization.
		 * The address of the cards found is stashed in ecc_alive[].
		 * We assume that the cards are already enabled and the
		 * base addresses have been set correctly by the monitor.
		 */
		for (ecc = ECCREG; ecc < &ECCREG[MAX_ECC]; ecc++) {
			if (peekc((char *)ecc) == -1)
				continue;
			ecc->eccena.ena_scrub = 1;
			ecc->eccena.ena_busena = 1;
			*ecc_nxt++ = ecc;
		}
		*ecc_nxt = (struct eccreg *)0;		/* terminate list */

		MEMREG->mr_eer = EER_INTENA | EER_CE_ENA;
	}
#endif SUN3_260

	/*
	 * Allocate pmegs for DVMA space
	 */
	for (i = ptos(btop(DVMA)); i < ptos(btop(DVMA) + dvmasize); i++) {
		u_char pm = pmegallocres();

		setsegmap(i, pm);
		for (v = (caddr_t)ctob(NPAGSEG * i);
			v < (caddr_t)ctob(NPAGSEG * (i+1));
		    v += NBPG)
			setpgmap(v, (long)0);
	}

	/*
	 * Now go through all the other contexts and set up the segment
	 * maps so that all segments are mapped the same.
	 * We have to use a PROM routine to do this since we don't want
	 * to switch to a new (unmapped) context to call setsegmap()!
	 */
	for (c = 0; c < NCONTEXT; c++) {
		if (c == KCONTEXT)
			continue;

		for (v = (caddr_t)0, i = 0;
		    v < (caddr_t)ctob(NPAGSEG * NSEGMAP); v += NBSG, i++)
			(*romp->v_setcxsegmap)(c, v, getsegmap(i));
	}

	/*
	 * Initialize kernel page table entries.
	 */
	pte = &Sysmap[0];

	/* invalid until start except scb page which is kernel writable */
	for (v = (caddr_t)KERNELBASE; v < (caddr_t)start; v += NBPG) {
		if (v == (caddr_t)&scb)
			*(int *)pte = PG_V | PG_KW | getpgmap(v) & PG_PFNUM;
		else
			*(int *)pte = 0;
		setpgmap(v, *(long *)pte++);
	}

	/* set up kernel text pages */
	for (; v < (caddr_t)etext; v += NBPG) {
		if (kernprot)		/* is kernel to be protected? */
			*(int *)pte = PG_V | PG_KR | getpgmap(v) & PG_PFNUM;
		else
			*(int *)pte = PG_V | PG_KW | getpgmap(v) & PG_PFNUM;
		setpgmap(v, *(long *)pte++);
	}

	/* set up kernel data/bss pages to be writeable */
	for (; v < (caddr_t)end; v += NBPG) {
		*(int *)pte = PG_V | PG_KW | getpgmap(v) & PG_PFNUM;
		setpgmap(v, *(long *)pte++);
	}

	/* invalid until end of this segment */
	i = ((u_int)end + SGOFSET) & ~SGOFSET;
	for (; v < (caddr_t)i; v += NBPG)
		setpgmap(v, (long)0);

	/*
	 * Remove user access to monitor-set-up maps.
	 */
	for (i = MONSTART>>SGSHIFT; i < MONEND>>SGSHIFT; i++) {
		if (getsegmap(i) == SEGINV)
			continue;
		for (v = (caddr_t)ctob(NPAGSEG * i); 
			v < (caddr_t)ctob(NPAGSEG * (i+1));
		    v += NBPG)
			setpgmap(v, (long)(((getpgmap(v) & ~PG_PROT) | PG_KW)));
	}

	/*
	 * Invalidate any other pages in last segment
	 * besides the u area, EEPROM_ADDR, CLKADDR,
	 * MEMREG, INTERREG and MONSHORTPAGE.  This sets
	 * up the kernel redzone below the u area.  We
	 * get interrupt redzone for free when the kernel
	 * is write protected as the interrupt stack is
	 * the first thing in the data area.  Since u
	 * and MONSHORTPAGE are defined as 32 bit virtual
	 * addresses (to get short references to work),
	 * we must mask to get only the 28 bits we really
	 * want to look at.
	 */
	for (v = (caddr_t)ctob(NPAGSEG * (NSEGMAP - 1));
	     v < (caddr_t)ctob(NPAGSEG * NSEGMAP); v += NBPG) {
		if (((u_int)v < ((u_int)&u & 0x0FFFFFFF) ||
		    (u_int)v >= (((u_int)&u & 0x0FFFFFFF) + UPAGES*NBPG)) &&
		    (u_int)v != ((u_int)MONSHORTPAGE & 0x0FFFFFFF) &&
		    (u_int)v != (u_int)EEPROM_ADDR &&
		    (u_int)v != (u_int)CLKADDR &&
		    (u_int)v != (u_int)MEMREG &&
		    (u_int)v != (u_int)INTERREG)
			setpgmap(v, (long)0);
	}

	/*
	 * v_memorysize is the amount of physical memory while
	 * v_memoryavail is the amount of usable memory in versions
	 * equal or greater to 1.  Mon_mem is the difference which
	 * is the number of pages hidden by the monitor.
	 */
	if (romp->v_romvec_version >= 1)
		mon_mem = btop(*romp->v_memorysize - *romp->v_memoryavail);
	else
		mon_mem = 0;
	/*
	 * If physmem is patched to be non-zero, use it instead of
	 * the monitor value unless physmem is larger than the total
	 * amount of memory on hand.
	 */
	if (physmem == 0 || physmem > btop(*romp->v_memorysize))
		physmem = btop(*romp->v_memorysize);
	/*
	 * Adjust physmem down for the pages stolen by the monitor.
	 */
	physmem -= mon_mem;
	maxmem = physmem;

	/*
	 * v_vector_cmd is the handler for new monitor vector
	 * command in versions equal or greater to 2.
	 * We install v_handler() there for Unix.
	 */
	if (romp->v_romvec_version >= 2)
		*romp->v_vector_cmd = v_handler;

#include "bwtwo.h"
#if NBWTWO > 0
	if (physmem > btop(OBFBADDR + FBSIZE))
	        fbobmemavail = 1;
	else
	        fbobmemavail = 0;
#else
	fbobmemavail = 0;
#endif

	/*
	 * Determine if anything lives in DVMA bus space.
	 * We're paranoid and go through both the 16 bit
	 * and 32 bit device types.
 	 */
	disable_dvma();
	for (dvmapage = 0; dvmapage < btoc(dvmasize); dvmapage++) {
		mapin(CMAP1, btop(CADDR1), (u_int)(dvmapage | PGT_VME_D16),
		    1, PG_V | PG_KW);
		if (poke((short *)CADDR1, TESTVAL) == 0)
			break;
		mapin(CMAP1, btop(CADDR1), (u_int)(dvmapage | PGT_VME_D32),
		    1, PG_V | PG_KW);
		if (poke((short *)CADDR1, TESTVAL) == 0)
			break;
	}
	enable_dvma();

	/*
	 * Initialize error message buffer (in low real memory before start).
	 * Printf's which occur prior to this will not be captured.
	 */
	mapin(msgbufmap, (u_int)btop(&msgbuf),
	    (u_int)btop((int)start - sizeof (struct msgbuf) - KERNELBASE),
	    (int)btoc(sizeof (struct msgbuf)), PG_V | PG_KW);
	msgbufinit = 1;

	/*
	 * Allocate IOPB memory space just below the message
	 * buffer and map it to the first pages of DVMA space.
	 */
	maxmem -= IOPBMEM;
	for (v = (caddr_t)DVMA, i = maxmem; i < maxmem + IOPBMEM;
	    v += NBPG, i++) {
		struct pte tmp;			/* scratch pte */

		mapin(&tmp, btop(v), i, 1, PG_V | PG_KW);
	}

	/*
	 * Good {morning,afternoon,evening,night}.
	 * When printing memory, use the total including
	 * those hidden by the monitor (mon_mem).
	 */
	printf(version);

	if (dvmapage < btoc(dvmasize)) {
		printf("CAN'T HAVE PERIPHERALS IN RANGE 0 - %dKB\n",
		    ctob(dvmasize) / 1024);
		panic("dvma collision");
	}

#ifndef lint
	if ((int)start - (int)ctob(btoc(sizeof (struct msgbuf))) <= (int)&scb)
		panic("msgbuf too large");

	if (sizeof (struct user) > UPAGES * NBPG)
		panic("user area too large");
#endif lint

	if ((int)Syslimit > (CSEG << SGSHIFT))
		panic("system map tables too large");

        /*
	 * Determine how many buffers to allocate.
	 * Use 10% of memory (not counting 512K for kernel), with min of 16.
	 * We allocate 1/4 as many swap buffer headers as file i/o buffers.
	 */
	if (bufpages == 0)
		bufpages = (physmem * NBPG - 512 * 1024) / 10 / CLBYTES;
	if (nbuf == 0) {
		nbuf = bufpages;
		if (nbuf < 16)
			nbuf = 16;
	}
	if (bufpages > nbuf * (BUFSIZE / CLBYTES))
		bufpages = nbuf * (BUFSIZE / CLBYTES);
	if (nswbuf == 0) {
		nswbuf = (nbuf / 4) &~ 1;	/* force even */
		if (nswbuf > 32)
			nswbuf = 32;		/* sanity */
	}
	printf("real mem = %d nbuf = %d nswbuf = %d\n",
		ctob(physmem + mon_mem), nbuf, nswbuf);

	/*
	 * Allocate space for system data structures.
	 * The first available real memory address is in "firstaddr".
	 * The first available kernel virtual address is in "v".
	 * As pages of kernel virtual memory are allocated, "v" is incremented.
	 * "mapaddr" is the real memory address where the tables start.
	 * It is used when remapping the tables later.
	 * In order to support the frame buffer which might appear in 
	 * the middle of contiguous memory we adjust the map address to 
	 * start after the end of the frame buffer.  Later we will adjust
	 * the core map to take this hole into account.  The reason for
	 * this is to keep all the kernel tables contiguous in virtual space.
	 */
	if (fbobmemavail)
		mapaddr = btoc(OBFBADDR + FBSIZE);
	else
		mapaddr = firstaddr;
	v = (caddr_t)(ctob(firstaddr) + KERNELBASE);
#define	valloc(name, type, num) \
	    (name) = (type *)(v); (v) = (caddr_t)((name)+(num))
#define	valloclim(name, type, num, lim) \
	    (name) = (type *)(v); (v) = (caddr_t)((lim) = ((name)+(num)))
	valloc(swbuf, struct buf, nswbuf);
	valloclim(inode, struct inode, ninode, inodeNINODE);
	valloclim(file, struct file, nfile, fileNFILE);
	valloclim(proc, struct proc, nproc, procNPROC);
	valloclim(text, struct text, ntext, textNTEXT);
	valloclim(lnodes, struct kern_lnode, maxusers, lnodesMAXUSERS);
	valloc(callout, struct callout, ncallout);
	valloc(swapmap, struct map, nswapmap = nproc * 2);
	valloc(argmap, struct map, ARGMAPSIZE);
	valloc(kernelmap, struct map, nproc);
	valloc(iopbmap, struct map, IOPBMAPSIZE);
	valloc(mb_hd.mh_map, struct map, DVMAMAPSIZE);

	/*
	 * Now allocate space for core map
	 * Allow space for all of physical memory minus the amount 
	 * dedicated to the system. The amount of physical memory
	 * dedicated to the system is the total virtual memory of
	 * the system minus the space in the buffers which is not
	 * allocated real memory.
	 */
	ncmap = physmem - firstaddr;
	valloclim(cmap, struct cmap, ncmap, ecmap);
	unixsize = btoc((int)(ecmap+1) - KERNELBASE);

	if ((int)unixsize > SYSPTSIZE)
		panic("sys pt too small");

	/*
	 * Clear allocated space, and make r/w entries
	 * for the space in the kernel map.
	 */
	if (unixsize >= physmem - 8*UPAGES)
		panic("no memory");

	pte = &Sysmap[firstaddr];
	for (i = firstaddr + btop(KERNELBASE); i < btoc(v); i++) {
		mapin(pte++, i, mapaddr,  1, PG_V | PG_KW);
		clearseg(mapaddr++);
	}

	/*
	 * Initialize callouts.
	 */
	callfree = callout;
	for (i = 1; i < ncallout; i++)
		callout[i-1].c_next = &callout[i];

	/*
	 * Initialize memory allocator and swap
	 * and user page table maps.
	 */
	if (fbobmemavail) {
	        meminit((int)firstaddr, maxmem);
		memialloc((int)firstaddr, (int)btop(OBFBADDR));
		memialloc((int)(1 + mapaddr), maxmem);
	} else {
	        meminit((int)mapaddr, maxmem);
		memialloc((int)mapaddr, maxmem);
	}
	maxmem = freemem;
	printf("avail mem = %d\n", ctob(maxmem));
	rminit(kernelmap, (long)(USRPTSIZE - CLSIZE), (long)CLSIZE,
	    "usrpt", nproc);
	rminit(iopbmap, (long)ctob(IOPBMEM), (long)DVMA, 
	    "IOPB space", IOPBMAPSIZE);
	rminit(mb_hd.mh_map, (long)(dvmasize - IOPBMEM), (long)IOPBMEM,
	    "DVMA map space", DVMAMAPSIZE);

	/*
	 * Configure the system.
	 */
	bootflags();		/* get the boot options */
	configure();		/* set up devices */
	if (fbobmemavail) {
		/*
		 * Onboard frame buffer memory still
		 * available, put back onto the free list.
		 */
		memialloc((int)btop(OBFBADDR), (int)btop(OBFBADDR + FBSIZE));
		fbobmemavail = 0;
	}
	bufmemall();
	uinit();		/* initialize the u area */
	(void) spl0();		/* drop priority */
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
 * This system call sets the time of year clock without touching
 * the software clock.  It returns the previous clock value.  If
 * the argument is zero or the caller is not the super-user, it
 * does not change the clock.
 * Since the clocks are the same on the SUN, it just calls stime.
 */
settod()
{
	register struct a {
		long unsigned tod;
	} *uap;

	u.u_r.r_time = time;
	if (uap->tod != 0 && suser())
		stime();
}

/*
 * Allocate physical memory for system buffers
 * In Ethernet memory if the right board exists &
 * the root device is ND & there are no block I/O DMA devices
 */
bufmemall()
{
	struct pte *pte;
	long a, va;
	int npages;
	int i, j, base, residual;

	a = rmalloc(kernelmap, (long)(nbuf*BUFSIZE/NBPG));
	if (a == 0)
		panic("no vmem for buffers");
	buffers = (caddr_t)kmxtob(a);
	pte = &Usrptmap[a];
	base = bufpages / nbuf;
	residual = bufpages % nbuf;
	for (i = 0; i < nbuf; i++) {
		if (i < residual)
			npages = base+1;
		else
			npages = base;
		/* XXX - this loop only works if CLSIZE == 1 */
		for (j = 0; j < npages; j += CLSIZE) {
			if (memall(pte+j, CLSIZE, &proc[0], CSYS) == 0)
				panic("no mem for buffers");
			*(int *)(pte+j) |= PG_V|PG_KW;
			va = (int)kmxtob(a+j);
			vmaccess(pte+j, (caddr_t)va, 1);
			bzero((caddr_t)va, CLBYTES);
		}
		pte += BUFSIZE/CLBYTES;
		a += BUFSIZE/CLBYTES;
	}
	/*
	 * Double map and then unmap the last page of the last
 	 * buffer to insure the presence of a pmeg.
	 * AARRRGGGHHH. Kludge away.
	 */
	if (base < BUFSIZE/CLBYTES) {
		pte -= BUFSIZE/CLBYTES;
		va = (int)kmxtob(a-1);
		pte[BUFSIZE/CLBYTES - 1] = pte[0];
		vmaccess(&pte[BUFSIZE/CLBYTES - 1], (caddr_t)va, 1);
		/* now unmap without disturbing the pmeg */
		*(int *)&pte[BUFSIZE/CLBYTES - 1] = 0;
		setpgmap((caddr_t)va, (long)0);
	}
	buf = (struct buf *)zmemall(memall, nbuf * sizeof(struct buf));
	if (buf == 0)
		panic("no mem for buf headers");
}

struct	bootf {
	char	let;
	short	bit;
} bootf[] = {
	'a',	RB_ASKNAME,
	's',	RB_SINGLE,
	'i',	RB_INITNAME,
	'h',	RB_HALT,
	0,	0,
};
char *initname = "/etc/init";

/*
 * Parse the boot line to determine boot flags .
 */
bootflags()
{
	register struct bootparam *bp = (*romp->v_bootparam);
	register char *cp;
	register int i;

	cp = bp->bp_argv[1];
	if (cp && *cp++ == '-')
		do {
			for (i = 0; bootf[i].let; i++) {
				if (*cp == bootf[i].let) {
					boothowto |= bootf[i].bit;
					break;
				}
			}
			cp++;
		} while (bootf[i].let && *cp);
	if (boothowto & RB_INITNAME)
		initname = bp->bp_argv[2];
	if (boothowto & RB_HALT)
		halt("bootflags");
}

/*
 * Start the initial user process.
 * The program [initname] is invoked with one argument
 * containing the boot flags.
 */
icode()
{
	struct execa {
		char	*fname;
		char	**argp;
		char	**envp;
	} *ap;
	char *ucp, **uap, *arg0, *arg1;
	int i;

	u.u_error = 0;				/* paranoid */
	/* Make a user stack (1 page) */
	expand(1, 1);
	(void) swpexpand(0, 1, &u.u_dmap, &u.u_smap);

	/* Move out the boot flag argument */
	ucp = (char *)USRSTACK;
	(void) subyte(--ucp, 0);		/* trailing zero */
	for (i = 0; bootf[i].let; i++) {
		if (boothowto & bootf[i].bit)
			(void) subyte(--ucp, bootf[i].let);
	}
	(void) subyte(--ucp, '-');		/* leading hyphen */
	arg1 = ucp;

	/* Move out the file name (also arg 0) */
	for (i = 0; initname[i]; i++)
		;				/* size the name */
	for (; i >= 0; i--)
		(void) subyte(--ucp, initname[i]);
	arg0 = ucp;

	/* Move out the arg pointers */
	uap = (char **) ((int)ucp & ~(NBPW-1));
	(void) suword((caddr_t)--uap, 0);	/* terminator */
	(void) suword((caddr_t)--uap, (int)arg1);
	(void) suword((caddr_t)--uap, (int)arg0);

	/* Point at the arguments */
	u.u_ap = u.u_arg;
	ap = (struct execa *)u.u_ap;
	ap->fname = arg0;
	ap->argp = uap;
	ap->envp = 0;
	u.u_dirp = (caddr_t)u.u_arg[0];
	
	/* Now let exec do the hard work */
	exece();
	if (u.u_error) {
		printf("Can't invoke %s, error %d\n", initname, u.u_error);
		panic("icode");
	}
}

/*
 * Set up page tables for process 0 U pages.
 * This is closely related to way the code
 * in locore.s sets things up.
 */
uinit()
{
	register struct pte *pte;
	u_int page;
	register int i;
	extern char end[];

	/*
	 * main() will initialize proc[0].p_p0br to u.u_pcb.pcb_p0br
	 * and proc[0].p_szpt to 1.  All we have to do is set up
	 * the pcb_p{0,1}{b,l}r registers in the pcb for now.
	 */

	/* initialize base and length of P0 region */
	u.u_pcb.pcb_p0br = usrpt;
	u.u_pcb.pcb_p0lr = 0;		/* no user text/data (P0) for proc 0 */

	/*
	 * initialize base and length of P1 region,
	 * where the length here is for invalid pages
	 */
	u.u_pcb.pcb_p1br = initp1br(usrpt + 1 * NPTEPG);
	u.u_pcb.pcb_p1lr = P1PAGES;	/* no user stack (P1) for proc 0 */

	/*
	 * Doublely map the page containing the scb to contain the
	 * ptes whose virtual address is usrpt.  Got that?
 	 */
	page = (u_int)(getpgmap((caddr_t)&scb) & PG_PFNUM);
	mapin(&Usrptmap[0], btop(usrpt), page, 1, PG_V | PG_KW); 

	/*
	 * Now build the software page maps to map virtual U to physical U.
	 * These pages have already been set up using the real pages beyond
	 * end by locore.s.
	 */
	pte = usrpt + 1 * NPTEPG - UPAGES;
	page = btop((int)end + (NBPG - 1) - KERNELBASE);
	for (i = 0; i < UPAGES; i++)
		*(int *)pte++ = PG_V | PG_KW | page++;
}

struct	sigcontext {
	int	sc_sp;			/* sp to restore */
	int	sc_pc;			/* pc to retore */
	int	sc_ps;			/* psl to restore */
};
/*
 * Send an interrupt to process.
 *
 * When using new signals user code must do a
 * sys #139 to return from the signal, which
 * calls sigcleanup below, which resets the
 * signal mask and the notion of onsigstack,
 * and returns from the signal handler.
 */
sendsig(p, sig)
	int (*p)(), sig;
{
	register int usp, *regs, scp;
	struct nframe {
		int	sig;
		int	code;
		int	scp;
	} frame;
	struct sigcontext sc;

#define	mask(s)	(1<<((s)-1))
	regs = u.u_ar0;

	usp = regs[SP];
	usp -= sizeof (struct sigcontext);
	scp = usp;
	usp -= sizeof (frame);
	if (usp <= USRSTACK - ctob(u.u_ssize))
		(void) grow((unsigned)usp);
	if (useracc((caddr_t)usp, sizeof(frame) + sizeof(sc), B_WRITE) == 0) {
		/*
		 * Process has trashed its stack; give it an illegal
		 * instruction to halt it in its tracks.
		 */
		u.u_signal[SIGILL] = SIG_DFL;
		u.u_procp->p_siga0 &= ~(1<<(SIGILL-1));
		u.u_procp->p_siga1 &= ~(1<<(SIGILL-1));
		psignal(u.u_procp, SIGILL);
	}
	/*
	 * push sigcontext structure.
	 */
	sc.sc_sp = regs[SP];
	sc.sc_pc = regs[PC];
	sc.sc_ps = regs[PS];
	/*
	 * If trace mode was on for the user process
	 * when we came in here, it may have been because
	 * of an ast-induced trace on a trap instruction,
	 * in which case we do not want to restore the
	 * trace bit in the status register later on
	 * in sigcleanup().  If we were to restore it
	 * and another ast trap had been posted, we would
	 * end up marking the trace trap as a user-requested
	 * real trace trap and send a bogus "Trace/BPT" signal.
	 */
	if ((sc.sc_ps & PSL_T) && (u.u_pcb.pcb_p0lr & TRACE_AST))
		sc.sc_ps &= ~PSL_T;
	(void) copyout((caddr_t)&sc, (caddr_t)scp, sizeof (sc));
	/*
	 * push call frame.
	 */
	frame.sig = sig;
	if (sig == SIGILL || sig == SIGFPE || sig == SIGEMT) {
		frame.code = u.u_code;
		u.u_code = 0;
	} else
		frame.code = 0;
	frame.scp = scp;
	(void) copyout((caddr_t)&frame, (caddr_t)usp, sizeof (frame));
	regs[SP] = usp;
	regs[PC] = (int)p;
}

/*
 * Routine to cleanup state after a signal
 * has been taken.  Reset signal mask and
 * notion of on signal stack from context
 * left there by sendsig (above).  Pop these
 * values and perform rti.
 */
sigcleanup()
{
	struct sigcontext *scp, sc;

	scp = (struct sigcontext *)fuword((caddr_t)u.u_ar0[SP] + sizeof(int));
	if ((int)scp == -1)
		return;
	if (copyin((caddr_t)scp, (caddr_t)&sc, sizeof (sc)))
		return;
	u.u_ar0[SP] = sc.sc_sp;
	u.u_ar0[PC] = sc.sc_pc;
	u.u_ar0[PS] = sc.sc_ps;
	u.u_ar0[PS] &= ~PSL_USERCLR;
	u.u_ar0[PS] |= PSL_USERSET;
	u.u_eosys = REALLYRETURN;
}
#undef mask

int	waittime = -1;

boot(arghowto)
	int arghowto;
{
	register int howto;
	static int prevflag = 0;
	register struct buf *bp;
	int iter, nbusy;
	int s;

	consdev = 0;
	startnmi();
	howto = arghowto;
	if ((howto&RB_NOSYNC)==0 && waittime < 0 && bfreelist[0].b_forw) {
		waittime = 0;
		update();
		printf("syncing disks... ");
		while (++waittime <= 5)
			sleep((caddr_t)&lbolt, PZERO);
		printf("done\n");
	}
	s = spl7();				/* extreme priority */
	if (howto & RB_HALT) {
		halt((char *)NULL);
		/* MAYBE REACHED */
	} else {
		if ((howto & RB_PANIC) && prevflag == 0) {
			prevflag = 1;
			dumpsys();

		}
		printf("Rebooting Unix...\n");
		(*romp->v_boot_me)(howto & RB_SINGLE ? "-s" : "");
		/*NOTREACHED*/
	}
	(void) splx(s);
}

/*
 * Initialize UNIX's vector table:
 * Vectors are copied from protoscb unless
 * they are zero; zero means preserve whatever the
 * monitor put there.  If the protoscb is zero,
 * then the original contents are copied into
 * the scb we are setting up.
 */
initscb()
{
	register int *s, *p, *f;
	register int n;
	struct scb *orig, *getvbr();

	orig = getvbr();
	exit_vector = orig->scb_trap[14];
	s = (int *)&scb;
	p = (int *)&protoscb;
	f = (int *)orig;
	for (n = sizeof (struct scb)/sizeof (int); n--; s++, p++, f++) {
		if (*p) 
			*s = *p;
		else
			*s = *f;
	}
	setvbr(&scb);
}

/*
 * Clear a segment (page (click)).
 */
clearseg(pgno)
	u_int pgno;
{
	extern char CADDR1[];

	mapin(CMAP1, btop(CADDR1), pgno, 1, PG_V | PG_KW); 
	bzero(CADDR1, NBPG);
}

/*
 * Copy a segment (page) from a user virtual address
 * to a physical page number.
 */
copyseg(vaddr, pgno)
	caddr_t vaddr;
	int pgno;
{
	register struct pte *pte;
	register int lock;
	extern char CADDR1[];

	/*
	 * Make sure the user's page is valid and locked.
	 */
	pte = vtopte(u.u_procp, btop(vaddr));
	if (lock = !pte->pg_v) {
		pagein((u_int)vaddr, &u, 1);		/* return it locked */
		pte = vtopte(u.u_procp, btop(vaddr));	/* pte may move */
	}
	/*
	 * Map the destination page into kernel address space.
	 */
	mapin(CMAP1, btop(CADDR1), (u_int)pgno, 1, PG_V | PG_KW);
	(void) copyin(vaddr, CADDR1, CLBYTES);
	if (lock)
		munlock(pte->pg_pfnum);
}

/*
 * Handle "physical" block transfers.
 */
physstrat(bp, strat, pri)
	register struct buf *bp;
	int (*strat)();
	int pri;
{
	register int npte, n;
	register long a;
	unsigned v;
	register struct pte *pte, *kpte;
	struct proc *rp;
	int va, s, o;

	v = btop(bp->b_un.b_addr);
	o = (int)bp->b_un.b_addr & PGOFSET;
	npte = btoc(bp->b_bcount + o) + 1;
	while ((a = rmalloc(kernelmap, (long)npte)) == NULL) {
		kmapwnt++;
		sleep((caddr_t)kernelmap, PSWP+4);
	}
	kpte = &Usrptmap[a];
	rp = bp->b_flags&B_DIRTY ? &proc[2] : bp->b_proc;
	if ((bp->b_flags & B_PHYS) == 0)
		pte = &Sysmap[btop((int)bp->b_un.b_addr - KERNELBASE)];
	else if (bp->b_flags & B_UAREA)
		pte = &rp->p_addr[v];
	else if (bp->b_flags & B_PAGET)
		pte = &Usrptmap[btokmx((struct pte *)bp->b_un.b_addr)];
	else
		pte = vtopte(rp, v);
	for (n = npte; --n != 0; kpte++, pte++)
		*(int *)kpte = PG_V | PG_KW | (*(int *)pte & PG_PFNUM);
	*(int *)kpte = 0;
	va = (int)kmxtob(a);
	vmaccess(&Usrptmap[a], (caddr_t)va, npte);
	bp->b_saddr = bp->b_un.b_addr;
	bp->b_un.b_addr = (caddr_t)(va | o);
	bp->b_kmx = a;
	bp->b_npte = npte;
	(*strat)(bp);
	if (bp->b_flags & B_DIRTY)
		return;
	s = spl6();
	while ((bp->b_flags & B_DONE) == 0)
		sleep((caddr_t)bp, pri);
	(void) splx(s);
	bp->b_un.b_addr = bp->b_saddr;
	bp->b_kmx = 0;
	bp->b_npte = 0;
	mapout(&Usrptmap[a], npte);
	rmfree(kernelmap, (long)npte, a);
}

/*
 * Halt the machine and return to the monitor
 */
halt(s)
	char *s;
{
	extern struct scb *getvbr();

	if (s)
		(*romp->v_printf)("(%s) ", s);
	(*romp->v_printf)("Unix Halted\n\n");
	startnmi();
	if (exit_vector)
		getvbr()->scb_trap[14] = exit_vector;
	asm("trap #14");
	if (exit_vector)
		getvbr()->scb_trap[14] = protoscb.scb_trap[14];
	stopnmi();
}

/*
 * Print out a traceback for the caller - can be called anywhere
 * within the kernel or from the monitor by typing "g4" (for sun-2
 * compatibility) or "w trace".  This causes the monitor to call
 * the v_handler() routine which will call tracedump() for these cases.
 */
/*VARARGS0*/
tracedump(x1)
	caddr_t x1;
{
	struct frame *fp = (struct frame *)(&x1 - 2);
	u_int tospage = btoc(fp);

	(*romp->v_printf)("Begin traceback...fp = %x\n", fp);
	while (btoc(fp) == tospage) {
		if (fp == fp->fr_savfp) {
			(*romp->v_printf)("FP loop at %x", fp);
			break;
		}
		(*romp->v_printf)("Called from %x, fp=%x, args=%x %x %x %x\n",
		    fp->fr_savpc, fp->fr_savfp,
		    fp->fr_arg[0], fp->fr_arg[1], fp->fr_arg[2], fp->fr_arg[3]);
		fp = fp->fr_savfp;
	}
	(*romp->v_printf)("End traceback...\n");
}

/*
 * Buscheck is called by mbsetup to check to see it the requested
 * setup is a valid busmem type (i.e. VMEbus).  Returns 1 if ok
 * busmem type, returns 0 if not busmem type.  This routine
 * make checks and panic's if an illegal busmem type request is detected.
 */
buscheck(pte, npf)
	register struct pte *pte;
	register int npf;
{
	register int i, pf;
	register int pt = *(int *)pte & PGT_MASK;

	if (pt == PGT_VME_D16 || pt == PGT_VME_D32) {
		pf = pte->pg_pfnum;
		if (pf < btoc(DVMASIZE))
			panic("buscheck: busmem in DVMA range");
		for (i = 0; i < npf; i++, pte++, pf++) {
			if ((*(int *)pte & PGT_MASK) != pt ||
			    pte->pg_pfnum != pf)
				panic("buscheck: request not contiguous");
		}
		return (1);
	}
	return (0);
}

/* 
 * Compute the address of an I/O device within standard address
 * ranges and return the result.  This is used by DKIOCINFO
 * ioctl to get the best guess possible for the actual address
 * set on the card.
 */
getdevaddr(addr)
	caddr_t addr;
{
	int off = (int)addr & PGOFSET;
	int pte = getkpgmap(addr);
	int physaddr = ((pte & PG_PFNUM) & ~PGT_MASK) * NBPG;

	switch (pte & PGT_MASK) {
	case PGT_VME_D16:
	case PGT_VME_D32:
		if (physaddr > VME16_BASE) {
			/* 16 bit VMEbus address */
			physaddr -= VME16_BASE;
		} else if (physaddr > VME24_BASE) {
			/* 24 bit VMEbus address */
			physaddr -= VME24_BASE;
		}
		/*
		 * else 32 bit VMEbus address,
		 * physaddr doesn't require adjustments
		 */
		break;

	case PGT_OBMEM:
	case PGT_OBIO:
		/* physaddr doesn't require adjustments */
		break;
	}

	return (physaddr + off);
}

static int (*mon_nmi)();		/* monitor's level 7 nmi routine */
extern int level7();			/* Unix's level 7 nmi routine */

stopnmi()
{
	struct scb *vbr, *getvbr();

	vbr = getvbr();
	if (vbr->scb_autovec[7 - 1] != level7) {
#ifndef GPROF
		set_clk_mode(0, IR_ENA_CLK7);	/* disable level 7 clk intr */
#endif !GPROF
		mon_nmi = vbr->scb_autovec[7 - 1];	/* save mon vec */
		vbr->scb_autovec[7 - 1] = level7;	/* install Unix vec */
	}
}

startnmi()
{
	struct scb *getvbr();

	if (mon_nmi) {
		getvbr()->scb_autovec[7 - 1] = mon_nmi;	/* install mon vec */
#ifndef GPROF
		set_clk_mode(IR_ENA_CLK7, 0);	/* enable level 7 clk intr */
#endif !GPROF
	}
}

/*
 * Handler for monitor vector cmd -
 * For now we just implement the old "g0" and "g4"
 * commands and a printf hack.
 */
void
v_handler(addr, str)
	int addr;
	char *str;
{

	switch (*str) {
	case '\0':
		/*
		 * No (non-hex) letter was specified on
		 * command line, use only the number given
		 */
		switch (addr) {
		case 0:		/* old g0 */
		case 0xd:	/* 'd'ump short hand */
			panic("zero");
			/*NOTREACHED*/
		
		case 4:		/* old g4 */
			tracedump();
			break;

		default:
			goto err;
		}
		break;

	case 'p':		/* 'p'rint string command */
	case 'P':
		(*romp->v_printf)("%s\n", (char *)addr);
		break;

	case '%':		/* p'%'int anything a la printf */
		(*romp->v_printf)(str, addr);
		(*romp->v_printf)("\n");
		break;

	case 't':		/* 't'race kernel stack */
	case 'T':
		tracedump();
		break;

	case 'u':		/* d'u'mp hack ('d' look like hex) */
	case 'U':
		if (addr == 0xd) {
			panic("zero");
		} else
			goto err;
		break;

	default:
	err:
		(*romp->v_printf)("Don't understand 0x%x '%s'\n", addr, str);
	}
}

/*
 * Handle parity/ECC memory errors.  XXX - use something like
 * vax to only look for soft ecc errors periodically?
 */
memerr()
{
	u_char per, eer;
	char *mess = 0;
	int c;
	long pme;

	eer = per = MEMREG->mr_er;
#ifdef SUN3_260
	if (cpu == CPU_SUN3_260 && (eer & EER_ERR) == EER_CE) {
		softecc();
		MEMREG->mr_dvma = 1;	/* clear latching */
		return;
	} 
#endif SUN3_260

	/*
	 * Since we are going down in flames, disable further
	 * memory error interrupts to prevent confusion.
	 */
	MEMREG->mr_er &= ~ER_INTENA;

#if defined(SUN3_160) || defined(SUN3_50)
	if ((cpu == CPU_SUN3_160 || cpu == CPU_SUN3_50) &&
	    (per & PER_ERR) != 0) {
		printf("Parity Error Register %b\n", per, PARERR_BITS);
		mess = "parity error";
	}
#endif defined(SUN3_160) || defined(SUN3_50)

#ifdef SUN3_260
	if ((cpu == CPU_SUN3_260) && (eer & EER_ERR) != 0) {
		printf("ECC Error Register %b\n", eer, ECCERR_BITS);
		mess = "uncorrectable ECC error";
	}
#endif SUN3_260

	if (!mess) {
		printf("Memory Error Register %b %b\n",
		    per, PARERR_BITS, eer, ECCERR_BITS);
		mess = "unknown memory error";
	}

	printf("DVMA = %x, context = %x, virtual address = %x\n",
		MEMREG->mr_dvma, MEMREG->mr_ctx, MEMREG->mr_vaddr);

	c = getcontext();
	setcontext((int)MEMREG->mr_ctx);
	pme = getpgmap((caddr_t)MEMREG->mr_vaddr);
	printf("pme = %x, physical address = %x\n", pme,
	    ptob(((struct pte *)&pme)->pg_pfnum) + (MEMREG->mr_vaddr&PGOFSET));
	setcontext(c);

	/*
	 * Clear the latching by writing to the top
	 * nibble of the memory address register
	 */
	MEMREG->mr_dvma = 1;

	panic(mess);
	/*NOTREACHED*/
}

#ifdef SUN3_260
int prtsoftecc = 1;

/*
 * Probe memory cards to find which one(s) had ecc error(s).
 * If prtsoftecc is non-zero, log messages regarding the failing
 * syndrome.  Then clear the latching on the memory card.
 */
softecc()
{
	register struct eccreg **ecc_nxt, *ecc;

	for (ecc_nxt = ecc_alive; *ecc_nxt != (struct eccreg *)0; ecc_nxt++) {
		ecc = *ecc_nxt;
		if (ecc->syndrome.sy_ce) {
			if (prtsoftecc) {
				printf("mem%d: soft ecc addr %x+%x=%x syn %b\n",
				    ecc - ECCREG,
				    (ecc->eccena.ena_addr << 22),
				    (ecc->syndrome.sy_addr << 3),
				    (ecc->eccena.ena_addr << 22) +
				      (ecc->syndrome.sy_addr << 3),
				    ecc->syndrome.sy_synd, SYNDERR_BITS);
			}
			ecc->syndrome.sy_ce = 1;	/* clear latching */
		}
	}
}
#endif SUN3_260
