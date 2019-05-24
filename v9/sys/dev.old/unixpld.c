/*
 *  Raw line discipline and communications with unixp datakit controller
 */
#include "xp.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/stream.h"
#include "../h/dkio.h"
#include "../h/pte.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/ubavar.h"
#include "../h/conf.h"
#include "../h/dkstat.h"
#include "../h/dkmod.h"
#include "sparam.h"
#include "dk.h"
#include "kdi.h"
#include "kmc.h"

/* channel states */
#define	CLOSED	0
#define	RCLOSE	1
#define	LCLOSE	2
#define	OPEN	3

/*
 * Unixp message structure
 */
struct dialin {
	char	type;
	char	srv;
	short	param0;
	short	param1;
	short	param2;
	short	param3;
	short	param4;
};

/*
 * Message codes
 */
#define	T_SRV	1
#define	T_REPLY	2
#define	T_CHG	3
#define	T_ALIVE	4
#define	T_RESTART 8
#define	D_CLOSE	1
#define	D_ISCLOSED 2
#define	D_CLOSEALL 3
#define	D_OK	1
#define	D_OPEN	2
#define D_FAIL	3

/*
 * Raw-mode line discipline for DK (only)
 */
int	nulldev(), xpopen(), xpclose(), xpiput(), xpoput(), xposrv();
static	struct qinit xprinit = { xpiput, NULL, xpopen, xpclose, 72, 36};
static	struct qinit xpwinit = { xpoput, xposrv, xpopen, nulldev, 72, 36};
struct	streamtab xpinfo = { &xprinit, &xpwinit };

extern	struct	dkstat dkstat;
#define	NDKMOD	(NDK+NKMC)	/* number of different DK devices */
struct	dkmodule dkmod[NDKMOD];

#define	NDKRAW	32
struct	xp {
	struct	queue *q;
	struct	dkmodule *module;
	short	chan;
} xp[NDKRAW];

xpopen(q, dev)
register struct queue *q;
{
	static timer_on = 0;
	register struct xp *dkp, *edkp;
	register struct dkmodule *dkm;

	edkp = NULL;
	for (dkp=xp; ; dkp++) {
		if (dkp >= &xp[NDKRAW])
			break;
		if (dkp->q == q) {
			if (dkp->chan != minor(dev) || WR(q)->ptr==NULL) {
				printf("q %x dkp %x\n", q, dkp);
				panic("xpopen botch");
			}
			return(1);
		}
		if (dkp->q == NULL && edkp==NULL)
			edkp = dkp;
	}
	if (edkp==NULL)
		return(0);
	for (dkm=dkmod; ; dkm++) {
		if (dkm >= &dkmod[NDKMOD])
			return(0);
		if (major(dev) == dkm->dev)
			break;
	}
	edkp->q = q;
	edkp->chan = minor(dev);
	edkp->module = dkm;
	q->flag |= QDELIM;
	q->ptr = 0;
	WR(q)->ptr = (caddr_t)edkp;
	if (timer_on==0) {
		xptimer();
		timer_on = 1;
	}
	return(1);
}

xpclose(q)
register struct queue *q;
{
	register struct block *bp;
	register struct xp *dkp = (struct xp *)WR(q)->ptr;

if (dkp==NULL) printf("null ptr; q %x wq %x\n", q, WR(q));
	if (WR(q)==dkp->module->listnrq)
		dkp->module->listnrq = NULL;
	while (bp = getq(q))
		(*q->next->qinfo->putp)(q->next, bp);
	dkp->q = NULL;
	dkp->module = NULL;
}

/*
 * output put procedure.
 */
xpoput(q, bp)
register struct queue *q;
register struct block *bp;
{
	register struct xp *dkp = (struct xp *)q->ptr;
	register struct dkmodule *modp = dkp->module;
	register i;
	struct ioctl1 {		/* shouldn't be here */
		int	com;
		int	stuff;
	};

	switch (bp->type) {

	case M_IOCTL:
		switch (((struct ioctl1 *)bp->rptr)->com) {

		/* hang everybody up */
		case DIOCHUP:
			/* must be on channel 1 or 0 */
			if (dkp->chan <= 1) {
				xpmesg(modp, T_ALIVE, 0, 0);
				bp->type = M_IOCACK;
				bp->rptr = bp->wptr;
				qreply(q, bp);
				return;
			}
			break;

		/* announce listener channel */
		case DIOCLHN:
			if (modp->listnrq == NULL) {
				modp->listnrq = q;
				xpmesg(modp, T_ALIVE, 0, 0);
				bp->type = M_IOCACK;
				bp->rptr = bp->wptr;
				qreply(q, bp);
				return;
			}
			break;

		/* delay input */
		case DIOCSTOP:
			RD(q)->ptr = (caddr_t)1;
			bp->type = M_IOCACK;
			bp->rptr = bp->wptr;
			qreply(q, bp);
			return;

		/* suggest a free outgoing channel */
		case DIOCCHAN:
			for (i=2; i<modp->nchan; i++) {
				if (modp->dkstate[i]==CLOSED) {
					((struct ioctl1 *)bp->rptr)->stuff = i;
					bp->wptr =
					     bp->rptr+sizeof(struct ioctl1);
					bp->type = M_IOCACK;
					break;
				}
			}
			qreply(q, bp);
			return;

		default:
			(*q->next->qinfo->putp)(q->next, bp);
			return;
		}
		bp->type = M_IOCNAK;
		bp->wptr = bp->rptr;
		qreply(q, bp);
		return;
	}
	putq(q, bp);
}

xposrv(q)
register struct queue *q;
{
	register struct block *bp;

	while ((q->next->flag&QFULL)==0 && (bp = getq(q)))
		(*q->next->qinfo->putp)(q->next, bp);
}

/*
 * input put procedure
 *   -- ignores all control bytes
 */
xpiput(q, bp)
register struct queue *q;
register struct block *bp;
{
	register struct xp *dkp = (struct xp *)WR(q)->ptr;

	switch (bp->type) {

	case M_DATA:
		if (WR(q)==dkp->module->listnrq && xplstnr(dkp->module, bp))
			return;
		if (q->next->flag&QFULL) { /* sorry, you lose */
			freeb(bp);
			return;
		}
		if (q->ptr) {	/* input delayed */
			putq(q, bp);
			qpctl(q, M_DELIM);
			return;
		}
		(*q->next->qinfo->putp)(q->next, bp);
		putctl(q->next, M_DELIM);
		return;

	case M_IOCACK:
	case M_IOCNAK:
	case M_HANGUP:
		(*q->next->qinfo->putp)(q->next, bp);
		return;

	case M_CLOSE:
		if (dkp->module->dkstate[*bp->rptr] == RCLOSE)
			xpmesg(dkp->module, T_CHG, D_ISCLOSED, *bp->rptr);
		else
			xpmesg(dkp->module, T_CHG, D_CLOSE, *bp->rptr);
		freeb(bp);
		return;

	default:
		freeb(bp);
	}
}

/*
 * listener sends a message to CMC
 */
xpmesg(modp, type, srv, p0)
register struct dkmodule *modp;
{
	register struct dialin *dp;
	register struct block *bp;

	if (modp->listnrq==NULL || modp->listnrq->next->flag&QFULL)
		return;
	if ((bp = allocb(sizeof(struct dialin))) == NULL)
		return;		/* hope it will be retried later */
	dp = (struct dialin *)bp->wptr;
	dp->type = type;
	dp->srv = srv;
	dp->param0 = p0;
	dp->param1 = 0;
	dp->param2 = 0;
	dp->param3 = 0;
	dp->param4 = 0;
	bp->wptr += sizeof(struct dialin);
	(*modp->listnrq->next->qinfo->putp)(modp->listnrq->next, bp);
	putctl(modp->listnrq->next, M_DELIM);
}

/*
 * Receive message for listener
 */
xplstnr(modp, bp)
register struct block *bp;
struct dkmodule *modp;
{
	register struct dialin *dialp;
	register i;
	register struct queue *listnrq = modp->listnrq;
	register struct xp *dkp;

	dialp = (struct dialin *)bp->rptr;
	switch (dialp->type) {

	case T_CHG:
		i = dialp->param0;
		if (i <= 0 || i >= modp->nchan) {
			dkstat.chgstrange++;
			if (dialp->srv)
				xpmesg(modp, T_CHG, D_ISCLOSED, i);
			freeb(bp);
			return(1);
		}
		switch (dialp->srv) {

		case D_CLOSE:		/* remote shutdown */
			switch (modp->dkstate[i]) {

			case RCLOSE:
				dkstat.notclosed++;
			case OPEN:
				putctl1(listnrq->next, M_CLOSE, i);
				break;

			case LCLOSE:
			case CLOSED:
				xpmesg(modp, T_CHG, D_ISCLOSED, i);
				putctl1(listnrq->next, M_CLOSE, i);
				break;
			}
			break;
		
		case D_ISCLOSED:
			switch (modp->dkstate[i]) {

			case LCLOSE:
			case CLOSED:
				putctl1(listnrq->next, M_CLOSE, i);
				break;

			case OPEN:
			case RCLOSE:
				dkstat.isclosed++;
				break;
			}
			break;
		}
		freeb(bp);
		return(1);

	case T_REPLY:	/* CMC sends reply; find, and hand to process */
		i = dialp->param0;
		if (i < 0 || i >= modp->nchan)
			return(0);
		for (dkp=xp; dkp<&xp[NDKRAW]; dkp++) {
			if (dkp->module==modp && dkp->chan==i) {
				(*dkp->q->next->qinfo->putp)(dkp->q->next, bp);
				putctl(dkp->q->next, M_DELIM);
				return(1);
			}
		}
		return(0);

	default:
		return(0);
	}
}

/*
 * 15-second timer
 */
xptimer()
{
	register i;
	register struct dkmodule *dkp;

	for (dkp = dkmod; dkp < &dkmod[NDKMOD]; dkp++) {
		if (dkp->listnrq) {
			xpmesg(dkp, T_ALIVE, 0, 0);
			for (i=0; i< dkp->nchan; i++)
				if (dkp->dkstate[i] == LCLOSE)
					xpmesg(dkp, T_CHG, D_CLOSE, i);
		}
	}
	timeout(xptimer, (caddr_t)NULL, 15*hz);
}
