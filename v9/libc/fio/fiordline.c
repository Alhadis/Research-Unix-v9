#define	FIO_IMP
#include	"fio.h"
#include	<libc.h>

char *
Frdline(fd)
{
	register unsigned char *p;
	register Fbuffer *f = Ffb[fd];
	register n;
	register char *nextnl;

	FIOSET(f, fd);
	while((nextnl = memchr(f->next, '\n', (COUNT)(f->end-f->next))) == 0){
		if(f->next == f->buf){
			if(f->end >= &f->buf[FIOBSIZE])
				f->end = &f->buf[FIOBSIZE-64];	/* full buffer, no newline */
		} else {
			n = f->next-f->buf;
			memcpy(f->buf, f->next, (COUNT)(FIOBSIZE-n));
			f->next -= n;
			f->end -= n;
		}
		if(Ffillbuf(fd) <= 0)
			return((char *)0);
	}
	f->lnext = f->next;
	f->next = (unsigned char *)(nextnl+1);
	*nextnl = 0;
	f->rdlast = 1;
	return((char *)f->lnext);
}
