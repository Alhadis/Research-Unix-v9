/*
 * base addresses of segments in /proc files
 */

#define	TXTBASE	0		/* base of text */
#define	DATBASE	0		/* base of data */
/* stack is considered part of data */
#define	UBASE	(0-4*NBPG)	/* u-block */
