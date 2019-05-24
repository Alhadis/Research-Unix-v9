#include "mgr.h"
#include <pwd.h>
#include <sys/ioctl.h>
#include "defs.h"

/*
 *  action routines to pushline disciplines
 */
mesgld(rp, ap)
	Request *rp;
	Action *ap;
{
	extern int rmesg_ld;

	USE(ap);
	return ioctl(rp->i->cfd, FIOPUSHLD, &rmesg_ld);
}

ttyld(rp, ap)
	Request *rp;
	Action *ap;
{
	extern int tty_ld;

	USE(ap);
	return ioctl(rp->i->cfd, FIOPUSHLD, &tty_ld);
}

