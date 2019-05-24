/*
 * tcp line discipline; only one, to be pushed on /dev/ip6.
 */

#include "tcp.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/stream.h"
#include "../h/inio.h"
#include "../h/ttyio.h"
#include "../h/ttyld.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/ubavar.h"
#include "../h/conf.h"

#include "../h/inet/mbuf.h"
#include "../h/inet/in.h"
#include "../h/inet/ip.h"
#include "../h/inet/ip_var.h"
#include "../h/inet/tcp.h"
#include "../h/inet/tcp_fsm.h"
#include "../h/inet/tcp_seq.h"
#include "../h/inet/tcp_timer.h"
#include "../h/inet/tcp_var.h"
#include "../h/inet/tcpip.h"

extern int tcp_busy;	/* set to discourage timers */
int tcp_maxseg = 512;	/* default that doesn't last long */
struct queue *tcpqueue;

int	tcpopen(), tcpiput(), tcpisrv(), tcpclose();
int	tcposrv();
static	struct qinit tcprinit = { tcpiput, tcpisrv, tcpopen, tcpclose, 4096, 64 };
static	struct qinit tcpwinit = { putq, tcposrv, tcpopen, tcpclose, 512, 64 };
struct streamtab tcpinfo = { &tcprinit, &tcpwinit };

tcpopen(q, dev)
register struct queue *q;
{
	static int timing;
	extern int rbsize[];

	if (q->ptr)
		return(0);
	tcpqueue = q;	/* RD queue */
	if(!timing){
		timing = 1;
		tcp_fasttimo();
		tcp_slowtimo();
	}
	q->flag |= QDELIM;
	WR(q)->flag |= QDELIM;
	q->ptr = (caddr_t)1;
	WR(q)->ptr = (caddr_t)1;
	q->flag |= QNOENB;	/* ipiput calls qenable() */
	tcp_maxseg = rbsize[3]-sizeof(struct tcpiphdr);	/* to fit in a BLKBIG */
	return(1);
}

tcpclose(q)
register struct queue *q;
{
	if(tcpqueue == q)
		tcpqueue = 0;
}

tcpisrv(q)
register struct queue *q;
{
	register struct block *bp, *head, *tail;

	/* there is now a whole packet waiting
	 * on this queue; strip it off and pass to tcp_input().
	 * things other than data or delims are forwarded directly
	 * by tcpiput().
	 */
	head = tail = (struct block *) 0;
	while(bp = getq(q)){
		if(bp->type == M_DELIM){
			freeb(bp);
			if(head){
				tcp_busy++;
				MCHECK(head);
				tcp_input(head);
				--tcp_busy;
			} else {
				printf("tcpisrv: no data\n");
			}
			head = tail = (struct block *) 0;
		} else if(bp->type == M_DATA){
			bp->next = (struct block *) 0;
			if(head == (struct block *) 0){
				head = bp;
			} else {
				tail->next = bp;
			}
			tail = bp;
		} else {
			printf("tcpisrv: weird type %d\n", bp->type);
			(*q->next->qinfo->putp)(q->next, bp);
		}	
	}
	if (head)
		bp_putback(q, head);
}


tcpiput(q, bp)
register struct queue *q;
register struct block *bp;
{
	switch(bp->type){
	case M_DATA:
		putq(q, bp);	/* putq does compression into blocks */
		break;
	case M_DELIM:
		putq(q, bp);
		qenable(q);
		break;
	default:
		(*q->next->qinfo->putp)(q->next, bp);
		break;
	}
		
}

tcposrv(q)
register struct queue *q;
{
	register union stmsg *sp;
	register struct block *bp;

	register int *intp;

	while(bp = getq(q)){
		if(bp->type == M_IOCTL){
			sp = (union stmsg *)bp->rptr;
			switch(sp->ioc0.com){
			case TCPIOMAXSEG:
				intp = (int *)(sp->iocx.xxx);
				tcp_maxseg = *intp;
				bp->type = M_IOCACK;
				qreply(q, bp);
				break;
			default:
				(*q->next->qinfo->putp)(q->next, bp);
				break;
			}
		} else {
			(*q->next->qinfo->putp)(q->next, bp);
		}
	}
}

tcp_ldout(bp)
register struct block *bp;
{
	register struct block *bp1;
	register struct queue *q;

	if(tcpqueue == 0){
		bp_free(bp);
		return(1);
	}
	q = WR(tcpqueue);
	if(q->next->flag&QFULL){
		printf("tcp_ldout: QFULL\n");
		bp_free(bp);
		return(1);
	}
	MCHECK(bp);
	while(bp){
		bp1 = bp->next;
		(*q->next->qinfo->putp)(q->next, bp);
		bp = bp1;
	}
	bp1 = allocb(0);
	if(bp1){
		bp1->type = M_DELIM;
		(*q->next->qinfo->putp)(q->next, bp1);
	} else {
		printf("tcp_ldout: no allocb for delim\n");
		return(1);
	}
	return(0);
}
