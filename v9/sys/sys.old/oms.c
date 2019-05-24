/*
 *	This file contains the routines which support mounted
 *	streams (file system type 3).
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/inode.h"
#include "../h/stream.h"
#include "../h/user.h"
#include "../h/mount.h"
#include "../h/file.h"
#include "../h/stat.h"
#include "../h/conf.h"

/*
 *  The following definitions apply to all comments:
 *	- `locked' means that an inode has had `plock' applied to it
 *	- `reserved' means that the inode's i_count has been incremented to
 *	  ensure that the inode will be freed during an operation.
 */

msmount(cip, dip, flag, mnt, fstyp)
struct inode *cip, *dip;
{
	/* only writer can mount/unmount */
	if(access(dip, IWRITE))
		return;		/* errno set by access */

	if (mnt) {
		/*
		 * from fmount(), `dip' is locked and reserved
		 */
		mson(cip, dip, flag, fstyp);
	} else {
		/*
		 * from fmount(), `dip' not yet locked or reserved
		 */
		dip->i_count++;		/* reserve dip */
		msoff(dip);
		iput(dip);		/* let it disappear */
	}
}

/*
 *  Unmount the stream currently mounted and mount a new one.
 *  `rip' is locked and reserved.
 */
msoffon(cip, rip, flag, fstyp)
	register struct inode *cip;	/* stream being mounted */
	register struct inode *rip;	/* root inode for new fs */
{
	register struct inode *dip;	/* mount point */

	dip = rip->i_mpoint;
	dip->i_count++;		/* reserve dip */

	prele(rip);
	msoff(dip);

	plock(dip);
	mson(cip, dip, flag, fstyp);

	iput(dip);		/* let dip disappear */
}

/*
 *  Create an in core root node, mount it, and attach a communications inode.
 *  On entry, `dip' is locked and reserved.  On exit, `dip' is still locked
 *  and reserved (i.e. shoud be iput).
 */
mson(cip, dip, flag, fstyp)
	register struct inode *cip;	/* stream being mounted */
	register struct inode *dip;	/* mount point */
{
	register struct inode *rip;	/* root inode for new fs */
	struct inode *ifake();

	/* must be mounting a stream */
	if(cip->i_sptr == NULL) {
		u.u_error = ENXIO;
		return;
	}

	/* already mounted? */
	if(dip->i_fstyp == fstyp) {
		if (dip->i_un.i_cip==NULL || dip->i_un.i_cip->i_sptr->flag&HUNGUP)
			u.u_error = EBUSY;		/* in use */
		else
			msoffon(cip, dip, flag, fstyp);	/* old mount is stale */
		return;
	}

	/* create a new inode for the fs root */
	rip = ifake(fstyp);
	if(rip == NULL)
		return;		/* errno already set */

	rip->i_un.i_cip = cip;
	rip->i_mpoint = dip;
	rip->i_mroot = NULL;
	prele(rip);
	cip->i_count++;
	dip->i_mroot = rip;
	dip->i_flag |= IMOUNT;
	dip->i_count++;
}

/*
 *  Unmount the stream and close it if approriate.
 *  On entry, `dip' is reserved.  On exit, it is still reserved.
 */
msoff(dip)
	register struct inode *dip;	/* mount point */
{
	register struct inode *rip;	/* root inode */
	register struct inode *cip;	/* mounted stream */

	/* lock the mount point */
	plock(dip);

	/* if not mounted, we're done */
	if (!(dip->i_flag&IMOUNT)) {
		prele(dip);
		return;
	}

	/* lock the root */
	rip = dip->i_mroot;
	plock(rip);

	/* disassociate the stream from the mount point */
	cip = rip->i_un.i_cip;
	rip->i_un.i_cip = NULL;
	if(cip != NULL) {
		plock(cip);
		if(cip->i_count==1)
			stclose(cip, 1);
		iput(cip);
	}

	/* unmount root from file system */
	dip->i_flag &= ~IMOUNT;
	dip->i_mroot = NULL;
	rip->i_mpoint = NULL;
	iput(dip);
	iput(rip);
}

/*
 * get the inode
 * it should always be the `root,'
 * which is always in core,
 * hence we should get here only when creating the inode.
 */
struct inode *
msget(fstyp, dev, ino, rip)
	dev_t dev;
	struct inode *rip;
{
	rip->i_flag |= IACC;		/* to force iupdat's */
	rip->i_mode = 0;		/* mark as unopened */
	return rip;
}

/*
 *  Pass read onto the stream's inode.
 *  `rip' is locked during this operation.
 */
msread(rip)
	struct inode *rip;
{
	register struct inode *cip;

	cip = rip->i_un.i_cip;
	if (cip!=NULL)
		readi(cip);
	else
		u.u_error = EPIPE;
}

/*
 *  Pass write onto the stream's inode.
 *  `rip' is locked during this operation.
 */
mswrite(rip)
	struct inode *rip;
{
	register struct inode *cip;

	cip = rip->i_un.i_cip;
	if (cip!=NULL)
		writei(cip);
	else
		u.u_error = EPIPE;
}

/*
 *  Pass ioctl onto the stream's inode.
 *  `rip' is locked during this operation.
 */
msioctl(rip, cmd, cmarg, flag)
register struct inode *rip;
caddr_t cmarg;
{
	register struct inode *cip;

	cip = rip->i_un.i_cip;
	if (cip!=NULL)
		stioctl(cip, cmd, cmarg);
	else
		u.u_error = EPIPE;
}

/*
 * stat `mounted' inode
 */
msstat(rip, ub)
	struct inode *rip;
	struct stat *ub;
{
	struct stat ds;

	ds.st_dev = rip->i_dev;
	ds.st_ino = rip->i_number;
	ds.st_mode = rip->i_mode;
	ds.st_nlink = 0;
	ds.st_uid = rip->i_uid;
	ds.st_gid = rip->i_gid;
	ds.st_rdev = (dev_t)0;
	ds.st_size = 0;
	ds.st_atime = time;
	ds.st_mtime = time;
	ds.st_ctime = time;
	if (copyout((caddr_t)&ds, (caddr_t)ub, sizeof(ds)) < 0)
		u.u_error = EFAULT;
}

/*
 * If the node is still mounted and the stream has hung up,
 * unmount it.  On entry, rip is locked.
 * This is called by `update()'.
 */
msupdat(rip, ta, tm, waitfor)
	register struct inode *rip;
	time_t *ta, *tm;
{
	register struct inode *cip;
	register struct inode *dip;

	cip = rip->i_un.i_cip;
	if (cip!=NULL && cip->i_sptr->flag&HUNGUP) {
		dip = rip->i_mpoint;
		dip->i_count++;		/* reserve dip */
		prele(rip);
		msoff(dip);
		iput(dip);		/* let dip disappear */
	}
}

/*
 *  Change an open of the root into a stopen of the communications inode.
 *  As usual, be wary of i_count's.
 */
struct inode *
msopen(rip, rw)
register struct inode *rip;
{
	register struct inode *cip;
	register struct inode *dip;
	dev_t dev;

	cip = rip->i_un.i_cip;
	if (cip==NULL) {
		u.u_error = ENXIO;
		return NULL;
	}

	/* dismount if the stream is closed */
	if (cip->i_sptr->flag&HUNGUP) {
		dip = rip->i_mpoint;
		dip->i_count++;		/* reserve dip */
		prele(rip);
		msoff(dip);
		iput(dip);		/* let dip disappear */
		u.u_error = ENXIO;
		return NULL;
	}

	/* replace rip by cip as inode being opened */
	cip->i_count++;	/* corresponding iput done by stopen (yech!) */
	iput(rip);	/* because of namei in open/creat */
	plock(cip);

	/* reopen the stream (must already be open) */
	if (cip->i_sptr==NULL)
		panic("msopen");
	dev = (dev_t)cip->i_un.i_rdev;
	return(stopen(0, dev, rw, cip));
}
