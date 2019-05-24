/*
 * traditional disk filesystem
 */
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/ino.h"
/*#include "../h/reg.h"*/
#include "../h/buf.h"
#include "../h/filsys.h"
#include "../h/mount.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/inode.h"
#include "../h/file.h"
#include "../h/conf.h"
#include "../h/vlimit.h"
#include "../h/stat.h"
#include "../h/filio.h"
#include "../h/stream.h"

/*
 * silly mount/unmount routine for fs type 0
 */

fsmount(sip, ip, flag, mnt, fstyp)
register struct inode *ip, *sip;
int flag, mnt, fstyp;
{

	if (!suser())
		return;
	if (mnt)
		fson(sip, ip, flag, fstyp);
	else
		fsoff(ip, fstyp);
}

/*
 * mount a type 0 filesystem
 * maintain the mount table for vm code's sake
 */

fson(sip, ip, ronly, fstyp)
register struct inode *ip, *sip;
int ronly, fstyp;
{
	dev_t dev;
	register struct mount *mp;
	struct inode *rip;
	register struct filsys *fp;
	struct buf *bp;
	struct inode pi;	/* primer */

	if(ip->i_count!=1) {
		u.u_error = EINVAL;
		return;
	}
	if ((ip->i_mode & IFMT) != IFDIR
	&&  (ip->i_mode & IFMT) != IFREG) {
		u.u_error = EINVAL;
		return;
	}
	dev = (dev_t)sip->i_un.i_rdev;
	if ((sip->i_mode&IFMT) != IFBLK || major(dev) >= nblkdev) {
		u.u_error = EINVAL;
		return;
	}
	if ((mp = allocmount(fstyp, dev)) == NULL) {
		u.u_error = EBUSY;
		return;
	}
	(*bdevsw[major(dev)].d_open)(dev, !ronly);
	if(u.u_error) {
		freemount(mp);
		return;
	}
	bp = bread(dev, SUPERB);
	if(u.u_error) {
		freemount(mp);
		brelse(bp);
		return;
	}
	mp->m_inodp = ip;
	bp->b_flags |= B_LOCKED;
	mp->m_bufp = bp;
	fp = bp->b_un.b_filsys;
	fp->s_ilock = 0;
	fp->s_flock = 0;
	fp->s_ronly = ronly & 1;
	fp->s_nbehind = 0;
	fp->s_lasti = 1;
	brelse(bp);
	if(BITFS(dev) && !fp->s_valid && !fp->s_ronly) {
		bp->b_flags &= ~B_LOCKED;
		freemount(mp);
		u.u_error = EBUSY;			/* NOT IMPLEMENTED */
		return;
	}
	pi.i_dev = dev;
	pi.i_fstyp = fstyp;
	pi.i_un.i_bufp = bp;
	if ((rip = iget(&pi, dev, ROOTINO)) == NULL) {
		bp->b_flags &=~ B_LOCKED;
		freemount(mp);
		u.u_error = EBUSY;
		return;
	}
	if (rip->i_count > 1) {
		iput(rip);
		bp->b_flags &=~ B_LOCKED;
		freemount(mp);
		u.u_error = EBUSY;
		return;
	}
	ip->i_mroot = rip;
	rip->i_mpoint = ip;
	ip->i_count++;
	ip->i_flag |= IMOUNT;
	prele(rip);
}

fsoff(mip, fstyp)
register struct inode *mip;
{
	dev_t dev;
	register struct mount *mp;
	register struct inode *xip, *rip;
	struct buf *bp;
	int stillopen, flag;

	rip = mip->i_mroot;
	plock(rip);
	dev = rip->i_dev;
	xumount(dev);	/* remove unused sticky files from text table */
	update();	/* silly */
	if (rip->i_count > 1) {
		u.u_error = EBUSY;
		prele(rip);
		return;
	}
	stillopen = 0;
	for(xip = inode; xip < inodeNINODE; xip++) {
		if (xip == rip)
			continue;
		if (xip->i_number != 0 && dev == xip->i_dev) {
			u.u_error = EBUSY;
			prele(rip);
			return;
		}
		if (xip->i_number != 0 && (xip->i_mode&IFMT) == IFBLK &&
		    xip->i_un.i_rdev == dev)
			stillopen++;
	}
	plock(mip);
	mip->i_flag &= ~IMOUNT;
	mip->i_mroot = NULL;
	iput(mip);
	if ((bp = getblk(dev, SUPERB)) != rip->i_un.i_bufp)
		panic("umount");
	iput(rip);
	bp->b_un.b_filsys->s_valid = 1;
	bp->b_flags &= ~B_LOCKED;
	flag = !bp->b_un.b_filsys->s_ronly;
	if(BITFS(dev) && flag)
		bwrite(bp);
	else
		brelse(bp);
	if ((mp = findmount(fstyp, dev)) == NULL)
		panic("umount mp");
	mpurge(mp - &mount[0]);
	freemount(mp);
	if (!stillopen) {
		(*bdevsw[major(dev)].d_close)(dev, flag);
		binval(dev);
	}
}

fsnami(p, pflagp, follow)
struct nx *p;
struct argnamei **pflagp;
{	register struct inode *dp;
	register char *cp;
	register struct buf *bp;
	register struct inode *dip;
	int i;
	struct argnamei *flagp = *pflagp;
	struct inode *domkfile();
	ino_t dsearch();
#ifdef	CHAOS
	extern long cdevpath;
#endif	CHAOS

	cp = p->cp;
	dp = p->dp;

	/*
	 * dp must be a directory and
	 * must have X permission.
	 * cp is a path name relative to that directory.
	 */

dirloop:
	if((dp->i_mode&IFMT) != IFDIR)
		u.u_error = ENOTDIR;
	(void) access(dp, IEXEC);
	for (i=0; *cp!='\0' && *cp!='/'; i++) {
		if (i >= DIRSIZ) {
			u.u_error = ENOENT;
			break;
		}
		u.u_dbuf[i] = *cp++;
	}
	if(u.u_error)
		goto outnull;
	while (i < DIRSIZ)
		u.u_dbuf[i++] = '\0';
	if (u.u_dbuf[0] == '\0') {		/* null name, e.g. "/" or "" */
		if (flagp) {
			u.u_error = ENOENT;
			goto outnull;
		}
		goto outOK;
	}
	u.u_segflg = 1;
	if (dsearch(dp, (int *)NULL) == 0) {
		if (u.u_error)
			goto outnull;
		/*
		 * Search failed.
		 */
		if(flagp==0 || *cp) {
			u.u_error = ENOENT;
			goto outnull;
		}
		dip = domkfile(dp, pflagp);
		if (dip) {
			dp = dip;
			goto outOK;
		}
		goto outnull;
	}
	if(flagp && flagp->flag == NI_DEL && *cp == 0) {
		dormfile(dp);
		goto outnull;
	}
	if(flagp && flagp->flag == NI_RMDIR && *cp == 0) {
		dormdir(dp);
		goto outnull;
	}
	/*
	 * Special handling for ".."
	 */
	if (u.u_dent.d_name[0]=='.' && u.u_dent.d_name[1]=='.' &&
	    u.u_dent.d_name[2]=='\0') {
		if (dp == u.u_rdir)
			u.u_dent.d_ino = dp->i_number;
		else if (dp != rootdir && u.u_dent.d_ino==ROOTINO &&
		   dp->i_number == ROOTINO) {
			if ((dip = dp->i_mpoint) == NULL)
				panic("namei: mpoint");
			iput(dp);
			dp = dip;
			plock(dp);
			dp->i_count++;
			cp -= 2;     /* back over .. */
			goto dirloop;
		}
	}
	/*
	 * search succeeded
	 */
	dip = dp;
	prele(dip);
	dp = iget(dip, dip->i_dev, u.u_dent.d_ino);
	if(dp == NULL) {
		iput(dip);
		goto outOK;
	}
	if(dip->i_fstyp != dp->i_fstyp) {
		iput(dip);
		for(; *cp == '/'; cp++)
			;
		p->dp = dp;
		p->cp = cp;
		return(2);
	}
	/*
	 * Check for symbolic link
	 */
	if ((dp->i_mode&IFMT)==IFLNK && (follow || *cp=='/')) {
		char *ocp;

		ocp = cp;
		while (*cp++)
			;
		if (dp->i_size + (cp-ocp) >= BSIZE(dp->i_dev)-1
		|| ++p->nlink>8) {
			u.u_error = ELOOP;
			iput(dip);
			goto outnull;
		}
		ovbcopy(ocp, p->nbp->b_un.b_addr+dp->i_size, cp-ocp);
		bp = bread(dp->i_dev, bmap(dp, (daddr_t)0, B_READ));
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			iput(dip);
			goto outnull;
		}
		bcopy(bp->b_un.b_addr, p->nbp->b_un.b_addr, dp->i_size);
		brelse(bp);
		cp = p->nbp->b_un.b_addr;
		iput(dp);
		if (*cp == '/') {
			iput(dip);
			while (*cp == '/')
				cp++;
			if ((dp = u.u_rdir) == NULL)
				dp = rootdir;
			plock(dp);
			dp->i_count++;
		} else {
			dp = dip;
			plock(dp);
		}
		goto dirloop;
	}
	iput(dip);
#ifdef	CHAOS
#define	CHDEV_OFFSET	32
	if ((dp->i_mode & IFMT) == IFCHR && (cdevpath & (1L << (major(dp->i_un.i_rdev) - CHDEV_OFFSET))))
	{
		char *ocp;

		while (*cp == '/')
			cp++;
		ocp = cp;
		while (*cp++)
			;
		u.u_dirp -= (cp - ocp);
		goto outOK;
	}
#endif	CHAOS
	if (*cp == '/') {
		while (*cp == '/')
			cp++;
		goto dirloop;
	}
	/* search finally succeeded */
	if (flagp
	 && (flagp->flag==NI_LINK || flagp->flag==NI_NXCREAT
	    || flagp->flag == NI_MKDIR)) {
		u.u_error = EEXIST;
		goto outnull;
	}
	goto outOK;

outnull:
	p->dp = dp;
	return(1);
outOK:
	p->dp = dp;
	return(0);
}

/*
 * create a new file of some sort in directory dp
 *   name is in u.u_dbuf
 *   offset left over in u.u_offset
 *   return inode if it is needed
 */
struct inode *
domkfile(dp, pflagp)
register struct inode *dp;
register struct argnamei **pflagp;
{
	register struct inode *dip;
	register struct argnamei *flagp = *pflagp;
	struct direct x[2];
	register off_t off;
	register i;

	if(access(dp, IWRITE))
		return(NULL);
	bcopy((caddr_t)u.u_dbuf, (caddr_t)u.u_dent.d_name, DIRSIZ);
	u.u_count = sizeof(struct direct);
	u.u_base = (caddr_t)&u.u_dent;
	switch(flagp->flag) {

	case NI_LINK:	/* make a link */
		if(dp->i_dev != flagp->il->i_dev) {
			u.u_error = EXDEV;
			return(NULL);
		}
		u.u_dent.d_ino = flagp->il->i_number;
		writei(dp);
		return(NULL);

	case NI_CREAT:	/* create a new file */
	case NI_NXCREAT:
		dip = ialloc(dp);
		if(dip == NULL)
			return(NULL);
		dip->i_flag |= IACC|IUPD|ICHG;
		dip->i_mode = flagp->mode;
		if((dip->i_mode & IFMT) == 0)
			dip->i_mode |= IFREG;
		dip->i_nlink = 1;
		dip->i_uid = u.u_uid;
		dip->i_gid = dp->i_mode & ISGID ? u.u_gid : dp->i_gid;
		if (u.u_uid && !groupmember(dip->i_gid))
			dip->i_mode &= ~ISGID;
		iupdat(dip, &time, &time, 1);
		u.u_dent.d_ino = dip->i_number;
		flagp->done = dip->i_number;
		writei(dp);
		iput(dp);
		return(dip);

	case NI_MKDIR:	/* make a new directory */
		dip = ialloc(dp);
		if(dip == NULL)
			return(NULL);
		u.u_dent.d_ino = dip->i_number;
		off = u.u_offset;
		dip->i_mode = flagp->mode;
		dip->i_nlink = 1;
		dip->i_uid = u.u_uid;
		dip->i_gid = dp->i_gid;
		dip->i_flag |= IACC|IUPD|ICHG;
		x[0].d_ino = dip->i_number;
		x[1].d_ino = dp->i_number;
		for(i = 0; i < DIRSIZ; i++)
			x[0].d_name[i] = x[1].d_name[i] = 0;
		x[0].d_name[0] = x[1].d_name[0] = x[1].d_name[1] = '.';
		u.u_count = sizeof(x);
		u.u_base = (caddr_t)x;
		u.u_offset = 0;
		u.u_segflg = 1;
		writei(dip);
		if (u.u_error) {
			dip->i_nlink--;
			iput(dip);
			return(NULL);
		}
		dip->i_nlink++;
		dp->i_nlink++;
		iupdat(dip, &time, &time, 1);
		u.u_count = sizeof(u.u_dent);
		u.u_base = (caddr_t)&u.u_dent;
		u.u_offset = off;
		writei(dp);
		iput(dip);
		return(NULL);
	}
	u.u_error = EINVAL;
	return(NULL);
}

/*
 * delete a non-directory file
 */
dormfile(dp)
register struct inode *dp;
{
	register struct inode *dip;

	if(access(dp, IWRITE))
		return;
	if(dp->i_number == u.u_dent.d_ino) {	/* for '.' */
		dip = dp;
		dp->i_count++;
	} else
		dip = iget(dp, dp->i_dev, u.u_dent.d_ino);
	if(dip == NULL)
		return;
	if(dip->i_dev != dp->i_dev || dip->i_fstyp != dp->i_fstyp) {	/* mounted FS */
		u.u_error = EBUSY;
		iput(dip);
		return;
	}
	if((dip->i_mode&IFMT) == IFDIR && !suser()) {
		iput(dip);
		return;
	}
	if(dip->i_flag&ITEXT)
		xrele(dip);		/* free busy text */
	u.u_base = (caddr_t)&u.u_dent;
	u.u_count = sizeof(struct direct);
	u.u_dent.d_ino = 0;
	writei(dp);		/* offset, segflg already set*/
	dip->i_nlink--;
	dip->i_flag |= ICHG;
	iput(dip);
}

/*
 * remove a directory (fsnami)
 * dp is inode of containing directory
 * u.u_dent is the entry (which exists)
 * u.u_offset is the offset in containing directory of this entry
 */
dormdir(dp)
register struct inode *dp;
{
	register struct inode *dip;
	off_t doff;
	struct direct dent;
	int nentry;
	ino_t dotino, dotdotino;
	ino_t dsearch();

	if(access(dp, IWRITE))
		return;
	if(dp->i_number == u.u_dent.d_ino
	 || strncmp(u.u_dent.d_name, "..", DIRSIZ)==0) { /* gets "." and "" */
		u.u_error = EINVAL;
		return;
	}
	if((dip = iget(dp, dp->i_dev, u.u_dent.d_ino)) == NULL)
		return;
	if(dip->i_number <= ROOTINO) {
		u.u_error = EINVAL;
		iput(dip);
		return;
	}
	if(dip->i_dev != dp->i_dev || dip->i_count > 1) {
		u.u_error = EBUSY;
		iput(dip);
		return;
	}
	if((dip->i_mode & IFMT) != IFDIR) {
		u.u_error = ENOTDIR;
		iput(dip);
		return;
	}
	/* save state, search for ., .., other entries in dir */
	doff = u.u_offset;
	dent = u.u_dent;
	cpdirent("/");		/* cannot be found; just count entries */
	dsearch(dip, &nentry);
	cpdirent(".");
	if ((dotino = dsearch(dip, (int *)NULL)))
		nentry--;
	cpdirent("..");
	if ((dotdotino = dsearch(dip, (int *)NULL)))
		nentry--;
	if (nentry > 0) {
		u.u_error = EHASF;	/* removing nonempty directory */
		iput(dip);
		return;
	}
	if (dotino) {
		if (dip->i_number == dotino)
			dip->i_nlink--;
		/* else error? */
	}
	if (dotdotino) {
		if (dp->i_number == dotdotino)
			dp->i_nlink--;
		/* else error? */
	}
	u.u_dent = dent;
	u.u_base = (caddr_t)&u.u_dent;
	u.u_count = sizeof(struct direct);
	u.u_offset = doff;
	u.u_dent.d_ino = 0;
	writei(dp);
	dp->i_flag |= ICHG;
	iupdat(dp, &time, &time, 1);
	dip->i_nlink--;
	dip->i_flag |= ICHG;
	iput(dip);
}

cpdirent(s)
register char *s;
{
	register char *dp = u.u_dbuf;

	while (dp < &u.u_dbuf[DIRSIZ]) {
		*dp++ = *s;
		if (*s)
			s++;
	}
}

/*
 * search directory ip for entry u.u_dbuf
 *  success: return ino, leave u.u_dent with copy of entry,
 *           u.u_offset pointing at entry
 *  fail:    return 0, leave u.u_offset pointing at empty slot
 *
 *  In any event count entries (for rmdir)
 */

ino_t
dsearch(ip, nentp)
struct inode *ip;
int *nentp;
{
	register struct direct *dp, *dpe;
	register char *nm;
	register off_t off;
	struct buf *bp;
	register nentry;
	register int bsize;
	register daddr_t n, nblock;

	u.u_dent.d_ino = 0;
	nm = u.u_dbuf;
	bsize = BSIZE(ip->i_dev);
	nentry = 0;
	bp = NULL;
	u.u_offset = -1;
	nblock = (ip->i_size+bsize-1) / bsize;

	for (n=0, off=0; n<nblock; n++) {
		if (bp)
			brelse(bp);
		bp = bread(ip->i_dev, bmap(ip, n, B_READ));
		if (bp->b_flags & B_ERROR) {
			u.u_error = EIO;
			goto out;
		}
		dp = (struct direct *)bp->b_un.b_addr;
		dpe = &dp[min(bsize, ip->i_size-off) / sizeof(struct direct)];
		for (; dp < dpe; dp++, off+=sizeof(struct direct)) {
			if (dp->d_ino == 0) {
				if (u.u_offset<0)
					u.u_offset = off;
				continue;
			}
			nentry++;
			if (nm[2]==dp->d_name[2]	/* hash */
			 && strncmp(nm, dp->d_name, DIRSIZ) == 0) {
				u.u_offset = off;
				u.u_dent = *dp;
				goto out;
			}
		}
	}
out:
	if (u.u_offset<0)
		u.u_offset = off;
	if (bp)
		brelse(bp);
	if (nentp)
		*nentp = nentry;
	return(u.u_dent.d_ino);
}

strncmp(s1, s2, len)
register char *s1, *s2;
register len;
{
	do {
		if (*s1 != *s2++)
			return(1);
		if (*s1++ == '\0')
			return(0);
	} while (--len);
	return(0);
}

struct inode *
fsget(fip, dev, ino, ip)
register struct inode *fip;
dev_t dev;
ino_t ino;
register struct inode *ip;
{
	register struct buf *bp;

	ip->i_un.i_lastr = 0;
	bp = bread(fip->i_dev, itod(fip->i_dev, ino));
	/*
	 * Check I/O errors
	 */
	if((bp->b_flags&B_ERROR) != 0) {
		brelse(bp);
		iput(ip);
		return(NULL);
	}
	iexpand(ip, bp->b_un.b_dino+itoo(fip->i_dev, ino));
	brelse(bp);
	ip->i_un.i_bufp = fip->i_un.i_bufp;
	return(ip);
}

iexpand(ip, dp)
register struct inode *ip;
register struct dinode *dp;
{
	register char *p1, *p2;
	register int i;

	ip->i_mode = dp->di_mode;
	ip->i_nlink = dp->di_nlink;
	ip->i_uid = dp->di_uid;
	ip->i_gid = dp->di_gid;
	ip->i_size = dp->di_size;
	p1 = (char *)ip->i_un.i_addr;
	p2 = (char *)dp->di_addr;
	for(i=0; i<NADDR; i++) {
#ifdef sun
		*p1++ = 0;
#endif
		*p1++ = *p2++;
		*p1++ = *p2++;
		*p1++ = *p2++;
#ifdef vax
		*p1++ = 0;
#endif
	}
}

fsput(ip)
struct inode *ip;
{
	if ((ip->i_flag&(IUPD|IACC|ICHG)) != 0)
		fsiupdat(ip, &time, &time, 0);
}

fsupdat(ip, ta, tm, waitfor)
register struct inode *ip;
time_t *ta, *tm;
{
	fsiupdat(ip, ta, tm, waitfor);
	if (ip->i_un.i_bufp->b_un.b_filsys->s_fmod)
		fsfupdat(ip);
}

/*
 * update the inode on disk
 * if the super-block is dirty, write it too
 */

fsiupdat(ip, ta, tm, waitfor)
register struct inode *ip;
time_t *ta, *tm;
{
	register struct buf *bp;
	struct dinode *dp;
	register char *p1, *p2;
	register int i;

	if (ip->i_un.i_bufp->b_un.b_filsys->s_ronly)
		return;
	bp = bread(ip->i_dev, itod(ip->i_dev, ip->i_number));
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return;
	}
	dp = bp->b_un.b_dino;
	dp += itoo(ip->i_dev, ip->i_number);
	dp->di_mode = ip->i_mode;
	dp->di_nlink = ip->i_nlink;
	dp->di_uid = ip->i_uid;
	dp->di_gid = ip->i_gid;
	dp->di_size = ip->i_size;
	p1 = (char *)dp->di_addr;
	p2 = (char *)ip->i_un.i_addr;
	for(i=0; i<NADDR; i++) {
#ifdef sun
		if(*p2++)
			printf("iaddress > 2^24\n");
#endif
		*p1++ = *p2++;
		*p1++ = *p2++;
		*p1++ = *p2++;
#ifdef vax
		if(*p2++)
			printf("iaddress > 2^24\n");
#endif
	}
	if(ip->i_flag&IACC)
		dp->di_atime = *ta;
	if(ip->i_flag&IUPD)
		dp->di_mtime = *tm;
	if(ip->i_flag&ICHG)
		dp->di_ctime = time;
	ip->i_flag &= ~(IUPD|IACC|ICHG);
	if (waitfor)
		bwrite(bp);
	else
		bdwrite(bp);
}

/*
 * write the super-block
 */

fsfupdat(ip)
struct inode *ip;
{
	register struct filsys *fp;
	struct buf *bp;

	fp = ip->i_un.i_bufp->b_un.b_filsys;
	if (fp->s_fmod == 0 || fp->s_ilock || fp->s_flock || fp->s_ronly)
		return;
	bp = getblk(ip->i_dev, SUPERB);
	if (bp->b_un.b_filsys != fp)
		panic("fsfupdat");
	fp->s_fmod = 0;
	fp->s_time = time;
	bwrite(bp);
}

fsfree(ip)
register struct inode *ip;
{

	fstrunc(ip);
	ip->i_mode = 0;
	ip->i_flag |= IUPD|ICHG;
	ifree(ip, (ino_t)ip->i_number);
}

fsstat(ip, ub)
register struct inode *ip;
struct stat *ub;
{
	register struct dinode *dp;
	register struct buf *bp;
	struct stat ds;

	/*
	 * first copy from inode table
	 */
	ds.st_dev = ip->i_dev;
	ds.st_ino = ip->i_number;
	ds.st_mode = ip->i_mode;
	ds.st_nlink = ip->i_nlink;
	ds.st_uid = ip->i_uid;
	ds.st_gid = ip->i_gid;
	ds.st_rdev = (dev_t)ip->i_un.i_rdev;
	ds.st_size = ip->i_size;
	/*
	 * next the dates in the disk
	 */
	bp = bread(ip->i_dev, itod(ip->i_dev, ip->i_number));
	dp = bp->b_un.b_dino;
	dp += itoo(ip->i_dev, ip->i_number);
	ds.st_atime = dp->di_atime;
	ds.st_mtime = dp->di_mtime;
	ds.st_ctime = dp->di_ctime;
	brelse(bp);
	if (copyout((caddr_t)&ds, (caddr_t)ub, sizeof(ds)) < 0)
		u.u_error = EFAULT;
}

fstrunc(ip)
register struct inode *ip;
{
	register i;
	daddr_t bn;
	struct inode itmp;

	i = ip->i_mode & IFMT;
	if (i!=IFREG && i!=IFDIR && i!=IFLNK)
		return;
	/*
	 * Clean inode on disk before freeing blocks
	 * to insure no duplicates if system crashes.
	 */
	itmp = *ip;
	itmp.i_size = 0;
	for (i = 0; i < NADDR; i++)
		itmp.i_un.i_addr[i] = 0;
	itmp.i_flag |= ICHG|IUPD;
	iupdat(&itmp, &time, &time, 1);
	ip->i_flag &= ~(IUPD|IACC|ICHG);

	/*
	 * Now return blocks to free list... if machine
	 * crashes, they will be harmless MISSING blocks.
	 */
	for(i=NADDR-1; i>=0; i--) {
		bn = ip->i_un.i_addr[i];
		if(bn == (daddr_t)0)
			continue;
		ip->i_un.i_addr[i] = (daddr_t)0;
		switch(i) {

		default:
			free(ip, bn);
			break;

		case NADDR-3:
			tloop(ip, bn, 0, 0);
			break;

		case NADDR-2:
			tloop(ip, bn, 1, 0);
			break;

		case NADDR-1:
			tloop(ip, bn, 1, 1);
		}
	}
	ip->i_size = 0;
	/*
	 * Inode was written and flags updated above.
	 * No need to modify flags here.
	 */
}

tloop(fip, bn, f1, f2)
register struct inode *fip;
daddr_t bn;
{
	register i;
	register struct buf *bp;
	register daddr_t *bap;
	daddr_t nb;

	bp = NULL;
	for(i=NINDIR(fip->i_dev)-1; i>=0; i--) {
		if(bp == NULL) {
			bp = bread(fip->i_dev, bn);
			if (bp->b_flags & B_ERROR) {
				brelse(bp);
				return;
			}
			bap = bp->b_un.b_daddr;
		}
		nb = bap[i];
		if(nb == (daddr_t)0)
			continue;
		if(f1) {
			brelse(bp);
			bp = NULL;
			tloop(fip, nb, f2, 0);
		} else
			free(fip, nb);
	}
	if(bp != NULL)
		brelse(bp);
	free(fip, bn);
}

fsioctl(ip, cmd, cmarg, flag)
register struct inode *ip;
caddr_t cmarg;
{
	int fmt;
	register dev_t dev;

	fmt = ip->i_mode & IFMT;
	if (fmt != IFCHR) {
		if (cmd==FIONREAD && (fmt == IFREG || fmt == IFDIR)) {
			off_t nread = ip->i_size; /* - fp->f_offset */
			if (copyout((caddr_t)&nread, cmarg, sizeof(off_t)))
				u.u_error = EFAULT;
		} else
			u.u_error = ENOTTY;
		return;
	}
	dev = ip->i_un.i_rdev;
	u.u_r.r_val1 = 0;
	(*cdevsw[major(dev)].d_ioctl)(dev, cmd, cmarg, flag);
}

struct inode *
fsopen(ip, rw)
register struct inode *ip;
{
	dev_t dev;
	register unsigned int maj;

	dev = (dev_t)ip->i_un.i_rdev;
	maj = major(dev);

	switch(ip->i_mode&IFMT) {
	case IFCHR:
		if(maj >= nchrdev)
			goto bad;
		if (cdevsw[maj].qinfo)		/* stream device */
			return(stopen(cdevsw[major(dev)].qinfo, dev, rw, ip));
		(*cdevsw[maj].d_open)(dev, rw);
		break;

	case IFBLK:
		if(maj >= nblkdev)
			goto bad;
		(*bdevsw[maj].d_open)(dev, rw, ip);
	}
	return(NULL);

bad:
	u.u_error = ENXIO;
	return(NULL);
}

fsread(ip)
register struct inode *ip;
{
	struct buf *bp;
	dev_t dev;
	daddr_t lbn, bn;
	off_t diff;
	register int on, type;
	register unsigned n;

	dev = (dev_t)ip->i_un.i_rdev;
	if (u.u_offset < 0 && (ip->i_mode&IFMT) != IFCHR) {
		u.u_error = EINVAL;
		return;
	}
	type = ip->i_mode&IFMT;
	if (type==IFCHR) {
		(*cdevsw[major(dev)].d_read)(dev);
		return;
	}
	if (type != IFBLK)
		dev = ip->i_dev;
	do {
		lbn = bn = u.u_offset >> BSHIFT(dev);
		on = u.u_offset & BMASK(dev);
		n = MIN((unsigned)(BSIZE(dev)-on), u.u_count);
		if (type!=IFBLK) {
			diff = ip->i_size - u.u_offset;
			if (diff <= 0)
				return;
			if (diff < n)
				n = diff;
			bn = bmap(ip, bn, B_READ);
			if (u.u_error)
				return;
		} else
			rablock = bn+1;
		if ((long)bn<0) {
			bp = geteblk();
			clrbuf(bp);
		} else if (ip->i_un.i_lastr+1==lbn)
			bp = breada(dev, bn, rablock);
		else
			bp = bread(dev, bn);
		ip->i_un.i_lastr = lbn;
		n = MIN(n, BSIZE(dev)-bp->b_resid);
		if (n!=0) {
#ifndef UNFAST
			iomove(bp->b_un.b_addr+on, n, B_READ);
#else
			if (u.u_segflg != 1) {
				if (copyout(bp->b_un.b_addr+on, u.u_base, n)) {
					u.u_error = EFAULT;
					goto bad;
				}
			} else
				bcopy(bp->b_un.b_addr+on, u.u_base, n);
			u.u_base += n;
			u.u_offset += n;
			u.u_count -= n;
bad:
			;
#endif
		}
		if (n+on==BSIZE(dev) || u.u_offset==ip->i_size)
			bp->b_flags |= B_AGE;
		brelse(bp);
	} while(u.u_error==0 && u.u_count!=0 && n!=0);
}

fswrite(ip)
register struct inode *ip;
{
	struct buf *bp;
	dev_t dev;
	daddr_t bn;
	register int on, type;
	register unsigned n;

	dev = (dev_t)ip->i_un.i_rdev;
	if(u.u_offset < 0 && (ip->i_mode&IFMT) != IFCHR) {
		u.u_error = EINVAL;
		return;
	}
	type = ip->i_mode&IFMT;
	if (type==IFCHR) {
		ip->i_flag |= IUPD|ICHG;
		(*cdevsw[major(dev)].d_write)(dev);
		return;
	}
	if (u.u_count == 0)
		return;
	if ((ip->i_mode&IFMT)==IFREG &&
	    u.u_offset + u.u_count > u.u_limit[LIM_FSIZE]) {
		psignal(u.u_procp, SIGXFSZ);
		u.u_error = EMFILE;
		return;
	}
	if (type != IFBLK)
		dev = ip->i_dev;
	do {
		bn = u.u_offset >> BSHIFT(dev);
		on = u.u_offset & BMASK(dev);
		n = MIN((unsigned)(BSIZE(dev)-on), u.u_count);
		if (type!=IFBLK) {
			bn = bmap(ip, bn, B_WRITE);
			if((long)bn<0)
				return;
		}
		if (bn && mfind(dev, fsbtodb(dev, bn)))
			munhash(dev, fsbtodb(dev, bn));
		if(n == BSIZE(dev)) 
			bp = getblk(dev, bn);
		else
			bp = bread(dev, bn);
#ifndef UNFAST
		iomove(bp->b_un.b_addr+on, n, B_WRITE);
#else
		if (u.u_segflg != 1) {
			if (copyin(u.u_base, bp->b_un.b_addr+on, n)) {
				u.u_error = EFAULT;
				goto bad;
			}
		} else
			bcopy(u.u_base, bp->b_un.b_addr+on, n);
		u.u_base += n;
		u.u_offset += n;
		u.u_count -= n;
bad:
		;
#endif
		if(u.u_error != 0)
			brelse(bp);
		else {
			if ((ip->i_mode&IFMT) == IFDIR &&
			    ((struct direct *)(bp->b_un.b_addr+on))->d_ino == 0)
				/*
				 * Writing to clear a directory entry.
				 * Must insure the write occurs before
				 * the inode is freed, or may end up
				 * pointing at a new (different) file
				 * if inode is quickly allocated again
				 * and system crashes.
				 */
				bwrite(bp);
			else if (n+on==BSIZE(dev)) {
				bp->b_flags |= B_AGE;
				bawrite(bp);
			} else
				bdwrite(bp);
		}
		if(u.u_offset > ip->i_size &&
		   (type==IFDIR || type==IFREG || type==IFLNK))
			ip->i_size = u.u_offset;
		ip->i_flag |= IUPD|ICHG;
	} while(u.u_error==0 && u.u_count!=0);
}
