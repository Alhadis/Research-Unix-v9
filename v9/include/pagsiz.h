#define	NBPG	8192			/* bytes/page */
#define	PGOFSET		(NBPG-1)	/* byte offset into page */
#define	PGSHIFT		13		/* LOG2(NBPG) */
#define	CLSIZE	1
#define	CLOFSET		(CLSIZE*NBPG-1)	/* for clusters, like PGOFSET */
#define BITFS(dev)	((dev) & 64)
#define	BSIZE(dev)	(4192)
#define	BMASK(dev)	(017777)
#define	BSHIFT(dev)	(13)
#define	PAGSIZ	(NBPG*CLSIZE)
#define	PAGRND	((PAGSIZ)-1)
