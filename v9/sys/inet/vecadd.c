#include "../h/param.h"

/*
 * Compute a 16 bit checksum for the internet.
 * This routine will only work on a 68020 as it assumes
 * a short pointer can be used to access odd addresses.
 * If odd is set, reverse the bytes in the sum.
 */
vecadd(w, len, sum, odd)
register u_short *w;
register int len;
register u_long sum;
int odd;
{
	register u_char *c;
	int extra;

	if (odd) {
		c = (u_char *)w;
		sum += *c++;
		w = (u_short *)c;
		len--;
	}
	extra = (int)len & 1;
	len >>= 1;
	while (len--)
		sum += *w++;
	if (extra) {
		c = (u_char *)w;
		sum += *c << 8;
	}
	sum = (sum >> 16) + (sum & 0xffff);
	/* Check for a carry */
	if (sum & 0x10000)
		sum = (sum & ~0x10000) + 1;
	return (sum);
}
