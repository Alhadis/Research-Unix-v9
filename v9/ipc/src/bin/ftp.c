#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <signal.h>
#include <sys/timeb.h>
#include <errno.h>
#include <sys/inet/tcp_user.h>
#include <libc.h>
#include "ftp.h"
#include "ftp_var.h"

in_addr myaddr;
tcp_port myctlport;
in_addr hisaddr;
tcp_port dataport;
int	data = -1;
int	connected;

FILE	*cin, *cout;
FILE	*dataconn();

char *
hookup(host, port)
	char *host;
	int port;
{
	int s;
	struct tcpuser tu;
	extern in_addr in_address();
	extern char *in_host();

	hisaddr = in_address(host);
	if (hisaddr == 0) {
		perror("ftp: host unknown");
		return (0);
	}
	hostname = in_host(hisaddr);
	if ((s = tcp_sock()) < 0) {
		perror("ftp: socket");
		return (0);
	}
	tu.laddr = 0;
	tu.lport = 0;
	tu.faddr = hisaddr;
	tu.fport = port;
	tu.param = 0;
	if (tcp_connect(s, &tu) < 0) {
		perror("ftp: tcp_connect");
		goto bad;
	}
	myaddr = tu.laddr;
	myctlport = tu.lport;
	cin = fdopen(s, "r");
	cout = fdopen(s, "w");
	if (cin == NULL || cout == NULL) {
		fprintf(stderr, "ftp: fdopen failed.\n");
		if (cin)
			fclose(cin);
		if (cout)
			fclose(cout);
		goto bad;
	}
	if (verbose)
		printf("Connected to %s.\n", hostname);
	(void) getreply(0); 		/* read startup message from server */
	return (hostname);
bad:
	close(s);
	return ((char *)0);
}

login(hp)
	char *hp;
{
	char acct[80];
	char user[80], passwd[80];
	int n;
	struct sgttyb sbuf;
	int oldflags;

	getresponse("login: ", user, sizeof(user));
	ioctl(fileno(stdin), TIOCGETP, &sbuf);
	oldflags = sbuf.sg_flags;
	sbuf.sg_flags = oldflags & ~ECHO;
	ioctl(fileno(stdin), TIOCSETP, &sbuf);
	getresponse("password: ", passwd, sizeof(passwd));
	sbuf.sg_flags = oldflags;
	ioctl(fileno(stdin), TIOCSETP, &sbuf);
	printf("\n"); (void) fflush(stdout);
	n = command("USER %s", user);
	if (n == CONTINUE)
		n = command("PASS %s", passwd);
	if (n == CONTINUE) {
		getresponse("ACCOUNT: ", acct, sizeof(acct));
		n = command("ACCT %s", acct);
	}
	if (n != COMPLETE) {
		fprintf(stderr, "Login failed.\n");
		return (0);
	}
	return (1);
}

getresponse(prompt, cp, len)
	char *prompt;
	char *cp;
	int len;
{
	int c;
	char *p = cp;

	printf(prompt); (void) fflush(stdout);
	while(c = getchar())
		if (c != ' ' && c != '\t')
			break;
	while(c != '\n' && c != EOF) {
		if ((cp + len - 1) > p)
			*p++ = c;
		c = getchar();
	}
	*p = '\0';
}



/*VARARGS 1*/
command(fmt, args)
	char *fmt;
{

	if (debug) {
		printf("---> ");
		_doprnt(fmt, &args, stdout);
		printf("\n");
		(void) fflush(stdout);
	}
	if (cout == NULL) {
		perror ("No control connection for command");
		return (0);
	}
	_doprnt(fmt, &args, cout);
	fprintf(cout, "\r\n");
	(void) fflush(cout);
	return (getreply(!strcmp(fmt, "QUIT")));
}

#include <ctype.h>

getreply(expecteof)
	int expecteof;
{
	register int c, n;
	register int code, dig;
	int originalcode = 0, continuation = 0;

	for (;;) {
		dig = n = code = 0;
		while ((c = getc(cin)) != '\n') {
			dig++;
			if (c == EOF) {
				if (expecteof)
					return (0);
				lostpeer();
				exit(1);
			}
			if (verbose && c != '\r' ||
			    (n == '5' && dig > 4))
				putchar(c);
			if (dig < 4 && isdigit(c))
				code = code * 10 + (c - '0');
			if (dig == 4 && c == '-')
				continuation++;
			if (n == 0)
				n = c;
		}
		if (verbose || n == '5')
			putchar(c);
		if (continuation && code != originalcode) {
			if (originalcode == 0)
				originalcode = code;
			continue;
		}
		if (expecteof || empty(cin))
			return (n - '0');
	}
}

empty(f)
	FILE *f;
{
	int mask;

	if (f->_cnt > 0)
		return (0);
	mask = (1 << fileno(f));
	(void) select(20, &mask, 0, 0);
	return (mask == 0);
}

jmp_buf	sendabort;

abortsend()
{

	longjmp(sendabort, 1);
}

sendrequest(cmd, local, remote)
	char *cmd, *local, *remote;
{
	FILE *fin, *dout, *popen();
	int (*closefunc)(), pclose(), fclose(), (*oldintr)();
	char buf[BUFSIZ];
	register int bytes = 0, c;
	struct stat st;
	struct timeb start, stop;
	extern int errno;

	closefunc = NULL;
	if (setjmp(sendabort))
		goto bad;
	oldintr = signal(SIGINT, abortsend);
	if (strcmp(local, "-") == 0)
		fin = stdin;
	else if (*local == '|') {
		fin = popen(local + 1, "r");
		if (fin == NULL) {
			perror(local + 1);
			goto bad;
		}
		closefunc = pclose;
	} else {
		fin = fopen(local, "r");
		if (fin == NULL) {
			perror(local);
			goto bad;
		}
		closefunc = fclose;
		if (fstat(fileno(fin), &st) < 0 ||
		    (st.st_mode&S_IFMT) != S_IFREG) {
			fprintf(stderr, "%s: not a plain file.", local);
			goto bad;
		}
	}
	if (initconn())
		goto bad;
	if (remote) {
		if (command("%s %s", cmd, remote) != PRELIM)
			goto bad;
	} else
		if (command("%s", cmd) != PRELIM)
			goto bad;
	dout = dataconn("w");
	if (dout == NULL)
		goto bad;
	ftime(&start);
	switch (type) {

	case TYPE_I:
	case TYPE_L:
		errno = 0;
		while ((c = read(fileno (fin), buf, sizeof (buf))) > 0) {
			if (write(fileno (dout), buf, c) < 0)
				break;
			bytes += c;
		}
		if (c < 0)
			perror(local);
		else if (errno)
			perror("netout");
		break;

	case TYPE_A:
		while ((c = getc(fin)) != EOF) {
			if (c == '\n') {
				if (ferror(dout))
					break;
				putc('\r', dout);
				bytes++;
			}
			putc(c, dout);
			bytes++;
			if (c == '\r') {
				putc('\0', dout);
				bytes++;
			}
		}
		if (ferror(fin))
			perror(local);
		else if (ferror(dout))
			perror("netout");
		break;
	}
	ftime(&stop);
	if (closefunc != NULL)
		(*closefunc)(fin);
	(void) fclose(dout);
	(void) getreply(0);
done:
	signal(SIGINT, oldintr);
	if (bytes > 0 && verbose)
		ptransfer("sent", bytes, &start, &stop);
	return;
bad:
	if (data >= 0)
		(void) close(data), data = -1;
	if (closefunc != NULL && fin != NULL)
		(*closefunc)(fin);
	goto done;
}

jmp_buf	recvabort;

abortrecv()
{

	longjmp(recvabort, 1);
}

recvrequest(cmd, local, remote)
	char *cmd, *local, *remote;
{
	FILE *fout, *din, *popen();
	char buf[BUFSIZ];
	int (*closefunc)(), pclose(), fclose(), (*oldintr)(), c;
	register int bytes = 0;
	struct timeb start, stop;
	extern int errno;

	closefunc = NULL;
	if (setjmp(recvabort))
		goto bad;
	oldintr = signal(SIGINT, abortrecv);
	if (strcmp(local, "-") && *local != '|')
		if (access(local, 2) < 0) {
			char *dir = strrchr(local, '/');

			if (dir != NULL)
				*dir = 0;
			if (access(dir ? dir : ".", 2) < 0) {
				perror(local);
				goto bad;
			}
			if (dir != NULL)
				*dir = '/';
		}
	if (initconn())
		goto bad;
	if (remote) {
		if (command("%s %s", cmd, remote) != PRELIM)
			goto bad;
	} else
		if (command("%s", cmd) != PRELIM)
			goto bad;
	if (strcmp(local, "-") == 0)
		fout = stdout;
	else if (*local == '|') {
		fout = popen(local + 1, "w");
		closefunc = pclose;
	} else {
		fout = fopen(local, "w");
		closefunc = fclose;
	}
	if (fout == NULL) {
		perror(local + 1);
		goto bad;
	}
	din = dataconn("r");
	if (din == NULL)
		goto bad;
	ftime(&start);
	switch (type) {

	case TYPE_I:
	case TYPE_L:
		errno = 0;
		while ((c = read(fileno(din), buf, sizeof (buf))) > 0) {
			if (write(fileno(fout), buf, c) < 0)
				break;
			bytes += c;
		}
		if (c < 0)
			perror("netin");
		if (errno)
			perror(local);
		break;

	case TYPE_A:
		while ((c = getc(din)) != EOF) {
			if (c == '\r') {
				bytes++;
				if ((c = getc(din)) != '\n') {
					if (ferror (fout))
						break;
					putc ('\r', fout);
				}
				if (c == '\0') {
					bytes++;
					continue;
				}
			}
			putc (c, fout);
			bytes++;
		}
		if (ferror (din))
			perror ("netin");
		if (ferror (fout))
			perror (local);
		break;
	}
	ftime(&stop);
	(void) fclose(din);
	if (closefunc != NULL)
		(*closefunc)(fout);
	(void) getreply(0);
done:
	signal(SIGINT, oldintr);
	if (bytes > 0 && verbose)
		ptransfer("received", bytes, &start, &stop);
	return;
bad:
	if (data >= 0)
		(void) close(data), data = -1;
	if (closefunc != NULL && fout != NULL)
		(*closefunc)(fout);
	goto done;
}

/*
 * Need to start a listen on the data channel
 * before we send the command, otherwise the
 * server's connect may fail.
 */
initconn()
{
	int result;
	char *a, *p;
	in_addr hostorderaddr;
	tcp_port hostorderport;
	struct tcpuser tu;

	data = tcp_sock();
	if (data < 0) {
		perror("ftp: tcp_sock");
		return (1);
	}
	tu.lport = tu.fport = 0;
	tu.laddr = tu.faddr = 0;
	tu.param = 0;
	if (tcp_listen(data, &tu) < 0) {
		perror("ftp: tcp_listen");
		goto bad;
	}
	dataport = tu.lport;
	hostorderport = htons(dataport);
	hostorderaddr = htonl(myaddr);
	p = (char *)&hostorderport;
	a = (char *)&hostorderaddr;
#define	UC(b)	(((int)b)&0xff)
	result =
	    command("PORT %d,%d,%d,%d,%d,%d",
	      UC(a[0]), UC(a[1]), UC(a[2]), UC(a[3]),
	      UC(p[0]), UC(p[1]));
	return (result != COMPLETE);
bad:
	(void) close(data), data = -1;
	return (1);
}

FILE *
dataconn(mode)
	char *mode;
{
	in_addr from_addr;
	tcp_port from_port;
	struct tcpuser tu;
	int s;

	tu.lport = tu.fport = 0;
	tu.laddr = tu.faddr = 0;
	tu.param = 0;
	s = tcp_accept(data, &tu);
	if (s < 0) {
		perror("ftp: accept");
		(void) close(data), data = -1;
		return (NULL);
	}
	(void) close(data);
	data = s;
	return (fdopen(data, mode));
}

ptransfer(direction, bytes, t0, t1)
	char *direction;
	int bytes;
	struct timeb *t0, *t1;
{
	struct timeb td;
	double ms, bs;

	tvsub(&td, t1, t0);
	ms = (td.time * 1000) + td.millitm;
	ms += 0.05;
	bs = bytes * 1000;
	bs = bs/(ms*1024.0);
	printf("%d bytes %s in %.3f seconds (%.2f Kbytes/s)\n",
		bytes, direction, ms/1000.0, bs);
}

tvadd(tsum, t0)
	struct timeb *tsum, *t0;
{

	tsum->time += t0->time;
	tsum->millitm += t0->millitm;
	if (tsum->time > 1000)
		tsum->time++, tsum->millitm -= 1000;
}

tvsub(tdiff, t1, t0)
	struct timeb *tdiff, *t1, *t0;
{

	tdiff->time = t1->time - t0->time;
	tdiff->millitm = t1->millitm - t0->millitm;
	if (tdiff->millitm < 0)
		tdiff->time--, tdiff->millitm += 1000;
}
