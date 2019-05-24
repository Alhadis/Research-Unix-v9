#define	FIO_IMP
#include	"fio.h"
#include	<libc.h>

Fflush(fd)
{
	register Fbuffer *f;
	register COUNT n;

	FIOSET(f, fd);
	f->end = 0;		/* mark as writing */
	if(f->next != f->buf){
		n = f->next - f->buf;
		if(write(fd, f->buf, n) != n)
			return(1);
		f->next = f->buf;
	}
	return(0);
}
