/************************************************************************
 *									*
 *			Copyright (c) 1985 by				*
 *		Digital Equipment Corporation, Maynard, MA		*
 *			All rights reserved.				*
 *		   If you touch this presotto, you DIE!			*
 ************************************************************************/

/*
 * DEQNA Ethernet Communications Controller interface.
 * Provides fairly raw access to a number of controllers.
 * Minor device N talks to unit N / 8. Each written record should
 * be an interlan output packet - 6 bytes addr, 2 bytes type, data.
 * An ethernet packet type may be associated with a minor device with
 * the ENIOTYPE ioctl; input packets of that type will be sent to
 * the minor device in question. Input packets include 6 bytes src addr,
 * 6 bytes dest, 2 bytes type, and data.
 *
 * The physical address of a controller may be fetched with the ENIOADDR
 * ioctl on a minor device associated with that unit.
 */

/*
 * NB: Both transmit and receive buffer descriptor lists are rings.  The
 *     last descriptor in each list points to the first.  To prevent the
 *     device from overrunning the software, one descriptor in each ring
 *     is left invalid.
 */

#include "qe.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/stream.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/ubavar.h"
#include "../h/conf.h"
#include "../h/enio.h"
#include "../h/ttyio.h"
#include "../h/ttyld.h"
#include "../h/deqna.h"
#include "../h/ethernet.h"

/* for address calcualations of stream blocks */
extern u_char blkdata[];	/* stream.c */
extern long blkubad;

/*
 * This constant should really be 60 because the qna adds 4 bytes of crc.
 * However when set to 60 our packets are ignored by deuna's , 3coms are
 * okay ??????????????????????????????????????????
 */
#define ETHERMINTU 64
#define ETHERMAXTU 1564

/*
 * The number of recieve packets is set to minimize the possibility
 * of incuring a race condition between the hardware and software.
 */
#define NRCV	12	 		/* Receive descriptors		*/
#define NXMT	12	 		/* Transmit descriptors		*/
#define NTOT	(NXMT + NRCV)
#define MAX_RCV_BYTES 3*ETHERMAXTU	/* maximum bytes in receive ring */
int	nNQE = NQE;
int	nNRCV = NRCV;
int	nNXMT = NXMT;
int	nNTOT = NTOT;

/* mask for setting deqna deqna csr */
int qebits = QE_RCV_ENABLE|QE_INT_ENABLE|QE_XMIT_INT|QE_RCV_INT|QE_ILOOP;

/*  One of these structures exists per minor device number (stream) */
#define CHANS_PER_UNIT	8
struct qechan{
	int unit;			/* device unit number */
	struct queue *rq;		/* stream queues */
	int type;			/* ethernet protocol # */
	struct block *xmt[NXMT];	/* next packet to xmt (null terminated) */
	int nxmt;
};
struct qechan *qefindchan();

/*  One of these structures exists per device */
struct	qe {
	/* statistics */
	int	ierrors,oerrors;	/* input/output errors		*/
	int	ipackets,opackets;	/* input/output packets		*/

	/* for xmit/rcv */
	int	rindex;			/* Receive index		*/
	int	orindex;
	int	xindex;			/* Transmit index		*/
	int	oxindex;
	int	rbytes;			/* bytes in receive buffer	*/
	int	status;
	struct	qe_ring rring[NRCV+1];	/* Receive ring descriptors 	*/
	struct	qe_ring xring[NXMT+1];	/* Transmit ring descriptors 	*/
	struct	block *rbp[NRCV];	/* receive blocks 		*/
	struct	block *xbp[NXMT];	/* transmit blocks		*/
	struct	qe_ring *rringaddr;	/* mapping info for rings	*/
	struct	qe_ring *xringaddr;	/*       ""			*/

	/* channel info */
	int	lastch;			/* next channel to scan		*/
	struct qechan chan[CHANS_PER_UNIT];

	/* device parammeters */
	int	qe_intvec;		/* Interrupt vector 		*/
	struct	qedevice *addr;		/* device addr			*/
	u_char	setup_pkt[16][8];	/* Setup packet			*/
} qe[NQE];

#define NEXTCH(c) (((c)+1)%CHANS_PER_UNIT)	/* next channel */

/* flag settings */
#define ATTACHED	0x4
#define SETUPQD		0x8

/* handy macros for ring management */
#define NFREE(s) ((s)->xindex>=(s)->oxindex?(NXMT-((s)->xindex-(s)->oxindex)-1):\
					    ((s)->oxindex-(s)->xindex-1))
#define NEXTRCV(c) (((c)+1)%NRCV)
#define NEXTXMT(c) (((c)+1)%NXMT)

/* uba structure */
int	qeprobe(), qeattach();
u_short qestd[] = { 0 };
struct	uba_device *qeinfo[NQE];
struct	uba_driver qedriver =
	{ qeprobe, 0, qeattach, 0, qestd, "qe", qeinfo };
#define ADDRMASK 0x3ffff

/* Build a setup packet - the physical address will already be present
 * in first column.
 */
qesetup(qp)
	register struct qe *qp;
{
	int i, j, offset = 0, next = 3;

	/*
	 * Copy the target address to the rest of the entries in this row.
	 */
	 for ( j = 0; j < 6 ; j++ )
		for ( i = 2 ; i < 8 ; i++ )
			qp->setup_pkt[j][i] = qp->setup_pkt[j][1];
	/*
	 * Duplicate the first half.
	 */
	bcopy(qp->setup_pkt, qp->setup_pkt[8], 64);
	/*
	 * Fill in the broadcast address.
	 */
	for ( i = 0; i < 6 ; i++ )
		qp->setup_pkt[i][2] = 0xff;
}

/* Invalidate a ring descriptor */
qeinvaldesc(rp)
	register struct qe_ring *rp;
{
	/* invalidate a descriptor */
	bzero(rp, sizeof(struct qe_ring));
}

/* Fill in a descriptor */
qefilldesc(rp, addr, len, setup, eom)
	register struct qe_ring *rp;
	u_long addr;			/* Qbus address */
	int len;
	int setup;			/* true if a setup packet */
	int eom;			/* true if end of message */
{
	/* fill in the descriptor */
	bzero( rp, sizeof(struct qe_ring));
	if ((addr) & 1) {
		/* buffer starts on an odd byte */
		len++;
		rp->qe_odd_begin = 1;
		addr &= 0xfffe;
	}
	if (len & 1) {
		/* buffer ends on an odd byte */
		len++;
		rp->qe_odd_end = 1;
	}
	rp->qe_buf_len = -(len/2);
	addr &= ADDRMASK;
	rp->qe_addr_lo = (short)addr;
	rp->qe_addr_hi = (short)(addr >> 16);
	if (setup)
		rp->qe_setup = 1;
	if (eom)
		rp->qe_eomsg = 1;
	rp->qe_status2 = 1;
	rp->qe_flag = rp->qe_status1 = QE_NOTYET;
	rp->qe_valid = 1;
}

/* set up a receive descriptor */
qercvblock(qp)
	register struct qe *qp;
{
	register long a;
	register int len;
	register struct block *bp;

	/* make sure we don't hog too many buffers or wrap around */
	if (qp->rbytes>MAX_RCV_BYTES || NEXTRCV(qp->rindex)==qp->orindex)
		return -1;

	/* get a block */
	if ((bp = allocb(ETHERMAXTU))==NULL)
		panic("qebinitdesc: no blocks");
	qp->rbp[qp->rindex] = bp;

	/* translate the block data address to a q-bus address */
	if((unsigned)(bp->rptr) < (unsigned)blkdata){
		printf("memaddr %x blkdata %x\n", bp->rptr, blkdata);
		panic("qebinitdesc: translation");
	}
	a = (long)(bp->rptr)-(long)blkdata+((long)blkubad&ADDRMASK);

	/* fill in the descriptor */
	len = bp->lim - bp->rptr;
	qefilldesc(&qp->rring[qp->rindex], a, len, 0, 0);
	qp->rbytes += len;

	/* make sure there are blocks queued for input */
	/*printf("qercvblock: csr %x\n", qp->addr->qe_csr);*/
	if ((qp->addr->qe_csr&QE_RL_INVALID) &&
	    qp->rring[qp->rindex].qe_status1 == QE_NOTYET) {
		a = (int)&qp->rringaddr[qp->rindex];
		qp->addr->qe_rcvlist_lo = (short)a;
		qp->addr->qe_rcvlist_hi = (short)(a >> 16);
	}
	qp->rindex = NEXTRCV(qp->rindex);
	return 0;
}

/* add a buffer to the transmit ring */
qexmtblock(qp, bp, setup, eom)
	struct qe *qp;
	struct block *bp;
	int setup;		/* true if a setup block */
	int eom;		/* true if end of message */
{
	register long a;
	register int len;

	if (qp->xring[qp->xindex].qe_status1 == QE_NOTYET)
		panic("qexmitbp");
	qp->xbp[qp->xindex] = bp;

	/* translate the block data address to a q-bus address */
	if((unsigned)(bp->rptr) < (unsigned)blkdata){
		printf("memaddr %x blkdata %x\n", bp->rptr, blkdata);
		panic("qebinitdesc: translation");
	}
	a = (long)(bp->rptr)-(long)blkdata+((long)blkubad&ADDRMASK);

	/* fill in the descriptor */
	len = bp->wptr - bp->rptr;
	qefilldesc(&qp->xring[qp->xindex], a, len, setup, eom);

	/* kick device if xmit list is invalid */
	/*printf("qexmtblock: csr %x\n", qp->addr->qe_csr);*/
	if ((qp->addr->qe_csr&QE_XL_INVALID) &&
	    qp->xring[qp->xindex].qe_status1==QE_NOTYET) {
		a = (int)(qp->xringaddr+qp->xindex);
		qp->addr->qe_xmtlist_lo = (short)a;
		qp->addr->qe_xmtlist_hi = (short)(a >> 16);
	}
	qp->xindex = NEXTXMT(qp->xindex);
}

/* put a setup block on the xmt ring */
qexmtsetup(qp)
	register struct qe *qp;
{
	struct block *bp;

	/* get a free block */
	bp = allocb(sizeof(qp->setup_pkt));
	if (bp==NULL)
		panic("qexmtsetup: no setup blocks");
	if (bp->lim - bp->rptr < sizeof(qp->setup_pkt)) {
		freeb(bp);
		panic("qexmtsetup: too small");
	}

	/* and fill it in */
	bcopy(qp->setup_pkt, bp->rptr, sizeof(qp->setup_pkt));
	bp->wptr = bp->rptr + sizeof(qp->setup_pkt);
	qexmtblock(qp, bp, 1, 1);
	return 0;
}

/*  Reset the interface */
qereset(qp)
	register struct qe *qp;
{
	register int i;

	/* software reset */
	qp->addr->qe_csr = QE_RESET;
	/*printf("qereset: csr = %x\n", qp->addr->qe_csr);*/
}

/*  Setup the buffer descriptor lists (into rings) and enable for reception */
qeenable(qp)
	register struct qe *qp;
{
	register int i, ch;

	/* clear out all queued blocks */
	for (i=0; i<NRCV; i++)
		if (qp->rbp[i]) {
			freeb(qp->rbp[i]);
			qp->rbp[i] = NULL;
		}
	for (i=0; i<NXMT; i++)
		if (qp->xbp[i]) {
			freeb(qp->xbp[i]);
			qp->xbp[i] = NULL;
		}
	for (ch=0; ch<CHANS_PER_UNIT; ch++) {
		qp->chan[ch].nxmt = 0;
		for (i=0; i<NXMT; i++)
			if (qp->chan[ch].xmt[i]) {
				freeb(qp->chan[ch].xmt[i]);
				qp->chan[ch].xmt[i] = NULL;
			}
	}

	/*  Invalidate all decriptors and form the xmt and rcv rings */
	for (i = 0 ; i < NRCV ; i++)
		qeinvaldesc(&qp->rring[i]);
	qefilldesc(&qp->rring[i], qp->rringaddr, 0, 0, 0);
	qp->rring[i].qe_chain = 1;
	for (i = 0 ; i < NXMT ; i++)
		qeinvaldesc(&qp->xring[i]);
	qefilldesc(&qp->xring[i], qp->xringaddr, 0, 0, 0);
	qp->xring[i].qe_chain = 1;

	/* initialize ring pointers */
	qp->lastch = qp->orindex = qp->oxindex = qp->xindex = qp->rindex = 0;
	qp->rbytes = 0;

	/* enable interrupts */
	qp->addr->qe_vector = qp->qe_intvec;
	qp->addr->qe_csr = qebits;

	/* set up some buffers */
	while (qercvblock(qp)==0)
		;
	qp->status |= SETUPQD;
	while (qexmtpacket(qp)==0)
		;
	/*printf("qe csr after enable: %x\n", qp->addr->qe_csr);*/
}

/*
 * Probe the device to see if it's there.  We do this by sending a setup
 * packet and waiting for it to loop back.
 */
qeprobe(reg)
	caddr_t reg;
{
	register int br, cvec;		/* r11, r10 value-result */

	register struct qedevice *addr = (struct qedevice *)reg;
	register int i;
	static int next=0;		/* softc index		*/
	register struct qe *qp = &qe[next++];
	u_char *setupaddr, *sa;
	u_short csr;

#ifdef lint
	br = 0; cvec = br; br = cvec;
#endif
	/* zero EVERYTHING */
	bzero(qp, sizeof(struct qe));
	addr->qe_vector = (uba_hd[numuba].uh_lastiv -= 4);
	qp->status = 0;
	qp->addr = addr; /* for qereset */
	qereset(qp);

	/* Map the communications area and the setup packet. */
	setupaddr = (u_char *)
		uballoc(0, qp->setup_pkt, sizeof(qp->setup_pkt), 0);
	sa = (u_char *)((long)setupaddr & ADDRMASK);
	qp->rringaddr = (struct qe_ring *)
		uballoc(0, qp->rring, sizeof(struct qe_ring)*(NTOT+2),0);
	qp->rringaddr = (struct qe_ring *)((int)(qp->rringaddr) & ADDRMASK);
	qp->xringaddr = qp->rringaddr+NRCV+1;

	/*
	 * The QNA will loop the setup packet back to the receive ring
	 * for verification, therefore we initialize the first 
	 * receive & transmit ring descriptors and link the setup packet
	 * to them.
	 */
	qefilldesc(qp->xring, sa, sizeof(qp->setup_pkt), 1, 1);
	qefilldesc(qp->rring, sa, sizeof(qp->setup_pkt), 0, 0);

	/*
	 * Get the addr off of the interface and place it into the setup
	 * packet. This code looks strange due to the fact that the address
	 * is placed in the setup packet in col. major order. 
	 */
	for( i = 0 ; i < 6 ; i++ )
		qp->setup_pkt[i][1] = addr->qe_sta_addr[i];
	qesetup(qp);

	/* Start the interface and wait for the packet. */
	addr->qe_csr = QE_INT_ENABLE | QE_XMIT_INT | QE_RCV_INT;
	addr->qe_rcvlist_lo = (short)qp->rringaddr;
	addr->qe_rcvlist_hi = (short)((int)(qp->rringaddr) >> 16);
	addr->qe_xmtlist_lo = (short)qp->xringaddr;
	addr->qe_xmtlist_hi = (short)((int)(qp->xringaddr) >> 16);
	DELAY(10000);
	csr = addr->qe_csr;

	/* clamp down on the device again */
	qereset(qp);

	/* release the mapping */
	ubarelse(0, &setupaddr);

	addr->qe_vector = uba_hd[numuba].uh_lastiv;
	if (!(csr&(QE_XMIT_INT|QE_RCV_INT))) {
		printf("deqna failed setup: csr = %x\n", csr);
		return 0;
	}
	cvec = addr->qe_vector;
	br = 0x15;
	return 1;
}

/* set up sc[i] to reflect the physical parameters of unit number i*/
qeattach(ui)
	struct uba_device *ui;
{
	register struct qe *qp = &qe[ui->ui_unit];
	register struct qedevice *addr = (struct qedevice *)ui->ui_addr;
	register int i;
	int s=spl6();

	/* Save the vector for initialization at reset time */
	qp->qe_intvec = addr->qe_vector;
	qp->addr = addr;

	/* Read the address from the prom and save it */
	for( i=0 ; i<6 ; i++ )
		qp->setup_pkt[i][1] = addr->qe_sta_addr[i] & 0xff;

	/* close all channels */
	for (i=0; i<CHANS_PER_UNIT; i++) {
		qp->chan[i].nxmt = 0;
		qp->chan[i].rq = NULL;
	}

	/* clear block pointers */
	for (i=0; i<NRCV; i++)
		qp->rbp[i] = NULL;
	for (i=0; i<NXMT; i++)
		qp->xbp[i] = NULL;
	qp->status |= ATTACHED;
	splx(s);
}

/* start transmitting */
qexmtpacket(qp)
	register struct qe *qp;
{
	register int unit = qp - qe;
	register int i;
	register struct qechan *cp;
	int a;

	/* first send a setup packet if requested */
	if (qp->status&SETUPQD) {
		/*printf("qexmtpacket: setup\n");*/
		qp->status &= ~SETUPQD;
		if (NFREE(qp)<1 || qexmtsetup(qp)<0) {
			qp->status |= SETUPQD;
			return -1;		/* no more blocks */
		}
		/*printf("qexmtpacket: setup qd\n");*/
	}

	/* Look for a channel that has a packet that will fit */
	for(i=NEXTCH(qp->lastch); ; i=NEXTCH(i)) {
		cp = &qp->chan[i];
		if (cp->nxmt && cp->nxmt<=NFREE(qp))
			break;
		if (i==qp->lastch)
			return -1;
	}
	qp->lastch = i;

	/* send the packet */
	qedebug(cp->xmt[0], 1);
	for (i=0; i<cp->nxmt-1; i++) {
		qexmtblock(qp, cp->xmt[i], 0, 0);
		cp->xmt[i] = (struct block *)NULL;
	}
	qexmtblock(qp, cp->xmt[i], 0, 1);
	cp->xmt[i] = (struct block *)NULL;
	cp->nxmt = 0;
	qestagepacket(cp);
	return 0;

}

 
/* All device interrupts come here */
qeintr(unit)
	int unit;
{
	register struct qe *qp = &qe[unit];
	int a, csr, s=spl6();

	csr = qp->addr->qe_csr;
	/*printf("qeintr: in csr %x\n", csr);*/
	qp->addr->qe_csr = qebits;
	if( csr & QE_RCV_INT ) 
		qerint(qp);
	if( csr & QE_XMIT_INT )
		qexint(qp);
	if( csr & QE_NEX_MEM_INT )
		printf("qe: Non existant memory interrupt\n");
	/*printf("qeintr: out csr %x\n", qp->addr->qe_csr);*/
	splx(s);
}
 
/*  Take transmitted packets out of the ring.  Since the device cannot receive
 *  its own broadcasts, loop back locally destined packets here.
 */
qexint(qp)
	register struct qe *qp;
{
	register int last;		/* index of last block of packet */
	register int first;		/* index of first block of packet */
	register struct qechan *cp;

	/* loop once per packet */
	for(;;) {
		/* loop once per block */
		first = qp->oxindex;
		for (last=first; ; last=NEXTXMT(last)) {
			if (last==qp->xindex ||
			    qp->xring[last].qe_status1==QE_NOTYET){
				while (qexmtpacket(qp)==0)
					;
				return;
			}
			if (qp->xring[last].qe_eomsg==1)
				break;
		}

		/* statistics */
		if (qp->xring[last].qe_status1&QE_ERROR) {
			printf("qexint: failure status1 %x status2 %x\n",
				qp->xring[last].qe_status1,
				qp->xring[last].qe_status2);
			qp->oerrors++;
		} else {
			qp->opackets++;
		}

		/* find correct channel (for possible loop back) */
		if (qebits&QE_ELOOP)
			cp = NULL;
		else
			cp = qefindchan(qp, qp->xbp[first], 1);

		/* loop once per block to free (or loop back) packet */
		for(last=NEXTXMT(last); first!=last; first=NEXTXMT(first)) {
			/*printf("qexint: removing %d to %x\n", first, cp);*/
			if (cp)
				(*cp->rq->next->qinfo->putp)(cp->rq->next,qp->xbp[first]);
			else
				freeb(qp->xbp[first]);
			qeinvaldesc(&qp->xring[first]);
			qp->xbp[first] = NULL;
		}
		qp->oxindex = last;

		/* add a delim */
		if (cp) {
			struct block *bp = allocb(0);
			if (bp==NULL)
				panic("qexint: out of delim blocks");
			bp->type = M_DELIM;
			(*cp->rq->next->qinfo->putp)(cp->rq->next, bp);
		}
	}
}
 
/*  Take received packets off the ring and pass them the appropriate
 *  input channel.
 */
qerint(qp)
	register struct qe *qp;
{
	register int last;		/* index of last block of packet */
	register int first;		/* index of first block of packet */
	register struct qechan *cp;
	struct qe_ring *rp;
	struct block *bp;
	int len;

	/* loop once per packet */
	for(;;) {
		/* loop once per block */
		first = qp->orindex;
		for (last=first; ; last=NEXTRCV(last)) {
			if (last==qp->rindex ||
			    qp->rring[last].qe_status1==QE_NOTYET) {
				/* put more buffer space on the ring */
				while(qercvblock(qp)==0)
					;
				return;
			}
			if (!(qp->rring[last].qe_status1&QE_LASTNOT))
				break;
		}

		/* statistics */
		if (qp->rring[last].qe_status1&QE_ERROR) {
			/* printf("qerint: failure\n");*/
			cp = NULL;
			qp->oerrors++;
		} else if (qp->rring[last].qe_status1&QE_ESETUP) { 
			/* printf("qerint: setup\n");*/
			cp = NULL;
		} else {
			cp = qefindchan(qp, qp->rbp[first], 0);
			qp->opackets++;
		}

		/* loop once per block to free (or loop back) packet */
		if (cp) {
			qedebug(qp->rbp[first], 1);
			rp = &qp->rring[last];
			len = ((rp->qe_status1&QE_RBL_HI)|
			      (rp->qe_status2&QE_RBL_LO))+60;
		}
		for(last=NEXTRCV(last); first!=last; first=NEXTRCV(first)) {
			/*printf("qerint: received %d to %x\n", first, cp);*/
			bp = qp->rbp[first];
			qp->rbytes -= bp->lim - bp->rptr;
			if (cp) {
				rp = &qp->rring[first];
				if (rp->qe_status1&QE_LASTNOT) {
					bp->wptr = bp->lim;
					len -= bp->lim-bp->rptr;
				} else {
					bp->wptr = bp->rptr + len;
					if (bp->wptr > bp->lim)
						panic("qerint: length");
				}
				(*cp->rq->next->qinfo->putp)(cp->rq->next, bp);
			} else
				freeb(bp);
			qeinvaldesc(&qp->rring[first]);
			qp->rbp[first] = NULL;
		}

		/* add a delim */
		if (cp) {
			struct block *bp = allocb(0);
			if (bp==NULL)
				panic("qerint: out of delim blocks");
			bp->type = M_DELIM;
			(*cp->rq->next->qinfo->putp)(cp->rq->next, bp);
		}
		qp->orindex = last;
	}
}

int	nodev(), qeopen(), qeclose(), qeput();
struct	qinit qerinit = { nodev, NULL, qeopen, qeclose, 0, 0 };
struct	qinit qewinit = { qeput, NULL, qeopen, qeclose, 4*ETHERMAXTU, 64 };
struct	streamtab qesinfo = { &qerinit, &qewinit };

/* find the input queue for a packet */
struct qechan *
qefindchan(qp, bp, checklocal)
	register struct qe *qp;
	struct block *bp;
	int checklocal;
{
	register int i;
	register struct etherpup *ep=(struct etherpup *)bp->rptr;
	int ok;

	/*printf("qefindchan: %x,%x,%x,%x,%x,%x:%d\n", 
		ep->dhost[0], ep->dhost[1], ep->dhost[2],
		ep->dhost[3], ep->dhost[4], ep->dhost[5],
		ep->type);*/
	if (checklocal) {
		ok = 3;
		for (i=0; i<6; i++) {
			if (ep->dhost[i]!=0xff)
				ok &= ~1;
			if (ep->dhost[i]!=qp->setup_pkt[i][1])
				ok &= ~2;
			if (!ok)
				return NULL;
		}
	}
	for (i=0; i<CHANS_PER_UNIT; i++)
		if (qp->chan[i].rq!=NULL && qp->chan[i].type==ep->type)
			return(&qp->chan[i]);
	return NULL;
}

/* open a channel */
qeopen(q, dev)
register struct queue *q;
register dev_t dev;
{
	register struct qechan *cp;
	register struct qe *qp;
	int unit, s, ch, chan;

	dev = minor(dev);
	chan = dev % CHANS_PER_UNIT;
	unit = dev / CHANS_PER_UNIT;
	if(unit >= NQE)
		return(0);
	qp = &qe[unit];
	if (!(qp->status&ATTACHED))
		return(0);
	cp = &qp->chan[chan];
	if(cp->rq)
		return(0);
	cp->unit = unit;
	cp->type = 0;
	q->ptr = (caddr_t)cp;
	WR(q)->ptr = (caddr_t)cp;
	WR(q)->flag |= QDELIM|QBIGB;
	q->flag |= QDELIM;
	cp->rq = q;

	/* reset and enable on first open of the device */
	s = spl6();
	for (ch=0; ch < CHANS_PER_UNIT; ch++)
		if (ch!=chan && qp->chan[ch].rq!=NULL)
			break;
	if (ch==CHANS_PER_UNIT) {
		qereset(qp);
		qeenable(qp);
	}
	splx(s);

	return(1);
}

qeclose(q)
register struct queue *q;
{
	register struct qechan *cp;

	cp = (struct qechan *)q->ptr;
	cp->rq = 0;
	cp->type = 0;
}

/* qeput expects the first block of each packet to contain a ethernet header */
qeput(q, bp)
	register struct queue *q;
	struct block *bp;
{
	register struct qechan *cp=(struct qechan *)q->ptr;
	int s;

	if(bp->type == M_DATA){
		putq(q, bp);
		return;
	} else if(bp->type == M_IOCTL){
		qeioctl(q, bp);
		return;
	} else if(bp->type == M_DELIM){
		putq(q, bp);
	} else {
		freeb(bp);
		return;
	}

	/* get the next packet from the queue and start transmitting */
	s = spl6();
	qestagepacket(cp);
	while (qexmtpacket(&qe[cp->unit])==0)
		;
	splx(s);
}

/* Stage the next packet for this channel */
qestagepacket(cp)
	register struct qechan *cp;
{
	register struct block *bp;
	register int nblocks=0;
	register int nbytes=0;

	if (cp->nxmt!=0)
		return;

	/* gather the next packet */
	while(bp = getq(WR(cp->rq))) {
		if (bp->type == M_DATA) {
			if (nblocks < NXMT) {
				cp->xmt[nblocks++] = bp;
				nbytes += bp->wptr-bp->rptr;
			} else {
				printf("qestagepacket: too many blocks\n");
				freeb(bp);
			}
		} else if (bp->type == M_DELIM) {
			struct etherpup *ep;
			struct qe *qp=&qe[cp->unit];
			int i;

			if (nblocks==0) {
				printf("qestagepacket: no data\n");
				break;
			}
			if (nbytes<ETHERMINTU) {
				cp->xmt[nblocks-1]->wptr += ETHERMINTU-nbytes;
			} else if (nbytes>ETHERMAXTU) {
				printf("qestagepacket: too long\n");
				break;
			}
			if (cp->xmt[0]->wptr-cp->xmt[0]->rptr < sizeof(struct etherpup)) {
				printf("qestagepacket: too wierd\n");
				break;
			}
			ep = (struct etherpup *)cp->xmt[0]->rptr;
			ep->type = cp->type;
			for(i=0; i<6; i++)
				ep->shost[i] = qp->setup_pkt[i][1];
			cp->nxmt = nblocks;
			freeb(bp);
			/*printf("qestagepacket: staged %d\n", cp->nxmt);*/
			return;
		} else {
			printf("qestagepacket: bad bp->type\n");
			break;
		}
	}
	/* something is wrong */
	if (bp)
		freeb(bp);
	else if (nblocks!=0)
		printf("qestagepacket: missing delim\n");
	for(--nblocks; nblocks>=0; nblocks--)
		freeb(cp->xmt[nblocks]);
	cp->nxmt = 0;
}

#define QEDEBSIZE 64
struct {
	time_t	time;
	unsigned short code;
	struct etherpup pup;
} qed[QEDEBSIZE];

int qei = 0;

qedebug(bp, code)
register struct block *bp;
{
	qed[qei].time = time;
	qed[qei].code = code;
	bcopy(bp->rptr, &qed[qei].pup, sizeof(qed[qei].pup));
	qei = (qei + 1) % QEDEBSIZE;
}

qeioctl(q, bp)
register struct queue *q;
register struct block *bp;
{
	union stmsg *sp;
	register struct qechan *cp;
	int i;
	char *ap;

	cp = (struct qechan *)q->ptr;
	sp = (union stmsg *)bp->rptr;
	bp->type = M_IOCACK;
	switch(sp->ioc0.com){
	case ENIOTYPE:
		cp->type = *((int *)(sp->iocx.xxx));
		break;
	case ENIOADDR:
		for (ap=sp->iocx.xxx, i=0; i<6; i++) 
			*ap++ = qe[cp->unit].addr->qe_sta_addr[i];
		break;
	default:
		bp->type = M_IOCNAK;
		break;
	}
	qreply(q, bp);
}
