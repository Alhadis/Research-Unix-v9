/* Copyright (c) 1982 Regents of the University of California, who write
	lousy code */
/* and modified by pjw in 1986 */

#include <sys/types.h>
#include <sys/stat.h>
#include "ndir.h"

/*
 * open a directory.
 */
DIR *
opendir(name)
char *name;
{	int i;
	register DIR *dirp;
	register char *p;
	struct stat st;

	dirp = (DIR *)malloc(sizeof(DIR));
	if(!dirp)
		return(0);
	dirp->dd_fd = open(name, 0);
	if (dirp->dd_fd == -1) {
		free(dirp);
		return(0);
	}
	dirp->dd_loc = 0;
	i = read(dirp->dd_fd, dirp->dd_buf, DIRBLKSIZ);
	if(i <= 0) {
		close(dirp->dd_fd);
		free(dirp);
		return(0);
	}
	lseek(dirp->dd_fd, 0L, 0);
/* the cray and the 68000 are big-endians, vax is not
  v9:	??.0000000000000??..000000000000|||||||||
  cray:	????????.00000000000000000000000????????..0000000000000000000000|||||||
  68k:	????0X01.000????0X02..00||||||||	X=12
  vaxn:	????X010.000????X020..00|||||||
	/* shortest cray sys v dir is 64 bytes, shortest v8 is 32, and shortest
	 * sun 4.2(68k) is 24 (except it's supposed to be a whole block long)
	 */
	if(i < 24)	/* mysteriously short directory */
		goto old;
	p = dirp->dd_buf;
	if(p[8] != '.') {
old:
		if (fstat(dirp->dd_fd, &st) < 0) {
			close(dirp->dd_fd);
			free(dirp);
			return(0);
		}
		if (st.st_ino == *(ino_t *)dirp->dd_buf)
			dirp->dd_type = TOLD;
		else
			dirp->dd_type = TOLDSWAP;
		return(dirp);
	}
	if(p[20] == '.' && p[21] == '.') {	/* bsd-like or bogus */
		if(*(short *)&p[4] == 0x000c) {
			dirp->dd_type = TBSD;
			return(dirp);
		}
		if(*(short *)&p[4] == 0x0c00) {
			dirp->dd_type = TBSDSWAP;
			return(dirp);
		}
		dirp->dd_type = TUNK;
		return(dirp);
	}
	if(i >= 32 && p[9] == 0 && p[40] == '.' && p[41] == '.' && p[42] == 0) {
		dirp->dd_type = TCRAY;
		return(dirp);
	}
	dirp->dd_type = TUNK;
	return(dirp);
}
