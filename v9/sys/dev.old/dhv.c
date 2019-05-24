/*
 * DHV-11 driver for Ninth Edition UNIX
 * by Andrew Hume (loosely based on dh driver from toronto)
 */

#include "dhv.h"
#if NDHV > 0
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

#define NPER	8		/* lines per board */
#define	NDHVLINE	(NDHV*NPER)

/* bits in dhv[]->state */
#define	ISOPEN	0x0002
#define	WOPEN	0x0004
#define	TIMEOUT	0x0008
#define	CARRIER	0x0010
#define	DHSTOP	0x0020
#define	HPCL	0x0040
#define	BRKING	0x0080
#define BUSY	0x0100
#define FLUSH	0x0200

#define SSPEED	B9600	/* reasonable default nowadays */
#define DHPRI	30

/* hardware structure */
struct dhvreg
{
	unsigned short csr;
	short rbuf;		/* coincides with txchar (not used) */
	unsigned short lpr;	/* line param */
	unsigned short lstat;	/* line status */
	unsigned short lctl;	/* line control */
	unsigned short addr1;
	unsigned short addr2;
	unsigned short cnt;
};

struct hilo	/* for utterly wretched appalling hardware */
{
	char low;
	char high;
};
#define	LOW(ptr)	((struct hilo *)&(ptr))->low
#define	HIGH(ptr)	((struct hilo *)&(ptr))->high

/* csr bits */
#define	TXACT		0x8000
#define	TXIE		0x4000
#define	DIAGFAIL	0x2000
#define	TXERR		0x1000
#define	TXLINE(csr)	(((csr)>>8)&0xF)
#define	RXAVAIL		0x0080
#define	RXIE		0x0040
#define	MRESET		0x0020
#define	POINT(r, l)	LOW((r)->csr) = (((l)&0xF) | RXIE)

/* rbuf */
#define	VALID		0x8000
#define	OERR		0x4000
#define	FERR		0x2000
#define	PERR		0x1000
#define	LINE(rbuf)	(((rbuf)>>8)&0xF)
#define	ISMODEM(rbuf)	(((rbuf)&0x7000)==0x7000)
#define	DCD		0x10	/* note that it is shifted left 8 bits in lstat */

/* lpr bits */
#define	BITS5		0x00
#define	BITS6		0x08
#define	BITS7		0x10
#define	BITS8		0x18
#define	PENAB		0x20
#define	EPARITY		0x40
#define	STOP2		0x80

/* lctl bits */
#define	TXABORT		0x0001
#define	RXENABLE	0x0004
#define	BREAK		0x0008
#define	MODEM		0x0100
#define	DTR		0x0200
#define	RTS		0x1000

/* addr2 */
#define	TXEN		0x8000
#define	TXSTART		0x0080

char dhvsp[16] =
{
	0, 0, 1, 2, 3, 4 ,0, 5, 6, 7, 8, 10, 11, 13, 14, 0
};


int	dhvprobe(), dhvattach(), dhvrint(), dhvtint();
struct	uba_device *dhvboard[NDHV];
u_short	dhvstd[] = { 0 };
struct	uba_driver dhvdriver =
	{ dhvprobe, 0, dhvattach, 0, dhvstd, "dhv", dhvboard };

/*
 * interface with stream i/o system
 */
int	dhvopen(), dhvclose(), dhvoput(), nodev();
static	struct qinit dhvrinit = { nodev, NULL, dhvopen, dhvclose, 0, 0 };
static	struct qinit dhvwinit = { dhvoput, NULL, dhvopen, dhvclose, 200, 100 };
struct	streamtab dhvinfo = { &dhvrinit, &dhvwinit };

/*
	all stream buffers are mapped into unibus/qbus space.
*/
#define UBACVT(x)	((0x03ffffL&blkubad) + ((long)(caddr_t)(x)-(long)(caddr_t)blkdata))
#define	UBLADDR(x)	(UBACVT(x)&0xffff)	/* low order 16 bits */
#define UBHADDR(x)	((UBACVT(x)>>16)&0x3f)	/* high order 6 bits */

/*
	One structure per line
*/
struct	dhv {
	short state;
	short flags;
	struct queue *rdq;
	char board;
	char line;
	char ispeed, ospeed;
	short lctl;
	struct block *oblock;
} dhv[NDHVLINE];

/*
 * misc private data
 */
extern long blkubad;
extern u_char blkdata[];
int	dhvoverrun;
int	dhvmiss;			/* chars lost due to q full */

/*ARGSUSED*/
dhvprobe(reg)
	caddr_t reg;
{
	register int br, cvec;		/* these are ``value-result'' */
	register struct dhvreg *regs = (struct dhvreg *)reg;
	int i, c;

#ifdef lint
	br = 0; cvec = br; br = cvec;
	dhvrint(0); dhvtint(0);
#endif
	regs->csr = MRESET;
	DELAY(1500000);		/* 1.5 seconds */
	if((regs->csr&MRESET) || (regs->csr&DIAGFAIL)){
		printf("dhv master reset fail csr=0x%x\n", regs->csr);
		return(0);
	}
	/* all's well after the reset; now allow the interrupt */
	regs->csr = RXIE;
	DELAY(1000);		/* fifo is nonempty so it won't take long */
	printf("dhv reset:");
	for(i = 0; i < 8; i++){
		c = regs->rbuf&0xFF;
		if((c == 0201) || (c == 0203)) continue;	/* fillers */
		if((c&0x80) || ((c&1) == 0)){
			printf("dhv reset silo: bad code 0%o\n", c);
			return(0);
		}
		printf(" rom 0x%x", c);
	}
	printf("\n");
	POINT(regs, 0);	/* init to a known state */
	return 1;
}

/*
	as throughout this driver, we assume uba zero
*/
dhvattach(ui)
	register struct uba_device *ui;
{
	register int i;
	register struct dhv *dhvp;

	for(i = 0, dhvp = &dhv[ui->ui_unit*NPER]; i < NPER; i++, dhvp++){
		dhvp->state = 0;
		dhvp->board = ui->ui_unit;
		dhvp->line  = i;
	}
	return 1;
}

/*
	use tsleep so user can interrupt without wrecking accounting
*/
dhvopen(q, d, flag)
	register struct queue *q;
	dev_t d;
	int flag;
{
	register int dev;
	register struct dhv *dhvp;

	dev = minor(d);
	if(dev >= NDHVLINE)
		return(0);
	dhvp = &dhv[dev];
	q->ptr = (caddr_t)dhvp;
	WR(q)->ptr = (caddr_t)dhvp;
	/*
		If this is first open, initialise tty state to default.
	 */
	if(((dhvp->state&ISOPEN)==0) || ((dhvp->state&CARRIER)==0)){
		register int s = spl5();

		dhvp->flags = ODDP|EPARITY;
		dhvp->ispeed = dhvp->ospeed = SSPEED;
		dhvp->lctl = MODEM|DTR|RTS|RXENABLE;
		dhvp->state = 0;
		dhvparam(dhvp);
		while(!(dhvp->state & CARRIER))
			if(tsleep((caddr_t)dhvp, DHPRI, 0) != TS_OK){
				splx(s);
				return(0);
			}
		dhvp->rdq = q;
		dhvp->oblock = 0;
		dhvp->state |= ISOPEN;
		splx(s);
	}
	return 1;
}

/*
 * Close a DHV11 line.
 */
/*ARGSUSED*/
dhvclose(q, dev)
	dev_t dev;
	register struct queue *q;
{
	register struct dhv *dhvp;
	register struct dhvreg *regs;
	int s = spl5();

	dhvp = (struct dhv *)q->ptr;
	if(dhvp->oblock){
		freeb(dhvp->oblock);
		dhvp->oblock = 0;
	}
	flushq(WR(q), 1);
	dhvp->rdq = NULL;
	regs = ((struct dhvreg *)dhvboard[dhvp->board]->ui_addr);
	POINT(regs, dhvp->line);
	regs->lctl = 0;
	dhvp->state = 0;
	splx(s);
}


/*
 * dhv11 write put routine
 */
dhvoput(q, bp)
	register struct queue *q;
	register struct block *bp;
{
	register struct dhv *dhvp = (struct dhv *)q->ptr;
	register union stmsg *sp;
	register int s;
	int delaytime;

	switch(bp->type)
	{
	case M_IOCTL:
		sp = (union stmsg *)bp->rptr;
		switch(sp->ioc0.com)
		{
		case TIOCSDEV:
			delaytime = 0;
			if(dhvp->ispeed != sp->ioc3.sb.ispeed)
				delaytime = 20;
			dhvp->ospeed = sp->ioc3.sb.ospeed;
			dhvp->ispeed = sp->ioc3.sb.ispeed;
			dhvp->flags = sp->ioc3.sb.flags;
			bp->type = M_IOCACK;
			bp->wptr = bp->rptr;
			qreply(q, bp);
			qpctl1(q, M_DELAY, delaytime);	/* wait a bit */
			qpctl(q, M_CTL);		/* means do dhvparam */
			dhvstart(dhvp);
			return;
		case TIOCGDEV:
			sp->ioc3.sb.ispeed = dhvp->ispeed;
			sp->ioc3.sb.ospeed = dhvp->ospeed;
			sp->ioc3.sb.flags = dhvp->flags;
			sp->ioc3.sb.flags &= (F8BIT | EVENP | ODDP);
			bp->type = M_IOCACK;
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
		dhvp->state |= DHSTOP;
		freeb(bp);
		dhvstop(dhvp);
		splx(s);
		return;
	case M_START:
		dhvp->state &= ~DHSTOP;
		dhvstart(dhvp);
		break;
	case M_FLUSH:
		flushq(q, 1);
		freeb(bp);
		return;
	case M_BREAK:
		qpctl1(q, M_DELAY, 10);
		putq(q, bp);
		qpctl1(q, M_DELAY, 10);
		dhvstart(dhvp);
		return;
	case M_HANGUP:
		dhvp->state &= ~DHSTOP;
	case M_DELAY:
	case M_DATA:
		putq(q, bp);
		dhvstart(dhvp);
		return;
	default:		/* not handled; just toss */
		break;
	}
	freeb(bp);
}

/*
	Set parameters from open or stty into the DH hardware
	registers.
*/
dhvparam(dhvp)
	register struct dhv *dhvp;
{
	register struct dhvreg *regs;
	register int lpar;
	register s;

	regs = (struct dhvreg *)dhvboard[dhvp->board]->ui_addr;
	if(dhvp->ospeed)
		dhvp->lctl |= (DTR|RTS);
	else
		dhvp->lctl &= ~(DTR|RTS);
	lpar = (dhvsp[dhvp->ospeed]<<12) | (dhvsp[dhvp->ispeed]<<8);
	if((dhvp->ospeed) == B134)
		lpar |= BITS6|PENAB;
	else if (dhvp->flags & F8BIT)
		lpar |= BITS8;
	else if(((s = dhvp->flags&(EVENP|ODDP)) == (EVENP|ODDP)) || (s == 0))
		lpar |= BITS8;
	else {
		lpar |= BITS7|PENAB;
		if(dhvp->flags&EVENP)
			lpar |= EPARITY;
	}
	if ((dhvp->ospeed) == B110)	/* 110 baud (ugh!): 2 stop bits */
		lpar |= STOP2;
	dhvp->state &= ~CARRIER;
	POINT(regs, dhvp->line);
	regs->lpr = lpar;
	regs->lctl = dhvp->lctl;
	if(regs->lstat&(DCD<<8))
		dhvp->state |= CARRIER;
}

/*
	DHV11 receiver interrupt.
*/
dhvrint(dev)
	dev_t dev;
{
	register struct block *bp;
	register struct dhvreg *regs;
	register struct dhv *dhvp;
	register int c;
	int hangup;

	if(dev >= NDHV) {
		printf("dhv%d: bad rd intr\n", dev);
		return;
	}
	regs = (struct dhvreg *)dhvboard[dev]->ui_addr;
	/*
		get chars from the silo for this line
	*/
	while((c = regs->rbuf) < 0) { /* char present */
		dhvp = &dhv[dev*NPER + LINE(c)];
		hangup = 0;
		if(ISMODEM(c)){
			dhvp->state &= ~CARRIER;
			if((c&DCD) == 0)
				hangup = 1;
			else {
				dhvp->state |= CARRIER;
				wakeup((caddr_t)dhvp);
				continue;
			}
			c &= ~(OERR|FERR|PERR);
		}
		if(c&OERR){
			++dhvoverrun;
			continue;
		}
		if(dhvp->rdq == NULL)
			continue;
		if(dhvp->rdq->next->flag & QFULL) {
			dhvmiss++;	/* you lose */
			continue;
		}
		if((bp = allocb(16)) == NULL){
			printf("rint: out of space\n");
			continue;	/* out of space - you lose */
		}
		if(hangup)
			bp->type = M_HANGUP;
		else if(c&FERR)		/* frame error == BREAK */
			bp->type = M_BREAK;
		else
			*bp->wptr++ = c;
		(*dhvp->rdq->next->qinfo->putp)(dhvp->rdq->next, bp);
	}
}


/*
	DHV11 transmitter interrupt. Dev is board number.
	Restart each line which used to be active but has
	terminated transmission since the last interrupt.
*/
dhvtint(dev)
	dev_t dev;
{
	register struct dhvreg *regs;
	register struct dhv *dhvp;
	register struct block *bp;
	register struct queue *q;
	dev_t unit;
	short csr;

	regs = (struct dhvreg *)dhvboard[dev]->ui_addr;
	while((csr = regs->csr) < 0){
		unit = TXLINE(csr);
		dhvp = &dhv[unit + dev*NPER];
		dhvp->state &= ~BUSY;
		POINT(regs, unit);
		if((csr&(TXACT|TXERR)) == (TXACT|TXERR)){	/* somebody goofed */
			printf("dhv%d: txerr csr=0x%x addr2=0x%x addr1=0x%x\n",
				dev, csr, regs->addr2, regs->addr1);
			regs->cnt = 0;	/* allow progress */
		}
		q = WR(dhvp->rdq);
		if(bp = dhvp->oblock){
			bp->rptr = bp->wptr - regs->cnt;
			if(bp->rptr == bp->wptr){
				dhvp->oblock = 0;
				freeb(bp);
			}
		}
		dhvstart(dhvp);
	}
}

dhvtimo(dhvp)
	register struct dhv *dhvp;
{
	register struct dhvreg *regs = (struct dhvreg *)dhvboard[dhvp->board]->ui_addr;

	if(dhvp->state&BRKING) {
		int s = spl5();
		POINT(regs, dhvp->line);
		regs->lctl &= ~BREAK;
		splx(s);
	}
	dhvp->state &= ~(TIMEOUT|BRKING);
	dhvstart(dhvp);
}

/*
	Start (restart) transmission on the given DH11 line.
*/
dhvstart(dhvp)
register struct dhv *dhvp;
{
	register struct dhvreg *regs =
		(struct dhvreg *)dhvboard[dhvp->board]->ui_addr;
	register int unit;
	register int s = spl5();
	register struct block *bp;
	caddr_t addr;
	short goo;

	unit = dhvp->line;
	if(dhvp->state & BUSY)
		goto done;
again:
	if(dhvp->state&(TIMEOUT|DHSTOP|BRKING) || (dhvp->rdq == NULL))
		goto done;
	if((bp = dhvp->oblock) == NULL){
		if((bp = getq(WR(dhvp->rdq))) == NULL)
			goto done;
	}
	switch(bp->type)
	{
	case M_DATA:
		if(bp->wptr > bp->rptr){	/* positive length transfer */
			LOW(regs->csr) = RXIE | (unit&0xF);
			while(regs->addr2&TXSTART)
				printf("dhv: start set\n");
			if(regs->lctl&TXABORT)
				regs->lctl &= ~TXABORT;
			HIGH(regs->csr) = TXIE>>8;
			regs->addr1 = UBLADDR(bp->rptr);
			regs->cnt = bp->wptr - bp->rptr;
			HIGH(regs->addr2) = TXEN>>8;
			LOW(regs->addr2) = UBHADDR(bp->rptr) | TXSTART;
			dhvp->state |= BUSY;
			dhvp->oblock = bp;
		} else {
			freeb(bp);
			dhvp->oblock = 0;
		}
		break;
	case M_BREAK:
		dhvp->state |= BRKING|TIMEOUT;
		timeout(dhvtimo, (caddr_t)dhvp, 15);	/* about 250 ms */
		freeb(bp);
		break;
	case M_DELAY:
		dhvp->state |= TIMEOUT;
		timeout(dhvtimo, (caddr_t)dhvp, (int)*bp->rptr + 6);
		freeb(bp);
		break;
	case M_HANGUP:
		dhvp->ispeed = dhvp->ospeed = 0;
		/* fall through */
	case M_CTL:
		freeb(bp);
		dhvparam(dhvp);
		goto again;

	}
done:
	splx(s);
}


/*
	Stop output on a line, e.g. for ^S/^Q or output flush (e.g. ^O).
*/
dhvstop(dhvp)
	register struct dhv *dhvp;
{
	register struct dhvreg *regs = 
		(struct dhvreg *)dhvboard[dhvp->board]->ui_addr;
	register int unit, s = spl5();

	if(dhvp->state & BUSY){
		/*
			just stop it, tint looks at the count
		*/
		POINT(regs, dhvp->line);
		regs->lctl |= TXABORT;
	}
	splx(s);
}

/*
	At software clock interrupt time or after a UNIBUS reset
	empty all the dhv silos.
*/
dhvtimer()
{
	register dev_t dhv;
	register int s = spl5();

	for(dhv = 0; dhv < NDHV; dhv++)
		dhvrint(dhv);
	splx(s);
}

/*
	reset driver after a unibus reset
	this code should work but i don't really know
	what it is supposed to do.
*/
dhvreset(uban)
	int uban;
{
	register int i;
	register struct dhv *dhvp;
	register int s = spl5();

	for(i = 0, dhvp = &dhv[uban*NPER]; i < NPER; i++, dhvp++){
		dhvparam(dhvp);
		dhvp->state &= ~BUSY;
		dhvstart(dhvp);
	}
	splx(s);
}

qpr(q)
	register struct queue *q;
{
	printf("Q@0x%x: flag=0%o cnt=%d ptr=0x%x f=0x%x l=0x%x\n", q, q->flag,
		q->count, q->ptr, q->first, q->last);
}
