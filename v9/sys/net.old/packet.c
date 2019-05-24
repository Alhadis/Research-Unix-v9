/*
 *	       P A C K E T
 *
 * Utilities for working with "packets".  These are groups of (stream) data
 * blocks terminated by a delimiter.  When these blocks are removed from a
 * queue, they are kept as a chain of blocks, without the final M_DELIM, and
 * are referred to as a dequeued packet.
 *
 *
 * Written by Kurt Gollhardt  (Nirvonics, Inc.)
 * Last update Wed Feb 12 21:24:10 1986
 *
 */

#include "../h/param.h"
#include "../h/stream.h"

#define PKT_COMPLETE	 0
#define PKT_PARTIAL	 1


/*
 * This routine does all the work for a server on a queue using packets.
 * Arguments provide routines to do the application-specific operations:
 *   'blocked' is called with a pointer to the queue;
 *	  it should return non-zero if the location which the first
 *	  packet on the queue would be sent to is full.  If it does
 *	  return non-zero, it must ensure that this queue will be
 *	  re-enabled when the blocking condition goes away.
 *   'allow_partial' is called with a pointer to the queue;
 *	  it should return non-zero if this queue is allowed to receive
 *	  partial packets at any time.  (Any queue may be given partial
 *	  packets if not doing so would cause a deadlock.)
 *   'process' is called with a pointer to the queue, a dequeued
 *	  packet (struct block *), and a flag which is non-zero if
 *	  this is a partial packet.
 */
packet_srvp(q, delim_count, blocked, allow_partial, process, other)
     register struct queue    *q;	/* queue being serviced */
     int	    *delim_count;	/* ptr to count of delims on queue */
     int	    (*blocked)();	/* routine to see if a queue blocks */
     int	    (*allow_partial)();	/* routine to say if partial pkts ok */
     int	    (*process)();	/* routine to process a packet */
     int	    (*other)();		/* routine to handle misc blocks */
{
     register struct block    *bp, *first_bp, *prev_bp;
     register int	 processed = 0;
     static check_other();

     check_other(q, other);
     while (*delim_count > 0) {
	  if ((*blocked) (q))
	       return;
	  if ((bp = getq(q))->type == M_DELIM)
	       first_bp = NULL;
	  else {
	       first_bp = prev_bp = bp;
	       while ((bp = getq(q))->type != M_DELIM)
		    prev_bp = bp;
	       prev_bp->next = NULL;
	  }
	  freeb(bp);
	  (*process) (q, first_bp, PKT_COMPLETE);
	  --*delim_count;
	  ++processed;
	  check_other(q, other);
     }

     if (q->first != NULL) {
	  if ((*allow_partial) (q) || /* Huge partial packet */
		    ((q->flag & QWANTW) && processed == 0)) {
	       if ((*blocked) (q))
		    return;
	       first_bp = q->first;
	       while (getq(q))
		    ;
	       (*process) (q, first_bp, PKT_PARTIAL);
	  } else
	       q->flag |= QWANTR;
     }
}

static check_other(q, other)
     register struct queue    *q;
     int	    (*other)();
{
     register struct block    *bp;

     while (q->first && q->first->type != M_DATA && q->first->type != M_DELIM) {
	  bp = getq(q);
	  if (other)
	       (*other)(q, bp);
	  else
	       freeb(bp);
     }
}


/*
 * This routine does all the work for a put routine on a queue using packets.
 * Arguments provide routines to do the application-specific operations:
 *   'ioctl' (optional) is called with a pointer to the queue, and a pointer
 *	  to an M_IOCTL block.
 *   'other' (optional) is called with a pointer to the queue, and a pointer
 *	  to a block of type other than M_DATA, M_DELIM, or M_IOCTL.
 */
packet_putp(q, bp, delim_count, ioctl, other)
     register struct queue    *q;	/* queue being put onto */
     register struct block    *bp;	/* block being put on */
     int	    *delim_count;	/* ptr to count of delims on queue */
     int	    (*ioctl)();		/* routine to handle an ioctl */
     int	    (*other)();		/* routine to handle misc blocks */
{
     register int   ps, (*func)();

     switch (bp->type) {
	  case M_DATA:
	       putq(q, bp);
	       return;
	  case M_DELIM:
	       ps = spl6();
	       putq(q, bp);
	       ++*delim_count;
	       qenable(q);
	       splx(ps);
	       return;
	  case M_IOCTL:
	       func = ioctl;
	       break;
	  default:
	       func = other;
     }

     if (func)
	  (*func) (q, bp);
     else if (q->next)
	  (*q->next->qinfo->putp) (q->next, bp);
     else
	  freeb(bp);
}


#define BLEN(bp)    ((bp)->wptr - (bp)->rptr)
#define BSZ(bp)	    ((bp)->lim - (bp)->base)
#undef MIN
#define MIN(a,b)    ((a) > (b) ? (b) : (a))

/*
 * This routine takes a dequeued packet and a length.
 * It makes sure that the specified length of data appears in the first
 * block of the packet, if possible.
 */
struct block *packet_pullup(bp, len)
     register struct block    *bp;	/* the dequeued packet */
     unsigned	    len;		/* desired length */
			 /* returns the new packet (the old one is invalid) */
{
     register struct block    *nbp;
     register unsigned	      count;


     if (bp == NULL)
          goto bad;
     if (BLEN(bp) >= len)
          return bp;

     nbp = allocb(len);
     if (nbp == NULL || BSZ(nbp) < len) {
	  free_blocks(bp);
          goto cleanup;
     }
     nbp->next = bp;

     while (bp && bp->type == M_DATA) {
	  count = BSZ(nbp) - BLEN(nbp);	     /* space left in new block */
          count = MIN(count, len);	     /* clip to desired length */
	  count = MIN(count, BLEN(bp));	     /* clip to length of this block */
          bcopy(bp->rptr, nbp->wptr, count);
          len -= count;
          bp->rptr += count;
          nbp->wptr += count;
          if (BLEN(bp) > 0)
               break;
          nbp->next = bp->next;
          freeb(bp);
	  bp = nbp->next;
     }

     if (len == 0)
          return nbp;

cleanup:
     free_blocks(nbp);
bad:
     printf("packet_pullup bad\n");
     return NULL;
}


/*
 * Free space taken by a dequeued packet.
 */
free_blocks(bp)
     register struct block    *bp;	/* dequeued packet */
{
     register struct block    *nbp;

     while (bp) {
	  nbp = bp->next;
	  freeb(bp);
	  bp = nbp;
     }
}


/*
 * This routine puts blocks from a dequeued packet onto a queue's successor.
 */
packet_put(q, bp)
     register struct queue    *q;	/* Note that we put to q->next */
     register struct block    *bp;
{
     register struct block    *bnext;

     while (bp != NULL) {     
	  bnext = bp->next;
	  if (bp->rptr >= bp->wptr)
	       freeb(bp);
	  else
	       (*q->next->qinfo->putp) (q->next, bp);
	  bp = bnext;
     }
     if (q->flag & QDELIM)
	  putctl(q->next, M_DELIM);
}
