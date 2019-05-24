/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "ie.h"
#if NIE > 0
/*
 * Sun Intel Ethernet Controller interface
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
#include "../sundev/iereg.h"
#include "../sundev/iem.h"
#include "../sundev/ieob.h"

/* controller types */
#define IE_MB	1	/* Multibus */
#define IE_OB	2	/* Onboard I/O */
 
#define	IEOBMEMDFT	(40*1024)	/* default memory allocated for obie */
#define CBCACHESIZ	(80)		/* size of control block cache */
#define OVFLWARNMASK	(3)		/* overflow count mask before warning */
#define NPOOL_RBD	50
#define	NTRANSBUF	4		/* NUmber of transmit buffers */

int	ieprobe(), ieattach(), iepoll();
struct	mb_device *ieinfo[NIE];
struct	mb_driver iedriver = {
	ieprobe, 0, ieattach, 0, 0, iepoll,
	/* take the larger of the two devices */
	sizeof (struct mie_device), "ie", ieinfo,
};

int iebark(), iedog();
caddr_t from_ieaddr();
ieoff_t to_ieoff();
caddr_t from_ieoff();
ieaddr_t to_ieaddr();
long  escbget(), escbput();
ieint_t to_ieint();
#define	from_ieint	to_ieint

#define iepages(x)	(((x) + IEPAGSIZ-1)>>IEPAGSHIFT)

int iecbcsmall;
int iecbclarge;

/*
 * Ethernet software status per interface.
 */
struct	ie_softc {
/* Added by D.A.K for version 9 compatability */
	char myetheradr[6];
	char	attached;	/* Non zero when ie has been attached */
	int	state;
	int	collisions;
	int	ierrors, oerrors;
	int	ipackets, opackets;
	struct ietfd *es_tfd[NTRANSBUF];	/* transmit TFD */
	struct ietbd *es_tbd[NTRANSBUF];	/* transmit TBD */
	caddr_t	es_tbuffer[NTRANSBUF];		/* transmit buffer */
	int	es_tfree;			/* transmitter bit map */
/* Supplied by SUN */
	struct mie_device *es_mie;	/* Multibus board registers */
	struct obie_device *es_obie;	/* On-board registers */
	caddr_t	es_memspace;		/* + chip addr = kernel addr */
	int	es_paddr;		/* starting page number for board */
	int	es_vmemsize;		/* size of multibus mem port */
	caddr_t	es_base;		/* our addr of control block base */
	struct map *es_cbmap;		/* rmap for control blocks */
	struct map *es_memmap;		/* rmap for memory */
	struct map *es_pgmap;		/* rmap for 1K page (ND) memory
					   accessed thru if_memmap */
	struct iescb *es_scb;		/* SCB ptr */
	struct iescp *es_scp;		/* SCP ptr */
	struct iecb *es_cbhead;		/* CBL head */
	struct iecb *es_cbtail;		/* CBL tail */
	struct ierfd *es_rfdhead;	/* head of RFD list */
	struct ierfd *es_rfdtail;	/* tail of RFD list */
	struct ierbd *es_rbdhead;	/* head of RBD list */
	struct ierbd *es_rbdtail;	/* tail of RBD list */
	struct ieipack *es_iepavail;	/* standby input packets */
	time_t	es_latest;		/* latest packet arrival time */
	int	es_obmem;		/* total IE_OB main memory */
	char	es_type;		/* type of controller */
	char	es_simple;		/* doing simple flag */
	u_int	es_runotready;		/* RU was not ready counter */
	u_int	es_xmiturun;		/* xmit DMA underrun counter */
	u_int	es_dogreset;		/* iedog reset counter */
	u_int	es_ieheart;		/* heartbeat counter */
	u_int	es_iedefer;		/* deferred transmission counter */
	struct ieierr {
		u_int	crc;
		u_int	aln;
		u_int	rsc;
		u_int	ovr;
	}	 es_ierr;		/* input error counters */
	int	es_cbsize;		/* control block area size */
	int	es_cbc_lo;		/* small block cache pointer */
	int	es_cbc_hi;		/* large block cache pointer */
	u_int	es_cbc_ovfl;		/* overflow counter */
	int	es_cbcbusy;		/* cache is loaded */
	long	es_cbcache[CBCACHESIZ];	/* control block cache */
} ie_softc[NIE];

#define	NIECHAN	(CHANS_PER_UNIT * NIE)
struct	iechan {
	int	unit;
	int	packets;
	struct	queue *ieq;
	int	type;
} iechan[NIECHAN];

int	nodev(), ieopen(), ieclose(), ieput(), iesrv();
static struct qinit ierinit = { nodev, NULL, ieopen, ieclose, 0, 0 };
static struct qinit iewinit = { ieput, NULL, ieopen, ieclose, 1514, 0 };
	/* 1514 bytes is minimum highwater mark to send 1514 byte packets */
struct streamtab iesinfo = { &ierinit, &iewinit };

#define IEMAPSIZ	100
struct map iecbmap[NIE][IEMAPSIZ];
struct map iememmap[NIE][IEMAPSIZ];
struct map iepgmap[NIE][IEMAPSIZ];
#define	escballoc(es, type, cached) (type *)escbget(es, sizeof(type), cached)
#define	escbfree(es, ptr)	escbput(es, sizeof(*ptr), (long)ptr);
#define escbpin(es, len, addr)	rmget(es->es_cbmap, (int)len, (int)addr)
#define esmemalloc(es, len)	rmalloc(es->es_memmap, (long)len)
#define esmemfree(es, len, ptr)	rmfree(es->es_memmap, (long)len, (long)ptr)
#define esmempin(es, len, addr) rmget(es->es_memmap, (int)len, (int)addr)

/*
 * fixed header size for page alignment of received data buffers
 * this is sizeof(struct ndpack) for ND.  (doesn't include ether header)
 */
#define	OPTHDRSIZ	48
#define	IPACKOVH	8	/* struct ieipack header overhead */
struct ieipack {
	struct ieipack	*iep_next;	/* next in list; MUST BE FIRST FIELD */
	struct ie_softc	*iep_es;	/* assoc ie_softc */
	char	iep_data[1500];		/* the packet */
};

#define IEDELAY	400000 		/* delay period (in ms) before giving up */
#define IEKLUDGE 20		/* delay period (in ms) to make chip work */

/*
 * Probe for device.
 */
ieprobe(reg, unit)
	caddr_t reg;
{
	register short *sp;
	struct ie_softc *es = &ie_softc[unit];

	if ((getkpgmap(reg) & PGT_MASK) == PGT_OBIO) {
		register struct obie_device *obie = (struct obie_device *)reg;

		/* onboard Ethernet */
		if (pokec(reg, 0))
			return (0);
		if (peekc(reg) == -1)
			return (0);
		if (obie->obie_noreset)
			return (0);
		es->es_type = IE_OB;
	} else {
		register struct mie_device *mie = (struct mie_device *)reg;

		/* Multibus Ethernet */
		sp = (short *)mie;
		if (poke(sp, 0))
			return (0);
		sp = &mie->mie_prom[0];
		if (poke(sp, 0x6789))
			return (0);
		if (peek(sp) == 0x6789)
			return (0);
		es->es_type = IE_MB;
	} 
	return (1);
}

/*
 * Interface exists; make available by filling in network interface
 * record.  System will initialize the interface when it is ready
 * to accept packets.
 */
ieattach(md)
	struct mb_device *md;
{
	struct ie_softc *es = &ie_softc[md->md_unit];

	if (md->md_intr)	/* set up vectored interrupts */
		*(md->md_intr->v_vptr) = (int)es;

	if (es->es_type == IE_OB) {
		es->es_obie = (struct obie_device *)md->md_addr;
		es->es_obmem = IEOBMEMDFT;
	} else {
		es->es_mie = (struct mie_device *)md->md_addr;
		/*
		 * magic thru config used to artificially restrict
		 * the amount of MBMEM used, as there is only 1 Mb
		 * available, making it a scarce resource.
		 */
		switch (md->md_flags) {
		case 0:
		default:
			es->es_vmemsize = 256*1024;
			break;
		case 1:
			es->es_vmemsize = 128*1024;
			break;
		case 2:
			es->es_vmemsize = 64*1024;
			break;
		}
	}
	iechipreset(es);
	localetheraddr(NULL, es->myetheradr);
	ieinit(md->md_unit);
	es->attached = 1;
}

/* ARGSUSED */
ieopen(q, dev)
register struct queue *q;
register dev_t dev;
{
	register struct iechan *ied;
	register struct ie_softc *es;
	register unit;

	dev = minor(dev);
	unit = dev / CHANS_PER_UNIT;

	if (dev >= NIECHAN)
		return(0);
	if (unit >= NIE)
		return(0);

	es = &ie_softc[unit];
	if(!es->attached)
		return(0);
	ied = &iechan[dev];
	if (ied->ieq)
		return(0);

	ied->unit = unit;
	ied->ieq = q;
	q->ptr = (caddr_t)ied;
	q->flag |= QBIGB | QDELIM;
	WR(q)->ptr = (caddr_t)ied;
	WR(q)->flag |= QBIGB|QDELIM;
	return(1);
}

ieclose(q)
register struct queue *q;
{
	register struct iechan *ied;

	ied = (struct iechan *)q->ptr;
	flushq(WR(q), 1);
	ied->ieq = NULL;
}

ieput(q, bp)
register struct queue *q;
register struct block *bp;
{
	register struct iechan *ied;
	register struct ie_softc *es;
	register unit, s;

	ied = (struct iechan *)q->ptr;
	unit = ied->unit;
	es = &ie_softc[unit];

	switch(bp->type) {
	
		case M_IOCTL:
			ieioctl(q, bp);
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
	s = splie();
	ied->packets++;
	if (es->es_tfree)
		iestartout(unit);
	splx(s);
}

/*
 * Process an ioctl request.
 */
ieioctl(q, bp)
register struct queue *q;
register struct block *bp;
{
	register struct iechan *ied;
	register struct ie_softc *es;
	register u_char	*msg;
	int	unit, cmd;
	
	ied = (struct iechan *)q->ptr;
	unit = ied->unit;
	es = &ie_softc[unit];
	cmd = ((union stmsg *)(bp->rptr))->ioc1.com;
	msg = (u_char *)&((union stmsg *)(bp->rptr))->ioc1.sb;

	bp->type = M_IOCACK;
	switch (cmd) {
		case ENIOADDR:          /* get my Ethernet address */
			bcopy(es->myetheradr, (int *)msg, 6);
			break;

		case ENIOTYPE:
			ied->type = *(int *)msg;
			break;
			
		default:
			bp->type = M_IOCNAK;
			break;
	}
	qreply(q, bp);
}

/*
 * Unresponsive chip alarm. Scheduled by iedog; chip has some number
 * of seconds to execute a nop, else alarm goes off.
 */
iebark(es)
	struct ie_softc *es;
{
	int unit = es - ie_softc;

	printf("ie%d: iebark reset\n", unit);
	ieinit(unit);
}

/*
 * Watchdog (deadman) timer routine, invoked every 10 seconds.
 */
iedog(es)
	struct ie_softc *es;
{
	int unit = es - ie_softc;
	int s = splie();
	struct iecb *cb;
	int fatal = 0;

	/* poll for bus error on obie */
	if (es->es_type == IE_OB)
		if (es->es_obie->obie_buserr) {
			printf("ie%d: obie buserr reset\n", unit);
			fatal = 1;
			goto reset;
		}

	/* ensure cu ok with a nop */
	cb = escballoc(es, struct iecb, 1);
	if (cb == NULL)
		goto reset;
	bzero((caddr_t)cb, sizeof (struct iecb));
	cb->ie_cmd = IE_NOP;
	iedocb(es, cb);
	iecustart(es);
	/* some number of seconds delay before iebark */
	timeout(iebark, (caddr_t)es, hz<<2);

	/* ensure ru ok with recent reception */
	if (es->es_latest + 90 < time) {
		/* restart timeout period */
		es->es_latest = time;
		goto reset;
	}

	timeout(iedog, (caddr_t)es, 10*hz);
	goto exit;
reset:
	es->es_dogreset++;
	ieinit(unit);
	if (fatal)
		panic("iedog");
	/* FALL THRU */
exit:
	(void) splx(s);
}

/*
 * Unconditionally restart the interface from ground zero.
 */
ieinit(unit)
	int unit;
{
	struct ie_softc *es = &ie_softc[unit];
	int s = splie();

	iechipreset(es);
	untimeout(iebark, (caddr_t)es);
	untimeout(iedog, (caddr_t)es);

	iedomaps(es);
	if (!iechipinit(es))
		goto exit;
	iedoaddr(es);
	/*
	 * Hang out receive buffers and start any pending writes.
	 */
	ierustart(es);
	iesplice(es);
	iestartout(unit);
	timeout(iedog, (caddr_t)es, 10*hz);
	es->es_latest = time;
exit:
	(void) splx(s);
}

/*
 * Handle Polling Ethernet interrupts
 */
iepoll()
{
	register struct ie_softc *es;
	register int found = 0;

	for (es = ie_softc; es < &ie_softc[NIE]; es++) {
		switch (es->es_type) {
		case IE_MB:
			if (es->es_mie->mie_pie && es->es_mie->mie_pe) {
				ieparity(es);
				return (1);
			}
			if (es->es_mie->mie_ie == 0)
				continue;
			if (es->es_mie->mie_intr)
				found = 1;
			break;
		case IE_OB:
			if (es->es_obie->obie_buserr)
				panic("ie: bus error");
			if (es->es_obie->obie_ie == 0)
				continue;
			if (es->es_obie->obie_intr)
				found = 1;
			break;
		default:	/* not present */
			continue;
		}
		/*
		 * Since the 82586 can take away an interrupt
		 * request after presenting it to the processsor
		 * (to facilitate an update of a new interrupt
		 * condition in the scb), we also have to check
		 * the scb to see if it indicates an interrupt
		 * condition from the chip.
		 */
		if (found == 0) {
			register struct iescb *scb = es->es_scb;

			if (scb->ie_cx || scb->ie_fr ||
			    scb->ie_cnr || scb->ie_rnr)
				found = 1;
		}
		if (found) {
		        ieintr(es);
			break;
		}
	}
	return (found);
}

/*
 * Handle Ethernet interrupts
 */
ieintr(es)
	register struct ie_softc *es;
{
	register struct iescb *scb;
	register int cmd, unit;

	unit = es - ie_softc;
	scb = es->es_scb;
	iechkcca(scb);
	if (scb->ie_cmd != 0)  {
		register struct mb_device *md;

		printf("ie%d: lost synch\n", unit);
		iestat(es);
		if (unit >= NIE || (md = ieinfo[unit]) == 0 ||
		    md->md_alive == 0)
			return;
		ieinit(unit);
		return;
	}
	cmd = 0;
/*
 * This can be done faster with something like:
 * cmd = (*(short *)scb->ie_cx) &
 *                (IECMD_ACK_CX|IECMD_ACK_FR|IECMD_ACK_CNR|IECMD_ACK_RNR);
 */

	if (scb->ie_cx)
		cmd |= IECMD_ACK_CX;
	if (scb->ie_fr)
		cmd |= IECMD_ACK_FR;
	if (scb->ie_cnr)
		cmd |= IECMD_ACK_CNR;
	if (scb->ie_rnr)
		cmd |= IECMD_ACK_RNR;
	if (cmd == 0) {
		printf("ie%d: spurious intr\n", unit);
		cmd = IECMD_ACK_CX+IECMD_ACK_FR+IECMD_ACK_CNR+IECMD_ACK_RNR;
	}
	scb->ie_cmd = cmd;
	ieca(es);

	if (cmd & (IECMD_ACK_RNR|IECMD_ACK_FR))
		ierecv(es);
	if (cmd & (IECMD_ACK_CNR|IECMD_ACK_CX))
		iecbdone(es);

	if (es->es_cbhead && scb->ie_cus == IECUS_IDLE) {
		iechkcca(scb);
		if (es->es_cbhead && scb->ie_cus == IECUS_IDLE &&
		    !scb->ie_cx && !scb->ie_cnr)
			printf("ie%d: CU out of synch\n", unit);
	}
}

/*
 * Tell the chip its ethernet address
 */
iedoaddr(es)
	register struct ie_softc *es;
{
	register struct ieiaddr *iad;

	iad = escballoc(es, struct ieiaddr, 1);
	if (iad == 0)
		panic("iedoaddr: iad");
	bzero((caddr_t)iad, sizeof (struct ieiaddr));
	iad->ieia_cb.ie_cmd = IE_IADDR;
	bcopy(es->myetheradr, iad->ieia_addr, sizeof(es->myetheradr));
	iesimple(es, &iad->ieia_cb);
	escbfree(es, iad);
}

/*
 * Move info from driver toward protocol interface
 */
ieread(es, rfd)
	register struct ie_softc *es;
	register struct ierfd *rfd;
{
	int unit = es - ie_softc;
	register struct etherpup *header;
	register struct ierbd *rbd;
	register struct iechan *ied;
	register struct queue *q;
	register struct block *bp;
	register len, min, i;
	register u_char *p;
	short type;

	if (!rfd->ierfd_ok) {
		es->ierrors++;
		printf("ie%d: receive error\n", unit);
		return;
	}
	if (rfd->ierfd_rbd == IENORBD) {
		printf("ie%d: runt packet\n", unit);
		es->ierrors++;
		return;
	}
	rbd = (struct ierbd *)from_ieoff(es, (ieoff_t)rfd->ierfd_rbd);
	if (!rbd->ierbd_eof) {		/* length > 1500  */
		printf("ie%d: giant packet\n", unit);
		es->ierrors++;
		return;
	}
	/*
	 * Pull packet off interface.
	 */
	header = (struct etherpup *)rfd->ierfd_dhost;
	type = header->type;
	ied = &iechan[unit * CHANS_PER_UNIT];
	for (i = 0; i < CHANS_PER_UNIT; i++, ied++)
		if (ied->ieq && ied->type == type)
			break;
	if (i >= CHANS_PER_UNIT)
		return;
	q = ied->ieq;
	if (q->next->flag & QFULL) {
		es->ierrors++;
		return;
	}
	len = (rbd->ierbd_cnthi << 8) + rbd->ierbd_cntlo;

	/*
	 * Splice in the ethernet header
	 * Assumes allocb will return a block at least as big as etherpup.
	 */
	bp = allocb(len + sizeof(struct etherpup));
	*(struct etherpup *)bp->wptr = *header;
	bp->wptr += sizeof(struct etherpup);

	/*
	 * Now copy the data
	 */
	p = (u_char *)from_ieaddr(es, rbd->ierbd_buf);
	i = MIN(((bp->lim - bp->base) - sizeof(struct etherpup)), len);
	for(;;) {
		len -= i;
		while (i-- > 0)
			*bp->wptr++ = *p++;
		(*q->next->qinfo->putp)(q->next, bp);
		if (len <= 0)
			break;
		bp = allocb(len);
		i = MIN((bp->lim - bp->base), len);
	}
	if (putctl(q->next, M_DELIM))
		es->ipackets++;
	else 
		printf("ierint no DELIM bp\n");
}

/*
 * Process completed input packets
 * and recycle resources;
 * only called from interrupt level by ieintr
 */
ierecv(es)
	register struct ie_softc *es;
{
	register struct ierfd *rfd, *nrfd;
	register struct ierbd *rbd, *nrbd;
	register struct iescb *scb = es->es_scb;
	int eof, e;

	es->es_latest = time;		/* latch arrival time */
	
	rfd = es->es_rfdhead;
	if (rfd == NULL)		/* not initialized */
		return;
top:
	while (rfd && rfd->ierfd_done) {
		ieread(es, rfd);
		if (rfd->ierfd_rbd != IENORBD) {
			rbd = (struct ierbd *)from_ieoff(es,
			    (ieoff_t)rfd->ierfd_rbd);
			if (rbd != es->es_rbdhead)
				panic("ierecv rbd");
			while (rbd && rbd->ierbd_used) {
				if (rbd != (struct ierbd *)from_ieoff(es,
				    (ieoff_t)es->es_rbdtail->ierbd_next))
					panic("ierecv rbd list");
				nrbd = (struct ierbd *)from_ieoff(es,
					(ieoff_t)rbd->ierbd_next);
				rbd->ierbd_el = 1;
				eof = rbd->ierbd_eof;
				*(short *)rbd = 0;
				es->es_rbdtail->ierbd_el = 0;
				es->es_rbdtail = rbd;
				rbd = es->es_rbdhead = nrbd;
				if (eof)
					break;
			}
		}
		if (rfd != (struct ierfd *)from_ieoff(es,
		    (ieoff_t)es->es_rfdtail->ierfd_next))
			panic("ierecv rfd list");
		nrfd = (struct ierfd *)from_ieoff(es, (ieoff_t)rfd->ierfd_next);
		rfd->ierfd_rbd = IENORBD;
		rfd->ierfd_el = 1;
		*(short *)rfd = 0;
		es->es_rfdtail->ierfd_el = 0;
		es->es_rfdtail = rfd;
		rfd = es->es_rfdhead = nrfd;
	}
	if (e = scb->ie_crcerrs) {	/* count of CRC errors */
		scb->ie_crcerrs = 0;
		e = from_ieint(e);
		es->ierrors += e;
		es->es_ierr.crc += e;
	}
	if (e = scb->ie_alnerrs) {	/* count of alignment errors */
		scb->ie_alnerrs = 0;
		e = from_ieint(e);
		es->ierrors += e;
		es->es_ierr.aln += e;
	}
	if (e = scb->ie_rscerrs) {	/* count of discarded packets */
		scb->ie_rscerrs = 0;
		e = from_ieint(e);
		es->ierrors += e;
		es->es_ierr.rsc += e;
	}
	if (e = scb->ie_ovrnerrs) {	/* count of overrun packets */
		scb->ie_ovrnerrs = 0;
		e = from_ieint(e);
		es->ierrors += e;
		es->es_ierr.ovr += e;
	}
	if (scb->ie_rus == IERUS_READY)		/* as expected */
		return;
	es->es_runotready++;
	/* following test must be made when we know chip is quiet */
	if (es->es_rfdhead->ierfd_done)		/* more snuck in */
		goto top;
	es->es_rfdhead->ierfd_rbd = to_ieoff(es, (caddr_t)es->es_rbdhead);
	iechkcca(scb);
	scb->ie_rfa = to_ieoff(es, (caddr_t)es->es_rfdhead);
	scb->ie_cmd = IECMD_RU_START;
	ieca(es);
}

/*
 * Free up the resources after transmitting a packet.
 * Called by iecuclean at splie or hardware level 3.
 */
iexmitdone(es, td)
	register struct ie_softc *es;
	register struct ietfd *td;
{
	int unit = es - ie_softc;
	register int i;

	if (td->ietfd_ok) {
		es->collisions += td->ietfd_ncoll;
		es->opackets++;
		if (td->ietfd_defer) es->es_iedefer++;
		if (td->ietfd_heart) es->es_ieheart++;
	} else {
		es->oerrors++;
		if (td->ietfd_xcoll)
			printf("ie%d: Ethernet jammed\n", unit);
		if (td->ietfd_nocarr)
			printf("ie%d: no carrier\n", unit);
		if (td->ietfd_nocts)
			printf("ie%d: no CTS\n", unit);
		if (td->ietfd_underrun)
			es->es_xmiturun++;
	}
	if (!td->ietfd_tbd)
		panic("ie%d: iexmitdone: no tbd");
	for(i = 0; i < NTRANSBUF; i++)
		if (td == es->es_tfd[i])
			break;
	if (es->es_tfree & (1 << i))
		printf("ie%d stray xmit interrupt\n", unit);
	es->es_tfree |= (1 << i);
}

#define rndtoeven(x)	(((x)+1) & ~1)
/*
 * Start or restart output to wire.
 */
iestartout(unit)
	int unit;
{
 	register struct ie_softc *es = &ie_softc[unit];
	register struct iechan *ied;
	register cnt, i;
	register caddr_t to;
	int count, bufnum;
	struct ietfd *td;
	register struct ietbd *tbd;
	register struct queue *q;
	register struct block *bp, *nbp;
	struct block *head, **bnext;

	if (!es->es_tfree)
		goto out;
	ied = &iechan[unit * CHANS_PER_UNIT];
	for(i = 0; i < CHANS_PER_UNIT; i++, ied++)
		if (ied->ieq && ied->packets > 0)
			break;
	if (i >= CHANS_PER_UNIT)
		goto out;

	ied->packets--;
	q = WR(ied->ieq);

	/* The packet must be justified to the end of the buffer.
	 * Therefore, we have to count the bytes before copying.
	 */
	bnext = &head;
	while (*bnext = bp = getq(q)) {
		if (bp->type == M_DELIM) {
			bp->next = 0;
			break;
		}
		cnt += bp->wptr - bp->rptr;
		bnext = &bp->next;
	}
	bp = head;

	/* if there is no ethernet header in packet, throw it out */
	if (cnt < sizeof(struct etherpup) ||
	    (bp->wptr - bp->rptr) < sizeof(struct etherpup)) {
		while(bp) {
			nbp = bp->next;
			freeb(bp);
			bp = nbp;
		}
		goto out;
	}

	bufnum = ffs(es->es_tfree);
	es->es_tfree &= ~(1 << bufnum);
	td = es->es_tfd[bufnum];
	tbd = es->es_tbd[bufnum];

	/* Setup the header */
	bcopy(bp->rptr, (caddr_t)td->ietfd_dhost, 6);	/* Dest */
	bp->rptr += 12;					/* Skip src */
	bcopy(bp->rptr, (caddr_t)&td->ietfd_type, 2);	/* Type */
	bp->rptr += 2;					/* Skip src */
	cnt -= sizeof(struct etherpup);

	/* Setup the data */
	if (cnt > 1500)			/* test for too large packets */
		cnt = 1500;
	count = cnt;
	to = es->es_tbuffer[bufnum];
	while (cnt > 0 && bp != 0) {
		i = bp->wptr - bp->rptr;
		bcopy(bp->rptr, to, i);
		cnt -= i;
		to += i;
		nbp = bp->next;
		freeb(bp);
		bp = nbp;
	}

	if (count < 46)			/* test for small packets */
		count = 46;

	/* flush remaining buffers, if any (too large packet: count > 1500) */
	while (bp) {
		nbp = bp->next;
		freeb(bp);
		bp = nbp;
	}

	/* Setup the buffer descriptor */
	tbd->ietbd_eof = 1;
	tbd->ietbd_next = 0;
	tbd->ietbd_buf = to_ieaddr(es, es->es_tbuffer[bufnum]);
	tbd->ietbd_cntlo = count & 0xFF;
	tbd->ietbd_cnthi = count >> 8;
	td->ietfd_tbd = to_ieoff(es, (caddr_t)tbd);
	td->ietfd_cmd = IE_TRANSMIT;
	iedocb(es, (struct iecb *)td);
out:
	iecustart(es);
	return;
}

/*
 * Set the control block area size
 */
iesetcbsize(es)
	register struct ie_softc *es;
{
	int tfds;	/* number of tfd's */
	int tfdsize;	/* cbsize for each tfd */
	int tbufsize;	/* cbsize for each tbuf */
	int rbufs;	/* number of rbuf's in cb area */
	int rbufsize;	/* cbsize for each rbuf */
	int rfds;	/* number of rfd's */
	int rfdsize;	/* cbsize for each rfd */
	int fixed;	/* fixed overhead, including slop */

	fixed = sizeof (struct iescp) + sizeof (struct ieiscp)
		+ sizeof (struct iescb) + sizeof (struct ieconf)
		+ 100;
	tfdsize = sizeof (struct ietfd) + sizeof (struct ietbd);
	tbufsize = sizeof (struct ieipack);
	rbufsize = sizeof (struct ieipack);
	rfdsize = sizeof (struct ierfd) + sizeof (struct ierbd);

	switch (es->es_type) {
	case IE_OB:
		tfds = NTRANSBUF;
		rbufs = (es->es_obmem - fixed - tfds * (tfdsize+tbufsize)) /
			(rbufsize + rfdsize);
		break;
	case IE_MB:
		tfds = 50;		/* maximum if_snd */
		/* each rbuf eats into a 2K section of vmemsize */
		rbufs = min(NPOOL_RBD, (u_int)(es->es_vmemsize>>11));
		break;
	}
	rfds = rbufs;

	es->es_cbsize = fixed
		+ tfds * tfdsize
		+ rfds * rfdsize;
}

/*
 * Called by ieinit to (allocate and re)initialize rmap's
 * We need to be careful, since the board may be in use
 * (ieipacks loaned out, pages swapped)
 */
iedomaps(es)
	register struct ie_softc *es;
{
	int unit = es - ie_softc;

	iesetcbsize(es);
	bzero((caddr_t)iecbmap[unit], sizeof iecbmap[unit]);
	bzero((caddr_t)iememmap[unit], sizeof iememmap[unit]);
	es->es_cbhead = NULL;
	es->es_cbcbusy = NULL;
	iecbcinit(es);

	if (es->es_type == IE_OB) {
		int memall();
		caddr_t va;

		if (es->es_base == 0) {
			va = wmemall(memall, es->es_obmem);
			if (va == 0)
				panic("ieattach: no memory");
			es->es_base = va;
		}
		va = es->es_base;
		es->es_cbmap = &iecbmap[unit][0];
		es->es_memmap = es->es_cbmap;
		rminit(es->es_cbmap, (long)es->es_obmem,
		    (long)es->es_base, "iecb", IEMAPSIZ);
		es->es_memspace = (caddr_t)KERNELBASE;
#ifdef sun3
		/* use the page already set up by the prom monitor */
		es->es_scp = (struct iescp *)(IESCPADDR+es->es_memspace);
#else
		/*
                 * Remap the [preallocated] virtual page containing
                 * the SCP to make it point to the same physical
                 * location as va, so that we can muck with the
                 * control blocks using the address returned from
                 * memall, and the chip can locate the scp at its
                 * fixed address
                 * Cache note: since we can not, in general, enforce
                 * the cache antialiasing separation requirement, we
                 * would need to avoid caching this physical page
		 */
		{
			struct pte dummypte;	/* for mapin to write on */
			int paddr = getkpgmap(va) & PG_PFNUM;

			mapin(&dummypte, (u_int)btop(IESCPADDR+
			    es->es_memspace), (u_int)paddr, 1, PG_V | PG_KW);
			es->es_scp = (struct iescp *)escbpin(es, 
			    (long)sizeof (struct iescp),
			    (long)es->es_base + (IESCPADDR & PGOFSET));
		}
#endif sun3
	} else { /* IE_MB */
		register struct mie_device *mie = es->es_mie;
		struct miepg *pg;
		short *ap;
		int i;
		int a, vaddr, paddr;
		int firsttime = es->es_memspace == 0;

		if (firsttime) {
			if ((a = rmalloc(kernelmap,(long)btoc(es->es_vmemsize)))
			      == 0) {
				printf("ie%d: no kernelmap for ie memory\n",
				    unit);
				panic("iedomaps");
			}
			es->es_memspace = (caddr_t)kmxtob(a);
		}
		vaddr = (int)es->es_memspace;
		a = btokmx((struct pte *)vaddr);
		/* board's mem boundary to byte addr */
		paddr = mie->mie_mbmhi << 16;
		/* preserve pagetype bits and which megabyte its in (for VME) */
		es->es_paddr = getkpgmap((caddr_t)mie) & PG_PFNUM;
		es->es_paddr &= ~(0x100000/NBPG -1);
		es->es_paddr |= btop(paddr);
		mapin(&Usrptmap[a], (u_int)btop(vaddr), (u_int)es->es_paddr,
			(int)btoc(es->es_vmemsize), PG_V | PG_KW);

		/* clear the board unless in use */
		if (firsttime) {
			ap = (short *)mie->mie_pgmap;
			for (i=0; i<IEVVSIZ; i++)	/* clears mp_p2mem */
				*ap++ = 0;
			for (i=0; i<IEPMEMSIZ/IEPAGSIZ; i++) {
				mie->mie_pgmap[0].mp_pfnum = i;
				bzero(es->es_memspace, IEPAGSIZ);
			}
			pg = &mie->mie_pgmap[0];
			for (i=0; i<es->es_vmemsize/IEPAGSIZ; i++) {
				pg->mp_swab = 1;
				pg->mp_pfnum = i;
				pg++;
			}

			/* use last onboard ie page for chip init */
			/* (no need to reclaim, since beyond vmemsize) */
			pg = &mie->mie_pgmap[IEVVSIZ-1];
			pg->mp_swab = 1;
			pg->mp_pfnum = 0;

			/* patch potential powerup parity problem */
			mie->mie_peack = 1;
		}

		es->es_base = es->es_memspace;
		es->es_cbmap = &iecbmap[unit][0];
		rminit(es->es_cbmap, (long)es->es_cbsize,
		    (long)es->es_base, "iecb", IEMAPSIZ);
		es->es_scp = (struct iescp *)escbpin(es, 
			(long)sizeof (struct iescp),
			(long)es->es_base + (IESCPADDR & (IEPAGSIZ-1)));
		es->es_memmap = &iememmap[unit][0];
		es->es_pgmap = &iepgmap[unit][0];
		/*
		 * if we have enough memory, create a page pool which
		 * can manipulated via if_memmap, say by ND
		 */
		if (es->es_vmemsize == 256*1024) {
			rminit(es->es_memmap, (long)(128*1024-es->es_cbsize),
			    (long)(es->es_memspace+es->es_cbsize),
			    "iemem", IEMAPSIZ);
			if (firsttime)
				rminit(es->es_pgmap, (long)(128*1024),
				    (long)(es->es_memspace+128*1024),
				    "iepg", IEMAPSIZ);
		} else {
			rminit(es->es_memmap,
			    (long)(es->es_vmemsize-es->es_cbsize), 
			    (long)(es->es_memspace+es->es_cbsize),
			    "iemem", IEMAPSIZ);
			rminit(es->es_pgmap, (long)0, (long)0,
			    "iepg", IEMAPSIZ);
		}
	}
}

/*
 * Basic 82586 initialization
 */
int
iechipinit(es)
	register struct ie_softc *es;
{
	int unit = es - ie_softc;
	struct ieiscp *iscp;
	struct iescb *scb;
	struct iecb *cb;
	struct ieconf *ic;
	int ok = 0;
	int gotintr;
	int i;
#ifdef notdef
	struct ietdr *tdr;
#endif

	if (es->es_scp == 0) {
		printf("ie%d: scp alloc failed\n", unit);
		goto exit;
	}
	iscp = escballoc(es, struct ieiscp, 0);
	if (iscp == 0) {
		printf("ie%d: iscp alloc failed\n", unit);
		goto exit;
	}
	scb = escballoc(es, struct iescb, 0);
	if (scb == 0) {
		printf("ie%d: scb alloc failed\n", unit);
		goto exit;
	}
	es->es_scb = scb;

reset:
	bzero((caddr_t)es->es_scp, sizeof (struct iescp));
	es->es_scp->ie_iscp = to_ieaddr(es, (caddr_t)iscp);
	bzero((caddr_t)iscp, sizeof (struct ieiscp));
	iscp->ie_busy = 1;
	iscp->ie_cbbase = to_ieaddr(es, es->es_base);
	iscp->ie_scb = to_ieoff(es, (caddr_t)scb);
	bzero((caddr_t)scb, sizeof (struct iescb));
	scb->ie_magic = IEMAGIC;
	/*
	 * Hardware reset the chip.  We make the interval from
	 * reset to initial channel attention as small as reasonable
	 * to reduce the risk of scribbling chips getting us.
	 */
	switch (es->es_type) {
	case IE_MB:
		/* hardware reset already occurred in iechipreset */
		break;

	case IE_OB:
		es->es_obie->obie_noreset = 1;
		DELAY(IEKLUDGE);		/* REQUIRED */
		break;
	}
	ieca(es);
	CDELAY(!iscp->ie_busy, IEDELAY);	/* ensure chip eats iscp */
	CDELAY(scb->ie_cnr, IEDELAY);		/* ensure scb updated too */
	gotintr = iewaitintr(es);		/* wait for interrupt */
	if (iscp->ie_busy || !scb->ie_cnr || !gotintr) {
		printf("ie%d: init failed:%s%s%s\n", unit,
		    iscp->ie_busy?" iscp busy":"",
		    !scb->ie_cnr ?" no cnr":"",
		    !gotintr	 ?" no intr":"");
		goto exit;
	}
	if (scb->ie_cus != IECUS_IDLE ) {
		printf("ie%d: cus not idle after reset\n", unit);
		iechipreset(es);
		goto reset;
	}

	cb = escballoc(es, struct iecb, 1);
	if (cb == NULL) {
		printf("ie%d: cb alloc failed\n", unit);
		goto exit;
	}
	bzero((caddr_t)cb, sizeof (struct iecb));
	cb->ie_cmd = IE_DIAGNOSE;
	iesimple(es, cb);
	if (!cb->ie_ok) {
		printf("ie%d: Intel 82586 failed diagnostics\n", unit);
		escbfree(es, cb);
		goto exit;
	}
	escbfree(es, cb);
#ifdef notdef
	/* skip, since hardware requires quiet net to work */
	tdr = escballoc(es, struct ietdr, 1);
	if (tdr == NULL) {
		printf("ie%d: tdr alloc failed\n", unit);
		goto exit;
	}
	bzero((caddr_t)tdr, sizeof (struct ietdr));
	tdr->ietdr_cb.ie_cmd = IE_TDR;
	iesimple(es, &tdr->ietdr_cb);
	if (!tdr->ietdr_ok) {
#define TDRCONST	12	/* approx 0.77c/10Mhz */
		int dist = (tdr->ietdr_timhi<<8)+tdr->ietdr_timlo;
		if (dist != 0x7FF)
			printf("ie%d: link not OK - distance = ~%dm\n",
				unit, TDRCONST*dist);
		else
			printf("ie%d: link not OK\n", unit);
		escbfree(es, tdr);
		goto exit;
	}
	if (tdr->ietdr_xcvr) printf("ie%d: transceiver bad\n", unit);
	if (tdr->ietdr_open) printf("ie%d: net not terminated\n", unit);
	if (tdr->ietdr_shrt) printf("ie%d: net shorted\n", unit);
	escbfree(es, tdr);
#endif notdef
	ic = escballoc(es, struct ieconf, 1);
	if (ic == NULL) {
		printf("ie%d: ic alloc failed\n", unit);
		goto exit;
	}
	iedefaultconf(ic);
	iesimple(es, &ic->ieconf_cb);
	escbfree(es, ic);
	for (i = 0; i < NTRANSBUF; i++) {
		if ((es->es_tfd[i] = escballoc(es, struct ietfd, 0)) == NULL) {
			printf("ie%d: tfd alloc failed\n", unit);
			goto exit;
		}
		if ((es->es_tbd[i] = escballoc(es, struct ietbd, 0)) == NULL) {
			printf("ie%d: tbd alloc failed\n", unit);
			goto exit;
		}
		if ((es->es_tbuffer[i]=(caddr_t)esmemalloc(es, 1500)) ==NULL) {
			printf("ie%d: tbuffer alloc failed\n", unit);
			goto exit;
		}
		es->es_tfree |= (1 << i);
	}
	ok = 1;
exit:
	return (ok);
}

/*
 * called by ierustart to create the receiver buffer list
 */
iegetrbufs(es)
	register struct ie_softc *es;
{
	caddr_t addr;
	int count, avail;
	register int page, last;
	register struct ieipack *iep;

	es->es_iepavail = NULL;
	if (es->es_type == IE_OB) {
		last = (es->es_obmem-es->es_cbsize) /
			(sizeof (struct ieipack)) - NTRANSBUF;
		for (count = 0; count < last; count++) {
			iep = (struct ieipack *)esmemalloc(es,
				sizeof (struct ieipack));
			if (iep == 0)
				break;
			iep->iep_es = es;
			iep->iep_next = es->es_iepavail;
			es->es_iepavail = iep;
		}
		avail = count;
	} else { /* IE_MB */
		count = NPOOL_RBD;
		/* XXX not really, since there could be page pool */
		last = es->es_vmemsize >> IEPAGSHIFT;
		/* 2 pages for xmit (vice receive) buffer */
		page = iepages(es->es_cbsize) + 2;
		for (;  count > 0 && page < last; page++) {
			addr = es->es_memspace + page*IEPAGSIZ;
			addr += IEPAGSIZ - (IPACKOVH + OPTHDRSIZ);
			if (!esmempin(es, sizeof (struct ieipack), (int)addr))
				continue;
			count--;
			iep = (struct ieipack *)addr;
			iep->iep_es = es;
			iep->iep_next = es->es_iepavail;
			es->es_iepavail = iep;
		}
		avail = NPOOL_RBD - count;
	}

	return (avail);
}

/*
 * Initialize and start the Receive Unit
 */
ierustart(es)
	register struct ie_softc *es;
{
	int unit = es - ie_softc;
	register struct ierbd *rbd;
	register struct ierfd *rfd;
	register struct iescb *scb;
	register struct ieipack *iep;
	register int i, nrfd, ninit_rbd;

	ninit_rbd = iegetrbufs(es);
	es->es_rbdhead = NULL;
	for (i = 0; i < ninit_rbd; i++) {
		if ((iep = es->es_iepavail) == NULL)
                        break;
                rbd = escballoc(es, struct ierbd, 0);
                if (rbd == NULL)
                        break;
                es->es_iepavail = iep->iep_next;
		*(short *)rbd = 0;
		if (es->es_rbdhead) {
			rbd->ierbd_next = to_ieoff(es, (caddr_t)es->es_rbdhead);
			rbd->ierbd_el = 0;
		} else {
			es->es_rbdtail = rbd;
			rbd->ierbd_next = 0;
			rbd->ierbd_el = 1;
		}
		es->es_rbdhead = rbd;
		rbd->ierbd_buf = to_ieaddr(es, iep->iep_data);
		rbd->ierbd_sizehi = 1500 >> 8;
		rbd->ierbd_sizelo = 1500 & 0xFF;
		rbd->ierbd_iep = iep;
	}
	/*
	 * We allocate one fewer RFD than RBD to
	 * avoid a suspected microcode bug in the chip
	 */
	nrfd = i-1;
	es->es_rbdtail->ierbd_next = to_ieoff(es, (caddr_t)es->es_rbdhead);
	es->es_rfdhead = NULL;
	for (i=0; i<nrfd; i++) {
		rfd = escballoc(es, struct ierfd, 0);
		if (rfd == NULL) {
			break;
		}
		*(short *)rfd = 0;
		if (es->es_rfdhead) {
			rfd->ierfd_next = to_ieoff(es, (caddr_t)es->es_rfdhead);
			rfd->ierfd_el = 0;
		} else {
			es->es_rfdtail = rfd;
			rfd->ierfd_next = 0;
			rfd->ierfd_el = 1;
		}
		es->es_rfdhead = rfd;
		rfd->ierfd_susp = 0;
		rfd->ierfd_rbd = IENORBD;
	}
	if (i != nrfd)
		printf("ie%d: fewer RFD's were allocated than expected\n",
		    unit);
	es->es_rfdtail->ierfd_next = to_ieoff(es, (caddr_t)es->es_rfdhead);
	rfd = es->es_rfdhead;
	rfd->ierfd_rbd = to_ieoff(es, (caddr_t)rbd);
	scb = es->es_scb;
	if (scb->ie_rus != IERUS_IDLE) {
		printf("ie%d: RU not idle??\n", unit);
		iestat(es);
		iechkcca(scb);
		scb->ie_cmd = IECMD_RU_ABORT;
		ieca(es);
	}
	iechkcca(scb);
	scb->ie_rfa = to_ieoff(es, (caddr_t)rfd);
	scb->ie_cmd = IECMD_RU_START;
	ieca(es);
	CDELAY(scb->ie_rus == IERUS_READY, IEDELAY);
	if (scb->ie_rus != IERUS_READY)
		printf("ie%d: RU did not become ready\n", unit);
}

/*
 * Put a CB on the CBL
 */
iedocb(es, cb)
	register struct ie_softc *es;
	register struct iecb *cb;
{
	int s = splie();

	*(short *)cb = 0;	/* clear status bits */
        cb->ie_susp = 0;        /* clear suspend bit */
	cb->ie_el = 1;		/* will be reset in iecustart */
	cb->ie_intr = 1;
	cb->ie_next = 0;
	if (es->es_cbhead) {
		es->es_cbtail->ie_next = to_ieoff(es, (caddr_t)cb);
		es->es_cbtail = cb;
	} else {
		es->es_cbhead = es->es_cbtail = cb;
	}
	(void) splx(s);
}

/*
 * Process completed CBs, reclaiming specified storage.
 * Allocator is responsible for reclaiming other storage.
 * Called by iecustart at splie.
 * Called by iecbdone at splie or hardware level 3.
 */
iecuclean(es)
	register struct ie_softc *es;
{
	register struct iecb *cb;
		
	while ((cb = es->es_cbhead) && cb->ie_done) {
		if (cb->ie_next)
			es->es_cbhead = (struct iecb *)from_ieoff(es,
							(ieoff_t)cb->ie_next);
		else
			es->es_cbhead = NULL;
		switch (cb->ie_cmd) {
		case IE_TRANSMIT:
			iexmitdone(es, (struct ietfd *)cb);
			break;
		case IE_NOP:
			untimeout(iebark, (caddr_t)es);
			escbfree(es, cb);
			break;
		default:
			if (!es->es_simple)
				printf("ie%d: unknown cmd %x done\n",
					es-ie_softc, cb->ie_cmd);
			break;
		}
	}
}

/*
 * Start the CU with the current CBL
 */
iecustart(es)
	register struct ie_softc *es;
{
	register struct iecb *cb;
	register struct iescb *scb = es->es_scb;
	int s = splie();
		
	iechkcca(scb);
	if (es->es_cbhead == NULL ||
	    scb->ie_cus == IECUS_READY) {	/* still going */
		(void) splx(s);
		return;
	}
	iecuclean(es);
	/* link remaining CBs into continuous list */
	if ((cb = es->es_cbhead) == NULL) {
		(void) splx(s);
		return;
	}
	while (cb && cb->ie_next) {
		cb->ie_el = 0;
		cb = (struct iecb *)from_ieoff(es, (ieoff_t)cb->ie_next);
	}
	/* start CU */
	scb->ie_cbl = to_ieoff(es, (caddr_t)es->es_cbhead);
	scb->ie_cmd = IECMD_CU_START;
	ieca(es);
	(void) splx(s);
}

/*
 * Clean up and restart the CBs on the CBL
 * Called by ieintr at hardware level 3.
 * Called by iesimple at splie.
 */
iecbdone(es)
	register struct ie_softc *es;
{

	iecuclean(es);
	/* generate more CBs */
	iestartout(es - ie_softc);
}

/*
 * Do the command simply.
 * Attempted sleep/wakeup calls, but it refused to work.
 */
iesimple(es, cb)
	register struct ie_softc *es;
	register struct iecb *cb;
{
	register struct iescb *scb = es->es_scb;
	int s, cmd = 0;

	es->es_simple++;
	iedocb(es, cb);
	iecustart(es);
	CDELAY(cb->ie_done, IEDELAY);
	if (!cb->ie_done) {
		iestat(es);
		panic("iesimple");
	}
	s = splie();
	iechkcca(scb);
	if (scb->ie_cx)
		cmd |= IECMD_ACK_CX;
	if (scb->ie_cnr)
		cmd |= IECMD_ACK_CNR;
	scb->ie_cmd = cmd;
	ieca(es);
	if (cmd & (IECMD_ACK_CNR|IECMD_ACK_CX))
		iecbdone(es);
	es->es_simple--;
	(void) splx(s);
}

/*
 * Return chip's idea of given address or 0 if not chip accessible
 */
ieaddr(es, cp)
	register struct ie_softc *es;
	caddr_t cp;
{
	int pte;

	if (es->es_type == IE_OB) {
		/* onboard ie may only reference obmem */
		if ((getkpgmap(cp) & PGT_MASK) != PGT_OBMEM)
			cp = (caddr_t)0;
#ifdef sun3
		else if (cp < es->es_memspace)
			cp = (caddr_t)0;
		else
			cp -= (u_long)es->es_memspace;
#endif sun3
		return ((int)cp);
	}
	pte = getkpgmap(cp) & PG_PFNUM;
	if (pte >= es->es_paddr && pte < es->es_paddr + btoc(es->es_vmemsize))
		return ((pte - es->es_paddr) << BSHIFT(0)) + ((int)cp & CLOFSET);
	return (0);
}

/*
 * Check Control Command Acceptance by 82586
 */
iechkcca(scb)
	register struct iescb *scb;
{
	register i;

	for (i=0; i < IEDELAY; i++) {
		if (scb->ie_magic != IEMAGIC)
			panic("ie: scb overwritten");
		if (scb->ie_cmd == 0)
			break;
	}
	if (i >= IEDELAY) {
		printf("ie: cmd not accepted\n");
		panic("iechkcca");
	}
}

/*
 * The control block caching code to bypass rmalloc/rmfree.
 * It also allows us to check allocated lengths, as well
 * as aiding instrumentation for verification and tuning.
 * We have two sizes, small and large.
 * If the request exceeds the small threshold, we allocate
 * the larger block.
 */

#define lo	es->es_cbc_lo
#define hi	es->es_cbc_hi
#define cache	es->es_cbcache
#define ovfl	es->es_cbc_ovfl
#define small	iecbcsmall
#define large	iecbclarge
#define head	es->es_cbchain.next
#define chain	es->es_cbchain

/*
 * Initialize the control block cache
 */
iecbcinit(es)
	register struct ie_softc *es;
{
	if (es->es_cbcbusy)
		iecbcflush(es);
	lo = -1;
	hi = CBCACHESIZ;
	small = sizeof (struct ietbd);
	large = sizeof (struct ietfd);
}

/*
 * cached is a flag used to indicate that the block must be
 * cached, as it will be freed (for reallocation.)
 */
long
escbget(es, len0, cached)
	register struct ie_softc *es;
	int len0;
	int cached;
{
	long result;
	int len = rndtoeven(len0);
	if (!cached)
		result = (rmalloc(es->es_cbmap, (long)len));
	else if (len <= small)
		if (lo > -1)
			result = (cache[lo--]);
		else
			result = (rmalloc(es->es_cbmap, (long)small));
	else if (len <= large)
		if (hi < CBCACHESIZ)
			result = (cache[hi++]);
		else
			result = (rmalloc(es->es_cbmap, (long)large));
	else
		panic ("escbget");	/* len too large */

	if (result == 0 && es->es_cbcbusy) {
		iecbcflush(es);
		result = escbget(es, len0, cached);
	}
	return (result);
}

/*
 * flush the cb cache
 */
iecbcflush(es)
	struct ie_softc *es;
{
	printf("WARNING: ie%d: flushing cb cache\n", es - ie_softc);
	while (lo > -1)
		rmfree(es->es_cbmap, (long)small, (long)cache[lo--]);
	while (hi < CBCACHESIZ)
		rmfree(es->es_cbmap, (long)large, (long)cache[hi++]);
	es->es_cbcbusy = 0;
}

long
escbput(es, len, ptr)
	register struct ie_softc *es;
	int len;
	long ptr;
{
	int overflow = 0;

	es->es_cbcbusy = 1;
	len = rndtoeven(len);
	if (len <= small)
		if (lo < hi - 1)
			cache[++lo] = ptr;
		else {
			overflow++;
			rmfree(es->es_cbmap, (long)small, (long)ptr); 
		}
	else if (len <= large)
		if (hi > lo + 1)
			cache[--hi] = ptr;
		else {
			overflow++;
			rmfree(es->es_cbmap, (long)large, (long)ptr);
		}
	else
		panic("escbput");

	if (overflow) {
		ovfl++;
		if ((ovfl & OVFLWARNMASK) == 0) {
			ovfl = 0;
			printf("ie%d: cache overflowed\n", es - ie_softc);
		}
	}
}

#undef lo
#undef hi
#undef cache
#undef ovfl
#undef small
#undef large

int iefifolim = 12;
/*
 * Set default configuration parameters
 */
iedefaultconf(ic)
	register struct ieconf *ic;
{
	bzero((caddr_t)ic, sizeof (struct ieconf));
	ic->ieconf_cb.ie_cmd = IE_CONFIG;
	ic->ieconf_bytes = 12;
	ic->ieconf_fifolim = iefifolim;
	ic->ieconf_pream = 2;		/* 8 byte preamble */
	ic->ieconf_alen = 6;
	ic->ieconf_acloc = 0;
	ic->ieconf_space = 96;
	ic->ieconf_slttmh = 512 >> 8;
	ic->ieconf_minfrm = 64;
	ic->ieconf_retry = 15;
	ic->ieconf_crfilt = 3;
}

iestat(es)
	struct ie_softc *es;
{
	register struct iescb *scb = es->es_scb;
	static char *cus[] = { "idle", "suspended", "ready", "<3>",
				"<4>", "<5>", "<6>", "<7>" };
	static char *rus[] = { "idle", "suspended", "no resources",
				"<3>", "ready", "<5>", "<6>", "<7>" };

	printf("ie%d: scb: ", es - ie_softc);
	if (scb->ie_cx) printf("cx ");
	if (scb->ie_fr) printf("fr ");
	if (scb->ie_cnr) printf("cnr ");
	if (scb->ie_rnr) printf("rnr ");
	printf("cus=%s ", cus[scb->ie_cus]);
	printf("rus=%s\n", rus[scb->ie_rus]);
	printf("cbl=0x%x rfa=0x%x crc=0x%x aln=0x%x rsc=0x%x ovrn=0x%x\n",
		scb->ie_cbl, scb->ie_rfa,
		scb->ie_crcerrs, scb->ie_alnerrs, scb->ie_rscerrs,
		scb->ie_ovrnerrs);
	if (scb->ie_cmd) printf("cmd=0x%x\n", scb->ie_cmd & 0xFFFF);
}

/*
 * Parity error! Scan entire memory for errors
 */
ieparity(es)
	register struct ie_softc *es;
{
	register struct mie_device *mie = es->es_mie;
	register u_short *s, *e, x;

	printf("ie%d: parity error src=%d byte=%d addr=%x\n", es-ie_softc,
		mie->mie_pesrc, mie->mie_pebyte, mie->mie_erraddr);
	mie->mie_peack = 1;
	s = (u_short *)es->es_memspace;
	e = (u_short *)(es->es_memspace + es->es_vmemsize);
	printf("scanning...\n");
	while (s < e) {
		x = *s;
#ifdef lint
		x = x;
#endif
		if (mie->mie_pe) {
			printf("off=%x src=%d byte=%d addr=%x\n", 
				(int)s - (int)es->es_memspace,
				mie->mie_pesrc, mie->mie_pebyte,
				mie->mie_erraddr);
			mie->mie_peack = 1;
		}
		s++;
	}
	printf("done\n");
}

/*
 * Activate the channel attention line
 */
ieca(es)
	register struct ie_softc *es;
{
	if (es->es_type == IE_MB) {
		es->es_mie->mie_ca = 1;
		es->es_mie->mie_ca = 0;
	} else {
		es->es_obie->obie_ca = 1;
		es->es_obie->obie_ca = 0;
	}
}

/*
 * Wait for an interrupt and relay results
 */
int
iewaitintr(es)
	register struct ie_softc *es;
{
	register struct obie_device *obie = es->es_obie;
	register struct mie_device *mie = es->es_mie;
	int ok;

	switch (es->es_type) {
	case IE_OB:
		CDELAY(obie->obie_intr, IEDELAY);
		ok = obie->obie_intr;
		break;

	case IE_MB:
		CDELAY(mie->mie_intr, IEDELAY);
		ok = mie->mie_intr;
		break;
	}
	return (ok);
}

/*
 * Cut the chip out of the loop and halt it by starting the reset cycle.
 * For IE_MB, we must enable pagemaps, hence we complete the reset cycle.
 */
iechipreset(es)
	register struct ie_softc *es;
{

	switch (es->es_type) {
	case IE_MB:
		es->es_mie->mie_reset = 1;
		DELAY(IEKLUDGE);			/* REQUIRED */
		*(char *)&es->es_mie->mie_status = 0;	/* power on reset */
		break;

	case IE_OB:
		*(char *)es->es_obie = 0;		/* power on reset */
		break;

	default:
		panic("iechipreset");
	}
}

/*
 * Splice the chip into the loop
 */
iesplice(es)
	register struct ie_softc *es;
{
	switch (es->es_type) {
	case IE_MB:
		es->es_mie->mie_ie = 1;		/* enable chip interrupts */
		es->es_mie->mie_pie = 1;	/* enable parity interrupts */
		es->es_mie->mie_noloop = 1;	/* enable cable */
		break;

	case IE_OB:
		es->es_obie->obie_ie = 1;	/* enable interrupts */
		es->es_obie->obie_noloop = 1;	/* enable cable */
		break;
	}
}

/* 
 * Change 68000 address to Intel 24-bit address.
 * We take advantage of the fact that all 82586 blocks with 24-bit
 * addresses have an adjacent unused 8-bit field, and store 32 bits.
 */
ieaddr_t
to_ieaddr(es, cp)
	struct ie_softc *es;
	caddr_t cp;
{
	union {
		int	n;
		char	c[4];
	} a, b;

	a.n = ieaddr(es, cp);		/* necessary for double mapping */
	b.c[0] = a.c[3];
	b.c[1] = a.c[2];
	b.c[2] = a.c[1];
	b.c[3] = 0;
	return (b.n);
}

caddr_t
from_ieaddr(es, n)
	struct ie_softc *es;
	ieaddr_t n;
{
	union {
		int	n;
		char	c[4];
	} a, b;

	a.n = n;
	b.c[0] = 0;
	b.c[1] = a.c[2];
	b.c[2] = a.c[1];
	b.c[3] = a.c[0];
	return (es->es_memspace + b.n);
}

ieoff_t
to_ieoff(es, addr)
	register struct ie_softc *es;
	caddr_t addr;
{
	union {
		ieoff_t	s;
		char	c[2];
	} a, b;

	a.s = (ieoff_t)(addr - es->es_base);
	b.c[0] = a.c[1];
	b.c[1] = a.c[0];
	return (b.s);
}

caddr_t
from_ieoff(es, off)
	register struct ie_softc *es;
	ieoff_t off;
{
	union {
		ieoff_t	s;
		char	c[2];
	} a, b;

	a.s = off;
	b.c[0] = a.c[1];
	b.c[1] = a.c[0];
	return (es->es_base + b.s);
}

ieint_t
to_ieint(n)
	ieint_t n;
{
	union {
		ieint_t	s;
		char	c[2];
	} a, b;

	a.s = n;
	b.c[0] = a.c[1];
	b.c[1] = a.c[0];
	return (b.s);
}
#endif NIE > 0
