/*	main.c	4.14	81/04/23	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/filsys.h"
#include "../h/mount.h"
#include "../h/map.h"
#include "../h/proc.h"
#include "../h/inode.h"
#include "../h/seg.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../machine/pte.h"
/*#include "../h/clock.h"*/
#include "../h/vm.h"
#include "../h/cmap.h"
#include "../h/text.h"
#include "../h/vlimit.h"
#include "../h/trace.h"

/*
 * Initialization code.
 * Called from cold start routine as
 * soon as a stack and segmentation
 * have been established.
 * Functions:
 *	clear and free user core
 *	turn on clock
 *	hand craft 0th process
 *	call all initialization routines
 *	fork - process 0 to schedule
 *	     - process 2 to page out
 *	     - process 1 execute bootstrap
 *
 * loop at loc 13 (0xd) in user mode -- /etc/init
 *	cannot be executed.
 */
#ifdef vax
main(firstaddr)
#endif
#ifdef sun
main(regs)
	struct regs regs;
#endif
{
	register int i;
	register struct proc *p;

	rqinit();
#ifdef vax
	startup(firstaddr);
#endif
#ifdef sun
	startup();
#endif

	/*
	 * set up system process 0 (swapper)
	 */
	p = &proc[0];
	p->p_p0br = u.u_pcb.pcb_p0br;
	p->p_p1br = u.u_pcb.pcb_p1br;
	p->p_szpt = 1;
	p->p_addr = uaddr(p);
	p->p_stat = SRUN;
	p->p_flag |= SLOAD|SSYS;
	p->p_nice = NZERO;
	setredzone(p->p_addr, (caddr_t)&u);
	u.u_procp = p;
#ifdef sun
	u.u_ar0 = &regs.r_r0;
#endif
	u.u_cmask = CMASK;
	for (i = 0; i < NGROUPS; i++)
		u.u_groups[i] = NOGROUP;	/* null access group for init */
	for (i = 1; i < sizeof(u.u_limit)/sizeof(u.u_limit[0]); i++)
		switch (i) {

		case LIM_STACK:
			u.u_limit[i] = ctob(MAXSSIZ);
			continue;
		case LIM_DATA:
			u.u_limit[i] = ctob(MAXDSIZ);
			continue;
		case LIM_TEXT:
			u.u_limit[i] = ctob(MAXTSIZ);
			continue;
		default:
			u.u_limit[i] = INFINITY;
			continue;
		}
	p->p_maxrss = INFINITY/NBPG;

	/*
	 * Initialise procs and lnodes.
	 */

	initlnodes(lnodes);
	p->p_lnode = lnodes;

	clkstart();

	/*
	 * Initialize devices and
	 * set up 'known' i-nodes
	 */

	ihinit();
	bhinit();
	qinit();
	binit();
	bswinit();
	iinit();
	/* should really mount root fs here */
	u.u_dmap = zdmap;
	u.u_smap = zdmap;

	/*
	 * Set the scan rate and other parameters of the paging subsystem.
	 */
	setupclock();

	/*
	 * make page-out daemon (process 2)
	 * the daemon has ctopt(nswbuf*CLSIZE*KLMAX) pages of page
	 * table so that it can map dirty pages into
	 * its address space during asychronous pushes.
	 */

	mpid = 1;
	proc[0].p_szpt = clrnd(ctopt(nswbuf*CLSIZE*KLMAX + UPAGES));
	proc[1].p_stat = SZOMB;		/* force it to be in proc slot 2 */
	if (newproc(0)) {
		proc[2].p_flag |= SLOAD|SSYS;
		proc[2].p_dsize = u.u_dsize = nswbuf*CLSIZE*KLMAX; 
		pageout();
	}

	/*
	 * make init process and
	 * enter scheduling loop
	 */

	mpid = 0;
	proc[1].p_stat = 0;
	proc[0].p_szpt = CLSIZE;
	if (newproc(0)) {
#ifdef vax
		expand(clrnd((int)btoc(szicode)), P0BR);
		(void) swpexpand(u.u_dsize, 0, &u.u_dmap, &u.u_smap);
		(void) copyout((caddr_t)icode, (caddr_t)0, (unsigned)szicode);
#endif
#ifdef sun
		icode();
#endif
		/*
		 * Return goes to loc. 0 of user init
		 * code just copied out.
		 */
		return;
	}
	proc[0].p_szpt = 1;
#ifdef TRACE
	traceflags[TR_VADVISE]++;
	for(i = 0; i < 10; i++)
		trace(TR_VADVISE, i, 0);
	for(i = 0; i < 10; i++)
		traceflags[i]++;
#endif TRACE
	sched();
}

/*
 * iinit is called once (from main)
 * very early in initialization.
 * It reads the root's super block
 * and initializes the current date
 * from the last modified date.
 *
 * panic: iinit -- cannot read the super
 * block. Usually because of an IO error.
 */
iinit()
{
	register struct buf *bp;
	register struct filsys *fp;
	register int i;
	register struct mount *mp;
	extern int rootfstyp;
	struct inode pi;

	(*bdevsw[major(rootdev)].d_open)(rootdev, 1);
	bp = bread(rootdev, SUPERB);
	if(u.u_error)
		panic("iinit");
	bp->b_flags |= B_LOCKED;		/* block can never be re-used */
	brelse(bp);
	mp = allocmount(rootfstyp, rootdev);
	if (mp == NULL)
		panic("iinit");
	mp->m_bufp = bp;
	fp = bp->b_un.b_filsys;
	fp->s_flock = 0;
	fp->s_ilock = 0;
	fp->s_ronly = 0;
	fp->s_lasti = 1;
	fp->s_nbehind = 0;
	fp->s_fsmnt[0] = '/';
	for (i = 1; i < sizeof(fp->s_fsmnt); i++)
		fp->s_fsmnt[i] = 0;
	clkinit(fp->s_time);
	bootime = time;
	pi.i_dev = rootdev;
	pi.i_fstyp = rootfstyp;	/* which is probably 0 */
	pi.i_un.i_bufp = bp;
	rootdir = iget(&pi, rootdev, (ino_t)ROOTINO);
	if (rootdir == NULL)
		panic("rootdir");
	rootdir->i_flag &= ~ILOCK;
	rootdir->i_mpoint = rootdir;
	rootdir->i_mroot = rootdir;
	u.u_cdir = iget(rootdir, rootdev, (ino_t)ROOTINO);
	u.u_cdir->i_flag &= ~ILOCK;
	u.u_rdir = NULL;
}

/*
 * Initialize the buffer I/O system by freeing
 * all buffers and setting all device buffer lists to empty.
 */
binit()
{
	register struct buf *bp;
	register struct buf *dp;
	register int i;
	struct bdevsw *bdp;
	struct swdevt *swp;

	for (dp = bfreelist; dp < &bfreelist[BQUEUES]; dp++) {
		dp->b_forw = dp->b_back = dp->av_forw = dp->av_back = dp;
		dp->b_flags = B_HEAD;
	}
	dp--;				/* dp = &bfreelist[BQUEUES-1]; */
	for (i=0; i<nbuf; i++) {
		bp = &buf[i];
		bp->b_dev = NODEV;
		bp->b_un.b_addr = buffers + i * BUFSIZE;
		bp->b_back = dp;
		bp->b_forw = dp->b_forw;
		dp->b_forw->b_back = bp;
		dp->b_forw = bp;
		bp->b_flags = B_BUSY|B_INVAL;
		brelse(bp);
	}
	for (bdp = bdevsw; bdp->d_open; bdp++)
		nblkdev++;
	/*
	 * Count swap devices, and adjust total swap space available.
	 * Some of this space will not be available until a vswapon()
	 * system is issued, usually when the system goes multi-user.
	 */
	nswdev = 0;
	for (swp = swdevt; swp->sw_dev; swp++)
		nswdev++;
	if (nswdev == 0)
		panic("binit");
	nswap *= nswdev;
	maxpgio *= nswdev;
	swfree(0);
}

/*
 * Initialize linked list of free swap
 * headers. These do not actually point
 * to buffers, but rather to pages that
 * are being swapped in and out.
 */
bswinit()
{
	register int i;
	register struct buf *sp = swbuf;

	bswlist.av_forw = sp;
	for (i=0; i<nswbuf-1; i++, sp++)
		sp->av_forw = sp+1;
	sp->av_forw = NULL;
}
