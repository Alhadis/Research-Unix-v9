/*
 * code related to mounting and unmounting filesystems
 */

/*
 * bugs:
 * temporary extra antique mount points
 * the mount table is now anachronistic.
 * it is still maintained
 * only to help the virtual memory code.
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mount.h"
#include "../h/ino.h"
#include "../h/reg.h"
#include "../h/buf.h"
#include "../h/filsys.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/inode.h"
#include "../h/file.h"
#include "../h/stream.h"
#include "../h/conf.h"
#include "../h/stat.h"

/*
 * sys fmount
 * call the filesystem-specific mount routine with
 *	the inode of the device to be mounted
 *	the inode of the mount point, still locked
 *	the flag argument
 */

fmount()
{
	register struct a {
		int fstype;
		int fd;
		char *name;
		int flag;
	} *uap = (struct a *)u.u_ap;
	struct file *fp;
	struct inode *ip;

	if (uap->fstype < 0 || uap->fstype >= nfstyp) {
		u.u_error = EINVAL;
		return;
	}
	if ((fp = getf(uap->fd)) == NULL) {
		u.u_error = EBADF;
		return;
	}
	u.u_dirp = (caddr_t)uap->name;
	if ((ip = namei(uchar, (struct argnamei *)NULL, 1)) == NULL)
		return;
	(*fstypsw[uap->fstype].t_mount)(fp->f_inode, ip, uap->flag, 1, uap->fstype);
	iput(ip);
}

/*
 * sys funmount
 * call the fs-particular unmount routine with
 * the inode of the mount point
 */

funmount()
{
	struct a {
		char *name;
	} *uap = (struct a *)u.u_ap;
	struct inode *ip, *mip;

	u.u_dirp = (caddr_t)uap->name;
	if ((ip = namei(uchar, (struct argnamei *)NULL, 1)) == NULL)
		return;
	if ((mip = ip->i_mpoint) == NULL) {
		iput(ip);
		u.u_error = EINVAL;
		return;
	}
	iput(ip);
	if (mip->i_mroot == NULL)
		panic("fsunmount");
	if (mip->i_mroot == rootdir) {
		u.u_error = EINVAL;
		return;
	}
	(*fstypsw[ip->i_fstyp].t_mount)((struct inode *)NULL, mip, 0, 0, ip->i_fstyp);
}

/* find the mount slot assigned a particular device */
struct mount *
findmount(fstyp, dev)
	int fstyp;
	dev_t dev;
{
	register struct mount *mp;

	for(mp = mount; mp < mount + NMOUNT; mp++)
		if(mp->m_flags&M_MOUNTED && mp->m_dev==dev && mp->m_fstyp==fstyp)
			return mp;
	return NULL;
}

/* get a free mount slot */
struct mount *
allocmount(fstyp, dev)
	int fstyp;
	dev_t dev;
{
	register struct mount *mp, *free;

	free = NULL;
	for(mp = mount; mp < mount + NMOUNT; mp++) {
		if(!mp->m_flags&M_MOUNTED) {
			if (free == NULL)
				free = mp;
		} else if(mp->m_dev == dev && mp->m_fstyp == fstyp)
			return NULL;	/* mounted twice */
	}
	if (free != NULL) {
		free->m_flags |= M_MOUNTED;
		free->m_dev = dev;
		free->m_fstyp = fstyp;
	}
	return free;
}

/* free up a mount structure */
freemount(mp)
	struct mount *mp;
{
	if (mp == NULL)
		panic("freemount");
	mp->m_flags &= ~M_MOUNTED;
}

/*
 * the old mount system call.
 */

sysmount()
{
	register struct inode *ip, *sip;
	register struct a {
		char	*fspec;
		char	*freg;
		int	ronly;
	} *uap;

	uap = (struct a *)u.u_ap;
	u.u_dirp = (caddr_t)uap->fspec;
	if ((sip = namei(uchar, (struct argnamei *)NULL, 1)) == NULL)
		return;
	u.u_dirp = (caddr_t)uap->freg;
	if ((ip = namei(uchar, (struct argnamei *)NULL, 1)) == NULL) {
		iput(sip);
		return;
	}
	(*fstypsw[0].t_mount)(sip, ip, uap->ronly, 1, 0);
	iput(sip);
	iput(ip);
}

/*
 * the old umount system call.
 */

sysumount()
{
	register struct inode *ip;
	register struct mount *mp;
	register struct a {
		char	*fspec;
	} *uap = (struct a *)u.u_ap;

	u.u_dirp = (caddr_t)uap->fspec;
	if ((ip = namei(uchar, (struct argnamei *)NULL, 1)) == NULL)
		return;
	if ((mp = findmount(0, (dev_t)ip->i_un.i_rdev)) == NULL) {
		iput(ip);
		u.u_error = ENODEV;
		return;
	}
	iput(ip);
	if ((ip = mp->m_inodp) == NULL)
		panic("umount");
	if (ip->i_mroot == rootdir) {
		u.u_error = EINVAL;
		return;
	}
	(*fstypsw[0].t_mount)((struct inode *)NULL, ip, 0, 0, 0);
}

/* gmount(fstyp, ...) */
/* glue until converted to new stuff */
gmount()
{
	struct inode *ip;
	struct inode *cip;
	register struct file *fp;
	struct inode pi;
	struct a {
		unsigned int fstyp;
		int gumby;	/* unqname or freg */
		int flag;
		int cfd;
		char *freg;
	} *uap = (struct a *)u.u_ap;

	switch (uap->fstyp) {
	case 1:
		if (uap->flag == 0) {	/* mount */
			if ((fp = getf(uap->cfd)) == NULL)
				return;
			u.u_dirp = (caddr_t)uap->freg;
			if ((ip = namei(uchar, (struct argnamei *)NULL, 1)) == NULL)
				return;
			(*fstypsw[1].t_mount)(fp->f_inode, ip, uap->gumby, 1, 1);
			iput(ip);
		}
		else {			/* unmount */
			pi.i_fstyp = uap->fstyp;
			pi.i_dev = uap->gumby;
			pi.i_un.i_cip = NULL;
			if ((ip = iget(&pi, pi.i_dev, ROOTINO)) == NULL) {
				u.u_error = EINVAL;	/* for setup */
				return;
			}
			if (ip->i_un.i_cip == NULL) {	/* got the fake */
				iput(ip);
				u.u_error = EINVAL;
				return;
			}
			cip = ip->i_mpoint;
			iput(ip);
			(*fstypsw[1].t_mount)((struct inode *)NULL, cip, uap->gumby, 0, 1);
		}
		break;

	case 2:
		u.u_dirp = (caddr_t)uap->gumby;
		if ((ip = namei(uchar, (struct argnamei *)NULL, 1)) == NULL)
			return;
		(*fstypsw[2].t_mount)((struct inode *)NULL, ip, 0, !uap->flag, 2);
		if (uap->flag == 0)
			iput(ip);
		break;

	case 3:
		if (uap->flag == 0) {
			if ((fp = getf(uap->cfd)) == NULL)
				return;
			cip = fp->f_inode;
		}
		u.u_dirp = (caddr_t)uap->freg;
		if ((ip = namei(uchar, (struct argnamei *)NULL, 1)) == NULL)
			return;
		(*fstypsw[3].t_mount)(cip, ip, 0, !uap->flag, 3);
		iput(ip);
		break;

	default:
		u.u_error = EINVAL;
		break;
	}
}
