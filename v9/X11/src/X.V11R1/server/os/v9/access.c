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

/* $Header: access.c,v 1.18 87/09/03 14:24:02 toddb Exp $ */

#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include <errno.h>
#undef NULL
#include <stdio.h>
#include "dixstruct.h"
#include "osdep.h"


typedef struct _host {
	short		family;
	short		len;
	unsigned char	addr[4];	/* will need to be bigger eventually */
	struct _host *next;
} HOST;

static HOST *selfhosts = NULL;
static HOST *validhosts = NULL;
static int AccessEnabled = TRUE;

/* Define this host for access control.  Find all the hosts the OS knows about 
 * for this fd and add them to the selfhosts list.
 */
DefineSelf (fd)
    int fd;
{}

/* Reset access control list to initial hosts */
ResetHosts (display)
    char *display;
{}

/* Add a host to the access control list.  This is the external interface
 * called from the dispatcher */

int
AddHost (client, family, length, pAddr)
    int			client;
    int                 family;
    int                 length;        /* of bytes in pAddr */
    pointer             pAddr;
{}

/* Add a host to the access control list. This is the internal interface 
 * called when starting or resetting the server */
NewHost (family, addr)
    short	family;
    pointer	addr;
{}

/* Remove a host from the access control list */

int
RemoveHost (client, family, length, pAddr)
    int			client;
    int                 family;
    int                 length;        /* of bytes in pAddr */
    pointer             pAddr;
{}

/* Get all hosts in the access control list */
int
GetHosts (data, pnHosts, pEnabled)
    pointer		*data;
    int			*pnHosts;
    BOOL		*pEnabled;
{
    *pEnabled = AccessEnabled ? EnableAccess : DisableAccess;
    *pnHosts = 0;
    return (0);
}

ChangeAccessControl(client, fEnabled)
    ClientPtr client;
    int fEnabled;
{}
