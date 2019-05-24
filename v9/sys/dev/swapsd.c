#include "../h/param.h"
#include "../h/conf.h"
/*
 * Single rp0?/rm?? configuration
 *	root on sd1a
 *	paging on sd1b
 */
int	rootfstyp = 0;
dev_t	rootdev	= makedev(7, 8);
dev_t	argdev	= makedev(7, 9);
dev_t	dumpdev	= makedev(7, 9);
long	dumplo	= 33504 - 10 * 2048;

/*
 * Nswap is the basic number of blocks of swap per
 * swap device, and is multiplied by nswdev after
 * nswdev is determined at boot.
 */
int	nswap = 33504;

struct	swdevt swdevt[] =
{
	makedev(7, 9),	0,		/* sd1b */
	0,		0,
};
