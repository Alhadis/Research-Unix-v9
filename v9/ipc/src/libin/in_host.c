#include <sys/types.h>
#include <sys/stat.h>
#include <sys/inet/in.h>
#include <stdio.h>
#include <ctype.h>
#include "config.h"

struct namefile {
	char *name;
	char *data;
	long time;
};

static struct namefile files[] = {
	{ "/usr/ipc/lib/inaddr.local", 0, 0 },
	{ "/usr/ipc/lib/inaddr", 0, 0 },
	{ "/usr/inet/lib/hosts", 0, 0 },
	{ "/usr/inet/lib/networks", 0, 0 },
};
#define NFIL (sizeof(files)/sizeof(struct namefile))

struct trbuf {
	struct trbuf *next;
	in_addr addr;
	char *name;
};

#define TLBSIZE 1023
static struct trbuf tlb[TLBSIZE];

/* imported */
extern char *in_getw();
extern char *malloc();

/* exported */
int in_host_dostat=1;

static int
getfile(i)
	int i;
{
	int fd;
	struct stat sbuf;
	unsigned int x;

	if (in_host_dostat)
		stat(files[i].name, &sbuf);
	else
		sbuf.st_mtime = files[i].time;
	if (files[i].data == 0 || sbuf.st_mtime!=files[i].time) {
		if (!in_host_dostat)
			stat(files[i].name, &sbuf);
		fd = open(files[i].name, 0);
		if (fd<0)
			return -1;
		if (files[i].data!=0)
			free(files[i].data);
		x = sbuf.st_size;
		files[i].data = malloc(x+2);
		if (files[i].data==0) {
			close(fd);
			return -1;
		}
		if (read(fd, files[i].data, x)!=x){
			close(fd);
			return -1;
		}
		close(fd);
		files[i].data[sbuf.st_size] = '\n';
		files[i].data[sbuf.st_size+1] = 0;
		files[i].time = sbuf.st_mtime;
		for (i=0; i<TLBSIZE; i++)
			tlb[i].addr = 0;
	}
	return 0;
}

#define SKIPWHITE while(*p==' ' || *p=='\t') p++
#define GETBLACK	xp = b;\
			while(*p!=' ' && *p!='\t' && *p!='\n' && *p!='\0')\
				*xp++ = *p++;\
			*xp = '\0'

char *
in_host(addr)
in_addr addr;
{
	register char *p, *xp, *op;
	unsigned char *yp;
	static char b[32];
	int f;
	in_addr x, in_netof(), in_aton();

	if (addr==0)
		return "*";
	for(f=0; f<NFIL; f++){
		if (getfile(f) < 0) {
			continue;
		} else if (addr == tlb[addr%TLBSIZE].addr) {
			p = tlb[addr%TLBSIZE].name;
			GETBLACK;
			return(b);
		} else for (p = files[f].data; *p; p++){
			/* get number */
			SKIPWHITE;
			GETBLACK;
			x = in_aton(b);
			SKIPWHITE;
			if (x != 0 && *p != '\n') {
				/* get name */
				op = p;
				GETBLACK;
	
				/* see if this is the one */
				if (x==addr){
					int i;

					i = x%TLBSIZE;
					tlb[i].addr = x;
					tlb[i].name = op;
					return b;
				}
			}
			/* drop rest of line */
			while(*p!='\n'&&*p!='\0') p++;
		}
	}
	yp = (unsigned char *) &addr;
	sprintf(b, "%d.%d.%d.%d", yp[3], yp[2], yp[1], yp[0]);
	return(b);
}
