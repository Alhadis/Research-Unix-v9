#include <stdio.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/inet/in.h>
#define KERNEL	1	/* get kernel definitions */
#include <sys/inet/tcp.h>
#include <sys/inet/socket.h>
#include <sys/inet/udp.h>
#include <sys/inet/ip_var.h>
#include <sys/inet/tcp_timer.h>
#define TCPSTATES 1
#include <sys/inet/tcp_fsm.h>
#include <sys/inet/tcp_var.h>
#include <sys/inet/tcpip.h>
#include <sys/inet/tcpdebug.h>
#include <sys/ethernet.h>

static int strip = 0; /* true if we're stripping the high bit from kernel addrs */

/* predeclared */
doseek();
void doseekoff();
void doread();

main(argc, argv)
char *argv[];
{
	char *xinu, *kmem;
	char action = '\0';
	char *operand;
	int ai=1;

	xinu = "/unix";
	kmem = "/dev/kmem";

	if(argc > 1 && argv[ai][0] == '-') {
		action = argv[ai][1];
		operand = &argv[ai++][2];
	}
	if (argc > ai+1)
		xinu = argv[ai++];
	if (argc > ai)
		kmem = argv[ai];
	kern_init(xinu, kmem);

	switch(action) {
	case '\0':
	case 'c':
		tcps(0);
		udps();
		break;
	case 'i':
		interfaces();
		break;
	case 's':
		statistics();
		break;
	case 'r':
		routes(0);
		break;
	case 'R':
		routes(1);
		break;
	case 'a':
		arps();
		break;
	case 'C':
		tcps(1);
		break;
	case 't':
		if (*operand == '\0' || strcmp(operand, "tcp")==0)
			debugtcp();
		else if (strcmp(operand, "il")==0)
			debugil();
		else if (strcmp(operand, "qe")==0)
			debugqe();
		break;
	default:
		fprintf(stderr, "Usage: %s -icCrat [unix [vmcore]]\n", argv[0]);
		break;
	}
}

/*
 *  truncate name to rightmost n chars
 */
trunc(name, n)
	char *name;
	int n;
{
	register int i=strlen(name);

	return ((i<=n) ? name : name+(i-n));
}


#include "nlist.h"
struct nlist nl[] = {
	{"_ipif", 0},
#define NL_INET 0
	{"_ipstat", 0},
#define NL_IPSTAT 1
	{"_tcpstat", 0},
#define NL_TCPSTAT 2
	{"_Ntcp", 0},
#define NL_NTCP 3
	{"_tcpsocks", 0},
#define NL_TCP 4
	{"_ip_routes", 0},
#define NL_ROUTE 5
	{"_ip_arps", 0},
#define NL_ARP 6
	{"_Ninet", 0},
#define NL_NINET 7
	{"_Nip_route", 0},
#define NL_NROUTE 8
	{"_Nip_arp", 0},
#define NL_NARP 9
	{"_ip_default_route", 0},
#define NL_DR 10
	{"_bugarr", 0},
#define NL_TCPDEB 11
	{"_Nbugarr", 0},
#define NL_NBUGARR 12
	{"_Nudp", 0},
#define NL_NUDP 13
	{"_udpconn", 0},
#define NL_UDP 14
	{"_ild", 0},
#define NL_ILDEB 15
	{"_qed", 0},
#define NL_QEDEB 16
	{0, 0}
};
int kern_fd;

kern_init(xinu, kmem)
char *xinu, *kmem;
{
	int i;
	nlist(xinu, nl);
	if((long)nl[0].n_value == 0){
		fprintf(stderr, "nlist %s failed\n", xinu);
		exit(1);
	}
	if(strcmp(kmem, "/dev/kmem") != 0){
		for(i = 0; nl[i].n_name; i++){
			nl[i].n_value &= 0xffffff;
		}
		strip = 1;
	}
	kern_fd = open(kmem, 0);
	if(kern_fd < 0){
		perror(kmem);
		exit(1);
	}
}

interfaces()
{
	extern char *xflags();
	struct ipif ipif[128];
	int i, ninet;

	if (doseek(NL_NINET) < 0) {
		printf("Internet not compiled into this kernel\n");
		return;
	}
	doread((char *)&ninet, sizeof ninet);
	if (ninet > (sizeof(ipif)/sizeof(struct ipif)))
		ninet = sizeof(ipif)/sizeof(struct ipif);
	printf("%-4s %-12s %-12s  %5s %6s %6s %6s %6s\n",
		"Mtu", "Network", "Address", "Flags",
		"Ipkts", "Ierrs", "Opkts", "Oerrs");
	doseek(NL_INET);
	doread((char *)ipif, ninet*sizeof(struct ipif));
	for(i = 0; i < ninet; i++){
		if((ipif[i].flags&IFF_UP) == 0)
			continue;
		printf("%-4d %-12s ", ipif[i].mtu,
			trunc(in_host(ipif[i].that),12));
		printf("%-12s  %5s %6d %6d %6d %6d\n",
			trunc(in_host(ipif[i].thishost),12),
			xflags(ipif[i].flags, "UHA?", "   "),
			ipif[i].ipackets, ipif[i].ierrors,
			ipif[i].opackets, ipif[i].oerrors);
	}
}

statistics()
{
	struct ipstat stats;
	struct tcpstat tcpstat;

	if (doseek(NL_IPSTAT) < 0) {
		printf("Internet not compiled into this kernel\n");
		return;
	}
	doread((char *)&stats, sizeof(stats));
	printf("IP:\n");
	printf("%6d bad sums\n", stats.ips_badsum);
	printf("%6d short packets\n", stats.ips_tooshort);
	printf("%6d short data\n", stats.ips_toosmall);
	printf("%6d bad header lengths\n", stats.ips_badhlen);
	printf("%6d real bad header lengths\n", stats.ips_badlen);
	printf("%6d queue overflows\n", stats.ips_qfull);
	printf("%6d output routing errors\n", stats.ips_route);
	printf("%6d fragmented packets\n", stats.ips_fragout);
	if (doseek(NL_TCPSTAT) < 0) {
		printf("Tcp not compiled into this kernel\n");
		return;
	}
	doread((char *)&tcpstat, sizeof(tcpstat));
	printf("TCP:\n");
	printf("%6d bad sums\n", tcpstat.tcps_badsum);
	printf("%6d bad offsets\n", tcpstat.tcps_badoff);
	printf("%6d header drops\n", tcpstat.tcps_hdrops);
	printf("%6d bad segments\n", tcpstat.tcps_badsegs);
	printf("%6d retransmit timeouts\n", tcpstat.tcps_timeouts[0]);
	printf("%6d persist timeouts\n", tcpstat.tcps_timeouts[1]);
	printf("%6d keep-alive timeouts\n", tcpstat.tcps_timeouts[2]);
	printf("%6d quiet time timeouts\n", tcpstat.tcps_timeouts[3]);
	printf("%6d duplicate packets received\n", tcpstat.tcps_duplicates);
	printf("%6d possibly late packets received\n", tcpstat.tcps_delayed);
}

tcps(flag)
{
	int i, ntcp;
	struct socket so;
	struct tcpcb tcpcb;
	extern char *xflags();
	char b1[100], b2[100];
	struct in_service *sp;
#define SS_INTERESTING (SS_RCVATMARK|SS_OPEN|SS_ACTIVE|SS_WAITING|SS_PLEASEOPEN)

	if (doseek(NL_NTCP) < 0) {
		printf("Tcp not compiled into this kernel\n");
		return;
	}
	doread((char *)&ntcp, sizeof ntcp);
	printf("Proto Dev Wque Rque State %18s %18s %14s\n",
		"Remote Addr", "Local Addr", "Cstate");
	for(i = 0; i < ntcp; i++){
		doseekoff(NL_TCP, sizeof(so)*i);
		doread((char *)&so, sizeof(so));
		if((so.so_state&SS_INTERESTING) == 0)
			continue;
		if(so.so_tcpcb){
			if(strip)
				so.so_tcpcb =
					(struct tcpcb *)((int)(so.so_tcpcb)&0xffffff);
			lseek(kern_fd, so.so_tcpcb, 0);
			doread((char *)&tcpcb, sizeof(tcpcb));
		} else {
			tcpcb.t_state = 0;
		}
		sp = in_service(0, "tcp", so.so_fport);
		sprintf(b1, "%11s.%s", trunc(in_host(so.so_faddr),11), sp->name);
		sp = in_service(0, "tcp", so.so_lport);
		sprintf(b2, "%11s.%s", trunc(in_host(so.so_laddr),11), sp->name);
		if(!flag || tcpcb.t_state != TCPS_LISTEN)
		printf("  tcp  %02d %4d %4d %5s %18.18s %18.18s %14s\n",
			so.so_dev,
			so.so_wcount, so.so_rcount,
			xflags(so.so_state, "OPRWA?", "      "),
			b1, b2,
			tcpstates[tcpcb.t_state]);
#ifdef ALL
		if(!flag || tcpcb.t_state != TCPS_LISTEN)
			printf("template %x\n", tcpcb.t_template);
#endif
		if(flag && tcpcb.t_state != TCPS_LISTEN){
			printf("  Timers:\n");
			printf("\tretransmit %d\n\tpersist %d\n\tkeepalive %d\n\t2msl %d\n",
				tcpcb.t_timer[0], tcpcb.t_timer[1],
				tcpcb.t_timer[2], tcpcb.t_timer[3]);
			printf("  Send sequence variables:");
			printf("\n\tunacked %d\n\tnext %d\n\turgent ptr %d",
				tcpcb.snd_una, tcpcb.snd_nxt, tcpcb.snd_up);
			printf("\n\tinitial number %d\n\twindow %d\n\thighest sent %d\n",
				tcpcb.iss, tcpcb.snd_wnd, tcpcb.snd_max);
			printf("  Receive sequence variables:");
			printf("\n\tnext %d\n\turgent ptr %d",
				tcpcb.rcv_nxt, tcpcb.rcv_up);
			printf("\n\tinitial number %d\n\twindow %d\n\tadvertised %d\n",
				tcpcb.irs, tcpcb.rcv_wnd, tcpcb.rcv_adv);
			printf("  Transmit timing:");
			printf("\n\tinactive %d\n\tround trip %d\n\tseq # timed %d\n\tsmoothed round trip %f\n",
				tcpcb.t_idle, tcpcb.t_rtt, tcpcb.t_rtseq,
				tcpcb.t_srtt);
			printf("  Status:");
			printf("\n\tmax segment size %d", tcpcb.t_maxseg);
			printf("\n\tforcing out byte %d", tcpcb.t_force);
			printf("\n\tflags 0x%x\n", tcpcb.t_flags);
			printf("\n");
		}
	}
}

udps()
{
	int i, nudp;
	struct udp udp[132];
	extern char *xflags();
	char b1[100], b2[100];
	struct in_service *sp;

	if (doseek(NL_NUDP) < 0) {
		printf("Udp not compiled into this kernel\n");
		return;
	}
	doread((char *)&nudp, sizeof nudp);
	printf("\nProto Dev State %18s %18s\n",
		"Remote Addr", "Local Addr");
	doseek(NL_UDP);
	doread((char *)udp, nudp*sizeof(struct udp));
	for (i = 0; i < nudp; i++) {
		if(udp[i].rq == 0)
			continue;
		sp = in_service(0, "udp", udp[i].sport);
		sprintf(b1, "%s.%.6s", trunc(in_host(udp[i].src),11), sp->name);
		sp = in_service(0, "udp", udp[i].dport);
		sprintf(b2, "%s.%.6s", trunc(in_host(udp[i].dst),11), sp->name);
		printf("  udp  %02d %5s %18.18s %18.18s\n",
			i, xflags(udp[i].flags, "ILC?", "      "),
			b2, b1);
	}
}

char *
xflags(fl, fs, buf)
char *fs, *buf;
{
	int i, len;
	char *os;

	os = buf;
	len = strlen(fs);
	for(i = 0; i < len; i++){
		if(fl & (1<<i))
			*buf++ = fs[i];
	}
	*buf++ = '\0';
	return(os);
}

routes(all)
	int all;
{
	struct ip_route r[256];
	int i;
	int nroute;

	doseek(NL_DR);
	doread((char *)r, sizeof(struct ip_route));
	if (r[0].gate != 0)
		printf("Default route is %s\n\n", in_host(r[0].gate));
	doseek(NL_NROUTE);
	doread((char *)&nroute, sizeof nroute);
	if (nroute > (sizeof(r)/sizeof(struct ip_route)))
		nroute = sizeof(r)/sizeof(struct ip_route);
	doseek(NL_ROUTE);
	doread((char *)r, nroute*sizeof(struct ip_route));
	printf("%-14s %-14s\n", "Destination", "Gateway");
	for(i = 0; i < nroute; i++){
		if(all || r[i].dst!=r[i].gate){
			printf("%-14s ", in_host(r[i].dst));
			printf("%-14s\n", in_host(r[i].gate));
		}
	}
}

arps()
{
	struct ip_arp a[256];
	int i, j, narp;

	if (doseek(NL_NARP) < 0) {
		printf("Arp not compiled into this kernel\n");
		return;
	}
	doread((char *)&narp, sizeof narp);
	if (narp > (sizeof(a)/sizeof(struct ip_arp)))
		narp = sizeof(a)/sizeof(struct ip_arp);
	doseek(NL_ARP);
	doread((char *)a, narp*sizeof(struct ip_arp));
	printf("%-11s Ether-Address\n", "Host");
	for(i = 0; i < narp; i++){
		if(a[i].inaddr){
			printf("%-11s ", in_host(a[i].inaddr));
			for(j = 0; j < 6; j++){
				printf("%02x ", a[i].enaddr[j]);
			}
			printf("\n");
		}
	}
}

#define NEXT(i) ((i+1)%SIZDEBUG)
#define PREV(i) ((i+SIZDEBUG-1)%SIZDEBUG)
#define HEADER\
"\ntime             src           dst      seq bytes      ack wind cksm      urg flags\n"
#define FORMAT "%4d %c %8x.%-4d %8x.%-4d %8x   %3x %8x %4x %04x %8x %s\n"

debugtcp()
{
	struct tcpdebug d[SIZDEBUG];
	int i, lasti= 0;
	time_t t, lastt = 0;
	time_t base = 0;
	int line = 0;
	char *xflags();
	char xbuf[9];

	for(;;) {
		/* get array */
		if (doseek(NL_TCPDEB) < 0) {
			printf("Tcp debugging not compiled into this kernel\n");
			return;
		}
		doread((char *)d, sizeof(d));

		/* see if we've overflowed the buffer (or first time around) */
		if (d[lasti].stamp != lastt) {
			if (lastt != 0)
				printf("** missing messages **\n");
			/* find last(highest) timestamped entry */
			for (i = NEXT(lasti); i != lasti; i = NEXT(i)) {
				if (d[i].stamp < lastt) {
					if (base == 0)
						base = d[i].stamp;
					lasti = PREV(i);
					lastt = 0; /* for start of next loop */
					break;
				}
				lastt = d[i].stamp;
			}
		}

		/* print all new entries */
		for (i = NEXT(lasti); i != lasti; i = NEXT(i)) {
			if (d[i].stamp < lastt)
				break;
			if (d[i].stamp == 0)
				continue;
			if ((line++ % 22) == 0)
				printf(HEADER);
			printf(FORMAT, d[i].stamp-base, d[i].inout ? 'o' : 'i',
				ntohl(d[i].hdr.ti_src), ntohs(d[i].hdr.ti_sport),
				ntohl(d[i].hdr.ti_dst), ntohs(d[i].hdr.ti_dport),
				ntohl(d[i].hdr.ti_seq), 
				ntohs(d[i].hdr.ti_len) - sizeof(struct tcphdr),
				ntohl(d[i].hdr.ti_ack),
				ntohs(d[i].hdr.ti_win),
				ntohs(d[i].hdr.ti_sum),
				ntohs(d[i].hdr.ti_urp),
				xflags(d[i].hdr.ti_flags&0xff, "FSRPAU??", xbuf)
			);
			lastt = d[i].stamp;
			lasti = i;
		}
		fflush(stdout);
		sleep(1);
	}
}

#define QEDEBSIZE 64
#define QENEXT(i) ((i+1)%QEDEBSIZE)
#define QEPREV(i) ((i+QEDEBSIZE-1)%QEDEBSIZE)
#define QEHEADER\
"\n   time            src          dst     type\n"
#define QEFORMAT "%7d %c %02x%02x%02x%02x%02x%02x %02x%02x%02x%02x%02x%02x %4x\n"

#define ILDEBSIZE 64
#define ILNEXT(i) ((i+1)%ILDEBSIZE)
#define ILPREV(i) ((i+ILDEBSIZE-1)%ILDEBSIZE)
#define ILHEADER\
"\n   time            dst          arp     type\n"
#define ILFORMAT "%7d %c %02x%02x%02x%02x%02x%02x %02x%02x%02x%02x%02x%02x %4x\n"

debugil()
{
	struct {
		time_t	time;
		unsigned short code;
		struct etherpup pup;
	} d[ILDEBSIZE];
	int i, lasti= 0;
	time_t t, lastt = 0;
	time_t base = 0;
	int line = 0;
	char *xflags();
	char xbuf[9];

	for(;;) {
		/* get array */
		if (doseek(NL_ILDEB) < 0) {
			printf("Interlan debugging not compiled into this kernel\n");
			return;
		}
		doread((char *)d, sizeof(d));

		/* see if we've overflowed the buffer (or first time around) */
		if (d[lasti].time != lastt) {
			if (lastt != 0)
				printf("** missing messages **\n");
			/* find last(highest) timestamped entry */
			for (i = ILNEXT(lasti); i != lasti; i = ILNEXT(i)) {
				if (d[i].time < lastt) {
					if (base == 0)
						base = d[i].time;
					lasti = ILPREV(i);
					lastt = 0; /* for start of next loop */
					break;
				}
				lastt = d[i].time;
			}
		}

		/* print all new entries */
		for (i = ILNEXT(lasti); i != lasti; i = ILNEXT(i)) {
			if (d[i].time < lastt)
				break;
			if (d[i].time == 0)
				continue;
			if ((line++ % 22) == 0)
				printf(ILHEADER);
			printf(ILFORMAT, d[i].time-base, d[i].code?'o':'i',
				 d[i].pup.dhost[0], d[i].pup.dhost[1],
				 d[i].pup.dhost[2], d[i].pup.dhost[3],
				 d[i].pup.dhost[4], d[i].pup.dhost[5],
				 d[i].pup.shost[0], d[i].pup.shost[1],
				 d[i].pup.shost[2], d[i].pup.shost[3],
				 d[i].pup.shost[4], d[i].pup.shost[5],
				 d[i].pup.type
			);
			lastt = d[i].time;
			lasti = i;
		}
		fflush(stdout);
		sleep(1);
	}
}

debugqe()
{
	struct {
		time_t	time;
		unsigned short code;
		struct etherpup pup;
	} d[QEDEBSIZE];
	int i, lasti= 0;
	time_t t, lastt = 0;
	time_t base = 0;
	int line = 0;
	char *xflags();
	char xbuf[9];

	for(;;) {
		/* get array */
		if (doseek(NL_QEDEB) < 0) {
			printf("Interlan debugging not compqeed into this kernel\n");
			return;
		}
		doread((char *)d, sizeof(d));

		/* see if we've overflowed the buffer (or first time around) */
		if (d[lasti].time != lastt) {
			if (lastt != 0)
				printf("** missing messages **\n");
			/* find last(highest) timestamped entry */
			for (i = QENEXT(lasti); i != lasti; i = QENEXT(i)) {
				if (d[i].time < lastt) {
					if (base == 0)
						base = d[i].time;
					lasti = QEPREV(i);
					lastt = 0; /* for start of next loop */
					break;
				}
				lastt = d[i].time;
			}
		}

		/* print all new entries */
		for (i = QENEXT(lasti); i != lasti; i = QENEXT(i)) {
			if (d[i].time < lastt)
				break;
			if (d[i].time == 0)
				continue;
			if ((line++ % 22) == 0)
				printf(QEHEADER);
			printf(QEFORMAT, d[i].time-base, d[i].code?'o':'i',
				 d[i].pup.dhost[0], d[i].pup.dhost[1],
				 d[i].pup.dhost[2], d[i].pup.dhost[3],
				 d[i].pup.dhost[4], d[i].pup.dhost[5],
				 d[i].pup.shost[0], d[i].pup.shost[1],
				 d[i].pup.shost[2], d[i].pup.shost[3],
				 d[i].pup.shost[4], d[i].pup.shost[5],
				 d[i].pup.type
			);
			lastt = d[i].time;
			lasti = i;
		}
		fflush(stdout);
		sleep(1);
	}
}

doseek(nlitem)
	unsigned int nlitem;
{
	if (nl[nlitem].n_value == 0)
		return -1;
	if(lseek(kern_fd, (long)nl[nlitem].n_value, 0) == -1){
		perror("seek");
		exit(1);
	}
	return 0;
}

void
doseekoff(nlitem, offset)
	unsigned int nlitem;
	unsigned int offset;
{
	if(lseek(kern_fd, (long)(nl[nlitem].n_value+offset), 0) == -1){
		perror("seek");
		exit(1);
	}
}

void
doread(addr, size)
	char *addr;
	unsigned int size;
{
	if(read(kern_fd, addr, size) < 0){
		perror("read");
		exit(1);
	}
}
