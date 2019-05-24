#include "me.h"
/*
 * various metheus models on dr11-w, hacked from rob's raster tech driver
 * td 86.09.13
 * Bugs:
 *	should support the cursor
 */
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/pte.h"
#include "../h/map.h"
#include "../h/ubareg.h"
#include "../h/ubavar.h"
struct meregs{
	short	wcr;		/* Unibus word count reg */
	u_short	bar;		/* Unibus address register */
	u_short	csr;		/* Unibus status/command reg */
	u_short	dr;		/* Data Register */
};
#define	eir	csr		/* Error and Information Register */
/*
 * Unibus status/command register bits
 */
#define GO		0000001
#define	FUNC1		0000002	/* if set, a dma reads rather than writes */
#define	FUNC2		0000004
#define	FUNC3		0000010
#define IENABLE		0000100
#define READY		0000200
#define	MAINT		0010000
#define	ATTN		0020000
#define	ERROR		0100000
/*
 * setting FUNC1 causes metheus to assert attention,
 * abort any dma in progress, cause an error and
 * interrupt (if enabled)
 */
#define	ABORT	FUNC3

#define DMAPRI (PZERO-1)
#define WAITPRI (PZERO+1)

int	meprobe(), meattach(), meintr();
struct	uba_device *medinfo[NME];
u_short	mestd[] = { 0772410, 0772430, 0, 0 };
struct	uba_driver medriver = {
	meprobe, 0, meattach, 0, mestd, "me", medinfo, 0, 0
};

struct mesoftc {
	char	open;		/* flag for device open */
	char	busy;		/* flag for io in progress */
	int	ubinfo;		/* ubasetup/ubarelse datum */
	int	count;		/* io byte count */
	struct	buf *bp;	/* io buffer header */
	int	bufp;		/* io memory address, in UNIBUS coordinates */
	int	icnt;		/* interrupt count */
} mesoftc[NME];
int	mestrategy();
u_int	meminphys();
struct	buf rmebuf[NME];
#define UNIT(dev) (minor(dev))
meprobe(reg)
	caddr_t reg;
{
	register int br, cvec;		/* value-result */
	register struct meregs *meaddr = (struct meregs *)reg;

#ifdef lint
	br = 0; cvec = br; br = cvec;
#endif
	meaddr->csr=ATTN|ABORT|IENABLE;
	DELAY(10000);
	meaddr->csr=IENABLE;
	/* grumble */
	br = 0x15;
	switch ((int)meaddr&077){
	case 010: cvec=0124; break;
	case 030: cvec=0134; break;
	}
	return sizeof(struct meregs);
}
/*ARGSUSED*/
meattach(ui)
struct uba_device *ui;
{
}
meopen(dev)
dev_t dev;
{
	register struct mesoftc *mep;
	register struct uba_device *ui;
	if(UNIT(dev)>=NME
	|| (mep=&mesoftc[minor(dev)])->open
	|| (ui=medinfo[UNIT(dev)])==0
	|| ui->ui_alive==0)
		u.u_error=EBUSY;
	else{
		mep->open=1;
		mep->icnt=0;
		mep->busy=0;
	}
}
meclose(dev)
dev_t dev;
{
	mesoftc[minor(dev)].open=0;
	mesoftc[minor(dev)].busy=0;
}
meread(dev)
dev_t dev;
{
	return merdwr(dev, B_READ);
}
mewrite(dev)
dev_t dev;
{
	return merdwr(dev, B_WRITE);
}
merdwr(dev, flag)
dev_t dev;
{
	register int unit=UNIT(dev);
	if (unit>=NME)
		return ENXIO;
	return physio(mestrategy, &rmebuf[unit], dev, flag, meminphys);
}
u_int meminphys(bp)
register struct buf *bp;
{
	if(bp->b_bcount>32768)	/* what's the actual upper bound? */
		bp->b_bcount=32768;
}
mestrategy(bp)
register struct buf *bp;
{
	register struct mesoftc *mep = &mesoftc[UNIT(bp->b_dev)];
	register struct uba_device *ui;
	if(UNIT(bp->b_dev)>=NME
	|| (ui=medinfo[UNIT(bp->b_dev)])==0
	|| ui->ui_alive==0)
		goto Bad;
	spl5();
	while(mep->busy)
		sleep((caddr_t)mep, DMAPRI+1);
	mep->busy++;
	mep->bp=bp;
	if(bp->b_bcount<=0
	|| bp->b_bcount&1
	|| (int)bp->b_un.b_addr&1){
		u.u_error=EINVAL;
		goto Bad;
	}
	mep->ubinfo=ubasetup(ui->ui_ubanum, bp, UBA_NEEDBDP);
	mep->bufp=mep->ubinfo&0x3ffff;
	mep->count=-(bp->b_bcount>>1);	/* it's a word count */
	mestart(ui);
	if(tsleep((caddr_t)mep, DMAPRI+1, 15)!=TS_OK){
		register struct meregs *meaddr=(struct meregs *)ui->ui_addr;
		bp->b_flags|=B_ERROR;
		/* stop the dma */
		meabort(meaddr);
#ifdef deleted
		meaddr->wcr=0xFFFF;	/* gently; -1 first... */
		meaddr->wcr=0x0000;	/* then zero */
		/* reset device */
		meabort(meaddr);
#endif
		/* fake an interrupt to clear software status */
		meintr(UNIT(bp->b_dev));
	}
	mep->count=0;
	mep->bufp=0;
	(void) spl0();
	ubarelse(ui->ui_ubanum, &mep->ubinfo);
	mep->bp=0;
	iodone(bp);
	wakeup((caddr_t)mep);
	return;
Bad:
	mep->busy=0;
	bp->b_flags|=B_ERROR;
	iodone(bp);
}
meabort(meaddr)
register struct meregs *meaddr;
{
	meaddr->csr=ATTN|ABORT|IENABLE;
	DELAY(100);
	meaddr->csr=IENABLE;
	DELAY(100);
}
mestart(ui)
register struct uba_device *ui;
{
	register int csr;
	register struct meregs *meaddr = (struct meregs *) ui->ui_addr;
	register struct mesoftc *mep = &mesoftc[UNIT(ui->ui_unit)];
	csr=IENABLE|((mep->bufp>>12)&060);
	if(mep->bp->b_flags&B_WRITE)
		csr|=FUNC1;
	meaddr->wcr=mep->count;
	meaddr->bar=mep->bufp;
	meaddr->csr=csr;	/* No GO bit; let function codes settle */
	meaddr->csr=csr|GO;
}
/*ARGSUSED*/
meioctl(dev, cmd, data)
dev_t dev;
int cmd;
register caddr_t data;
{
	return ENOTTY;
}
/*ARGSUSED*/
meintr(dev)
dev_t dev;
{
	register struct meregs *meaddr=
			(struct meregs *)medinfo[UNIT(dev)]->ui_addr;
	register struct mesoftc *mep=&mesoftc[UNIT(dev)];
	register csr;
	mep->icnt++;
	if(mep->busy){
		csr=meaddr->csr;
		if(csr&ERROR){
			meaddr->eir|=ERROR;
			printf("me: csr %x eir %x\n", csr, meaddr->eir);
		}
		meaddr->csr=csr&~(FUNC1|FUNC2|FUNC3);
		mep->busy=0;
		wakeup((caddr_t)mep);
	}
}
