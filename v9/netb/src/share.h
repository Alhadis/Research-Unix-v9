/* the makefile has to -I the proper directories */
#ifndef ns32000		/* wretched sequent nonsense */
#include "types.h"
#endif
#include "param.h"
#include "stat.h"
#include "errno.h"

/* what the tags refer to */
typedef struct {
	unsigned char *name;	/* the full path name */
	int fd;		/* the file descriptor i've got */
	char flags;	/* 1=>read-only */
	struct stat stb;	/* for various purposes */
	long tag;
	unsigned long pos;	/* cache to avoid some lseeks */
} file;
extern file files[];
#if sun
#define	FILES	28
#else
#define	FILES	128
#endif

/* with this setup, permissions have to be checked on every read/write,
 * which doesn't quite match unix semantics. */

/* translation table between host devs and client devs */
typedef struct {
	int hdev, cdev;
} dev;
extern dev *devs;
extern int ndev;
/* file descriptors */
extern int cfd;		/* talk to client */
extern int dbgfd;	/* debugging output */
extern int pfd;		/* used to read the exceptions during perm setup */

extern int errno;

/* the big server structure, translates from and to client messages */
/* the types have to be adjusted to cope with new kinds of clients */
extern struct client {
	char cmd;
	char flags;
	long trannum;
	long len;
	long tag;
	short uid, gid;
	unsigned short mode;
	long ta, tm, tc;	/* (&ta)[1] must be tm */
	long offset;
	long count;	/* len for read or write */
	int dev;
	long ino;	/* for linking */
	/* additional stuff for responses */
	short errno;
	long size;
	long resplen;
	short nlink;
	int used;
	short namiflags;
} client;

extern unsigned char *inbuf, *nmbuf;
extern unsigned char *slash;	/* pos of last slash in nmbuf */
extern int inlen, iamroot;
extern int nmoffset;	/* used to compute client.used */
extern int hisdev;	/* the major device the client thinks we are */
extern int roottag;	/* root's tag */
extern int otherok;	/* host other perms for mystery users from client? */

/* these are the nami flags, and have to be the same as in the client! (inode.h) */
#define NI_DEL	1	/* unlink this file */
#define NI_CREAT 2	/* create it if it doesn't exits */
#define NI_NXCREAT 3	/* create it, error if it already exists */
#define NI_LINK	4	/* make a link */
#define NI_MKDIR 5	/* make a directory */
#define NI_RMDIR 6	/* remove a directory */

/* these have to be the same as in the client! (netb.h) */
#define NETB 2
/* commands */
#define NBPUT	1
#define NBGET	2
#define NBUPD	3
#define NBREAD	4
#define NBWRT	5
#define NBNAMI	6
#define NBSTAT	7
#define NBIOCTL	8
#define NBTRNC	9
/* response flags */
#define NBROOT 1

/* delta time */
extern int dtime;
#ifndef NULL
#define NULL 0
#endif
#ifndef ROOTINO
#define ROOTINO 2
#endif
extern char *malloc(), *realloc();
extern int proto;	/* values are 'd' for datakit and 't' for tcp */
extern int clienttype;	/* values are 's' for sun, 'c' for cray,
			 * and 'v' for vax */

/* logic for deternining host type */
#if !defined(vax) && !defined(sun) && !defined(cray) && !defined(ns32000)
xx() { xx(host);}
#endif
#if !defined(cray)
#define cray 0
#endif
