#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/inet/in.h>

extern errno;

main(argc, argv)
char *argv[];
{
	struct goo{
		u_long inaddr;
		u_char enaddr[6];
	} goo;

	/* get and open arping device */

	if(ap->arp_op == ntohs(ARPOP_REPLY)){
		goo.inaddr = spa;
		bcopy(ap->arp_sha, goo.enaddr, 6);
		if(ioctl(ipfd, IPIORESOLVE, &goo) < 0)
			perror("IPIORESOLVE");
		return;
	}
}
