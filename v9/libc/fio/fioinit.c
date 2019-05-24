#define	FIO_IMP
#include	"fio.h"
#include	<libc.h>

void
Finit(fd, buf)
	char *buf;
{
	register Fbuffer *f;
	static setfioexit = 1;
	extern Fexit();

	if(setfioexit){
		setfioexit = 0;
		onexit(Fexit);
	}
	fd &= 0x7f;
	if(buf)
		Ffb[fd] = (Fbuffer *)buf;
	else if(!Ffb[fd])
		Ffb[fd] = (Fbuffer *)FIOMALLOC((COUNT)sizeof(Fbuffer));
	f = Ffb[fd];
	FIORESET(f);
	f->offset = SEEK(fd, 0L, 1);
}

#include	<sys/param.h>

Fbuffer *Ffb[NOFILE];

Fexit()
{
	register n;

	for(n = 0; n < NOFILE; n++)
		if(Ffb[n] && (Ffb[n]->end == 0))
			F_flush(Ffb[n], n);
}

F_flush(f, fd)
	register Fbuffer *f;
{
	register COUNT n;

	f->end = 0;		/* mark as writing */
	if(f->next != f->buf){
		n = f->next - f->buf;
		if(write(fd, f->buf, n) != n)
			return(1);
		f->next = f->buf;
	}
	return(0);
}
