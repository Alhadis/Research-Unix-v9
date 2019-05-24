/*
 * Address resolution
 */

#ifndef _ARP_
#define _ARP_

#define MAX_HARDSIZE     6
#define MAX_PRSIZE       4

struct ether_arp {
	u_short	arp_hrd;
#define ARPHRD_ETHER	1
	u_short	arp_pro;
	u_char	arp_hln;
	u_char	arp_pln;
	u_short	arp_op;
#define ARPOP_REQUEST	1
#define ARPOP_REPLY	2
	u_char	arp_addr[2 * (MAX_HARDSIZE + MAX_PRSIZE)];
};

static u_char broadaddr[MAX_HARDSIZE] = {
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

struct arp {
     u_char    paddr[MAX_PRSIZE];
     u_char    hdaddr[MAX_HARDSIZE];
     unsigned  time;
};

static u_char  pzero[MAX_PRSIZE];
static u_char  hzero[MAX_HARDSIZE];

#define NPROTO 2
#define NPAIR  30

struct arp_dev {
     dev_t          pdev;
     struct queue   *rdq;
     u_char	    hdaddr[MAX_HARDSIZE];
     int	    delim_count;
};

struct proto {
     struct arp_dev *ptr;
     u_short        type;
     u_char         psize;
     u_char         phfirst;
     struct arp     pair[NPAIR];
};

#ifndef CHANS_PER_UNIT
#define CHANS_PER_UNIT   8
#endif

#define physical(dev)    ((dev) & ~(CHANS_PER_UNIT - 1))

#endif
