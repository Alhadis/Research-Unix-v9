#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/inode.h"
#include "../h/file.h"
#include "../h/reg.h"
#include "../h/inline.h"
#include "../h/proc.h"
#include "../h/stream.h"
#include "../h/stat.h"

struct	inode *mkpipend();
extern int pipefstyp;
extern struct streamtab nilinfo;

/*
 * The sys-pipe entry.
 * Allocate 2 open inodes, stream them, and splice them together
 */
pipe()
{
	struct inode *ip1, *ip2;
	register struct file *rf, *wf;
	register r;

	rf = falloc();
	if(rf == NULL)
		return;
	r = u.u_r.r_val1;
	wf = falloc();
	if(wf == NULL) {
		rf->f_count = 0;
		u.u_ofile[r] = NULL;
		return;
	}
	u.u_r.r_val2 = u.u_r.r_val1;
	u.u_r.r_val1 = r;
	if (makepipe(&ip1, &ip2)==0) {
		rf->f_count = 0;
		wf->f_count = 0;
		u.u_ofile[u.u_r.r_val1] = NULL;
		u.u_ofile[u.u_r.r_val2] = NULL;
		return;
	}
	wf->f_flag = FREAD|FWRITE;
	wf->f_inode = ip1;
	rf->f_flag = FREAD|FWRITE;
	rf->f_inode = ip2;
}

makepipe(ip1, ip2)
register struct inode **ip1, **ip2;
{

	*ip1 = mkpipend();
	*ip2 = mkpipend();
	if (*ip1==NULL || *ip2==NULL) {
		if (*ip1) {
			stclose(*ip1, 0);
			iput(*ip1);
		}
		if (*ip2) {
			stclose(*ip2, 0);
			iput(*ip2);
		}
		return(0);
	}
	qdetach(RD((*ip1)->i_sptr->wrq->next), 1);
	(*ip1)->i_sptr->wrq->next = RD((*ip2)->i_sptr->wrq);
	qdetach(RD((*ip2)->i_sptr->wrq->next), 1);
	(*ip2)->i_sptr->wrq->next = RD((*ip1)->i_sptr->wrq);
	return(1);
}

struct inode *
mkpipend()
{
	register struct inode *ip;
	struct inode *ifake();

	ip = ifake(pipefstyp);
	if (ip==NULL)
		return(NULL);
	stopen(&nilinfo, (dev_t)0, 0, ip);
	if (u.u_error) {
		iput(ip);
		return(NULL);
	}
	prele(ip);
	return(ip);
}

#ifndef plock		/* done inline */
/*
 * Lock an inode
 * If its already locked,
 * set the WANT bit and sleep.
 */
plock(ip)
register struct inode *ip;
{

	while(ip->i_flag&ILOCK) {
		ip->i_flag |= IWANT;
		sleep((caddr_t)ip, PINOD);
	}
	ip->i_flag |= ILOCK;
}

/*
 * Unlock an inode.
 * If WANT bit is on,
 * wakeup.
 */
prele(ip)
register struct inode *ip;
{
	ip->i_flag &= ~ILOCK;
	if(ip->i_flag&IWANT) {
		ip->i_flag &= ~IWANT;
		wakeup((caddr_t)ip);
	}
}
#endif

/* the pipe file system type - the following calls short circuit ALL
 * disk activity for pipe inodes.
 */

struct inode *
pipget(fip, dev, ino, ip)
	struct inode *fip;
	dev_t dev;
	ino_t ino;
	struct inode *ip;
{
	/* mark as an unopened pipe */
	ip->i_mode = 0;
	return ip;
}

pipmount()
{
	/* can't be mounted */
	u.u_error = ENXIO;
	return;
}

pipstat(ip, ub)
	struct inode *ip;
	struct stat *ub;
{
	struct stat ds;

	ds.st_dev = ip->i_dev;
	ds.st_ino = ip->i_number;
	ds.st_mode = ip->i_mode;
	ds.st_nlink = 0;
	ds.st_uid = ip->i_uid;
	ds.st_gid = ip->i_gid;
	ds.st_rdev = (dev_t)0;
	ds.st_size = 0;
	ds.st_atime = time;
	ds.st_mtime = time;
	ds.st_ctime = time;
	if (copyout((caddr_t)&ds, (caddr_t)ub, sizeof(ds)) < 0)
		u.u_error = EFAULT;
}
