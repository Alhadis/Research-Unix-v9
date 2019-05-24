/*	sys.c	4.4	81/03/22	*/

#include <sys/param.h>
#include <sys/ino.h>
#include <sys/inode.h>
#include <sys/filsys.h>
#include <sys/dir.h>
#include "saio.h"

ino_t	dlook();

static
openi(n,io)
register struct iob *io;
{
	register struct dinode *dp;

	io->i_offset = 0;
	io->i_bn = fsbtodb(io->i_blksz, itod(io->i_blksz, n));
	io->i_cc = BSIZE(io->i_blksz);
	io->i_ma = io->i_buf;
	devread(io);

	dp = (struct dinode *)io->i_buf;
	dp = &dp[itoo(io->i_blksz, n)];
	io->i_ino.i_number = n;
	io->i_ino.i_mode = dp->di_mode;
	io->i_ino.i_size = dp->di_size;
	l3tol((char *)io->i_ino.i_un.i_addr, (char *)dp->di_addr, NADDR);
}

static
find(path, file)
register char *path;
struct iob *file;
{
	register char *q;
	char c;
	int n;

	if (path==NULL || *path=='\0') {
		printf("null path\n");
		return(0);
	}

	openi((ino_t) ROOTINO, file);
	while (*path) {
		while (*path == '/')
			path++;
		q = path;
		while(*q != '/' && *q != '\0')
			q++;
		c = *q;
		*q = '\0';

		if ((n=dlook(path, file))!=0) {
			if (c=='\0')
				break;
			openi(n, file);
			*q = c;
			path = q;
			continue;
		} else {
			printf("%s not found\n",path);
			return(0);
		}
	}
	return(n);
}

static daddr_t
sbmap(io, bn)
register struct iob *io;
daddr_t bn;
{
	register i;
	register struct inode *ip;
	int j, sh;
	daddr_t nb, *bap;
	int ibn = bn;

	ip = &io->i_ino;
	if(bn < 0) {
		printf("bn negative\n");
		return((daddr_t)0);
	}

	/*
	 * blocks 0..NADDR-4 are direct blocks
	 */
	if(bn < NADDR-3) {
		i = bn;
		nb = ip->i_un.i_addr[i];
		return(nb);
	}

	/*
	 * addresses NADDR-3, NADDR-2, and NADDR-1
	 * have single, double, triple indirect blocks.
	 * the first step is to determine
	 * how many levels of indirection.
	 */
	sh = 0;
	nb = 1;
	bn -= NADDR-3;
	for(j=3; j>0; j--) {
		sh += NSHIFT(io->i_blksz);
		nb <<= NSHIFT(io->i_blksz);
		if(bn < nb)
			break;
		bn -= nb;
	}
	if(j == 0) {
		printf("bn ovf %D\n",bn);
		return((daddr_t)0);
	}

	/*
	 * fetch the address from the inode
	 */
	nb = ip->i_un.i_addr[NADDR-j];
	if(nb == 0) {
		printf("bn void %D\n",bn);
		return((daddr_t)0);
	}

	/*
	 * fetch through the indirect blocks
	 */
	for(; j<=3; j++) {
		if (blknos[j] != nb) {
			io->i_bn = fsbtodb(io->i_blksz, nb);
			io->i_ma = b[j];
			io->i_cc = BSIZE(io->i_blksz);
			devread(io);
			bap = (daddr_t *)b[j];
			blknos[j] = nb;
		}
		bap = (daddr_t *)b[j];
		sh -= NSHIFT(io->i_blksz);
		i = (bn>>sh) & NMASK(io->i_blksz);
		nb = bap[i];
		if(nb == 0) {
			printf("bn void %D\n",bn);
			return((daddr_t)0);
		}
	}
	return(nb);
}

static ino_t
dlook(s, io)
char *s;
register struct iob *io;
{
	register struct direct *dp;
	register struct inode *ip;
	daddr_t bn;
	int n,dc;

	if (s==NULL || *s=='\0')
		return(0);
	ip = &io->i_ino;
	if ((ip->i_mode&IFMT)!=IFDIR) {
		printf("not a directory\n");
		return(0);
	}

	n = ip->i_size/sizeof(struct direct);

	if (n==0) {
		printf("zero length directory\n");
		return(0);
	}

	dc = BSIZE(io->i_blksz);
	bn = (daddr_t)0;
	while(n--) {
		if (++dc >= BSIZE(io->i_blksz)/sizeof(struct direct)) {
			io->i_bn = fsbtodb(io->i_blksz, sbmap(io, bn++));
			io->i_ma = io->i_buf;
			io->i_cc = BSIZE(io->i_blksz);
			devread(io);
			dp = (struct direct *)io->i_buf;
			dc = 0;
		}

		if (match(s, dp->d_name))
			return(dp->d_ino);
		dp++;
	}
	return(0);
}

static
match(s1,s2)
register char *s1,*s2;
{
	register cc;

	cc = DIRSIZ;
	while (cc--) {
		if (*s1 != *s2)
			return(0);
		if (*s1++ && *s2++)
			continue; else
			return(1);
	}
	return(1);
}

lseek(fdesc, addr, ptr)
int	fdesc;
off_t	addr;
int	ptr;
{
	register struct iob *io;

	if (ptr != 0) {
		printf("Seek not from beginning of file\n");
		return(-1);
	}
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES || ((io = &iob[fdesc])->i_flgs&F_ALLOC) == 0)
		return(-1);
	io->i_si.si_flgs &= ~F_EOF;
	io->i_offset = addr;
	io->i_bn = fsbtodb(io->i_blksz, addr/BSIZE(io->i_blksz));
	io->i_cc = 0;
	return(0);
}

getc(fdesc)
int	fdesc;
{
	register struct iob *io;
	register char *p;
	register  c;
	int off;


	if (fdesc >= 0 && fdesc <= 2)
		return(getchar());
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES || ((io = &iob[fdesc])->i_flgs&F_ALLOC) == 0)
		return(-1);
	p = io->i_ma;
	if (io->i_cc <= 0) {
		io->i_bn = fsbtodb(io->i_blksz,
			io->i_offset/(off_t)BSIZE(io->i_blksz));
		if (io->i_flgs&F_FILE)
			io->i_bn = fsbtodb(io->i_blksz,
				sbmap(io, dbtofsb(io->i_blksz, io->i_bn)));
		io->i_ma = io->i_buf;
		io->i_cc = BSIZE(io->i_blksz);
		devread(io);
		if (io->i_flgs&F_FILE) {
			off = io->i_offset % (off_t)BSIZE(io->i_blksz);
			if (io->i_offset+(BSIZE(io->i_blksz)-off) >= io->i_ino.i_size)
				io->i_cc = io->i_ino.i_size - io->i_offset + off;
			io->i_cc -= off;
			if (io->i_cc <= 0)
				return(-1);
		} else
			off = 0;
		p = &io->i_buf[off];
	}
	io->i_cc--;
	io->i_offset++;
	c = (unsigned)*p++;
	io->i_ma = p;
	return(c);
}


read(fdesc, buf, count)
int	fdesc;
char	*buf;
int	count;
{
	register i;
	register struct iob *file;

	if (fdesc >= 0 && fdesc <= 2) {
		i = count;
		do {
			*buf = getchar();
		} while (--i && *buf++ != '\n');
		return(count - i);
	}
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES || ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0)
		return(-1);
	if ((file->i_flgs&F_READ) == 0)
		return(-1);
	if ((file->i_flgs&F_FILE) == 0) {
		file->i_cc = count;
		file->i_ma = buf;
		i = devread(file);
		file->i_bn += CLSIZE;
		return(i);
	}
	else {
		if (file->i_offset+count > file->i_ino.i_size)
			count = file->i_ino.i_size - file->i_offset;
		if ((i = count) <= 0)
			return(0);
		do {
			*buf++ = getc(fdesc+3);
		} while (--i);
		return(count);
	}
}

write(fdesc, buf, count)
int	fdesc;
char	*buf;
int	count;
{
	register i;
	register struct iob *file;

	if (fdesc >= 0 && fdesc <= 2) {
		i = count;
		while (i--)
			putchar(*buf++);
		return(count);
	}
	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES || ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0)
	if ((file->i_flgs&F_WRITE) == 0)
		return(-1);
	file->i_cc = count;
	file->i_ma = buf;
	i = devwrite(file);
	file->i_bn += CLSIZE;
	return(i);
}

/*
 * Open a file on a physical device, or open the device itself.
 *
 * The device is identified by a struct bootparam which gives pointers
 * to the struct boottab, containing the device open, strategy and close
 * routines; and the device parameters such as controller #, unit #, etc.
 *
 * If *str == 0, the device is opened, else the named file in the file
 * system on the device is opened.
 */
int
physopen(bp, str, how)
register struct bootparam *bp;
char	*str;
int	how;
{
	int fdesc;
	register struct iob *file;

	fdesc = getiob();		/* Allocate an IOB */
	file = &iob[fdesc];

	file->i_boottab = bp->bp_boottab; /* Record pointer to boot table */
	file->i_ctlr = bp->bp_ctlr;
	file->i_unit = bp->bp_unit;
	file->i_boff = bp->bp_part;
	file->i_ino.i_dev = 0;	/* Call it device 0 for chuckles */

	return(openfile(fdesc, file, str, how));
}

int	openfirst = 1;

/*
 * Allocate an IOB for a newly opened file.
 */
int
getiob()
{
	register int fdesc;

	if (openfirst) {
		openfirst = 0;
		for (fdesc = 0; fdesc < NFILES; fdesc++)
			iob[fdesc].i_flgs = 0;
	}

	for (fdesc = 0; fdesc < NFILES; fdesc++)
		if (iob[fdesc].i_flgs == 0)
			goto gotfile;
	_stop("No more file slots");
gotfile:
	(&iob[fdesc])->i_flgs |= F_ALLOC;
	return fdesc;
}

/* Hex string scanner for open() */
static char *
openhex(p, ip)
	register char *p;
	int *ip;
{
	register int ac = 0;

	while (*p) {
		if (*p >= '0' && *p <= '9')
			ac = (ac<<4) + (*p - '0');
		else if (*p >= 'a' && *p <= 'f')
			ac = (ac<<4) + (*p - 'a' + 10);
		else if (*p >= 'A' && *p <= 'F')
			ac = (ac<<4) + (*p - 'A' + 10);
		else
			break;
		p++;
	}
	if (*p == ',')
		p++;
	*ip = ac;
	return (p);
}

/*
 * Open a device or file-in-file-system-on-device.
 */
int
open(str, how)
char	*str;
int	how;
{
	register char *cp;
	register struct iob *file;
	register struct boottab *dp;
	register struct boottab **tablep;
	int	fdesc;
	long	atol();

	extern struct boottab *(devsw[]);

	fdesc = getiob();		/* Allocate an IOB */
	file = &iob[fdesc];

	for (cp = str; *cp && *cp != '('; cp++)
			;
	if (*cp++ != '(') {
		file->i_flgs = 0;
		goto badsyntax;
	}
	for (tablep = devsw; 0 != (dp = *tablep); tablep++) {
		if (str[0] == dp->b_dev[0] && str[1] == dp->b_dev[1])
			goto gotdev;
	}
	printf("Unknown device: %c%c; known devices:\n", str[0], str[1]);
	for (tablep = devsw; 0 != (dp = *tablep); tablep++)
		printf("  %s\n", dp->b_desc);
	file->i_flgs = 0;
	return(-1);
gotdev:
	file->i_boottab = dp;		/* Record pointer to boot table */
	file->i_ino.i_dev = tablep-devsw;
	cp = openhex(cp, &file->i_ctlr);
	cp = openhex(cp, &file->i_unit);
	cp = openhex(cp, (int *)&file->i_boff);
	if (*cp == '\0') goto doit;
	if (*cp != ')') {
badsyntax:
		printf("%s: bad syntax, try dev(ctlr,unit,part)name\n", str);
		return -1;
	}
	do
		cp++;
	while (*cp == ' ');

doit:
	return(openfile(fdesc, file, cp, how));
}


/*
 * File processing for open call
 */
openfile(fdesc, file, cp, how)
int		fdesc;
struct iob	*file;
char 		*cp;
int		how;
{
	int	i;

	if (devopen(file)) {
		file->i_flgs = 0;
		return(-1);	/* if devopen fails, open fails */
	}

	if (*cp == '\0') {	/* Opening a device */
		file->i_flgs |= how+1;
		file->i_cc = 0;
		file->i_offset = 0;
		return(fdesc+3);
	}
	if ((i = find(cp, file)) == 0) {
		file->i_flgs = 0;
		return(-1);
	}
	if (how != 0) {
		printf("Can't write files yet.. Sorry\n");
		file->i_flgs = 0;
		return(-1);
	}
	openi(i, file);
	file->i_offset = 0;
	file->i_cc = 0;
	file->i_flgs |= F_FILE | (how+1);
	return(fdesc+3);
}


close(fdesc)
int	fdesc;
{
	struct iob *file;

	fdesc -= 3;
	if (fdesc < 0 || fdesc >= NFILES || ((file = &iob[fdesc])->i_flgs&F_ALLOC) == 0)
		return(-1);
	devclose(file);
	file->i_flgs = 0;
	return(0);
}

exit()
{
	_stop("Exit called");
}

_stop(s)
char	*s;
{
	int i;

	for (i = 0; i < NFILES; i++)
		if (iob[i].i_flgs != 0)
			close(i);
	printf("%s\n", s);
	_exit();
}
