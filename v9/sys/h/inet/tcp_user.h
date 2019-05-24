/*
 * things users need to know to talk to /dev/tcp*
 * open a free tcp device, write a tcpuser struct on it,
 * then wait for a tcpreply.
 */

/* the following is defined in kernel .h's */
#ifndef KERNEL
#include <sys/inet/in.h>
#include <sys/inet/tcp.h>
#endif

struct tcpuser{
	int code;		/* request or reply code */
	tcp_port lport, fport;
	in_addr laddr, faddr;
	int param;		/* options or device number */
};

#define TCPC_LISTEN	1
#define TCPC_CONNECT	2
#define TCPC_OK		3
#define TCPC_SORRY	4
