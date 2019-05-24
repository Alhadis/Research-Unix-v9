/*
 *  ZS driver
 */
#include "zs.h"
#define	TRC(c)

#include "../h/param.h"
#include "../h/stream.h"
#include "../h/ttyio.h"
#include "../h/ttyld.h"
#include "../machine/pte.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/conf.h"

#include "../sundev/mbvar.h"
#include "../sundev/zsreg.h"
#include "../sundev/zscom.h"
#include "../sundev/zsvar.h"

#define	ZSWR1_INIT	(ZSWR1_SIE|ZSWR1_TIE|ZSWR1_RIE)
#define	ZS_ON	(ZSWR5_DTR|ZSWR5_RTS)
#define	ZS_OFF	0

#define	NZSLINE 	(NZS*2)
#define	SSPEED		B9600		/* std speed = 9600 baud */

#define	EXISTS	01
#define	ISOPEN	02
#define	WOPEN	04
#define	TIMEOUT	010
#define	CARR_ON	020
#define	ZSSTOP	040
#define	HPCL	0100
#define	BRKING	0200
#define	DIALOUT	0400	/* set when used as dialout with smart modems */

#define	ZSPRI	30

/*
 * Modem control commands.
 */
#define	DMSET		0
#define	DMBIS		1
#define	DMBIC		2
#define	DMGET		3

/*
 * One structure per line.
 * Includes communication between H/W level 6 interrupts
 * and software interrupts
 */
#define ZSIBUFSZ	256
struct	zsaline {
/* from v9 dz.c */
	short	state;
	short	flags;
	struct	block	*oblock;
	struct	queue	*rdq;
	char	speed;
/* from SUN zsasync.c */
	short	za_needsoft;		/* need for software interrupt */
	short	za_break;		/* break occurred */
	short	za_overrun;		/* overrun (either hw or sw) */
	short	za_ext;			/* modem status change */
	short	za_work;		/* work to do */
	u_char	za_rr0;			/* for break detection */
	u_char	za_ibuf[ZSIBUFSZ];	/* circular input buffer */
	short	za_iptr;		/* producing ptr for input */
	short	za_sptr;		/* consuming ptr for input */
/* additions */
	dev_t	za_dev;
	struct	zscom *za_addr;
} zsaline[NZSLINE];

extern int zssoftflags[NZSLINE];

int	zsopen(), zsclose(), zsoput(), nodev();

static	struct qinit zsrinit = { nodev, NULL, zsopen, zsclose, 0, 0 };
	struct qinit zswinit = { zsoput, NULL, zsopen, zsclose, 200, 100 };
struct	streamtab zsinfo = { &zsrinit, &zswinit };


#define	PCLK	(19660800/4)	/* basic clock rate for UARTs */
#define	ZSPEED(n)	ZSTimeConst(PCLK, n)
#define	NSPEED	16	/* max # of speeds */
u_short	zs_speeds[NSPEED] = {
	0,
	ZSPEED(50),
	ZSPEED(75),
	ZSPEED(110),
#ifdef lint
	ZSPEED(134),
#else
	ZSPEED(134.5),
#endif
	ZSPEED(150),
	ZSPEED(200),
	ZSPEED(300),
	ZSPEED(600),
	ZSPEED(1200),
	ZSPEED(1800),
	ZSPEED(2400),
	ZSPEED(4800),
	ZSPEED(9600),
	ZSPEED(19200),
	ZSPEED(38400),
};

int zsticks = 3;		/* polling frequency */

/*
 * The async zs protocol
 */
int	zsa_attach(), zsa_txint(), zsa_xsint(), zsa_rxint(), zsa_srint(),
	zsa_softint();

struct zsops zsops_async = {
	zsa_attach,
	zsa_txint,
	zsa_xsint,
	zsa_rxint,
	zsa_srint,
	zsa_softint,
};

zsa_attach(zs)
	register struct zscom *zs;
{
	register struct zsaline *za = &zsaline[zs->zs_unit];

	za->za_addr = zs;
	za->za_dev = zs->zs_unit;
	za->state = EXISTS;
}

/*
 * Get the current speed of the console and turn it into something
 * UNIX knows about - used to preserve console speed when UNIX comes up
 */
zsgetspeed(dev)
	dev_t dev;
{
	struct zscom *zs;
	register struct zsaline *za;
	int uspeed, zspeed;

	dev = minor(dev);
	if (dev >= NZSLINE)
		return(SSPEED);
	za = &zsaline[dev];
	zs = za->za_addr;
	zspeed = ZREAD(12);
	zspeed |= ZREAD(13) << 8;
	for (uspeed = 0; uspeed < NSPEED; uspeed++)
		if (zs_speeds[uspeed] == zspeed)
			return (uspeed);
	return (SSPEED);
}

/*ARGSUSED*/
zsopen(q, d, flag)
register struct queue *q;
{
	register dev;
	register struct zsaline *za;
	struct zscom *zs;
	int s;
	static int first = 1;
	int zspoll();
 
	if (q->ptr)			/* already attached */
		return(1);
	dev = minor(d);
	za = &zsaline[dev];
	if (dev >= NZSLINE || (za->state&EXISTS)==0)
		return(0);
	zs = za->za_addr;
	q->ptr = (caddr_t)za;
	WR(q)->ptr = (caddr_t)za;
	s = splzs();
	zs->zs_priv = (caddr_t)za;
	zsopinit(zs, &zsops_async);
	(void) splx(s);
	if (first) {
		first = 0;
		timeout(zspoll, (caddr_t)0, zsticks);
	}
	if ((za->state&ISOPEN)==0 || (za->state&CARR_ON)==0) {
		register s = spl5();
		/* clear any stale input */
		za->za_iptr = 0;
		za->za_sptr = 0;
		za->za_overrun = 0;
		zsmctl(zs, ZS_ON, DMSET);
		for (;;) {
			za->flags = F8BIT|ODDP|EVENP;
			if (zssoftflags[dev] & ZS_KBDMS)
				za->speed = zsgetspeed(dev);
			else
				za->speed = SSPEED;
			zsparam(za);
			if (za->state & CARR_ON)
				break;
			/* ignore carrier? */
			if (zssoftflags[dev] & 1) {
				za->state |= (DIALOUT | CARR_ON);
				break;
			}
			if (tsleep((caddr_t)za, ZSPRI, 0) != TS_OK) {
				wakeup((caddr_t)za);
				za->speed = 0;
				zsparam(za);
				splx(s);
				return(0);
			}
		}
		za->rdq = q;
		za->state |= ISOPEN;
		/*
		 * The 0 channel of the keyboard UART controls
		 * the keyboard, so push the keyboard line
		 * discipline on it automatically.
		 */
		if ((zssoftflags[dev] & ZS_KBDMS) && !(dev & 1))
			pushkeyld(WR(q->next), d);
		splx(s);
	}
	TRC('o');
	return(1);
}

zsclose(q, d)
register struct queue *q;
{
	register struct zsaline *za;
	register s;
	int s1;

	za = (struct zsaline *)q->ptr;
	s = spl5();
	s1 = splzs();
	if (za->oblock) {
		register struct block *bp = za->oblock;
		za->oblock = NULL;
		splx(s1);
		freeb(bp);
	}
	flushq(WR(q), 1);
	za->rdq = NULL;
	if (za->state&HPCL || (za->state&CARR_ON)==0) {
		za->speed = 0;
		zsparam(za);
	}
	za->state &= EXISTS;
	splx(s);
	TRC('c');
}

/*
 * ZS write put routine
 */
zsoput(q, bp)
register struct queue *q;
register struct block *bp;
{
	register struct zsaline *za = (struct zsaline *)q->ptr;
	register union stmsg *sp;
	register s;
	int s1;
	int delaytime;

	TRC('p');
	switch(bp->type) {

	case M_IOCTL:
		TRC('i');
		sp = (union stmsg *)bp->rptr;
		switch (sp->ioc0.com) {

		case TIOCSDEV:
			delaytime = 0;
			if (za->speed != sp->ioc3.sb.ispeed)
				delaytime = 20;
			za->speed = sp->ioc3.sb.ispeed;
			za->flags = sp->ioc3.sb.flags;
			bp->type = M_IOCACK;
			bp->wptr = bp->rptr;
			qreply(q, bp);
			qpctl1(q, M_DELAY, delaytime);	/* wait a bit */
			qpctl(q, M_CTL);		/* means do zsparam */
			zsstart(za);
			return;

		case TIOCGDEV:
			sp->ioc3.sb.ispeed =
			  sp->ioc3.sb.ospeed = za->speed;
			sp->ioc3.sb.flags = za->flags;
			bp->type = M_IOCACK;
			qreply(q, bp);
			return;

		case TIOCHPCL:
			za->state |= HPCL;
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
		za->state |= ZSSTOP;
		freeb(bp);
		s1 = splzs();
		if (bp = za->oblock) {
			za->oblock = NULL;
			splx(s1);
			putbq(q, bp);
		}
		splx(s);
		return;

	case M_START:
		za->state &= ~ZSSTOP;
		zsstart(za);
		break;

	case M_FLUSH:
		flushq(q, 1);
		freeb(bp);
		s = spl5();
		s1 = splzs();
		if (bp = za->oblock) {
			za->oblock = NULL;
			splx(s1);
			freeb(bp);
		}
		splx(s);
		return;

	case M_BREAK:
		qpctl1(q, M_DELAY, 10);
		putq(q, bp);
		qpctl1(q, M_DELAY, 10);
		zsstart(za);
		return;

	case M_HANGUP:
		za->state &= ~ZSSTOP;
	case M_DELAY:
	case M_DATA:
		putq(q, bp);
		TRC('d');
		zsstart(za);
		return;

	default:		/* not handled; just toss */
		break;
	}
	freeb(bp);
}
 
/*
 * set device parameters
 */
zsparam(za)
	register struct zsaline *za;
{
	register struct zscom *zs = za->za_addr;
	register int wr1, wr3, wr4, wr5, speed;
	int loops;
	int s;
	char c;
 
	if (za->speed == 0) {
		(void) zsmctl(zs, ZS_OFF, DMSET);	/* hang up line */
		return;
	}
	wr1 = ZSWR1_INIT;
	wr3 = ZSWR3_RX_ENABLE;
	wr4 = ZSWR4_X16_CLK;
	wr5 = (zs->zs_wreg[5] & (ZSWR5_RTS|ZSWR5_DTR)) | ZSWR5_TX_ENABLE;
	if (za->speed == B134) {	/* what a joke! */
		wr1 |= ZSWR1_PARITY_SPECIAL;
		wr3 |= ZSWR3_RX_6;
		wr4 |= ZSWR4_PARITY_ENABLE | ZSWR4_PARITY_EVEN;
		wr5 |= ZSWR5_TX_6;
	} else if (za->flags&(F8BIT)) {
		wr3 |= ZSWR3_RX_8;
		wr5 |= ZSWR5_TX_8;
	} else switch (za->flags & (EVENP|ODDP)) {
	case 0:
		wr3 |= ZSWR3_RX_8;
		wr5 |= ZSWR5_TX_8;
		break;

	case EVENP:
		wr1 |= ZSWR1_PARITY_SPECIAL;
		wr3 |= ZSWR3_RX_7;
		wr4 |= ZSWR4_PARITY_ENABLE + ZSWR4_PARITY_EVEN;
		wr5 |= ZSWR5_TX_7;
		break;

	case ODDP:
		wr1 |= ZSWR1_PARITY_SPECIAL;
		wr3 |= ZSWR3_RX_7;
		wr4 |= ZSWR4_PARITY_ENABLE;
		wr5 |= ZSWR5_TX_7;
		break;

	case EVENP|ODDP:
		wr3 |= ZSWR3_RX_7;
		wr4 |= ZSWR4_PARITY_ENABLE + ZSWR4_PARITY_EVEN;
		wr5 |= ZSWR5_TX_7;
		break;
	}
	if (za->speed == B110)
		wr4 |= ZSWR4_2_STOP;
	else if (za->speed == B134)
		wr4 |= ZSWR4_1_5_STOP;
	else
		wr4 |= ZSWR4_1_STOP;
	speed = zs->zs_wreg[12] + (zs->zs_wreg[13] << 8);
	if (wr1 != zs->zs_wreg[1] || wr3 != zs->zs_wreg[3] ||
	    wr4 != zs->zs_wreg[4] || wr5 != zs->zs_wreg[5] ||
	    speed != zs_speeds[za->speed&017]) {
		/* 
		 * Wait for that last damn character to get out the
		 * door.  At most 1000 loops of 100 usec each is worst
		 * case of 110 baud.  If something appears on the output
		 * queue then somebody higher up isn't synchronized
		 * and we give up.
		 */
		s = splzs();
		loops = 1000;
		while ((ZREAD(1) & ZSRR1_ALL_SENT) == 0 && --loops > 0) {
			(void) splx(s);
			DELAY(100);
			s = splzs();
		}
		ZWRITE(3, 0);	/* disable receiver while setting parameters */
		zs->zs_addr->zscc_control = ZSWR0_RESET_STATUS;
		zs->zs_addr->zscc_control = ZSWR0_RESET_ERRORS;
		c = zs->zs_addr->zscc_data; /* swallow junk */
		c = zs->zs_addr->zscc_data; /* swallow junk */
		c = zs->zs_addr->zscc_data; /* swallow junk */
		ZWRITE(1, wr1);
		ZWRITE(4, wr4);
		ZWRITE(3, wr3);
		ZWRITE(5, wr5);
		speed = zs_speeds[za->speed&017];
		ZWRITE(11, ZSWR11_TXCLK_BAUD + ZSWR11_RXCLK_BAUD);
		ZWRITE(14, ZSWR14_BAUD_FROM_PCLK);
		ZWRITE(12, speed);
		ZWRITE(13, speed >> 8);
		ZWRITE(14, ZSWR14_BAUD_ENA + ZSWR14_BAUD_FROM_PCLK);
		(void) splx(s);
	}
}
 
zstimo(za)
register struct zsaline *za;
{
	register struct zscom *zs = za->za_addr;
	int s;

	if (za->state&BRKING) {
		s =  splzs();
		ZBIC(5, ZSWR5_BREAK);
		splx(s);
	}
	za->state &= ~(TIMEOUT|BRKING);
	zsstart(za);
}

zsstart(za)
register struct zsaline *za;
{
	register struct zscom *zs = za->za_addr;
	register s = spl5();
	register struct block *bp;
	int s1;
 
	TRC('s');
again:
	if (za->state & (TIMEOUT|ZSSTOP|BRKING) || za->oblock) {
		TRC('t');
		goto out;
	}
	if (za->rdq == NULL)
		goto out;
	if ((bp = getq(WR(za->rdq))) == NULL) {
		TRC('n');
		goto out;
	}
	switch (bp->type) {

	case M_DATA:
		za->oblock = bp;
		if (zs->zs_addr->zscc_control & ZSRR0_TX_READY)
			zs->zs_addr->zscc_data = *bp->rptr++;
		break;

	case M_BREAK:
		s1 = splzs();
		ZBIS(5, ZSWR5_BREAK);
		splx(s1);
		za->state |= BRKING|TIMEOUT;
		timeout(zstimo, (caddr_t)za, 15);	/* about 250 ms */
		freeb(bp);
		break;

	case M_DELAY:
		za->state |= TIMEOUT;
		timeout(zstimo, (caddr_t)za, (int)*bp->rptr + 6);
		freeb(bp);
		break;

	case M_HANGUP:
		za->speed = 0;
	case M_CTL:
		freeb(bp);
		zsparam(za);
		goto again;

	}
out:
	splx(s);
}
 
zsmctl(zs, bits, how)
	register struct zscom *zs;
	int bits, how;
{
	register int mbits, s;

	s = splzs();
	mbits = zs->zs_wreg[5] & (ZSWR5_RTS|ZSWR5_DTR);
	zs->zs_addr->zscc_control = ZSWR0_RESET_STATUS;
	DELAY(2);
	mbits |= zs->zs_addr->zscc_control & (ZSRR0_CD|ZSRR0_CTS);
	switch (how) {
	case DMSET:
		mbits = bits;
		break;

	case DMBIS:
		mbits |= bits;
		break;

	case DMBIC:
		mbits &= ~bits;
		break;

	case DMGET:
		(void) splx(s);
		return (mbits);
	}
	zs->zs_wreg[5] &= ~(ZSWR5_RTS|ZSWR5_DTR);
	ZBIS(5, mbits & (ZSWR5_RTS|ZSWR5_DTR));
	(void) splx(s);
	return (mbits);
}

zsa_txint(zs)
	register struct zscom *zs;
{
	register struct zsaline *za = (struct zsaline *)zs->zs_priv;
	register struct zscc_device *zsaddr = zs->zs_addr;
	register struct block *bp;

	if ((bp = za->oblock) && bp->rptr < bp->wptr &&
	    (zsaddr->zscc_control & ZSRR0_TX_READY)) {
		zsaddr->zscc_data = *bp->rptr++;
	} else {
		za->za_work++;
		zsaddr->zscc_control = ZSWR0_RESET_TXINT;
		ZSSETSOFT(zs);
	}
}

zsa_xsint(zs)
	register struct zscom *zs;
{
	register struct zsaline *za = (struct zsaline *)zs->zs_priv;
	register struct zscc_device *zsaddr = zs->zs_addr;
	register u_char s0, x0, c;

	s0 = zsaddr->zscc_control;
	x0 = s0 ^ za->za_rr0;
	za->za_rr0 = s0;
	zsaddr->zscc_control = ZSWR0_RESET_STATUS;
	if ((x0 & ZSRR0_BREAK) && (s0 & ZSRR0_BREAK) == 0) {
		za->za_break++;
		c = zsaddr->zscc_data; /* swallow null */
#ifdef lint
		c = c;
#endif
		zsaddr->zscc_control = ZSWR0_RESET_ERRORS;
/*
		if (za->za_dev == kbddev)
			montrap(*romp->v_abortent);
*/
	}
	za->za_work++;
	za->za_ext++;
	ZSSETSOFT(zs);
}

zsa_rxint(zs)
	register struct zscom *zs;
{
	register struct zsaline *za = (struct zsaline *)zs->zs_priv;
	register struct zscc_device *zsaddr = zs->zs_addr;
	register u_char c;

	c = zsaddr->zscc_data;
	if (c == 0 && (za->za_rr0 & ZSRR0_BREAK))
		return;
	za->za_ibuf[za->za_iptr++] = c;
	if (za->za_iptr >= ZSIBUFSZ)
		za->za_iptr = 0;
	if (za->za_iptr == za->za_sptr)
		za->za_overrun++;
	za->za_work++;
	if (++za->za_needsoft > 20) {
		za->za_needsoft = 0;
		ZSSETSOFT(zs);
	}
}

zsa_srint(zs)
	register struct zscom *zs;
{
	register struct zsaline *za = (struct zsaline *)zs->zs_priv;
	register struct zscc_device *zsaddr = zs->zs_addr;
	register short s1;
	register u_char c;

	s1 = ZREAD(1);
	c = zsaddr->zscc_data;	/* swallow bad char */
#ifdef lint
	c = c;
#endif
	zsaddr->zscc_control = ZSWR0_RESET_ERRORS;
	if (s1 & ZSRR1_DO) {
		za->za_overrun++;
		za->za_work++;
		ZSSETSOFT(zs);
	}
}
/*
 * Handle a software interrupt 
 */
zsa_softint(zs)
	register struct zscom *zs;
{
	register struct zsaline *za = (struct zsaline *)zs->zs_priv;

	if (zsa_process(za))	/* true if too much work at once */
		zspoll(1);
	return (0);
}

/*
 * Poll for events in the zscom structures
 * This routine is called at level 1, we jack up to 3 to lock
 * out zsa_softint.
 */
zspoll(direct)
{
	register struct zsaline *za;
	register short more;
	register int s;

	do {
		more = 0;
		for (za = &zsaline[0]; za < &zsaline[NZSLINE]; za++)
		if (za->za_work) {
			za->za_work = 0;
			s = spl3();
			if (zsa_process(za)) {
				za->za_work++;
				more++;
			}
			(void) splx(s);
		}
	} while (more);
	if (!direct)
		timeout(zspoll, (caddr_t)0, zsticks);
}

/* 
 * Process software interrupts (or poll)
 * Crucial points:
 * 1.  Inner loop gives equal priority to input and output so that
 *     in TANDEM mode the stop character has a chance of being sent
 *     before enough input arrives to exceed TTYHOG.  This has happened
 *     in very busy systems.
 * 2.  The inner loop is executed at most 20 times before the next line
 *     is serviced -- this "schedules" more fairly among lines.
 * 3.  BUG - breaks are handled "out-of-band" - their relative position
 *     among input events is lost, as well as multiple breaks together.
 *     This is probably not a problem in practice.
 */
zsa_process(za)
	register struct zsaline *za;
{
	register struct zscc_device *zsaddr = za->za_addr->zs_addr;
	register struct block *bp;
	register short i;
	register u_char c;

	if (za->za_ext) {
		za->za_ext = 0;
		/* carrier up? */
		if (zsaddr->zscc_control & ZSRR0_CD) {
			/* carrier present */
			if ((za->state & CARR_ON)==0)
				wakeup((caddr_t)za);
			za->state |= CARR_ON;
		} else if ((za->state & CARR_ON) && !(za->state & DIALOUT)) {
			/* carrier lost */
			if (za->state & ISOPEN) {
				(void) zsmctl(za->za_addr, ZSWR5_DTR, DMBIC);
				if (za->rdq)
					putctl(za->rdq->next,M_HANGUP);
			}
			za->state &= ~CARR_ON;
		}
	}
	if (za->za_overrun) {
		za->za_overrun = 0;
		za->za_iptr = 0;
		za->za_sptr = 0;
		if (za->state & ISOPEN)
			printf("zs%d: silo overflow\n", minor(za->za_dev));
	}
	if (za->za_break && (zsaddr->zscc_control & ZSRR0_BREAK) == 0) {
		za->za_break = 0;
		if (za->rdq != NULL && !(za->rdq->next->flag&QFULL) &&
		   (bp = allocb(16)) != NULL) {
			bp->type = M_BREAK;
			(*za->rdq->next->qinfo->putp)(za->rdq->next, bp);
		}
	}
	/* need to handle I & O in same loop to make TANDEM mode work */
	i = 0;
	do {
		if (za->za_sptr != za->za_iptr) {
			c = za->za_ibuf[za->za_sptr++];
			if (za->za_sptr >= ZSIBUFSZ)
				za->za_sptr = 0;
			if (za->rdq != NULL && !(za->rdq->next->flag&QFULL) &&
			   (bp = allocb(16)) != NULL) {
			      *bp->wptr++ = c;
			      (*za->rdq->next->qinfo->putp)(za->rdq->next, bp);
			}
		}
		if ((bp = za->oblock) && bp->rptr >= bp->wptr) {
			freeb(bp);
			za->oblock = NULL;
			zsstart(za);
		}
	} while (za->za_sptr != za->za_iptr && ++i < 20);
	return (i >= 20);
}

/*
 * push the keyboard line discipline on the line
 * function equivalent of FIOPUSHLD, but occurs automatically on open
 */
#include "kbd.h"
#if NKBD > 0
    extern struct streamtab kbdinfo;
#else
#   define kbdinfo	*(struct streamtab *)NULL
#endif

pushkeyld(q, d)
register struct queue *q;
{
	if (&kbdinfo == (struct streamtab *)NULL)
		return;
	if (qattach(&kbdinfo, RD(q), d)) {
		long nip =
		  (*q->next->qinfo->qopen)(RD(q->next), d);
		if (nip ==0)
			qdetach(RD(q->next), 0);
	}
}
