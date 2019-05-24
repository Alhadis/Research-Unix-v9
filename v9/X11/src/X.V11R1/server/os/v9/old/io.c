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
/* $Header: io.c,v 1.36 87/09/09 23:04:35 rws Exp $ */
/*****************************************************************
 * i/o functions
 *
 *   WriteToClient, ReadRequestFromClient
 *
 *****************************************************************/

#include <stdio.h>
#include "Xmd.h"
#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include "X.h"
#include "Xproto.h"
#include "os.h"
#include "osdep.h"
#include "opaque.h"
#include "dixstruct.h"

extern long ClientsWithInput[];
extern long PartialRequest;
extern ClientPtr ConnectionTranslation[];
static int timesThisConnection = 0;

extern int errno;

#define request_length(req, cli) ((cli->swapped ? \
	lswaps((req)->length) : (req)->length) << 2)
#define MAX_TIMES_PER         10

/*****************************************************************
 * ReadRequestFromClient
 *    Returns one request from client.  If the client misbehaves,
 *    returns NULL.  The dispatcher closes down all misbehaving clients.  
 *
 *        client:  index into bit array returned from WaitForSomething() 
 *
 *        status: status is set to
 *            > 0 the number of bytes in the request if the read is sucessful 
 *            = 0 if action would block (entire request not ready)
 *            < 0 indicates an error (probably client died)
 *
 *        oldbuf:
 *            To facilitate buffer management (e.g. on multi-processor
 *            systems), the diX layer must tell the OS layer when it is 
 *            done with a request, so the parameter oldbuf is a pointer 
 *            to a request that diX is finished with.  In the 
 *            sample implementation, which is single threaded,
 *            oldbuf is ignored.  We assume that when diX calls
 *            ReadRequestFromClient(), the previous buffer is finished with.
 *
 *    The returned string returned must be contiguous so that it can be
 *    cast in the dispatcher to the correct request type.  Because requests
 *    are variable length, ReadRequestFromClient() must look at the first 4
 *    bytes of a request to determine the length (the request length is
 *    always the 3rd byte in the request).  
 *
 *    Note: in order to make the server scheduler (WaitForSomething())
 *    "fair", the ClientsWithInput mask is used.  This mask tells which
 *    clients have FULL requests left in their buffers.  Clients with
 *    partial requests require a read.  Basically, client buffers
 *    are drained before select() is called again.  But, we can't keep
 *    reading from a client that is sending buckets of data (or has
 *    a partial request) because others clients need to be scheduled.
 *****************************************************************/

ConnectionInput inputBuffers[MAXSOCKS];    /* buffers for clients */

char *
ReadRequestFromClient(who, status, oldbuf)
    ClientPtr who;
    int *status;          /* read at least n from client */
    char *oldbuf;
{
    int client = ((osPrivPtr)who->osPrivate)->fd;
    int result, gotnow, needed;
    register ConnectionInput *pBuff;
    register xReq *request;

        /* ignore oldbuf, just assume we're done with prev. buffer */

    if (client == -1) 
    {
	ErrorF( "OH NO, %d translates to -1\n", who);
	return((char *)NULL);
    }

    pBuff = &inputBuffers[client];
    pBuff->bufptr += pBuff->lenLastReq;
    pBuff->lenLastReq = 0;

            /* handle buffer empty or full case first */

    if ((pBuff->bufptr - pBuff->buffer >= pBuff->bufcnt) || (!pBuff->bufcnt))
    {
        result = read(client, pBuff->buffer, pBuff->size);
	if (result <= 0) 
	{
	    *status = -1;
	    BITCLEAR(ClientsWithInput, client);
	    isItTimeToYield = TRUE;
	    return((char *)NULL);
	}
	else 
	{
	    pBuff->bufcnt = result; 
	    pBuff->bufptr = pBuff->buffer;
	}
    }
              /* now look if there is enough in the buffer */

    request = (xReq *)pBuff->bufptr;
    gotnow = pBuff->bufcnt + pBuff->buffer - pBuff->bufptr;

    if (gotnow < sizeof(xReq))
       needed = sizeof(xReq) - gotnow;
    else
    {
        needed = request_length(request, who);
        if (needed > MAXBUFSIZE)
        {
    	    *status = -1;
	    BITCLEAR(ClientsWithInput, client);
	    isItTimeToYield = TRUE;
    	    return((char *)NULL);
        }
	if (needed <= 0)
            needed = sizeof(xReq);
    }
        /* if the needed amount won't fit in what's remaining,
	   move everything to the front of the buffer.  If the
	   entire header isn't available, move what's there too */
    if ((pBuff->bufptr + needed - pBuff->buffer > pBuff->size) ||
		(gotnow < sizeof(xReq)))
    {
        bcopy(pBuff->bufptr, pBuff->buffer, gotnow);
	pBuff->bufcnt = gotnow;
        if (needed > pBuff->size)
        {
	    pBuff->size = needed;
    	    pBuff->buffer = (char *)Xrealloc(pBuff->buffer, needed);
        }
        pBuff->bufptr = pBuff->buffer;
    }
               /* don't have a full header */
    if (gotnow < sizeof(xReq))
    {
        while (pBuff->bufcnt + pBuff->buffer - pBuff->bufptr < sizeof(xReq))
	{
	    result = read(client, pBuff->buffer + pBuff->bufcnt, 
		      pBuff->size - pBuff->bufcnt); 
	    if (result <= 0)
	    {
		*status = -1;
		BITCLEAR(ClientsWithInput, client);
		isItTimeToYield = TRUE;
		return((char *)NULL);
	    }
            pBuff->bufcnt += result;        
	}
        request = (xReq *)pBuff->bufptr;
        gotnow = pBuff->bufcnt + pBuff->buffer - pBuff->bufptr;
        needed = request_length(request, who);
	if (needed <= 0)
            needed = sizeof(xReq);
        if (needed > pBuff->size)
        {
	    pBuff->size = needed;
    	    pBuff->buffer = (char *)Xrealloc(pBuff->buffer, needed);
        }
        pBuff->bufptr = pBuff->buffer;
    }	

    if (gotnow < needed )   
    {
	int i, wanted;

	wanted = needed - gotnow;
	i = 0;
	while (i < wanted) 
	{
	    result = read(client, pBuff->buffer + pBuff->bufcnt, 
			  pBuff->size - pBuff->bufcnt); 
	    if (result <= 0) { 
		*status = -1;
		BITCLEAR(ClientsWithInput, client);
		isItTimeToYield = TRUE;
		return((char *)NULL);
	    }
	    i += result;
    	    pBuff->bufcnt += result;
	}
    }
    *status = needed;
    pBuff->lenLastReq = needed;

    /*
     *  Check to see if client has at least one whole request in the
     *  buffer.  If there is only a partial request, treat like buffer
     *  is empty so that select() will be called again and other clients
     *  can get into the queue.   
     */

    if (pBuff->bufcnt + pBuff->buffer >= pBuff->bufptr + needed + sizeof(xReq)) 
    {
	request = (xReq *)(pBuff->bufptr + needed);
        if ((pBuff->bufcnt + pBuff->buffer) >= 
            ((char *)request + request_length(request, who)))
	    BITSET(ClientsWithInput, client);
        else
	{
	    BITCLEAR(ClientsWithInput, client);
	    isItTimeToYield = TRUE;
	}
    }
    else
    {
        BITCLEAR(ClientsWithInput, client);
	isItTimeToYield = TRUE;
    }
    if ((++timesThisConnection == MAX_TIMES_PER) || (isItTimeToYield))
    {
        isItTimeToYield = TRUE;
        timesThisConnection = 0;
    }
    return((char *)pBuff->bufptr);
}


/*****************
 * WriteToClient
 *    We might have to wait, if the client isn't keeping up with us.  We 
 *    wait for a short time, then close the connection.  This isn't a 
 *    wonderful solution,
 *    but it rarely seems to be a problem right now, and buffering output for
 *    asynchronous delivery sounds complicated and expensive.
 *    Long word aligns all data.
 *    Makes assumption that at least three bytes after address given
 *	are writeable (used for padding)
 *****************/

    /* lookup table for adding padding bytes to data that is read from
    	or written to the X socket.  */
static int padlength[4] = {0, 3, 2, 1};

int
WriteToClient (who, cnt, buf)
    ClientPtr who;
    char *buf;
    int cnt;
{
#define OUTTIME 2		
    int connection = ((osPrivPtr)who->osPrivate)->fd;
    register int n;

    if (connection == -1) 
    {
	ErrorF( "OH NO, %d translates to -1\n", connection);
	return(-1);
    }

    if (connection == -2) 
    {
#ifdef notdef
	ErrorF( "CONNECTION %d ON ITS WAY OUT\n", connection);
#endif
	return(-1);
    }

    n = write(connection, buf, cnt + padlength[cnt & 3]);
    if (n < cnt) {
	close(connection);
	MarkClientException(who);
	return(-1);
    }
    return(cnt);
}

