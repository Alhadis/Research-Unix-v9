/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */
#include "le.h"

/*
 * AMD Am7990 LANCE Ethernet Controller driver
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../machine/pte.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/vmmac.h"
#include "../h/conf.h"
#include "../h/ttyio.h"
#include "../h/enio.h"
#include "../h/ttyld.h"
#include "../h/stream.h"
#include "../h/ethernet.h"

#include "../sundev/mbvar.h"
#include "../sundev/lereg.h"

#define ETHERMAXP	(1500 + sizeof(struct etherpup))

/* corresponds to minor device numbers (# of servers) */
#define	CHANS_PER_UNIT	8

int	leprobe(), leattach(), leintr();
struct	mb_device *leinfo[NLE];
struct	mb_driver ledriver = {
	leprobe, 0, leattach, 0, 0, leintr,
	sizeof (struct le_device), "le", leinfo, 0, 0, 0,
};

/*
 * Transmit and receive buffer layout.
*	The buffer size is chosen to give room for the maximum ether
 *	transmission unit, an overrun consisting of the entire fifo
 *	contents, and slop that experience indicates is necessary.
 *	(The exact amount of slop required is still unknown.)
 */
struct lebuf {
	char		buffer[MAXBUF];	/* Packet's data */
};

struct	leu {
	struct	le_device *dev;		/* hardware pointer */
	struct	le_init_block *ib;	/* Initialization block */
	struct	le_md *rdrp;		/* Receive Descriptor Ring Ptr */
	struct	le_md *tdrp;		/* Transmit Descriptor Ring Ptr */
	struct	le_md *rdrend;		/* Receive Descriptor Ring End */
	struct	le_md *his_rmd;		/* Next descriptor in ring */
	int	nrmdp2;			/* log(2) Num. Rcv. Msg. Descs. */
	int	ntmdp2;			/* log(2) Num. Tran. Msg. Descs. */
	int	nrmds;			/* Num. Rcv. Msg. Descs. */
	int	ntmds;			/* Num. Xmit. Msg. Descs. */
	struct	lebuf *rbufs;		/* Receive Buffers */
	struct	lebuf *tbufs;		/* Transmit Buffers */
	/* Error counters */
	int	frame;			/* Receive Framing Errors (dribble) */
	int	crc;			/* Receive CRC Errors */
	int	oflo;			/* Receive overruns */
	int	uflo;			/* Transmit underruns */
	int	retries;		/* Transmit retries */
	int	missed;			/* Number of missed packets */
	int	noheartbeat;		/* Number of nonexistent heartbeats */
	int	ierrors, oerrors;
	int	ipackets, opackets;
	char	myetheradr[6];
	char	attached;
	char	active;
} leu[NLE];

#define	NLECHAN	(CHANS_PER_UNIT * NLE)
struct	lechan {
	struct	leu *leu;
	int	packets;
	struct	queue *q;
	int	type;
} lechan[NLECHAN];

int	nodev(), leopen(), leclose(), leput();
static struct qinit lerinit = { nodev, NULL, leopen, leclose, 0, 0 };
static struct qinit lewinit = { leput, NULL, leopen, leclose, 1514, 0 };
	/* 1514 bytes is minimum highwater mark to send 1514 byte packets */
struct streamtab lesinfo = { &lerinit, &lewinit };

/*
 * Resource amounts.
 */
/* Numbers of ring descriptors */
int le_ntmdp2 = 0;		/* 2 ^ 0 = 1 */
int le_nrmdp2 = 4;		/* 2 ^ 4 = 16 */

/*
 * Return the address of an adjacent descriptor in the given ring.
 */
#define next_rmd(lu,rmdp)	((rmdp) == (lu)->rdrend		\
					? (lu)->rdrp : ((rmdp) + 1))

/*
 * Probe for device.
 */
leprobe(reg)
	caddr_t reg;
{
	register struct le_device *le = (struct le_device *)reg;

	if (pokec((char *)&le->le_rdp, 0))
		return (0);
	return (sizeof (struct le_device));
}

/*
 * Interface exists: make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
leattach(md)
	struct mb_device *md;
{
	struct leu *lu = &leu[md->md_unit];

	/* Reset the chip. */
	lu->dev = (struct le_device *)md->md_addr;
	lu->dev->le_rap = LE_CSR0;
	lu->dev->le_csr = LE_STOP;
	localetheraddr(NULL, lu->myetheradr);
	le_alloc_buffers(lu);
	lu->attached = 1;
}

/* ARGSUSED */
leopen(q, dev)
register struct queue *q;
register dev_t dev;
{
	register struct lechan *led;
	register struct leu *lu;
	register unit;

	dev = minor(dev);
	unit = dev / CHANS_PER_UNIT;

	if (dev >= NLECHAN)
		return(0);
	if (unit >= NLE)
		return(0);

	lu = &leu[unit];
	if(!lu->attached)
		return(0);
	led = &lechan[dev];
	if (led->q)
		return(0);
	led->leu = lu;
	led->q = q;
	q->ptr = (caddr_t)led;
	q->flag |= QBIGB | QDELIM;
	WR(q)->ptr = (caddr_t)led;
	WR(q)->flag |= QBIGB|QDELIM;
	return(1);
}

leclose(q)
register struct queue *q;
{
	register struct lechan *led;

	led = (struct lechan *)q->ptr;
	flushq(WR(q), 1);
	lechan->q = NULL;
}

leput(q, bp)
register struct queue *q;
register struct block *bp;
{
	register struct lechan *led;
	register struct leu *lu;
	register s;

	led = (struct lechan *)q->ptr;
	lu = led->leu;

	switch(bp->type) {
	
		case M_IOCTL:
			leioctl(q, bp);
			return;

		case M_DATA:
			putq(q, bp);
			return;
		
		case M_DELIM:
			break;

		default:
			freeb(bp);
			return;
	}

	putq(q, bp);
	s = splimp();
	led->packets++;
	if (lu->active == 0)
		lestart(lu);
	splx(s);
}

/*
 * Grab memory for message descriptors and buffers and set fields
 * in the interfaces's software status structure accordingly.
 */
le_alloc_buffers(lu)
register struct leu *lu;
{
	int size;
	int memall();
	caddr_t wmemall(), v;

	/* Set numbers of message descriptors. */
	lu->nrmdp2 = le_nrmdp2;
	lu->ntmdp2 = le_ntmdp2;
	lu->nrmds = 1 << lu->nrmdp2;
	lu->ntmds = 1 << lu->ntmdp2;

	size = sizeof(struct le_md) * (lu->nrmds + lu->ntmds) +
		sizeof(struct lebuf) * (lu->nrmds + lu->ntmds) +
		sizeof(struct le_init_block);
	v = wmemall(memall, size);

#define	valloc(name, type, num) \
	    (name) = (type *)(v); (v) = (caddr_t)((name)+(num))

	valloc(lu->rdrp, struct le_md , lu->nrmds);
	valloc(lu->tdrp, struct le_md , lu->ntmds);
	valloc(lu->ib, struct le_init_block , 1);
	valloc(lu->rbufs, struct lebuf , lu->nrmds);
	valloc(lu->tbufs, struct lebuf , lu->ntmds);

	/*
	 * Remember address of last descriptor in the ring for
	 * ease of bumping pointers around the ring.
	 */
	lu->rdrend = &((lu->rdrp)[lu->nrmds-1]);
	leinit(lu);
}

/*
 * Initialization of interface; clear recorded pending
 * operations.
 */
leinit(lu)
register struct leu *lu;
{
	register struct le_device *le;
	register struct le_init_block *ib;
	register int s;
	register struct lebuf	*bp;
	register struct le_md *md;
	int i;

	s = splimp();

	le = lu->dev;
	le->le_csr = LE_STOP;		/* reset the chip */

	/*
	 * Reset message descriptors.
	 */
	lu->his_rmd = lu->rdrp;

	/* Construct the initialization block */
	ib = lu->ib;
	bzero((caddr_t)ib, sizeof (struct le_init_block));

	ib->ib_padr[0] = lu->myetheradr[1];
	ib->ib_padr[1] = lu->myetheradr[0];
	ib->ib_padr[2] = lu->myetheradr[3];
	ib->ib_padr[3] = lu->myetheradr[2];
	ib->ib_padr[4] = lu->myetheradr[5];
	ib->ib_padr[5] = lu->myetheradr[4];
						
	/* No multicast filter yet, FIXME MULTICAST, leave zeros. */

	ib->ib_rdrp.drp_laddr = (long)lu->rdrp;
	ib->ib_rdrp.drp_haddr = (long)lu->rdrp >> 16;
	ib->ib_rdrp.drp_len   = (long)lu->nrmdp2;
	ib->ib_tdrp.drp_laddr = (long)lu->tdrp;
	ib->ib_tdrp.drp_haddr = (long)lu->tdrp >> 16;
	ib->ib_tdrp.drp_len   = (long)lu->ntmdp2;

	/* Clear all the descriptors */
	bzero((caddr_t)lu->rdrp, lu->nrmds * sizeof (struct le_md));
	bzero((caddr_t)lu->tdrp, lu->ntmds * sizeof (struct le_md));

	/* Hang out the receive buffers. */
	for(i = 0; i < lu->nrmds; i++) {
		bp = &lu->rbufs[i];
		md = &lu->rdrp[i];
		md->lmd_ladr = (u_short) bp;
		md->lmd_hadr = (long)bp >> 16;
		md->lmd_bcnt = -MAXBUF;
		md->lmd_mcnt = 0;
		md->lmd_flags = LMD_OWN;
	}
	lu->his_rmd == lu->rdrp;

	/* Transmit md intialization */
	for(i = 0; i < lu->ntmds; i++) {
		bp = &lu->tbufs[i];
		md = &lu->tdrp[i];
		md->lmd_ladr = (u_short) bp;
		md->lmd_hadr = (long)bp >> 16;
	}

	/* Give the init block to the chip */
	le->le_rap = LE_CSR1;	/* select the low address register */
	le->le_rdp = (long)ib & 0xffff;

	le->le_rap = LE_CSR2;	/* select the high address register */
	le->le_rdp = ((long)ib >> 16) & 0xff;

	le->le_rap = LE_CSR3;	/* Bus Master control register */
	le->le_rdp = LE_BSWP;

	le->le_rap = LE_CSR0;	/* main control/status register */
	le->le_csr = LE_INIT;

	for (i = 10000; ! (le->le_csr & LE_IDON); i-- )
		if (i <= 0)
			panic("le: chip didn't initialize");
	le->le_csr = LE_IDON;		/* Now reset the interrupt */
	lu->active = 0;
	/* (Re)start the chip. */
	le->le_csr = LE_STRT | LE_INEA;
	(void) splx(s);
}

/*
 * Start or restart output on interface.
 * If interface is already active, then this is a nop.
 * If interface is not already active, get another packet
 * to send from the interface queue, and map it to the
 * interface before starting the output.
 */
lestart(lu)
register struct leu *lu;
{
	register char *to;
	register struct lechan *led;
	register struct queue *q;
	register struct block *bp;
	register count, i;
	struct le_device *le;
	register struct le_md *t;

	led = &lechan[(lu - leu) * CHANS_PER_UNIT];
	for(i = 0; i < CHANS_PER_UNIT; i++, led++)
		if (led->q && led->packets > 0)
			break;
	if (i >= CHANS_PER_UNIT)
		return;

	led->packets--;
	q = WR(led->q);

	le = (struct le_device *)lu->dev;
	to = lu->tbufs->buffer;
	count = 0;
	while (bp = getq(q)) {
		if (bp->type == M_DELIM) {
			freeb(bp);
			break;
		}
		i = bp->wptr - bp->rptr;
		/*
		 * If larger than maximum packet, throw out extra
		 */
		if (count + i > ETHERMAXP) {
			i = ETHERMAXP - count;
			bcopy(bp->rptr, to, i);
			count += i;
			freeb(bp);
			while (bp = getq(q)) {
				if (bp->type == M_DELIM) {
					freeb(bp);
					break;
				}
				freeb(bp);
			}
			break;
		}
		bcopy(bp->rptr, to, i);
		to += i;
		count += i;
		freeb(bp);
	}

	/* if there is no ethernet header in packet, throw it out */
	if (count < sizeof(struct etherpup))
		return;
	if (count < 60)
		count = 60;

	to = lu->tbufs->buffer + 6;
	bcopy(lu->myetheradr, to, 6);
	t = lu->tdrp;
	if (t->lmd_flags & LMD_OWN)
		panic("lestart: tmd ownership conflict");
	t->lmd_bcnt = -count;
	t->lmd_flags3 = 0; 
	t->lmd_flags = 0;
	t->lmd_flags = LMD_STP|LMD_ENP;
	t->lmd_flags |= LMD_OWN;
	lu->active = 1;
	le->le_csr = LE_TDMD | LE_INEA;

}

/*
 * interrupt routine
 */
leintr()
{
	register struct leu *lu;
	register struct le_device *le;
	register struct le_md *lmd;
	register struct mb_device *md;
	int serviced = 0;

	lu = leu;
	for (lu = leu; lu < &leu[NLE]; lu++) {
		if (!lu->attached)
			continue;
		le = lu->dev;
		if (!(le->le_csr & LE_INTR))
			continue;

		/* Keep statistics for lack of heartbeat */
		if (le->le_csr & LE_CERR) {
			le->le_csr = LE_CERR | LE_INEA;
			lu->noheartbeat++;
		}

		/* Check for receive activity */
		if ( (le->le_csr & LE_RINT) && (le->le_csr & LE_RXON) ) {
			/* Pull packets off interface */
			for (lmd = lu->his_rmd;
			     !(lmd->lmd_flags & LMD_OWN);
			     lu->his_rmd = lmd = next_rmd(lu, lmd)) {
				serviced = 1;
				le->le_csr = LE_RINT | LE_INEA;
				leread(lu, lmd);
				lmd->lmd_mcnt = 0;
				lmd->lmd_flags = LMD_OWN;
			}
			if (!serviced)
				panic("RINT with buffer owned by chip");
		}

		/* Check for transmit activity */
		if ((le->le_csr & LE_TINT) && (le->le_csr & LE_TXON)) {
			lmd = lu->tdrp;
			if (lmd->lmd_flags & (TMD_MORE | TMD_ONE))
				lu->retries++;
			if (lmd->lmd_flags3 &
			    (TMD_BUFF|TMD_UFLO|TMD_LCOL|TMD_LCAR|TMD_RTRY))
				le_xmit_error(lu, lmd->lmd_flags3, le);
			le->le_csr = LE_TINT | LE_INEA;
			lu->active = 0;
			lu->opackets++;
			lestart(lu);
			serviced = 1;
		}

		/*
		 * Check for errors not specifically related
		 * to transmission or reception.
		 */
		if ( (le->le_csr & (LE_BABL|LE_MERR|LE_MISS|LE_TXON|LE_RXON))
		     != (LE_RXON|LE_TXON) ) {
			serviced = 1;
			le_chip_error(lu, le);
		}
	}
	return (serviced);
}

/*
 * Move info from driver toward protocol interface
 */
leread(lu, rmd)
register struct leu *lu;
register struct le_md *rmd;
{
	register struct lechan *led;
	register struct queue *q;
	register struct block *bp;
	register short type;
	register caddr_t from;
	register len, i;

	/* Check for packet errors. */
	if ((rmd->lmd_flags & ~RMD_OFLO) != (LMD_STP|LMD_ENP)) {
		le_rcv_error(lu, rmd->lmd_flags);
		lu->ierrors++;
		return;
	}

	/*
	 * Get input data length (minus crc)
	 */
	len = rmd->lmd_mcnt - 4;	/* subtract off trailing crc */
	if (len == 0) {
		printf("runt packet\n");
		lu->ierrors++;
		return;
	}

	from = (caddr_t)lu->rbufs[rmd - lu->rdrp].buffer;
	type = ((struct etherpup *)from)->type;
	led = &lechan[(lu - leu) * CHANS_PER_UNIT];
	for (i = 0; i < CHANS_PER_UNIT; i++, led++)
		if (led->q && led->type == type)
			break;
	if (i >= CHANS_PER_UNIT)
		return;
	q = led->q;
	if (q->next->flag & QFULL) {
		lu->ierrors++;
		return;
	}
	while (len > 0) {
		bp = allocb(len);
		i = MIN((bp->lim - bp->base), len);
		len -= i;
		bcopy(from, bp->wptr, i);
		bp->wptr += i;
		from += i;
		(*q->next->qinfo->putp)(q->next, bp);
	}
	if (putctl(q->next, M_DELIM))
		lu->ipackets++;
	else 
		printf("leread no DELIM bp\n");
}

/*
 * Process an ioctl request.
 */
leioctl(q, bp)
register struct queue *q;
register struct block *bp;
{
	register struct lechan *led;
	register struct leu *lu;
	register u_char	*msg;
	int cmd;
	
	led = (struct lechan *)q->ptr;
	lu = led->leu;
	cmd = ((union stmsg *)(bp->rptr))->ioc1.com;
	msg = (u_char *)&((union stmsg *)(bp->rptr))->ioc1.sb;

	bp->type = M_IOCACK;
	switch (cmd) {
		case ENIOADDR:          /* get my Ethernet address */
			bcopy(lu->myetheradr, (int *)msg, 6);
			break;

		case ENIOTYPE:
			led->type = *(int *)msg;
			break;
			
		default:
			bp->type = M_IOCNAK;
			break;
	}
	qreply(q, bp);
}

le_rcv_error(lu, flags)
	struct leu *lu;
	u_char flags;
{
	if (flags & RMD_FRAM)
		lu->frame++;
	if (flags & RMD_CRC )
		lu->crc++;
	if (flags & RMD_OFLO)
		lu->oflo++;
	if (flags & RMD_BUFF)
		printf("Receive buffer error - BUFF bit set in rmd\n");
	if (!(flags & LMD_STP))
		printf("Received packet with STP bit in rmd cleared\n");
	if (!(flags & LMD_ENP))
		printf("Received packet with ENP bit in rmd cleared\n");
}

le_xmit_error(lu, flags, le)
	struct leu *lu;
	u_short flags;
	struct le_device *le;
{
	/*
	 * The BUFF bit isn't valid if the RTRY bit is set.
	 */
	if ((flags & (TMD_BUFF | TMD_RTRY)) == TMD_BUFF)
		printf("Transmit buffer error - BUFF bit set in tmd\n");
	if (flags & TMD_UFLO) {
		printf("Transmit underflow error\n");
		lu->uflo++;
	}
	if (flags & TMD_LCOL)
		printf("Transmit late collision - net problem?\n");
	if (flags & TMD_LCAR)
		printf("No carrier - transceiver cable problem?\n");
	if (flags & TMD_RTRY)
		printf("Transmit retried more than 16 times - net jammed\n");
}

/* Handles errors that are reported in the chip's status register */
/* ARGSUSED */
le_chip_error(lu, le)
	struct leu *lu;
	struct le_device *le;
{
	register u_short	csr = le->le_csr;
	int restart = 0;

	if (csr & LE_MISS) {
		lu->missed++;
		le->le_csr = LE_MISS | LE_INEA;
	}

	if (csr & LE_BABL) {
	    printf("Babble error - sent a packet longer than the maximum length\n");
	    le->le_csr = LE_BABL | LE_INEA;
	}
	/*
	 * If a memory error has occurred, both the transmitter
	 * and the receiver will have shut down.
	 */
	if (csr & LE_MERR) {
	    printf("Memory Error!  Ethernet chip memory access timed out\n");
	    le->le_csr = LE_MERR | LE_INEA;
	}
	if ( !(csr & LE_RXON) ) {
	    printf("Reception stopped\n");
	    restart++;
	}
	if ( !(csr & LE_TXON) ) {
	    printf("Transmission stopped\n");
	    restart++;
	}
	if (restart) {
	    le_print_csr(csr);
	    leinit(lu);
	}
}

/*
 * Print out a csr value in a nicely formatted way.
 */
le_print_csr (csr)
	register u_short	csr;
{
	printf("csr: %b\n", csr,
"\20\20ERR\17BABL\16CERR\15MISS\14MERR\13RINT\12TINT\11IDON\10INTR\7INEA\6RXON\5TXON\4TDMD\3STOP\2STRT\1INIT\n");
}
