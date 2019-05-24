#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sgtty.h>
#include <sys/inet/in.h>
#include <sys/inet/ip_var.h>
#include <sys/inet/udp_user.h>
#include "config.h"

#define IPDEVICE "/dev/ip16"	/* device for getting ip info from */
#define RPORT 520

int udpfd, ipfd, verbose, trace, quiet;
extern errno;
extern char *in_ntoa();
extern u_short cksum();

struct udppacket {
	struct udpaddr addr;
	char buf[4096];
};

main(argc, argv)
char *argv[];
{
	int n, i;
	struct udppacket packet;

	for(i = 1; i < argc; i++){
		if(strcmp(argv[i], "-v") == 0)
			verbose++;
		else if(strcmp(argv[i], "-q") == 0)
			quiet++;
		else if(strcmp(argv[i], "-t") == 0)
			trace++;
		else
			recsafe(argv[i]);
	}

	if ((udpfd = udp_datagram(RPORT)) < 0) {
		printf("udp route daemon: no connection\n");
		exit(1);
	}
	if ((ipfd = open(IPDEVICE, 2)) < 0) {
		perror(IPDEVICE);
		exit(1);
	}

	readgateways();
	timer();

again:
	while((n = read(udpfd, &packet, sizeof(struct udppacket))) > 0) {
		if(packet.addr.port == RPORT)
			routed(&packet, n - sizeof(struct udpaddr));
		else
			fprintf(stderr, "routed: %s %s masquerading as routed\n",
				in_host(packet.addr.host), packet.addr.port);
	}
	if(n < 0 && errno == EINTR)
		goto again;
}


/*	protocol.h	4.10	83/08/11	*/
/*
 * Routing Information Protocol
 *
 * Derived from Xerox NS Routing Information Protocol
 * by changing 32-bit net numbers to sockaddr's and
 * padding stuff to 32-bit boundaries.
 */
#define	RIPVERSION	1
#define AF_INET		2

struct sockaddr{
	short sin_family;
	u_short sin_port;
	u_long sin_addr;
	char sin_zero[8];
};
struct netinfo {
	struct	sockaddr rip_dst;	/* destination net/host */
	int	rip_metric;		/* cost of route */
};

struct rip {
	u_char	rip_cmd;		/* request/response */
	u_char	rip_vers;		/* protocol version # */
	u_char	rip_res1[2];		/* pad to 32-bit boundary */
	union {
		struct	netinfo ru_nets[1];	/* variable length... */
		char	ru_tracefile[1];	/* ditto ... */
	} ripun;
#define	rip_nets	ripun.ru_nets
#define	rip_tracefile	ripun.ru_tracefile
};
 
/*
 * Packet types.
 */
#define	RIPCMD_REQUEST		1	/* want info */
#define	RIPCMD_RESPONSE		2	/* responding to request */
#define	RIPCMD_TRACEON		3	/* turn tracing on */
#define	RIPCMD_TRACEOFF		4	/* turn it off */

#define	RIPCMD_MAX		5
#ifdef RIPCMDS
char *ripcmds[RIPCMD_MAX] =
  { "#0", "REQUEST", "RESPONSE", "TRACEON", "TRACEOFF" };
#endif

#define	HOPCNT_INFINITY		16	/* per Xerox NS */
#define	MAXPACKETSIZE		512	/* max broadcast size */
#define MAXROUTESPERPACKET	(MAXPACKETSIZE-4)/sizeof(struct netinfo)

/*
 * Timer values used in managing the routing table.
 * Every update forces an entry's timer to be reset.  After
 * EXPIRE_TIME without updates, the entry is marked invalid,
 * but held onto until GARBAGE_TIME so that others may
 * see it "be deleted".
 */
#define	TIMER_RATE		30	/* alarm clocks every 30 seconds */

#define	SUPPLY_INTERVAL		30	/* time to supply tables */

#define	EXPIRE_TIME		180	/* time to mark entry invalid */
#define	GARBAGE_TIME		240	/* time to garbage collect */

#define NIFS 20
u_long ifs[2*NIFS];		/* interface network/address pairs */

routed(up, len)
	struct udppacket *up;
	u_int len;
{
	struct rip *rip;
	struct sockaddr *sin;
	struct netinfo *np;

	rip = (struct rip *)(up->buf);
	if(rip->rip_cmd != RIPCMD_RESPONSE || rip->rip_vers != RIPVERSION)
		return;
	if(myaddr(up->addr.host))
		return;
	np = rip->rip_nets;
	if(trace)
		printf("%s\n", in_ntoa(up->addr.host));
	while(np < (struct netinfo *)(&(up->buf[len]))){
		sin = &(np->rip_dst);
		np->rip_metric = ntohl(np->rip_metric);
		sin->sin_port = ntohs(sin->sin_port);
		sin->sin_addr = ntohl(sin->sin_addr);
		if(trace) {
			printf(" %s %d\n", in_ntoa(sin->sin_addr), np->rip_metric);
			fflush(stdout);
		}
		if (np->rip_metric > 0 && saferoute(sin->sin_addr) == 0)
			rtinstall(sin->sin_addr, up->addr.host, np->rip_metric, 0);
		np++;
	}
}

struct route{
	u_long dst;
	u_long gate;
	int metric;
	int age;
};
#define NROUTES 150
struct route routes[NROUTES];

rtinstall(dst, gate, metric, age)
u_long dst, gate;
{
	struct route *rp, *save = 0;

	if(metric >= HOPCNT_INFINITY)
		return;
	for(rp = routes; rp < &routes[NROUTES]; rp++){
		if(rp->dst == dst)
			break;
		if(rp->dst == 0 && save == 0)
			save = rp;
	}
	if(rp >= &routes[NROUTES] && (rp = save) == 0){
		printf("routed: out of routes?\n");
		return;
	}
	if(rp->dst == 0)
		rp->metric = HOPCNT_INFINITY + 1;
	if(gate != rp->gate && rp->age > 1){
		/* let it age some more to avoid loops */
		return;
	}
	if(metric < rp->metric
	   || (gate == rp->gate && metric != rp->metric)
	   || (gate == rp->gate && age != rp->age)){
		rp->metric = metric;
		rp->age = age;
		if(rp->dst != dst || rp->gate != gate){
			if(verbose){
				printf("%s ", in_host(gate));
				printf("installing as route to %s, metric %d\n",
					in_host(dst), metric);
				fflush(stdout);
			}
			rp->dst = dst;
			rp->gate = gate;
			if(ioctl(ipfd, IPIOROUTE, rp) < 0)
				perror("IPIOROUTE");
		} else {
			if(verbose > 1){
				printf("%s ", in_host(gate));
				printf("confirmed as route to %s, metric %d\n",
					in_host(dst), metric);
				fflush(stdout);
			}
			rp->dst = dst;
			rp->gate = gate;
		}
	}
}

/*
 * keep a list of routes
 * that belong to us;
 * don't let anyone claim them
 */

#define	NSAFE	20

u_long safer[NSAFE];
int nsafe = 0;

recsafe(s)
char *s;
{
	if (nsafe < NSAFE)
		safer[nsafe++] = in_address(s);
}

saferoute(dst)
u_long dst;
{
	register int i;

	for (i = 0; i < nsafe; i++)
		if (dst == safer[i])
			return (1);
	return (0);
}

readgateways()
{
	FILE *fp;
	char buf[512];
	char net[32], gateway[32], which[32];
	u_long dst, gate;
	int metric;

	if((fp = fopen(GATEWAYS, "r")) == 0)
		return;
	while(fgets(buf, sizeof(buf), fp)){
		if(sscanf(buf, "%s %s gateway %s metric %d",
			  which, net, gateway, &metric) != 4)
			continue;
		dst = in_address(net);
		gate = in_address(gateway);
		rtinstall(dst, gate, metric, -1);
	}
}


timer()
{
	int i,j;
	struct route *rp;
	struct ipif ipif;

	signal(SIGALRM, SIG_IGN);
	alarm(0);

	for(j=i=0; j<2*NIFS; i++){
		*(int *)&ipif = i;
		if(ioctl(ipfd, IPIOGETIFS, &ipif) < 0)
			break;
		if(ipif.flags&IFF_UP) {
			ifs[j++] = ipif.that;
			ifs[j++] = ipif.thishost;
		}
	}
	ifs[j]=0;
	for(i = 0; ifs[i]; i+= 2)
		if(ifs[i] != ifs[i+1] && ifs[i] != 0x7f000000)
			rtinstall(in_netof(ifs[i]), in_netof(ifs[i]), 0, 0);

	for(rp = routes; rp < &routes[NROUTES]; rp++){
		if(rp->dst == 0)
			continue;
		if(rp->age > 0 && rp->metric < HOPCNT_INFINITY)
			rp->metric = HOPCNT_INFINITY - 1;
	}

	broadcast();

	for(rp = routes; rp < &routes[NROUTES]; rp++){
		if(rp->dst == 0)
			continue;
		if(rp->age > 10){
			rtdelete(rp);
		} else if(rp->age >= 0){
			rp->age++;
		}
	}

	fflush(stdout);
	alarm(60);
	signal(SIGALRM, timer);
}

broadcast()
{
	struct route *rp;
	struct rip *rip;
	struct netinfo *np;
	struct sockaddr *sin;
	struct udppacket p;

	rip = (struct rip *)p.buf;
	rip->rip_cmd = RIPCMD_RESPONSE;
	rip->rip_vers = RIPVERSION;
	np = rip->rip_nets;
	if(trace)
		printf("BROADCAST:\n");

	/* first send info about directly connected networks */
	for(rp = routes; rp < &routes[NROUTES]; rp++){
		if(rp->dst == 0 || rp->metric > 0)
			continue;
		bzero(np, sizeof(struct netinfo));
		sin = &(np->rip_dst);
		np->rip_metric = htonl(rp->metric + 1);
		sin->sin_port = 0;
		sin->sin_addr = htonl(rp->dst);
		sin->sin_family = htons((u_short)AF_INET);
		if(trace)
			printf(" %s %d\n",
				in_ntoa(ntohl(sin->sin_addr)), ntohl(np->rip_metric));
		np++;
		if (np-rip->rip_nets == MAXROUTESPERPACKET) {
			pbroadcast(&p, (char *)np);
			np = rip->rip_nets;
		}
	}

	/* now send info about the rest */
	for(rp = routes; rp < &routes[NROUTES]; rp++){
		if(rp->dst==0 || rp->metric>=HOPCNT_INFINITY || rp->metric<=0) 
			continue;
		bzero(np, sizeof(struct netinfo));
		sin = &(np->rip_dst);
		np->rip_metric = htonl(rp->metric + 1);
		sin->sin_port = 0;
		sin->sin_addr = htonl(rp->dst);
		sin->sin_family = htons((u_short)AF_INET);
		if(trace)
			printf(" %s %d\n",
				in_ntoa(ntohl(sin->sin_addr)), ntohl(np->rip_metric));
		np++;
		if (np-rip->rip_nets == MAXROUTESPERPACKET) {
			pbroadcast(&p, (char *)np);
			np = rip->rip_nets;
		}
	}
	if (np > rip->rip_nets)
		pbroadcast(&p, (char *)np);
}

/* broadcast one routing packet everywhere */
pbroadcast(pp, cp)
	struct udppacket *pp;
	char *cp;
{
	int len=sizeof(struct udpaddr)+cp-(char *)pp;
	int i;

	if(quiet)
		return;
	pp->addr.port = 520;
	for(i = 0; ifs[i]; i += 2){
		if (trace) {
			printf("on ifs[%d]\n", i);
			fflush(stdout);
		}
		if(ifs[i]!=0 && ifs[i]!=ifs[i+1] && ifs[i]!=0x7f000000) {
			pp->addr.host = ifs[i];
			if (write(udpfd, pp, len) !=len)
				perror("udp write");
		}
	}
}

myaddr(x)
u_long x;
{
	int i;

	for(i = 0; ifs[i]; i += 2)
		if(ifs[i+1] == x)
			return(1);
	return(0);
}

rtdelete(rp)
struct route *rp;
{
	if(verbose) {
		printf("deleting %s %d\n", in_ntoa(rp->dst), rp->metric);
		fflush(stdout);
	}
	if(rp->gate != rp->dst){
		rp->gate = 0;
		if(ioctl(ipfd, IPIOROUTE, rp) < 0)
			perror("IPIOROUTE");
	}
	rp->gate = rp->dst = 0;
	rp->age = rp->metric = 0;
}
