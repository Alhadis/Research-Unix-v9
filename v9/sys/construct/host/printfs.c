#include "/usr/include/stdio.h"
#include <sys/param.h>
#include <sys/ino.h>
#include <sys/inode.h>
#include <sys/filsys.h>
#include <sys/fblk.h>
#include <sys/dir.h>

int	fsi;
main(argc,argv)
int	argc;
char	*argv[];
{
	char	buf[BSIZE(0)];
	struct filsys *f;
	int i;

	if (argc != 2)  {
		printf("usage: printfs dev\n");
		exit(1);
	}
	fsi = open(argv[1], 0);
	rdfs(1, buf);
	f = (struct filsys *)buf;

	printf("Inode blocks = %d\n", f->s_isize);	
	printf("Total blocks = %d\n", f->s_fsize);	
	printf("Inodes in s_ninode = %d\n", f->s_ninode);
	for(i = 0; i < NICINOD; i++)
		if(f->s_inode[i] != 0)
			printf("%d is %d\n", i, f->s_inode[i]);

	printf("Flock = %d\n", f->s_flock);	
	printf("ilock = %d\n", f->s_ilock);	
	printf("fmod = %d\n", f->s_fmod);	
	printf("ronly = %d\n", f->s_ronly);	
	printf("Update time = %d\n", f->s_time);	
	printf("Free blocks = %d\n", f->s_tfree);	
	printf("Free inodes = %d\n", f->s_tinode);	
	printf("M = %d\n", f->s_m);	
	printf("N = %d\n", f->s_n);	
	printf("Block size = %d\n", BSIZE(0));
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
