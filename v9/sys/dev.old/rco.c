/*	rco.c		86/03/10	*/

#include "rco.h"
#if NRCO > 0
/*
 * UNIBUS DS11-A driver for Ricoh scanner
 *
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/cpu.h"
#include "../h/nexus.h"
#include "../h/dk.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/map.h"
#include "../h/pte.h"
#include "../h/mtpr.h"
#include "../h/vm.h"
#include "../h/ubavar.h"
#include "../h/ubareg.h"
#include "../h/cmap.h"

#include "../h/rcocmd.h"

struct rcodevice{
	u_short rco_csr;	/* control/status register */
	u_short rco_vr;		/* virtual register (write only)*/
	u_short rco_ar;		/* address register */
	u_short rco_xar;	/* extended address register */
};
struct rcosoftc {
	u_short resol;		/* resolution*/
	u_short dither;		/* 1=dither on */
	int rco_reg;		/* index into uba_map */
	int rco_which;
	u_short rco_offset;	/* offset from 2k bdry */
	caddr_t rcob_addr;
	unsigned rco_bdp;	/* bits to or into pte for ubareg update*/
	unsigned rco_ct;	/* interrupt counter */
} rcosoftc[NRCO];

#define	RCO_GO	01
#define RCO_FAULT	02
#define RCO_TIMEOT	04
#define RCO_OVERF	010
#define RCO_ERROR	016
#define RCO_RESL	060
#define RCO_HALF	0100
#define RCO_RDY		0200
#define RCO_UBACT	07400
#define RCO_UBALOW	010000
#define RCO_TEOP	020000
#define	RCO_IE		040000
#define RCO_IRQST	0100000

#define RCO_AR(x)	(x & 03776)	/* bits 9&10 of map reg + 1-8 of offset */
#define RCO_XA(x)	((x & 017774000) >> 8) /* bits 17 to 11 of map reg */
#define RCO_REG(x)	((x & 0774000) >> 9)
#define RCO_RES(x)	((x << 4) & RCO_RESL)
#define RCO_DITH(x)	((x << 6) & RCO_HALF)

#define RCOSIZE 512*4
#define RCOPAD	512*4
#define RCOMASK (RCOPAD-1)


int	rcoprobe(), rcoattach(), rcodgo(), rcointr();
int	rcostrategy(), rcostart();
u_int rcominp();
struct	uba_ctlr *rcominfo[NRCO];
struct	uba_device *rcodinfo[NRCO];
struct	uba_ctlr rcoctlr[NRCO];	/* rcominfo points to this */
u_short rcostd[]	= { 0774000, 0};	
struct	uba_driver rcodriver =
	{ rcoprobe,0,rcoattach,rcodgo,rcostd,"rco",rcodinfo,"rco",rcominfo };

#define ui_open	ui_type
struct	buf	rcobuf[NRCO];
struct	buf	rcoutab[NRCO];




/*ARGSUSED*/
rcoprobe(reg)
caddr_t reg;
{
	register int br, cvec;	/*int vec at XXX */
#ifdef LINT
	br = 0; cvec = br; br = cvec;
#endif

	/* can't make this thing interrupt */
	br = 0x15;
	cvec = 0150;
	return(1);
}

rcoattach(ui)
register struct uba_device *ui;
{
	register struct uba_ctlr *um;
	register int unit;

	unit = ui->ui_unit;

	um = &rcoctlr[unit];
	rcominfo[unit] = um;
	ui->ui_ctlr = unit;
	ui->ui_mi = um;
	um->um_driver = ui->ui_driver;
	um->um_ctlr = unit;
	um->um_ubanum = ui->ui_ubanum;
	um->um_alive = 1;
	um->um_intr = ui->ui_intr;
	um->um_addr = ui->ui_addr;
	um->um_hd = ui->ui_hd;
}

rcoopen(dev)
dev_t dev;
{
	register int unit = minor(dev);
	register struct rcosoftc *rcop = &rcosoftc[minor(dev)];
	register struct uba_device *ui = rcodinfo[unit];
	register struct rcodevice *draddr = (struct rcodevice *)ui->ui_addr;

	if((unit >= NRCO) || (rcodinfo[unit]->ui_open)) {
		u.u_error = ENXIO;
		return;
	}
	rcodinfo[unit]->ui_open++;
	draddr->rco_csr = 0;
	rcop->dither = 0;
	rcop->resol = 2;
	rcop->rco_ct = 0;
}

rcoclose(dev)
dev_t dev;
{
	register int unit;

	rcodinfo[minor(dev)]->ui_open = 0;
}

rcoread(dev)
dev_t dev;
{
	register int unit = minor(dev);
	physio(rcostrategy,&rcobuf[unit],dev,B_READ,rcominp);

}
u_int
rcominp(bp)
struct buf *bp;
{
}

/*
 * Due to the fact the scstrategy routine is called only by rcoread
 * via physio, there will only be one transaction in each
 * DS11A's queue at any time.  Therefore, one can just tack the given
 * buffer header pointer on at the end of the queue, and call rcostart.
 */
rcostrategy(bp)
register struct buf *bp;
{
	register struct uba_device *ui;
	register struct uba_ctlr *um;
	register struct buf *dp;
	register int s;
	struct rcodevice *draddr;
	dev_t unit;

	unit = minor(bp->b_dev);	/* chose a DS11A */
	ui = rcodinfo[unit];
	um = ui->ui_mi;		/* get ctlr ptr */
	dp = &rcoutab[unit];
	s = spl5();
	dp->b_actf = bp;
	dp->b_actl = bp;
	bp->av_forw = NULL;
	um->um_tab.b_actf = dp;
	um->um_tab.b_actl = dp;
	rcostart(um);
	splx(s);	/*dennis says this should go away*/
	switch(tsleep((caddr_t)bp, PRIBIO+1, 20)) {
	case TS_OK:
		break;
	case TS_SIG:	/* not supposed to happen*/
	case TS_TIME:
		draddr = (struct rcodevice *)um->um_addr;
		bp->b_flags |=  B_ERROR;
		s = spl6();	/* if other goes this can to*/
		printf("timeout calling rcointr\n");
		rcointr(unit);
		splx(s);	/*likewise this*/
	}
/*	splx(s);	this is where dennis thinks it should be*/

}

rcostart(um)
register struct uba_ctlr *um;
{
	register struct buf *bp,*dp;
	register struct rcodevice *draddr;
	register struct rcosoftc *rcop;
	int cmd;

	dp = um->um_tab.b_actf;
	bp = dp->b_actf;
	rcop = &rcosoftc[minor(bp->b_dev)];

	um->um_tab.b_active++;
	draddr = (struct rcodevice *)um->um_addr;
	if(bp->b_flags & B_READ)
		cmd = RCO_IE;	
	else{
		u.u_error = EFAULT;
	}
	um->um_cmd = cmd | RCO_DITH(rcop->dither) | RCO_RES(rcop->resol) | RCO_GO;

	bp->b_bcount = RCOSIZE + RCOPAD;

	DELAY(10);
	if( ubago(rcodinfo[minor(bp->b_dev)])== 0)
		printf("ubago returned 0\n");
}

rcodgo(um)
struct uba_ctlr *um;
{
	register struct rcodevice *draddr = (struct rcodevice *)um->um_addr;
	register unsigned addr, temp;
	register struct rcosoftc *rcop;
	struct buf *bp, *dp;


	dp = um->um_tab.b_actf;
	bp = dp->b_actf;
	rcop = &rcosoftc[minor(bp->b_dev)];

	addr = um->um_ubinfo;
	addr = addr + (RCOSIZE-1) & ~(RCOSIZE-1);
	draddr->rco_ar = RCO_AR(addr);
	draddr->rco_xar = RCO_XA(addr);
	printf("um %o addr %o ar %o xar %o\n", um->um_ubinfo,addr,RCO_AR(addr),
		RCO_XA(addr));

	rcop->rco_reg = RCO_REG(addr);
	rcop->rco_which = 0;
	rcop->rco_offset = (addr - um->um_ubinfo) & (RCOSIZE-1);

	rcop->rcob_addr = bp->b_un.b_addr + rcop->rco_offset + RCOSIZE;
	temp = UBAI_BDP(addr);
	rcop->rco_bdp = (temp << UBAMR_DPSHIFT) | UBAMR_MRV;
	printf("reg %o offset %o addr %o temp %o bdp %o\n",rcop->rco_reg,
		rcop->rco_offset,rcop->rcob_addr,temp,rcop->rco_bdp);
	for(temp=0;temp<4;temp++)
		draddr->rco_vr = 1;
	while(1){
		temp = draddr->rco_csr;
		if(temp & RCO_RDY)
			break;
	}
	printf("temp= %o about to set go %o\n",temp,um->um_cmd);
	draddr->rco_csr = um->um_cmd;
}

rcoioctl(dev, cmd, arg)
dev_t dev;
int cmd;
register caddr_t arg;
{
	register struct rcosoftc *rcop = &rcosoftc[minor(dev)];
	register struct uba_device *ui = rcodinfo[minor(dev)];
	register struct rcodevice *draddr = (struct rcodevice *)ui->ui_addr;
	register struct uba_ctlr *um;
	u_short realcmd;
	u_short stat;
	if(cmd != RCOHACK){
		if(copyin(arg, (caddr_t)&realcmd, sizeof(realcmd))){
			u.u_error = EFAULT;
			return;
		}
	}
	switch(cmd){
	case RCOHACK:
		realcmd = rcop->rco_offset;
		break;
	case RCORES:
		rcop->resol = realcmd;
		return;
	case RCODITHER:
		rcop->dither = realcmd;
		return;
	default:
		u.u_error = ENXIO;
		return;
	}
	if(copyout((caddr_t)&realcmd, arg, sizeof(realcmd)))
		u.u_error = EFAULT;
}
rcoreset() {}

rcointr(dr11)
register dr11;
{
	register struct buf *bp,*dp;
	register struct rcodevice *draddr;
	register struct uba_ctlr *um;
	register unsigned stat, reg,temp;
	u_short bit;
	struct pte *io, *pte;
	struct rcosoftc *rcop;
	um = rcominfo[dr11];
	rcop = &rcosoftc[dr11];
	rcop->rco_ct++;
	if(um->um_tab.b_active == 0)
		return;

	if(rcodinfo[dr11]->ui_open == 0)
		return;
	dp = um->um_tab.b_actf;
	bp = dp->b_actf;
	draddr = (struct rcodevice *)um->um_addr;
	stat = draddr->rco_csr;
	if(stat & RCO_GO){
		reg = rcop->rco_reg + rcop->rco_which++;
		if(rcop->rco_which > 3)
			rcop->rco_which = 0;

		stat = btop(rcop->rcob_addr);
		pte = vtopte(bp->b_proc, stat);
		rcop->rcob_addr += NBPG;

		io = &um->um_hd->uh_uba->uba_map[reg];
		if(pte->pg_pfnum == 0)
			*(int *)io = pte->pg_pfnum;
		else *(int *)io = pte->pg_pfnum | rcop->rco_bdp;
		draddr->rco_vr = 1;
		return;
	}
	if(stat & RCO_ERROR){
		printf("scanner error %o ct= %d\n",stat, rcop->rco_ct);
		goto rcobad;
	}
/* must be end of page */
/*	if(!(stat & RCO_TEOP))goto rcodone;*/
rcodone:
rcobad:
	draddr->rco_csr = 0;
	um->um_tab.b_active = 0;
	um->um_tab.b_errcnt = 0;
	um->um_tab.b_actf = dp->b_forw;
	dp->b_errcnt = 0;
	dp->b_active = 0;
	bp->b_resid = 0;
	ubadone(um);
	iodone(bp);
}
#endif
