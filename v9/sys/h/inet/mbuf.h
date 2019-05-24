/*
 * mbuf emulation.
 */

#define mbuf		block
#define m_next		next
#define mtod(m, type)	((type) (m->rptr))
#define m_get(x1, x2)	bp_get()
#define m_free(m)	freeb(m)
#define m_freem(m)	bp_free(m)
#define m_pullup	bp_pullup
#define m_adj		bp_adj
#define dtom(x)		bp_dtom((struct block *) x)
#define m_copy		bp_copy

#define BLEN(bp) ((bp)->wptr - (bp)->rptr)
#define BSZ(bp) ((bp)->lim - (bp)->base)
#define MAXBLEN	64

#define MGET(m, x, y)	((m) = m_get(x, y))

#ifdef KERNEL
extern struct block *bp_dtom(), *bp_copy();
extern struct block *bp_get();
#endif

#include "sparam.h"
#define NBLOCK (NBLKBIG+NBLK64+NBLK16+NBLK4)
extern struct block cblock[];

#ifdef IN_PARANOID
#define BLOCKCHK(bp)\
	if(bp < cblock || cblock >= &cblock[NBLOCK])\
		panic("bp_check bad bp");\
	if(bp->base == 0 || bp->lim == 0)\
		panic("bp_check 0");\
	if(bp->rptr >= bp->lim || bp->rptr < bp->base)\
		panic("bp_check rptr");\
	if(bp->wptr > bp->lim || bp->wptr < bp->base)\
		panic("bp_check wptr");\
	if(bp->rptr > bp->wptr)\
		panic("bp_check rptr > wptr")
#define MCHECK(bp) bp_check(bp)
#else
#define BLOCKCHK(bp)
#define MCHECK(bp)
#endif IN_PARANOID
