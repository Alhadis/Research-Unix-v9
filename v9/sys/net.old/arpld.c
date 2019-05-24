/*
 *             A R P  L D
 *
 * Generic (ethernet) address-resolution line discipline.
 * Implements an LRU cache of protocol address to ethernet address mappings.
 *
 *
 * Written by Kurt Gollhardt  (Nirvonics, Inc.)
 * Last update Wed Feb 12 20:43:28 1986
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/stream.h"
#include "../h/conf.h"
#include "../h/ioctl.h"
#include "../h/ttyld.h"
#include "../h/ethernet.h"
#include "../h/order.h"
#include "../net/arpld.h"

#include "arp.h"
#if NARP > 0

#define NARPROTO  (NARP * NPROTO)

struct arp_dev Arp_dev[NARP];
struct proto   Arp_proto[NARPROTO];

static int     hd_size = MAX_HARDSIZE,
               pr_size, pr_hfirst;

static u_char  hd_sender[MAX_HARDSIZE],
               hd_target[MAX_HARDSIZE],
	       pr_sender[MAX_PRSIZE],
	       pr_target[MAX_PRSIZE];

static unsigned	    arp_clock;

static int install(), request();
static int pack(), unpack();
static int bcmp();

struct block *packet_pullup();

#define DEBUG

#ifdef DEBUG
     int  Arpdebug = 0;
#    define debug(x)     if (Arpdebug) x
#else
#    define debug(x)
#endif


#define order(x,size,hfirst) ((hfirst) ? order_hfirst(x,size) \
                                       : order_lfirst(x,size))


arp_enable(dev, type, psize, phfirst, paddr)
     dev_t     dev;
     u_char    *paddr;
{
     register struct proto    *pr, *pr2;
     register struct arp_dev  *arpd;
     register struct arp      *arp;
     register int        i;
     int       ps = spl6();

     dev = physical(dev);

          /* Find the per-device structure for the desired device */
     for (arpd = Arp_dev, i = 0; i < NARP; ++i, ++arpd) {
          if (arpd->pdev == dev)
	       break;
     }
     if (i == NARP) {
          printf("ARP: no arp for dev (%d,%d)\n", major(dev), minor(dev));
	  goto bad;
     }
     if (bcmp(arpd->hdaddr, hzero, MAX_HARDSIZE)) {
	  printf("ARP: hardware address not set for dev (%d,%d)\n",
			 major(dev), minor(dev));
	  goto bad;
     }

          /* Find a free protocol structure for this device */
     pr = 0;
     for (pr2 = &Arp_proto[NPROTO * i], i = 0; i < NPROTO; ++i, ++pr2) {
          if (pr2->psize == 0)
	       pr = pr2;
          else if (pr2->type == type) {
	       printf("ARP: duplicate enable - dev (%d,%d) type 0x%x\n",
	                 major(dev), minor(dev), type);
               goto bad;
          }
     }
     if (pr == 0) {
          printf("ARP: too many protocols on dev (%d,%d)\n",
	            major(dev), minor(dev));
          goto bad;
     }

     if (psize < 1 || psize > MAX_PRSIZE) {
          printf("ARP: protocol address size (%d) out of range\n", psize);
	  goto bad;
     }
     pr->ptr = arpd;
     pr->type = type;
     pr->psize = psize;
     pr->phfirst = phfirst;

          /* Clear out the arp pairs */
     for (arp = pr->pair; arp < &pr->pair[NPAIR]; ++arp)
          bcopy(pzero, arp->paddr, psize);

          /* Add our address as arp pair 0 */
     bcopy(paddr, pr->pair[0].paddr, pr->psize);
     bcopy(arpd->hdaddr, pr->pair[0].hdaddr, hd_size);

     splx(ps);

     debug(printf("ARP: enabled protocol 0x%x on dev (%d,%d), channel %d\n",
               type, major(dev), minor(dev), pr - Arp_proto));
     debug(printf("     my addr is"));
     debug(print_addr(paddr, pr->psize));
     debug(printf(" at ethernet"));
     debug(print_addr(arpd->hdaddr, hd_size));
     debug(printf("  order is %s-byte first\n", phfirst? "high" : "low"));

     return pr - Arp_proto;

bad:
     splx(ps);
     return -1;
}

arp_getaddr(i, paddr, hdaddr)
     u_short   i;
     u_char    *paddr, *hdaddr;
{
     register struct proto    *pr = &Arp_proto[i];
     register struct arp      *arp;
     int       ps;

     if (i >= NARPROTO || pr->psize == 0) {
          printf("ARP: getaddr on un-enabled channel %d\n", i);
	  return -1;
     }

     debug(printf("ARP: getaddr for"));
     debug(print_addr(paddr, pr->psize));
     debug(printf(" on channel %d\n", i));

     ps = spl6();

     for (arp = pr->pair; arp < &pr->pair[NPAIR]; ++arp) {
          if (bcmp(arp->paddr, paddr, pr->psize)) {
	       bcopy(arp->hdaddr, hdaddr, hd_size);
	       arp->time = ++arp_clock;
	       splx(ps);
	       return 0;
          }
     }

     request(pr, paddr);
     splx(ps);
     return -1;
}

struct block *
arp_resolve(i, bp, paddr)
     struct block   *bp;
     u_char    *paddr;
{
     register struct block    *bp1;
     register struct etherpup	   *ep;

     if ((bp1 = allocb(sizeof(struct etherpup))) == 0) {
	  printf("ARP: can't alloc block for resolve\n");
bad:
	  while (bp != 0) {
	       bp1 = bp->next;
	       freeb(bp);
	       bp = bp1;
	  }
	  return 0;
     }

     ep = (struct etherpup *)bp1->wptr;
     bp1->wptr += sizeof(struct etherpup);
     if (arp_getaddr(i, paddr, ep->dhost) < 0) {
	  freeb(bp1);
	  goto bad;
     }

     ep->type = hfirst_short(Arp_proto[i].type);
     bp1->next = bp;
     return bp1;
}

arp_disable(i)
     u_short   i;
{
     register struct proto    *pr = &Arp_proto[i];
     register struct arp      *arp;

     if (i >= NARPROTO || pr->psize == 0) {
          printf("ARP: disable on un-enabled channel %d\n", i);
	  return;
     }

     debug(printf("ARP: disabled channel %d\n", i));

     pr->psize = 0;
}

arp_broadcast(hdaddr)
     u_char    *hdaddr;
{
     bcopy(broadaddr, hdaddr, hd_size);
}


static install(pr, paddr, hdaddr)
     register struct proto    *pr;
     u_char    *paddr, *hdaddr;
{
     register struct arp      *arp, *empty;

     debug(printf("ARP: install()\n"));

          /* Look for an arp pair for this address, or a free one, or LRU */
     for (empty = arp = pr->pair; arp < &pr->pair[NPAIR]; ++arp) {
          if (bcmp(arp->paddr, paddr, pr->psize))
	       break;
          if (bcmp(arp->paddr, pzero, pr->psize))
	       (empty = arp)->time = arp_clock + 1;
	  else if (arp_clock - arp->time > arp_clock - empty->time)
	       empty = arp;
     }

     if (arp == &pr->pair[NPAIR])
	  (arp = empty)->time = ++arp_clock;

     bcopy(paddr, arp->paddr, pr->psize);
     bcopy(hdaddr, arp->hdaddr, hd_size);

     debug(printf("ARP: installed paddr"));
     debug(print_addr(paddr, pr->psize));
     debug(printf(" at ethernet"));
     debug(print_addr(hdaddr, hd_size));
     debug(printf(" on channel %d\n", pr - Arp_proto));

     return 0;
}

#define OUT_SIZE    (sizeof(struct etherpup) + sizeof(struct ether_arp) \
                     - 2*(MAX_HARDSIZE + MAX_PRSIZE) + 2*(hd_size + pr_size))

static request(pr, paddr)
     register struct proto    *pr;
     u_char    *paddr;
{
     register struct queue    *q;
     register struct block    *bp;
     struct ether_arp         *arpkt;
     struct etherpup         *ep;
     long      my_paddr, his_paddr;

     debug(printf("ARP: request()\n"));

     pr_size = pr->psize;
     pr_hfirst = pr->phfirst;

     q = WR(pr->ptr->rdq);
     if ((bp = allocb(OUT_SIZE)) == 0) {
          printf("ARP: can't alloc block for request\n");
          return;
     }
     if (bp->lim - bp->wptr < OUT_SIZE) {
          freeb(bp);
	  printf("ARP: block too small for request\n");
	  return;
     }

     ep = (struct etherpup *)bp->wptr;
     arpkt = (struct ether_arp *)(bp->wptr + sizeof(struct etherpup));
     bp->wptr += OUT_SIZE;

     bcopy(broadaddr, ep->dhost, hd_size);
     ep->type = hfirst_short(ETHERPUP_ARPTYPE);

     arpkt->arp_hrd = hfirst_short(ARPHRD_ETHER);
     arpkt->arp_pro = hfirst_short(pr->type);
     arpkt->arp_hln = hd_size;
     arpkt->arp_pln = pr_size;
     arpkt->arp_op = hfirst_short(ARPOP_REQUEST);

     bcopy(pr->pair[0].hdaddr, hd_sender, hd_size);
     bcopy(pr->pair[0].paddr, pr_sender, pr_size);
     bcopy(paddr, pr_target, pr_size);
     bcopy(hzero, hd_target, hd_size);
     pack(arpkt);

     debug(printf("ARP: sending a request for address"));
     debug(print_addr(paddr, pr->psize));
     debug(printf(" on channel %d\n", pr - Arp_proto));

     if (q->next->flag & QFULL) {
          freeb(bp);
          debug(printf("ARP: QFULL in request()\n"));
     } else {
          (*q->next->qinfo->putp)(q->next, bp);
	  putctl(q->next, M_DELIM);
     }
}


int arp_open(), arp_close(), arp_iput(), arp_srv(), arp_bypass(), arp_rcvpkt();
static struct qinit arp_rinit = {
          arp_iput, arp_srv, arp_open, arp_close, 750, 250
};
static struct qinit arp_winit = {
          arp_bypass, NULL, arp_open, arp_close, 0, 0
};
struct streamtab arpinfo = { &arp_rinit, &arp_winit };


arp_open(q, dev)
     register struct queue    *q;
     dev_t     dev;
{
     register int   i;
     register struct arp_dev  *arpd, *narp;

     if (q->ptr)    /* If this stream is already open, don't do anything */
          return 1;

     dev = physical(dev);

     /* Look for a free per-device structure */
     narp = 0;
     for (arpd = Arp_dev; arpd < &Arp_dev[NARP]; ++arpd) {
          if (arpd->pdev == dev) {
	       printf("ARP: multiple arps on device (%d,%d)\n",
	                 major(dev), minor(dev));
               return 0;
          }
	  if (arpd->pdev == 0)
	       narp = arpd;
     }
     if (narp == 0)
          return 0;   /* Open fails: no more line disciplines */

     narp->pdev = dev;
     narp->delim_count = 0;
     bcopy(hzero, narp->hdaddr, MAX_HARDSIZE);
     narp->rdq = q;

     q->flag |= QDELIM|QNOENB;
     q->ptr = (caddr_t)narp;
     return 1;
}

arp_close(q)
     register struct queue    *q;
{
     register struct arp_dev  *arpd;
     int       n, i;

     arpd = (struct arp_dev *)q->ptr;
     arpd->pdev = 0;
     q->ptr = 0;
     n = (arpd - Arp_dev) * NPROTO;
     for (i = n; i < n + NPROTO; ++i)
          Arp_proto[i].psize = 0;
}

arp_bypass(q, bp)
     register struct queue    *q;
     register struct block    *bp;
{
     (*q->next->qinfo->putp)(q->next, bp);
}

arp_misc(q, bp)
     struct queue    *q;
     struct block    *bp;
{
     register union stmsg *sp;

     if (bp->type == M_IOCACK) {
	  sp = (union stmsg *)bp->rptr;
	  if (sp->iocx.com == ENIOADDR)
	       bcopy(sp->iocx.xxx, ((struct arp_dev *)q->ptr)->hdaddr,
			 MAX_HARDSIZE);
     }
     /* Anything else, just pass on to the next guy */
     (*q->next->qinfo->putp)(q->next, bp);
}

arp_iput(q, bp)
     struct queue    *q;
     struct block    *bp;
{
     packet_putp(q, bp, &((struct arp_dev *)q->ptr)->delim_count, 0, arp_misc);
}

arp_false(q)
     struct queue   *q;
{
     return 0;
}

arp_srv(q)
     struct queue   *q;
{
     packet_srvp(q, &((struct arp_dev *)q->ptr)->delim_count,
		    arp_false, arp_false, arp_rcvpkt, 0);
}

arp_rcvpkt(q, bp, partial)
     register struct queue    *q;
     register struct block    *bp;
{
     register struct ether_arp *arpkt;
     register struct proto    *pr;
     register struct etherpup *ep;
     register int        i, type;
     short     op;
     int       ps;

     if (partial || bp == NULL) {
	  free_blocks(bp);
	  return;
     }
     i = sizeof(struct etherpup) + sizeof(struct ether_arp);
     if ((bp = packet_pullup(bp, i)) == 0) {
	  printf("ARP: bad packet\n");
	  return;
     }
     if (bp->next)
	  free_blocks(bp);

     pr = &Arp_proto[((struct arp_dev *)q->ptr - Arp_dev) * NPROTO];
     arpkt = (struct ether_arp *)(bp->rptr + sizeof(struct etherpup));
     type = hfirst_short(arpkt->arp_pro);
     op = hfirst_short(arpkt->arp_op);

     debug(printf("ARP: rcvd arp %s for protocol 0x%x\n",
                    (op == ARPOP_REQUEST ? "request" :
	                 (op == ARPOP_REPLY ? "reply" : "(BAD)")), type));

     ps = spl6();

     for (i = 0; i < NPROTO; ++i, ++pr) {
          if (pr->psize != 0 && pr->type == type)
	       break;
     }
     if (i == NPROTO)
          goto out; /* Not one of the types we recognize */

     pr_size = pr->psize;
     pr_hfirst = pr->phfirst;
     unpack(arpkt);

     if (!bcmp(pr_target, pr->pair[0].paddr, pr_size)) {
out:
          splx(ps);
          freeb(bp);
	  return;
     }
     if (bcmp(pr_sender, pr->pair[0].paddr, pr_size)) {
          printf("ARP: someone is pretending to be me!!!\n");
          goto out;
     }
     if (op == ARPOP_REPLY && !bcmp(hd_target, pr->pair[0].hdaddr, hd_size))
          goto out;

     install(pr, pr_sender, hd_sender);
     if (op != ARPOP_REQUEST)
          goto out;

     arpkt->arp_hrd = hfirst_short(ARPHRD_ETHER);
     arpkt->arp_op = hfirst_short(ARPOP_REPLY);

     debug(printf("ARP: sending reply to"));
     debug(print_addr(pr_sender, pr_size));
     debug(printf(" at ethernet"));
     debug(print_addr(hd_sender, hd_size));
     debug(printf(" on channel %d\n", pr - Arp_proto));

     bcopy(pr_sender, pr_target, pr_size);
     bcopy(hd_sender, hd_target, hd_size);
     bcopy(pr->pair[0].paddr, pr_sender, pr_size);
     bcopy(pr->pair[0].hdaddr, hd_sender, hd_size);
     pack(arpkt);

     ep = (struct etherpup *)(bp->rptr += sizeof(struct etherpup)
                                             - sizeof(struct etherpup));
     bcopy(hd_target, ep->dhost, hd_size);
     ep->type = hfirst_short(ETHERPUP_ARPTYPE);

     splx(ps);

     q = WR(q);
     if (q->next->flag & QFULL) {
          freeb(bp);
          debug(printf("ARP: QFULL on reply\n"));
     } else {
          (*q->next->qinfo->putp) (q->next, bp);
          putctl(q->next, M_DELIM);
     }
}


static pack(arpkt)
     struct ether_arp    *arpkt;
{
     register u_char     *p = arpkt->arp_addr;
     long      paddr;

     debug(printf("ARP: packing arp from"));
     debug(print_addr(pr_sender, pr_size));
     debug(printf(" at"));
     debug(print_addr(hd_sender, hd_size));
     debug(printf(", target is"));
     debug(print_addr(pr_target, pr_size));
     debug(printf(" (at"));
     debug(print_addr(hd_target, hd_size));
     debug(printf(")\n"));

     bcopy(hd_sender, p, hd_size);
     paddr = order(*(long *)pr_sender, pr_size, pr_hfirst);
     bcopy(&paddr, p += hd_size, pr_size);
     bcopy(hd_target, p += pr_size, hd_size);
     paddr = order(*(long *)pr_target, pr_size, pr_hfirst);
     bcopy(&paddr, p += hd_size, pr_size);
}

static unpack(arpkt)
     struct ether_arp    *arpkt;
{
     register u_char     *p = arpkt->arp_addr;
     long      paddr;

     bcopy(p, hd_sender, hd_size);
     paddr = order(*(long *)(p += hd_size), pr_size, pr_hfirst);
     bcopy(&paddr, pr_sender, pr_size);
     bcopy(p += pr_size, hd_target, hd_size);
     paddr = order(*(long *)(p += hd_size), pr_size, pr_hfirst);
     bcopy(&paddr, pr_target, pr_size);

     debug(printf("ARP: unpacked arp from"));
     debug(print_addr(pr_sender, pr_size));
     debug(printf(" at"));
     debug(print_addr(hd_sender, hd_size));
     debug(printf(", target is"));
     debug(print_addr(pr_target, pr_size));
     debug(printf(" (at"));
     debug(print_addr(hd_target, hd_size));
     debug(printf(")\n"));
}


static bcmp(a, b, n)
     register u_char     *a, *b;
     register int        n;
{
     while (n-- > 0)
          if(*a++ != *b++)
               return 0;
     return 1;
}


#ifdef DEBUG

print_addr(addr, size)
     register u_char     *addr;
     register u_short    size;
{
     while (size-- > 0)
          printf(" %x", *addr++);
}

#endif DEBUG

#endif NARP
