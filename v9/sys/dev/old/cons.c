/*
 * SUN-3 console driver
 */
#include "../h/param.h"
#include "../h/inode.h"
#include "../h/stream.h"
#include "../h/ttyio.h"
#include "../h/ttyld.h"
#include "../h/conf.h"
#include "../machine/sunromvec.h"

#define	TIMEOUT	04

short	cnstate;
struct	queue	*cnrq;
struct	queue	*cnwq;

int	cnopen(), cnclose(), cnoput(), nodev();

static struct qinit cnrinit = { nodev, NULL, cnopen, cnclose, 0, 0 };
static struct qinit cnwinit = { cnoput, NULL, cnopen, cnclose, 200, 100 };
struct streamtab cninfo = { &cnrinit, &cnwinit };

cnopen(qp, dev)
register struct queue *qp;
{
	cnrq = qp;
	cnwq = WR(qp);
	return(1);
}

cnclose(qp)
{
	cnrq = NULL;
	cnwq = NULL;
}

/*
 * Console write put routine
 */
cnoput(q, bp)
register struct queue *q;
register struct block *bp;
{
	register union stmsg *sp;

	switch(bp->type) {

	case M_IOCTL:
		sp = (union stmsg *)bp->rptr;
		switch (sp->ioc0.com) {

		case TIOCGDEV:
			sp->ioc3.sb.ispeed =
			  sp->ioc3.sb.ospeed = B9600;
			bp->type = M_IOCACK;
			qreply(q, bp);
			return;
		case TIOCSDEV:
			bp->wptr = bp->rptr;
			bp->type = M_IOCACK;
			qreply(q, bp);
			return;
		default:
			bp->type = M_IOCNAK;
			bp->wptr = bp->rptr;
			qreply(q, bp);
			return;
		}

	case M_STOP:
		cnstate |= TTSTOP;
		break;

	case M_START:
		cnstate &= ~TTSTOP;
		cnstart();
		break;

	case M_FLUSH:
		flushq(q, 0);
		break;

	case M_DELAY:
	case M_DATA:
		putq(q, bp);
		cnstart();
		return;
	
	default:
		break;
	}
	freeb(bp);
}

/*
 * Console receive interrupt
 */
/*ARGSUSED*/
/*
cnrint(dev)
{
	register int c;

	c = mfpr(RXDB);
	if (c&RXDB_ID || cnrq==NULL)
		return;
	if ((cnrq->next->flag & QFULL) == 0)
		putd(cnrq->next->qinfo->putp, cnrq->next, c);
}
*/

cntime()
{
	cnstate &= ~TIMEOUT;
	cnstart();
}

cnstart()
{
	register s;
	register struct block *bp;
	register u_char *cp;

	if (cnwq==NULL)
		return;
	s = spl1();
	while ((cnstate & (TIMEOUT|TTSTOP))==0 && cnwq->count) {
		bp = getq(cnwq);
		switch (bp->type) {

		case M_DATA:
			/* Must clear high bit for monitor */
			for(cp = bp->rptr; cp < bp->wptr; cp++)
				*cp &= 0177;
			(*romp->v_fwritestr)(bp->rptr, bp->wptr - bp->rptr,
					romp->v_fbaddr);
			freeb(bp);
			break;

		case M_DELAY:
			timeout(cntime, (caddr_t)NULL, (int)*bp->rptr);
			cnstate |= TIMEOUT;
			freeb(bp);
			splx(s);
			return;
		default:
			freeb(bp);
			break;
		}
	}
	splx(s);
}

/*
 * Print a character on console.
 */
cnputc(c)
	register int c;
{
	register int s;

	s = spl7();
	if (c == '\n')
		(*romp->v_putchar)('\r');
	(*romp->v_putchar)(c);
	(void) splx(s);
}

cngetc()
{
	register int c;

	while ((c = (*romp->v_mayget)()) == -1)
		;
	return (c);
}
