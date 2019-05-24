/* new netfs headers, adjusted to 8 byte boundaries */
#define NETB 2
struct sendb {
	char version;	/* 1 mostly */
	char cmd;	/* see below */
	char flags;	/* see below, for nami mostly */
	char rsvd;	/* space */
	long trannum;	/* uniq transaction id */

	long len;	/* including this struct and any data */
	long tag;	/* who it is about */
};

/* additional structures for each message type, adjusted to 8 bytes */
/* put		nothing */
/* get		nothing, never sent */
struct snbstat {		/* get */
	time_t	ta;	/* synchronization */
	long	rsvd;	/* padding */
};
/* free		nothing, and no message either */
struct snbup {		/* update, rather overloaded */
	long rsvd2;	/* the server's handle */
	short uid, gid;	/* for the inode */

	unsigned short mode;
	dev_t rdvdd;
	long rsvd;	/* alignment space */

	long ta;
	long tm;	/* access and modified times */
};
struct snbread {
	long len;	/* how much */
	long offset;	/* starting where */
};
struct snbwrite {
	long len;	/* how much (redundant) */
	long offset;
};
/* trunc doesn't need any */
struct snbnami {
	long rsvd;	/* of current directory for request */
	short uid, gid;	/* for permissions and creating */

	short mode;	/* for creating, if any */
	dev_t dev;
	long ino;	/* ino and dev for linking */
};

/* expected responses */
struct recvb {	/* common header */
	long trannum;	/* sanity */
	short errno;	/* error messages */
	char flags;	/* and comments */
	char rsvd;

	long len;	/* total length, including this struct */
	long rsvd2;
};
/* additional responses per command */
/* put		nothing */
struct rnbget {
	short mode;
	short uid, gid;
	short nlink;

	long tag;
	long size;
};
/* free		nothing */
/* updat	server may disagree with changes, but resetting might not
		be a good idea, so nothing */
struct rnbstat {
	long ino;
	dev_t dev;
	short mode;

	short nlink;
	short uid, gid;
	dev_t rdev;	/* is this useful? */

	long size;
	time_t ta;

	time_t tm;
	time_t tc;
};
/* read		nothing (would a redundant len be useful?) */
/* write	ditto */
struct rnbnami {	/* include enough so the next stat can be avoided */
	long tag;
	long ino;

	dev_t dev;
	short mode;
	long used;	/* chars of name used if recvb.flags == NROOT */

	short nlink;
	short uid, gid;
	dev_t rdev;	/* is this useful? */

	long size;
	time_t ta;

	time_t tm;
	time_t tc;
};

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
