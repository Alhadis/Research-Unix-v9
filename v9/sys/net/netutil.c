/*
 *	       N E T  U T I L
 *
 * Utility routines used by various network kernel code.
 *
 *
 * Written by Kurt Gollhardt  (Nirvonics, Inc.)
 * Last update Fri Apr  5 16:36:40 1985
 *
 */

#include "../h/param.h"
#include "../h/stream.h"

#define BLEN(bp)    ((bp)->wptr - (bp)->rptr)
#define BSZ(bp)	    ((bp)->lim - (bp)->base)
#undef MIN
#define MIN(a,b)    ((a) > (b) ? (b) : (a))

extern int bsize[];


struct block *block_pullup(q, len)
     register struct queue *q;
{
     register struct block *bp, *m;
     int count, ps = spl6();

     if ((bp = q->first) == 0)
          goto bad;
     if (BLEN(bp) >= len) {
          splx(ps);
          return bp;
     }

     m = allocb(len);
     if (m == 0 || BSZ(m) < len)
          goto bad;
     m->next = bp;
     q->first = m;
     q->count += bsize[m->class];

     while (bp && bp->type == M_DATA) {
	  count = BSZ(m) - BLEN(m);
          count = MIN(count, len);
	  count = MIN(count, BLEN(bp));
          bcopy(bp->rptr, m->wptr, (unsigned)count);
          len -= count;
          m->wptr += count;
          bp->rptr += count;
          if (BLEN(bp) > 0)
               break;
	  if (q->last == bp)
	       q->last = m;
	  q->count -= bsize[bp->class];
          m->next = bp->next;
          freeb(bp);
	  bp = m->next;
     }

     if (len == 0) {
          splx(ps);
          return m;
     }
bad:
     splx(ps);
     printf("block_pullup bad\n");
     return 0;
}


     /* Removes blocks through first delim; returns true if delim found */
flush_packet(q)
     register struct queue    *q;
{
     register struct block    *bp;
     register int   isdelim;

     while (bp = getq(q)) {
          isdelim = (bp->type == M_DELIM);
	  freeb(bp);
	  if (isdelim)
	       return 1;
     }
     return 0;
}
