/*

Copyright 1985, 1986, 1987 by the Massachusetts Institute of Technology

Permission to use, copy, modify, and distribute this
software and its documentation for any purpose and without
fee is hereby granted, provided that the above copyright
notice appear in all copies and that both that copyright
notice and this permission notice appear in supporting
documentation, and that the name of M.I.T. not be used in
advertising or publicity pertaining to distribution of the
software without specific, written prior permission.
M.I.T. makes no representations about the suitability of
this software for any purpose.  It is provided "as is"
without express or implied warranty.

*/

#ifndef lint
static char *rcsid_xhost_c = "$Header: xhost.c,v 11.11 87/08/26 21:14:06 toddb Exp $";
#endif
 
#include <signal.h>
#include <setjmp.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netdb.h>
#include <netinet/in.h>
#ifdef notdef
#include <arpa/inet.h>
	bogus definition of inet_makeaddr() in BSD 4.2 and Ultrix
#else
extern unsigned long inet_makeaddr();
#endif
#ifdef DNETCONN
#include <netdnet/dn.h>
#include <netdnet/dnetdb.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xproto.h>
 
char *index();
int local_xerror();

#define NAMESERVER_TIMEOUT 5	/* time to wait for nameserver */

typedef struct {
	int af, xf;
} FamilyMap;

static FamilyMap familyMap[] = {
#ifdef	AF_DECnet
    {AF_DECnet, FamilyDECnet},
#endif
#ifdef	AF_CHAOS
    {AF_CHAOS, FamilyChaos},
#endif
#ifdef	AF_INET
    {AF_INET, FamilyInternet}
#endif
};

#define FAMILIES ((sizeof familyMap)/(sizeof familyMap[0]))

int nameserver_timedout;
 
static int XFamily(af)
    int af;
{
    int i;
    for (i = 0; i < FAMILIES; i++)
	if (familyMap[i].af == af)
	    return familyMap[i].xf;
    return -1;
}

main(argc, argv)
	int argc;
	char **argv;
{
	Display *dpy;
	char host[256];
	register char *arg;
	int display, i, w, nhosts;
	XHostAddress *address, *get_address();
	char *hostname, *get_hostname();
	XHostAddress *list;
	Bool enabled = False;
#ifdef DNETCONN
	char *dnet_htoa();
	struct nodeent *np;
	struct dn_naddr *nlist, dnaddr, *dnaddrp, *dnet_addr();
	char *cp, *index();
#endif
 
	if ((dpy = XOpenDisplay(NULL)) == NULL) {
	    fprintf(stderr, "%s: Can't open display /* '%s' */\n",
		    argv[0]/*, XDisplayName("\0")*/);
	    exit(1);
	}

	XSetCloseDownMode(dpy, RetainPermanent);

	XSetErrorHandler(local_xerror);
 
 
	if (argc == 1) {
#ifdef DNETCONN
		setnodeent(1); /* keep the database accessed */
#endif
		sethostent(1); /* don't close the data base each time */
		list = XListHosts(dpy, &nhosts, &enabled);
		printf ("Host Access Control %s.\n", 
			enabled ? "enabled": "disabled");
		if (nhosts != 0) {
		    for (i = 0; i < nhosts; i++ )  {
		      hostname = get_hostname(&list[i]);
		      printf("%s\t", hostname);
		      if (nameserver_timedout)
			printf("(nameserver did not respond in %d seconds)\n",
			        NAMESERVER_TIMEOUT);
		      else printf("\n");
		    }
		    free(list);
		    endhostent();
		}
		exit(0);
	}
 
	for (i = 1; i < argc; i++) {
	    arg = argv[i];
	    if (*arg == '-') {
	    
	        if (!argv[i][1] && ((i+1) == argc))
		    XEnableAccessControl(dpy);
		else {
		    arg = argv[i][1]? &argv[i][1] : argv[++i];
                    if ((address = get_address(arg)) == NULL) 
		         fprintf(stderr, "%s: bad host: %s\n", argv[0], arg);
                    else XRemoveHost(dpy, address);
		}
	    } else {
	        if (*arg == '+' && !argv[i][1] && ((i+1) == argc))
		    XDisableAccessControl(dpy);
		else {
		    if (*arg == '+') {
		      arg = argv[i][1]? &argv[i][1] : argv[++i];
		    }
                    if ((address = get_address(arg)) == NULL) 
		         fprintf(stderr, "%s: bad host: %s\n", argv[0], arg);
                    else XAddHost(dpy, address);
		}
	    }
	}
	XCloseDisplay (dpy);  /* does an XSync first */
	exit(0);
}

 

/*
 * get_address - return a pointer to an internet address given
 * either a name (CHARON.MIT.EDU) or a string with the raw
 * address (18.58.0.13)
 */

XHostAddress *get_address (name) 
char *name;
{
  struct hostent *hp;
  static XHostAddress ha;
  static struct in_addr addr;	/* so we can point at it */
#ifdef DNETCONN
  struct dn_naddr *dnaddrp;
  struct nodeent *np;
  char *cp, *index();
  static struct dn_naddr dnaddr;
#endif /* DNETCONN */

#ifdef DNETCONN
  if ((cp = index (name, ':')) && (*(cp + 1) == ':')) {
    *cp = '\0';
    ha.family = FamilyDECnet;
    if (dnaddrp = dnet_addr(name)) {
      dnaddr = *dnaddrp;
    } else {
      if ((np = getnodebyname (name)) == NULL)
	return (NULL);
      dnaddr.a_len = np->n_length;
      bcopy (np->n_addr, dnaddr.a_addr, np->n_length);
    }
    ha.length = sizeof(struct dn_naddr);
    ha.address = (char *)&dnaddr;
    return(&ha);	/* Found it */
  }
#endif /* DNETCONN */
  /*
   * First see if inet_addr() can grok the name; if so, then use it.
   */
  if ((addr.s_addr = inet_addr(name)) != -1) {
    ha.family = FamilyInternet;
    ha.length = sizeof(addr.s_addr);
    ha.address = (char *)&addr.s_addr;
    return(&ha);	/* Found it */
  } 
  /*
   * Is it in the namespace?
   */
  else if (((hp = gethostbyname(name)) == (struct hostent *)NULL)
       || hp->h_addrtype != AF_INET) {
    return (NULL);		/* Sorry, you lose */
  } else {
    ha.family = XFamily(hp->h_addrtype);
    ha.length = hp->h_length;
    ha.address = hp->h_addr;
    return (&ha);	/* Found it. */
  }
}


/*
 * get_hostname - Given an internet address, return a name (CHARON.MIT.EDU)
 * or a string representing the address (18.58.0.13) if the name cannot
 * be found.
 */

jmp_buf env;

char *get_hostname (ha)
XHostAddress *ha;
{
  struct hostent *hp = NULL;
  int nameserver_lost();
  char *inet_ntoa();
#ifdef DNETCONN
  struct nodeent *np;
  static char nodeaddr[16];
#endif /* DNETCONN */

  if (ha->family == FamilyInternet) {
    /* gethostbyaddr can take a LONG time if the host does not exist.
       Assume that if it does not respond in NAMESERVER_TIMEOUT seconds
       that something is wrong and do not make the user wait.
       gethostbyaddr will continue after a signal, so we have to
       jump out of it. 
       */
    nameserver_timedout = 0;
    signal(SIGALRM, nameserver_lost);
    alarm(4);
    if (setjmp(env) == 0) {
      hp = gethostbyaddr (ha->address, ha->length, AF_INET);
    }
    alarm(0);
    if (hp)
      return (hp->h_name);
    else return (inet_ntoa(*((int *)ha->address)));
  }
#ifdef DNETCONN
  if (ha->family == FamilyDECnet) {
    if (np = getnodebyaddr(ha->address, ha->length, AF_DECnet)) {
      sprintf(nodeaddr, "%s::", np->n_name);
    } else {
      sprintf(nodeaddr, "%s::", dnet_htoa(ha->address));
    }
    return(nodeaddr);
  }
#endif

  return (NULL);
}

nameserver_lost()
{
  nameserver_timedout = 1;
  longjmp(env, -1);
}

/*
 * local_xerror - local non-fatal error handling routine. If the error was
 * that an X_GetHosts request for an unknown address format was received, just
 * return, otherwise call the default error handler _XDefaultError.
 */
local_xerror (dpy, rep)
    Display *dpy;
    XErrorEvent *rep;
{
    if ((rep->error_code == BadValue) && (rep->request_code == X_ListHosts)) {
	return;
    } else {
	_XDefaultError(dpy, rep);
    }
}

