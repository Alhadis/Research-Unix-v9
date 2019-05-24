#include "copyright.h"
/* $Header: XConnDis.c,v 11.21 87/09/13 23:03:07 toddb Exp $ */
/* Copyright    Massachusetts Institute of Technology    1985, 1986	*/
#define NEED_EVENTS
#include <stdio.h>
#include "Xlibint.h"
#include <strings.h>

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
	struct utsname uts;
	char displaybuf[256];		/* Display string buffer */	
	char ipcstring[256];		/* ipcopen string */	
	register char *display_ptr;	/* Display string buffer pointer */
	register char *numbuf_ptr;	/* Server number buffer pointer */
	char *screen_ptr;		/* Pointer for locating screen num */
	int display_num;		/* Display number */
        int fd;				/* Network socket */
	int port;
	char numberbuf[16];
	char *dot_ptr = NULL;		/* Pointer to . before screen num */

	/* 
	 * Find the ':' seperator and extract the hostname and the
	 * display number.
	 */
	(void) strncpy(displaybuf, display_name, sizeof(displaybuf));
	if ((display_ptr = SearchString(displaybuf,':')) == NULL) return (-1);
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
        port = display_num + X_TCP_PORT;

	/*
	 * If the display name is missing, use current host.
	 */
	if (displaybuf[0] == '\0') {
		sprintf(ipcstring, "/cs/tcp.%d", port);
		/*
		 * Read in the local system name
		 */
		uname(&uts);
		strcpy(displaybuf, uts.sysname);
	} else
		sprintf(ipcstring, "/cs/tcp!%s!tcp.%d", displaybuf, port);
	if ((fd = ipcopen(ipcstring, "heavy")) == -1) {
		(void) close (fd);
		return(-1);
	}

	/*
	 * set it non-blocking.  This is so we can read data when blocked
	 * for writing in the library.
	 */
	(void) ioctl(fd, FIOWNBLK, 0);
	/*
	 * Return the id if the connection succeeded. Rebuild the expanded
	 * spec and return it in the result parameter.
	 */
	display_ptr = displaybuf;
	while (*(++display_ptr) != '\0')
	    ;
	*(display_ptr++) = ':';
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
	    nfound = select (dpy->fd + 1, r_mask, w_mask, 0x6fffffff);
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
	result = select(dpy->fd + 1, r_mask, NULL, 0x7fffffff);
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

