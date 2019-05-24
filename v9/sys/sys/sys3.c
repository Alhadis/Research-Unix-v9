/*	sys3.c	4.9	81/03/08	*/

#include "../h/param.h"
#include "../h/systm.h"
/*#include "../h/reg.h"*/
#include "../h/buf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/inode.h"
#include "../h/file.h"
#include "../h/conf.h"
#include "../h/stat.h"

/*
 * the fstat system call.
 */
fstat()
{
	register struct file *fp;
	register struct a {
		int	fdes;
		struct stat *sb;
	} *uap;
	register struct inode *ip;

	uap = (struct a *)u.u_ap;
	fp = getf(uap->fdes);
	if(fp == NULL) {
		u.u_error = EBADF;
		return;
	}
	ip = fp->f_inode;
	plock(ip);
	iupdat(ip, &time, &time, 0);
	(*fstypsw[ip->i_fstyp].t_stat)(ip, uap->sb);
	prele(ip);
}

/*
 * the stat system call.  This version follows links.
 */
stat()
{
	register struct inode *ip;
	register struct a {
		char	*fname;
		struct stat *sb;
	} *uap;

	uap = (struct a *)u.u_ap;
	ip = namei(uchar, (struct argnamei *)NULL, 1);
	if(ip == NULL)
		return;
	iupdat(ip, &time, &time, 0);
	(*fstypsw[ip->i_fstyp].t_stat)(ip, uap->sb);
	iput(ip);
}

/*
 * the lstat system call.  This version does not follow links.
 */
lstat()
{
	register struct inode *ip;
	register struct a {
		char	*fname;
		struct stat *sb;
	} *uap;

	uap = (struct a *)u.u_ap;
	ip = namei(uchar, (struct argnamei *)NULL, 0);
	if(ip == NULL)
		return;
	iupdat(ip, &time, &time, 0);
	(*fstypsw[ip->i_fstyp].t_stat)(ip, uap->sb);
	iput(ip);
}

/*
 *  readlink -- return target name of a symbolic link
 */
readlink()
{
	register struct inode *ip;
	register struct a {
		char	*name;
		char	*buf;
		int	count;
	} *uap;

	ip = namei(uchar, (struct argnamei *)NULL, 0);
	if (ip == NULL)
		return;
	uap = (struct a *)u.u_ap;
	if ((ip->i_mode&IFMT) != IFLNK) {
		u.u_error = ENXIO;
		goto out;
	}
	u.u_offset = 0;
	u.u_base = uap->buf;
	u.u_count = uap->count;
	u.u_segflg = 0;
	readi(ip);
out:
	iput(ip);
	u.u_r.r_val1 = uap->count - u.u_count;
}

/*
 * symlink -- make a symbolic link
 */
symlink()
{
	register struct a {
		char	*target;
		char	*linkname;
	} *uap;
	register struct inode *ip;
	register char *tp;
	register c, nc;
	struct argnamei nmarg;

	uap = (struct a *)u.u_ap;
	tp = uap->target;
	nc = 0;
	while (c = fubyte(tp)) {
		if (c < 0) {
			u.u_error = EFAULT;
			return;
		}
		tp++;
		nc++;
	}
	u.u_dirp = uap->linkname;
	nmarg = nilargnamei;
	nmarg.flag = NI_NXCREAT;
	nmarg.mode = IFLNK|0777;
	ip = namei(uchar, &nmarg, 0);
	if (ip == NULL)
		return;
	u.u_base = uap->target;
	u.u_count = nc;
	u.u_offset = 0;
	u.u_segflg = 0;
	writei(ip);
	iput(ip);
}

/*
 * the dup system call.
 */
dup()
{
	register struct file *fp;
	register struct a {
		int	fdes;
		int	fdes2;
	} *uap;
	register i, m;

	uap = (struct a *)u.u_ap;
	m = uap->fdes & ~077;
	uap->fdes &= 077;
	fp = getf(uap->fdes);
	if(fp == NULL) {
		u.u_error = EBADF;
		return;
	}
	if ((m&0100) == 0) {
		if ((i = ufalloc()) < 0)
			return;
	} else {
		i = uap->fdes2;
		if (i<0 || i>=NOFILE) {
			u.u_error = EBADF;
			return;
		}
		u.u_r.r_val1 = i;
	}
	if (i!=uap->fdes) {
		if (u.u_ofile[i]!=NULL) {
			if (u.u_pofile[i] & MMAPPED)
				munmapfd(i);
			closef(u.u_ofile[i]);
		}
		u.u_ofile[i] = fp;
		u.u_pofile[i] = 0;
		fp->f_count++;
	}
}
