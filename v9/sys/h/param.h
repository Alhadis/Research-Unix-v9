#ifdef KERNEL
#include "../machine/param.h"
#else
#include <machine/param.h>
#endif

#define	NPTEPG		(NBPG/(sizeof (struct pte)))

/*
 * Tunable variables which do not usually vary per system.
 *
 * The sizes of most system tables are configured
 * into each system description.  The file system buffer
 * cache size is assigned based on available memory.
 * The tables whose sizes don't vary often are given here.
 */

#define	NMOUNT	62		/* number of mountable file systems (cmap.h) */
#define	MSWAPX	15		/* pseudo mount table index for swapdev */
#if MANYPROC
#define	MAXUPRC	75
#else
#define	MAXUPRC	35		/* max processes per user */
#endif
#define	NOFILE	128		/* max open files per process */
#define	NSYSFILE 4		/* stdin, stdout, stderr, /dev/tty */
#define	CANBSIZ	256		/* max size of typewriter line */
#define	NCARGS	(16*1024)	/* # characters in exec arglist */
#define NGROUPS	32		/* number of simultaneous groups */

/*
 * priorities
 * probably should not be
 * altered too much
 */

#define	PSWP	0
#define	PINOD	10
#define	PRIBIO	20
#define	PRIUBA	24
#define	PZERO	25
#define	PPIPE	26
#define	PWAIT	30
#define	PSLEP	40
#define	PUSER	50

#define	NZERO	20

/*
 * signals
 * dont change
 */

#ifndef	NSIG
#ifdef KERNEL
#include "../h/signal.h"
#else
#include <signal.h>
#endif
#endif

/*
 * Return values from tsleep().
 */
#define	TS_OK	0	/* normal wakeup */
#define	TS_TIME	1	/* timed-out wakeup */
#define	TS_SIG	2	/* asynchronous signal wakeup */

/*
 * fundamental constants of the implementation--
 * cannot be changed easily.
 */

#define	NBBY		8		/* number of bits in a byte */
#define	NBPW		sizeof(int)	/* number of bytes in an integer */

#define	NULL	0
#define	CMASK	0		/* default mask for file creation */
#define	NODEV	(dev_t)(-1)
#define	ROOTINO	((ino_t)2)	/* i number of all roots */
#define	SUPERB	((daddr_t)1)	/* block number of the super block */
#define	DIRSIZ	14		/* max characters per directory */

/*
 * Clustering of hardware pages on machines with ridiculously small
 * page sizes is done here.  The paging subsystem deals with units of
 * CLSIZE pte's describing NBPG (from vm.h) pages each... BSIZE must
 * be CLSIZE*NBPG in the current implementation, that is the paging subsystem
 * deals with the same size blocks that the file system uses.
 *
 * NOTE: SSIZE, SINCR and UPAGES must be multiples of CLSIZE
 */
#define	CLBYTES		(CLSIZE*NBPG)
#define	CLOFSET		(CLSIZE*NBPG-1)	/* for clusters, like PGOFSET */

/* give the base virtual address (first of CLSIZE) */
#define	clbase(i)	((i) &~ (CLSIZE-1))

/* round a number of clicks up to a whole cluster */
#define	clrnd(i)	(((i) + (CLSIZE-1)) &~ (CLSIZE-1))

#if CLSIZE==1
#define BITFS(dev)	((dev) & 64)
#define BUFSIZE		8192
#define	BSIZE(dev)	(BUFSIZE)
#define	INOPB(dev)	(128)
#define	BMASK(dev)	(017777)
#define	BSHIFT(dev)	(13)
#define	NMASK(dev)	(03777)
#define	NSHIFT(dev)	(11)
#define	NICINOD	100
#define	NICFREE	946
#define itod(dev, x)	((daddr_t)((((unsigned)(x)+2*INOPB(dev)-1)/INOPB(dev))))
#define itoo(dev, x)	((int)(((x)+2*INOPB(dev)-1)%INOPB(dev)))
#define fsbtodb(dev, b)	((b)*16)
#define dbtofsb(dev, b)	((b)/16)
#define NINDIR(dev)	(BSIZE(dev)/sizeof(daddr_t))
#endif

#ifndef INTRLVE
/* macros replacing interleaving functions */
#define	dkblock(bp)	((bp)->b_blkno)
#define	dkunit(bp)	(minor((bp)->b_dev & 077) >> 3)
/* that means 8 units with at most 8 pieces each */
#endif

#define	CBSIZE	28		/* number of chars in a clist block */
#define	CROUND	0x1F		/* clist rounding; sizeof(int *) + CBSIZE -1*/

/*
 * Macros for fast min/max
 */
#define	MIN(a,b) (((a)<(b))?(a):(b))
#define	MAX(a,b) (((a)>(b))?(a):(b))

/*
 * Macros for counting and rounding.
 */
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))

#ifndef KERNEL
#include	<sys/types.h>
#else
#ifndef LOCORE
#include	"../h/types.h"
#endif
#endif
