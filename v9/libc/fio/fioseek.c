#define	FIO_IMP
#include	"fio.h"
#include	<libc.h>

long
Fseek(fd, n, cmd)
	long n;
{
	register Fbuffer *f;
	register long dest, k;

	FIOSET(f, fd);
	dest = SEEK(fd, n, cmd);
	if(dest < 0)
		return(dest);
	k = f->end-f->lnext;
	if((dest >= f->offset) || (dest < f->offset-k)){
		FIORESET(f);
		f->offset = dest;
	} else {
		f->next = f->lnext + (dest-(f->offset-k));
		SEEK(fd, f->offset, 0);
	}
	return(dest);
}
