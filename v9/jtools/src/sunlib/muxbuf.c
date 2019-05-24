#include "jerq.h"
#ifdef SUNTOOLS
static char *buffile = "/tmp/muxbuf";
#include <sys/file.h>
#endif SUNTOOLS

typedef struct String{
	char *s;	/* pointer to string */
	short n;	/* number used, no terminal null */
	short size;	/* size of allocated area */
} String;

getmuxbuf (pmb)
String *pmb;
{
	char *ans;
	int n;

#ifdef X11
	ans = XFetchBytes(dpy, &n);
#endif X11
#ifdef SUNTOOLS
#define BSIZE 4096
	char buffer[BSIZE];
	int fd;

	fd = open(buffile, O_RDONLY);
	if (fd < 0)
		return;
	n = read(fd, buffer, BSIZE);
	close(fd);
	if (n < 0)
		return;
	ans = buffer;
#endif SUNTOOLS
	if (pmb->size < (n+1)) {
		pmb->size = n+1;
		gcalloc(pmb->size, &(pmb->s));
	}
	pmb->n = n;
	strncpy(pmb->s, ans, n+1);
#ifdef X11
	free(ans);
#endif X11
}

setmuxbuf(pmb)
String *pmb;
{
#ifdef X11
	XStoreBytes(dpy, pmb->s, pmb->n);
#endif X11
#ifdef SUNTOOLS
	int fd = open(buffile, O_WRONLY|O_CREAT|O_TRUNC, 0666);
	if (fd < 0)
		return;
	write(fd, pmb->s, pmb->n);
	close(fd);
#endif SUNTOOLS
}
