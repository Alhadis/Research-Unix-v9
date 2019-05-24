/* Copyright (c) 1982 Regents of the University of California */
/* and modified by pjw in 1986 */

/*
 * this must be a power of 2 and a multiple of all the ones in the system
 */
#define DIRBLKSIZ 512

/*
 * This limits the directory name length. Its main constraint
 * is that it appears twice in the user structure. (u. area) in bsd systems
 */
#define MAXNAMLEN 255

struct	direct {
	unsigned long	d_ino;
	short	d_reclen;
	short	d_namlen;
	char	d_name[MAXNAMLEN + 1];
	/* typically shorter */
};

struct _dirdesc {
	int	dd_fd;
	long	dd_loc;
	long	dd_size;
	char	dd_buf[DIRBLKSIZ];
	char	dd_type;
};
/* directory types */
#define TUNK 0
#define TOLD 1
#define TCRAY		2
#define TBSDSWAP	3	/* bsd style on other-endian */
#define TBSD		4	/* bsd style */
#define TOLDSWAP	5	/* Old on other-endian, i.e. vax from sun */

/*
 * useful macros.
 */
#undef DIRSIZ
#define DIRSIZ(dp) \
    ((sizeof(struct direct) - MAXNAMLEN + (dp)->d_namlen + sizeof(ino_t) - 1) &\
    ~(sizeof(ino_t) - 1))
typedef	struct _dirdesc DIR;
#ifndef	NULL
#define	NULL	0
#endif

/*
 * functions defined on directories
 */
extern DIR *opendir(char*);
extern direct *readdir(DIR*);
extern long telldir(DIR*);
extern void seekdir(DIR*, long);
#define rewinddir(dirp)	seekdir((dirp), 0)
extern void closedir(DIR*);
