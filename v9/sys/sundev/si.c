#ifndef lint
static	char sccsid[] = "@(#)si.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "si.h"

#if NSI > 0
/*
 * Generic scsi routines.
 */
 
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
#include "../machine/scb.h"

#include "../sun/dklabel.h"
#include "../sun/dkio.h"

#include "../sundev/mbvar.h"
#include "../sundev/screg.h"
#include "../sundev/sireg.h"
#include "../sundev/scsi.h"

struct	scsi_ctlr sictlrs[NSI];
#define SINUM(si)	(si - sictlrs)

int	siprobe(), sislave(), siattach(), sigo(), sidone(), sipoll();
int	siustart(), sistart(), si_getstatus();
int	si_off(), si_cmd(), si_cmdwait(), si_reset(), si_dmacnt();

struct	mb_ctlr *siinfo[NSI];
extern struct mb_device *sdinfo[];
struct	mb_driver sidriver = {
	siprobe, sislave, siattach, sigo, sidone, sipoll,
	sizeof (struct scsi_si_reg), "sd", sdinfo, "si", siinfo, MDR_BIODMA,
};

/* routines available to devices specific portion of scsi driver */
struct scsi_ctlr_subr sisubr = {
	siustart, sistart, sidone, si_cmd, si_getstatus, si_cmdwait,
	si_off, si_reset, si_dmacnt, sigo,
};

extern int scsi_debug;		/* generic debug information */
extern int scsi_ntype;
extern struct scsi_unit_subr scsi_unit_subr[];

int scsi_disre_enable = 0;	/* enable disconnect/reconnect */
int scsi_dis_debug = 0;		/* disconnect debug info */
int scsi_resel_debug = 0;	/* reselection debug info */
int scsi_reset_debug = 0;	/* scsi bus reset debug information */

/*
 * Determine existence of SCSI host adapter.
 */
siprobe(reg, ctlr)
	register struct scsi_si_reg *reg;
	register int ctlr;
{
	register struct scsi_ctlr *c;

	/* probe for different scsi host adaptor interfaces */
	c = &sictlrs[ctlr];

	/* 
	 * Check for sbc - NCR 5380 Scsi Bus Ctlr chip.
	 * sbc is common to sun3/50 onboard scsi and vme
	 * scsi board.
	 */
	if (peekc(&reg->sbc_rreg.cbsr) == -1) {
		return (0);
	}

	/*
	 * Determine whether the host adaptor interface is onboard or vme.
	 */
	if (cpu == CPU_SUN3_50) {
		/* probe for sun3/50 dma interface */
		if (peek(&reg->udc_rdata) == -1)
			return (0);
		c->c_flags = SCSI_ONBOARD;
	} else {
		/*
		 * Probe for vme scsi card but make sure it is not
		 * the SC host adaptor interface. SI vme scsi host
		 * adaptor occupies 2K bytes in the vme address space. 
		 * SC vme scsi host adaptor occupies 4K bytes in the 
		 * vme address space. So, peek past 2K bytes to 
		 * determine which host adaptor is there.
		 */
		if (peek(&reg->dma_addr) == -1)
			return (0);
		if (peek((int)reg+0x800) != -1)
			return (0);
		c->c_flags = 0;
	}

	/* allocate memory for sense information */
	c->c_sense = (struct scsi_sense *) rmalloc(iopbmap,
	    (long) sizeof (struct scsi_sense));
	if (c->c_sense == NULL) {
		printf("siprobe: no iopb memory for sense.\n");
		return (0);
	}

	/* init controller information */
	c->c_flags |= SCSI_PRESENT;
	if (scsi_disre_enable)
		c->c_flags |= SCSI_EN_DISCON;
	c->c_sir = reg;
	c->c_ss = &sisubr;
	si_reset(c);
	return (sizeof (struct scsi_si_reg));
}


/*
 * See if a slave exists.
 * Since it may exist but be powered off, we always say yes.
 */
/*ARGSUSED*/
sislave(md, reg)
	register struct mb_device *md;
	register struct scsi_si_reg *reg;
{
	register struct scsi_unit *un;
	register int type;

	/*
	 * This kludge allows autoconfig to print out "sd" for
	 * disks and "st" for tapes.  The problem is that there
	 * is only one md_driver for scsi devices.
	 */
	type = TYPE(md->md_flags);
	if (type >= scsi_ntype) {
		panic("sislave: unknown type in md_flags");
	}

	/* link unit to its controller */
	un = (struct scsi_unit *)(*scsi_unit_subr[type].ss_unit_ptr)(md);
	if (un == 0) {
		panic("sislave: md_flags scsi type not configured in\n");
	}
	un->un_c = &sictlrs[md->md_ctlr];
	md->md_driver->mdr_dname = scsi_unit_subr[type].ss_devname;
	return (1);
}

/*
 * Attach device (boot time).
 */
siattach(md)
	register struct mb_device *md;
{
	register int type = TYPE(md->md_flags);
	register struct mb_ctlr *mc = md->md_mc;
	register struct scsi_ctlr *c = &sictlrs[md->md_ctlr];

	if (type >= scsi_ntype) {
		panic("siattach: unknown type in md_flags");
	}
	(*scsi_unit_subr[type].ss_attach)(md);

	if (c->c_flags & SCSI_ONBOARD) {
		return;
	}

	/* 
	 * Initialize interrupt vector and address modifier register.
	 * Address modifier specifies standard supervisor data access
	 * with 24 bit vme addresses. May want to change this in the
	 * future to handle 32 bit vme addresses.
	 */
	if (mc->mc_intr) {
		/* setup for vectored interrupts - we will pass ctlr ptr */
		c->c_sir->iv_am = (mc->mc_intr->v_vec & 0xff) | 
		    VME_SUPV_DATA_24;
		(*mc->mc_intr->v_vptr) = (int)c;
	}
}

/*
 * SCSI unit start routine.
 * Called by SCSI device drivers.
 */
siustart(un)
	register struct scsi_unit *un;
{
	register struct buf *dp;
	register struct mb_ctlr *mc;
	register int s;

	mc = un->un_mc;
	dp = &un->un_utab;
	/* 
	 * Caller guarantees: dp->b_actf != NULL && dp->b_active == 0 
	 * Note: dp->b_active == 1 on a reconnect.
	 */
	/*
	 * Put device on ready queue for bus.
	 */
	if (mc->mc_tab.b_actf == NULL) {
		mc->mc_tab.b_actf = dp;
	} else {
		mc->mc_tab.b_actl->b_forw = dp;
	}
	dp->b_forw = NULL;
	mc->mc_tab.b_actl = dp;
	dp->b_active = 1;
	dp->b_un.b_addr = (caddr_t) un;
}

/*
 * Set up a scsi operation.
 */
sistart(un)
	register struct scsi_unit *un;
{
	register struct mb_ctlr *mc;
	register struct buf *bp, *dp;

	mc = un->un_mc;
	dp = mc->mc_tab.b_actf;	     /* != NULL guaranteed by caller */
	un = (struct scsi_unit *) dp->b_un.b_addr;
	bp = dp->b_actf;
	for (;;) {
		if (bp == NULL) {	  /* no more blocks for this device */
			un->un_utab.b_active = 0;
			dp = mc->mc_tab.b_actf = dp->b_forw;
			if (dp == NULL) {  /* no more devices for this ctlr */
				si_idle(un->un_c);
				return;
			}
			un = (struct scsi_unit *) dp->b_un.b_addr;
			bp = dp->b_actf;
		} else {
			if ((*un->un_ss->ss_start)(bp, un)) {
				mc->mc_tab.b_active = 1;
				un->un_c->c_un = un;
				if (bp == &un->un_sbuf  &&
				    ((un->un_flags & SC_UNF_DVMA) == 0)) {
					sigo(mc);
				} else {
					(void) mbgo(mc);
				}
				return;
			}
			dp->b_actf = bp = bp->av_forw;
		}
	}
}

/*
 * Start up a scsi operation.
 * Called via mbgo after buffer is in memory.
 */
sigo(mc)
	register struct mb_ctlr *mc;
{
	register struct scsi_unit *un;
	register struct scsi_ctlr *c;
	register struct buf *bp, *dp;
	register int unit;

	c = &sictlrs[mc->mc_ctlr];
	dp = mc->mc_tab.b_actf;
	if (dp == NULL || dp->b_actf == NULL) {
		panic("sigo queueing error 1");
	}
	bp = dp->b_actf;
	un = c->c_un;
	if (dp != &un->un_utab) {
		panic("sigo queueing error 2");
	}
	un->un_baddr = MBI_ADDR(mc->mc_mbinfo);
	if ((unit = un->un_md->md_dk) >= 0) {
		dk_busy |= 1<<unit;
		dk_xfer[unit]++;
		dk_wds[unit] += bp->b_bcount >> 6;
	}
	(*un->un_ss->ss_mkcdb)(c, un);
	if (si_cmd(c, un, 1) == 0) {
		(*un->un_ss->ss_intr)(c, 0, SE_FATAL);
		si_off(un);
	}
}

/*
 * Handle a polling SCSI bus interrupt.
 */
sipoll()
{
	register struct scsi_ctlr *c;
	register int serviced = 0;

	for (c = sictlrs; c < &sictlrs[NSI]; c++) {
		if ((c->c_flags & SCSI_PRESENT) == 0)
			continue;
		if ((c->c_sir->csr & 
		    (SI_CSR_SBC_IP | SI_CSR_DMA_IP | SI_CSR_DMA_CONFLICT)) 
		    == 0) {
			continue;
		}
		serviced = 1;
		siintr(c);
	}
	return (serviced);
}

/*
 * Clean up queues, free resources, and start next I/O
 * all done after I/O finishes
 * Called by mbdone after moving read data from Mainbus
 */
sidone(mc)
	register struct mb_ctlr *mc;
{
	register struct buf *bp, *dp;
	register struct scsi_unit *un;
	register struct scsi_ctlr *c;

	bp = mc->mc_tab.b_actf->b_actf;
	c = &sictlrs[mc->mc_ctlr];
	un = c->c_un;

	/* advance controller queue */
	dp = mc->mc_tab.b_actf;
	mc->mc_tab.b_active = 0;
	mc->mc_tab.b_actf = dp->b_forw;

	/* advance unit queue */
	dp->b_active = 0;
	dp->b_actf = bp->av_forw;

	iodone(bp);

	/* start next I/O on unit */
	if (dp->b_actf)
		siustart(un);

	/* start next I/O on controller */
	if (mc->mc_tab.b_actf && mc->mc_tab.b_active == 0) {
		sistart(un);
	} else {
		c->c_un = NULL;
		si_idle(c);
	}
}

/*ARGSUSED*/
si_off(un)
	register struct scsi_unit *un;
{

#ifdef notdef
	/* if done to root real bad things happen... */
	un->un_present = 0;
	printf("scsi unit %d/%d offline\n", un->un_target, un->un_lun);
	if (un->un_md->md_dk > 0) {
		dk_mspw[un->un_md->md_dk]=0;
	}
#endif
}

/*
 * Pass a command to the SCSI bus.
 */
si_cmd(c, un, intr)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register int intr;
{
	register u_char *cp;
	register int i;
	register int errct;
	register struct scsi_si_reg *sir = c->c_sir;

	errct = 0;
	do {
		/* disallow disconnects if waiting for command completion */
		if (intr == 0) {
			c->c_flags &= ~SCSI_EN_DISCON;
		} else {
			if (scsi_disre_enable)
				c->c_flags |= SCSI_EN_DISCON;
			else
				c->c_flags &= ~SCSI_EN_DISCON;
			un->un_wantint = 1;
		}

		/*
		 * For vme host adaptor interface, dma enable bit may
		 * be set to allow reconnect interrupts to come in.
		 * This must be disabled before arbitration/selection
		 * of target is done. Don't worry about re-enabling
		 * dma. If arb/sel fails, then si_idle() will re-enable.
		 * If arb/sel succeeds then handling of command will
		 * re-enable.
		 * Also, disallow sbc to accept reconnect attempts.
		 * Again, si_idle() will re-enable this if arb/sel fails.
		 * If arb/sel succeeds then we do not want to allow
		 * reconnects anyway.
		 */
		if ((c->c_flags & SCSI_ONBOARD) == 0) {
			sir->csr &= ~SI_CSR_DMA_EN;
		}
		sir->sbc_wreg.ser = 0;
		c->c_flags &= ~SCSI_EN_RECON;

		/* performing target selection */
		if (si_arb_sel(c, un) == 0) {
			goto bad;
		}

		/*
		 * Must split dma setup into 2 parts due to sun3/50
		 * which requires bcr to be set before target
		 * changes phase on scsi bus to data phase.
		 *
		 * Three fields in the per scsi unit structure
		 * hold information pertaining to the current dma
		 * operation: un_dma_curdir, un_dma_curaddr, and
		 * un_dma_curcnt. These fields are used to track
		 * the amount of data dma'd especially when disconnects 
		 * and reconnects occur.
		 * If the current command does not involve dma,
		 * these fields are set appropriately.
		 */
		if (un->un_dma_count > 0) {
			if ((c->c_cdb.cmd == SC_READ) || 
			    (c->c_cdb.cmd == SC_REQUEST_SENSE))  {
				un->un_dma_curdir = SI_RECV_DATA;
				sir->csr &= ~SI_CSR_SEND;
			} else {
				un->un_dma_curdir = SI_SEND_DATA;
				sir->csr |= SI_CSR_SEND;
			}
			/* save current dma info for disconnect */
			un->un_dma_curaddr = un->un_dma_addr;
			un->un_dma_curcnt = un->un_dma_count;

			/* tape has scsi id of 4 */
			if ((un->un_target == 4) && scsi_dis_debug) {
				printf("si_cmd: cmd= %x, addr= %x, cnt= %x\n", 
				    c->c_cdb.cmd, un->un_dma_curaddr, 
				    un->un_dma_curcnt);
			}

			/* reset fifo */
			sir->csr &= ~SI_CSR_FIFO_RES;
			sir->csr |= SI_CSR_FIFO_RES;

			/* must init bcr before target goes into data phase */
			sir->bcr = un->un_dma_curcnt;

			/* 
			 * Currently we don't use all 24 bits of the
			 * count register on the vme interface. To do
			 * this changes are required other places, e.g.
			 * in the scsi_unit structure the fields
			 * un_dma_curcnt and un_dma_count would need to
			 * be changed.
			 */
			if ((c->c_flags & SCSI_ONBOARD) == 0) {
				sir->bcrh = 0;
			}
		} else {
			un->un_dma_curdir = SI_NO_DATA;
			un->un_dma_curaddr = 0;
			un->un_dma_curcnt = 0;
		}

		cp = (u_char *) &c->c_cdb;
		if (scsi_debug) {
			printf("si%d: si_cmd: target %d issuing command ",
			     SINUM(c), un->un_target);
			for (i = 0; i < sizeof (struct scsi_cdb); i++) {
				printf("%x ", *cp++);
			}
			printf("\n");
			cp = (u_char *) &c->c_cdb;
		}

		/* put scsi command out on scsi bus */
		if (si_putdata(c, PHASE_COMMAND, cp, sizeof(struct scsi_cdb)) 
		    == 0) {
			errct++;
		} else {
			/* do final dma setup and start dma operation */
			if (un->un_dma_count > 0) {
				if (c->c_flags & SCSI_ONBOARD)
					si_ob_dma_setup(c, un);
				else
					si_vme_dma_setup(c, un);
			} else if (un->un_wantint) {
				/*
				 * If this command does not involve
				 * any dma, we must set things up
				 * so we get an interrupt when the
				 * target is ready to give us status.
				 * The interrupt we get is a phase
				 * mismatch, however, to get this
				 * interrupt the sbc must be in dma
				 * mode. Also, the vme scsi card does
				 * not generate any interrupts unless
				 * dma is enabled in the csr.
				 */
				sir->sbc_wreg.mr |= SBC_MR_DMA;
				sir->sbc_wreg.tcr = TCR_UNSPECIFIED;
				if ((c->c_flags & SCSI_ONBOARD) == 0) {
					sir->csr &= ~SI_CSR_SEND;
					sir->csr |= SI_CSR_DMA_EN;
				}
			}
			return (1);
		}
	} while (errct < SI_NUM_RETRIES);

	/* clear fields if we were not able to issue scsi cmd */
	printf("si_cmd: too many errors\n");
	if (un->un_dma_count != 0) {
		un->un_dma_curdir = 0;
		un->un_dma_curaddr = 0;
		un->un_dma_curcnt = 0;
		sir->bcr = 0;
		sir->csr &= ~SI_CSR_SEND;
	}
bad:
	un->un_wantint = 0;
	si_idle(c);
	return (0);
}

/*
 * Perform the SCSI arbitration and selection phases.
 */
si_arb_sel(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register u_char *icrp = &sir->sbc_wreg.icr;
	register u_char *mrp = &sir->sbc_wreg.mr;
	register int j;
	register u_char icr;
	u_char id;

	/* wait for scsi bus to become free */
	if (si_sbc_wait((caddr_t)&sir->sbc_rreg.cbsr, SBC_CBSR_BSY, 0) == 0) {
		printf("si_arb_sel: continuously busy\n");
		si_reset(c);
		return (0);
	}

	/* arbitrate for the scsi bus */
	sir->sbc_wreg.odr = SI_HOST_ID;
	for (j = 0; j < SI_NUM_RETRIES; j++) {
		*mrp |= SBC_MR_ARB;

		/* wait for sbc to begin arbitration */
		if (si_sbc_wait((caddr_t)icrp, SBC_ICR_AIP, 1) == 0) {
			/* problem with sbc if arb never begins */
			/* 
			 * Current bug in Emulex MT02 firmware.
			 * It does not go through the reselection timeout 
			 * procedure documented in the scsi protocol spec.
			 */
			if ((sir->sbc_rreg.cbsr & SBC_CBSR_SEL) && 
			    (sir->sbc_rreg.cbsr & SBC_CBSR_IO) &&
			    (sir->sbc_rreg.cdr & SI_HOST_ID)) {
				printf("si_arb_sel: REselect, flgs %x\n", 
				    c->c_flags);
				if (scsi_resel_debug) halt("");
			} else {
				printf("si_arb_sel: AIP never set, ??\n");
			}

			*mrp &= ~SBC_MR_ARB;
			return (0);
			/* si_reset(c); */
		}

		/* check to see if we won arbitration */
		DELAY(SI_ARBITRATION_DELAY);
		if ((*icrp & SBC_ICR_LA) == 0) {
		    	if ( ((sir->sbc_rreg.cdr & ~SI_HOST_ID)
			    < SI_HOST_ID) &&
			    ((*icrp & SBC_ICR_LA) == 0)) {
			    	/* won arbitration */
				icr = *icrp & ~SBC_ICR_AIP;
				*icrp = icr | SBC_ICR_ATN;
				icr = *icrp & ~SBC_ICR_AIP;
			    	*icrp = icr | SBC_ICR_SEL;
			    	DELAY(SI_BUS_CLEAR_DELAY + SI_BUS_SETTLE_DELAY);
			    	break;
		    	}
		}

		/* lost arbitration, clear arbitration mode */
		*mrp &= ~SBC_MR_ARB;
	}

	/* couldn't win arbitration */
	if (j == SI_NUM_RETRIES) {
		/* should never happen since we have highest pri scsi id */
		*mrp &= ~SBC_MR_ARB;
		*icrp = 0;
		printf("si_arb_sel: couldn't win arbitration\n");
		return (0);
	}

	/* won arbitration, perform selection */
	sir->sbc_wreg.odr = (1 << un->un_target) | SI_HOST_ID;
	icr = *icrp & ~SBC_ICR_AIP;
	*icrp = icr | SBC_ICR_DATA | SBC_ICR_BUSY;
	*mrp &= ~SBC_MR_ARB;

	/* wait for target to acknowledge selection */
	*icrp &= ~SBC_ICR_BUSY;
	if (si_sbc_wait((caddr_t)&sir->sbc_rreg.cbsr, SBC_CBSR_BSY, 1) == 0) {
		if (scsi_debug)
			printf("si_arb_sel: cbsr bsy never set\n");
		si_reset(c);
		return (0);
	}
	*icrp &= ~(SBC_ICR_SEL | SBC_ICR_DATA);

	/* send target identify message */
	if (c->c_flags & SCSI_EN_DISCON)
		id = SC_DR_IDENTIFY | c->c_cdb.lun;
	else
		id = SC_IDENTIFY | c->c_cdb.lun;
	if (si_putdata(c, PHASE_MSG_OUT, &id, 1) == 0) {
		if (scsi_debug)
			printf("si_arb_sel: put of ID MSG %x failed\n", id);
	}
	*icrp = 0;
	return (1);
}

/*
 * Set up the SCSI control logic for a dma transfer for vme host adaptor.
 */
si_vme_dma_setup(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct scsi_si_reg *sir = c->c_sir;

	/* setup starting dma address and number bytes to dma */
	sir->dma_addr = un->un_dma_curaddr;
	sir->dma_count = un->un_dma_curcnt;

	/* set up byte packing control info */
	if (sir->dma_addr & 0x2) {
		/* setup word dma transfers across vme bus */
		sir->csr |= SI_CSR_BPCON;
	} else {
		/* setup longword dma transfers across vme bus */
		sir->csr &= ~SI_CSR_BPCON;
	}

	if (scsi_debug) {
		printf("si_vme_dma_setup: addr %x, cnt %x, csr %x\n", 
		    sir->dma_addr, sir->dma_count, sir->csr);
	}

	/* init sbc for dma transfer */
	si_sbc_dma_setup(sir, un->un_dma_curdir);

	/* enable dma - this must be the last step */
	sir->csr |= SI_CSR_DMA_EN;
}

/*
 * Set up the SCSI control logic for a dma transfer for onboard host
 * adaptor.
 */
si_ob_dma_setup(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register struct udc_table *udct = &c->c_udct;
	register int addr;

	/* set up udc dma information */
	addr = un->un_dma_curaddr;
	if (addr < DVMA_OFFSET)
		addr += DVMA_OFFSET;
	udct->haddr = ((addr & 0xff0000) >> 8) | UDC_ADDR_INFO;
	udct->laddr = addr & 0xffff;
	udct->hcmr = UDC_CMR_HIGH;
	udct->count = un->un_dma_curcnt / 2; /* #bytes -> #words */
	if (un->un_dma_curdir == SI_RECV_DATA) {
		udct->rsel = UDC_RSEL_RECV;
		udct->lcmr = UDC_CMR_LRECV;
	} else {
		udct->rsel = UDC_RSEL_SEND;
		udct->lcmr = UDC_CMR_LSEND;
		if (un->un_dma_curcnt & 1) {
			udct->count++;
		}
	}

	/* initialize udc chain address register */
	sir->udc_raddr = UDC_ADR_CAR_HIGH;
	DELAY(SI_UDC_WAIT);
	sir->udc_rdata = ((int)udct & 0xff0000) >> 8;
	DELAY(SI_UDC_WAIT);
	sir->udc_raddr = UDC_ADR_CAR_LOW;
	DELAY(SI_UDC_WAIT);
	sir->udc_rdata = (int)udct & 0xffff;

	if (scsi_debug) {
		printf("si_ob_dma_setup: udct= %x, %x -> %x\n", udct, 
		    un->un_dma_curaddr, addr);
	}

	/* initialize udc master mode register */
	DELAY(SI_UDC_WAIT);
	sir->udc_raddr = UDC_ADR_MODE;
	DELAY(SI_UDC_WAIT);
	sir->udc_rdata = UDC_MODE;

	/* issue channel interrupt enable command, in case of error, to udc */
	DELAY(SI_UDC_WAIT);
	sir->udc_raddr = UDC_ADR_COMMAND;
	DELAY(SI_UDC_WAIT);
	sir->udc_rdata = UDC_CMD_CIE;

	/* issue start chain command to udc */
	DELAY(SI_UDC_WAIT);
	sir->udc_rdata = UDC_CMD_STRT_CHN;

	/* put sbc in dma mode and start dma transfer */
	si_sbc_dma_setup(sir, un->un_dma_curdir);
}

/*
 * Setup and start the sbc for a dma operation.
 */
si_sbc_dma_setup(sir, dir)
	register struct scsi_si_reg *sir;
	register int dir;
{
	sir->sbc_wreg.mr |= SBC_MR_DMA;
	if (dir == SI_RECV_DATA) {
		sir->sbc_wreg.tcr = TCR_DATA_IN;
		sir->sbc_wreg.ircv = 0;
	} else {
		sir->sbc_wreg.tcr = TCR_DATA_OUT;
		sir->sbc_wreg.icr = SBC_ICR_DATA;
		sir->sbc_wreg.send = 0;
	}
}

/*
 * Cleanup up the SCSI control logic after a dma transfer.
 */
si_dma_cleanup(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = c->c_sir;

	/* disable dma controller */
	if (c->c_flags & SCSI_ONBOARD) {
		sir->udc_raddr = UDC_ADR_COMMAND;
		DELAY(SI_UDC_WAIT);
		sir->udc_rdata = UDC_CMD_RESET;
	} else {
		sir->dma_addr = 0;
		sir->dma_count = 0;
	}

	/* reset fifo */
	sir->csr &= ~SI_CSR_FIFO_RES;
	sir->csr |= SI_CSR_FIFO_RES;

	/* take sbc out of dma mode and reset bcr */
	sir->sbc_wreg.icr = 0;
	sir->sbc_wreg.mr &= ~SBC_MR_DMA;
	sir->sbc_wreg.tcr = 0;
	sir->bcr = 0;
}

/*
 * Handle special dma receive situations, e.g. an odd number of bytes 
 * in a dma transfer.
 * The Sun3/50 onboard interface has different situations which
 * must be handled than the vme interface.
 */
si_dma_recv(c) 
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register struct scsi_unit *un = c->c_un;
	register int offset;

	offset = un->un_dma_curaddr + (un->un_dma_curcnt - sir->bcr);

	/* handle the onboard scsi situations */
	if (c->c_flags & SCSI_ONBOARD) {
		sir->udc_raddr = UDC_ADR_COUNT;

		/* wait for the fifo to empty */
		if (si_wait((u_short *)&sir->csr, SI_CSR_FIFO_EMPTY, 1) == 0) {
			printf("si_dma_recv: fifo never emptied\n");
			return (0);
		}

		/* handle odd byte */
		if ((un->un_dma_curcnt - sir->bcr) & 1) {
			DVMA[offset - 1] = (sir->fifo_data & 0xff00) >> 8;

		/*
		 * The udc may not dma the last word from the fifo_data
		 * register into memory due to how the hardware turns
		 * off the udc at the end of the dma operation.
		 */
		} else if (((sir->udc_rdata*2) - sir->bcr) == 2) {
			DVMA[offset - 2] = (sir->fifo_data & 0xff00) >> 8;
			DVMA[offset - 1] = sir->fifo_data & 0x00ff;
		}

	/* handle the vme scsi situations */
	} else if ((sir->csr & SI_CSR_LOB) != 0) {
	    /*
	     * Grabs last few bytes which may not have been dma'd.
	     * Worst case is when longword dma transfers are being done
	     * and there are 3 bytes leftover.
	     * If BPCON bit is set then longword dmas were being done,
	     * otherwise word dmas were being done.
	     */
	    if ((sir->csr & SI_CSR_BPCON) == 0) {
		switch (sir->csr & SI_CSR_LOB) {
		case SI_CSR_LOB_THREE:
			DVMA[offset - 3] = (sir->bpr & 0xff000000) >> 24;
			DVMA[offset - 2] = (sir->bpr & 0x00ff0000) >> 16;
			DVMA[offset - 1] = (sir->bpr & 0x0000ff00) >> 8;
			break;

		case SI_CSR_LOB_TWO:
			DVMA[offset - 2] = (sir->bpr & 0xff000000) >> 24;
			DVMA[offset - 1] = (sir->bpr & 0x00ff0000) >> 16;
			break;

		case SI_CSR_LOB_ONE:
			DVMA[offset - 1] = (sir->bpr & 0xff000000) >> 24;
			break;
		}
	    } else {
		DVMA[offset - 1] = (sir->bpr & 0x0000ff00) >> 8;
	    }
	}
	return (1);
}


/*
 * Handle a scsi interrupt.
 */
siintr(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register struct scsi_unit *un = c->c_un;
	register int bsr;
	register int status;
	register int resid;
	register int dma_cleanup;
	register int i;
	int disconnect;
	u_char msg;
	struct buf *dp;
	int reset_occurred;

	/* set misc flags */
	status = SE_NO_ERROR;
	resid = 0;
	dma_cleanup = 0;
	disconnect = 0;
	reset_occurred = 0;

	/* 
	 * For vme host adaptor interface, must disable dma before
	 * accessing any registers other than the csr or the 
	 * SI_CSR_DMA_CONFLICT bit in the csr will be set.
	 */
	if ((c->c_flags & SCSI_ONBOARD) == 0) {
		sir->csr &= ~SI_CSR_DMA_EN;
	}

	/* check for dma related errors */
	if (sir->csr & (SI_CSR_DMA_IP | SI_CSR_DMA_CONFLICT)) {
		if (sir->csr & SI_CSR_DMA_BUS_ERR) {
			printf("siintr: bus error during dma\n");
		} else if (sir->csr & SI_CSR_DMA_CONFLICT) {
			printf("siintr: invalid reg access during dma\n");
		} else {
			if (c->c_flags & SCSI_ONBOARD)
				printf("siintr: dma ip, unknown reason\n");
			else
				printf("siintr: dma overrun\n");
		}
		/*
		 * Either we were waiting for an interrupt on a phase change 
		 * on the scsi bus, an interrupt on a reconnect attempt,
		 * or an interrupt upon completion of a real dma operation.
		 * Each of these situations must be handled appropriately.
		 */
		if (sir->sbc_rreg.tcr == TCR_UNSPECIFIED) {
			sir->sbc_wreg.mr &= ~SBC_MR_DMA;
		} else if (un == NULL) {
			si_reset(c);
			si_idle(c);
			return;
		} else {
			dma_cleanup = 1;
		}
		status = SE_FATAL;

	/* check for interrupt from sbc */
	} else  if (sir->csr & SI_CSR_SBC_IP) {

		/* grab the bsr and find out why sbc interrupted */
		bsr = sir->sbc_rreg.bsr;

		/* acknowledge sbc interrupt */
		msg = sir->sbc_rreg.clr;

		/*
		 * Check for sbc end of operation interrupt.
		 * This is the normal interrupt upon completion
		 * of a dma operation. Very soon after this 
		 * interrupt is generated a phase mismatch
		 * should occur on the scsi bus as the target
		 * changes from the data in/out phase to the
		 * status phase.
		 */
		if (bsr & SBC_BSR_EDMA) {
			if (si_sbc_wait((caddr_t)&sir->sbc_rreg.bsr, 
			    SBC_BSR_PMTCH, 0) == 0) {
				dma_cleanup = 1;
				status = SE_FATAL;
				printf("si: scsi sbc dma end op\n");
				goto done;
			}
			goto phmismatch;

		/* check for phase mismatch */
		} else if ((bsr & SBC_BSR_PMTCH) == 0) {
phmismatch:
			if ((sir->sbc_rreg.mr & SBC_MR_DMA) == 0) {
				if (c->c_flags & SCSI_EN_RECON)
					goto recon;
				else
					goto discon;
			}

			/*
			 * Handle fake dma mode which is used when we
			 * want an interrupt when target changes scsi
			 * bus to status phase.
			 */
			sir->sbc_wreg.mr &= ~SBC_MR_DMA;
			if (sir->sbc_rreg.tcr == TCR_UNSPECIFIED) {
				sir->sbc_wreg.tcr = 0;
			} else {
				/* need to reset some registers after a dma */
				dma_cleanup = 1;

				/* handle special dma recv situations */
				if (un->un_dma_curdir == SI_RECV_DATA) {
					if (si_dma_recv(c) == 0) {
						status = SE_RETRYABLE;
					}
				}
			}

discon:
			/* check for disconnect */
			if ((sir->sbc_rreg.cbsr & SBC_CBSR_REQ) &&
			    ((sir->sbc_rreg.cbsr & CBSR_PHASE_BITS) == 
			    PHASE_MSG_IN)) {
				if (scsi_debug || scsi_dis_debug)
			 		printf("siintr: DISconnect\n");
				if (si_disconnect(c)) {
					disconnect = 1;
				}
			}
			goto done;
		}

recon:
		/* check for reconnect attempt */
		if ((sir->sbc_rreg.cbsr & SBC_CBSR_SEL) && 
		    (sir->sbc_rreg.cbsr & SBC_CBSR_IO) &&
		    (sir->sbc_rreg.cdr & SI_HOST_ID)) {
			if (scsi_debug || scsi_dis_debug)
				printf("siintr: REconnect\n");
			si_reconnect(c);
			return;
		}

		/*
		 * Scsi bus reset occurred. Put registers in correct
		 * state. Must cleanup disconnected tasks which will
		 * never reconnect due to the scsi bus reset.
		 */
		if (scsi_debug)
			printf("siintr: got scsi bus reset\n");
		sir->csr = 0;
		DELAY(10);
		sir->csr = SI_CSR_INTR_EN|SI_CSR_SCSI_RES|SI_CSR_FIFO_RES;
		sir->sbc_wreg.mr = SBC_MR_EEI;
		c->c_flags &= ~SCSI_EN_RECON;
		if (c->c_disqh != NULL) {
			c->c_flags |= SCSI_FLUSH_DISQ;
			c->c_flush = c->c_disqt;
		}
		status = SE_FATAL;
		reset_occurred = 1;

		/* flush disconnect tasks now if possible */
		if ((un == NULL) && c->c_disqh) {
			si_idle(c);
			return;
		}
	}

done:
	/* cleanup after a dma operation */
	if (dma_cleanup) {
		resid = sir->bcr;
		si_dma_cleanup(c);
	}

	/* pass interrupt info to unit */
	if (un && un->un_wantint && (disconnect == 0)) {
		un->un_wantint = 0;
		if (status == SE_NO_ERROR) {
			if (si_getstatus(un, 0) == 0) {
				status = SE_RETRYABLE;
			}
		} else if (reset_occurred == 0) {
			si_reset(c);
		}
		(*un->un_ss->ss_intr)(c, resid, status);
	}

	if (disconnect) {
		/* start next I/O activity on controller */
		if ((un->un_mc->mc_tab.b_actf) && 
		    (un->un_mc->mc_tab.b_active == 0)) {
			sistart(un);
		} else {
			/* enable reconnect attempts */
			sir->sbc_wreg.ser = SI_HOST_ID;
			c->c_flags |= SCSI_EN_RECON;
			if ((c->c_flags & SCSI_ONBOARD) == 0) {
				sir->csr &= ~SI_CSR_SEND;
				sir->csr |= SI_CSR_DMA_EN;
			}
		}
	}
}

/*
 * Handle target disconnecting.
 */
si_disconnect(c) 
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un = c->c_un;
	register struct mb_ctlr *mc = un->un_mc;
	register struct buf *dp;
	register struct scsi_si_reg *sir = c->c_sir;
	register u_char msg;

	/* get disconnect message(s) */
	msg = si_getdata(c, PHASE_MSG_IN);
	if (msg == SC_SAVE_DATA_PTR) {
		msg = si_getdata(c, PHASE_MSG_IN);
	}
	if (msg != SC_DISCONNECT) {
		return (0);
	}

	/* save dma info for reconnect */
	if (un->un_dma_curdir != SI_NO_DATA) {

		/* debug information */
		if (c->c_flags & SCSI_ONBOARD) {
			if (scsi_dis_debug) {
				sir->udc_raddr = UDC_ADR_COUNT;
				printf("\tdiscon: udc_cnt %x\n", 
				    sir->udc_rdata);
			}
			if ((un->un_dma_curaddr & 0xffff) != c->c_udct.laddr) {
				printf("discon: caddr= %x, udc= %x\n",
				    un->un_dma_curaddr, c->c_udct.laddr);
			}
		}
		if (scsi_dis_debug) {
		    printf("\taddr= %x, cnt= %x, bcr= %x, sr= %x, baddr= %x\n",
			un->un_dma_curaddr, un->un_dma_curcnt, 
			sir->bcr, sir->csr, un->un_baddr);
		}

		/*
		 * Save dma information so dma can be restarted when
		 * a reconnect occurs.
		 */
		un->un_dma_curaddr += un->un_dma_curcnt - sir->bcr;
		un->un_dma_curcnt = sir->bcr;
	}

	/* 
	 * Remove this disconnected task from the ctlr ready queue and save 
	 * on disconnect queue until a reconnect is done.
	 * Advance controller queue. Remove mainbus resource alloc info.
	 */
	dp = mc->mc_tab.b_actf;
	mc->mc_tab.b_active = 0;
	mc->mc_tab.b_actf = dp->b_forw;
	mc->mc_mbinfo = 0;
	if (c->c_disqh == NULL) 
		c->c_disqh = dp;
	else
		c->c_disqt->b_forw = dp;
	dp->b_forw = NULL;
	c->c_disqt = dp;
	c->c_un = NULL;
	return (1);
}

/*
 * Complete reselection phase and reconnect to target.
 *
 * NOTE: this routine cannot use si_getdata() to get identify msg
 * from reconnecting target due to sun3/50 scsi interface. The bcr
 * must be setup before the target changes scsi bus to data phase
 * if the command being reconnected involves dma (which we do not
 * know until we get the identify msg). Thus we cannot acknowledge
 * the identify msg until some setup of the host adaptor registers 
 * is done.
 */
si_reconnect(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register struct buf *dp;
	register struct buf *pdp;
	register struct scsi_unit *un;
	register int i;
	register u_char msg;
	register u_char lun;
	register u_char cdr;

	/* get reselecting target scsi id */
	cdr = sir->sbc_rreg.cdr & ~SI_HOST_ID;

	/* make sure there are only 2 scsi id's set */
	for (i=0; i < 8; i++) {
		if (cdr & (1<<i))
			break;
	}
	cdr &= ~(1<<i);
	if (cdr != 0) {
		printf("si_recon: > 2 scsi ids\n");
		return;
	}

	/* acknowledge reselection */
	sir->sbc_wreg.icr = SBC_ICR_BUSY;
	if (si_sbc_wait((caddr_t)&sir->sbc_rreg.cbsr, SBC_CBSR_SEL, 0) == 0) {
		printf("si_recon: target never rel SEL\n");
		si_reset(c);
		return;
	}
	sir->sbc_wreg.icr = 0;

	/* setup for getting identify message from reconnecting target */
	sir->sbc_wreg.tcr = TCR_MSG_IN;
	if (si_sbc_wait((caddr_t)&sir->sbc_rreg.cbsr, SBC_CBSR_REQ, 1) == 0) {
		printf("si_recon: REQ not active\n");
		si_reset(c);
		return;
	}
	if ((sir->sbc_rreg.bsr & SBC_BSR_PMTCH) == 0) {
		printf("si_recon: phase mismatch\n");
		si_reset(c);
		return;
	}

	/* grab identify message */
	msg = sir->sbc_rreg.cdr;
	sir->sbc_wreg.tcr = 0;
	if ((msg != SC_IDENTIFY) && (msg != SC_DR_IDENTIFY)) {
		printf("si: recon, not id msg\n");
		si_reset(c);
		return;
	}
	lun = msg & 0x07;

	/* search disconnect q for reconnecting task */
	for (dp = c->c_disqh, pdp = NULL; dp; pdp = dp, dp = dp->b_forw) {
		un = (struct scsi_unit *)dp->b_un.b_addr;
		if ((un->un_target == i) && (un->un_lun == lun))
			break;
	}
	if (dp == NULL) {
		printf("si: recon, never found dis unit\n");
		si_reset(c);
		return;
	}

	/* make sure there is no active I/O */
	if (un->un_mc->mc_tab.b_actf != NULL) {
		printf("si: recon, other I/O active\n");
		si_reset(c);
		return;
	}

	/* disable other reconnection attempts */
	sir->sbc_wreg.ser = 0;
	c->c_flags &= ~SCSI_EN_RECON;

	/* remove entity from disconnect q */
	if (dp == c->c_disqh)
		c->c_disqh = dp->b_forw;
	else
		pdp->b_forw = dp->b_forw;
	if (dp == c->c_disqt)
		c->c_disqt = pdp;
	dp->b_forw = NULL;

	/* requeue on controller queue */
	siustart(un);
	un->un_mc->mc_tab.b_active = 1;
	c->c_un = un;

	/* restart disconnect activity */
	if (un->un_dma_curdir != SI_NO_DATA) {
		/* restore mainbus resource allocation info */
		un->un_mc->mc_mbinfo = un->un_baddr;

		/* do initial dma setup */
		if (un->un_dma_curdir == SI_RECV_DATA)
			sir->csr &= ~SI_CSR_SEND;
		else
			sir->csr |= SI_CSR_SEND;
		sir->csr &= ~SI_CSR_FIFO_RES;
		sir->csr |= SI_CSR_FIFO_RES;
		sir->bcr = un->un_dma_curcnt;
		if ((c->c_flags & SCSI_ONBOARD) == 0) {
			sir->bcrh = 0;
		}

		if (scsi_dis_debug) {
		    printf("\taddr= %x, cnt= %x, bcr= %x, sr= %x, baddr= %x\n", 
			un->un_dma_curaddr, un->un_dma_curcnt, sir->bcr, 
			sir->csr, un->un_baddr);
		}

	}

	/* we can finally acknowledge identify message */
	sir->sbc_wreg.icr = SBC_ICR_ACK;
	if (si_sbc_wait((caddr_t)&sir->sbc_rreg.cbsr, SBC_CBSR_REQ, 0) == 0) {
		printf("si_recon: REQ not INactive\n");
		si_reset(c);
		return;
	}
	sir->sbc_wreg.icr = 0;

	/* may get restore pointers message */
	if ((sir->sbc_rreg.cbsr & SBC_CBSR_REQ) &&
	    ((sir->sbc_rreg.cbsr & CBSR_PHASE_BITS) == PHASE_MSG_IN)) {
		msg = si_getdata(c, PHASE_MSG_IN);
	}

	/* do final setup for dma operation and start dma */
	if (un->un_dma_curdir != SI_NO_DATA) {
		if (c->c_flags & SCSI_ONBOARD)
			si_ob_dma_setup(c, un);
		else
			si_vme_dma_setup(c, un);
	} else if (un->un_wantint) {
		sir->sbc_wreg.mr |= SBC_MR_DMA;
		sir->sbc_wreg.tcr = TCR_UNSPECIFIED;
		if ((c->c_flags & SCSI_ONBOARD) == 0) {
			sir->csr &= ~SI_CSR_SEND;
			sir->csr |= SI_CSR_DMA_EN;
		}
	}
}

/*
 * No current activity for the scsi bus. May need to flush some
 * disconnected tasks if a scsi bus reset occurred before the
 * target reconnected, since a scsi bus reset causes targets to 
 * "forget" about any disconnected activity.
 * Also, enable reconnect attempts.
 */
si_idle(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register struct buf *dp;
	register struct scsi_unit *un;
	register int i;
	register int resid;

	if (c->c_flags & SCSI_FLUSHING) {
		if (scsi_reset_debug)
			printf("si_idle: flushing, flags %x\n", c->c_flags);
		return;
	}

	/* flush disconnect tasks if a reconnect will never occur */
	if (c->c_flags & SCSI_FLUSH_DISQ) {
		if (scsi_reset_debug) {
		    printf("si_idle: flush: flags= %x, disqh= %x, disqt= %x\n", 
			c->c_flags, c->c_disqh, c->c_disqt);
		}

		/* now in process of flushing tasks */
		c->c_flags &= ~SCSI_FLUSH_DISQ;
		c->c_flags |= SCSI_FLUSHING;

		for (dp = c->c_disqh; dp && c->c_flush; dp = c->c_disqh) {
			/* keep track of last task to flush */
			if (c->c_flush == c->c_disqh) 
				c->c_flush = NULL;

			/* remove tasks from disconnect q */
			un = (struct scsi_unit *)dp->b_un.b_addr;
			c->c_disqh = dp->b_forw;
			dp->b_forw = NULL;

			/* requeue on controller q */
			siustart(un);
			un->un_mc->mc_tab.b_active = 1;
			c->c_un = un;

			/* inform device routines of error */
			if (un->un_dma_curdir != SI_NO_DATA) {
				un->un_mc->mc_mbinfo = un->un_baddr;
				resid = un->un_dma_curcnt;
			} else {
				resid = 0;
			}
			(*un->un_ss->ss_intr)(c, resid, SE_FATAL);
		}
		if (c->c_disqh == NULL) {
			c->c_disqt = NULL;
		}
		c->c_flags &= ~SCSI_FLUSHING;
	}

	/* enable reconnect attempts */
	sir->sbc_wreg.ser = SI_HOST_ID;
	c->c_flags |= SCSI_EN_RECON;
	if ((c->c_flags & SCSI_ONBOARD) == 0) {
		sir->csr &= ~SI_CSR_SEND;
		sir->csr |= SI_CSR_DMA_EN;
	}
}

/*
 * Get status bytes from scsi bus.
 */
si_getstatus(un, recurse)
	register struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;
	register u_char *cp = (u_char *)&c->c_scb;
	struct scsi_cdb save_cdb;
	struct scsi_scb save_scb;
	register int i;
	register int b;
	register int save_dma_addr;
	register int save_dma_count;
	register int save_reg_dma_count;
	int resid;
	short retval = 1;
	int s;

	/* get all the status bytes */
	for (i = 0;;) {
		b = si_getdata(c, PHASE_STATUS);
		if (b < 0) {
			break;
		}
		if (i < STATUS_LEN) {
			cp[i++] = b;
		}
	}

	/* get command complete message */
	b = si_getdata(c, PHASE_MSG_IN);
	if (b != SC_COMMAND_COMPLETE) {
		if (scsi_debug) {
			printf("Invalid SCSI message: %x\n", b);
			printf("Status bytes (%d):", i);
			for (b = 0; b < i; b++) {
				printf(" %x", cp[b]);
			}
			printf("\n");
		}
		return (0);
	}
	if (scsi_debug) {
		printf("si%d: si_getstatus: Got status (%d) ", SINUM(c), i);
		for (b = 0; b < i; b++) {
			printf(" %x", cp[b]);
		}
		printf("\n");
	}
	if (c->c_scb.busy) {
		return (0);
	}

	/* check for sense data */
	if (c->c_scb.chk) {
		if (recurse) {
			printf("scsi: chk on sense: invalid\n");
			return (0);
		}

		/* save information while we get sense */
		save_cdb = c->c_cdb;
		save_scb = c->c_scb;
		save_dma_addr = un->un_dma_addr;
		save_dma_count = un->un_dma_count;
		save_reg_dma_count = c->c_sir->bcr;

		/* set up for getting sense */
		c->c_cdb.cmd = SC_REQUEST_SENSE;
		c->c_cdb.lun = un->un_lun;
		cdbaddr(&c->c_cdb, 0);
		c->c_cdb.count = sizeof(struct scsi_sense);
		un->un_dma_addr = (int)c->c_sense - (int)DVMA;
		un->un_dma_count = sizeof(struct scsi_sense);

		/* get sense */
		if ((s=0),si_cmd(c, un, 0) == 0 ||
		    (s=1),si_cmdwait(c) == 0 ||
		    (s=2),si_getstatus(un, 1) == 0) {
			printf("scsi: cannot get sense %d\n", s);
			si_off(un);
			retval = 0;
		} 

		/* restore pre sense information */
		c->c_cdb = save_cdb;
		c->c_scb = save_scb;
		un->un_dma_addr = save_dma_addr;
		un->un_dma_count = save_dma_count;
		c->c_sir->bcr = save_reg_dma_count;
	}
	return (retval);
}

/* 
 * Wait for a scsi dma request to complete.
 * Disconnects were disabled in si_cmd() when polling for command completion.
 */
si_cmdwait(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register struct scsi_unit *un = c->c_un;
	register u_char junk;

	/* wait for dma transfer to complete */
	if (si_wait((u_short *)&sir->csr, SI_CSR_DMA_ACTIVE, 0) == 0) {
		printf("si_cmdwait: DMA_ACTIVE still on\n");
		si_reset(c);
		return (0);
	}

	/* if command does not involve dma activity, then we are finished */
	if (un->un_dma_curdir == SI_NO_DATA) {
		return (1);
	}

	/* wait for indication of dma completion */
	if (si_wait((u_short *)&sir->csr, 
	    SI_CSR_SBC_IP|SI_CSR_DMA_IP|SI_CSR_DMA_CONFLICT, 1) == 0) {
		printf("si_cmdwait: dma op never completed\n");
		si_reset(c);
		return (0);
	}

	/* 
	 * For vme host adaptor interface, must disable dma before
	 * accessing any registers other than the csr or a dma
	 * conflict error will occur.
	 */
	if ((c->c_flags & SCSI_ONBOARD) == 0) {
		sir->csr &= ~SI_CSR_DMA_EN;
	}

	/* make sure dma completely complete */
	if ((sir->csr & SI_CSR_SBC_IP) == 0) {
		if (sir->csr & SI_CSR_DMA_BUS_ERR) {
			printf("si_cmdwait: bus error during dma\n");
		} else if (sir->csr & SI_CSR_DMA_CONFLICT) {
			printf("si_cmdwait: invalid reg access during dma\n");
		} else {
			if (c->c_flags & SCSI_ONBOARD)
				printf("si_cmdwait: dma ip, unknown reason\n");
			else
				printf("si_cmdwait: dma overrun\n");
		}
		si_reset(c);
		return (0);
	}

	/* handle special dma recv situations */
	if (un->un_dma_curdir == SI_RECV_DATA) {
		if (si_dma_recv(c) == 0) {
			si_reset(c);
			return (0);
		}
	}

	/* ack sbc interrupt and cleanup */
	junk = sir->sbc_rreg.clr;
	si_dma_cleanup(c);
	return (1);
}

/*
 * Wait for a condition to be (de)asserted on the scsi bus.
 */
si_sbc_wait(reg, cond, set)
	register caddr_t reg;
	register u_char cond;
	register int set;
{
	register int i;
	register u_char regval;

	for (i = 0; i < SI_WAIT_COUNT; i++) {
		regval = *reg;
		if ((set == 1) && (regval & cond)) {
			return (1);
		}
		if ((set == 0) && !(regval & cond)) {
			return (1);
		} 
		DELAY(10);
	}
	return (0);
}

/*
 * Wait for a condition to be (de)asserted.
 */
si_wait(reg, cond, set)
	register u_short *reg;
	register u_short cond;
	register int set;
{
	register int i;
	register u_short regval;

	for (i = 0; i < SI_WAIT_COUNT; i++) {
		regval = *reg;
		if ((set == 1) && (regval & cond)) {
			return (1);
		}
		if ((set == 0) && !(regval & cond)) {
			return (1);
		} 
		DELAY(10);
	}
	return (0);
}

/*
 * Put data onto the scsi bus.
 */
si_putdata(c, phase, data, num)
	register struct scsi_ctlr *c;
	register u_short phase;
	register u_char *data;
	register int num;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register int i;

	/* set up tcr so a phase match will occur */
	if (phase == PHASE_COMMAND) {
		sir->sbc_wreg.tcr = TCR_COMMAND;
	} else if (phase == PHASE_MSG_OUT) {
		sir->sbc_wreg.tcr = TCR_MSG_OUT;
	} else {
		if (scsi_debug)
			printf("si_putdata %d phase not supported\n", phase);
		return (0);
	}

	/* put all desired bytes onto scsi bus */
	for (i = 0; i < num; i++ ) {

		/* wait for target to request a byte */
		if (si_sbc_wait((caddr_t)&sir->sbc_rreg.cbsr, SBC_CBSR_REQ, 1) 
		    == 0) {
			if (scsi_debug)
				printf("si: putdata, REQ not active\n");
			si_reset(c);
			return (0);
		}

		/* make sure phase match occurred */
		if ((sir->sbc_rreg.bsr & SBC_BSR_PMTCH) == 0) {
			if (phase != PHASE_MSG_OUT) {
				if (scsi_debug)
					printf("si: putdata, phase mismatch\n");
				si_reset(c);
			} else {
				sir->sbc_wreg.tcr = 0;
			}
			return (0);
		}

		/* load data */
		sir->sbc_wreg.odr = *data++;
		sir->sbc_wreg.icr = SBC_ICR_DATA;

		/* complete req/ack handshake */
		sir->sbc_wreg.icr |= SBC_ICR_ACK;
		if (si_sbc_wait((caddr_t)&sir->sbc_rreg.cbsr, SBC_CBSR_REQ, 0) 
		    == 0) {
			if (scsi_debug)
				printf("si: putdata, req not INactive\n");
			si_reset(c);
			return (0);
		}
		sir->sbc_wreg.icr = 0;
	}
	sir->sbc_wreg.tcr = 0;
	return (1);
}

/*
 * Get data from the scsi bus.
 */
si_getdata(c, phase)
	register struct scsi_ctlr *c;
	register u_short phase;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register u_char data;

	/* check for valid phase */
	if (phase == PHASE_STATUS) {
		sir->sbc_wreg.tcr = TCR_STATUS;
	} else if (phase == PHASE_MSG_IN) {
		sir->sbc_wreg.tcr = TCR_MSG_IN;
	} else {
		if (scsi_debug)
			printf("si: getdata %d phase not supported\n", phase);
		return (-1);
	}

	/* wait for target request */
	if (si_sbc_wait((caddr_t)&sir->sbc_rreg.cbsr, SBC_CBSR_REQ, 1) == 0) {
		if (scsi_debug) {
			printf("si: getdata, REQ not active, cbsr %x\n",
			    sir->sbc_rreg.cbsr);
		}
		sir->sbc_wreg.tcr = 0;
		return (-1);
	}

	/* check for correct phase on scsi bus */
	if ((sir->sbc_rreg.bsr & SBC_BSR_PMTCH) == 0) {
		if (phase != PHASE_STATUS) {
			if (scsi_debug) {
				printf("si: getdata, bad phase\n");
			}
			si_reset(c);
		} else {
			sir->sbc_wreg.tcr = 0;
		}
		return (-1);
	}

	/* grab data and complete req/ack handshake */
	data = sir->sbc_rreg.cdr;
	sir->sbc_wreg.icr = SBC_ICR_ACK;
	if (si_sbc_wait((caddr_t)&sir->sbc_rreg.cbsr, SBC_CBSR_REQ, 0) == 0) {
		if (scsi_debug) 
			printf("si: getdata, REQ not inactive\n");
		si_reset(c);
		return (-1);
	}
	sir->sbc_wreg.icr = 0;
	sir->sbc_wreg.tcr = 0;
	return (data);
}

/*
 * Reset SCSI control logic and bus.
 */
si_reset(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = c->c_sir;
	register u_char junk;

	if (scsi_debug) {
		printf("scsi reset, sr= %x, bcr= %x\n", sir->csr, sir->bcr);
	}

	/* reset scsi control logic */
	sir->bcr = 0;
	sir->csr = 0;
	DELAY(10);
	sir->csr = SI_CSR_SCSI_RES|SI_CSR_FIFO_RES;
	if ((c->c_flags & SCSI_ONBOARD) == 0) {
		sir->dma_addr = 0;
		sir->dma_count = 0;
	}

	/* issue scsi bus reset (make sure interrupts from sbc are disabled) */
	sir->sbc_wreg.icr = SBC_ICR_RST;
	DELAY(10);
	sir->sbc_wreg.icr = 0;
	junk = sir->sbc_rreg.clr;

	/* enable sbc interrupts */
	sir->csr |= SI_CSR_INTR_EN;
	sir->sbc_wreg.mr = SBC_MR_EEI;

	/* reconnection attempts are no longer enabled */
	c->c_flags &= ~SCSI_EN_RECON;

	/* disconnect queue needs to be flushed */
	if (c->c_disqh != NULL) {
		c->c_flags |= SCSI_FLUSH_DISQ;
		c->c_flush = c->c_disqt;
	}
}

/*
 * Return residual count for a dma.
 */
si_dmacnt(c)
	register struct scsi_ctlr *c;
{
	if (c->c_flags & SCSI_ONBOARD) {
		return (c->c_sir->bcr);
	} else {
		return ( ((c->c_sir->bcrh) << 16) | (c->c_sir->bcr) );
	}
}

#endif NSI > 0
