/***********************************************************
Copyright 1987 by Digital Equipment Corporation, Maynard, Massachusetts,
and the Massachusetts Institute of Technology, Cambridge, Massachusetts.

                        All Rights Reserved

Permission to use, copy, modify, and distribute this software and its 
documentation for any purpose and without fee is hereby granted, 
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in 
supporting documentation, and that the names of Digital or MIT not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  

DIGITAL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
DIGITAL BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/
/* $Header: connection.c,v 1.62 87/09/07 17:12:18 rws Exp $ */
/*****************************************************************
 *  Stuff to create connections --- OS dependent
 *
 *      EstablishNewConnections, CreateWellKnownSockets,
 *      CloseDownConnection, CheckConnections, AddEnabledDevice,
 *	RemoveEnabledDevice, OnlyListToOneClient,
 *      ListenToAllClients,
 *
 *      (WaitForSomething is in its own file)
 *
 *      In this implementation, a client socket table is not kept.
 *      Instead, what would be the index into the table is just the
 *      file descriptor of the socket.  This won't work for if the
 *      socket ids aren't small nums (0 - 2^8)
 *
 *****************************************************************/

#include <dbm.h>
#undef NULL
#include "X.h"
#include "Xproto.h"
#include <sys/param.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <signal.h>
#include <fcntl.h>
#include <setjmp.h>
#ifdef TCPCONN
#include <netinet/in.h>
#endif
#ifdef UNIXCONN
#include <sys/un.h>
#endif
#include <stdio.h>
#include <sys/uio.h>
#include <strings.h>
#include "osstruct.h"
#include "osdep.h"
#include "opaque.h"

#include "dixstruct.h"

#ifdef DNETCONN
#include <netdnet/dn.h>
#endif /* DNETCONN */

typedef long CCID;      /* mask of indices into client socket table */

#define X_UNIX_PATH	"/tmp/.X11-unix" 

char *display;			/* The display number */
int lastfdesc;                  /* maximum file descriptor */

long WellKnownConnections;    /* Listener mask */
long EnabledDevices;		/* mask for input devices that are on */
long AllSockets[mskcnt];        /* select on this */
long AllClients[mskcnt];              /* available clients */
long LastSelectMask[mskcnt];          /* mask returned from last select call */
long ClientsWithInput[mskcnt];
long MaxClients = MAXSOCKS ;
long NConnBitArrays = mskcnt;
long FirstClient;

static Bool debug_conns = FALSE;

static char whichByteIsFirst;

static int SavedAllClients[mskcnt];
static int SavedAllSockets[mskcnt];
static int SavedClientsWithInput[mskcnt];
static Bool GrabDone = FALSE;

ClientPtr ConnectionTranslation[MAXSOCKS];
extern ClientPtr NextAvailableClient();

extern ConnectionInput inputBuffers[];

int swappedClients[MAXSOCKS];

extern int AutoResetServer();
extern int GiveUp();

/*****************
 * CreateWellKnownSockets
 *    At initialization, create the sockets to listen on for new clients.
 *    There are potentially 4: DECnet, UNIX Domain, TCP-IP with MSB first, 
 *    with TCP-IP with LSB first.
 *****************/

void
CreateWellKnownSockets()
{
    char	fname[32];
    int		request, i;
    int		whichbyte;	    /* used to figure out whether this is
   					 LSB or MSB */
#ifdef TCPCONN
    struct sockaddr_in insock;
    int		tcpportReg;	    /* port with same byte order as server */

#ifdef SO_LINGER
    static int linger[2] = { 0, 0 };
#endif /* SO_LINGER */

#endif /* TCPCONN */

#ifdef UNIXCONN
    struct sockaddr_un unsock;
    int         oldUmask;
#endif /* UNIXCONN */

#ifdef DNETCONN
    struct sockaddr_dn dnsock;
#endif /* DNETCONN */
    int retry;

    CLEARBITS(AllSockets);
    CLEARBITS(AllClients);
    CLEARBITS(LastSelectMask);
    CLEARBITS(ClientsWithInput);

    for (i=0; i<MAXSOCKS; i++) ConnectionTranslation[i] = (ClientPtr)NULL;
    
    lastfdesc = getdtablesize() - 1;
    if (lastfdesc > MAXSOCKS)
    {
	lastfdesc = MAXSOCKS;
	ErrorF( "GOT TO END OF SOCKETS %d\n", MAXSOCKS);
    }

	/* hack test to decide where to log errors */

    if (write (2, fname, 0)) 
    {
	long t; 

	char *ctime();
        close(stdin);
        close(stdout);
	strcpy (fname, "/usr/adm/X");
	strcat (fname, display);
	strcat (fname, "msgs");
	freopen (fname, "a+", stderr);
	setlinebuf(stderr);
	time (&t);
	fprintf (stderr, "start %s", ctime(&t));
    }
	if (getpgrp (0) == 0)
	    setpgrp (0, getpid ());

    WellKnownConnections = 0;
    whichbyte = 1;
    
    if (*(char *) &whichbyte)
        whichByteIsFirst = 'l';
    else
        whichByteIsFirst = 'B';


#ifdef TCPCONN

    tcpportReg = atoi (display); 
    tcpportReg += X_TCP_PORT;

    if ((request = socket (AF_INET, SOCK_STREAM, 0)) < 0) 
    {
	Notice ("Creating TCP socket");
    } 
    else 
    {
	bzero ((char *)&insock, sizeof (insock));
	insock.sin_family = AF_INET;
	insock.sin_port = htons (tcpportReg);
	insock.sin_addr.s_addr = htonl(INADDR_ANY);
	retry = 20;
	while (i = bind(request, (struct sockaddr *) &insock, sizeof (insock))) 
	{
	    if (--retry == 0)
		Error ("Binding MSB TCP socket");
	    sleep (10);
	}
#ifdef SO_LINGER
	if(setsockopt (request, SOL_SOCKET, SO_LINGER, linger, sizeof(linger)))
	    Notice ("Setting TCP SO_LINGER\n");
#endif /* SO_LINGER */
	if (listen (request, 5))
	    Error ("Reg TCP Listening");
	WellKnownConnections |= (1 << request);
	DefineSelf (request);
    }

#endif /* TCPCONN */

#ifdef UNIXCONN
    unsock.sun_family = AF_UNIX;
    oldUmask = umask (0);
    mkdir (X_UNIX_PATH, 0777);
    (void)umask(oldUmask);
    strcpy (unsock.sun_path, X_UNIX_PATH);
    strcat (unsock.sun_path, "/X");
    strcat (unsock.sun_path, display);
    unlink (unsock.sun_path);
    if ((request = socket (AF_UNIX, SOCK_STREAM, 0)) < 0) 
    {
	Notice ("Creating Unix socket");
    } 
    else 
    {
	if(bind(request,(struct sockaddr *)&unsock, strlen(unsock.sun_path)+2))
	    Error ("Binding Unix socket");
	if (listen (request, 5)) Error ("Unix Listening");
	WellKnownConnections |= (1 << request);
    }
#endif /*UNIXCONN */

#ifdef DNETCONN
    if ((request = socket (AF_DECnet, SOCK_STREAM, 0)) < 0) 
    {
	Notice ("Creating DECnet socket");
    } 
    else 
    {
	bzero ((char *)&dnsock, sizeof (dnsock));
	dnsock.sdn_family = AF_DECnet;
	sprintf(dnsock.sdn_objname, "X%d", atoi (display));
	dnsock.sdn_objnamel = strlen(dnsock.sdn_objname);
	if (bind (request, (struct sockaddr *) &dnsock, sizeof (dnsock)))
		Error ("Binding DECnet socket");
	if (listen (request, 5)) Error ("DECnet Listening");
	WellKnownConnections |= (1 << request);
	DefineSelf (request);
    }
#endif /* DNETCONN */
    if (WellKnownConnections == 0)
        Error ("No Listeners, nothing to do");
    signal (SIGPIPE, SIG_IGN);
    signal (SIGHUP, AutoResetServer);
    signal (SIGINT, GiveUp);
    signal (SIGTERM, GiveUp);
    FirstClient = request + 1;
    AllSockets[0] = WellKnownConnections;
    ResetHosts(display);

    for (i=0; i<MaxClients; i++)
    {
	inputBuffers[i].buffer = (char *) NULL;
	inputBuffers[i].bufptr = (char *) NULL;
	inputBuffers[i].bufcnt = 0;
	inputBuffers[i].lenLastReq = 0;
	inputBuffers[i].size = 0;
    }
}

/* We want to read the connection information.  If the client doesn't
 * send us enough data, however, we want to time out eventually.
 * The scheme is to clear a flag, set an alarm, and keep doing non-blocking
 * reads until we get all the data we want. If the alarm goes
 * off, the handler will clear the flag.  If we see that the flag is
 * cleared, we know we've timed out and return with an error.
 *
 * there remains one problem with this code:
 * there is a window of vulnerability in which we might get an alarm
 * even though all the data has come in properly.  This is because I
 * can't atomically clear the alarm.
 * 
 * Anyone who sees how to fix this problem should do so and
 * submit a fix.
 */

jmp_buf	env;
void TimeOut()
{
    longjmp(env, 1);
}
static Bool 
ReadBuffer(conn, buffer, charsWanted)
    long conn;
    char *buffer;
    int charsWanted;
{
    char *bptr = buffer;
    int got, fTimeOut;
    struct itimerval	itv;

    signal(SIGALRM, TimeOut);
    fTimeOut = FALSE;
    /* only 1 alarm, please, not 1 per minute */
    timerclear(&itv.it_interval);
    itv.it_value.tv_sec = TimeOutValue;
    itv.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);
    /* It better not take a full minute to get to the read call */

    while (charsWanted && (fTimeOut = setjmp(env)) == FALSE)
    {
	got = read(conn, bptr, charsWanted);	
	if (got <= 0)
	    return FALSE;
	if(got > 0)
	{
	    charsWanted -= got;
	    bptr += got;
	    /* Ok, we got something, reset the timer */
 	    itv.it_value.tv_sec = TimeOutValue;
            itv.it_value.tv_usec = 0;
	    setitimer(ITIMER_REAL, &itv, NULL);
	}
    }
    /* disable the timer */
    timerclear(&itv.it_value);
    setitimer(ITIMER_REAL, &itv, NULL);
    /* If we got here and we didn't time out, then return TRUE, because
     * we must have read what we wanted. If we timed out, return FALSE */
    if(fTimeOut && debug_conns)
	ErrorF("Timed out on connection %d\n", conn);
    return (!fTimeOut);
}

/*****************************************************************
 * ClientAuthorized
 *
 *    Sent by the client at connection setup:
 *                typedef struct _xConnClientPrefix {
 *                   CARD8	byteOrder;
 *                   BYTE	pad;
 *                   CARD16	majorVersion, minorVersion;
 *                   CARD16	nbytesAuthProto;    
 *                   CARD16	nbytesAuthString;   
 *                 } xConnClientPrefix;
 *
 *     	It is hoped that eventually one protocol will be agreed upon.  In the
 *        mean time, a server that implements a different protocol than the
 *        client expects, or a server that only implements the host-based
 *        mechanism, will simply ignore this information.
 *
 *****************************************************************/

int 
ClientAuthorized(conn, pswapped, reason)
    long conn;
    int  *pswapped;
    char **reason;   /* if authorization fails, put reason in here */
{
    short slen;
    union {
	struct sockaddr sa;
#ifdef UNIXCONN
	struct sockaddr_un un;
#endif /* UNIXCONN */
#ifdef TCPCONN
	struct sockaddr_in in;
#endif /* TCPCONN */
#ifdef DNETCONN
	struct sockaddr_dn dn;
#endif /* DNETCONN */
    } from;
    int	fromlen;
    xConnClientPrefix xccp;
    char auth_proto[100];
    char auth_string[100];

    if (!ReadBuffer(conn, &xccp, sizeof(xConnClientPrefix)))
    {
	/* If they can't even give us this much, just blow them off
	 * without an error message */
	*reason = 0;
        return 0;
    }
    if (xccp.byteOrder != whichByteIsFirst)
    {        
	SwapConnClientPrefix(&xccp);
	*pswapped = TRUE;
    }
    else
        *pswapped = FALSE;
    if ((xccp.majorVersion != X_PROTOCOL) ||
	(xccp.minorVersion != X_PROTOCOL_REVISION))
    {        
#define STR "Protocol version mismatch"
        *reason = (char *)Xalloc(strlen(STR) + 1);
        strcpy(*reason, STR);
	if (debug_conns)
	    ErrorF("%s\n", STR);
#undef STR
        return 0;
    }
    fromlen = sizeof (from);
    if (getpeername (conn, &from.sa, &fromlen) ||
        InvalidHost (&from.sa, fromlen)) 
    {
#define STR "Server is not authorized to connect to host"	
        *reason = (char *)Xalloc(strlen(STR));
        strcpy(*reason, STR);
#undef STR
        return 0;
    }
    
   slen = (xccp.nbytesAuthProto + 3) & ~3;  
    if ( slen )
        if (!ReadBuffer(conn, auth_proto, slen))
        {
#define STR "Length error in xConnClientPrefix for protocol authorization "
            *reason = (char *)Xalloc(strlen(STR));
            strcpy(*reason, STR);
            return 0;
#undef STR
	}
    auth_proto[slen] = '\0';

    slen = (xccp.nbytesAuthString + 3) & ~3;   
    if ( slen)
        if (!ReadBuffer(conn, auth_string, slen))
        {
#define STR "Length error in xConnClientPrefix for protocol string"
            *reason = (char *)Xalloc(strlen(STR));
            strcpy(*reason, STR);
            return 0;
#undef STR
	}
    auth_string[slen] = '\0';

    /* At this point, if the client is authorized to change the access control
     * list, we should getpeername() information, and add the client to
     * the selfhosts list.  It's not really the host machine, but the
     * true purpose of the selfhosts list is to see who may change the
     * access control list.
     */
    return(1);
}    

static int padlength[4] = {0, 3, 2, 1};

/*****************
 * EstablishNewConnections
 *    If anyone is waiting on listened sockets, accept them.
 *    Returns a mask with indices of new clients.  Updates AllClients
 *    and AllSockets.
 *****************/

void
EstablishNewConnections(newclients, nnew)
    ClientPtr	        *newclients;
    int 		*nnew;
{
    long readyconnections;     /* mask of listeners that are ready */
    long curconn;                  /* fd of listener that's ready */
    long newconn;                  /* fd of new client */
    int	 swapped;		/* set by ClientAuthorized if connection is
				 * swapped */
    char *reason;
    struct iovec iov[2];
    char p[3];

#ifdef TCP_NODELAY
    union {
	struct sockaddr sa;
#ifdef UNIXCONN
	struct sockaddr_un un;
#endif /* UNIXCONN */
#ifdef TCPCONN
	struct sockaddr_in in;
#endif /* TCPCONN */
#ifdef DNETCONN
	struct sockaddr_dn dn;
#endif /* DNETCONN */
    } from;
    int	fromlen;
#endif TCP_NODELAY

    *nnew = 0;
    if (readyconnections = (LastSelectMask[0] & WellKnownConnections)) 
    {
	while (readyconnections) 
	{
	    curconn = ffs (readyconnections) - 1;
	    if ((newconn = accept (curconn,
				  (struct sockaddr *) NULL, 
				  (int *)NULL)) >= 0) 
	    {
		if (newconn >= lastfdesc)
		{
		    if (debug_conns)
ErrorF("Didn't make connection: Out of file descriptors for connections\n");
		    close (newconn);
		} 
		else 
		{
#ifdef TCP_NODELAY
		    fromlen = sizeof (from);
                    if (!getpeername (newconn, &from.sa, &fromlen))
		    {
			if (fromlen && (from.sa.sa_family == AF_INET)) 
			{
			    mi = 1;
			    setsockopt (newconn, IPPROTO_TCP, TCP_NODELAY,
				       &mi, sizeof (int));
			}
		    }
#endif /* TCP_NODELAY */
		    if (ClientAuthorized(newconn, &swapped, &reason))
		    {
			ClientPtr next;

		        fcntl (newconn, F_SETFL, FNDELAY);
			inputBuffers[newconn].used = 1; 
                        if (! inputBuffers[newconn].size) 
			{
			    inputBuffers[newconn].buffer = 
					(char *)Xalloc(BUFSIZE);
			    inputBuffers[newconn].size = BUFSIZE;
			    inputBuffers[newconn].bufptr = 	
					inputBuffers[newconn].buffer;
			}
			if (GrabDone)
			{
			    BITSET(SavedAllClients, newconn);
			    BITSET(SavedAllSockets, newconn);
		        }
			else
			{
			    BITSET(AllClients, newconn);
			    BITSET(AllSockets, newconn);
		        }
			next = NextAvailableClient();
			if (next != (ClientPtr)NULL)
			{
			   osPrivPtr priv;

			   newclients[(*nnew)++] = next;
			   next->swapped = swapped;
			   ConnectionTranslation[newconn] = next;
			   priv =  (osPrivPtr)Xalloc(sizeof(osPrivRec));
			   priv->fd = newconn;
			   next->osPrivate = (pointer)priv;
		        }
		    }
		    else
		    {
			xConnSetupPrefix c;

			if(reason)
			{
			    c.success = xFalse;
			    c.lengthReason = strlen(reason);
			    c.length = (c.lengthReason + 3) >> 2;
			    c.majorVersion = X_PROTOCOL;
			    c.minorVersion = X_PROTOCOL_REVISION;
			    if(swapped)
			    {
				int	n;

				swaps(&c.majorVersion, n);
				swaps(&c.minorVersion, n);
				swaps(&c.length, n);
			    }

			    write(newconn, &c, sizeof(xConnSetupPrefix)); 
			    iov[0].iov_len = c.lengthReason;
			    iov[0].iov_base = reason;
			    iov[1].iov_len = padlength[c.lengthReason & 3];
			    iov[1].iov_base = p;
			    writev(newconn, iov, 2);
			    if (debug_conns)
			        ErrorF("Didn't make connection:%s\n", reason);
			}
			close(newconn);
			Xfree(reason);
		    }

		}
	    }
	    readyconnections &= ~(1 << curconn);
	}
    }
}

/************
 *   CloseDwonFileDescriptor:
 *     Remove this file descriptor and it's inputbuffers, etc.
 ************/

void
CloseDownFileDescriptor(connection)
    long connection;
{
    close(connection);

    inputBuffers[connection].bufptr = inputBuffers[connection].buffer;
    inputBuffers[connection].bufcnt = 0;
    inputBuffers[connection].lenLastReq = 0;
    inputBuffers[connection].used = 0;

    BITCLEAR(AllSockets, connection);
    BITCLEAR(AllClients, connection);
    BITCLEAR(ClientsWithInput, connection);

}

/*****************
 * CheckConections
 *    Some connection has died, go find which one and shut it down 
 *    The file descriptor has been closed, but is still in AllClients.
 *    If would truly be wonderful if select() would put the bogus
 *    file descriptors in the exception mask, but nooooo.  So we have
 *    to check each and every socket individually.
 *****************/

void
CheckConnections()
{
    long		mask[mskcnt];
    long		tmask[mskcnt]; 
    register int	curclient;
    int			i;
    struct timeval	notime;
    ClientPtr           bad;
    int r;

    notime.tv_sec = 0;
    notime.tv_usec = 0;

    COPYBITS(AllClients, mask);
    for (i=0; i<mskcnt; i++)
    {
        while (mask[i])
    	{
	    curclient = ffs (mask[i]) - 1 + (i << 5);
            CLEARBITS(tmask);
            BITSET(tmask, curclient);
            r = select (curclient + 1, tmask, (int *)NULL, (int *)NULL, 
			&notime);
            if (r < 0)
            {
	        if (bad = ConnectionTranslation[curclient])
    		    CloseDownClient(bad);
                else
                    CloseDownFileDescriptor(curclient);
            }
	    BITCLEAR(mask, curclient);
	}
    }	
}


/*****************
 * CloseDownConnection
 *    Delete client from AllClients and free resources 
 *****************/

CloseDownConnection(client)
    ClientPtr client;
{
    int connection = ((osPrivPtr)client->osPrivate)->fd;

    ConnectionTranslation[connection] = (ClientPtr)NULL;
    CloseDownFileDescriptor(connection);
    Xfree(client->osPrivate);
}


AddEnabledDevice(fd)
    int fd;
{
    EnabledDevices |= (1<<fd);
    BITSET(AllSockets, fd);
}


RemoveEnabledDevice(fd)
    int fd;
{
    EnabledDevices &= ~(1<<fd);
    BITCLEAR(AllSockets, fd);
}

/*****************
 * OnlyListenToOneClient:
 *    Only accept requests from  one client.  Continue to handle new
 *    connections, but don't take any protocol requests from the new
 *    ones.  Note that if GrabDone is set, EstablishNewConnections
 *    needs to put new clients into SavedAllSockets and SavedAllClients.
 *    Note also that there is no timeout for this in the protocol.
 *    This routine is "undone" by ListenToAllClients()
 *****************/

OnlyListenToOneClient(client)
    ClientPtr client;
{
    int connection = ((osPrivPtr)client->osPrivate)->fd;

    if (! GrabDone)
    {
	COPYBITS (ClientsWithInput, SavedClientsWithInput);
        BITCLEAR (SavedClientsWithInput, connection);
	if (GETBIT(ClientsWithInput, connection))
	{
	    CLEARBITS(ClientsWithInput);	    
	    BITSET(ClientsWithInput, connection);
	}
	else
        {
	    CLEARBITS(ClientsWithInput);	    
	}
	COPYBITS(AllSockets, SavedAllSockets);
	COPYBITS(AllClients, SavedAllClients);

	UNSETBITS(AllSockets, AllClients);
	BITSET(AllSockets, connection);
	CLEARBITS(AllClients);
	BITSET(AllClients, connection);
	GrabDone = TRUE;
    }
}

/****************
 * ListenToAllClients:
 *    Undoes OnlyListentToOneClient()
 ****************/

ListenToAllClients()
{
    if (GrabDone)
    {
	ORBITS(AllSockets, AllSockets, SavedAllSockets);
	ORBITS(AllClients, AllClients, SavedAllClients);
	ORBITS(ClientsWithInput, ClientsWithInput, SavedClientsWithInput);
	GrabDone = FALSE;
    }	
}


