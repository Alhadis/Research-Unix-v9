/*	kern_mman.c	6.1	83/07/29	*/

#include "../machine/reg.h"
#include "../machine/psl.h"
#include "../machine/pte.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/lnode.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/inode.h"
#include "../h/seg.h"
#include "../h/acct.h"
#include "../h/wait.h"
#include "../h/vm.h"
#include "../h/text.h"
#include "../h/file.h"
#include "../h/cmap.h"
#include "../h/mman.h"
#include "../h/conf.h"

getpagesize()
{

	u.u_r.r_val1 = NBPG * CLSIZE;
}

smmap()
{
	struct a {
		caddr_t	addr;
		int	len;
		int	prot;
		int	fd;
		off_t	pos;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;
	register struct inode *ip;
	register struct fpte *pte;
	int off;
	int fv, lv, pm;
	dev_t dev;
	int (*mapfun)();

	if ((fp = getf(uap->fd)) == NULL) {
		u.u_error = EBADF;
		return;
	}
	ip = fp->f_inode;
	if ((ip->i_mode & IFMT) != IFCHR) {
		u.u_error = EINVAL;
		return;
	}
	dev = ip->i_un.i_rdev;
	mapfun = cdevsw[major(dev)].d_mmap;
	if (mapfun == NULL) {
		u.u_error = EINVAL;
		return;
	}
	if (((int)uap->addr & CLOFSET) || (uap->len & CLOFSET) ||
	    (uap->pos & CLOFSET)) {
		u.u_error = EINVAL;
		return;
	}
	if ((uap->prot & PROT_WRITE) && (fp->f_flag&FWRITE) == 0) {
		u.u_error = EINVAL;
		return;
	}
	if ((uap->prot & PROT_READ) && (fp->f_flag&FREAD) == 0) {
		u.u_error = EINVAL;
		return;
	}
	fv = btop(uap->addr);
	lv = btop(uap->addr + uap->len - 1);
	if (lv < fv || !isadsv(u.u_procp, fv) || !isadsv(u.u_procp, lv)) {
		u.u_error = EINVAL;
		return;
	}
	for (off=0; off<uap->len; off += NBPG) {
		if ((*mapfun)(dev, uap->pos+off, uap->prot) == -1) {
			u.u_error = EINVAL;
			return;
		}
	}
	if (uap->prot & PROT_WRITE)
		pm = PG_UW;
	else
		pm = PG_URKR;
	pm |= PG_V | PG_FOD;		/* mark as an mmap'ed page */
	for (off = 0; off < uap->len; off += NBPG) {
		pte = (struct fpte *)vtopte(u.u_procp, fv);
		u.u_procp->p_rssize -= vmemfree((struct pte *)pte, 1);
		*(int *)pte =
		    ((*mapfun)(dev, uap->pos+off, uap->prot) & PG_PFNUM) | pm;
		pte->pg_source = uap->fd;
		fv++;
	}
	fv = btop(uap->addr);
	newptes(vtopte(u.u_procp, fv), fv, (int)btoc(uap->len));
	u.u_pofile[uap->fd] |= MMAPPED;
}

munmap()
{
	register struct a {
		caddr_t	addr;
		int	len;
	} *uap = (struct a *)u.u_ap;
	int off;
	int fv, lv;
	register struct pte *pte;

	if (((int)uap->addr & CLOFSET) || (uap->len & CLOFSET)) {
		u.u_error = EINVAL;
		return;
	}
	fv = btop(uap->addr);
	lv = btop(uap->addr + uap->len - 1);
	if (lv < fv || !isadsv(u.u_procp, fv) || !isadsv(u.u_procp, lv)) {
		u.u_error = EINVAL;
		return;
	}
	for (off = 0; off < uap->len; off += NBPG) {
		pte = vtopte(u.u_procp, fv);
		u.u_procp->p_rssize -= vmemfree(pte, 1);
		*(int *)pte = (PG_UW|PG_FOD);
		((struct fpte *)pte)->pg_source = PG_FZERO;
		fv++;
	}
	fv = btop(uap->addr);
	newptes(vtopte(u.u_procp, fv), (u_int)fv, (int)btoc(uap->len));
}

munmapfd(fd)
{
	register struct fpte *pte;
	register int i;

	for (i = 0; i < u.u_dsize; i++) {
		pte = (struct fpte *)dptopte(u.u_procp, i);
		if (pte->pg_v && pte->pg_fod && pte->pg_source == fd) {
			*(int *)pte = (PG_UW|PG_FOD);
			pte->pg_source = PG_FZERO;
		}
	}
	newptes(dptopte(u.u_procp, 0), dptov(u.u_procp, 0), u.u_dsize);
	u.u_pofile[fd] &= ~MMAPPED;
	
}
