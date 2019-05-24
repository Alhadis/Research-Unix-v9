#define	FIO_IMP
#include	"fio.h"

void
Ftie(ifd, ofd)
{
	register Fbuffer *f = Ffb[ifd];

	FIOSET(f, ifd);
	if(ofd > 0x7f)
		ofd = -1;
	f->oflush = ofd;
}
