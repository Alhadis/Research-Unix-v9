/*	@(#)st.c 1.1 86/02/03 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "st.h"
#if NST > 0

/*
 * Driver for Sysgen SC400 and Emulex MT02 SCSI tape controller.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/buf.h"
#include "../h/dir.h"
#include "../h/conf.h"
#include "../h/user.h"
#include "../h/file.h"
#include "../h/map.h"
#include "../h/vm.h"
#include "../h/mtio.h"
#include "../h/cmap.h"

#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "../machine/psl.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../sundev/mbvar.h"
#include "../sundev/screg.h"
#include "../sundev/sireg.h"
#include "../sundev/scsi.h"
#include "../sundev/streg.h"

#define	INF		1000000000
/*
 * Max # of buffers outstanding per unit.
 */
#define	MAXSTBUF	3

/*
 * Bits in minor device.
 */
#define	STUNIT(dev)	(minor(dev) & 03)
#define	T_NOREWIND	04
#define QIC_24		8

extern struct scsi_unit stunits[];
extern struct scsi_unit_subr scsi_unit_subr[];
extern struct scsi_tape stape[];
extern int nstape;

/*
 * Return a pointer to this unit's unit structure.
 */
stunitptr(md)
	register struct mb_device *md;
{
	return ((int)&stunits[md->md_unit]);
}

/*
 * Attach device (boot time).
 */
stattach(md)
	register struct mb_device *md;
{
	register struct scsi_unit *un;
	register struct scsi_tape *dsi;

	un = &stunits[md->md_unit];
	dsi = &stape[md->md_unit];
	un->un_md = md;
	un->un_mc = md->md_mc;
	un->un_unit = md->md_unit;
	un->un_target = TARGET(md->md_slave);
	un->un_lun = LUN(md->md_slave);
	un->un_ss = &scsi_unit_subr[TYPE(md->md_flags)];
	dsi->un_ctype = ST_TYPE_INVALID;
	dsi->un_openf = CLOSED;
}

stinit(dev)
	dev_t dev;
{
	register int unit;
	register struct scsi_tape *dsi;

	unit = STUNIT(dev);
	dsi = &stape[unit];

	dsi->un_ctype = 1;
	dsi->un_openf = OPENING;
	stcmd(dev, SC_TEST_UNIT_READY, 0);
	if (stcmd(dev, SC_REQUEST_SENSE, 0) == 0 || dsi->un_openf != OPEN) {
		dsi->un_openf = OPENING;
		if (stcmd(dev, SC_REQUEST_SENSE, 0) == 0 ||
		    dsi->un_openf != OPEN) {
			if (stcmd(dev, SC_REQUEST_SENSE, 0) == 0 ||
			    dsi->un_openf != OPEN) {
				if (dsi->un_openf == OPEN_FAILED) {
					uprintf("st%d: no cartridge loaded\n",
					    unit);
				} else {
					uprintf("st%d: tape not online \n", 
					    unit);
				}
				dsi->un_openf = CLOSED;
				dsi->un_ctype = ST_TYPE_INVALID;
				return (0);
			}
		}
	}
	dsi->un_openf = CLOSED;
	return (1);
}

sctopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct scsi_unit *un;
	register int unit, s;
	register struct scsi_tape *dsi;
	register int i;

	unit = STUNIT(dev);
	if (unit > nstape) {
		u.u_error = ENXIO;
		return;
	}
	un = &stunits[unit];
	dsi = &stape[unit];
	if (un->un_mc == 0) {	/* never attached */
		u.u_error = ENXIO;
		return;
	}

	/* determine type of tape controller */
	if (dsi->un_ctype == ST_TYPE_INVALID) {
		if (stinit(dev) == 0) {
			u.u_error = EIO;
			return;
		}
	}

	s = spl6();
	if (dsi->un_openf != CLOSED) {	/* already open */
		(void) splx(s);
		u.u_error = EBUSY;
		return;
	}
	dsi->un_openf = OPEN;
	(void) splx(s);
	dsi->un_read_only = 0;

	/* must be qic 24 format */
	if (!(minor(dev) & QIC_24)) {
		dsi->un_openf = CLOSED;
		u.u_error = EIO;
		return;
	}
	if ((flag & FWRITE) && dsi->un_read_only) {
		uprintf("st%d: cartridge is write protected \n", unit);
		dsi->un_openf = CLOSED;
		u.u_error = EIO;
		return;
	}
	dsi->un_lastiow = 0;
	dsi->un_lastior = 0;
	dsi->un_next_block = 0;
	dsi->un_last_block = INF;
	/* if we reset these here they will never be preserved for "mt status"
	dsi->un_retry_ct = 0;
	dsi->un_underruns = 0;
	*/
	u.u_error = 0;
	return;
}

/*ARGSUSED*/
sctclose(dev, flag)
	register dev_t dev;
	int flag;
{
	register struct scsi_unit *un;
	register struct scsi_tape *dsi;

	un = &stunits[STUNIT(dev)];
	dsi = &stape[STUNIT(dev)];
	dsi->un_openf = CLOSING;
	if (dsi->un_lastiow) {
		if (stcmd(dev, SC_WRITE_FILE_MARK, 0) == 0) {
			printf("st%d: stclose failed to write file mark\n",
				un - stunits);
		}
	}
	if ((minor(dev) & T_NOREWIND) == 0) {
		(void) stcmd(dev, SC_REWIND, -1);
	} 
	dsi->un_openf = CLOSED;
}

stcmd(dev, cmd, count)
	dev_t dev;
	int cmd, count;
{
	register struct buf *bp;
	register int s, error;
	register struct scsi_unit *un;

	un = &stunits[STUNIT(dev)];
	bp = &un->un_sbuf;
	s = splx(pritospl(un->un_mc->mc_intpri));
	while (bp->b_flags & B_BUSY) {
		/*
		 * special test because B_BUSY never gets cleared in
		 * the non-waiting rewind case.
		 */
		if (bp->b_bcount == -1 && (bp->b_flags & B_DONE)) {
			break;
		}
		bp->b_flags |= B_WANTED;
		sleep((caddr_t) bp, PRIBIO);
	}
	bp->b_flags = B_BUSY | B_READ;
	(void) splx(s);
	bp->b_dev = dev;
	bp->b_bcount = count;
	un->un_scmd = cmd;
	ststrategy(bp);
	/*
	 * In case of rewind on close, don't wait.
	 */
	if (cmd == SC_REWIND && count == -1) {
		return (1);
	}
	s = splx(pritospl(un->un_mc->mc_intpri));
	while ((bp->b_flags & B_DONE) == 0) {
		sleep((caddr_t) bp, PRIBIO);
	}
	(void) splx(s);
	error = geterror(bp);
	if (bp->b_flags & B_WANTED) {
		wakeup((caddr_t) bp);
	}
	bp->b_flags &= B_ERROR;		/* clears B_BUSY */
	return (error == 0);
}

ststrategy(bp)
	register struct buf *bp;
{
	register struct scsi_unit *un;
	register int unit, s;
	register struct buf *ap;
	register struct scsi_tape *dsi;

	unit = STUNIT(bp->b_dev);
	if (unit > nstape) {
		printf("st%d: ststrategy: invalid unit %x\n", unit, unit);
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	un = &stunits[unit];
	dsi = &stape[unit];
	if (dsi->un_openf != OPEN && bp != &un->un_sbuf) {
		bp->b_flags |= B_ERROR;
		iodone(bp);
		return;
	}
	s = splx(pritospl(un->un_mc->mc_intpri));
	while (dsi->un_bufcnt >= MAXSTBUF) {
		sleep((caddr_t) &dsi->un_bufcnt, PRIBIO);
	}
	dsi->un_bufcnt++;

	/*
	 * Put the block at the end of the queue.
	 * Should probably have a pointer to the end of
	 * the queue, but the queue can't get too long,
	 * so the added code complexity probably isn't
	 * worth it.
	 */
	ap = &un->un_utab;
	while (ap->b_actf != NULL) {
		ap = ap->b_actf;
	}
	ap->b_actf = bp;
	bp->b_actf = NULL;
	if (un->un_utab.b_active == 0) {
		(*un->un_c->c_ss->scs_ustart)(un);
		bp = &un->un_mc->mc_tab;
		if (bp->b_actf && bp->b_active == 0) {
			(*un->un_c->c_ss->scs_start)(un);
		}
	}
	(void) splx(s);
}

/*
 * Start the operation.
 */
/*ARGSUSED*/
ststart(bp, un)
	register struct buf *bp;
	register struct scsi_unit *un;
{
	register int bno;
	register struct scsi_tape *dsi;

	dsi = &stape[STUNIT(bp->b_dev)];
	/*
	 * Default is that last command was NOT a read/write command;
	 * if we issue a read/write command we will notice this in stintr().
	 */
	dsi->un_lastiow = 0;
	dsi->un_lastior = 0;
	if (bp == &un->un_sbuf) {
		un->un_cmd = un->un_scmd;
		un->un_count = bp->b_bcount;
	} else if (bp == &un->un_rbuf) {
		if (bp->b_flags & B_READ) {
			if (dsi->un_eof) {
				bp->b_resid = bp->b_bcount;
				iodone(bp);
				if (dsi->un_bufcnt-- >= MAXSTBUF) {
					wakeup((caddr_t) &dsi->un_bufcnt);
				}
				return (0);
			}
			un->un_cmd = SC_READ;
		} else {
			un->un_cmd = SC_WRITE;
		}
		un->un_count = howmany(bp->b_bcount, DEV_BSIZE);
		un->un_flags |= SC_UNF_DVMA;
	} else {
		bno = bp->b_blkno;
		if (bno > dsi->un_last_block && bp->b_flags & B_READ) {
			/* 
			 * Can't read past EOF.
			 */
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
			iodone(bp);
			if (dsi->un_bufcnt-- >= MAXSTBUF) {
				wakeup((caddr_t) &dsi->un_bufcnt);
			}
			return (0);
		}
		if (bno == dsi->un_last_block && bp->b_flags & B_READ) {
			/*
			 * Reading at EOF returns 0 bytes.
			 */
			bp->b_resid = bp->b_bcount;
			iodone(bp);
			if (dsi->un_bufcnt-- >= MAXSTBUF) {
				wakeup((caddr_t) &dsi->un_bufcnt);
			}
			return (0);
		}
		if ((bp->b_flags & B_READ) == 0) {
			/*
			 * Writing sets EOF.
			 */
			dsi->un_last_block = bno + 1;
		}
		if (bno != dsi->un_next_block) {
			/*
			 * Not the next record.
			 * In theory we could space forward, or even rewind
			 * and space forward, and maybe someday we will.
			 * For now, no one really needs this capability.
			 */
			bp->b_flags |= B_ERROR;
			bp->b_error = ENXIO;
			iodone(bp);
			if (dsi->un_bufcnt-- >= MAXSTBUF) {
				wakeup((caddr_t) &dsi->un_bufcnt);
			}
			return (0);
		}
		/*
		 * Position OK, we can do the read or write.
		 */
		if (bp->b_flags & B_READ) {
			un->un_cmd = SC_READ;
		} else {
			un->un_cmd = SC_WRITE;
		}
		un->un_count = howmany(bp->b_bcount, DEV_BSIZE);
		un->un_flags |= SC_UNF_DVMA;
	}
	bp->b_resid = 0;
	return (1);
}

/*
 * Make a command description block.
 */
stmkcdb(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct scsi_cdb *cdb;
	register struct scsi_tape *dsi;
	int density = 0;

	cdb = &c->c_cdb;
	bzero((caddr_t)cdb, sizeof (*cdb));
	cdb->cmd = un->un_cmd;
	cdb->lun = un->un_lun;
	un->un_dma_addr = un->un_dma_count = 0;
	dsi = &stape[un->un_unit];

	switch (un->un_cmd) {

	case SC_WRITE_FILE_MARK:
	case SC_LOAD:
		cdb->count = 1;
		break;

	case SC_TEST_UNIT_READY:
		break;
		
	case SC_REWIND:
		break;

	case SC_ERASE_CARTRIDGE:
		cdb->t_code = 1;
		break;

	case SC_REQUEST_SENSE:
		un->un_dma_addr = (int)c->c_sense - (int)DVMA;
		un->un_dma_count = cdb->count = sizeof (struct st_archive_sense);		
		break;

	case SC_READ:
	case SC_WRITE:
		cdb->t_code = 1;
		cdb->high_count = un->un_count >> 16;
		cdb->mid_count = (un->un_count >> 8) & 0xFF;
		cdb->low_count = un->un_count & 0xFF;
		un->un_dma_addr = un->un_baddr;
		un->un_dma_count = un->un_count * DEV_BSIZE;
		break;

	case SC_SPACE_FILE:
		if (un->un_count == 0)
			cdb->t_code = 3;	/* space to end */
		else
			cdb->t_code = 1;	/* space files, not records */
		/* fall through ... */

	case SC_SPACE_REC:
		cdb->cmd = SC_SPACE;
		cdb->high_count = un->un_count >> 16;
		cdb->mid_count = (un->un_count >> 8) & 0xFF;
		cdb->low_count = un->un_count & 0xFF;
		un->un_dma_addr = un->un_dma_count = 0;
		break;

	default:
		printf("st%d: stmkcdb: invalid command %x\n", un - stunits,
			un->un_cmd);
		break;	
	}
}

stintr(c, resid, error)
	register struct scsi_ctlr *c;
	register int resid, error;
{
	register struct scsi_unit *un;
	register struct buf *bp;
	register struct scsi_tape *dsi;
	struct st_archive_sense *ars;

	un = c->c_un;
	bp = un->un_mc->mc_tab.b_actf->b_actf;
	dsi = &stape[STUNIT(bp->b_dev)];
	ars = (struct st_archive_sense *)c->c_sense;

	/* 
	 * We determine which tape controller we have by the number of
	 * sense bytes we get back.
	 */
	if (dsi->un_openf == OPENING && un->un_cmd == SC_REQUEST_SENSE) {
		if (resid || ST_NO_CART(dsi, c->c_sense)) {
			dsi->un_openf = OPEN_FAILED;
		} else {
			dsi->un_openf = OPEN;
			if (ST_WRITE_PROT(dsi, c->c_sense))
				dsi->un_read_only = 1;
		}
	}

	if (c->c_scb.busy)
		bp->b_flags |= B_ERROR;
	else if (error || c->c_scb.chk || resid > 0) {
		if (c->c_scb.chk && ST_RESET(dsi, c->c_sense)) {
			/*
			 * Power on or reset occurred
			 */
			dsi->un_reset_occurred = 1;
			bp->b_flags |= B_ERROR;
		} else if (c->c_scb.chk && ST_NO_CART(dsi, c->c_sense)) {
			printf("st%d: no cartridge loaded \n", 
			    un - stunits);
			bp->b_flags |= B_ERROR;
		} else if (c->c_scb.chk && ST_FILE_MARK(dsi, c->c_sense)) {
			/*
			 * File mark detected.
			 */
			dsi->un_eof = 1;
			switch (un->un_cmd) {
			case SC_READ:
				dsi->un_next_block += un->un_count -
					resid / DEV_BSIZE;
				dsi->un_last_block = dsi->un_next_block;
				break;
			case SC_SPACE_REC:
				dsi->un_last_block = dsi->un_next_block +=
					un->un_count;	/* a little high */
				break;
			default:
				printf("st%d: scsi tape hit eof on cmd %x\n",
					un - stunits, un->un_cmd);
				break;
			}
			bp->b_resid = resid;
		} else if ((un->un_cmd == SC_WRITE || 
		    un->un_cmd == SC_WRITE_FILE_MARK) &&
		    c->c_scb.chk && ST_WRITE_PROT(dsi, c->c_sense)) {
			/*
			 * Write protected tape.
			 */
			printf("st%d: tape is write protected \n, ",
				un - stunits);
			dsi->un_read_only = 1;
			bp->b_flags |= B_ERROR;
		} else if (c->c_scb.chk && ST_EOT(dsi, c->c_sense)) {
			/*
			 * End of tape.
			 */
			bp->b_resid = bp->b_bcount;
			if (un->un_cmd == SC_WRITE) {
				/*
				 * Setting this flag makes stclose()
				 * write a file mark before closing.
				 * Until a file mark is written, the 
				 * tape will return invalid command
				 * indications and not respond to 
				 * rewinds.
				 */
				dsi->un_lastiow = 1;
			}
		} else if (c->c_scb.chk && ST_ILLEGAL(dsi, c->c_sense)) {
			printf("st%d: illegal command 0x%x\n", un-stunits, un->un_cmd);
			bp->b_flags |= B_ERROR;
		} else if (ST_CORRECTABLE(dsi, c->c_sense))
			goto success;
		else if (ST_EOD(dsi, c->c_sense))
			bp->b_flags |= B_ERROR;
		else {
			/*
			 * Some other error which we can't handle.
			 */
			if ((c->c_scb.chk && dsi->un_openf == OPEN))
				st_pr_sense(c, dsi);
			bp->b_flags |= B_ERROR;
		}
	} else {
success:
		switch (un->un_cmd) {

		case SC_REWIND:
		case SC_ERASE_CARTRIDGE:
		case SC_MODE_SELECT:
		case SC_LOAD:
			dsi->un_next_block = 0;
			dsi->un_eof = 0;
			break;

		case SC_WRITE:
			dsi->un_lastiow = 1;
			dsi->un_next_block += un->un_count;
			break;

		case SC_READ:
			dsi->un_lastior = 1;
			dsi->un_next_block += un->un_count;
			break;

		case SC_SPACE_FILE:
			dsi->un_next_block = 0;
			dsi->un_last_block = INF;
			dsi->un_eof = 0;
			break;

		case SC_SPACE_REC:
			dsi->un_next_block += un->un_count;
			break;

		case SC_REQUEST_SENSE:
			dsi->un_status = ((char *)ars)[2];
/*
			if (ars->ext_sense.add_len >= AR_ES_ADD_LEN) {
				dsi->un_retry_ct += 
				    (ars->retries_msb << 8) +
				    ars->retries_lsb;
			}
*/
			break;

		case SC_TEST_UNIT_READY:
			break;

		case SC_WRITE_FILE_MARK:
			dsi->un_next_block = 0;
			dsi->un_last_block = 0;	/* i.e. no reads allowed */
			break;

		default:
			printf("st%d: stintr: invalid command %x\n",
			    un - stunits, un->un_cmd);
			break;
		}
	}
	if (bp == &un->un_sbuf && 
	    ((un->un_flags & SC_UNF_DVMA) == 0)) {
		(*c->c_ss->scs_done)(un->un_mc);
	} else {
		mbdone(un->un_mc);
		un->un_flags &= ~SC_UNF_DVMA;
	}
	if (dsi->un_bufcnt-- >= MAXSTBUF) {
		wakeup((caddr_t) &dsi->un_bufcnt);
	}
}

st_pr_sense(c, dsi)
	register struct scsi_ctlr *c;
	register struct scsi_tape *dsi;
{
	register struct scsi_unit *un;
	register u_char *cp;
	register int i;

	un = c->c_un;
	cp = (u_char *)c->c_sense;
	printf("st%d: stintr: sense ", un - stunits);
	for (i=0; i < sizeof (struct st_archive_sense); i++)
		printf("%x ", *cp++);
	printf("\n");
}

sctread(dev)
	register dev_t dev;
{
	register struct scsi_unit *un;
	register int unit, r, resid;
	register struct scsi_tape *dsi;

	unit = STUNIT(dev);
	if (unit > nstape) {
		u.u_error = ENXIO;
		return;
	}
	un = &stunits[unit];
	dsi = &stape[unit];
	if (u.u_count % DEV_BSIZE) {
		u.u_error = ENXIO;
		return;	/* drive can't do it */
	}
	resid = u.u_count;
	dsi->un_next_block = u.u_offset / DEV_BSIZE;
	dsi->un_last_block = INF;
	physio(ststrategy, &un->un_rbuf, dev, B_READ, minphys);
	if (dsi->un_eof && u.u_count == resid) {
		dsi->un_eof = 0;	/* the user is really getting it */
	}
}

sctwrite(dev)
	register dev_t dev;
{
	register struct scsi_unit *un;
	register int unit;
	register struct scsi_tape *dsi;

	unit = STUNIT(dev);
	if (unit > nstape) {
		u.u_error = ENXIO;
		return;
	}
	un = &stunits[unit];
	dsi = &stape[unit];
	dsi->un_next_block = u.u_offset / DEV_BSIZE;
	dsi->un_last_block = INF;
	physio(ststrategy, &un->un_rbuf, dev, B_WRITE, minphys);
}

/*ARGSUSED*/
sctioctl(dev, cmd, data, flag)
	register dev_t dev;
	register int cmd;
	register caddr_t data;
	int flag;
{
	register int fcount;
	struct mtop mtop;
	struct mtget mtget;
	register int unit;
	register struct scsi_tape *dsi;
	static int ops[] = {
		SC_WRITE_FILE_MARK,	/* write tape mark */
		SC_SPACE_FILE,		/* forward space file */
		SC_SPACE_FILE,		/* backspace file */
		SC_SPACE_REC,		/* forward space record */
		SC_SPACE_REC,		/* backspace record */
		SC_REWIND,		/* rewind tape */
		SC_REWIND,		/* unload - we just rewind */
		SC_REQUEST_SENSE,	/* get status */
		SC_REWIND,		/* retension - rewind + vu_57 */
		SC_ERASE_CARTRIDGE,	/* erase entire tape */
	};

	unit = STUNIT(dev);
	if (unit > nstape) {
		u.u_error = ENXIO;
		return;
	}
	dsi = &stape[unit];
	switch (cmd) {
	case MTIOCTOP:	/* tape operation */
		if (copyin((caddr_t)data, (caddr_t)&mtop, sizeof(mtop))) {
			u.u_error = EFAULT;
			return;
		}
		switch (mtop.mt_op) {
		case MTWEOF:
		case MTERASE:
			if (dsi->un_read_only) {
				u.u_error = ENXIO;
				return;
			}
		}

		switch (mtop.mt_op) {
		case MTBSF:
		case MTBSR:
			fcount = -mtop.mt_count;
			break;
		case MTRETEN:
			dsi->un_reten_rewind = 1;
			/* FALL THRU */
		case MTFSF:
			fcount = mtop.mt_count;
			if (fcount == -1)
				fcount = 0;
		case MTWEOF:
		case MTREW:
		case MTOFFL:
		case MTNOP:
		case MTERASE:
			fcount = mtop.mt_count;
			break;
		default:
			u.u_error = ENXIO;
			return;
		}
		/*
		 * If eof_flag then we hit eof but didn't tell the user yet.
		 */
		if (ops[mtop.mt_op] == SC_SPACE_FILE && dsi->un_eof) {
			dsi->un_eof = 0;
			if (mtop.mt_op == MTFSF)
				fcount--;
		}
		if (stcmd(dev, ops[mtop.mt_op], fcount) == 0)
			u.u_error = EIO;
		return;

	case MTIOCGET:
		if (stcmd(dev, SC_REQUEST_SENSE, 0) == 0) {
			u.u_error = EIO;
			return;
		}
		mtget.mt_type = MT_ISSC;
		mtget.mt_erreg = dsi->un_status;
		mtget.mt_fileno = dsi->un_retry_ct;
		mtget.mt_blkno = dsi->un_underruns;
		dsi->un_retry_ct = dsi->un_underruns = 0;
		if (copyout((caddr_t)&mtget, data, sizeof(mtget)))
			u.u_error = EFAULT;
		return;

	default:
		u.u_error = ENXIO;
		return;
	}
}
#endif NST > 0
