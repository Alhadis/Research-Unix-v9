/*	fio.c	4.8	81/03/08	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/filsys.h"
#include "../h/file.h"
#include "../h/conf.h"
#include "../h/inode.h"
#include "../h/stream.h"
#include "../h/buf.h"
/*#include "../h/reg.h"*/
#include "../h/acct.h"
#include "../h/mount.h"

/*
 * Convert a user supplied
 * file descriptor into a pointer
 * to a file structure.
 * Only task is to check range
 * of the descriptor.
 */

#ifndef getf	/* done inline */

struct file *
getf(f)
register int f;
{
	register struct file *fp;

	if ((unsigned)f >= NOFILE || (fp = u.u_ofile[f]) == NULL)
		return (NULL);
	return (fp);
}

#endif

/*
 * Internal form of close.
 * Decrement reference count on
 * file structure.
 * Also make sure the pipe protocol
 * does not constipate.
 *
 * Decrement reference count on the inode following
 * removal to the referencing file structure.
 * Call device handler on last close.
 */
closef(fp)
register struct file *fp;
{
	register struct inode *ip;
#ifdef	CHAOS
	caddr_t cp;	/* Chaosnet connection pointer */
#endif	CHAOS
	int flag, mode;
	dev_t dev;
	register int (*cfunc)();

	if(fp == NULL)
		return;
	if (fp->f_count > 1) {
		fp->f_count--;
		return;
	}
	ip = fp->f_inode;
	flag = fp->f_flag;
	dev = (dev_t)ip->i_un.i_rdev;
	mode = ip->i_mode & IFMT;

	plock(ip);
#ifdef	CHAOS
	cp = (caddr_t)fp->f_conn;
#endif	CHAOS
	fp->f_count = 0;
	if (ip->i_sptr && ip->i_count==1)
		stclose(ip, 1);
	iput(ip);

	switch(mode) {

	case IFCHR:
		cfunc = cdevsw[major(dev)].d_close;
		break;

	case IFBLK:
		/*
		 * We don't want to really close the device if it is mounted
		 */
		if (findmount(0, dev) != NULL)
			return;
		cfunc = bdevsw[major(dev)].d_close;
		break;
	default:
		return;
	}
#ifdef	CHAOS
	if (cp == (caddr_t) 0)
#endif	CHAOS
	for(fp=file; fp < fileNFILE; fp++)
		if (fp->f_count && (ip=fp->f_inode) && ip->i_un.i_rdev==dev &&
		    (ip->i_mode&IFMT) == mode)
			return;
	if (mode == IFBLK) {
		/*
		 * on last close of a block device (that isn't mounted)
		 * we must invalidate any in core blocks
		 */
		bflush(dev);
		binval(dev);
	}
#ifdef	CHAOS
	if (cp)
		(*cfunc)(dev, flag, cp);
	else
#endif	CHAOS
		(*cfunc)(dev, flag, fp);
}

/*
 * the default open routine for the file system switch entry `t_open'
 */
struct inode *
nullopen(ip, rw)
	struct inode *ip;
{
	return(NULL);
}

/*
 * openi called to allow handler
 * of special files to initialize and
 * validate before actual IO.
 * May return an alternate inode for generic devices or streams.
 */
struct inode *
openi(ip, rw)
register struct inode *ip;
{
	dev_t dev;

	dev = (dev_t)ip->i_un.i_rdev;
	if (ip->i_sptr) 		/* stream is attached */
		return(stopen(cdevsw[major(dev)].qinfo, dev, rw, ip));
	return (*fstypsw[ip->i_fstyp].t_open)(ip, rw);
}

/*
 * Check mode permission on inode pointer.
 * Mode is READ, WRITE or EXEC.
 * In the case of WRITE, the
 * read-only status of the file
 * system is checked.
 * Also in WRITE, prototype text
 * segments cannot be written.
 * The mode is shifted to select
 * the owner/group/other fields.
 * The super user is granted all
 * permissions.
 */
access(ip, mode)
register struct inode *ip;
{
	register m;
	register short *gp;

	m = mode;
	if(m == IWRITE) {
		if(ip->i_fstyp==0 && ip->i_un.i_bufp->b_un.b_filsys->s_ronly != 0) {
			u.u_error = EROFS;
			return(1);
		}
		if (ip->i_flag&ITEXT)		/* try to free text */
			xrele(ip);
		if(ip->i_flag & ITEXT) {
			u.u_error = ETXTBSY;
			return(1);
		}
	}
	if(u.u_uid == 0)
		return(0);
	if(u.u_uid != ip->i_uid) {
		m >>= 3;
		if(u.u_gid == ip->i_gid)
			goto found;
		gp = u.u_groups;
		for (; gp < &u.u_groups[NGROUPS] && *gp != NOGROUP; gp++)
			if (ip->i_gid == *gp)
				goto found;
		m >>= 3;
	}
found:
	if((ip->i_mode&m) != 0)
		return(0);

	u.u_error = EACCES;
	return(1);
}

/*
 * Look up a pathname and test if
 * the resultant inode is owned by the
 * current user.
 * If not, try for super-user.
 * If permission is granted,
 * return inode pointer.
 */
struct inode *
owner(follow)
{
	register struct inode *ip;

	ip = namei(uchar, (struct argnamei *)NULL, follow);
	if(ip == NULL)
		return(NULL);
	if(u.u_uid == ip->i_uid)
		return(ip);
	if(suser())
		return(ip);
	iput(ip);
	return(NULL);
}

/*
 *  Return 0 if the file isn't open for writing, 1 if it is
 */
writers(ip)
	register struct inode *ip;
{
	register struct file *fp;

	for (fp = file; fp < fileNFILE; fp++)
		if (fp->f_count!=0 && fp->f_inode==ip && (fp->f_flag&FWRITE))
			return(1);
	return(0);
}

/*
 *  Control concurrent access to a file.  The possibile types are:
 *  - 1 writer and n readers	(I1WNR)
 *  - 1 writer or n readers	(ILCKD)
 *  The file pointers are searched.  If an open with the given mode
 *  would result in a violation, errno is set to ECONC.
 */
concurrency(ip, mode)
	register struct inode *ip;
	int mode;
{
	switch(ip->i_mode&ICCTYP) {
	case ISYNC:
		if ((mode&FWRITE) && writers(ip))
			break;
		else
			return;
	case IEXCL:
		if ((mode&FWRITE) || writers(ip))
			break;
		else
			return;
	default:
		return;
	}
	u.u_error = ECONC;
}

/*
 * Test if the current user is the
 * super user.
 */
suser()
{

	if(u.u_uid == 0) {
		u.u_acflag |= ASU;
		return(1);
	}
	u.u_error = EPERM;
	return(0);
}

/*
 * Allocate a user file descriptor.
 */
ufalloc()
{
	register i;

	for(i=0; i<NOFILE; i++)
		if(u.u_ofile[i] == NULL) {
			u.u_r.r_val1 = i;
			u.u_pofile[i] = 0;
			return(i);
		}
	u.u_error = EMFILE;
	return(-1);
}

struct	file *lastf;
/*
 * Allocate a user file descriptor
 * and a file structure.
 * Initialize the descriptor
 * to point at the file structure.
 */
struct file *
falloc()
{
	register struct file *fp;
	register i;

	i = ufalloc();
	if (i < 0)
		return(NULL);
	if ((fp = allocfile()) == NULL) {
		u.u_error = ENFILE;
		return(NULL);
	}
	u.u_ofile[i] = fp;
	return (fp);
}

/*
 * allocate file structure
 */
struct file *
allocfile()
{
	register struct file *fp;

	if (lastf == 0)
		lastf = file;
	for (fp = lastf; fp < fileNFILE; fp++)
		if (fp->f_count == 0)
			goto gotit;
	for (fp = file; fp < lastf; fp++)
		if (fp->f_count == 0)
			goto gotit;
	tablefull("file");
	return (NULL);
gotit:
	lastf = fp + 1;
	fp->f_count++;
	fp->f_offset = 0;
#ifdef	CHAOS
	fp->f_conn = (caddr_t) 0;
#endif	CHAOS
	return(fp);
}
