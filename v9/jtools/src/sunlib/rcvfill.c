#include "jerq.h"
#include "rcv.h"

rcvfill ()
{
	static char rbuf[1024];
	register i;

	i = min (1024, Jrcvbuf.size - Jrcvbuf.cnt);
	i = read(0, rbuf, i);
	if (i <= 0)
		exit(0);
	rcvbfill(rbuf, i);
}
