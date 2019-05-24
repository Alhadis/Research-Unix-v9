/*	@(#)sd.c 1.1 86/02/03 SMI	*/

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
 * SCSI driver for SCSI disks.
 */

#include "sd.h"
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dk.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/vmmac.h"
/*
 *#include "../h/ioctl.h"
 *#include "../h/uio.h"
 *#include "../h/kernel.h"
 */
#include "../h/dkbad.h"

#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"

#include "../sun/dklabel.h"
#include "../sun/dkio.h"

#include "../sundev/mbvar.h"
#include "../sundev/screg.h"
#include "../sundev/sireg.h"
#include "../sundev/scsi.h"

#define	MAX_RETRIES  3
#define	MAX_RESTORES 1

#define	LPART(dev)	(dev % NLPART)
#define	SDUNIT(dev)	((dev >> 3) % NUNIT)
#define	SDNUM(un)	(un - sdunits)

#define b_cylin b_resid
#define	SECSIZE	512

#define BUSY_RETRY 1000

/*
 * Error message control.
 */
#define	EL_RETRY	3
#define EL_REST		2
#define	EL_FAIL		1
int sderrlvl = EL_RETRY;

extern struct scsi_unit sdunits[];
extern struct scsi_unit_subr scsi_unit_subr[];
extern struct scsi_disk sdisk[];
extern int nsdisk;

/*
 * Return a pointer to this unit's unit structure.
 */
sdunitptr(md)
	register struct mb_device *md;
{
	return ((int)&sdunits[md->md_unit]);
}

/*
 * Attach device (boot time).
 */
sdattach(md)
	register struct mb_device *md;
{
	register struct scsi_unit *un;
	register struct dk_label *l;
	register int nbusy;

	un = &sdunits[md->md_unit];
	un->un_md = md;
	un->un_mc = md->md_mc;
	un->un_unit = md->md_unit;
	un->un_target = TARGET(md->md_slave);
	un->un_lun = LUN(md->md_slave);
	un->un_ss = &scsi_unit_subr[TYPE(md->md_flags)];

	nbusy = 0;
	for (;;) {
		if (simple(un, SC_TEST_UNIT_READY, 0, 0, 0)) {
			break;
		} else if (un->un_c->c_scb.busy && nbusy++ < BUSY_RETRY) {
			DELAY(5000);
			continue;
		} else {
			if (nbusy >= BUSY_RETRY) {
				printf("sd%d: scsi continuously busy\n",
				    SDNUM(un));
			}
			return;
		}
	}
	/* Allocate space for label in Multibus memory */
	l = (struct dk_label *)rmalloc(iopbmap, (long)SECSIZE);
	if (l == NULL) {
		printf("sd%d: sdattach: no space for disk label\n",
		    SDNUM(un));
		return;
	}
	l->dkl_magic = 0;
	if (getlabel(un, l) && islabel(un, l)) {
		uselabel(un, l);
	}
	rmfree(iopbmap, (long)SECSIZE, (long)l);
}

static
getlabel(un, l)
	register struct scsi_unit *un;
	register struct dk_label *l;
{
	register int retries, restores;

	for (restores = 0; restores < MAX_RESTORES; restores++) {
		for (retries = 0; retries < MAX_RETRIES; retries++) {
			if (simple(un, SC_READ, (char *) l - DVMA, 0, 1)) {
				return (1);
			}
		}
		(void) simple(un, SC_REZERO_UNIT, 0, 0, 0);
	}
	return (0);
}

static
islabel(un, l)
	register struct scsi_unit *un;
	register struct dk_label *l;
{
	if (l->dkl_magic != DKL_MAGIC) {
		return (0);
	}
	if (!ck_cksum(l)) {
		printf("sd%d: corrupt label\n", SDNUM(un));
		return (0);
	}
	return (1);
}

/*
 * Check the checksum of the label
 */
static
ck_cksum(l)
	register struct dk_label *l;
{
	short *sp, sum = 0;
	short count = sizeof(struct dk_label)/sizeof(short);

	sp = (short *)l;
	while (count--)  {
		sum ^= *sp++;
	}
	return (sum ? 0 : 1);
}

static
uselabel(un, l)
	register struct scsi_unit *un;
	register struct dk_label *l;
{
	register int i, intrlv;
	register struct scsi_disk *dsi;

	printf("sd%d: <%s>\n", SDNUM(un), l->dkl_asciilabel);
	un->un_present = 1;
	dsi = &sdisk[un->un_unit];
	dsi->un_g.dkg_ncyl = l->dkl_ncyl;
	dsi->un_g.dkg_bcyl = 0;
	dsi->un_g.dkg_nhead = l->dkl_nhead;
	dsi->un_g.dkg_bhead = l->dkl_bhead;
	dsi->un_g.dkg_nsect = l->dkl_nsect;
	dsi->un_g.dkg_gap1 = l->dkl_gap1;
	dsi->un_g.dkg_gap2 = l->dkl_gap2;
	dsi->un_g.dkg_intrlv = l->dkl_intrlv;
	for (i = 0; i < NLPART; i++)
		dsi->un_map[i] = l->dkl_map[i];
	if (un->un_md->md_dk >= 0) {
		intrlv = dsi->un_g.dkg_intrlv;
		if (intrlv <= 0 || intrlv >= dsi->un_g.dkg_nsect) {
			intrlv = 1;
		}
/*
		dk_bps[un->un_md->md_dk] = SECSIZE*60*dsi->un_g.dkg_nsect/intrlv;
*/
	}
}

static
simple(un, cmd, dma_addr, secno, nsect)
	register struct scsi_unit *un;
	register int cmd, dma_addr, secno, nsect;
{
	register struct scsi_cdb *cdb;
	register struct scsi_ctlr *c;
	register int count;

	c = un->un_c;
	cdb = &c->c_cdb;
	bzero((caddr_t)cdb, sizeof(struct scsi_cdb));
	cdb->cmd = cmd;
	c->c_un = un;
	cdb->lun = un->un_lun;
	cdbaddr(cdb, secno);
	cdb->count = nsect;
	un->un_dma_addr = dma_addr;
	un->un_dma_count = nsect * SECSIZE;
	if ((*c->c_ss->scs_cmd)(c, un, 0)) {
		if ((*c->c_ss->scs_cmd_wait)(c)) {
			count = (*c->c_ss->scs_dmacount)(c);
			* (char *) &c->c_scb = 0;
			if ((*c->c_ss->scs_getstat)(un, 0)) {
				if (count == 0) {
					if (c->c_scb.chk == 0 && 
					    c->c_scb.busy == 0) {
						return (1);
					}
				}
			}
		}
	}
	return (0);
}

/*ARGSUSED*/
sdopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct scsi_unit *un;
	register struct dk_label *l;
	register int unit;
	register struct scsi_disk *dsi;
	int memall();

	unit = SDUNIT(dev);
	if (unit >= nsdisk) {
		u.u_error = ENXIO;
		return;
	}
	un = &sdunits[unit];
	dsi = &sdisk[unit];
	if (un->un_mc == 0) {	/* never attached */
		u.u_error = ENXIO;
		return;
	}
	if (!un->un_present) {
		dsi->un_g.dkg_nsect = dsi->un_g.dkg_nhead = 1;   /* for strat */
		if (sdcmd(dev, SC_TEST_UNIT_READY, 0, 0, (caddr_t)0)) {
			u.u_error = EIO;
			return;
		}
		/* Allocate space for label */
		l = (struct dk_label *)wmemall(memall, SECSIZE);
		if (l == NULL) {
			printf("sd%d: no space for disk label\n",
			    SDNUM(un));
			u.u_error = EIO;
			return;
		}
		if (sdcmd(dev, SC_READ, 0, SECSIZE, (caddr_t)l)) {
			uprintf("sd%d: error reading label\n", SDNUM(un));
			wmemfree((caddr_t)l, SECSIZE);
			u.u_error = EIO;
			return;
		}
		if (islabel(un, l)) {
			uselabel(un, l);
		} else {
			wmemfree((caddr_t)l, SECSIZE);
			u.u_error = EIO;
			return;
		}
		wmemfree((caddr_t)l, SECSIZE);
	}
	return (0);
}

sdsize(dev)
	register dev_t dev;
{
	register struct scsi_unit *un;
	register struct dk_map *lp;
	register struct scsi_disk *dsi;

	un = &sdunits[SDUNIT(dev)];
	if (!un->un_present) {
		return (-1);
	}
	dsi = &sdisk[SDUNIT(dev)];
	lp = &dsi->un_map[LPART(dev)];
	return (lp->dkl_nblk);
}

sdstrategy(bp)
	register struct buf *bp;
{
	register struct scsi_unit *un;
	register struct dk_map *lp;
	register daddr_t bn;
	register int unit;
	register struct buf *dp;
	register struct scsi_disk *dsi;
	long sz;
	int s;

	sz = bp->b_bcount;
	sz = (sz+511) >> 9;
	unit = dkunit(bp);
	if (unit >= nsdisk)
		goto bad;
	un = &sdunits[unit];
	if (!un->un_present && bp != &un->un_sbuf)
		goto bad;
	dsi = &sdisk[unit];
	lp = &dsi->un_map[LPART(bp->b_dev)];
	if (bp->b_blkno < 0 || (bn = dkblock(bp))+sz > lp->dkl_nblk)
		goto bad;
	bp->b_cylin = bn / (dsi->un_g.dkg_nsect * dsi->un_g.dkg_nhead);
	bp->b_cylin += lp->dkl_cylno + dsi->un_g.dkg_bcyl;
	dp = &un->un_utab;
	s = splx(pritospl(un->un_mc->mc_intpri));
	disksort(dp, bp);
	if (dp->b_active == 0) {
		(*un->un_c->c_ss->scs_ustart)(un);
		bp = &un->un_mc->mc_tab;
		if (bp->b_actf && bp->b_active == 0) {
			(*un->un_c->c_ss->scs_start)(un);
		}
	}
	(void) splx(s);
	return;

bad:
	bp->b_flags |= B_ERROR;
	iodone(bp);
	return;
}

/*
 * Do a special command.
 */
sdcmd(dev, cmd, sector, len, addr)
	register dev_t dev;
	register int cmd, sector, len;
	register caddr_t addr;
{
	register struct scsi_unit *un;
	register struct buf *bp;
	register int s;

	un = &sdunits[SDUNIT(dev)];
	bp = &un->un_sbuf;
	s = splx(pritospl(un->un_mc->mc_intpri));
	while (bp->b_flags&B_BUSY) {
		bp->b_flags |= B_WANTED;
		sleep((caddr_t)bp, PRIBIO);
	}
	bp->b_flags = B_BUSY|B_READ;
	(void) splx(s);
	un->un_scmd = cmd;
	bp->b_dev = dev;
	bp->b_blkno = sector;
	bp->b_un.b_addr = addr;
	bp->b_bcount = len;
	sdstrategy(bp);
	iowait(bp);
	bp->b_flags &= ~B_BUSY;
	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);
	return (bp->b_flags & B_ERROR);
}

/*
 * Set up a transfer for the controller
 */
sdstart(bp, un)
	register struct buf *bp;
	register struct scsi_unit *un;
{
	register struct dk_map *lp;
	register int nblk;
	register struct scsi_disk *dsi;

	dsi = &sdisk[dkunit(bp)];
	lp = &dsi->un_map[LPART(bp->b_dev)];
	un->un_blkno = dkblock(bp) + 
		lp->dkl_cylno * dsi->un_g.dkg_nhead * dsi->un_g.dkg_nsect;
	if (bp == &un->un_sbuf) {
		un->un_cmd = un->un_scmd;
	} else if (bp->b_flags & B_READ) {
		un->un_cmd = SC_READ;
	} else {
		un->un_cmd = SC_WRITE;
	}
	if (un->un_cmd == SC_READ || un->un_cmd == SC_WRITE) {
		nblk = howmany(bp->b_bcount, SECSIZE);
		un->un_count = MIN(nblk, lp->dkl_nblk - bp->b_blkno);
		bp->b_resid = bp->b_bcount - un->un_count * SECSIZE;
		un->un_flags |= SC_UNF_DVMA;
	} else {
		un->un_count = 0;
	}
	return (1);
}

/*
 * Make a cdb for disk i/o.
 */
sdmkcdb(c, un)
	register struct scsi_ctlr *c;
	struct scsi_unit *un;
{
	register struct scsi_cdb *cdb;

	cdb = &c->c_cdb;
	bzero((caddr_t)cdb, sizeof (*cdb));
	cdb->cmd = un->un_cmd;
	cdb->lun = un->un_lun;
	cdbaddr(cdb, un->un_blkno);
	cdb->count = un->un_count;
	un->un_dma_addr = un->un_baddr;
	un->un_dma_count = un->un_count * SECSIZE;
}

/*
 * Interrupt processing.
 */
sdintr(c, resid, error)
	register struct scsi_ctlr *c;
	register int resid, error;
{
	register struct scsi_unit *un;
	register struct buf *bp;
	register struct mb_device *md;

	un = c->c_un;
	bp = un->un_mc->mc_tab.b_actf->b_actf;
	md = un->un_md;
	if (md->md_dk >= 0) {
		dk_busy &= ~(1 << md->md_dk);
	}
	if (error == SE_FATAL) {
		if (bp == &un->un_sbuf  &&
		    ((un->un_flags & SC_UNF_DVMA) == 0)) {
			(*c->c_ss->scs_done)(un->un_mc);
		} else {
			mbdone(un->un_mc);
			un->un_flags &= ~SC_UNF_DVMA;
		}
		bp->b_flags |= B_ERROR;
		printf("sd%d: SCSI FAILURE\n", SDNUM(un));
		(*c->c_ss->scs_off)(un);
		return;
	}
	if (error == SE_RETRYABLE || c->c_scb.chk || resid > 0) {
		sderror(c, un, bp);
		return;
	}
	if (c->c_cdb.cmd == SC_REZERO_UNIT && 
	    !(bp == &un->un_sbuf && 
	    un->un_scmd == SC_REZERO_UNIT)) {
		/* error recovery */
		sdmkcdb(c, un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) == 0) {
			printf("sd%d sdintr: scsi cmd failed 1\n", SDNUM(un));
			(*c->c_ss->scs_off)(un);
		}
		return;
	}
	/* transfer worked */
	un->un_retries = un->un_restores = 0;
	if (un->un_sec_left) {	/* single sector stuff */
		un->un_sec_left--;
		un->un_baddr += SECSIZE;
		un->un_blkno++;
		sdmkcdb(c, un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) == 0) {
			printf("sd%d: sdintr: scsi cmd failed 2\n", SDNUM(un));
			(*c->c_ss->scs_off)(un);
		}
	} else if (bp == &un->un_sbuf  &&
	    ((un->un_flags & SC_UNF_DVMA) == 0)) {
		(*c->c_ss->scs_done)(un->un_mc);
	} else {
		mbdone(un->un_mc);
		un->un_flags &= ~SC_UNF_DVMA;
	}
}

/*
 * Error handling.
 */
sderror(c, un, bp)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register struct buf *bp;
{
	if (un->un_present == 0) {	/* error trying to open */
		bp->b_flags |= B_ERROR;
		if (bp == &un->un_sbuf &&
		    ((un->un_flags & SC_UNF_DVMA) == 0)) {
			(*c->c_ss->scs_done)(un->un_mc);
		} else {
			mbdone(un->un_mc);
			un->un_flags &= ~SC_UNF_DVMA;
		}
	} else if (c->c_scb.chk && c->c_sense->class == 1 &&
	    c->c_sense->code == 5 && un->un_count > 1) {
		/* Seek errors - try single sectors (Adaptec bug) */
		un->un_sec_left = un->un_count - 1;
		un->un_count = 1;
		sdmkcdb(c, un);
		if ((*c->c_ss->scs_cmd)(c, un, 1) == 0) {
			printf("sd%d: sderror: scsi cmd failed 1\n", SDNUM(un));
			(*c->c_ss->scs_off)(un);
		}
	} else if (un->un_retries++ < MAX_RETRIES) {
		/* retry */
		if (sderrlvl >= EL_RETRY) {
			sderrmsg(c, un, bp, "retry");
		}
		if ((*c->c_ss->scs_cmd)(c, un, 1) == 0) {
			printf("sd%d: sderror: scsi cmd failed 2\n", SDNUM(un));
			(*c->c_ss->scs_off)(un);
		}
	} else if (un->un_restores++ < MAX_RESTORES) {
		/* retries exhausted, try restore */
		un->un_retries = 0;
		if (sderrlvl >= EL_REST) {
			sderrmsg(c, un, bp, "restore");
		}
		c->c_cdb.cmd = SC_REZERO_UNIT;
		cdbaddr(&c->c_cdb, 0);
		c->c_cdb.count = 0;
		un->un_dma_addr = un->un_dma_count = 0;
		if ((*c->c_ss->scs_cmd)(c, un, 1) == 0) {
			printf("sd%d: sderror: scsi cmd failed 3\n", SDNUM(un));
			(*c->c_ss->scs_off)(un);
		}
	} else {
		/* complete failure */
		if (sderrlvl >= EL_FAIL) {
			sderrmsg(c, un, bp, "failed");
		}
		(*c->c_ss->scs_off)(un);
		bp->b_flags |= B_ERROR;
		if (bp == &un->un_sbuf &&
		    ((un->un_flags & SC_UNF_DVMA) == 0)) {
			(*c->c_ss->scs_done)(un->un_mc);
		} else {
			mbdone(un->un_mc);
			un->un_flags &= ~SC_UNF_DVMA;
		}
	}
}

sdread(dev)
	dev_t dev;
{
	sdrw(dev, B_READ);
}

sdrw(dev, direction)
	dev_t dev;
	int direction;
{
	register struct scsi_unit *un;
	register int unit;
	
	unit = SDUNIT(dev);
	if (unit >= nsdisk) {
		u.u_error = ENXIO;
	}
	un = &sdunits[unit];
	physio(sdstrategy, &un->un_rbuf, dev, direction, minphys);
}

sdwrite(dev)
	dev_t dev;
{
	sdrw(dev, B_WRITE);
}

/*ARGSUSED*/
sdioctl(dev, cmd, data, flag)
	register dev_t dev;
	register caddr_t data;
{
	register struct scsi_unit *un;
	register struct dk_map *lp;
	register struct dk_info *inf;
	register int unit;
	register struct scsi_disk *dsi;

	unit = SDUNIT(dev);
	if (unit >= nsdisk) {
	    	return (ENXIO);
	}
	un = &sdunits[unit];
	dsi = &sdisk[unit];
	lp = &dsi->un_map[LPART(dev)];
	switch (cmd) {

	case DKIOCINFO:
		inf = (struct dk_info *)data;
		inf->dki_ctlr = getdevaddr(un->un_mc->mc_addr);
		inf->dki_unit = un->un_md->md_slave;
		inf->dki_ctype = DKC_SCSI;
		inf->dki_flags = DKI_FMTVOL;
		break;

	case DKIOCGGEOM:
		*(struct dk_geom *)data = dsi->un_g;
		break;

	case DKIOCSGEOM:
		if (!suser())
			return (u.u_error);
		dsi->un_g = *(struct dk_geom *)data;
		break;

	case DKIOCGPART:
		*(struct dk_map *)data = *lp;
		break;

	case DKIOCSPART:
		if (!suser())
			return (u.u_error);
		*lp = *(struct dk_map *)data;
		break;

	default:
		return (ENOTTY);
	}
	return (0);
}

sddump(dev, addr, blkno, nblk)
	register dev_t dev;
	register caddr_t addr;
	register daddr_t blkno, nblk;
{
	register struct scsi_unit *un;
	register struct dk_map *lp;
	static int first_time = 1;
	register struct scsi_disk *dsi;

	un = &sdunits[SDUNIT(dev)];
	if (un->un_present == 0) {
		return (ENXIO);
	}
	dsi = &sdisk[SDUNIT(dev)];
	lp = &dsi->un_map[LPART(dev)];
	if (blkno >= lp->dkl_nblk || (blkno + nblk) > lp->dkl_nblk) {
		return (EINVAL);
	}
	blkno += lp->dkl_cylno * dsi->un_g.dkg_nhead * dsi->un_g.dkg_nsect;
	if (first_time) {
		(*un->un_c->c_ss->scs_reset)(un->un_c); /* clr state - prevent err msg */
		first_time = 0;
	}
	if (simple(un, SC_WRITE, (int)(addr-DVMA), (int) blkno, (int) nblk)) {
		return (0);
	} else {
		return (EIO);
	}
}

char	*class_00_errors[] = {
	"no sense",
	"no index signal",
	"no seek complete",
	"write fault",
	"drive not ready",
	"drive not selected",
	"no track 00",
	"multiple drives selected",
	"no address acknowledged",
	"media not loaded",
	"insufficient capacity",
};

char	*class_01_errors[] = {
	"I.D. CRC error",
	"unrecoverable data error",
	"I.D. address mark not found",
	"data address mark not found",
	"record not found",
	"seek error",
	"DMA timeout error",
	"write protected",
	"correctable data check",
	"bad block found",
	"interleave error",
	"data transfer incomplete",
	"unformatted or bad format on drive",
	"self test failed",
	"defective track (media errors)",
};

char	*class_02_errors[] = {
	"invalid command",
	"illegal block address",
	"aborted",
	"volume overflow",
};

char	**sc_errors[] = {
	class_00_errors,
	class_01_errors,
	class_02_errors,
	0, 0, 0, 0,
};

int	sc_errct[] = {
	sizeof class_00_errors / sizeof (char *),
	sizeof class_01_errors / sizeof (char *),
	sizeof class_02_errors / sizeof (char *),
	0, 0, 0, 0,
};

char	*sc_ext_sense_keys [] = {
	"no sense",
	"recoverable error",
	"not ready",
	"media error",
	"hardware error",
	"illegal request",
	"media change",
	"write protect",
	"diagnostic unique",
	"vendor unique",
	"power up failed",
	"aborted command",
	"equal",
	"volume overflow",
};

#define N_EXT_SENSE_KEYS \
	(sizeof(sc_ext_sense_keys)/sizeof(sc_ext_sense_keys[0]))

char *sd_cmds[] = {
	"test unit ready",
	"rezero unit",
	"<bad cmd>",
	"request sense",
	"<bad cmd>",
	"<bad cmd>",
	"<bad cmd>",
	"<bad cmd>",
	"read",
	"<bad cmd>",
	"write",
	"seek",
};

sderrmsg(c, un, bp, action)
	register struct scsi_ctlr *c;
	struct scsi_unit *un;
	struct buf *bp;
	char *action;
{
	char *sensemsg, *cmdname;
	register struct scsi_sense *sense;
#define	ext_sense	((struct scsi_ext_sense* ) sense)
	register struct dk_map *lp;
	register int blkno;
	register struct scsi_disk *dsi;

	dsi = &sdisk[dkunit(bp)];
	sense = c->c_sense;
	if (c->c_scb.chk == 0) {
		sensemsg = "no sense";
	} else if (sense->class <= 6) {
		if (sense->code < sc_errct[sense->class]) {
			sensemsg = sc_errors[sense->class][sense->code];
		} else {
			sensemsg = "invalid sense code";
		}
	} else if (sense->class == 7) {
		if (ext_sense->key < N_EXT_SENSE_KEYS) {
			sensemsg = sc_ext_sense_keys[ext_sense->key];
		} else {
			sensemsg = "invalid sense code";
		}
	} else {
		sensemsg = "invalid sense class";
	}
	if (un->un_cmd < sizeof(sd_cmds)) {
		cmdname = sd_cmds[un->un_cmd];
	} else {
		cmdname = "unknown cmd";
	}
	blkno = (sense->high_addr << 16) | (sense->mid_addr << 8) |
	    sense->low_addr;
	lp = &dsi->un_map[LPART(bp->b_dev)];
	blkno -= lp->dkl_cylno * dsi->un_g.dkg_nhead * dsi->un_g.dkg_nsect;
	printf("sd%d%c: %s %s (%s) blk %d\n", SDNUM(un),
	    LPART(bp->b_dev) + 'a', cmdname, action, sensemsg, blkno);
}
