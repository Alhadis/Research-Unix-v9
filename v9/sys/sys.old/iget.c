/*	iget.c	4.4	81/03/08	*/

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mount.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/inode.h"
#include "../h/ino.h"
#include "../h/filsys.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/trace.h"
#include "../h/proc.h"

#define	INOHSZ	63
/* INOHASH can't depend on fstype as long as unmount changes fstyp to err-fs */
#define	INOHASH(dev, ino)	(((dev)+(ino))%INOHSZ)
struct inode *inohash[INOHSZ];
struct inode *ifreel;

/*
 * Initialize hash links for inodes
 * and build inode free list.
 */
ihinit()
{
	register int i;
	register struct inode *ip = inode;

	ifreel = inode;
	for (i = 0; i < ninode-1; i++, ip++)
		ip->i_hlink = ip + 1;
	ip->i_hlink = NULL;
	for (i = 0; i < INOHSZ; i++)
		inohash[i] = NULL;
}

/*
 * Find an inode if it is incore.
 * This is the equivalent, for inodes,
 * of ``incore'' in bio.c or ``pfind'' in subr.c.
 */
struct inode *
ifind(hp, ino)
register struct inode *hp;
register ino_t ino;
{
	register struct inode *ip;

	for (ip = inohash[INOHASH(hp->i_dev,ino)]; ip; ip = ip->i_hlink)
		if (ino==ip->i_number && hp->i_dev==ip->i_dev && hp->i_fstyp==ip->i_fstyp)
			return (ip);
	return ((struct inode *)0);
}

/*
 * default entry for file system switch entry `t_get'
 * put the inode, set errno, and return null.
 */
struct inode *
nullget(fip, dev, ino, ip)
	struct inode *fip;
	dev_t dev;
	ino_t ino;
	struct inode *ip;
{
	iput(ip);
	u.u_error = ENXIO;
	return(NULL);
}


/*
 * Look up an inode by filsys, i-number.
 * filsys is denoted by some inode in that filesystem.
 * If it is in core (in the inode structure),
 * honor the locking protocol.
 * If it is not in core, read it in from the
 * specified device.
 * If the inode is mounted on, perform
 * the indicated indirection.
 * In all cases, a pointer to a locked
 * inode structure is returned.
 *
 * panic: iget mroot -- if the mounted file
 *	system root is missing
 *	"cannot happen"
 */
struct inode *
iget(fip, dev, ino)
register struct inode *fip;
dev_t dev;
register ino_t ino;
{
	register struct inode *ip;
	register int slot;

loop:
	slot = INOHASH(dev, ino);
	for (ip = inohash[slot]; ip; ip = ip->i_hlink) {
		if(ino == ip->i_number && dev == ip->i_dev
			&& fip->i_fstyp == ip->i_fstyp) {
mloop:
			if((ip->i_flag&ILOCK) != 0) {
				ip->i_flag |= IWANT;
				ip->i_count++;	/* don't move! */
				sleep((caddr_t)ip, PINOD);
				ip->i_count--;
				goto loop;
			}
			if((ip->i_flag&IMOUNT) != 0) {
				if (ip->i_mroot == NULL)
					panic("iget mroot");
				ip = ip->i_mroot;
				goto mloop;
			}
			ip->i_count++;
			ip->i_flag |= ILOCK;
			return(ip);
		}
	}
	if(ifreel == NULL) {
		tablefull("inode");
		u.u_error = ENFILE;
		return(NULL);
	}
	ip = ifreel;
	ifreel = ip->i_hlink;
	ip->i_hlink = inohash[slot];
	inohash[slot] = ip;
	ip->i_dev = dev;
	ip->i_fstyp = fip->i_fstyp;
	ip->i_number = ino;
	ip->i_flag = ILOCK;
	ip->i_count++;
	ip->i_sptr = NULL;
	ip->i_mroot = NULL;
	ip->i_mpoint = fip->i_mpoint;	/* namei will fill in */
	return((*fstypsw[fip->i_fstyp].t_get)(fip, dev, ino, ip));
}

/*
 * Decrement reference count of
 * an inode structure.
 * On the last reference,
 * write the inode out and if necessary,
 * truncate and deallocate the file.
 */

iput(ip)
register struct inode *ip;
{
	register int i;
	register struct inode *jp;

	if(ip->i_count == 1) {
		ip->i_flag |= ILOCK;
		if(ip->i_nlink <= 0)
			(*fstypsw[ip->i_fstyp].t_free)(ip);
		(*fstypsw[ip->i_fstyp].t_put)(ip);
		i = INOHASH(ip->i_dev, ip->i_number);
		if (inohash[i] == ip)
			inohash[i] = ip->i_hlink;
		else {
			for (jp = inohash[i]; jp; jp = jp->i_hlink)
				if (jp->i_hlink == ip) {
					jp->i_hlink = ip->i_hlink;
					goto done;
				}
			panic("iput");
		}
done:
		prele(ip);
		ip->i_hlink = ifreel;
		ifreel = ip;
		ip->i_flag = 0;
		ip->i_number = 0;
	} else if(ip->i_count == 0) {
		panic("i_count==0");
		printf("i_count==0, ip %x dev %x ino %d fstyp %d\n", ip, ip->i_dev,
			ip->i_number, ip->i_fstyp);
		return;	/* that leaves the turkey locked (for safety) */
	} else
		prele(ip);
	ip->i_count--;
}

/*
 * Check accessed and update flags on
 * an inode structure.
 * If any is on, update the inode
 * with the current time.
 * If waitfor is given, then must insure
 * i/o order so wait for write to complete.
 */
iupdat(ip, ta, tm, waitfor)
register struct inode *ip;
time_t *ta, *tm;
int waitfor;
{

	if((ip->i_flag&(IUPD|IACC|ICHG)) != 0)
		(*fstypsw[ip->i_fstyp].t_updat)(ip, ta, tm, waitfor);
}

/*
 * create a non-disk inode for a file system type;
 * the inode returned is plocked and must be either
 * iput or prele'sed.
 */
struct inode *
ifake(fstyp)
	int fstyp;
{
	struct inode *ip;
	static ino_t ino=0;
	ino_t inostart;
	struct inode pi;		/* primer */

	pi.i_dev = 0;
	pi.i_fstyp = fstyp;
	for(inostart=ino;;) {
		ip = iget(&pi, 0, ino);
		if (ip == NULL)
			return(NULL);
		if (ip->i_count == 1)
			break;
		/*
		 *  This inode is still in use - pick another
		 */
		iput(ip);
		/*
		 *  Make sure we haven't gone through all the inodes
		 */
		if (inostart==++ino) {
			tablefull("fake inode");
			u.u_error = ENFILE;
			return(NULL);
		}
	}
	ip->i_mode = IFREG | (0666 & ~u.u_cmask);
	ip->i_uid = u.u_uid;
	ip->i_gid = u.u_gid;
	ip->i_un.i_rdev = (dev_t)0;
	ino = inostart+1;
	return(ip);
}
