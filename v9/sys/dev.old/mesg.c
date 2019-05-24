/*
 *  message processor-- written data turns into control, read control
 *   turns into data
 *  rmesg is just the opposite
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/stream.h"
#include "../h/conf.h"
#include "mesg.h"

#define	MS_OPEN	01
#define	MS_DEL	02

#define	MS_STOP	010
#define	MS_IOCTL 020

struct block *msgcollect();
int	msctodput(), msctodsrv(), msdtocput(), msdtocsrv(), msgopen(), rmsgopen(),
	msgclose(), rmsgclose();
struct qinit msgrinit = { msctodput, msctodsrv, msgopen, msgclose, 128, 65 };
struct qinit msgwinit = { msdtocput, msdtocsrv, msgopen, msgclose, 128, 65 };
struct streamtab msginfo = { &msgrinit, &msgwinit };

struct qinit rmsgrinit = { msdtocput, msdtocsrv, rmsgopen, rmsgclose, 128, 65 };
struct qinit rmsgwinit = { msctodput, msctodsrv, rmsgopen, rmsgclose, 128, 65 };
struct streamtab rmsginfo = { &rmsgrinit, &rmsgwinit };

struct imesg {
	char type;
	char msflag;
	short size;
} mesg[NMESG];

msgopen(q, dev)
register struct queue *q;
{
	register struct imesg *mp;

	if (WR(q)->ptr)
		return(1);
	for (mp = mesg; mp->msflag&MS_OPEN; mp++)
		if (mp >= &mesg[NMESG-1])
			return(0);
	mp->msflag = MS_OPEN;
	mp->size = 0;
	WR(q)->ptr = (caddr_t)mp;
	WR(q)->flag |= QNOENB;
	q->flag |= QDELIM;
	return(1);
}

rmsgopen(q, dev)
register struct queue *q;
{
	register struct imesg *mp;

	if (q->ptr)
		return(1);
	for (mp = mesg; mp->msflag&MS_OPEN; mp++)
		if (mp >= &mesg[NMESG-1])
			return(0);
	mp->msflag = MS_OPEN;
	mp->size = 0;
	q->ptr = (caddr_t)mp;
	q->flag |= QNOENB;
	q->flag |= QDELIM;
	return(1);
}

msgclose(q)
register struct queue *q;
{
	if (WR(q)->ptr)
		((struct imesg *)WR(q)->ptr)->msflag = 0;
}

rmsgclose(q)
register struct queue *q;
{
	if (q->ptr)
		((struct imesg *)q->ptr)->msflag = 0;
}

msctodput(q, bp)
register struct queue *q;
register struct block *bp;
{
	register struct imesg *mp = (struct imesg *)(OTHERQ(q)->ptr);
	register struct queue *bq = backq(q);

	/* propagate changes in delimiter status */
	if (mp->msflag&MS_DEL) {
		if ((bq->flag&QDELIM) == 0) {
			mp->msflag &= ~MS_DEL;
			putctl(q, M_NDEL);
		}
	} else {
		if (bq->flag&QDELIM) {
			mp->msflag |= MS_DEL;
			putctl(q, M_YDEL);
		}
	}
	if (bp->type==M_STOP) {
		freeb(bp);
		mp->msflag |= MS_STOP;
		return;
	}
	if (bp->type>=QPCTL) {		/* including M_START */
		if (bp->type==M_START)
			freeb(bp);
		else {
			/* ioctl transparency */
			if (mp->msflag & MS_IOCTL
			 && (bp->type==M_IOCACK || bp->type==M_IOCNAK)) {
				(*q->next->qinfo->putp)(q->next, bp);
				mp->msflag &=~ MS_IOCTL;
				return;
			}
			putq(q, bp);
		}
		mp->msflag &=~ MS_STOP;
		qenable(q);
		return;
	}
	putq(q, bp);
	/* enable if we don't have delimiters */
	if ((bq->flag&QDELIM) == 0)
		qenable(q);
	return;
}

msctodsrv(q)
register struct queue *q;
{
	register struct block *bp, *hbp;
	register struct imesg *mp = (struct imesg *)OTHERQ(q)->ptr;
	register type;
	register size;

	for (;;) {
		if (q->next->flag & QFULL || mp->msflag & MS_STOP)
			return;
		if ((bp = getq(q)) == NULL)
			return;
		if ((hbp = allocb(MSGHLEN)) == NULL) {
			putbq(q, bp);
			return;
		}
		((struct mesg *)(hbp->wptr))->magic = MSGMAGIC;
		((struct mesg *)(hbp->wptr))->type = type = bp->type;
		size = bp->wptr - bp->rptr;
		((struct mesg *)(hbp->wptr))->losize = size;
		((struct mesg *)(hbp->wptr))->hisize = size>>8;
		hbp->wptr += MSGHLEN;
		(*q->next->qinfo->putp)(q->next, hbp);
		bp->type = M_DATA;
		if (bp->wptr > bp->rptr)
			(*q->next->qinfo->putp)(q->next, bp);
		else
			freeb(bp);
		putctl(q->next, M_DELIM);
		if (type == M_HANGUP)
			putctl(q->next, M_HANGUP);
	}
}

msdtocput(q, bp)
register struct queue *q;
register struct block *bp;
{
	switch (bp->type) {

	default:
		freeb(bp);
		return;

	case M_FLUSH:
		flushq(OTHERQ(q), 1);
	case M_IOCACK:
	case M_IOCNAK:
	case M_HANGUP:
		(*q->next->qinfo->putp)(q->next, bp);
		return;

	case M_DATA:
	case M_IOCTL:
		putq(q, bp);
		qenable(q);
		return;
	}
}

msdtocsrv(q)
register struct queue *q;
{
	register struct block *bp;
	register struct imesg *mp = (struct imesg *)q->ptr;

	for (;;) {
		if (q->next->flag & QFULL)
			return;
		if (mp->size == 0) {		/* Start of message */
			bp = msgcollect(q, MSGHLEN, 0, 1);
			if (bp == NULL)
				return;
			mp->size = ((struct mesg *)bp->rptr)->losize;
			mp->size += ((struct mesg *)bp->rptr)->hisize<<8;
			mp->type = ((struct mesg *)bp->rptr)->type;
			/* magic ok; was checked in msgcollect */
			if (mp->size < 0)
				mp->size = 0;
			if (mp->size==0) {
				bp->type = mp->type;
				bp->rptr = bp->wptr;
				if (bp->type==M_YDEL)
					q->flag |= QDELIM;
				else if (bp->type==M_NDEL)
					q->flag &= ~QDELIM;
				(*q->next->qinfo->putp)(q->next, bp);
				continue;
			}
			freeb(bp);
		}
		bp = msgcollect(q, mp->size, mp->type == M_DATA, 0);
		if (bp == NULL)
			return;
		bp->type = mp->type;
		mp->size -= bp->wptr - bp->rptr;
		(*q->next->qinfo->putp)(q->next, bp);
	}
}

long ms_badmag;

struct block *
msgcollect(q, size, isdata, findmag)
register struct queue *q;
{
	register struct block *nbp, *bp;
	register ninb;
	register struct imesg *mp = (struct imesg *)q->ptr;

	if (findmag == 0) {
		if ((bp = getq(q)) == NULL)
			return (NULL);
	}
	else {
		while ((bp = getq(q)) != NULL) {
			if (bp->type != M_DATA) {
				/* prevent the ack from being turned to data */
				if (bp->type==M_IOCTL)
					mp->msflag |= MS_IOCTL;
				(*q->next->qinfo->putp)(q->next, bp);
				continue;
			}
			while (bp->rptr < bp->wptr-1) {
				if (bp->rptr[1] == MSGMAGIC)
					goto gotmagic;
				bp->rptr++;
				ms_badmag++;
			}
			freeb(bp);
		}
		if (bp == NULL)
			return (NULL);
	}
gotmagic:
	nbp = allocb(size);
	if (nbp == NULL) {
		putbq(q, bp);
		return(NULL);
	}
	if (size > nbp->lim - nbp->wptr)
		size = nbp->lim - nbp->wptr;
	while (size) {
		if (bp->type != M_DATA) {
			/* prevent the ack from being turned to data */
			if (bp->type==M_IOCTL)
				mp->msflag |= MS_IOCTL;
			(*q->next->qinfo->putp)(q->next, bp);
			if ((bp = getq(q)) == NULL)
				break;
			continue;
		}
		ninb = bp->wptr - bp->rptr;
		if (ninb > size) {
			bcopy((caddr_t)bp->rptr, (caddr_t)nbp->wptr, size);
			nbp->wptr += size;
			bp->rptr += size;
			size = 0;
			putbq(q, bp);
			break;
		}
		bcopy((caddr_t)bp->rptr, (caddr_t)nbp->wptr, ninb);
		size -= ninb;
		nbp->wptr += ninb;
		freeb(bp);
		if (size == 0)
			break;
		if ((bp = getq(q)) == NULL)
			break;
	}
	if (nbp->rptr >= nbp->wptr) {
		freeb(nbp);
		return(NULL);
	}
	if (size==0 || isdata)
		return(nbp);
	putbq(q, nbp);
	return(NULL);
}
