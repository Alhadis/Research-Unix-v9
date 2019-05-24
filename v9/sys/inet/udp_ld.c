/*
 * udp line discipline; only one, to be pushed on /dev/ip17.
 */

#include "udp.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/stream.h"
#include "../h/ttyio.h"
#include "../h/ttyld.h"
#include "../h/map.h"
#include "../h/buf.h"
/*#include "../h/ubavar.h"*/
#include "../h/conf.h"

#include "../h/inet/in.h"
#include "../h/inet/ip.h"
#include "../h/inet/ip_var.h"
#include "../h/inet/udp.h"
#include "../h/inet/udp_var.h"
#include "../h/inet/mbuf.h"

struct queue *udpqueue;

int	udpopen(), udpiput(), udpisrv(), udpclose();
int	udposrv(), udpoput();
static	struct qinit udprinit = { udpiput, udpisrv, udpopen, udpclose, 
				  UDP_MSG_LIMIT, 512 };
static	struct qinit udpwinit = { putq, udposrv, udpopen, udpclose, 
				  UDP_MSG_LIMIT, 64 };
struct streamtab udpinfo = { &udprinit, &udpwinit };

udpopen(q, dev)
register struct queue *q;
{
	if (q->ptr)
		return(0);
	udpqueue = q;	/* RD queue */
	q->flag |= QDELIM;
	WR(q)->flag |= QDELIM;
	q->ptr = (caddr_t)1;
	WR(q)->ptr = (caddr_t)1;
	q->flag |= QNOENB;	/* ipiput calls qenable() */
	return(1);
}

udpclose(q)
register struct queue *q;
{
	if(udpqueue == q)
		udpqueue = 0;
}

udpisrv(q)
register struct queue *q;
{
	register struct block *bp, *head, *tail;

	/* there is now a whole packet waiting
	 * on this queue; strip it off and pass to udp_input().
	 * things other than data or delims are forwarded directly
	 * by udpiput().
	 */
	head = tail = (struct block *) 0;
	while(bp = getq(q)){
		if(bp->type == M_DELIM){
			freeb(bp);
			if(head){
				MCHECK(head);
				udp_input(head);
			} else {
				printf("udpisrv: no data\n");
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
			printf("udpisrv: weird type %d\n", bp->type);
			(*q->next->qinfo->putp)(q->next, bp);
		}	
	}
	if (head)
		bp_putback(q, head);
}


udpiput(q, bp)
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

udposrv(q)
register struct queue *q;
{
	register union stmsg *sp;
	register struct block *bp;

	while(bp = getq(q)){
		if(bp->type == M_IOCTL){
			(*q->next->qinfo->putp)(q->next, bp);
		} else {
			(*q->next->qinfo->putp)(q->next, bp);
		}
	}
}

udp_ldout(bp)
register struct block *bp;
{
	register struct block *bp1;
	register struct queue *q;

	if(udpqueue == 0){
		bp_free(bp);
		return(1);
	}
	q = WR(udpqueue);
	if(q->next->flag&QFULL){
		printf("udp_ldout: QFULL\n");
		bp_free(bp);
		return(1);
	}
	MCHECK(bp);
	while(bp){
		bp1 = bp->next;
		(*q->next->qinfo->putp)(q->next, bp);
		bp = bp1;
	}
/*
 * send delim
 */

	putctl(q->next, M_DELIM);
}
