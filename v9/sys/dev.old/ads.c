/* 10-26-86 put in splx(ps)'s on error returns from dsstart loop */
/* 5-3-86 fixed infamous vstodb-on-close bug */
/* 5-1-86 converted to work with BSD4.2 */
/* 11-25-85 fixed buffer initialization bug */
/* 9-5-85 simulated DSWAIT ioctl inside adsclose */
/* 4-22-85 added check for partial buffer in adscleanup */
/* 3-27-85 cleaned up handling of signals; added DSSTOP, DSFILTER;
	made DSMONO, DSSTEREO sleep; added check for 0 in DSRATE */
/* 1-18-85 restricted use of priority-raising; check for offline in probe() */
/* 5-29-84  adsrelease checks for zombie state or even death */
/* adsclose() shuts down device */
/* DSWAIT added */
/* 5-25-84  process pointer saved in adsopen for adsintr() */
/*	vsunlock must then be duplicated using correct process structure */
/* the interrelation of adsstart-sleeping, adsintr, and adscleanup
	needs to be cleaned up 
	Probably adscleanup should not call wakeup */


#include "ads.h"
# if NADS > 0

/*
 * DSC System 200 driver
 * via DSC dma11.
 * Combined Eighth Edition & BSD 4.2 Unibus version
 */

/* NOTE: This requires BSD42 to be defined somehow, if BSD 4.2 is being used */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/mount.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/map.h"
#include "../h/conf.h"
#include "../h/proc.h"
#include "../h/file.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
/* Since "make depend" would otherwise get confused by the #ifdef'ed #includes,
     we fake it out, so it ignores all of these #include's */
#ifdef BSD42
# include "../h/uio.h"
# include "../h/kernel.h"
# include "../h/tsleep.h"
# include "../machine/pte.h"
# include "../vaxuba/ubareg.h"
# include "../vaxuba/ubavar.h"
# include "../vaxuba/adsreg.h"
# include "../vaxuba/adsvar.h"
#else
# include "../h/pte.h"
# include "../h/ubareg.h"
# include "../h/ubavar.h"
# include "../h/adsreg.h"
# include "../h/adsvar.h"
#endif

#ifdef BSD42
# define ERR(err)   return err
# define IOMOVE(buf,count,dir)	   uiomove(buf, count, dir, uio)
# define PHYSIO(strat,buf,dev,dir,minph) \
	       physio(strat, buf, dev, dir, minph, uio)
# define COUNT	    uio->uio_iov->iov_len
# define RESID	    uio->uio_resid
# define BASE	    uio->uio_iov->iov_base
# define IO_READ    UIO_READ
# define IO_WRITE   UIO_WRITE
# define UBA_WANTBDP	0
#else
# define ERR(err)   { u.u_error = (err);  return; }
# define IOMOVE(buf,count,dir)	   (iomove(buf, count, dir), u.u_error)
# define PHYSIO(strat,buf,dev,dir,minph) \
	       (physio(strat, buf, dev, dir, minph), u.u_error)
# define COUNT	    u.u_count
# define RESID	    u.u_count
# define BASE	    u.u_base
# define IO_READ    B_READ
# define IO_WRITE   B_WRITE
#endif

/*
 * THESE ARE SITE-SPECIFIC
 *
 * starting base for the
 * d/a and a/d converters,
 * for setting up sequence ram.
 */

/*
 * reset device
 */
# define RESETDEV	bit(4)
# define dsseq(sc, reg, conv, dir) \
	(sc->c_ascseq[reg] = conv | ((dir & 01) << 6))

/*
 * ds flags
 */
# define DS_OPEN	bit(0)
# define DS_IDLE	bit(1)
# define DS_MON		bit(3)
# define DS_BRD		bit(4) 
# define DS_READ	bit(5)
# define DS_WRITE	bit(6)


# define DSPRI		(PZERO+1)
# define SPL_DS()	spl6()
/*
 * Redundant book-keeping
 * to help avoid page-wise overlap of buffers
 *	remember that this device is both asynchronous and
 *	uses user-supplied buffers
 */
struct dspageinfo {
	int firstunit,
		lastunit;	/* in units of CLSIZE*NBPG, i.e. 1024 */
};
/*
 * relevant information
 * about the dma and asc
 */
struct ads_softc {
	short		c_dmacsr;	/* copy of dma csr */
	short		c_asccsr;	/* copy of asc csr */
	short		c_ascflags;	/* initial asc flags */
	short		c_ascsrt;	/* sampling rate */
	short		c_ascseq[8];
	int		c_flags;	/* internal flags */
	int		c_errs;		/* errors, returned via ioctl */
	int		c_bufno;	/* dsubinfo/buffer */
	int		c_outbufno;
	int		c_uid;		/* user id */
	short		c_pid;		/* process id */
	struct proc	*c_procp;	/* process structure pointer */
	int		c_ubinfo[NADSB];	/* uba info */
	struct dspageinfo	c_pageinfo[NADSB];
	struct buf	c_dsb[NADSB];	/* buffer list */
	struct buf	*c_ibp, *c_obp;	/* buffer list pointers */
	int		c_nbytes;	/* total # of bytes xferred since reset */
	int		c_to_idle;	/* total # times idle flag set */
	int		c_to_active;	/* total # times idle flag cleared */
} ads_softc[NADS];

int adsprobe(), adsattach(), adsintr();

struct uba_device *adsdinfo[NADS];
struct uba_ctlr *adscinfo[NADS];  /* attach() sets adscinfo to point to ctlr struct */

u_short adsstd[] = {
	0164400, 0
};

struct uba_driver adsdriver = {
	adsprobe, 0, adsattach, 0, adsstd, "ads", adsdinfo, "ads", adscinfo
};
int	adsdebug;

/*
 * all this just to generate
 * an interrupt; rather
 * involved isn't it?
 */
adsprobe(reg)
caddr_t reg;
{
	register int br, cvec;		/* value-result */
	register struct dsdevice *dsaddr;
	int dummy;

# ifdef lint	
	br = 0; cvec = br; br = cvec;
# endif lint

	dsaddr = (struct dsdevice *) reg;
	if ((dummy = dsaddr->dmacsr) & DMA_OFL) {
		printf("dsc offline\n");
		return 0;
	}

	printf("dsc interrupt vector at 0%o\n", dsaddr->dmaiva & 0776);
	dsaddr->dmacsr = 0;
	dsaddr->dmacls = 0;
	dsaddr->ascseq[0] = DABASE+0 | (DA << 6) | LAST_SEQ;
	dsaddr->ascsrt = 0100;
	dsaddr->dmablr = ~1;
	dsaddr->dmacsr = DMA_IE;
	dsaddr->asccsr = ASC_RUN | ASC_IE;

	DELAY(40000);

	/*
	 * now shut everything down.
	 */
	dsaddr->dmacls = dummy = 0;
	dummy = dsaddr->dmasar;			/* clears sar flag */
	dsaddr->dmaclr = dummy = 0;

	dummy = dsaddr->ascrst;
	dsaddr->ascrst = dummy = 0; 

	return sizeof(struct dsdevice);
}

adsattach(ui)
struct uba_device *ui;
{
	register int unit;
	register int i;

	unit = ui->ui_unit;

	ads_softc[unit].c_flags = DS_IDLE;
	for (i = 0; i < NADSB; i++)
		ads_softc[unit].c_dsb[i].b_flags = 0;
}

/* ARGSUSED */
adsopen(dev, flag)
dev_t dev;
{
	register struct dsdevice *dsaddr;
	register struct uba_device *ui;
	register struct ads_softc *sc;
	register int unit, i;
	register struct buf *bp;
	int dummy;

	if ((unit = (minor(dev) & ~RESETDEV)) >= NADS)
		goto bad;

	if ((ui = adsdinfo[unit]) == NULL)
		goto bad;

	if (ui->ui_alive == 0)
		goto bad;

	sc = &ads_softc[ui->ui_unit];
	dsaddr = (struct dsdevice *)ui->ui_addr;

	if(adsdebug)printf("adsopen: unit %d, flag %x\n",unit,flag);
	if (dsaddr->dmacsr & DMA_OFL)
		ERR(ENXIO);

	/*
	 * if this is the reset device
	 * then just do a reset and return.
	 */
	if (minor(dev) & RESETDEV) {
		/*
		 * if the converters are in use then
		 * only the current user or root can
		 * do a reset.
		 */
		if (sc->c_flags & DS_OPEN) {
			if ((sc->c_uid != u.u_ruid) && (u.u_uid != 0))
				ERR(ENXIO);
		}
		adscleanup(unit);
		sc->c_flags = DS_IDLE;
		printf("ads%d: reset\n",unit);
		return 0;
	}

	/*
	 * only one person can use it at a time
	 * and it can't be opened for both reading and writing
	 */
	if ((sc->c_flags & DS_OPEN) || (flag & (FREAD|FWRITE)) == (FREAD|FWRITE))
bad:		ERR(ENXIO);

	/*
	 * initialize
	 */


	/* set defaults and initial conditions in ads_softc */
	sc->c_flags = DS_IDLE;
	sc->c_errs = 0;

	sc->c_nbytes = sc->c_to_idle = sc->c_to_active = 0;

	sc->c_ascflags = (flag & FWRITE ? ASC_PLAY : ASC_RECORD) | ASC_HZ04;
	sc->c_ascseq[0] = (
		(flag & FWRITE) ?
			(DABASE+0 | (DA << 6)) :  (ADBASE+0 | (AD << 6))
						) | LAST_SEQ;
	sc->c_ascsrt = 399;	/* 10 kHz */

	sc->c_uid = u.u_ruid;
	sc->c_pid = u.u_procp->p_pid;
	sc->c_procp = u.u_procp;
	for (i = 0; i < NADSB; i++)
		sc->c_dsb[i].b_flags = 0;
	sc->c_ibp = sc->c_obp = &sc->c_dsb[0];
	sc->c_outbufno = sc->c_bufno = 0;

	if(adsdebug)printf("pid %d %d, flag %x stat %x\n",
			      sc->c_pid,sc->c_procp->p_pid,
			      u.u_procp->p_flag,u.u_procp->p_stat);
	sc->c_flags |= DS_OPEN | (flag & FREAD ? DS_READ : DS_WRITE);
	u.u_procp->p_flag |= SPHYSIO;
	dsaddr->dmacsr = 0;
	dummy = dsaddr->dmasar;

	return 0;
}

/* ARGSUSED */
adsclose(dev, flag) {
	register int unit;
	register struct dsdevice *dsaddr;
	register struct uba_device *ui;
	register struct ads_softc *sc;

	unit = minor(dev) & ~RESETDEV;
	sc = &ads_softc[unit];
	ui = adsdinfo[unit];
	dsaddr = (struct dsdevice *)ui->ui_addr;
	if(adsdebug)printf("X");

	/*  In case user doesn't use the DSWAIT ioctl */
	while (sc->c_outbufno != sc->c_bufno && !(sc->c_flags & DS_IDLE)) {
		if (tsleep((caddr_t)sc, DSPRI, 5) != TS_OK)
			break;
	}

	adscleanup(unit);
	sc->c_flags &= ~DS_OPEN;
	u.u_procp->p_flag &= ~SPHYSIO;
	if(adsdebug){
		printf("to_active %d, to_idle %d, c_nbytes %d\n",
		sc->c_to_active,
		sc->c_to_idle,
		sc->c_nbytes);
		printf("c_ascseq[0] %x\n",sc->c_ascseq[0]);
		adsdb(dsaddr);
	}
	return 0;
}

/*
 *
 * Using COUNT and BASE, each buffer header is set up.
 * The converters only need the base address of the buffer and the word
 * count.
 *
 */
adsstart(dev, rw, uio)
register dev_t dev;
register struct uio *uio;
{
	register struct dsdevice *dsaddr;
	register struct uba_device *ui;
	register struct ads_softc *sc;
	register struct buf *bp;
	register int	unit, c, bufno, ps;
	int dummy, i, count;
	caddr_t base;
	int bits;

	if(adsdebug)printf("S");

	unit = minor(dev) & ~RESETDEV;
	sc = &ads_softc[unit];
	ui = adsdinfo[unit];
	dsaddr = (struct dsdevice *)ui->ui_addr;

	/*
	 * check user access rights to buffer
	 */
	if (useracc(BASE, COUNT, rw==B_READ?B_WRITE:B_READ) == NULL)
		ERR(EFAULT);

	if (sc->c_errs)
		ERR(EIO);


	/*
	 * Get a buffer for each 64K block
	 * point each device buffer somewhere into
	 * the user's buffer
	 */
	/* NOTE that in order to get reasonable performance,
	 * buffer lengths (initial u.u_count) must be either
	 * less than 64K or sufficiently greater than a multiple
	 * of 64K.
	 */
	base = BASE;
	count = COUNT;
	while(count > 0) {

		if(adsdebug)printf("LT ");

		ps = SPL_DS();
		if ((sc->c_dmacsr = dsaddr->dmacsr) & DMA_SFL) {
			register	ts;

			/*
			 * no hardware buffers left, sleep till interrupt
			 * wakes us up
			 */
			if(adsdebug)printf("s");
			ts = tsleep((caddr_t)sc, DSPRI, 0);
			if(adsdebug)printf("w");
#ifdef RESTRICT
			splx(ps);
#endif
			switch(ts) {
				case TS_SIG:
#ifndef RESTRICT
					splx(ps);
#endif
					ERR(EINTR);
				case TS_OK:
					if ((sc->c_dmacsr = dsaddr->dmacsr) & DMA_SFL) {
#ifndef RESTRICT
						splx(ps);
#endif
						printf("ads%d  still full after tsleep\n",unit);
						adscleanup(unit);
						ERR(EIO); 
					}
			/* This is not necessary */
					if (!(sc->c_flags & DS_OPEN)) {
#ifndef RESTRICT
						splx(ps);
#endif
						printf("ads: wakeup unopen\n");
						adscleanup(unit);
						ERR(EIO);
					}
					break;
				default:
#ifndef RESTRICT
					splx(ps);
#endif
					printf("ads: %x from tsleep\n",ts);
					adscleanup(unit);
					ERR(EIO);
			}
#ifdef RESTRICT
		} else
			splx(ps);
#else
		}

		splx(ps);
#endif
		/*
		 * If device is IDLE (initial read/write, or datalate condition
		 *  has been detected in adsintr()), (re)initialize dsc
		 */

		ps = SPL_DS();

		if (sc->c_flags & DS_IDLE) {

			if(adsdebug) printf("I1 ");
			dsaddr->asccsr = sc->c_asccsr = sc->c_ascflags;
			dummy = dsaddr->ascrst;
			dsaddr->asccsr = (sc->c_asccsr |= ASC_IE);
			dsaddr->ascsrt = sc->c_ascsrt;
			{register	j, i = 0;
				do {
					dsaddr->ascseq[i] = sc->c_ascseq[i];
					j = i++;
				} while(!(sc->c_ascseq[j] & LAST_SEQ) && i<8);
			}
			dsaddr->dmacsr = sc->c_dmacsr = 0;
			dsaddr->dmacls = 0;
			dummy = dsaddr->dmasar;			/* clears sfl flag */
			dsaddr->dmawc = -1;
			dsaddr->dmacsr = sc->c_dmacsr = (DMA_IE | DMA_CHN
					| (rw == B_READ ? DMA_W2M : 0) );

			sc->c_bufno = sc->c_outbufno = 0;
			sc->c_ibp = sc->c_obp = &sc->c_dsb[0];	/* correct?  */

			sc->c_flags &= ~DS_IDLE;
			++sc->c_to_active;
		}
#ifdef RESTRICT
		splx(ps);
#endif
		/* get next device buffer and set it up */
		bp = sc->c_ibp++;
		if (sc->c_ibp > &sc->c_dsb[NADSB-1])
			sc->c_ibp = &sc->c_dsb[0];

		/* hopefully redundant check against going too fast */
		if(bp->b_flags & B_BUSY){
			printf("adsstart: about to set up BUSY buffer!\n");
			ERR(EIO);
		}

		c = MIN(count, 1024*64);
		bp->b_un.b_addr = base;
		bp->b_error = 0;
		bp->b_proc = u.u_procp;
		bp->b_dev = dev;
		bp->b_blkno = sc->c_bufno;
		bp->b_bcount = c;
		bufno = sc->c_bufno % NADSB;

		sc->c_pageinfo[bufno].firstunit = (int)base / CLSIZE;
		sc->c_pageinfo[bufno].lastunit =
				(((int)base + c + CLOFSET) / CLSIZE) - 1;

		for (i = 0; i < NADSB; i++) {
			if (!(sc->c_dsb[i].b_flags & B_BUSY))
				continue;
			if (sc->c_pageinfo[bufno].firstunit <=
						sc->c_pageinfo[i].lastunit
					&& sc->c_pageinfo[bufno].lastunit >=
						sc->c_pageinfo[i].firstunit) {
				printf("ads: buf %d overlaps %d\n",i,bufno);
				ERR(EIO);
			}
		}

		/* lock the buffer and send it off */

		bp->b_flags = B_BUSY | B_PHYS  | rw;
		if(adsdebug)printf("lk%d %x %d ",bufno,base,c);
		adslock(base, c);	/* fasten seat belts */

		sc->c_ubinfo[bufno] = ubasetup(ui->ui_ubanum, bp, UBA_WANTBDP);

#ifdef RESTRICT
		ps = SPL_DS();
#endif
		dsaddr->dmablr = ~ (c >> 1);
		dsaddr->dmasax = (sc->c_ubinfo[bufno] >> 16) & 03;
		dsaddr->dmasar = sc->c_ubinfo[bufno];
			
		sc->c_bufno++;

		/*
		 * make sure the ASC is running
		 */
		dsaddr->asccsr = (sc->c_asccsr |= ASC_RUN);

		splx(ps);

		base += c;
		count -= c;
		if(adsdebug)printf("LB\n");
	}
		
	RESID -= COUNT;	 /* Doesn't handle multiple iovecs */
	return 0;
}

adsdb(dsaddr)
register struct dsdevice *dsaddr;
{
	printf("dmacsr %b\n", dsaddr->dmacsr, DMA_BITS);
	printf("asccsr %b\n", dsaddr->asccsr, ASC_BITS);
	printf("dmawc %o\t", dsaddr->dmawc);
	printf("dmablr %o\n", dsaddr->dmablr);
	printf("dmaac %o\t", dsaddr->dmaac);
	printf("dmasar %o\n", dsaddr->dmasar);
	printf("dmaacx %o\t", dsaddr->dmaacx);
	printf("dmasax %o\n", dsaddr->dmasax);
	printf("ascsrt(decimal) %d\n", dsaddr->ascsrt);
}
/*
 * this is where the real work is done. we copy any device registers that
 * we will be looking at to decrease the amount of traffic on the ds 200
 * bus.
 *
 */
adsintr(dev)
dev_t dev;
{
	register struct dsdevice *dsaddr;
	register struct uba_device *ui;
	register struct ads_softc *sc;
	register struct buf *bp;
	register int bufno;
	register int i;
	int unit, dummy;

	unit = minor(dev) & ~RESETDEV;
	sc = &ads_softc[unit];
	ui = adsdinfo[unit];

	dsaddr = (struct dsdevice *)ui->ui_addr;
	
	sc->c_dmacsr = dsaddr->dmacsr;
	dsaddr->dmacls = dummy = 0;

	if (adsdebug) printf("i");
	if (sc->c_flags & DS_IDLE) {
		printf("ads%d: interrupt while idle\n", unit);
		return;
	}

	if (sc->c_dmacsr & DMA_ERR) {
		if(adsdebug)printf("e");
		sc->c_asccsr = (sc->c_dmacsr & DMA_XIN ? dsaddr->asccsr : 0);
		dsaddr->asccsr = dummy = 0;

		dsaddr->dmacsr = (sc->c_dmacsr &= ~DMA_CHN);
		if ((sc->c_dmacsr & (DMA_XER|DMA_UBA|DMA_AMPE|DMA_XBA))
				|| (sc->c_asccsr & (ASC_BA|ASC_DCN|ASC_DNP))) {
			++sc->c_errs;
			printf("ads%d error: asccsr=%b dmacsr=%b\n",
				unit, sc->c_asccsr, ASC_BITS,
				sc->c_dmacsr, DMA_BITS);
		}
		dsaddr->dmablr = ~0;
		adscleanup(unit);
		return;
	}
		
	/*
	 * get current buffer and release it
	 */

	bp = sc->c_obp++;
	if (sc->c_obp > &sc->c_dsb[NADSB-1])
		sc->c_obp = &sc->c_dsb[0];

	sc->c_outbufno++;
	if(adsdebug)printf("b ");
	adsrelease(sc, ui, bp);

	/*
	 * update byte count.
	 */

	sc->c_nbytes += bp->b_bcount;

	wakeup((caddr_t)sc);
	if (adsdebug) printf(" upi ");
}

/*
 * release resources, if any, associated with a buffer
 */
adsrelease(sc, ui, bp)
register struct ads_softc *sc;
struct uba_device *ui;
register struct buf *bp;
{
	register int	ps;
	register struct pte *pte;
	register int npf;
	int	bufno;

	bufno = bp->b_blkno % NADSB;
	if(adsdebug)printf("R%d ",bufno);
	/*
	 * release uba resources
	 * and memory resources if process seems healthy
	 */
	ps = SPL_DS();

	if (bp->b_flags & B_BUSY) {
		if(adsdebug) printf("%x %d ",bp->b_un.b_addr,bp->b_bcount);
		ubarelse(ui->ui_ubanum, &sc->c_ubinfo[bufno]);

		if (sc->c_pid != sc->c_procp->p_pid ||
			 (sc->c_procp->p_flag & SWEXIT) ||
			 sc->c_procp->p_stat == SZOMB){
				/* process is dying or dead - exit() will
				    release memory (does it unlock???) but
				    will not release uba resources */
			printf("\nDSC exit release pid %d %d flag %x stat %x ",
				sc->c_pid, sc->c_procp->p_pid,
				sc->c_procp->p_flag, sc->c_procp->p_stat);
			sc->c_flags &= ~DS_OPEN;
		}
		else {
			/* we can't call vsunlock() because u.u_procp may be
				different */
			pte = vtopte(sc->c_procp,btop(bp->b_un.b_addr));
			npf = btoc(bp->b_bcount + ((int)(bp->b_un.b_addr)&CLOFSET));
			if(adsdebug)printf("U%x %d ",pte->pg_pfnum,npf);
			while(npf > 0) {
				munlock(pte->pg_pfnum);
				if(sc->c_ascflags & ASC_RECORD)
					pte->pg_m = 1;	/* memory written */
				pte += CLSIZE;
				npf -= CLSIZE;
			}
		}

		bp->b_flags = 0;  /* better than resetting B_BUSY */
	}

	splx(ps);
}

/*
 * release all buffers and do general cleanup
 */
adscleanup(unit)
int unit;
{
	register struct dsdevice *dsaddr;
	register struct uba_device *ui;
	register struct ads_softc *sc;
	register struct buf *bp;
	int	dummy;

	unit = (unit & ~RESETDEV);
	sc = &ads_softc[unit];
	ui = adsdinfo[unit];
	dsaddr = (struct dsdevice *)ui->ui_addr;

	/* Check for partial buffer */
	if ((dummy = dsaddr->dmawc) != ~0)
		sc->c_nbytes += sc->c_obp->b_bcount - ((~dummy) << 1);

	dsaddr->asccsr = dummy = 0;
	dummy = dsaddr->ascrst;
	dsaddr->dmacsr = dummy = 0;
	dummy = dsaddr->dmasar;
	
	if(adsdebug)printf("C ");
	if (!(sc->c_flags & DS_IDLE)) {
		sc->c_flags |= DS_IDLE;
		++sc->c_to_idle;
	}

	for (bp = &(sc->c_dsb[0]); bp < &(sc->c_dsb[NADSB]); ++bp)
		adsrelease(sc, ui, bp);


	/* in case called from adsintr while adsstart is sleeping */

	wakeup((caddr_t)sc);
	if (adsdebug) printf(" upc ");
}

/*
 * a/d conversion
 */
adsread(dev, uio)
dev_t dev;
struct uio *uio;
{
	return adsstart(dev, B_READ, uio);	/* dsc => memory */
}

/*
 * d/a conversion
 */
adswrite(dev, uio)
dev_t dev;
struct uio *uio;
{
	return adsstart(dev, B_WRITE, uio);	/* memory => dsc */
}

/* ARGSUSED */
adsioctl(dev, cmd, addr, flag)
dev_t dev;
caddr_t addr;
{
	register struct dsdevice *dsaddr;
	register struct ads_softc *sc;
	register struct ds_seq *dq;
	register struct ds_err *de;
	register struct ds_trans *dt;
	struct uba_device *ui;
	int	unit, flts, i;
#ifdef BSD42
	int	*iarg;
#else
	int	iarg[2];
	struct ds_seq	ds_seq;
	struct ds_err	ds_err;
	struct ds_trans	ds_trans;
#endif

	if (minor(dev) & RESETDEV)
		ERR(EINVAL);
	unit = minor(dev);
	if (adsdebug) printf("IOC");

	sc = &ads_softc[unit];
	ui = adsdinfo[unit];
	dsaddr = (struct dsdevice *) ui->ui_addr;

#ifdef BSD42
	iarg = (int *)addr;
#endif

	switch (cmd) {
		/* set sample rate */
		case DSRATE:
#ifndef BSD42
			if (copyin(addr, (caddr_t)iarg, sizeof(int)))
				ERR(EFAULT);
#endif
			if (iarg[0] == 0)
				ERR(EINVAL);
			sc->c_ascsrt = 4000000/iarg[0] - 1;
			break;

		case DS08KHZ:
			sc->c_ascflags &= ~ ASC_HZMSK;
			sc->c_ascflags |= ASC_HZ08;	/* set 8kHz filter */
			break;

		case DS04KHZ:
			sc->c_ascflags &= ~ ASC_HZMSK;
			sc->c_ascflags |= ASC_HZ04;	/* set 4kHz filter */
			break;

		case DSBYPAS:
			sc->c_ascflags &= ~ ASC_HZMSK;
			sc->c_ascflags |= ASC_BYPASS;	/* set bypass */
			break;

		case DSFILTER:
#ifndef BSD42
			if (copyin(addr, (caddr_t)iarg, sizeof(int)))
				ERR(EFAULT);
#endif
			sc->c_ascflags &= ~ ASC_HZMSK;
			sc->c_ascflags |= (iarg[0] << ASC_HZSHIFT) & ASC_HZMSK;
			break;

		/* fetch errors */
		case DSERRS:
#ifdef BSD42
			de = (struct ds_err *)addr;
#else
			de = &ds_err;
#endif
			de->dma_csr = sc->c_dmacsr;
			de->asc_csr = sc->c_asccsr;
			de->errors = sc->c_errs;
#ifndef BSD42
			if (copyout((caddr_t)de, addr, sizeof(ds_err)))
				ERR(EFAULT);
#endif
			break;

		/* fetch transition counts */
		case DSTRANS:
#ifdef BSD42
			dt = (struct ds_trans *)addr;
#else
			dt = &ds_trans;
#endif
			dt->to_idle = sc->c_to_idle;
			dt->to_active = sc->c_to_active;
#ifndef BSD42
			if (copyout((caddr_t)dt, addr, sizeof(ds_trans)))
				ERR(EFAULT);
#endif
			break;

		/* how many samples actually converted */
		case DSDONE:
#ifdef BSD42
			*(int *)addr = sc->c_nbytes;
#else
			if (copyout((caddr_t)&sc->c_nbytes, addr, sizeof(int)))
				ERR(EFAULT);
#endif
			break;

		case DSSTEREO:
			while (!sc->c_flags & DS_IDLE) {
				if (tsleep(sc, DSPRI, 0) == TS_SIG)
					ERR(EINTR);
			}
			if (sc->c_flags & DS_READ) {
				dsseq(sc, 0, ADBASE, AD);
				dsseq(sc, 1, ADBASE+1 | LAST_SEQ, AD);
			} else {
				dsseq(sc, 0, DABASE, DA);
				dsseq(sc, 1, DABASE+1 | LAST_SEQ, DA);
			}
			break;

		case DSMONO:
			while (!sc->c_flags & DS_IDLE) {
				if (tsleep(sc, DSPRI, 0) == TS_SIG)
					ERR(EINTR);
			}
			if (sc->c_flags & DS_READ)
				dsseq(sc, 0, ADBASE | LAST_SEQ, AD);
			else
				dsseq(sc, 0, DABASE | LAST_SEQ, DA);
			break;

		case DSRESET:
			adscleanup(unit);	/* not adequate (?) */
			printf("adsreset\n");
			break;
		case DSDEBUG:
 			adsdebug = !adsdebug;
			break;

		case DSWAIT:
			if (adsdebug) printf("W%d %d ", sc->c_outbufno, sc->c_bufno);
			while (sc->c_outbufno != sc->c_bufno
					&& !(sc->c_flags & DS_IDLE)) {
				if (tsleep(sc, DSPRI, 0) == TS_SIG) {
					adscleanup(unit);
					ERR(EINTR);
				}
			}
			break;

		case DSSTOP:
			/* stop dsc, if active */
			adscleanup(unit);
			break;

		default:
			ERR(ENOTTY);
	}

	return 0;
}


/*
 * zero uba vector.
 * shut off converters.
 * set error bit.
 */
adsreset(uban) {
	register struct uba_device *ui;
	register int ds;

	for (ds = 0; ds < NADS; ds++) {
		if ((ui = adsdinfo[ds]) == NULL)
			continue;
		if (ui->ui_alive == 0)
			continue;
		if (ui->ui_ubanum != uban)
			continue;

		printf(" ads%d", ds);

		/*
		 * release unibus resources
		 */
		ads_softc[ds].c_flags = DS_IDLE;
		adscleanup(ds);
	}
}
/*
 equivalent to vslock() from vmmem.c
  inserted differently here for debugging convenience only.
 */
adslock(base, count)
	caddr_t base;
{
#if 0
	register unsigned v;
	register int npf, ps = spl6();
	register struct pte *pte;

	u.u_procp->p_flag |= SDLYU;
	v = btop(base);
	pte = vtopte(u.u_procp, v);
	npf = btoc(count + ((int)base & CLOFSET));
	if(adsdebug)printf("Lk %x %d ",pte->pg_pfnum,npf);
	while (npf > 0) {
		if (pte->pg_v) 
			mlock(pte->pg_pfnum);
		else
			if (fubyte((caddr_t)ctob(v)) < 0)
				panic("vslock");
		pte += CLSIZE;
		v += CLSIZE;
		npf -= CLSIZE;
	}
	u.u_procp->p_flag &= ~SDLYU;

	splx(ps);
#else
	if (adsdebug) printf("Lk %x %d ",
				vtopte(u.u_procp, btop(base))->pg_pfnum,
				btoc(count + ((int)base & CLOFSET)));
	vslock(base, count);
#endif
}
# endif
