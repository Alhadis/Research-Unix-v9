#ifndef lint
static	char sccsid[] = "@(#)vm_machdep.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Machine dependent virtual memory support.
 * Context and segment and page map support for the Sun-3.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/text.h"
#include "../h/mount.h"

#include "../machine/pte.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/reg.h"
#include "../machine/buserr.h"
#include "../machine/scb.h"
#include "../machine/mbvar.h"

u_char	getsegmap();
long	getpgmap();

struct	context context[NCONTEXT];	/* contexts */
int	ctxtime = 0;			/* pseudo-time for ctx lru */

struct	pmeg pmeg[NPMEG];		/* all the pmegs in the world */
struct	pmeg pmeghead;			/* pmeg free list */

int	kernpmeg = 0;			/* how many pmegs the kernel has */

/*
 * Initialize pmeg allocation list.
 */
pmeginit()
{
	register int i;

	pmeghead.pm_forw = pmeghead.pm_back = &pmeghead;
	for (i = 0; i < NPMEG; i++)		/* add all entries to queue */
		insque(&pmeg[i], &pmeghead);
}

/*
 * Take care of common pmeg release code for pmegalloc() and pmegallocres(),
 * called only when pmp->pm_procp is non-zero.
 */
pmegrelease(pmp)
	register struct pmeg *pmp;
{
	register struct context *cp;
	int ctx;

	pmegunload(pmp);
	cp = pmp->pm_procp->p_ctx;
	cp->ctx_pmeg[pmp->pm_seg] = 0;
	ctx = getcontext();
	setcontext((int)cp->ctx_context);
	setsegmap((u_int)pmp->pm_seg, (u_char)SEGINV);
	setcontext(ctx);
}

/*
 * Allocate a page map entry group.
 */
u_char
pmegalloc(p)
	register struct proc *p;	/* process to allocate it for */
{
	register struct pmeg *pmp;
	extern int potime;

	pmp = pmeghead.pm_forw;		/* get pmeg off head of chain */
	if (pmp->pm_procp)
		pmegrelease(pmp);
	remque(pmp);
	pmp->pm_procp = p;		/* set process pointer */
	pmp->pm_count = 0;		/* no valid pages yet */
	pmp->pm_seg = -1;		/* no segments yet */
	pmp->pm_time = potime - 1;	/* reset time */
	if (p)
		insque(pmp, pmeghead.pm_back);	/* put back on queue */
	else
		pmp->pm_forw = pmp->pm_back = 0;/* wanted by kernel */

	return ((u_char)(pmp - pmeg));	/* return index to pmeg in pmeg list */
}

/*
 * Free a pmeg.
 */
pmegfree(pmx)
	u_char pmx;		/* index of pmeg in pmeg list */
{
	register struct pmeg *pmp = &pmeg[pmx];

	if (pmp->pm_procp)	/* is there a process with this one? */
		remque(pmp);	/* take if off the queue its on */
	pmp->pm_procp = 0;	/* reset process number */
	insque(pmp, &pmeghead);	/* put back on free list */
}

/*
 * Reserve named pmeg for use by kernel and monitor.
 */
pmegreserve(n)
	u_char n;
{
	register struct pmeg *pmp = &pmeg[n];

	remque(pmp);
	pmp->pm_forw = pmp->pm_back = 0;
	pmp->pm_count = NPAGSEG;
	pmp->pm_procp = (struct proc *)0;
	pmp->pm_seg = 0;
	kernpmeg++;
}

/*
 * Allocate and reserve kernel pmeg
 */
u_char
pmegallocres()
{
	struct pmeg *pmp;
	u_char pm;

	pmp = pmeghead.pm_forw;
	if (pmp->pm_procp)
		pmegrelease(pmp);
	pm = (u_char)(pmp - pmeg);
	pmegreserve(pm);
	return (pm);
}

/*
 * Load all hardware page map entries for the specified
 * pmeg from the software page table entries.
 */
pmegload(seg, need)
	int seg;		/* segment we are loading */
	int need;		/* need a segment */
{
	register struct pte *pte;
	register struct pmeg *pmp;
	register int v, num, k, new = 0;
	register int i;
	register struct proc *p = u.u_procp;
	int pm, last, tse, dse, sss;
	struct context *cp = p->p_ctx;		/* process context number */
	u_char *pmxp = &cp->ctx_pmeg[seg];	/* which segments pmeg */
	int s = splimp();

	if (getcontext() == KCONTEXT) {
		printf("NO CONTEXT IN PMEGLOAD\n");
		(void) splx(s);
		return;
	}
	if (*pmxp == 0) {			/* is there a pmeg already */
		if (!need) {
			setsegmap((u_int)seg, (u_char)SEGINV);
			(void) splx(s);
			return;
		}
		*pmxp = pmegalloc(p);		/* get one */
		pmeg[*pmxp].pm_seg = seg;	/* assign it in context */
		new = 1;
	}
	pmp = &pmeg[*pmxp];			/* get pointer to pmeg */
	if (pmp->pm_procp != p)			/* better be same process */
		panic("dup alloc pmeg");
	if (pmp->pm_seg != seg)			/* better be same segment */
		panic("pmeg changed seg");
	pm = pmp - pmeg;	/* make pmeg index based on pmeg addr */
	v = seg * NPAGSEG;	/* make index based on segment number */
	/*
	 * Decide which of the text|data|stack
	 * segments the virtual segment seg is in.
	 *
	 * Lots of assumptions about the layout
	 * of virtual memory here.  Should be
	 * more parameterized.
	 */
	last = ptos(btop(USRSTACK));	/* find bottom of user stack seg */
	/* find end of text segment */
	tse = p->p_tsize ? ptos(tptov(p, 0) + p->p_tsize - 1) : 0;
	/* find end of data segment */
	dse = ptos(dptov(p, 0) + p->p_dsize - 1);
	/* compute start of stack segment */
	sss = last - ptos(p->p_ssize + NPAGSEG - 1);
	
	setsegmap((u_int)seg, (u_char)pm);	/* set the seg map */
	if (seg <= tse) {
	        if (seg == 0) {
		        setpgmap((caddr_t)0, (long)0);/* first page invalid */
			v = LOWPAGES;		      /* set addr in map */
			num = MIN((p->p_tsize ? p->p_tsize : p->p_dsize),
			    NPAGSEG - LOWPAGES);
			i = 0;
		} else {
		        i = v - tptov(p, 0);	/* compute index in text */
			num = MIN(p->p_tsize - i, NPAGSEG);
		}
		pte = tptopte(p, i);		/* get pointer ptes */
		pmp->pm_pte = pte;
		pmp->pm_count = num;
		for (k = num; k--; v++, pte++)
			loadpgmap((u_int)v, pte, new);
		for (k = NPAGSEG - num - ((seg == 0)? LOWPAGES : 0); k--; v++)
			setpgmap((caddr_t)ctob(v), (long)0);
		if (seg > cp->ctx_tdmax)
			cp->ctx_tdmax = seg;
	} else if (seg > tse && seg <= dse) {
		i = v - dptov(p, 0);		/* compute index in data */
		pte = dptopte(p, i);		/* get pointer ptes */
		num = MIN(p->p_dsize - i, NPAGSEG);
		pmp->pm_pte = pte;
		pmp->pm_count = num;
		for (k = num; k--; v++, pte++)
			loadpgmap((u_int)v, pte, new);
		for (k = NPAGSEG - num; k--; v++)
			setpgmap((caddr_t)ctob(v), (long)0);
		if (seg > cp->ctx_tdmax)
			cp->ctx_tdmax = seg;
	} else if (seg >= sss && seg < last) {
		i = btop(USRSTACK)-NPAGSEG - v;	/* compute index in stack */
		pte = sptopte(p, i);		/* get pointer ptes */
		num = MIN(p->p_ssize - i, NPAGSEG);
		pte -= num - 1;
		pmp->pm_pte = pte;
		pmp->pm_count = -num;
		for (k = NPAGSEG - num; k--; v++)
			setpgmap((caddr_t)ctob(v), (long)0);
		for (k = num; k--; v++, pte++)
			loadpgmap((u_int)v, pte, new);
		if (seg < cp->ctx_smin)
			cp->ctx_smin = seg;
	} else {
		if (need)
			panic("need pmeg in hole");
		pmegfree(*pmxp);
		*pmxp = 0;
		setsegmap((u_int)seg, (u_char)SEGINV);
	}
	(void) splx(s);
}

/*
 * Unload bits for specified pmeg.
 */
pmegunload(pmp)
	register struct pmeg *pmp;	/* pointer to pmeg */
{
	register struct pte *pte;
	register int num, k, v;

	if (pmp->pm_procp == 0)
		panic("pmegunload");
	setsegmap(CSEG, (u_char)(pmp - pmeg));
	v = NPAGSEG * CSEG;
	num = pmp->pm_count;
	pte = pmp->pm_pte;
	if (num < 0) {
		num = -num;
		v += NPAGSEG - num;
	}
	if (pmp->pm_seg == 0)		/* special case seg zero */
		v += LOWPAGES;
	for (k = num; k--; v++, pte++)
		unloadpgmap((u_int)v, pte);
	setsegmap(CSEG, (u_char)SEGINV);
}

/*
 * Get referenced and modified bits for
 * the pmeg containing page v.  Called
 * only by pageout.
 */
ptesync(p, v)
	register struct proc *p;
	register unsigned v;
{
	register struct context *cp;
	register struct pmeg *pmp;
	register int pm, s;

	s = splimp();
	if ((cp = p->p_ctx) == NULL)
		goto out;
	if ((pm = cp->ctx_pmeg[ptos(v)]) == 0)
		goto out;
	pmp = &pmeg[pm];
	if (pmp->pm_procp != p)
		panic("ptesync procp");
	if (pmp->pm_time != potime) {		/* check mod time, done? */
		pmegunload(pmp);
		pmp->pm_time = potime;
	}
out:
	(void) splx(s);
}

/*
 * get a kernel page map entry given an address
 */
getkpgmap(addr)
	caddr_t addr;
{

	return ((int)getpgmap(addr));
}

/*
 * Initialize the context structures.
 */
ctxinit()
{
	register int i;

	for (i = 0; i < NCONTEXT; i++) {
		if (i == KCONTEXT)
			continue;
		context[i].ctx_context = i;
	}
}

/*
 * Allocate a context and corresponding
 * page map entries for the current process.
 * If no free context must take one away
 * from someone.
 */
ctxalloc()
{
	register struct proc *p = u.u_procp;	/* process this is for */
	register struct context *cp, *scp = 0;
	register int ct, i;

	/* find a free context or an old one */
	for (cp = context; cp < &context[NCONTEXT]; cp++) {
		if (cp == &context[KCONTEXT])
			continue;
		if (cp->ctx_procp == 0)		/* if no process use this one */
			goto found;
		if (scp == 0) {			/* otherwise find the oldest */
			scp = cp;
			ct = cp->ctx_time;
		} else if (cp->ctx_time <= ct) {
			scp = cp;
			ct = cp->ctx_time;
		}
	}
	cp = scp;		/* reset pointer to save context pointer */
	if (cp->ctx_procp)	/* if in use free it up before using */
		ctxfree(cp->ctx_procp);
found:
	p->p_ctx = cp;		/* set context pointer in proc entry */
	cp->ctx_procp = p;	/* set proc pointer in context table */
	cp->ctx_time = ctxtime++;
	setcontext((int)cp->ctx_context);
	for (i = 0; i <= cp->ctx_tdmax; i++)
		setsegmap((u_int)i, (u_char)SEGINV);
	for (i = cp->ctx_smin; i < ptos(btop(USRSTACK)); i++)
		setsegmap((u_int)i, (u_char)SEGINV);
	cp->ctx_tdmax = 0;
	cp->ctx_smin = (ptos(btop(USRSTACK)));
	p->p_flag &= ~SPTECHG;
}

/*
 * Free the context and page map entries
 * of the specified process.
 */
ctxfree(p)
	register struct proc *p;
{
	register struct context *cp;
	register u_char *pmxp;
	register int s;

	if ((cp = p->p_ctx) == 0)	/* no context */
		return;
	if (p != cp->ctx_procp)		/* not the same process */
		panic("ctxfree");
	if ((p->p_flag&SWEXIT) == 0)	/* don't bother if dieing */
		ctxunload(p);
	s = splimp();

	/* free the pmegs for this context */
	for (pmxp = cp->ctx_pmeg; pmxp < &cp->ctx_pmeg[ptos(btop(USRSTACK))];
	    pmxp++) {
		if (*pmxp) {		/* is there a pmeg for this segment */
			pmegfree(*pmxp);
			*pmxp = 0;
		}
	}
	(void) splx(s);			/* back to normal */
	setcontext(KCONTEXT);		/* paranoid */
	cp->ctx_procp = 0;		/* reset proc pointer */
	p->p_ctx = 0;			/* reset context pointer */
}

/*
 * Set up the segment and page map entries for
 * the current process.
 */
ctxsetup()
{
	register int i;
	register int last = ptos(btop(USRSTACK));	/* last segment */
	register struct context *cp = u.u_procp->p_ctx;

	/*
	 * Initialize all segments.
	 */
	for (i = 0; i <= cp->ctx_tdmax; i++)	/* load pmegs for text/data */
		pmegload(i, 0);
	for (i = cp->ctx_smin; i < last; i++)	/* load pmegs for stack */
		pmegload(i, 0);

	u.u_procp->p_flag &= ~SPTECHG;
}

/*
 * Unload the referenced and modified bits
 * for the specified process.
 */
ctxunload(p)
	struct proc *p;
{
	register int last = ptos(btop(USRSTACK));	/* last segment */
	register int i, s = splimp();
	register u_char *pmxp;

	/*
	 * Unload bits from all allocated pmegs.
	 */
	pmxp = p->p_ctx->ctx_pmeg;	/* get pointer to pte's */
	for (i = 0; i < last; i++, pmxp++)
		if (*pmxp)		/* is it set */
			pmegunload(&pmeg[*pmxp]);
	(void) splx(s);
}

/*
 * Pass all resources associated with a context
 * from process p to process q.  Used by vfork.
 */
ctxpass(p, q)
	register struct proc *p, *q;
{
	register struct context *cp = p->p_ctx;
	register u_char *pmxp;
	register int last = ptos(btop(USRSTACK));	/* last segment */
	register int i;

	if (cp == 0)
		return;
	/*
	 * Pass the context from p to q.
	 */
	q->p_ctx = cp;		/* q gets p's context */
	p->p_ctx = 0; 		/* p loses the context */
	cp->ctx_procp = q;	/* context get q's proc id */
	q->p_flag |= SPTECHG;	/* conservative */
	setcontext(KCONTEXT);	/* paranoid */
	
	/*
	 * Change all pmegs to refer to q.
	 */
	pmxp = cp->ctx_pmeg;
	for (i = 0; i < last; i++, pmxp++)
		if (*pmxp)
			pmeg[*pmxp].pm_procp = q;
}

/*
 * Handle a page fault on a 68020.
 */
pagefault(accaddr)
	register int accaddr;
{
	register struct proc *p = u.u_procp;
	register int v = btop(accaddr);
	struct pte *addrtopte();
	int i, seg;
	int s;

	/*
	 * If user has no context, allocate one for him.
	 */
	if (getcontext() == KCONTEXT) {
		usetup();
		return (1);
	}

	if (addrtopte((caddr_t)accaddr, 1) == NULL)
		return (0);

	seg = ptos(v);
	if (p->p_ctx->ctx_pmeg[seg]) {
		if (getpgmap((caddr_t)accaddr) & PG_V)
			return (0);
		i = u.u_error;
		pagein((u_int)accaddr, &u, 0);
		u.u_error = i;
	}
	s = splimp();
	if (p->p_ctx && p->p_ctx->ctx_pmeg[seg] == 0)
		pmegload(seg, 1);
	(void) splx(s);
	return (1);
}

/*
 * Set up everything the user program might need.
 * If we need a context, allocate it.  If we need
 * to set up hardware segment and page maps, do it.
 */
usetup()
{
	register struct proc *p = u.u_procp;

	if (p->p_ctx == 0)			/* do we need a context */
		ctxalloc();
	else {
		p->p_ctx->ctx_time = ctxtime++;		/* update time */
		setcontext((int)p->p_ctx->ctx_context);	/* set to user */
		if (p->p_flag & SPTECHG)		/* are we changing? */
			ctxsetup();
	}
}

/*
 * Set a red zone below the kernel stack.
 * NO LONGER USED, startup() SETS THE REDZONE.
 */
/*ARGSUSED*/
setredzone(pte, vaddr)
	struct pte *pte;
	caddr_t vaddr;
{
}

/*
 * Map a physical address range into kernel virtual addresses.
 * Since the kernel appears in all contexts any new pmegs are
 * mapped in all contexts.
 */
mapin(ppte, v, paddr, size, access)
	register struct pte *ppte;	/* pointer to pte's */
	u_int v;			/* page number to map in */
	register u_int paddr;		/* physical address */
	register int size, access;	/* size in pages and access rights */
{
	register caddr_t vaddr = (caddr_t)ctob(v);
	register u_char pm;
	register int c, i;
	int s = splimp();

	while (size--) {
		if ((pm = getsegmap((u_int)ptos(v))) == SEGINV) {
			caddr_t va, vs;

			pm = pmegalloc((struct proc *)0);
			kernpmeg++;
			c = getcontext();
			/* need to map in new seg across all contexts */
			for (i = 0; i < NCONTEXT; i++) { 
				setcontext(i);
				setsegmap(ptos(v), pm);
			}
			setcontext(c);
			vs = (caddr_t)(ptos(v)<<SGSHIFT);
			for (va = vs; va < vs + NBSG; va += NBPG)
				setpgmap(va, (long)0);
		}
		/*
		 * Increment count of number of pme's used in this pmeg.
		 * Allow it to go one past the number of pme's in a pmeg;
		 * this indicates someone is doing a mapin without
		 * corresponding mapout's and will be noticed in mapout
		 * who will prevent the reference count from changing.
		 */
		if (pmeg[pm].pm_count <= NPAGSEG)
			pmeg[pm].pm_count++;
		*((int *)ppte) = (paddr & PG_PFNUM) | access;
		setpgmap(vaddr, *(long *)ppte);
		ppte++;
		paddr++;
		v++;
		vaddr += NBPG;
	}
	(void) splx(s);
}

/*
 * Release mapping for kernel.
 * This frees pmegs, which are the most critical resource.
 * Since the kernel appears in all contexts the pmeg has to be mapped 
 * out for all contexts.  Assumes that ppte is a pointer
 * to a pte within Sysmap.
 */
mapout(ppte, size)
	register struct pte *ppte;
{
	register int vaddr = ctob(ppte - Sysmap) + KERNELBASE;
	register u_char pm;
	register int c, i;
	int s = splimp();

	while (size--) {
		if (!ppte->pg_v)
			panic("mapout: invalid pte");
		ppte->pg_v = 0;
		if ((pm = getsegmap((u_int)ptos(btop(vaddr)))) == SEGINV)
			panic("mapout: invalid segment");
		if ((getpgmap((caddr_t)vaddr)&PG_V) == 0)
			panic("mapout: invalid page");
		if (pmeg[pm].pm_count <= 0)
			panic("mapout: pmeg count");
		setpgmap((caddr_t)vaddr, (long)0);
		if (pmeg[pm].pm_count <= NPAGSEG)
			if (--pmeg[pm].pm_count == 0) {	/* done with all ptes */
				c = getcontext();
				/* need to map out seg across all contexts */
				for (i = 0; i < NCONTEXT; i++) { 
					setcontext(i);
					setsegmap((u_int)ptos(btop(vaddr)),
						(u_char)SEGINV);
				}
				setcontext(c);		/* reset context */
				pmegfree(pm);
				kernpmeg--;
			}
		ppte++;					/* do next pte */
		vaddr += NBPG;
	}
	(void) splx(s);
}

/*
 * Check user accessibility to a given address.
 */
useracc(vaddr, count, access)
	caddr_t vaddr;
	u_int count;
	int access;
{
	register struct pte *pte;
	struct pte *addrtopte();

	pte = addrtopte(vaddr, count);
	if (pte == NULL)
		return (0);

	count = btop((int)(vaddr + count - 1)) - btop((int)vaddr) + 1;
	access = access == B_READ ? 0 : PG_W;
	while (count--) {
		if (((*(int *)pte) & PG_S) ||
		    (((*(int *)pte) & PG_W)) < access)
			return (0);
		pte++;
	}
	return (1);
}

/*
 * Check kernel accessibility to a given address.
 * Unlike the vax, vaddr is checked against the range of Sysmap only!
 */
kernacc(vaddr, count, access)
	caddr_t vaddr;
	u_int count;
	int access;
{
	register struct pte *ppte = &Sysmap[btop((int)vaddr - KERNELBASE)];
	extern struct pte ESysmap[];

	count = btoc((int)vaddr + count) - btop(vaddr);
	if (ppte + count > ESysmap || ppte < Sysmap)
		return (0);
	access = access == B_READ ? 0 : PG_W;
	while (count--) {
		if (!ppte->pg_v || ((((*(int *)ppte) & PG_W)) < access))
			return (0);
		ppte++;
	}
	return (1);
}

/*
 * Check for valid program size
 */
chksize(ts, ds, ss)
	unsigned ts, ds, ss;
{
	static int maxdmap = 0;

	if (ts > MAXTSIZ || ds > MAXDSIZ || ss > MAXSSIZ) {
		u.u_error = ENOMEM;
		return (1);
	}
	/* check for swap map overflow */
	if (maxdmap == 0) {
		register int i, blk;

		blk = DMMIN;
		for (i = 0; i < NDMAP; i++) {
			maxdmap += blk;
			if (blk < DMMAX)
				blk *= 2;
		}
	}
	if (ctod(ts) > NXDAD*DMTEXT ||
	    ctod(ds) > maxdmap || ctod(ss) > maxdmap) {
		u.u_error = ENOMEM;
		return (1);
	}
	/*
	 * Make sure the process isn't bigger than our
	 * virtual memory limit.
	 *
	 * THERE SHOULD BE A CONSTANT FOR THIS.
	 */
	if (ctos(ts + LOWPAGES) + ctos(ds) + ctos(ss + HIGHPAGES) >
	    ctos(btop(USRSTACK))) {
		u.u_error = ENOMEM;
		return (1);
	}
	return (0);
}

/*
 * Change the translation for the current proc
 * to reflect the change made in software ptes
 * starting at ppte for size ptes.
 */
newptes(ppte, v, size)
	register struct pte *ppte;
	u_int v;
	int size;
{
	register int i, fs, ls, need;

	if (getcontext() == KCONTEXT) {
		if (u.u_procp->p_ctx)
			usetup();
		else
			return;
	}
	fs = ptos(v);			/* convert page pointer for 1st seg */
	ls = ptos(v + size - 1);	/* convert page pointer for last seg */
	need = ppte->pg_v;
	for (i = fs; i <= ls; i++)	/* go through list and set ptes */
		pmegload(i, need);
}

/*
 * Move pages from one kernel virtual address to another.
 * Both addresses are assumed to reside in the Sysmap,
 * and size must be a multiple of CLSIZE.
 */
pagemove(from, to, size)
	register caddr_t from, to;
	int size;
{
	register struct pte *fpte, *tpte;

	if (size % CLBYTES)
		panic("pagemove");
	fpte = &Sysmap[btop((int)from - KERNELBASE)];
	tpte = &Sysmap[btop((int)to - KERNELBASE)];
	while (size > 0) {
		*tpte++ = *fpte;
		setpgmap(to, *(long *)fpte);
		*(int *)fpte++ = 0;
		setpgmap(from, (long)0);
		from += NBPG;
		to += NBPG;
		size -= NBPG;
	}
}

/*
 * Check the validity of a user address range and return NULL
 * on error or a pointer to the first pte for these addresses.
 */
struct pte *
addrtopte(vaddr, count)
	caddr_t vaddr;
	u_int count;
{
	register struct proc *p = u.u_procp;
	register int fv, lv;
	int tss, dss, sss;

	fv = btop((int)vaddr);
	lv = btop((int)(vaddr + count - 1));

	if (lv < fv || fv < btop(USRTEXT) || lv >= btop(USRSTACK))
		return (NULL);

	/*
	 * Check that the request was within the
	 * user's valid address space.  Can't use
	 * isa[tds]sv because they don't check the holes.
	 */
	tss = tptov(p, 0);
	dss = dptov(p, 0);
	sss = sptov(p, p->p_ssize - 1);

	if (fv >= tss && lv < tss + p->p_tsize)
		return (tptopte(p, vtotp(p, fv)));
	else if (fv >= dss && lv < dss + p->p_dsize)
		return (dptopte(p, vtodp(p, fv)));
	else if (fv >= sss && lv < sss + p->p_ssize)
		return (sptopte(p, vtosp(p, fv)));

	return (NULL);
}

#define	ONBPG 		2048	/* old page size */
#define	ONBSG 		32768	/* old segment size */

/*
 * Routine used to check to see if an a.out can be executed
 * by the current machine/architecture.
 */
chkaout()
{

	if ((u.u_exdata.ux_mach == M_68010) ||
	    (u.u_exdata.ux_mach == M_68020))
		return (0);
	else 
		return (ENOEXEC);
}

/* 
 * The following functions return information about an a.out
 * which is used when a program is executed.
 */

/* 
 * Return the size of the text segment adjusted for the type of a.out.
 */
size_t
getts()
{
	return (clrnd(btoc(u.u_exdata.ux_tsize)));
}

/* 
 * Return the size of the data segment depending on the type of a.out.
 * For the case of an old a.out we need to allow for the old segment
 * alignment and the fact that the text segment starts at 32k and not 8k.
 * To do this we calculate the size of the text segment and round
 * it to the next old Sun-2 segment boundary.
 */
size_t
getds()
{

	return (clrnd(btoc(u.u_exdata.ux_dsize + u.u_exdata.ux_bsize)));
}

/* 
 * Return the load memory address for the data segment.
 */
caddr_t
getdmem()
{

	return ((caddr_t)ctob(dptov(u.u_procp, 0)));
}

/* 
 * Return the starting disk address for the data segment.
 */
getdfile()
{

	if (u.u_exdata.ux_mag == ZMAGIC)
		return (u.u_exdata.ux_tsize);
	else
		return (sizeof (u.u_exdata) + u.u_exdata.ux_tsize);
}

/* 
 * Return the load memory address for the text segment.
 */
caddr_t
gettmem(up)
struct user *up;
{

	return ((caddr_t)USRTEXT);
}

/* 
 * Return the file byte offset for the text segment.
 */
gettfile(up)
struct user *up;
{

	if (up->u_exdata.ux_mag == ZMAGIC)
		return (0);
	else
		return (sizeof (u.u_exdata));
}
