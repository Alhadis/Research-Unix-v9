/*
 * DH-11/DM-11 driver for Eighth Edition UNIX
 * Synthesised by Ian Darwin and Geoff Collyer,
 * University of Toronto, January 1986.
 */

#include "dh.h"
#include "dm.h"
#if NDH > 0
#include "../h/param.h"
#include "../h/stream.h"
#include "../h/ioctl.h"
#include "../h/ttyld.h"
#include "../h/pte.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/ubareg.h"
#include "../h/ubavar.h"
#include "../h/conf.h"
#include "sparam.h"

#include "../h/dhreg.h"		/* half of hardware */
#include "../h/dmreg.h"		/* half of hardware */
#include "../h/mdmreg.h"	/* modem command stuff */


#define NPER	16		/* lines per board */
#define	NDHLINE	(NDH*NPER)

/* bits in dh[]->state */
#define	EXISTS	01
#define	ISOPEN	02
#define	WOPEN	04
#define	TIMEOUT	010
#define	CARR_ON	020
#define	DHSTOP	040
#define	HPCL	0100
#define	BRKING	0200
#define	DIALOUT	0400	/* used as dialout with smart modems */
#define BUSY	01000
#define FLUSH	02000

#define SSPEED	B4800	/* half reasonable default nowadays */
#define DHPRI	30

/*
 * interface to auto-configuration.
 * There is one definition for the dh and one for the dm.
 */
int	dhprobe(), dhattach(), dhrint(), dhtint();
struct	uba_device *dhboard[NDH];
u_short	dhstd[] = { 0 };
struct	uba_driver dhdriver =
	{ dhprobe, 0, dhattach, 0, dhstd, "dh", dhboard };

int	dmprobe(), dmattach(), dmintr();
#define	NDM	1
struct	uba_device *dmboard[NDM];
u_short	dmstd[] = { 0 };
struct	uba_driver dmdriver =
	{ dmprobe, 0, dmattach, 0, dmstd, "dm", dmboard };

/*
 * interface with stream i/o system
 */
int	dhopen(), dhclose(), dhoput(), nodev();
static	struct qinit dhrinit = { nodev, NULL, dhopen, dhclose, 0, 0 };
static	struct qinit dhwinit = { dhoput, NULL, dhopen, dhclose, 200, 100 };
struct	streamtab dhinfo = { &dhrinit, &dhwinit };

/*
 * private data for the driver
 */

/*
 * Well, we can't map the clist space onto each Unibus,
 * since we ain't got no more clists (rah!).
 * Fortunately, we can get the same effect by mapping all stream
 * buffers onto the Unibus space  (as does qinit()).
 */
#define UBADMASK 0x3ffff		/* 18 bits of address */
#ifdef MULTI_UBA
#define UBACVT(x, uba)  (dhblkubad[uba] + ((caddr_t)(x)-(caddr_t)blkdata))
#else
#define UBACVT(x, uba)  ((blkubad&UBADMASK) + ((caddr_t)(x)-(caddr_t)blkdata))
#endif
#define	UBADDR(x, uba)  (UBACVT((x), (uba))&0xffff)	/* low order 16 bits */
#define UBXADDR(x, uba) ((UBACVT((x), (uba))>>16)&0x3)	/* high order 2 bits */

/*
 * One structure per line
 */
struct	dh {
	short	state;
	short	flags;
	struct	block	*oblock;
	struct	queue	*rdq;
	char	board;
	char	line;
	char	ispeed, ospeed;
	short	brking;
} dh[NDHLINE];

/*
 * misc private data
 */
#ifdef MULTI_UBA
long	dhblkubad[MAXNUBA];		/* uba allocation */
#else
extern long blkubad;
extern u_char blkdata[];
#endif
int	dhoverrun;

short	dhsar[NDH];		/* software copy of last bar */
int	dhmiss;			/* chars lost due to q full */

/*
 * routines for top half hardware -- the dh11
 */
int	dhstart();

/*
 * Routine for configuration to force a dh to interrupt.
 * Set to transmit at 9600 baud, and cause a transmitter interrupt.
 *
 * From 4BSD.
 */
/*ARGSUSED*/
dhprobe(reg)
caddr_t reg;
{
	register int br, cvec;		/* these are ``value-result'' */
	register struct dhdevice *dhaddr = (struct dhdevice *)reg;

#ifdef lint
	br = 0; cvec = br; br = cvec;
	dhrint(0); dhtint(0);
#endif
	dhaddr->un.dhcsr = DH_RIE|DH_MM|DH_RI;
	DELAY(1000);
	dhaddr->un.dhcsr &= ~DH_RI;
	dhaddr->un.dhcsr = 0;
	return 1;
}

/*
 * Routine to attach a dh.
 * Map the stream buffers onto the unibus (qinit tries to do this,
 * but only does it for unibus zero; the dh might be on any unibus).
 */
dhattach(ui)
register struct uba_device *ui;
{
	register int i;
#ifdef MULTI_UBA
	/* this defn must match that in dev/stream.c: */
	extern u_char blkdata[1024*NBLKBIG + 64*NBLK64 + NBLK16*16 + NBLK4*4];
	extern long blkubad;		/* wanton licentiousness w/ streamld */
#endif

	for (i = 0; i < NPER; i++) {
		dh[ui->ui_unit*NPER+i].state = EXISTS;
		dh[ui->ui_unit*NPER+i].board = ui->ui_unit;
		dh[ui->ui_unit*NPER+i].line  = i;
	}
#ifdef MULTI_UBA
#if 0
	if (ui->ui_ubanum == 0)			/* uba 0 */
		/* won't work because blkubad isn't initialised yet */
		dhblkubad[ui->ui_ubanum] = blkubad;
	else
#endif
		/* adding 512 is from 4.2, allegedly a hardware kludge */
		dhblkubad[ui->ui_ubanum] = uballoc(ui->ui_ubanum,
			(caddr_t)blkdata, 512 + sizeof blkdata, 0);
	if (dhblkubad[ui->ui_ubanum] == 0) {
		printf("dh%d: can't map blkdata\n", ui->ui_unit);
		return 0;
	}
	dhblkubad[ui->ui_ubanum] &= UBADMASK;
#endif
	return 1;
}

/*
 * Open a DH11 line.
 * Do not monkey with uba mapping.
 * Turn on dh if this is its first open.
 * Call dmopen to wait for carrier.
 */
/*ARGSUSED*/
dhopen(q, d, flag)
register struct queue *q;
dev_t d;
int flag;
{
	register int dev;
	register struct dh *dhp;

	dev = minor(d);
	dhp = &dh[dev];
	if (dev >= NDHLINE || !(dhp->state & EXISTS))
		return (0);

	q->ptr = (caddr_t)dhp;
	WR(q)->ptr = (caddr_t)dhp;
	/*
	 * If this is first open, initialise tty state to default.
	 */
	if ((dhp->state&ISOPEN)==0 || (dhp->state&CARR_ON)==0) {
		register int s = spl5();

		dhp->flags = ODDP|EVENP;
		dhp->ispeed = dhp->ospeed = SSPEED;
		dhp->state |= WOPEN;
		dhparam(dhp);
		dmopen(dev);	/* wait for carrier */
		if (!(dhp->state & CARR_ON)) {
			wakeup((caddr_t)dhp);
			dhp->ispeed = dhp->ospeed = 0;
			dhparam(dhp);
			splx(s);
			return 0;
		}
		dhp->rdq = q;
		dhp->state |= ISOPEN;
		dhp->state &= ~WOPEN;
		splx(s);
	}
	return 1;
}

/*
 * Close a DH11 line, turning off the DM11.
 */
/*ARGSUSED*/
dhclose(q, d)
dev_t d;
register struct queue *q;
{
	register struct dh *dhp;
	int s = spl5();

	dhp = (struct dh *)q->ptr;
	if (dhp->oblock != NULL) {
		register struct block *bp = dhp->oblock;

		dhp->oblock = NULL;
		freeb(bp);
	}
	flushq(WR(q), 1);
	dhp->rdq = NULL;
	((struct dhdevice *)dhboard[dhp->board]->ui_addr)->dhbreak &=
		~(1<<(minor(d) & 017));
	if (dhp->state&HPCL || (dhp->state&CARR_ON)==0) {
		dhp->ispeed = dhp->ospeed = 0;
		dhparam(dhp);
	}
	dhp->state &= EXISTS;
	
	/* there is no dmclose() to call */
	splx(s);
}


/*
 * dh11 write put routine
 */
dhoput(q, bp)
register struct queue *q;
register struct block *bp;
{
	register struct dh *dhp = (struct dh *)q->ptr;
	register union stmsg *sp;
	register int s;
	int delaytime;

	switch (bp->type) {

	case M_IOCTL:
		sp = (union stmsg *)bp->rptr;
		switch (sp->ioc0.com) {

		case TIOCSETP:
		case TIOCSETN:
			delaytime = 0;
			if (dhp->ispeed != sp->ioc1.sb.sg_ispeed)
				delaytime = 20;
			dhp->ispeed = sp->ioc1.sb.sg_ispeed;
			dhp->flags = sp->ioc1.sb.sg_flags;
			bp->type = M_IOCACK;
			bp->wptr = bp->rptr;
			qreply(q, bp);
			qpctl1(q, M_DELAY, delaytime);	/* wait a bit */
			qpctl(q, M_CTL);		/* means do dhparam */
			dhstart(dhp);
			return;

		case TIOCGETP:
			sp->ioc1.sb.sg_ispeed = dhp->ispeed;
			sp->ioc1.sb.sg_ospeed = dhp->ospeed;
			bp->type = M_IOCACK;
			qreply(q, bp);
			return;

		case TIOCHPCL:
			dhp->state |= HPCL;
			bp->type = M_IOCACK;
			bp->wptr = bp->rptr;
			qreply(q, bp);
			return;

		default:
			bp->wptr = bp->rptr;
			bp->type = M_IOCNAK;
			qreply(q, bp);
			return;
		}

	case M_STOP:
		s = spl5();
		dhp->state |= DHSTOP;
		freeb(bp);
		if ((bp = dhp->oblock) != NULL) {
			dhp->oblock = NULL;
			putbq(q, bp);
		}
		dhstop(dhp);
		splx(s);
		return;

	case M_START:
		dhp->state &= ~DHSTOP;
		dhstart(dhp);
		break;

	case M_FLUSH:
		flushq(q, 1);
		freeb(bp);
		s = spl5();
		if ((bp = dhp->oblock) != NULL) {
			dhp->oblock = NULL;
			freeb(bp);
		}
		splx(s);
		return;

	case M_BREAK:
		qpctl1(q, M_DELAY, 10);
		putq(q, bp);
		qpctl1(q, M_DELAY, 10);
		dhstart(dhp);
		return;

	case M_HANGUP:
		dhp->state &= ~DHSTOP;
	case M_DELAY:
	case M_DATA:
		putq(q, bp);
		dhstart(dhp);
		return;

	default:		/* not handled; just toss */
		break;
	}
	freeb(bp);
}


/*
 * Set parameters from open or stty into the DH hardware
 * registers.
 */
dhparam(dhp)
register struct dh *dhp;
{
	register struct dhdevice *addr;
	register int lpar;
	int unit = dhp->line;

	addr = (struct dhdevice *)dhboard[dhp->board]->ui_addr;
	addr->un.dhcsr = (unit&017) | DH_IE;
	if ((dhp->ispeed)==0) {
		dhp->state |= HPCL;
		dmctl(unit, DML_OFF, DMSET);		/* hang up */
		return;
	}
	lpar = ((dhp->ospeed)<<10) | ((dhp->ispeed)<<6);
	if ((dhp->ispeed) == B134)
		lpar |= BITS6|PENABLE|HDUPLX;
	else if (dhp->flags & RAW)
		lpar |= BITS8;
	else
		lpar |= BITS7|PENABLE;
	if ((dhp->flags&EVENP) == 0)
		lpar |= OPAR;
	if ((dhp->ospeed) == B110)	/* 110 baud (ugh!): 2 stop bits */
		lpar |= TWOSB;
	addr->dhlpr = lpar;
}

/*
 * DH11 receiver interrupt.
 */
dhrint(dev)
int dev;
{
	register struct block *bp;
	register struct dhdevice *dhaddr;
	register struct dh *dhp;
	register int c;

	if (dev >= NDH) {
		printf("dh%d: bad rd intr\n", dev);
		return;
	}
	dhaddr = (struct dhdevice *)dhboard[dev]->ui_addr;
	/*
	 * get chars from the silo for this line
	 */
	while ((c = dhaddr->dhrcr) < 0) { /* char present */
		dhp = &dh[dev*NPER + ((c>>8) & 017)];
		if (c&DH_DO) {
			++dhoverrun;
			continue;
		}
		if (dhp->rdq == NULL)
			continue;
		if (dhp->rdq->next->flag & QFULL) {
			dhmiss++;	/* you lose */
			continue;
		}
		if ((bp = allocb(16)) == NULL)
			continue;	/* out of space - you lose */
		if (c&DH_FE)		/* frame error == BREAK */
			bp->type = M_BREAK;
		else
			*bp->wptr++ = c;
		(*dhp->rdq->next->qinfo->putp)(dhp->rdq->next, bp);
	}
}


/*
 * DH11 transmitter interrupt. Dev is board number.
 * Restart each line which used to be active but has
 * terminated transmission since the last interrupt.
 */
dhtint(dev)
int dev;
{
	register struct dhdevice *dhaddr = (struct dhdevice *)dhboard[dev]->ui_addr;
	register struct dh *dhp;
	register struct block *bp;
	register dev_t unit;
	short bar, *sbar, ttybit;

	if (dhaddr->un.dhcsr & DH_NXM) {	/* somebody goofed */
		dhaddr->un.dhcsr |= DH_CNI;	/* make amends */
		printf("dh%d: NXM\n", dev);
	}
	sbar = &dhsar[dev];
	bar = *sbar & ~dhaddr->dhbar;
	unit = dev * NPER;
	ttybit = 1;
	dhaddr->un.dhcsr &= (short)~DH_TI;	/* no more, please */
	for (; bar; unit++, ttybit <<= 1)
		if (bar & ttybit) {
			*sbar &= ~ttybit;
			bar &= ~ttybit;
			dhp = &dh[unit];
			dhp->state &= ~BUSY;
			if (dhp->state&FLUSH)
				dhp->state &= ~FLUSH;
			else {
				register u_short cntr;
				struct uba_device *ui = dhboard[dev];

				dhaddr->un.dhcsrl = (unit&017)|DH_IE;
				bp = dhp->oblock;
				if (bp != NULL) {
					/*
					 * Do arithmetic in a short to make up
					 * for lost 16&17 address bits.
					 * This is an awful kludge.
					 */
					cntr = dhaddr->dhcar -
						UBACVT(bp->rptr, ui->ui_ubanum);
					/* whole block done? */
					if (cntr == (bp->wptr - bp->rptr)) {
						freeb(bp);
					} else {	/* block interrupted */
						bp->rptr += cntr;
						putbq(dhp->rdq, bp);
					}
					dhp->oblock = NULL;
				}
			}
			dhstart(dhp);
		}
}

dhtimo(dhp)
register struct dh *dhp;
{
	if (dhp->state&BRKING) {
		dhp->brking &= ~(1 << dhp->line);
		((struct dhdevice *)dhboard[dhp->board]->ui_addr)->dhbreak =
		    dhp->brking;
	}
	dhp->state &= ~(TIMEOUT|BRKING);
	dhstart(dhp);
}

/*
 * Start (restart) transmission on the given DH11 line.
 */
dhstart(dhp)
register struct dh *dhp;
{
	register struct dhdevice *dhaddr =
		(struct dhdevice *)dhboard[dhp->board]->ui_addr;
	register int dh, unit;
	register int s = spl5();
	register struct block *bp;

#define	GETOUT	{ splx(s); return; }
	unit = dhp->line;
	dh = unit / NPER;
again:
	if (dhp->state&(TIMEOUT|DHSTOP|BRKING) || dhp->oblock != NULL) {
		GETOUT;
	}
	
	if (dhp->rdq == NULL)
		GETOUT;
	if ((bp = getq(WR(dhp->rdq))) == NULL) {
		GETOUT;
	}

	switch (bp->type) {
	case M_DATA:
		dhp->oblock = bp;
		if (bp->wptr > bp->rptr) {	/* positive length transfer */
			/*
			 * truly hideous, but what can you do when
			 * dealing with a VAX dh?
			 */
			dhaddr->un.dhcsrl = unit | DH_IE |
				UBXADDR(bp->rptr, dhboard[dh]->ui_ubanum) << 4;
			/*
			 * nonsense with `word' is to be sure the dhbar |= word
			 * below is done with an interlocking bisw2 instruction.
			 */
			{
			short word = 1 << unit;
	
			dhsar[dh] |= word;
			dhaddr->dhcar = UBADDR(bp->rptr, dhboard[dh]->ui_ubanum);
			dhaddr->dhbcr = -(bp->wptr - bp->rptr);
			dhaddr->dhbar |= word;
			}
			dhp->state |= BUSY;
		}
		break;

	case M_BREAK:
		dhp->brking |= 1 << dhp->line;
		dhaddr->dhbreak = dhp->brking;
		dhp->state |= BRKING|TIMEOUT;
		timeout(dhtimo, (caddr_t)dhp, 15);	/* about 250 ms */
		freeb(bp);
		break;

	case M_DELAY:
		dhp->state |= TIMEOUT;
		timeout(dhtimo, (caddr_t)dhp, (int)*bp->rptr + 6);
		freeb(bp);
		break;

	case M_HANGUP:
		dhp->ispeed = dhp->ospeed = 0;
	case M_CTL:
		freeb(bp);
		dhparam(dhp);
		goto again;

	}
	splx(s);
}


/*
 * Stop output on a line, e.g. for ^S/^Q or output flush (e.g. ^O).
 */
/*ARGSUSED*/
dhstop(dhp)
register struct dh *dhp;
{
	register struct dhdevice *addr = 
		(struct dhdevice *)dhboard[dhp->board]->ui_addr;
	register int unit, s = spl5();

	if (dhp->state & BUSY) {
		/*
		 * Device is transmitting; stop output
		 * by selecting the line and setting the byte
		 * count to -1.  We will clean up later
		 * by examining the address where the dh stopped.
		 */
		unit = dhp->line;
		addr->un.dhcsrl = (unit&017) | DH_IE;
		dhp = &dh[unit];
		if (!(dhp->state&DHSTOP))	/* not called for ^S */
			dhp->state |= FLUSH;	/* must have been for ^O */
		addr->dhbcr = -1;
	}
	splx(s);
}


/*
 * At software clock interrupt time or after a UNIBUS reset
 * empty all the dh silos.
 */
dhtimer()
{
	register int dh;
	register int s = spl5();

	for (dh = 0; dh < NDH; dh++)
		dhrint(dh);
	splx(s);
}

/*
 * reset driver after a unibus reset
 */
dhreset(uban)
int uban;
{
	register int dhn;
	register int unit;
	register struct dh *dhp;
	register struct uba_device *ui;
	int i;

	for (dhn = 0; dhn < NDH; dhn++) {
		ui = dhboard[dhn];
		if (ui == 0 || ui->ui_alive == 0 || ui->ui_ubanum != uban)
			continue;
		printf(" dh%d", dhn);
		((struct dhdevice *)ui->ui_addr)->un.dhcsr |= DH_IE;
		((struct dhdevice *)ui->ui_addr)->dhsilo = 16;
		unit = dhn * NPER;
		for (i = 0; i < NPER; i++) {
			dhp = &dh[unit];
			if (dhp->state & (ISOPEN|WOPEN)) {
				dhparam(dhp);
				dmctl(unit, DML_ON, DMSET);
				dhp->state &= ~BUSY;
				dhstart(dhp);
			}
			unit++;
		}
	}
	dhtimer();
}

/*
 * Routines for DM11 half of hardware
 */

/*
 * Configuration routine to cause a dm to interrupt.
 */
dmprobe(reg)
caddr_t reg;
{
	register int br, vec;			/* value-result */
	register struct dmdevice *dmaddr = (struct dmdevice *)reg;

#ifdef lint
	br = 0; vec = br; br = vec;
	dmintr(0);
#endif
	dmaddr->dmcsr = DM_DONE|DM_IE;
	DELAY(20);
	dmaddr->dmcsr = 0;
	return (1);
}

/*ARGSUSED*/
dmattach(ui)
struct uba_device *ui;
{

	/* no local state to set up */
}

/*
 * Turn on the line associated with dh dev.
 *
 * From 4BSD.
 */
dmopen(dev)
{
	register struct dh *dhp;
	register struct dmdevice *addr;
	register struct uba_device *ui;
	register dev_t unit;
	register int dm;
	int s;

	unit = minor(dev);
	dm = unit / NPER;
	dhp = &dh[unit];
	unit &= 0xf;
	if (dm >= NDM || (ui = dmboard[dm]) == 0 || ui->ui_alive == 0 ||
	    ui->ui_flags & (1<<unit)) {
		dhp->state |= CARR_ON|DIALOUT;
		return;
	}
	addr = (struct dmdevice *)ui->ui_addr;
	s = spl5();
	addr->dmcsr &= ~DM_SE;
	while (addr->dmcsr & DM_BUSY)
		/* please do not adjust yr set */ ;
	addr->dmcsr = unit;
	addr->dmlstat = DML_ON;
	if (addr->dmlstat&DML_CAR)
		dhp->state |= CARR_ON;
	addr->dmcsr = DM_IE|DM_SE;
	while ((dhp->state&CARR_ON)==0)
		if (tsleep((caddr_t)dhp, DHPRI, 0) != TS_OK)
			break;
	splx(s);
	printf("dmopen(0x%x) state=0x%x\n", dev, dhp->state);
}

/*
 * Dump control bits into the DM registers.
 */
dmctl(dev, bits, how)
int dev;
int bits, how;
{
	register struct uba_device *ui;
	register struct dmdevice *addr;
	register int unit, s;
	int dm;

	unit = minor(dev);
	dm = unit / NPER;
	if ((ui = dmboard[dm]) == 0 || ui->ui_alive == 0)
		return;
	addr = (struct dmdevice *)ui->ui_addr;
	s = spl5();
	addr->dmcsr &= ~DM_SE;
	while (addr->dmcsr & DM_BUSY)
		/* be patient, I'm only a DH-11 */ ;
	addr->dmcsr = unit & 0xf;
	switch (how) {
	case DMSET:
		addr->dmlstat = bits;
		break;
	case DMBIS:
		addr->dmlstat |= bits;
		break;
	case DMBIC:
		addr->dmlstat &= ~bits;
		break;
	}
	addr->dmcsr = DM_IE|DM_SE;
	splx(s);
}

/*
 * DM11 interrupt; deal with carrier transitions.
 */
dmintr(dm)
register int dm;
{
	register struct uba_device *ui;
	register struct dh *dhp;
	register struct dmdevice *addr;

	ui = dmboard[dm];
	if (ui == 0)
		return;
	addr = (struct dmdevice *)ui->ui_addr;
	if (addr->dmcsr&DM_DONE) {
		if (addr->dmcsr&DM_CF) {
			dhp = &dh[dm * NPER +(addr->dmcsr&0xf)];
			wakeup((caddr_t)dhp);
#ifdef	NOTDEF
			if ((dhp->state&WOPEN) == 0 ) {
				if (addr->dmlstat & DML_CAR) {
					dhp->state &= ~DHSTOP;
					wakeup((caddr_t)dhp);
				} else if ((dhp->state&DHSTOP) == 0) {
					dhp->state |= DHSTOP;
					dhstop(dhp);
				}
			} else 
#endif
			if ((addr->dmlstat&DML_CAR)==0) {
				if (!(dhp->state&WOPEN) && !(dhp->state&DIALOUT)) {
					putctl(dhp->rdq->next, M_HANGUP);
					addr->dmlstat = 0;
				}
				dhp->state &= ~CARR_ON;
			} else
				dhp->state |= CARR_ON;
		}
		addr->dmcsr = DM_IE|DM_SE;
	}
}

