/*
 * cp oldfile newfile
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#define	BLOCKSIZE	BUFSIZE
struct	stat	stbuf1, stbuf2;
char	iobuf[BLOCKSIZE];
char	zero[BLOCKSIZE];	/* keep as zero */
int	zflag = 0;
#define	wrcp()	write(2, "cp: ", 4)

main(argc, argv)
char *argv[];
{
	register i, r;

	if (argc < 3) 
		goto usage;
	i = 1;
	if(strcmp(argv[i], "-z") == 0)
		zflag = 1, i++;
	if ((argc-i) > 2) {
		if (stat(argv[argc-1], &stbuf2) < 0)
			goto usage;
		if ((stbuf2.st_mode&S_IFMT) != S_IFDIR) 
			goto usage;
	}
	r = 0;
	for(; i<argc-1;i++)
		r |= copy(argv[i], argv[argc-1]);
	exit(r);
usage:
	fprintf(stderr, "Usage: cp: [-z] f1 f2; or cp [-z] f1 ... fn d2\n");
	exit(1);
}

copy(from, to)
char *from, *to;
{
	int fold, fnew, n;
	register char *p1, *p2, *bp;
	int mode;
	int bsize, zsize;
	long lseek();

	if ((fold = open(from, 0)) < 0) {
		wrcp();
		perror(from);
		return(1);
	}
	fstat(fold, &stbuf1);
	mode = stbuf1.st_mode;
	/* is source a directory? */
	if ((mode&S_IFMT) == S_IFDIR) {
		fprintf(stderr, "cp: %s is a directory\n", from);
		close(fold);
		return(1);
	}
	/* is target a directory? */
	if (stat(to, &stbuf2) >=0 &&
	   (stbuf2.st_mode&S_IFMT) == S_IFDIR) {
		p1 = from;
		p2 = to;
		bp = iobuf;
		while(*bp++ = *p2++)
			;
		bp[-1] = '/';
		p2 = bp;
		while(*bp = *p1++)
			if (*bp++ == '/')
				bp = p2;
		to = iobuf;
	}
	if (stat(to, &stbuf2) >= 0) {
		if (stbuf1.st_dev == stbuf2.st_dev &&
		   stbuf1.st_ino == stbuf2.st_ino) {
			fprintf(stderr, "cp: cannot copy file to itself\n");
			close(fold);
			return(1);
		}
	}
	bsize = BSIZE(stbuf2.st_dev);
	if (bsize > BLOCKSIZE) {
		fprintf(stderr, "cp: BSIZE(%s) > %d\n", to, BLOCKSIZE);
		return 1;
		}
	if ((fnew = creat(to, mode)) < 0) {
		wrcp();
		perror(to);
		close(fold);
		return(1);
	}
	zsize = 0;
	while(n = read(fold, iobuf, bsize)) {
		if (n < 0) {
			wrcp();
			perror(from);
			close(fold);
			close(fnew);
			return(1);
		} else
			if(zflag && (n == bsize) && (memcmp(iobuf, zero, bsize) == 0)){
				if (lseek(fnew, (long)bsize, 1) < 0) {
					wrcp();
					perror(to);
					close(fold);
					close(fnew);
					return(1);
				}
				zsize = bsize;
			} else {
				if (write(fnew, iobuf, n) != n) {
					wrcp();
					perror(to);
					close(fold);
					close(fnew);
					return(1);
				}
				zsize = 0;
			}
	}

	if (zsize) {
		if (lseek(fnew, (long)-zsize, 1) < 0
		    || write(fnew,zero,zsize) != zsize) {
			wrcp();
			perror(to);
			close(fold);
			close(fnew);
			return(1);
		}
	}

	close(fold);
	close(fnew);
	return(0);
}
