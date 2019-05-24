#define	FIO_IMP
#include	"fio.h"
#include	<libc.h>

Fwrite(fd, addr, nbytes)
	register char *addr;
	register long nbytes;
{
	register Fbuffer *f;
	register COUNT n;
	long nnbytes = nbytes;

	FIOSET(f, fd);
	f->end = 0;		/* mark as writing */
	n = &f->buf[FIOBSIZE] - f->next;
	if(nbytes < n) n = nbytes;
	memcpy(f->next, addr, n);
	f->next += n;
	nbytes -= n;
	addr += n;
	if(nbytes){
		F_flush(f, fd);
		while(nbytes >= FIOBSIZE){
			if(write(fd, addr, (COUNT)FIOBSIZE) != FIOBSIZE)
				return(-1L);
			addr += FIOBSIZE;
			nbytes -= FIOBSIZE;
		}
		memcpy(f->next = f->buf, addr, (COUNT)(n = nbytes));
		f->next += n;
	}
	return(nnbytes);
}
