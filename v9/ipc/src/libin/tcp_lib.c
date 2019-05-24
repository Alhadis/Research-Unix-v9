#include <sys/inet/tcp_user.h>
#include <sys/inet/socket.h>
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include "config.h"

extern errno;

tcp_sock()
{
	int fd, n;
	char name[32];

	for(n = 01; n < 100; n += 2){
		sprintf(name, "/dev/tcp%02d", n);
		fd = open(name, 2);
		if(fd >= 0)
			return(fd);
		if(errno != ENXIO)
			break;
	}
	perror(name);
	return(-1);
}

tcp_connect(fd, tp)
	int fd;
	struct tcpuser *tp;
{
	tp->code = TCPC_CONNECT;
	if (write(fd, (char *)tp, sizeof(struct tcpuser))!=sizeof(struct tcpuser))
		return -1;
	if (read(fd, (char *)tp, sizeof(struct tcpuser))!=sizeof(struct tcpuser))
		return -2;
	if(tp->code != TCPC_OK)
		return -3;
	return 0;
}

tcp_listen(fd, tp)
	int fd;
	struct tcpuser *tp;
{
	tp->code = TCPC_LISTEN;
	if (write(fd, (char *)tp, sizeof(struct tcpuser))!=sizeof(struct tcpuser))
		return -1;
	if (read(fd, (char *)tp, sizeof(struct tcpuser))!=sizeof(struct tcpuser))
		return -1;
	if(tp->code != TCPC_OK)
		return -1;
	return 0;
}

tcp_accept(fd, tp)
	int fd;
	struct tcpuser *tp;
{
	char name[128];

	if (read(fd, (char *)tp, sizeof(struct tcpuser))!=sizeof(struct tcpuser))
		return -1;
	if (tp->code != TCPC_OK)
		return -1;
	sprintf(name, "/dev/tcp%02d", tp->param);
	return open(name, 2);
}

tcp_rcmd(host, port, locuser, remuser, cmd, fd2p)
char *host, *port, *locuser, *remuser, *cmd;
int *fd2p;
{
	char buf[1];
	int fd;
	struct in_service *sp;
	struct tcpuser tu;
	in_addr in_address();

	tu.faddr = in_address(host);
	if(tu.faddr == 0){
		fprintf(stderr, "%s: unknown host\n", host);
		return(-1);
	}
	sp = in_service(port, "tcp", 0L);
	if(sp == 0){
		fprintf(stderr, "%s: unknown service\n", port);
		return(-1);
	}
	tu.fport = sp->port;
	fd = tcp_sock();
	if(fd < 0){
		perror(host);
		return(-1);
	}
	tu.laddr = INADDR_ANY;
	tu.lport = 0;
	tu.param = 0;
	if(tcp_connect(fd, &tu) < 0){
		fprintf(stderr, "%s: connection failed\n", host);
		goto bad;
	}

	if(fd2p == 0)
		write(fd, "", 1);
	else
		goto bad;	/* later */

	write(fd, locuser, strlen(locuser)+1);
	write(fd, remuser, strlen(remuser)+1);
	write(fd, cmd, strlen(cmd)+1);

	if(read(fd, buf, 1) != 1){
		perror(host);
		goto bad;
	}
	if(buf[0] != 0){
		while(read(fd, buf, 1) == 1){
			write(2, buf, 1);
			if(buf[0] == '\n')
				break;
		}
		goto bad;
	}
	return(fd);
bad:
	close(fd);
	return(-1);
}
