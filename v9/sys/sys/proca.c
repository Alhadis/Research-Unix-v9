#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/inode.h"
#include "../h/user.h"
/*#include "../h/psl.h"*/
#include "../h/stat.h"
#include "../h/buf.h"
#include "../h/mount.h"
#include "../machine/pte.h"
#include "../h/cmap.h"
/*#include "../h/mtpr.h"*/
#include "../h/vmparam.h"
#include "../h/vmmac.h"
#include "../h/proc.h"
#include "../h/text.h"
#include "../h/file.h"
/*#include "../h/reg.h"*/
#include "../h/pioctl.h"
#include "../machine/psl.h"

/* I/O via /proc returns EBUSY if any of these bits are set in p_flag: */

#define	SPRBUSY	(SLOCK|SPAGE|SKEEP|SWEXIT|SVFORK|SVFDONE|SNOVM|SPROCIO)

#ifdef vax
#define SYSADR	((caddr_t)0x80000000)	/* virtual address  of system seg. */
#define SYSP	btop(0x80000000)	/* virtual page no. of system seg. */
#endif
#ifdef sun
#define SYSADR	((caddr_t)USRSTACK)	/* virtual address  of system seg. */
#define SYSP	btop(USRSTACK)		/* virtual page no. of system seg. */
#endif

#define TEXT		0
#define DATA		1
#define STACK		2
#define USERAREA	3

#define min(a,b)	((a) <= (b) ? (a) : (b))

/* inumber to pid */
#define PRMAGIC 64

#define PROCDEV (dev_t)0

struct proc *prpidlock, *prpidwant, *pfind();

extern struct pte Prbufmap[]; extern char priobuf[];
struct pte *prclmap();

prput(ip)
struct inode *ip;
{	struct proc *p;
	if(ip->i_number == ROOTINO)
		return;
	p = pfind(ip->i_number - PRMAGIC);
	if(p == 0) {
		/* printf("prput(%d)?\n", ip->i_number - PRMAGIC); */
		return;
	}
	p->p_trace = 0;
}

struct inode *
prget(fip, dev, ino, ip)
register struct inode *fip;
register struct inode *ip;
{
	register struct proc *p;
	if(ino == ROOTINO) {	/* fake up an inode */
		ip->i_mode = IFDIR | 0555;	/* dir, read and search */
		ip->i_nlink = 2;		/* complete fake */
		ip->i_uid = ip->i_gid = 0;
		ip->i_size = (nproc + 2) * sizeof(struct direct);
		return(ip);
	}
	p = pfind(ino - PRMAGIC);
	if (p == 0 || p->p_trace) {
		iput(ip);
		u.u_error = ENOENT;
		return(0);
	}
	p->p_trace = ip;
	if (p->p_textp && p->p_textp->x_iptr && access(p->p_textp->x_iptr, IREAD))
		ip->i_mode = IFREG;		/* regular, no permissions */
	else
		ip->i_mode = IFREG | 0600;	/* regular, r/w only by owner */
	ip->i_nlink = 1;
	ip->i_uid = p->p_uid;
	ip->i_gid = 1;			/* who cares */
	ip->i_size = (int)ptob(p->p_tsize+p->p_dsize+p->p_ssize+UPAGES);
	ip->i_un.i_proc = p;		/* the sanity check */
	ip->i_un.i_sigmask = 0;		/* signal trace mask */
	return(ip);
}

prfree(ip)
struct inode *ip;
{}

prupdat(ip, ta, tm, waitfor)
struct inode *ip;
time_t *ta, *tm;
{}

#define SDSIZ	sizeof(struct direct)

prread(ip)
struct inode *ip;
{
	static struct direct dotbuf[] = {
		{ ROOTINO, "."},
		{ ROOTINO, ".."}
	};
	struct direct dirbuf;
	register int i, n, j;
	int minproc, maxproc, modoff;
	struct proc *p;

	if (ip->i_number == ROOTINO) {	/* fake up . .. and the proc inodes */
		if (u.u_offset < 0 || u.u_offset >= ip->i_size ||
		    u.u_count <= 0)
			return;
		if (u.u_offset < 2*SDSIZ) {
			iomove((caddr_t)dotbuf + u.u_offset,
			    min(u.u_count, 2*SDSIZ - u.u_offset), B_READ);
			if (u.u_count <= 0 || u.u_error)
				return;
		}
		minproc = (u.u_offset - 2*SDSIZ)/SDSIZ;
		maxproc = (u.u_offset + u.u_count - 1)/SDSIZ;
		modoff = u.u_offset % SDSIZ;
		for (j = 0; j < DIRSIZ; j++)
			dirbuf.d_name[j] = 0;
		for (i=minproc; i<min(maxproc,nproc); i++) {
			if (n = proc[i].p_pid) {
				dirbuf.d_ino = n + PRMAGIC;
				for (j = 4; j >= 0; j--)
					dirbuf.d_name[j] = n%10 + '0', n /= 10;
			} else {
				dirbuf.d_ino = 0;
			}
			iomove((caddr_t)&dirbuf + modoff,
			    min(u.u_count, SDSIZ - modoff), B_READ);
			if (u.u_count <= 0 || u.u_error)
				return;
			modoff = 0;
		}
	} else if (p = pfind(ip->i_number - PRMAGIC)) {
		if (prlock(p))
			return;
		prusrio(p, B_READ);
		prunlock(p);
	} else {
		u.u_error = ENOENT;
	}
}

prwrite(ip)
struct inode *ip;
{
	register struct proc *p;
	if (ip->i_number == ROOTINO) {
		u.u_error = EISDIR;
	} else if (p = pfind(ip->i_number - PRMAGIC)) {
		if (prlock(p))
			return;
		prusrio(p, B_WRITE);
		prunlock(p);
	} else {
		u.u_error = ENOENT;
		return;
	}
}

prtrunc(ip)
struct inode *ip;
{}

prstat(ip, ub)
struct inode *ip;
struct stat *ub;
{	struct stat ds; struct proc *p = (struct proc *)0;
	if(ip->i_number == ROOTINO || (p=pfind(ip->i_number - PRMAGIC))) {
		ds.st_dev = -1;		/* who knows */
		ds.st_ino = ip->i_number;
		ds.st_mode = ip->i_mode;
		ds.st_nlink = ip->i_nlink;
		ds.st_uid = ip->i_uid;
		ds.st_gid = ip->i_gid;
		ds.st_rdev = -1;
		if (p) ip->i_size =
			(int)ptob(p->p_tsize+p->p_dsize+p->p_ssize+UPAGES);
		ds.st_size = ip->i_size;
		ds.st_atime = ds.st_mtime = ds.st_ctime = time;
		if(copyout((caddr_t)&ds, (caddr_t)ub, sizeof(ds)) < 0)
			u.u_error = EFAULT;
	}
	else
		u.u_error = ENOENT;
}

prnami(p, pflagp, follow)
register struct nx *p;
struct argnamei **pflagp;
{
	register char *cp = p->cp;
	register struct inode *dp = p->dp;
	register int n = 0;

	if ((dp->i_mode & IFMT) != IFDIR)
		u.u_error = ENOTDIR;
	(void) access(dp, IEXEC);
	if (*pflagp)
		u.u_error = EPERM;
	if (u.u_error)
		return 1;
	if (cp[0] == '.') {
		if (cp[1] == 0)
			return 0;
		if (cp[1] == '/')
			return (p->cp = cp + 2), 2;
		if (cp[1] != '.' || (cp[2] && cp[2] != '/'))
			return (u.u_error = ENOENT), 1;
		p->dp = dp->i_mpoint;
		iput(dp);
		plock(p->dp);
		p->dp->i_count++;
		return 2;
	}
	while (*cp) {
		if(*cp < '0' || *cp > '9')
			return (u.u_error = ENOENT), 1;
		n = 10 * n + *cp++ - '0';
	}
	prele(dp);
	p->dp = iget(dp, PROCDEV, n + PRMAGIC);
	iput(dp);
	return 0;
}

prmount(sip, ip, flag, mnt, fstyp)
struct inode *sip, *ip;
{
	if (!suser())
		return;
	if (mnt)
		pron(sip, ip, flag, fstyp);
	else
		proff(ip, fstyp);
}

pron(sip, ip, flag, fstyp)
struct inode *sip;
register struct inode *ip;
{
	register struct inode *rip;
	struct inode pi;

	if (ip->i_fstyp == fstyp) {
		u.u_error = EBUSY;
		return;
	}
	pi.i_fstyp = fstyp;
	pi.i_mpoint = NULL;
	if ((rip = iget(&pi, PROCDEV, (ino_t)ROOTINO)) == NULL) {
		u.u_error = EINVAL;
		return;
	}
	if (rip->i_mpoint) {
		iput(rip);
		u.u_error = EBUSY;
		return;
	}
	prele(rip);
	rip->i_mpoint = ip;
	ip->i_mroot = rip;
	ip->i_flag |= IMOUNT;
	ip->i_count++;
}

proff(mip, fstyp)
struct inode *mip;
{
	struct inode *rip;

	plock(mip);
	rip = mip->i_mroot;
	mip->i_flag &=~ IMOUNT;
	mip->i_mpoint = NULL;
	iput(mip);
	plock(rip);
	iput(rip);
}

/* special tracing stuff */
prioctl(ip, cmd, cmaddr)
struct inode *ip;
caddr_t cmaddr;
{	register struct proc *p; struct text *xp; struct inode *ixp;
	unsigned int signo; int n;
	switch (cmd) {
	default:
		u.u_error = EINVAL;
		return;

	case PIOCGETPR:		/* read struct proc */
	case PIOCOPENT:		/* open text file for reading */
	case PIOCSTOP:		/* send STOP signal and... */
	case PIOCWSTOP:		/* wait for process to STOP */
	case PIOCRUN:		/* make process runnable */
	case PIOCSMASK:		/* set signal trace mask */
	case PIOCCSIG:		/* clear current signal */
	case PIOCSSIG:		/* set current signal */
	case PIOCKILL:		/* send signal */
	case PIOCSEXEC:		/* stop on exec */
	case PIOCREXEC:		/* run on exec */
	case PIOCNICE:		/* set nice priority */
		p = pfind(ip->i_number - PRMAGIC);
		if (p == 0) {
			u.u_error = ENOENT;
			return;
		}
	}
	switch (cmd) {
	case PIOCGETPR:		/* read struct proc */
		u.u_base = cmaddr;
		u.u_offset = 0;
		u.u_count = sizeof(struct proc);
		iomove((char *)p, sizeof(struct proc), B_READ);
		return;

	case PIOCOPENT:		/* open text file for reading */
		if ((xp = p->p_textp) && (ixp = xp->x_iptr)) {
			plock(ixp);
			ixp->i_count++;
			open1(ixp, FREAD, 0);
		} else
			u.u_error = ENOENT;
		return;

	case PIOCSTOP:		/* send STOP signal and... */
		if (p->p_stat != SSTOP) {
			psignal(p, SIGSTOP);
		}
				/* fall through */
	case PIOCWSTOP:		/* wait for process to STOP */
		if (p->p_stat != SSTOP)
			tsleep((caddr_t)p->p_trace, PZERO+1, 0);
		if (p->p_pid != (ip->i_number - PRMAGIC)
		||  p->p_stat == SZOMB)
			u.u_error = ENOENT;
		else if (p->p_stat != SSTOP)
			u.u_error = EINTR;
		return;

	case PIOCRUN:		/* make process runnable */
		if (p->p_stat == SSTOP) {
			setrun(p);
		}
		return;

	case PIOCSMASK:		/* set signal trace mask */
		if (useracc((caddr_t)cmaddr, 4, B_READ)) {
			if (p->p_trace->i_un.i_sigmask = fuword((caddr_t)cmaddr))
				p->p_flag |=  (STRC|SPROCTR);
			else
				p->p_flag &= ~(STRC|SPROCTR);
		} else
			u.u_error = EFAULT;
		return;

	case PIOCCSIG:		/* clear current signal */
		p->p_cursig = 0;
		return;

	case PIOCSSIG:		/* set current signal */
		if (p->p_stat != SSTOP)
			u.u_error = EBUSY;
		else if (useracc((caddr_t)cmaddr, 4, B_READ)) {
			if ((signo = (u_long)fuword((caddr_t)cmaddr)) > NSIG)
				u.u_error = EINVAL;
			else
				p->p_cursig = signo;
		} else
			u.u_error = EFAULT;
		return;

	case PIOCKILL:		/* send signal */
		if (useracc((caddr_t)cmaddr, 4, B_READ)) {
			if ((signo = (u_long)fuword((caddr_t)cmaddr)) > NSIG)
				u.u_error = EINVAL;
			else
				psignal(p, signo);
		} else
			u.u_error = EFAULT;
		return;

	case PIOCSEXEC:		/* stop on exec */
		p->p_flag |=  SSEXEC;
		return;

	case PIOCREXEC:		/* run on exec */
		p->p_flag &= ~SSEXEC;
		return;

	case PIOCNICE:		/* set nice priority */
		if (useracc((caddr_t)cmaddr, 4, B_READ)) {
			n = p->p_nice + fuword((caddr_t)cmaddr);
			if (n >= 2*NZERO)
				n = 2*NZERO -1;
			if (n < 0)
				n = 0;
			if (n < p->p_nice && !suser())
				return;
			p->p_nice = n;
			(void) setpri(p);
		} else
			u.u_error = EFAULT;
		return;

	default:
		panic("prioctl");
	}
}

/* Lock the process p. */

prlock(p)
register struct proc *p;
{
	int s;

	if (p != u.u_procp) {
	waitloop:
		if (prpidlock == p) {		/* wait if p has the interlock */
			prpidwant = u.u_procp;
			if (tsleep((caddr_t)&prpidlock, PZERO+1, 0) == TS_SIG)
				return (u.u_error = EINTR);
			goto waitloop;
		}
		if (p->p_flag & SPAGE) {	/* wait if p is paging */
			p->p_flag |= SPROCWT;
			if (tsleep((caddr_t)&(p->p_stat), PZERO+1, 0) == TS_SIG)
				return (u.u_error = EINTR);
			goto waitloop;
		}

		s = spl6();	/* keep clock out */

		if (p->p_flag&SPRBUSY || (p->p_stat != SSLEEP &&
		    p->p_stat != SRUN && p->p_stat != SSTOP)) {
			splx(s);
			return (u.u_error = EBUSY);
		}
		if (p->p_flag&SLOAD && p->p_stat == SRUN)
			remrq(p);	/* he's now invisible to swtch() */

		/* interlock; also causes swapin, inhibits swapout, setrq, remrq */

		p->p_flag |= SPROCIO;

		splx(s);	/* now do your worst, we don't care */
	}
	u.u_procp->p_flag |= SKEEP;	/* if we get swapped, could deadlock */

	while ((p->p_flag&SLOAD) == 0) {
		/* sched will see SPROCIO, swap him in, and signal us */
		if (tsleep((caddr_t)&p->p_addr, PZERO+1, 0) == TS_SIG) {
			prunlock(p);
			return (u.u_error = EINTR);
		}
	}
	while (prpidlock) {
		prpidwant = u.u_procp;
		if (tsleep((caddr_t)&prpidlock, PZERO+1, 0) == TS_SIG) {
			prunlock(p);
			return (u.u_error = EINTR);
		}
	}
	prpidlock = u.u_procp;
	/* now map his user area into kernel space */
	uaccess(p, Prusrmap, &prusrutl);

	return (u.u_error = 0);
}

/* Undo prlock. */

prunlock(p)
register struct proc *p;
{
	int s;

	u.u_procp->p_flag &= ~SKEEP;

	if (p != u.u_procp) {
		s = spl6();	/* keep clock out during process state change */
		p->p_flag  &= ~SPROCIO;
		if (p->p_flag&SLOAD && p->p_stat == SRUN)
			setrq(p);	/* visible again to swtch() */
		splx(s);
	}
	if (prpidlock == u.u_procp) {
		prpidlock = 0;
		if (prpidwant) {
			prpidwant = 0;
			wakeup((caddr_t)&prpidlock);
		}
	}
	return 0;
}

/* Read/write from/to process p. */

#define REGADR(i)	((caddr_t)(regbase + i))
#define SEEKADR		((caddr_t)u.u_offset)

prusrio(p, flag)
register struct proc *p; int flag;
{
	register int *regbase;
	caddr_t maxadr; int segment, resid;
	register int v;

	u.u_error = 0;
	if ((u_long)SEEKADR < (u_long)(SYSADR)) {
		v = btop(SEEKADR);
		if (v < LOWPAGES)
			u.u_error = EIO;
		else if (isassv(p,v)) {
			segment = STACK;
			maxadr = (caddr_t)USRSTACK;
		}
		else if (isatsv(p,v)) {
			segment = TEXT;
			maxadr = ptob(tptov(p, p->p_tsize));
		}
		else if (isadsv(p,v)) {
			segment = DATA;
			maxadr = ptob(dptov(p, p->p_dsize));
		}
		else
			u.u_error = EIO;
	}
	else if (SEEKADR >= (caddr_t)&u &&
		 SEEKADR < (maxadr = (caddr_t)&u + sizeof(u)))
		segment = USERAREA;
	else
		u.u_error = EIO;

	if (u.u_error) return;

	if ((flag & B_READ) == 0) switch(segment) {
	case TEXT:
		prxdup(p);
		break;
	case USERAREA:
		regbase = prusrutl.u_ar0;
		if (SEEKADR < REGADR(R0) ||
		    (u_long)SEEKADR >= (u_long)(maxadr = REGADR(PC+1)))
			u.u_error = EIO;
		regbase = (int *)((u_long)&prusrutl + (u_long)regbase - (u_long)&u);
		if ((regbase[PS]&PSL_USERSET) != PSL_USERSET ||
		    (regbase[PS]&PSL_USERCLR) != 0)
			u.u_error = EBUSY;
		break;
	}

	if (u.u_error) return;

	resid = u.u_count;
	if ((u_long)((u_long)u.u_offset + (u_long)u.u_count) >= (u_long)maxadr)
		u.u_count = maxadr - (caddr_t)u.u_offset;
	resid -= u.u_count;

	if (segment == USERAREA) {
		iomove((caddr_t)&prusrutl + ((u_long)u.u_offset - (u_long)&u),
			u.u_count, flag);
		if ((flag & B_READ) == 0) {
			regbase[PS] |=  PSL_USERSET;
			regbase[PS] &= ~PSL_USERCLR;
		}
	} else
		priomove(p, flag);

	u.u_count += resid;
}

/* Move data between the object process and us. */

priomove(p, flag)
	register struct proc *p;
{
	register struct pte *pte;
	register clofset, clcount;
	int waslocked;

	while (u.u_count > 0 && u.u_error == 0) {
		pte = prclmap(p, (caddr_t)u.u_offset, &waslocked);
		clofset = u.u_offset & CLOFSET;
		clcount = min(u.u_count, CLSIZE*NBPG - clofset);
		iomove(&priobuf[clofset], clcount, flag);
		prclunmap(p, pte, flag | waslocked);
	}
}

/* Map and lock a cluster from process into system space. */

struct pte *
prclmap(p, vaddr, flagp)
	register struct proc *p;
	register caddr_t vaddr;
	int *flagp;
{
	register i;
	register struct pte *pte;
	i = clbase(btop(vaddr));
	pte = vtopte(p, i);

	*flagp = 0;
	if (pte->pg_v) {
		if (cmap[pgtocm(pte->pg_pfnum)].c_lock)
			*flagp = B_PHYS;
		else
			mlock(pte->pg_pfnum);
	} else {
#ifdef vax
		p->p_flag |= SDLYU;
		pagein(vaddr, &prusrutl);
		p->p_flag &= ~SDLYU;
#endif
#if sun
		pagein(vaddr, &prusrutl, 1);
#endif
		if (!pte->pg_v)
			panic("prclmap: pte not valid after pagein");
	}
#ifdef vax
	for (i=0; i<CLSIZE; i++) {
		*(int *)(Prbufmap + i) = PG_V | PG_KW | (pte->pg_pfnum + i);
		mtpr(TBIS, &priobuf[i*NBPG]);
	}
#endif
#ifdef sun
	mapin(Prbufmap, btop(priobuf), pte->pg_pfnum,CLSIZE,(int)(PG_V|PG_KW));
#endif
	return pte;
}

/* Release a cluster, updating its pte's. */

prclunmap(p, pte, flag)
	struct proc *p;
	register struct pte *pte;
{
	register i;
	if ((flag & B_PHYS) == 0)
		munlock(pte->pg_pfnum);
	if ((flag & B_READ) == 0) { /* Write to device writes memory */
		for (i=0; i<CLSIZE; i++)
			(pte+i)->pg_m = 1;
#if sun
		p->p_flag |= SPTECHG;
#endif
	}
}

/* Prepare the process' text segment for writing, duplicating it if necessary. */

prxdup(p)
	register struct proc *p;
{
	register struct text *xp, *pxp;

	if ((pxp = p->p_textp) == 0)
		return 0;
	if (!(pxp->x_flag&XPAGI) && pxp->x_count == 1) {
		pxp->x_flag |= XTRC|XWRIT;
		return 0;
	}
	if (pxp->x_flag&XTRC)
		panic("prxdup");

	for (xp = text; xp < textNTEXT && xp->x_iptr; xp++)
		/* void */ ;
	if (xp >= textNTEXT)
		return (u.u_error = ENOSPC);

	xp->x_flag = XLOCK|XTRC|XLOAD;
	xp->x_size = pxp->x_size;
	if (vsxalloc(xp) == NULL)
		return (u.u_error = ENOSPC);

	xp->x_count = 1;
	xp->x_ccount = 0;
	xp->x_rssize = 0;
	(xp->x_iptr = pxp->x_iptr)->i_count++;

	xlock(pxp);
	--pxp->x_count;
	pxp->x_flag &= ~XLOCK;
	xccdec(pxp, p);

	p->p_textp = xp;
	xlink(p);
	p->p_flag &= ~SPAGI;

	prxread(p, &prusrutl);

	xp->x_flag |= XWRIT;
	xp->x_flag &= ~XLOAD;
	xunlock(xp);
	return u.u_error;
}

prxread(p, up)
	register struct proc *p;
	struct user *up;
{
	register struct inode *ip = p->p_textp->x_iptr;
	register count; register struct pte *pte;
	register caddr_t vaddr; int waslocked;
	caddr_t gettmem();

	caddr_t ubase = u.u_base; unsigned int ucount = u.u_count;
	off_t uoffset = u.u_offset;
	vaddr = gettmem(up);

	plock(ip);
	ip->i_flag |= ITEXT;

	count = up->u_exdata.ux_tsize;
	u.u_offset = gettfile(up);

	u.u_segflg = 1;
	u.u_count = 0;
	while (count > 0 && u.u_count == 0) {
		pte = prclmap(p, vaddr, &waslocked);
		u.u_base = (caddr_t)priobuf;
		count -= u.u_count = min(count, CLSIZE*NBPG);
		vaddr += u.u_count;
		readi(ip);
		prclunmap(p, pte, B_WRITE | waslocked);
	}
	prele(ip);

	u.u_base = ubase; u.u_count = ucount;
	u.u_offset = uoffset; u.u_segflg = 0;

	return (count ? (u.u_error = EIO) : 0);
}
