/* new version for netfs */
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/netb.h"
#include "../h/inode.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/stat.h"
#include "../h/mount.h"
#include "../h/file.h"

extern long trannum;
struct sendb nilsendb = {NETB};
char *getnbbuf();
#define BSZ	(5*1024)	/* probably enough */
/* cache to avoid stat messages immediately after nami.  The cache is not
 * invalidated by iput (unless it is the cached item) because nami iput's
 * the directory before returning.  The cache is critical for the iget
 * inside nami.  Nbnami fills nbcache for iget to read, and there is a
 * race condition since nami does iput before the iget. */
struct nbcache {
	int valid;
	dev_t dev;
	ino_t ino;	/* although we get long back from server */
	long tag;
	short mode;
	short nlink;
	short uid, gid;
	dev_t rdev;	/* does anyone care? */
	long size;
	time_t ta, tm, tc;
	struct inode *cip;	/* the communications handle */
} nbcache;

/* called indirectly through iget on behalf of nami */
/* or else it is called for the root inode */
struct inode *
nbget(fip, dev, ino, ip)
struct inode *fip;
dev_t dev;
ino_t ino;
register struct inode *ip;
{

	if(minor(dev) == 0 && ino == ROOTINO) {	/* construct root */
		ip->i_un.i_cip = fip->i_un.i_cip;
		ip->i_dev = fip->i_dev;
		ip->i_un.i_tag = (ip->i_dev<<16)|ROOTINO;	/* server better understand */
		ip->i_mode = S_IFDIR | 0555;	/* perms corrected by stat */
		ip->i_uid = 1;	/* ditto */
		ip->i_gid = 1;	/* ditto */
		ip->i_nlink = 2;	/* ditto */
		ip->i_size = 512;	/* ditto */
		return(ip);
	}
	if(nbcache.valid == 0) {
		printf("nbget: nbcache invalid\n");
		u.u_error = ENOENT;
		return(0);
	}
	if(nbcache.dev != dev) {
		printf("nbget: dev invalid 0x%x 0x%x\n", nbcache.dev, dev);
		u.u_error = ENOENT;
		return(0);
	}
	/* in this error, should the ino be freed? */
	if(nbcache.ino != ino) {
		printf("nbget: ino invalid 0x%x 0x%x\n", nbcache.ino, ino);
		u.u_error = ENOENT;
		return(0);
	}
	ip->i_un.i_cip = nbcache.cip;
	ip->i_dev = dev;
	ip->i_mode = nbcache.mode;
	ip->i_un.i_tag = nbcache.tag;
	ip->i_uid = nbcache.uid;
	ip->i_gid = nbcache.gid;
	ip->i_nlink = nbcache.nlink;
	ip->i_size = nbcache.size;
	return(ip);
}
nbfree()
{	/* that's easy (after free, cache can't be looked at for that
		inode without another nami) */
}

nbput(ip)
struct inode *ip;
{	struct sendb x;
	struct recvb y;
	x = nilsendb;
	/* should check to see if nbcache is being tossed */

	/* don't put root, as optimization */
	if(ip->i_number == ROOTINO && minor(ip->i_dev) == 0)
		return;
	x.cmd = NBPUT;
	x.len = sizeof(struct sendb);
	x.tag = ip->i_un.i_tag;
	nbsend(ip, &x, &y, sizeof(y));
}

nbtrunc(ip)
struct inode *ip;
{	struct sendb x;
	struct recvb y;
	/* we might be truncing the cached inode */
	x = nilsendb;
	x.cmd = NBTRNC;
	x.len = sizeof(struct sendb);
	x.tag = ip->i_un.i_tag;
	nbsend(ip, &x, &y, sizeof(y));
}

/* the only times we're interested in are those which have waitfor == 0
 * so fix it! (or those which have ta or tb != &time)
 */
nbupdat(ip, ta, tb, waitfor)
time_t *ta, *tb;
struct inode *ip;
{	struct {
		struct sendb x;
		struct snbup b;
	} a;
	struct recvb y;
	/* what about the cache? */

	a.x = nilsendb;
	a.x.cmd = NBUPD;
	a.x.len = sizeof(a);
	a.x.tag = ip->i_un.i_tag;
	a.b.uid = ip->i_uid;
	a.b.gid = ip->i_gid;
	a.b.mode = ip->i_mode;
	if((ta != &time) && (ip->i_flag & IACC))
		a.b.ta = *ta;
	else
		a.b.ta = 0;
	if((tb != &time) && (ip->i_flag & IUPD))
		a.b.tm = *tb;
	else
		a.b.tm = 0;
	nbsend(ip, (struct sendb *) &a, &y,sizeof(y));
	ip->i_flag &= ~(IUPD|IACC|ICHG);
}

nbstat(ip, ub)
struct inode *ip;
struct stat *ub;
{	struct {
		struct sendb x;
		struct snbstat s;
	} a;
	struct {
		struct recvb y;
		struct rnbstat s;
	} b;
	struct stat ds;
	if(nbcache.valid &&	/* always, except for fstat() and root */
		nbcache.cip == ip->i_un.i_cip && nbcache.tag == ip->i_un.i_tag) {
		/* this first batch should already be in the inode */
		ds.st_dev = nbcache.dev;
		ds.st_ino = nbcache.ino;
		ds.st_mode = nbcache.mode;
		ds.st_nlink = nbcache.nlink;
		ds.st_uid = nbcache.uid;
		ds.st_gid = nbcache.gid;
		/* new stuff */
		ds.st_size = nbcache.size;
		ds.st_atime = nbcache.ta;
		ds.st_mtime = nbcache.tm;
		ds.st_ctime = nbcache.tc;
		goto outit;
	}
	a.x = nilsendb;
	a.x.trannum = trannum++;
	a.x.cmd = NBSTAT;
	a.x.len = sizeof(a);
	a.x.tag = ip->i_un.i_tag;
	a.s.ta = time;
	nbsend(ip, (struct sendb *)&a, (struct recvb *)&b, sizeof(b));
	if(u.u_error)
		return;
	/* unpack the returned stuff */
	ds.st_dev = ip->i_dev;
	ds.st_ino = ip->i_number;
	ds.st_mode = ip->i_mode = b.s.mode;
	ds.st_nlink = ip->i_nlink = b.s.nlink;
	ds.st_uid = ip->i_uid = b.s.uid;
	ds.st_gid = ip->i_gid = b.s.gid;
	ds.st_size = (ip->i_size = b.s.size);
	ds.st_atime = b.s.ta;
	ds.st_mtime = b.s.tm;
	ds.st_ctime = b.s.tc;
outit:
	if(copyout((caddr_t)&ds, (caddr_t)ub, sizeof(ds)))
		u.u_error = EFAULT;
}

nbread(ip)
struct inode *ip;
{	struct {
		struct sendb x;
		struct snbread r;
	} a;
	struct recvb *y;
	char *bp;
	int n;
	/* if this is the cached item, some time is now wrong */
	bp = getnbbuf();
	a.x = nilsendb;
	a.x.cmd = NBREAD;
	a.x.tag = ip->i_un.i_tag;
	a.x.len = sizeof(a);
	do {
		n = u.u_count;
		if(n > BSZ - sizeof(*y))
			n = BSZ - sizeof(*y);
		a.r.offset = u.u_offset;
		a.r.len = n;
		nbsend(ip, (struct sendb *)&a, (struct recvb *)bp, sizeof(*y));
		y = (struct recvb *)(bp);
		n = y->len - sizeof(*y);
		if(n > 0) {	/* give it to user */
			if(u.u_segflg != 1) {
				if(copyout(bp + sizeof(*y), u.u_base, n)) {
					u.u_error = EFAULT;
					break;
				}
			}
			else
				bcopy(bp + sizeof(*y), u.u_base, n);
			u.u_base += n;
			u.u_offset += n;
			u.u_count -= n;
		}
		if(y->errno)
			u.u_error = y->errno;
	} while(u.u_error == 0 && u.u_count > 0 && n > 0);
	putnbbuf(bp);
}

nbwrite(ip)
struct inode *ip;
{	struct xx {
		struct sendb x;
		struct snbwrite w;
	} *a;
	struct recvb y;
	int n;
	char *bp;
	/* if this is the cached item, it's not quite right any more*/
	bp = getnbbuf();
	a = (struct xx *)(bp);
	a->x = nilsendb;
	a->x.cmd = NBWRT;
	a->x.tag = ip->i_un.i_tag;
	do {
		n = u.u_count;
		if(n > BSZ - sizeof(*a))
			n = BSZ - sizeof(*a);
		a->x.len = n + sizeof(*a);
		a->w.len = n;
		a->w.offset = u.u_offset;
		if(u.u_segflg != 1) {
			if(copyin(u.u_base, bp + sizeof(*a), n)) {
				u.u_error = EFAULT;
				break;
			}
		}
		else
			bcopy(u.u_base, bp + sizeof(*a), n);
		nbsend(ip, (struct sendb *)a, &y, sizeof(y));
		if(y.errno) {
			u.u_error = y.errno;
			break;
		}
		ip->i_flag |= IUPD|ICHG;
		u.u_count -= n;
		u.u_offset += n;
		u.u_base += n;
	} while(u.u_error == 0 && u.u_count > 0);
	putnbbuf(bp);
}

/* send the whole path over.  this fails if another file system is mounted
 *	on this one, or if someone has chrooted into this one */
nbnami(p, pflagp, follow)
struct nx *p;
struct argnamei **pflagp;
{	char *bp, *s;
	struct inode *dp;
	struct aa {
		struct sendb x;
		struct snbnami n;
	} *a;
	struct bb {
		struct recvb y;
		struct rnbnami n;
	} *b;
	register int i;

	for(i = 0, s = p->cp; *s; s++)
		i++;
	if(i + sizeof(*a) >= BSZ || i + sizeof(*b) >= BSZ) {
		u.u_error = ENOENT;
		goto outnull;
	}
/*printf("nbnami %s\n", p->cp);*/
	dp = p->dp;
	bp = getnbbuf();
	a = (struct aa *)(bp);
	a->x = nilsendb;
	a->x.cmd = NBNAMI;
	a->x.len = i + sizeof(*a);
	a->x.tag = dp->i_un.i_tag;
	a->n.uid = u.u_uid;
	a->n.gid = u.u_gid;
	if(*pflagp) {
		a->x.flags = (*pflagp)->flag;
		a->n.mode = (*pflagp)->mode;
		if((*pflagp)->il)
			a->n.ino = (*pflagp)->il->i_un.i_tag;	/* fix the struct? */
		else
			a->n.ino = 0;
	}
	for(s = bp + sizeof(*a), i = 0; p->cp[i]; i++)
		*s++ = p->cp[i];
	*s = 0;
	b = (struct bb *) (bp);
	nbsend(dp, (struct sendb *)a, (struct recvb *)b, sizeof(*b));
	if(u.u_error)
		goto outnull;
	if(b->y.flags == NBROOT) {
		/* find my root */
		dp = dp->i_mpoint;
		iput(p->dp);
		plock(dp);
		dp->i_count++;
		if(!dp) {
			printf("nbnami lost root %s\n", p->cp);
			u.u_error = EGREG;
			goto outnull;
		}
/*printf("NBROOT: used %d %s dp #%x ino %d fstyp %d\n", b->n.used, p->cp, dp, dp->i_number, dp->i_fstyp);*/
		p->cp += b->n.used;
		/* now we have to prepend the .. */
		if(b->n.used < 2) {
			u.u_error = EGREG;
			goto outnull;
		}
		*--p->cp == '.';
		*--p->cp == '.';
/*printf("and %s\n", p->cp);*/
		goto more;
	}
	if(*pflagp == 0)
		goto saveit;
	switch((*pflagp)->flag) {
	default:
		printf("nbnami switch %c\n", (*pflagp)->flag);
		goto outnull;
	case NI_CREAT:	/* only case where pflagp is used later (creat()) */
	case NI_NXCREAT:	/* return an inode, like search */
		(*pflagp)->done = b->n.ino;	/* fix the struct? */
		goto saveit;
	case NI_DEL:	/* nothing returned */
	case NI_LINK:	/* nothing returned */
	case NI_MKDIR:	/* returns nothing */
	case NI_RMDIR:	/* nothing returned */
		goto outnull;
	}
saveit:	/* release p->dp, construct a locked inode, and cache for stat */
	/* first the cache (should there be more checking?) */
	nbcache.valid = 1;
	nbcache.tag = b->n.tag;
	nbcache.ino = b->n.ino;
	nbcache.dev = b->n.dev;
	nbcache.mode = b->n.mode;
	nbcache.nlink = b->n.nlink;
	nbcache.uid = b->n.uid;
	nbcache.gid = b->n.gid;
	nbcache.rdev = b->n.rdev;
	nbcache.size = b->n.size;
	nbcache.ta = b->n.ta;
	nbcache.tm = b->n.tm;
	nbcache.tc = b->n.tc;
	nbcache.cip = p->dp->i_un.i_cip;
	/* if that was the root, uncache it, for otherwise a lost conenction
	 * would go undetected.
	 */
	if(minor(nbcache.dev) == 0 && nbcache.ino == ROOTINO)
		nbcache.valid = 0;
	/* release old, construct new */
	prele(p->dp);
	dp = iget(p->dp, b->n.dev, b->n.ino);
	iput(p->dp);
	goto outgood;
outnull:	/* nami is not going to return an inode */
	p->dp = dp;
	putnbbuf(bp);
	return(1);
outgood:
	p->dp = dp;
	putnbbuf(bp);
	return(0);
more:	/* popped out, try some more */
	p->dp = dp;
	putnbbuf(bp);
	return(2);
}

nbmount(cip, dip, flag, mnt, fstyp)
struct inode *cip, *dip;
{
	if (mnt)
		nbon(cip, dip, flag, fstyp);
	else
		nboff(dip, fstyp);
}

nbon(cip, dip, flag, fstyp)
register struct inode *cip, *dip;
{
	struct inode pi;
	register struct inode *rip;

	if(cip->i_sptr == NULL) {	/* shouting into the storm */
		u.u_error = ENXIO;
		return;
	}
	if((dip->i_mode & IFMT) != IFDIR) {
		u.u_error = ENOTDIR;
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

#define	ERRFS	5

/* mip should be the guy mounted on */
nboff(mip, fstyp)
register struct inode *mip;
{
	register struct inode *xip, *rip;
	dev_t dev;

	rip = mip->i_mroot;
	if(!rip)
		panic("!rip in nboff\n");
	if(rip->i_fstyp != fstyp)
		panic("netb, wrong fstyp");		
	plock(rip);
	dev = rip->i_dev;
	xumount(dev);	/* remove sticky texts (independent of fstyp) */
	/* scan all the inodes, looking for us */
	for(xip = inode; xip < inodeNINODE; xip++) {
		if (xip == rip)
			continue;
		if(xip->i_number == 0 || xip->i_fstyp != fstyp || major(xip->i_dev) != major(dev))
			continue;
		xumount(xip->i_dev);	/* does this ever help? */
		/* let's use the error fs, and hope ip gets put someday */
		xip->i_fstyp = ERRFS;
	}
	if(u.u_error) {
		prele(rip);
		return;
	}
	mip->i_flag &= ~IMOUNT;
	mip->i_mroot = NULL;
	iput(mip);
	if (rip->i_un.i_cip == NULL)
		panic("naunmount2");
	iput(rip);
	stclose(rip->i_un.i_cip, 1);	/* does that iput it too? */
}

nbsend(ip, s, r, hdrsize)
struct inode *ip;
struct sendb *s;
struct recvb *r;
{	int n, large, tn, cnt;
	char *p;
	struct inode *cip = ip->i_un.i_cip;
	/* use key as lock */
	while(ip->i_fstyp != ERRFS && cip->i_un.i_key)
		sleep((caddr_t)cip, PZERO);
	if(ip->i_fstyp == ERRFS) {	/* it was unmounted under us */
		u.u_error = EIO;
		return;
	}
	cip->i_un.i_key = 1;

	u.u_error = 0;
	tn = s->trannum = trannum++;
	n = istwrite(cip, (char *)s, s->len);
	if(n < 0) {
		r->errno = EIO;
		goto rotten;
	}
	switch(s->cmd) {
	default:
		printf("nbsend cmd %d\n", s->cmd);
		goto rotten;
	case NBSTAT: case NBTRNC: case NBUPD: case NBPUT: case NBWRT: case NBNAMI:
		large = hdrsize;
		break;
	case NBREAD:
		large = BSZ;
	}
readagain:
	p = (char *)r;
	cnt = 0;
readmore:
	n = istread(cip, p, large);
	if(n <= 0)
		goto dying;
	cnt += n;
	p += n;
	if(cnt < sizeof(struct recvb))
		goto readmore;
	if(cnt < r->len) {
		if(!r->errno)
			goto readmore;
dying:
		if(!r->errno)
			r->errno = EIO;
		if(n >= 0)
			istwrite(cip, s, 0);	/* shut it off */
		goto rotten;
	}
	if(cnt != r->len) {	/* could we get too much? */
		printf("netb read %d, was sent %d\n", cnt, r->len);
		goto dying;
	}
	if(tn != r->trannum) {
		printf("netb got tran %d for %d\n", tn, r->trannum);
		if(r->trannum < tn)
			goto readagain;	/* distant past? */
		istwrite(cip, s, 0);	/* crunch */
		goto rotten;
	}
	if(cip->i_un.i_key) {
		cip->i_un.i_key = 0;
		wakeup((caddr_t)cip);
	}
	if(r->errno)
		u.u_error = r->errno;
	return;
rotten:
	cip->i_un.i_key = 0;
	wakeup((caddr_t)cip);
	if(r->errno)
		u.u_error = r->errno;
	else
		u.u_error = EIO;
	/* and we could unmount the file system here too, if we could find the root */
}

/* oh no, it's got its own tiny buffer pool: want to avoid breakage on
	normal (4k reads for now).  we could replace this with a real
	memory manager in the kernel */
#define NBUF	5	/* who knows? */
int nb_lock;
int nbflgs[NBUF];
char nbbuf[NBUF][BSZ];

char *
getnbbuf()
{	int i;
loop:
	for(i = 0; i < NBUF; i++)
		if(!nbflgs[i]) {
			nbflgs[i] = 1;
			return(nbbuf[i]);
		}
	nb_lock = 1;
	sleep((caddr_t)&nb_lock, PZERO);
	goto loop;
}

putnbbuf(s)
char *s;
{	int i;
	for(i = 0; i < NBUF; i++)
		if(nbbuf[i] == s) {
			nbflgs[i] = 0;
			if(nb_lock) {
				nb_lock = 0;
				wakeup((caddr_t) &nb_lock);
			}
			return;
		}
	/* this is a bad deal */
	printf("putnbbuf: s 0x%x BSZ 0x%x nbbuf 0x%x\n", s, BSZ, nbbuf);
}
