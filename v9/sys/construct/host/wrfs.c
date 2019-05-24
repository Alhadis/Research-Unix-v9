#include "/usr/include/stdio.h"
#include <sys/param.h>
#include <sys/ino.h>
#include <sys/inode.h>
#include <sys/filsys.h>
#include <sys/fblk.h>
#include <sys/dir.h>

int	fsi;
char	buf[BSIZE(0)];

main()
{
	short *s = (short *)buf;
	int i;

	fsi = open("/dev/rsd1a", 2);
	for(i = 0; i < sizeof(buf)/2; i++)
		*s++ = i;	
	wrfs(1, buf);
}

rdfs(bno, bf)
daddr_t bno;
char *bf;
{
	int n;

	lseek(fsi, bno*BSIZE(0), 0);
	n = read(fsi, bf, BSIZE(0));
	if(n != BSIZE(0)) {
		printf("read error: %ld\n", bno);
		exit(1);
	}
}

wrfs(bno, bf)
daddr_t bno;
char *bf;
{
	int n;

	lseek(fsi, bno*BSIZE(0), 0);
	n = write(fsi, bf, BSIZE(0));
	if(n != BSIZE(0)) {
		printf("write error: %ld\n", bno);
		exit(1);
	}
}
