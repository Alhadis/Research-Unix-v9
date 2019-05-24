#include "inet.h"
#if NINET > 0

#include "../h/param.h"
#include "../h/stream.h"
#include "../h/conf.h"
#include "../h/inet/in.h"
#include "../h/inet/ip.h"
#include "../h/inet/ip_var.h"
#include "../h/inet/mbuf.h"
#include "sparam.h"

extern long time;

struct block *
bp_get()
{
	register struct block *bp;

	bp = allocb(64);
	if(bp)
		bp->next = 0;
	return(bp);
}

bp_check(bp)
register struct block *bp;
{
	while(bp){
		BLOCKCHK(bp);
		bp = bp->next;
	}
}

/* given a char *, come up with the block that holds it. yuk. */
int dtom_hits, dtom_misses;
static u_char *xfirst16, *xfirst64, *xfirst1024, *xp;
struct block *xbp;

struct block *
bp_dtom(p)
u_char *p;
{
	extern struct block cblock[];
	extern u_char blkdata[];
	register u_char *first16, *first64, *first1024;
	register struct block *bp;

	/* guess. the order of things in blkdata is NBLK4, NBLK16, ... */
#ifdef CHKBLK
	first16 = &blkdata[5 * NBLK4];
	first64 = &first16[17 * NBLK16];
	first1024 = &first64[65 * NBLK64];
	if(p < first16){
		bp = &cblock[(p - blkdata) / 5];
	} else if(p < first64){
		bp = &cblock[NBLK4 + (p - first16) / 17];
	} else if(p < first1024){
		bp = &cblock[NBLK4 + NBLK16 + (p - first64) / 65];
	} else {
		bp = &cblock[NBLK4 + NBLK16 + NBLK64 + (p - first1024) / 1025];
	}
#else
	first16 = &blkdata[4 * NBLK4];
	first64 = &first16[16 * NBLK16];
	first1024 = &first64[64 * NBLK64];
	if(p < first16){
		bp = &cblock[(p - blkdata) / 4];
	} else if(p < first64){
		bp = &cblock[NBLK4 + (p - first16) / 16];
	} else if(p < first1024){
		bp = &cblock[NBLK4 + NBLK16 + (p - first64) / 64];
	} else {
		bp = &cblock[NBLK4 + NBLK16 + NBLK64 + (p - first1024) / 1024];
	}
#endif CHKBLK
	if(bp->base && bp->lim && bp->rptr && bp->wptr
	   && (p >= bp->base) && (p < bp->lim)){
		BLOCKCHK(bp);
		dtom_hits++;
		return(bp);
	}
	xfirst16 = first16;
	xfirst64 = first64;
	xfirst1024 = first1024;
	xp = p;
	xbp = bp;
	dtom_misses++;
	for(bp = &cblock[NBLOCK-1]; bp >= &cblock[0]; --bp){
		if(bp->base == 0 || bp->lim == 0 || bp->rptr == 0 || bp->wptr == 0)
			continue;
		if((p >= bp->base) && (p < bp->lim)){
			return(bp);
		}
	}
	panic("bp_dtom");
	/* NOTREACHED */
}

/* bp_pullup: make the first block have at least len bytes */
struct block *
bp_pullup(bp, len)
register struct block *bp;
{
	register struct block *m, *n, *nn;
	int count;

	n = bp;
	if(len > MAXBLEN)
		goto bad;
	m = allocb(MAXBLEN);
	if(m == 0)
		goto bad;
	do{
		count = len;
		if (m->lim - m->wptr < count)
			count = m->lim - m->wptr;
		if (BLEN(n) < count)
			count = BLEN(n);
		bcopy(n->rptr, m->wptr, (unsigned)count);
		len -= count;
		m->wptr += count;
		n->rptr += count;
		if(BLEN(n))
			break;
		nn = n->next;
		freeb(n);
		n = nn;
	} while(n);
	if(len){
		freeb(m);
		goto bad;
	}
	m->next = n;
	MCHECK(m);
	return(m);
bad:
	printf("m_pullup bad\n");
	bp_free(n);
	return(0);
}

bp_free(bp)
register struct block *bp;
{
	register struct block *p;

	while(bp){
		p = bp->next;
		BLOCKCHK(bp);
		freeb(bp);
		bp = p;
	}
}

struct block *
bp_copy(m, off, len)
register struct block *m;
int off;
register int len;
{
	register struct block *n, **np;
	struct block *top;
	register int clen;

	MCHECK(m);
	if(len == 0)
		return(0);
	if(off < 0 || len < 0)
		panic("m_copy");
	while(off > 0){
		if(m == 0)
			panic("m_copy 1");
		if(off < BLEN(m))
			break;
		off -= BLEN(m);
		m = m->next;
	}
	np = &top;
	top = 0;
	while(len > 0){
		if(m == 0)
			panic("m_copy 2");
		n = allocb(len);
		*np = n;
		if(n == 0)
			goto nospace;
		n->next = 0;
		np = &n->next;
		do {
			clen = len;
			if (BLEN(m) - off < clen)
				clen = BLEN(m) - off;
			if (n->lim - n->wptr < clen)
				clen = n->lim - n->wptr;
			bcopy((caddr_t)m->rptr+off, (caddr_t)n->wptr, clen);
			n->wptr += clen;
			len -= clen;
			if (len <= 0) {
				MCHECK(top);
				return (top);
			}
			if (m->rptr + off + clen < m->wptr)
				off += clen;
			else {
				off = 0;
				m = m->next;
			}
		} while (n->wptr < n->lim);
	}
	MCHECK(top);
	return(top);
nospace:
	bp_free(top);
	return(0);
}

bp_adj(m, len)
register struct block *m;
register int len;
{

	if (m == NULL)
		return;
	MCHECK(m);
	if (len > 0) {
		while(m && len > 0){
			if(BLEN(m) <= len){
				len -= BLEN(m);
				m->wptr = m->rptr;
				m = m->next;
			} else {
				m->rptr += len;
				break;
			}
		}
	}
	else if (len < 0) {
		len = bp_len(m) + len;
		if (len <= 0) {
			if (m->next)
				bp_free(m->next);
			m->next = 0;
			m->wptr = m->rptr;
			return;
		}
		while ((len -= BLEN(m)) > 0) {
			if ((m = m->next) == NULL)
				return;
		}
		m->wptr += len;		/* len is <= 0 */
		bp_free(m->next);
		m->next = 0;
	}
	MCHECK(m);
}

m_cat(m, n)
register struct block *m, *n;
{
	struct mbuf *xn;

	MCHECK(m); MCHECK(n);
	while(m->next)
		m = m->next;
	while(n){
		if((m->wptr + BLEN(n)) >= m->lim){
			/* just join the two chains */
			m->next = n;
			break;
		}
		/* splat the data from one into the other */
		bcopy(n->rptr, m->wptr, BLEN(n));
		m->wptr += BLEN(n);
		xn = n->next;
		freeb(n);
		n = xn;
	}
	MCHECK(m);
}

/* C version of 4.2bsd's Internet checksum routine */
/* This version assumes that no message exceeds 2^16 words */
in_cksum(m, len)
	register struct mbuf *m;
	register int len;
{
	register u_short *w;
	register u_long sum = 0;
	register int mlen = 0;
	register int odd = 0;	/* set if last block ended on an odd
				 * numbered byte */

	MCHECK(m);
	for (; len!=0; m=m->m_next) {
		if (m == NULL) {
			printf("cksum: out of data\n");
			break;
		}
		w = mtod(m, u_short *);
		mlen = BLEN(m);
		if (len < mlen)
			mlen = len;
		len -= mlen;
		/* vecadd returns a 16-bit checksum of the block + sum */
		sum = vecadd(w, mlen, sum, odd);
		if((int)mlen & 1)
			odd = !odd;
	}
	/* return complement of sum */
	return sum^0xffff;
}

in_addr
in_netof(x)
in_addr x;
{
	in_addr netmask;
	struct ipif *ifp;

	if(IN_CLASSC(x))
		netmask = IN_CLASSC_NET;
	else if(IN_CLASSB(x))
		netmask = IN_CLASSB_NET;
	else
		netmask = IN_CLASSA_NET;

	/* look for an interface for this network and use its subnet mask */
	for(ifp = &ipif[0]; ifp < &ipif[NINET]; ifp++){
		if(ifp->flags & IFF_UP){
			if(netmask&x == netmask&ifp->that)
				if (ifp->mask==0)
					return(x&netmask);
				else
					return(x&ifp->mask);
		}
	}

	/* no interface for this network, assume no subnetting */
	return x&netmask;
}

/*
 * Hash table for route entries.  Collision resolution is linear search until
 * encountering a hole.  Replacement is LRU.
 */
#define NROUTES 128
struct ip_route ip_routes[NROUTES];
int Nip_route = NROUTES;		/* let netstat know number of routes */
#define HASH(x) (((x)+((x)>>8))%NROUTES)
struct ip_route ip_default_route;

ip_doroute(dst, gate)
	in_addr dst, gate;
{
	register struct ip_route *rp, *sp, *op;
	register struct ipif *ifp;

	/* default is a special case */
	if (dst == 0){
		ip_default_route.gate = gate;
		return(0);
	}

	/* look for what should be a noop */
	if(gate){
		/* don't accept an indirect route, if we have a direct one */
		for(ifp = ipif; ifp < &ipif[NINET]; ifp++){
			if((ifp->flags&IFF_UP) && ifp->that==dst)
				return(0);
		}
	} else
		gate = dst;

	/* look through existing routes */
	op = sp = rp = &ip_routes[HASH(dst)];
	do {
		if (rp->dst==0 || rp->dst==dst) {
			op = rp;
			break;
		}
		if (rp->time<op->time)
			op = rp;
		if (++rp==&ip_routes[NROUTES])
			rp = ip_routes;
	} while (rp != sp);

	/* add a new route */
	op->dst = dst;
	op->gate = gate;
	op->time = time;
	return(0);
}

/* Look for a route on the circular list.  If the route is found, move
 * it to the beginning of the list.
 */
struct ip_route_info
ip_route(dst)
	in_addr dst;
{
	register struct ip_route *rp, *sp;
	extern unsigned long in_netof();
	unsigned long netof_dst;
	struct ip_route_info info;

	/* try a network to which we are directly connected */
	info.addr = dst;
	info.ifp = ip_ifonnetof(dst);
	if (info.ifp)
		return info;

	/* look for host routes */
	sp = rp = &ip_routes[HASH(dst)];
	do {
		if (dst==rp->dst && rp->dst!=rp->gate) {
			info.addr = rp->gate;
			info.ifp = ip_ifonnetof(info.addr);
			rp->time = time;
			return(info);
		}
		if (rp->dst==0)
			break;
		if (++rp==&ip_routes[NROUTES])
			rp = ip_routes;
	} while (rp != sp);

	/* now try nets */
	netof_dst = in_netof(dst);
	sp = rp = &ip_routes[HASH(netof_dst)];
	do {
		if (netof_dst==rp->dst && rp->dst!=rp->gate) {
			info.addr = rp->gate;
			info.ifp = ip_ifonnetof(info.addr);
			rp->time = time;
			return(info);
		}
		if (rp->dst==0)
			break;
		if (++rp==&ip_routes[NROUTES])
			rp = ip_routes;
	} while (rp != sp);

	/* if all else fails, use default route */
	/* N.B.  If the gate is a network, don't change the destination
	 *	address.  This allows multiple networks on one wire by
	 *	making that wire the default.
	 */
	if (ip_default_route.gate!=in_netof(ip_default_route.gate))
		info.addr = ip_default_route.gate;
	else
		info.addr = dst;
	info.ifp = ip_ifonnetof(ip_default_route.gate);
	return(info);
}

bp_len(bp)
register struct block *bp;
{
	register int n;

	n = 0;
	while(bp){
		n += BLEN(bp);
		bp = bp->next;
	}
	return(n);
}

bp_putback(q, list)
struct queue *q;
struct block *list;
{
	register struct block *bp;
	register struct block *prev, *next;

	/*
	 * reverse the list, to keep data in order
	 */
	prev = next = NULL;
	for (bp = list; bp; bp = next) {
		next = bp->next;
		bp->next = prev;
		prev = bp;
	}
	for (bp = prev; bp; bp = next) {
		next = bp->next;
		putbq(q, bp);
	}
}

in_addr
ip_hoston(dst)
in_addr dst;
{
	struct ip_route_info info;

	info = ip_route(dst);
	if(info.ifp == 0)
		return(0);
	return(info.ifp->thishost);
}

in_lnaof(i)
register u_long i;
{

	if(IN_CLASSA(i))
		return((i)&IN_CLASSA_HOST);
	else if(IN_CLASSB(i))
		return((i)&IN_CLASSB_HOST);
	else
		return((i)&IN_CLASSC_HOST);
}
#endif NINET
