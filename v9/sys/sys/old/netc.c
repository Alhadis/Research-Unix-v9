#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/inode.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/stat.h"
#include "../h/netc.h"
#include "../h/mount.h"
#include "../h/buf.h"
#include "../h/stream.h"
#include "../h/file.h"
#include "../h/inet/in.h"

extern struct stdata *stenter();
static struct sendc nilx;
#define ND 100
struct {
	char len, s, n;
	struct sendc x[ND];
	struct rcvc y[ND];
} netcbuf = {ND};
#define addx(z) netcbuf.x[netcbuf.n] = *z; netcbuf.s = 1;
#define addy(z) netcbuf.y[netcbuf.n++] = *z; if(netcbuf.n >= ND) netcbuf.n = 0; netcbuf.s = 0;
#define BUFFSIZE 1024	/* Things slow down when the buffer is larg with TCP */

ncmount(sip, ip, flag, mnt, fstyp)
struct inode *ip, *sip;
{
	if (!suser())
		return;
	if (mnt)
		ncdomount(sip, ip, flag, fstyp);
	else
		ncunmount(ip, fstyp);
}

ncdomount(cip, dip, flag, fstyp)
register struct inode *cip, *dip;
{
	struct inode pi;
	register struct inode *rip;

	if(cip->i_sptr == NULL) {
		u.u_error = ENXIO;
		return;
	}
	if((dip->i_mode & IFMT) != IFDIR) {
		u.u_error = ENOTDIR;
		return;
	}
	/*
	 * take care, because of the stupid user-generated device
	 */
	if (dip->i_fstyp == fstyp && dip->i_dev == flag) {
		u.u_error = EBUSY;
		return;
	}
	pi.i_dev = flag;
	pi.i_fstyp = fstyp;
	pi.i_un.i_cip = cip;
	if ((rip = iget(&pi, flag, ROOTINO)) == NULL)
		return;
	if (rip->i_count > 1) {
		iput(rip);
		u.u_error = EBUSY;
		return;
	}
	dip->i_mroot = rip;
	rip->i_mpoint = dip;
	rip->i_un.i_cip = cip;
	prele(rip);
	dip->i_count++;
	dip->i_flag |= IMOUNT;
	cip->i_count++;	
	cip->i_un.i_key = 0;
}

ncunmount(mip, fstyp)
register struct inode *mip;
{
	register struct inode *xip, *rip;
	dev_t dev;

	rip = mip->i_mroot;
	plock(rip);
	dev = rip->i_dev;
	xumount(dev);	/* shared text from remote root */
	if (rip->i_count > 1) {
		u.u_error = EBUSY;
		prele(rip);
		return;
	}
	/* this is silly.  fix it later. */
	for(xip = inode; xip < inodeNINODE; xip++) {
		if (xip == rip)
			continue;
		if(xip->i_number != 0 && xip->i_fstyp == fstyp
			&& major(xip->i_dev) == major(dev)) {
				xumount(xip->i_dev);	/* others, one at a time */
				u.u_error = EBUSY;
				prele(rip);
				return;
			}
	}
	plock(mip);
	mip->i_flag &= ~IMOUNT;
	mip->i_mroot = NULL;
	iput(mip);
	if ((mip = rip->i_un.i_cip) == NULL)
		panic("ncunmount2");
	iput(rip);
	stclose(mip, 1);
	iput(mip);
}

ncput(ip)
struct inode *ip;
{	struct sendc x;
	struct rcvc y;

	if (ip->i_un.i_cip == NULL)
		return;		/* obscure gmount safety */
	if(ip->i_flag & ICHG)
		ncupdat(ip, &time, &time, 0);
	x = nilx;
	x.trannum = trannum++;
	x.cmd = NPUT;
	x.uid = u.u_uid;
	x.tag = ip->i_un.i_tag;
	x.dev = ip->i_dev;
	x.ino = ip->i_number;
	sendc(ip->i_un.i_cip, &x, &y);
	if(y.errno)
		u.u_error = y.errno;
}

/*
 * special case:
 * the root is faked up locally
 * really just to avoid a silly deadlock in the face server
 */
struct inode *
ncget(fip, dev, ino, ip)
struct inode *fip;
struct inode *ip;
{	struct sendc x;
	struct rcvc y;

	ip->i_un.i_cip = NULL;		/* in case we have to put it */
	if(ino == 0) {	/* this must be a disaster of some sort */
		if(!u.u_error)
			u.u_error = EMFILE;	/* just in case for ncnami */
		printf("ncget 0 dev %d\n", ip->i_dev);
		iput(ip);
		return(0);
	}
	if (ino == ROOTINO && minor(dev) == 0) {
		ip->i_mode = IFDIR|0555;	/* fake */
		ip->i_un.i_tag = ((int)dev<<16)|ROOTINO;	/* very fake */
		ip->i_un.i_cip = fip->i_un.i_cip;
		ip->i_nlink = 2;
		ip->i_size = 32;
		ip->i_uid = ip->i_gid = -1;
		return (ip);
	}
	if (fip->i_un.i_cip == NULL) {
		u.u_error = ENXIO;
		iput(ip);
		return (0);
	}
	x = nilx;
	x.trannum = trannum++;
	x.cmd = NGET;
	x.uid = u.u_uid;
	x.dev = dev;
	x.ino = ino;
	sendc(fip->i_un.i_cip, &x, &y);
	if(y.errno == 0) {
		ip->i_mode = y.mode;
		ip->i_un.i_tag = y.tag;
		ip->i_un.i_cip = fip->i_un.i_cip;
		ip->i_nlink = y.nlink;
		ip->i_size = y.size;
		ip->i_uid = y.uid;
		ip->i_gid = y.gid;
		return(ip);
	}
	iput(ip);
	u.u_error = y.errno;
	return(0);
}

ncfree(ip)
struct inode *ip;
{	struct sendc x;
	struct rcvc y;

	if (ip->i_un.i_cip == NULL)
		return;		/* obscure gmount safety */
	x = nilx;
	x.trannum = trannum++;
	x.cmd = NFREE;
	x.uid = u.u_uid;
	x.tag = ip->i_un.i_tag;
	x.dev = ip->i_dev;
	x.ino = ip->i_number;
	sendc(ip->i_un.i_cip, &x, &y);
	if(y.errno)
		u.u_error = y.errno;
}

/* this is used by CREAT */
ncupdat(ip, ta, tm, waitfor)
struct inode *ip;
time_t *ta, *tm;
{	struct sendc x;
	struct rcvc y;

	if (ip->i_un.i_cip == NULL)
		return;		/* obscure gmount safety */
	x = nilx;
	x.trannum = trannum++;
	x.cmd = NUPDAT;
	x.uid = u.u_uid;
	x.gid = u.u_gid;
	x.newuid = ip->i_uid;
	x.newgid = ip->i_gid;
	x.mode = ip->i_mode;
	x.tag = ip->i_un.i_tag;
	x.dev = ip->i_dev;
	x.ino = ip->i_number;
	if(ip->i_flag & IACC)
		x.ta = *ta;
	else
		x.ta = 0;
	if(ip->i_flag & ICHG)
		x.tm = *tm;
	else
		x.tm = 0;
	sendc(ip->i_un.i_cip, &x, &y);
	if(y.errno) {
		u.u_error = y.errno;
		return;
	}
	ip->i_mode = y.mode;
	ip->i_nlink = y.nlink;
	ip->i_size = y.size;
	ip->i_uid = y.uid;
	ip->i_gid = y.gid;
	ip->i_flag &= ~(IUPD|IACC|ICHG);
}

ncread(ip)
struct inode *ip;
{	struct sendc x;
	struct rcvc y;
	struct buf *bp;
	int n;

	bp = geteblk();
	clrbuf(bp);	/* could use user's buffer */
	x = nilx;
	x.trannum = trannum++;
	x.cmd = NREAD;
	x.uid = u.u_uid;
	x.tag = ip->i_un.i_tag;
	x.dev = ip->i_dev;
	x.ino = ip->i_number;
	x.buf = bp->b_un.b_addr;
	do {
		n = u.u_count;
		if(n > BUFFSIZE)
			n = BUFFSIZE;
		x.count = n;
		x.offset = u.u_offset;
		sendc(ip->i_un.i_cip, &x, &y);
		if((n = y.count) > 0) {
			if(u.u_segflg != 1) {
				if(copyout(bp->b_un.b_addr, u.u_base, n)) {
					u.u_error = EFAULT;
					break;
				}
			}
			else
				bcopy(bp->b_un.b_addr, u.u_base, n);
			u.u_base += n;
			u.u_offset += n;
			u.u_count -= n;
		}
		if(y.errno)
			u.u_error = y.errno;
	} while(u.u_error == 0 && u.u_count != 0 && n > 0);
	brelse(bp);
}
ncwrite(ip)
struct inode *ip;
{	struct sendc x;
	struct rcvc y;
	struct buf *bp;
	int n;

	bp = geteblk();
	x = nilx;
	x.trannum = trannum++;
	x.cmd = NWRT;
	x.uid = u.u_uid;
	x.tag = ip->i_un.i_tag;
	x.dev = ip->i_dev;
	x.ino = ip->i_number;
	x.buf = bp->b_un.b_addr;
	do {
		n = u.u_count;
		if(n > BUFFSIZE)		/* should be bufsiz, but ... */
			n = BUFFSIZE;
		if(u.u_segflg != 1) {
			if(copyin(u.u_base, bp->b_un.b_addr, n)) {
				u.u_error = EFAULT;
				break;
			}
		}
		else
			bcopy(u.u_base, bp->b_un.b_addr, n);
		x.count = n;
		x.offset = u.u_offset;
		sendc(ip->i_un.i_cip, &x, &y);
		if(y.errno) {
			u.u_error = y.errno;
			break;
		}
		ip->i_flag |= IUPD|ICHG;
		u.u_count -= n;
		u.u_offset += n;
		u.u_base += n;
	} while(u.u_error == 0 && u.u_count != 0);
	brelse(bp);
}

nctrunc(ip)
struct inode *ip;
{	struct sendc x;
	struct rcvc y;

	x = nilx;
	x.trannum = trannum++;
	x.cmd = NTRUNC;
	x.uid = u.u_uid;
	x.tag = ip->i_un.i_tag;
	x.dev = ip->i_dev;
	x.ino = ip->i_number;
	sendc(ip->i_un.i_cip, &x, &y);
	if(y.errno)
		u.u_error = y.errno;
}

ncstat(ip, ub)
struct inode *ip;
struct stat *ub;
{	struct sendc x;
	struct rcvc y;
	struct stat ds;

	x = nilx;
	x.trannum = trannum++;
	x.cmd = NSTAT;
	x.uid = u.u_uid;
	x.tag = ip->i_un.i_tag;
	x.dev = ip->i_dev;
	x.ino = ip->i_number;
	x.ta = time;
	sendc(ip->i_un.i_cip, &x, &y);
	if(y.errno) {
		u.u_error = y.errno;
		return;
	}
	ds.st_dev = ip->i_dev;
	ds.st_ino = ip->i_number;
	ip->i_mode = ds.st_mode = y.mode;
	ip->i_nlink = ds.st_nlink = y.nlink;
	ip->i_uid = ds.st_uid = y.uid;
	ip->i_gid = ds.st_gid = y.gid;
	ds.st_size = ip->i_size = y.size;
	ds.st_atime = y.tm[0];
	ds.st_mtime = y.tm[1];
	ds.st_ctime = y.tm[2];
	if(copyout((caddr_t)&ds, (caddr_t)ub, sizeof(ds)) < 0)
		u.u_error = EFAULT;
}

/* a lot like fsnami */
ncnami(p, pflagp, follow)
struct nx *p;
struct argnamei **pflagp;
{	register struct inode *dp, *dip;
	register char *cp;
	int i, fstyp;
	dev_t d;
	struct argnamei *flagp = *pflagp;
	struct sendc x;
	struct rcvc y;

	cp = p->cp;
	dp = p->dp;
	x = nilx;
	x.cmd = NNAMI;
	x.uid = u.u_uid;
	x.gid = u.u_gid;

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

	/* send off the request, get back ino, dev, flagp stuff */
	x.trannum = trannum++;
	x.tag = dp->i_un.i_tag;
	x.dev = dp->i_dev;
	x.ino = dp->i_number;
	x.count = DIRSIZ;
	x.buf = u.u_dbuf;
	while(*cp == '/')
		cp++;
	if(flagp && *cp=='\0') {
		switch(flagp->flag) {
		case NI_DEL:
			x.flags = NDEL;
			break;
		case NI_LINK:
			x.flags = NLINK;
			x.dev = flagp->il->i_dev;
			x.ino = flagp->il->i_number;
			break;
		case NI_CREAT:
		case NI_NXCREAT:
			x.flags = NCREAT;
			x.mode = flagp->mode;
			break;
		case NI_MKDIR:
			x.flags = NMKDIR;
			x.mode = flagp->mode;
			break;
		case NI_RMDIR:
			x.flags = NRMDIR;
			break;
		default:
			u.u_error = ENXIO;
			goto outnull;
		}
	}
	sendc(dp->i_un.i_cip, &x, &y);
	if(y.errno) {
		u.u_error = y.errno;
		goto outnull;
	}
	if(y.flags == NOMATCH)
		goto nomatch;
	u.u_dent.d_ino = y.ino;
	if (y.ino == 0) {
		printf("y.ino 0 in ncnami\n");
		u.u_error = ENOENT;
		goto outnull;
	}
	d = y.dev;

	if(*cp == 0 && flagp
	 && (flagp->flag==NI_DEL || flagp->flag==NI_RMDIR)) {
		/* delete the entry, server did all but xrele */				dip = ifind(dp, u.u_dent.d_ino);
		if(dip == NULL)
			goto outnull;
		dip = iget(dp, d, u.u_dent.d_ino);	/* to lock it */
		if(dip->i_flag&ITEXT)
			xrele(dip);		/* free busy text */
		dip->i_nlink--;
		dip->i_flag |= ICHG;
		iput(dip);
		goto outnull;
	}
	/*
	 * Special handling for ".."
	 */
	fstyp = dp->i_fstyp;

	if(y.flags == NROOT) {	/* popped out of net fs */
		if (dp == u.u_rdir)
			u.u_dent.d_ino = dp->i_number;
		else if (dp != rootdir && u.u_dent.d_ino==ROOTINO
		     &&  dp->i_number == ROOTINO) {
			if ((dip = dp->i_mpoint) == NULL)
				panic("namei: mpoint");
			iput(dp);
			dp = dip;
			plock(dp);
			dp->i_count++;
			/* was there always .. there? */
			if(*cp && cp < p->nbp->b_un.b_addr + 3)
				panic("ncnami");
			if(!*cp && cp < p->nbp->b_un.b_addr + 2)
				panic("ncnami2");
			if(*cp)
				*--cp = '/';
			*--cp = '.';
			*--cp = '.';
			if(fstyp != dp->i_fstyp)
				goto more;
			goto dirloop;
		}
	}
	dip = dp;
	prele(dip);
	dp = iget(dip, d, u.u_dent.d_ino);
	if(dp == NULL) {
		iput(dip);
		goto outOK;
	}
	if(fstyp != dp->i_fstyp) {
		iput(dip);
		goto more;
	}
	/*
	 * Check for symbolic link
	 */
	if ((dp->i_mode&IFMT)==IFLNK && (follow || *cp)) {
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
		bcopy(ocp, p->nbp->b_un.b_addr+dp->i_size + 1, cp-ocp);
		*(p->nbp->b_un.b_addr + dp->i_size) = '/';
		u.u_base = p->nbp->b_un.b_addr;
		u.u_count = dp->i_size;
		u.u_offset = 0;
		readi(dp);
		if(u.u_error) {
			iput(dip);
			goto outnull;
		}
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
		if(fstyp != dp->i_fstyp)
			goto more;
		goto dirloop;
	}
	iput(dip);
	if(*cp)
		goto dirloop;
	goto outOK;
nomatch:
	/*
	 * Search failed.
	 */
	if(flagp) {		/* probably creating a new file */
		switch(flagp->flag) {
		case NI_LINK:	/* make a link */
		case NI_MKDIR:	/* make directory */
			goto outnull;
		case NI_CREAT:	/* create a new file */
		case NI_NXCREAT:
			if(y.errno == EACCES || y.ino == 0) {
				u.u_error = EACCES;
				goto outnull;
			}
			dip = iget(dp, y.dev, y.ino);
			if(dip == NULL)
				goto outnull;
			iput(dp);
			dp = dip;
			flagp->done = dip->i_number;
			goto outOK;
		}
	}
	u.u_error = ENOENT;
outnull:
	p->dp = dp;
	return(1);
outOK:
	p->dp = dp;
	return(0);
more:
	p->dp = dp;
	p->cp = cp;
	return(2);
}

static struct {
	int tran;
	short proc, dev;
} ntx[32];	/* more debuggery */
sendc(cip, x, y)
struct inode *cip;
struct sendc *x;
struct rcvc *y;
{	int n, tn, ix;
	struct sendc xx;
	x->version = NETVERSION;
	/* until demux works, use key as a lock */
	while(cip->i_un.i_key)
		sleep((caddr_t)cip, PZERO);
	cip->i_un.i_key = 1;
	tn = x->trannum;
	netcbuf.s = 2;
	for(ix = 0; ntx[ix].tran && ix < 32; ix++) {
		ntx[ix].tran = tn;
		ntx[ix].proc = u.u_procp->p_pid;
		ntx[ix].dev = x->dev;
	}
	xx.version = x->version;
	xx.cmd = x->cmd;
	xx.flags = x->flags;
	xx.rsvd = x->rsvd;
	xx.trannum = htonl(x->trannum);
	xx.uid = htons(x->uid);
	xx.gid = htons(x->gid);
	xx.dev = htons(x->dev);
	xx.tag = htonl(x->tag);
	xx.mode = htonl(x->mode);
	xx.newuid = htons(x->newuid);
	xx.newgid = htons(x->newgid);
	xx.ino = htonl(x->ino);
	xx.count = htonl(x->count);
	xx.offset = htonl(x->offset);
	xx.buf = (char *)htonl((long)x->buf);
	xx.ta = htonl(x->ta);
	xx.tm = htonl(x->tm);
	n = istwrite(cip, (char *)&xx, sizeof(xx));
	netcbuf.s = 3;
	if(n == -1) {
		y->errno = EIO;
		goto bad;
	}
	netcbuf.s = 4;
	if(x->count > 0 && x->buf && x->cmd != NREAD) {
		n = istwrite(cip, x->buf, x->count);
		if(n == -1)
			goto bad;
	}
readagain:
	netcbuf.s = 5;
	n = istread(cip, (char *)y, sizeof(*y), 0);
	netcbuf.s = 6;
	if(n != sizeof(*y))
		goto bad;
	y->trannum = ntohl(y->trannum);
	y->dev = ntohs(y->dev);
	y->size = ntohl(y->size);
	y->mode = ntohs(y->mode);
	y->uid = ntohs(y->uid);
	y->gid = ntohs(y->gid);
	y->tag = ntohl(y->tag);
	y->nlink = ntohs(y->nlink);
	y->rsvd = ntohs(y->rsvd);
	y->ino = ntohl(y->ino);
	y->count = ntohl(y->count);
	y->tm[0] = ntohl(y->tm[0]);
	y->tm[1] = ntohl(y->tm[1]);
	y->tm[2] = ntohl(y->tm[2]);
	if(y->errno == 0 && x->cmd == NREAD) {
		n = istread(cip, x->buf, y->count, 0);
		if(n != y->count) {
			printf("netc: read %d expected %d\n", n, y->count);
			/* shut it down */
			istwrite(cip, (char *)x, 0);
			goto bad;
		}
	}
	if(y->errno == 0) {
		if(y->trannum != tn) {
			if(y->trannum < tn)	/* distant past */
				goto readagain;
			printf("netc: sent %d got %d\n", tn, y->trannum);
			goto bad;
		}
		addx(x);
		addy(y);
		if(ix < 32)
			ntx[ix].tran = 0;
		cip->i_un.i_key = 0;
		wakeup((caddr_t)cip);
		return;
	}
bad:
	cip->i_un.i_key = 0;
	if(ix < 32)
		ntx[ix].tran = 0;
	for(ix = 0; ix < 32; ix++)
		if(ntx[ix].tran)
			printf("tran %d proc %d 0x%x\n", ntx[ix].tran, ntx[ix].proc, ntx[ix].dev);
	wakeup((caddr_t)cip);
	addx(x);
	addy(y);
	if(y->errno)
		return;
	y->errno = EIO;
}
static struct D { struct D *a; char *b;} VER = {&VER,"\n85/6/9:netc.c\n"};
