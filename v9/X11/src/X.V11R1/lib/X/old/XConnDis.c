#include "copyright.h"
/* $Header: XConnDis.c,v 11.21 87/09/13 23:03:07 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1985, 1986	*/
#define NEED_EVENTS
/*
 * THIS IS AN OS DEPENDENT FILE! It should work on 4.2BSD derived
 * systems.  VMS and System V should plan to have their own version.
 */
#include <stdio.h>
#include "Xlibint.h"
#include <fcntl.h>
#include <sys/socket.h>
#include <strings.h>

#ifdef UNIXCONN
#include <sys/un.h>
#define X_UNIX_PATH "/tmp/.X11-unix/X"
#endif /* UNIXCONN */
void bcopy();
/* 
 * Attempts to connect to server, given display name. Returns file descriptor
 * (network socket) or -1 if connection fails. The expanded display name
 * of the form hostname:number.screen ("::" if DECnet) is returned in a result
 * parameter. The screen number to use is also returned.
 */
int _XConnectDisplay (display_name, expanded_name, screen_num)

    char *display_name;
    char *expanded_name;	/* return */
    int *screen_num;		/* return */

{
	char displaybuf[256];		/* Display string buffer */	
	register char *display_ptr;	/* Display string buffer pointer */
	register char *numbuf_ptr;	/* Server number buffer pointer */
	char *screen_ptr;		/* Pointer for locating screen num */
	int display_num;		/* Display number */
	struct sockaddr_in inaddr;	/* INET socket address. */
#ifdef UNIXCONN
	struct sockaddr_un unaddr;	/* UNIX socket address. */
#endif
	struct sockaddr *addr;		/* address to connect to */
        struct hostent *host_ptr;
	int addrlen;			/* length of address */
	extern char *getenv();
	extern struct hostent *gethostbyname();
        int fd;				/* Network socket */
	char numberbuf[16];
	char *dot_ptr = NULL;		/* Pointer to . before screen num */
#ifdef DNETCONN
	int dnet = 0;
	char objname[20];
	extern int dnet_conn();
#endif

	/* 
	 * Find the ':' seperator and extract the hostname and the
	 * display number.
	 * NOTE - if DECnet is to be used, the display name is formatted
	 * as "host::number"
	 */
	(void) strncpy(displaybuf, display_name, sizeof(displaybuf));
	if ((display_ptr = SearchString(displaybuf,':')) == NULL) return (-1);
#ifdef DNETCONN
	if (*(display_ptr + 1) == ':') {
	    dnet++;
	    *(display_ptr++) = '\0';
	}
#endif
	*(display_ptr++) = '\0';
 
	/* displaybuf now contains only a null-terminated host name, and
	 * display_ptr points to the display number.
	 * If the display number is missing there is an error. */

	if (*display_ptr == '\0') return(-1);

	/*
	 * Build a string of the form <display-number>.<screen-number> in
	 * numberbuf, using ".0" as the default.
	 */
	screen_ptr = display_ptr;
	numbuf_ptr = numberbuf;
	while (*screen_ptr != '\0') {
	    if (*screen_ptr == '.') {
		dot_ptr = numbuf_ptr;
		*(screen_ptr++) = '\0';
		*(numbuf_ptr++) = '.';
	    } else {
		*(numbuf_ptr++) = *(screen_ptr++);
	    }
	}

	/*
	 * If the spec doesn't include a screen number, add ".0" (or "0" if
	 * only "." is present.)
	 */
	if (dot_ptr == NULL) {
	    dot_ptr = numbuf_ptr;
	    *(numbuf_ptr++) = '.';
	    *(numbuf_ptr++) = '0';
	} else {
	    if (*(numbuf_ptr - 1) == '.')
		*(numbuf_ptr++) = '0';
	}
	*numbuf_ptr = '\0';

	/*
	 * Return the screen number in the result parameter
	 */
	*screen_num = atoi(dot_ptr + 1);

	/*
	 * Convert the server number string to an integer.
	 */
	display_num = atoi(display_ptr);

	/*
	 * If the display name is missing, use current host.
	 */
	if (displaybuf[0] == '\0')
#ifdef DNETCONN
	    if (dnet) 
		(void) strcpy (displaybuf, "0");
            else
#endif
#ifdef UNIXCONN
		;	/* Do nothing if UNIX DOMAIN. Will be handled below. */
#else
		(void) gethostname (displaybuf, sizeof(displaybuf));
#endif

#ifdef DNETCONN
	if (dnet) {
	    /*
	     * build the target object name.
	     */
	    sprintf(objname, "X%d", display_num);
	    /*
	     * Attempt to open the DECnet connection, return -1 if fails.
	     */
	    if ((fd = dnet_conn(displaybuf, 
		   objname, SOCK_STREAM, 0, 0, 0, 0)) < 0)
		return(-1);	    /* errno set by dnet_conn. */
	} else
#endif
	{
#ifdef UNIXCONN
	    if ((displaybuf[0] == '\0') || 
		(strcmp("unix", displaybuf) == 0)) {
		/* Connect locally using Unix domain. */
		unaddr.sun_family = AF_UNIX;
		(void) strcpy(unaddr.sun_path, X_UNIX_PATH);
		strcat(unaddr.sun_path, display_ptr);
		addr = (struct sockaddr *) &unaddr;
		addrlen = strlen(unaddr.sun_path) + 2;
		/*
		 * Open the network connection.
	 	 */
	        if ((fd = socket((int) addr->sa_family, SOCK_STREAM, 0)) < 0)
		    return(-1);	    /* errno set by system call. */
	    } else
#endif
	    {
		/* Get the statistics on the specified host. */
		if ((inaddr.sin_addr.s_addr = inet_addr(displaybuf)) == -1) {
			if ((host_ptr = gethostbyname(displaybuf)) == NULL) {
				/* No such host! */
				errno = EINVAL;
				return(-1);
			}
			/* Check the address type for an internet host. */
			if (host_ptr->h_addrtype != AF_INET) {
				/* Not an Internet host! */
				errno = EPROTOTYPE;
				return(-1);
			}
 
			/* Set up the socket data. */
			inaddr.sin_family = host_ptr->h_addrtype;
			bcopy((char *)host_ptr->h_addr, 
			      (char *)&inaddr.sin_addr, 
			      sizeof(inaddr.sin_addr));
		} else {
			inaddr.sin_family = AF_INET;
		}
		addr = (struct sockaddr *) &inaddr;
		addrlen = sizeof (struct sockaddr_in);
		inaddr.sin_port = display_num;
		inaddr.sin_port += X_TCP_PORT;
		inaddr.sin_port = htons(inaddr.sin_port);
		/*
		 * Open the network connection.
		 */

		if ((fd = socket((int) addr->sa_family, SOCK_STREAM, 0)) < 0)
		    return(-1);	    /* errno set by system call. */
		/* make sure to turn off TCP coalescence */
#ifdef TCP_NODELAY
		{
		int mi;
		setsockopt (fd, IPPROTO_TCP, TCP_NODELAY, &mi, sizeof (int));
		}
#endif
	    }
 

	    if (connect(fd, addr, addrlen) == -1) {
		(void) close (fd);
		return(-1); 	    /* errno set by system call. */
	    }
        }
	/*
	 * set it non-blocking.  This is so we can read data when blocked
	 * for writing in the library.
	 */
	(void) fcntl(fd, F_SETFL, FNDELAY);
	/*
	 * Return the id if the connection succeeded. Rebuild the expanded
	 * spec and return it in the result parameter.
	 */
	display_ptr = displaybuf;
	while (*(++display_ptr) != '\0')
	    ;
	*(display_ptr++) = ':';
#ifdef DNETCONN
	if (dnet)
	    *(display_ptr++) = ':';
#endif
	numbuf_ptr = numberbuf;
	while (*numbuf_ptr != '\0')
	    *(display_ptr++) = *(numbuf_ptr++);
	*display_ptr = '\0';
	(void) strcpy(expanded_name, displaybuf);
	return(fd);

}

/* 
 * Disconnect from server.
 */

int _XDisconnectDisplay (server)

    int server;

{
    (void) close(server);
}

#undef NULL
#define NULL ((char *) 0)
/*
 * This is an OS dependent routine which:
 * 1) returns as soon as the connection can be written on....
 * 2) if the connection can be read, must enqueue events and handle errors,
 * until the connection is writable.
 */
_XWaitForWritable(dpy)
    Display *dpy;
{
    unsigned long r_mask[MSKCNT];
    unsigned long w_mask[MSKCNT];
    int nfound;

    CLEARBITS(r_mask);
    CLEARBITS(w_mask);

    while (1) {
	BITSET(r_mask, dpy->fd);
        BITSET(w_mask, dpy->fd);

	do {
	    nfound = select (dpy->fd + 1, r_mask, w_mask, NULL, NULL);
	    if (nfound < 0 && errno != EINTR)
		(*_XIOErrorFunction)(dpy);
	} while (nfound <= 0);

	if (ANYSET(r_mask)) {
	    char buf[BUFSIZE];
	    long pend_not_register;
	    register long pend;
	    register xEvent *ev;

	    /* find out how much data can be read */
	    if (BytesReadable(dpy->fd, (char *) &pend_not_register) < 0)
		(*_XIOErrorFunction)(dpy);
	    pend = pend_not_register;

	    /* must read at least one xEvent; if none is pending, then
	       we'll just block waiting for it */
	    if (pend < sizeof(xEvent)) pend = sizeof (xEvent);
		
	    /* but we won't read more than the max buffer size */
	    if (pend > BUFSIZE) pend = BUFSIZE;

	    /* round down to an integral number of XReps */
	    pend = (pend / sizeof (xEvent)) * sizeof (xEvent);

	    _XRead (dpy, buf, pend);
	    for (ev = (xEvent *) buf; pend > 0; ev++, pend -= sizeof(xEvent))
	    {
		if (ev->u.u.type == X_Error)
		    _XError (dpy, (xError *) ev);
		else		/* it's an event packet; enqueue it */
		    _XEnq (dpy, ev);
	    }
	}
	if (ANYSET(w_mask))
	    return;
    }
}


_XWaitForReadable(dpy)
  Display *dpy;
{
    unsigned long r_mask[MSKCNT];
    int result;
	
    CLEARBITS(r_mask);
    do {
	BITSET(r_mask, dpy->fd);
	result = select(dpy->fd + 1, r_mask, NULL, NULL, NULL);
	if (result == -1 && errno != EINTR) (*_XIOErrorFunction)(dpy);
    } while (result <= 0);
}

static int padlength[4] = {0, 3, 2, 1};

_XSendClientPrefix (dpy, client)
     Display *dpy;
     xConnClientPrefix *client;
{
	/*
	 * Authorization string stuff....  Must always transmit multiple of 4
	 * bytes.
	 */

        char *auth_proto = ""; 
	int auth_length;
	char *auth_string = "";
	int auth_strlen;
	char pad[3];
	char buffer[BUFSIZ], *bptr;

        int bytes=0;

        auth_length = strlen(auth_proto);
        auth_strlen = strlen(auth_string);
        client->nbytesAuthProto = auth_length;
	client->nbytesAuthString = auth_strlen;

	bytes = (sizeof(xConnClientPrefix) + 
                       auth_length + padlength[auth_length & 3] +
                       auth_strlen + padlength[auth_strlen & 3]);

	bcopy(client, buffer, sizeof(xConnClientPrefix));
        bptr = buffer + sizeof(xConnClientPrefix);
        if (auth_length)
	{
	    bcopy(auth_proto, bptr, auth_length);
            bptr += auth_length;
            if (padlength[auth_length & 3])
	    {
		bcopy(pad, bptr, padlength[auth_length & 3]);
	        bptr += padlength[auth_length & 3];
	    }
	}
        if (auth_strlen)
	{
	    bcopy(auth_string, bptr, auth_strlen);
            bptr += auth_strlen;
            if (padlength[auth_strlen & 3])
	    {
		bcopy(pad, bptr, padlength[auth_strlen & 3]);
	        bptr += padlength[auth_strlen & 3];
	    }
	}
	(void) WriteToServer(dpy->fd, buffer, bytes);
	return;
}

