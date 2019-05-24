#ifndef lint
static	char sccsid[] = "@(#)autoconf.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Setup the system to run on the current machine.
 *
 * Configure() is called at boot time and initializes the Mainbus
 * device tables and the memory controller monitoring.  Available
 * devices are determined (from possibilities mentioned in ioconf.c),
 * and the drivers are initialized.
 */

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/buf.h"
#include "../h/dk.h"
#include "../h/vm.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"

#include "../machine/pte.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/scb.h"
#include "../machine/mbvar.h"
#include "../machine/zsvar.h"
#include "../machine/sunromvec.h"
#include "../machine/idprom.h"

/*
 * The following several variables are related to
 * the configuration process, and are used in initializing
 * the machine.
 */
int	dkn;		/* number of iostat dk numbers assigned so far */

/*
 * This allocates the space for the per-Mainbus information.
 */
struct	mb_hd mb_hd;

/*
 * Determine mass storage and memory configuration for a machine.
 * Get cpu type, and then switch out to machine specific procedures
 * which will probe adaptors to see what is out there.
 */
configure()
{

	idprom();
	/*
	 * Configure the Mainbus.
	 */
	mbconfig();
#ifdef GENERIC
	setconf();
#endif
}

static	int	(*vec_save)();	/* used to save original vector value */

/*
 * Find devices on the Mainbus.
 * Uses per-driver routine to probe for existence of the device
 * and then fills in the tables, with help from a per-driver
 * slave initialization routine.
 */
mbconfig()
{
	register struct mb_device *md;
	register struct mb_ctlr *mc;
	u_short *reg;
	struct mb_driver *mdr;
	u_short *doprobe();

	vec_save = scb.scb_user[0];	/* save default trap routine */

	/*
	 * Grab some memory to record the Mainbus address space in use,
	 * so we can be sure not to place two devices at the same address.
	 * If we run out of kernelmap space, we could reuse the mapped
	 * pages if we did all probes first to determine the target
	 * locations and sizes, and then remucked with the kernelmap to
	 * share spaces, then did all the attaches.
	 *
	 * We could use just 1/8 of this (we only want a 1 bit flag) but
	 * we are going to give it back anyway, and that would make the
	 * code here bigger (which we can't give back), so ...
	 */

	/*
	 * Check each Mainbus mass storage controller.
	 * See if it is really there, and if it is record it and
	 * then go looking for slaves.
	 */
	for (mc = mbcinit; mdr = mc->mc_driver; mc++) {
		if ((reg = doprobe((u_long)mc->mc_addr, (u_long)mc->mc_space,
		    mdr, mdr->mdr_cname, mc->mc_ctlr, mc->mc_intpri,
		    mc->mc_intr)) == 0)
			continue;
		mc->mc_alive = 1;
		mc->mc_mh = &mb_hd;
		mc->mc_addr = (caddr_t)reg;
		if (mdr->mdr_cinfo)
			mdr->mdr_cinfo[mc->mc_ctlr] = mc;
		for (md = mbdinit; md->md_driver; md++) {
			if (md->md_driver != mdr || md->md_alive ||
			    md->md_ctlr != mc->mc_ctlr && md->md_ctlr != '?')
				continue;
			if ((*mdr->mdr_slave)(md, reg)) {
				md->md_alive = 1;
				md->md_ctlr = mc->mc_ctlr;
				md->md_hd = &mb_hd;
				md->md_addr = (caddr_t)reg;
				if (md->md_dk && dkn < DK_NDRIVE)
					md->md_dk = dkn++;
				else
					md->md_dk = -1;
				md->md_mc = mc;
				/* md_type comes from driver */
				if (mdr->mdr_dinfo)
					mdr->mdr_dinfo[md->md_unit] = md;
				printf("%s%d at %s%d slave %d\n",
				    mdr->mdr_dname, md->md_unit,
				    mdr->mdr_cname, mc->mc_ctlr, md->md_slave);
				if (mdr->mdr_attach)
					(*mdr->mdr_attach)(md);
			}
		}
	}

	/*
	 * Now look for non-mass storage peripherals.
	 */
	for (md = mbdinit; mdr = md->md_driver; md++) {
		if (md->md_alive || md->md_slave != -1)
			continue;
		if ((reg = doprobe((u_long)md->md_addr, (u_long)md->md_space,
		    mdr, mdr->mdr_dname, md->md_unit, md->md_intpri,
		    md->md_intr)) == 0)
			continue;
		md->md_hd = &mb_hd;
		md->md_alive = 1;
		md->md_addr = (caddr_t)reg;
		md->md_dk = -1;
		/* md_type comes from driver */
		if (mdr->mdr_dinfo)
			mdr->mdr_dinfo[md->md_unit] = md;
		if (mdr->mdr_attach)
			(*mdr->mdr_attach)(md);
	}
}

/*
 * Make non-zero if want to be set up to handle
 * both vectored and auto-vectored interrupts
 * for the same device at the same time.
 */
int paranoid = 0;

/*
 * Probe for a device or controller at the specified addr.
 * The space argument give the page type and cpu type for the device.
 */
u_short *
doprobe(addr, space, mdr, dname, unit, br, vp)
	register u_long addr, space;
	register struct mb_driver *mdr;
	char *dname;
	int unit, br;
	register struct vec *vp;
{
	register u_short *reg = NULL;
	char *name;
	long a = 0;
	int i, extent, machine;
	u_int pageval;

#define SP_MACHMASK	0xFFFF0000	/* space mask for machine type */
#define	MAKE_MACH(m)	((m)<<16)
#define	SP_MACH_ALL	MAKE_MACH(0)

#define SP_BUSMASK	0x0000FFFF	/* mask for bus type */
#define SP_VIRTUAL	0x00000001
#define SP_OBMEM	0x00000002
#define SP_OBIO		0x00000004
#define	SP_VME16D16	0x00000100
#define	SP_VME24D16	0x00000200
#define	SP_VME32D16	0x00000400
#define	SP_VME16D32	0x00001000
#define	SP_VME24D32	0x00002000
#define	SP_VME32D32	0x00004000

	machine = space & SP_MACHMASK;

	if (machine != SP_MACH_ALL && machine != MAKE_MACH(cpu & CPU_MACH))
		return(0);

	switch (space & SP_BUSMASK) {

	case SP_VIRTUAL:
		name = "virtual";
		reg = (u_short *)addr;
		break;

	case SP_OBMEM:
		name = "obmem";
		pageval = PGT_OBMEM | btop(addr);
		break;

	case SP_OBIO:
		name = "obio";
		pageval = PGT_OBIO | btop(addr);
		break;

	case SP_VME16D16:
		name = "vme16d16";
		pageval = PGT_VME_D16 | btop(VME16_BASE | (addr & VME16_MASK));
		break;

	case SP_VME24D16:
		name = "vme24d16";
		pageval = PGT_VME_D16 | btop(VME24_BASE | (addr & VME24_MASK));
		break;

	case SP_VME32D16:
		name = "vme32d16";
		pageval = PGT_VME_D16 | btop(addr);
		break;

	case SP_VME16D32:
		name = "vme16d32";
		pageval = PGT_VME_D32 | btop(VME16_BASE | (addr & VME16_MASK));
		break;

	case SP_VME24D32:
		name = "vme24d32";
		pageval = PGT_VME_D32 | btop(VME24_BASE | (addr & VME24_MASK));
		break;

	case SP_VME32D32:
		name = "vme32d32";
		pageval = PGT_VME_D32 | btop(addr);
		break;

	default:
		return (0);
	}

	if (reg == NULL) {
		int offset = addr & PGOFSET;

		extent = btoc(mdr->mdr_size + offset);
		if (extent == 0)
			extent = 1;
		if ((a = rmalloc(kernelmap, (long)extent)) == 0)
			panic("out of kernelmap for devices");
		reg = (u_short *)((int)kmxtob(a) | offset);
		mapin(&Usrptmap[a], btop(reg), pageval, extent, PG_V | PG_KW);
	}

	i = (*mdr->mdr_probe)(reg, unit);
	if (i == 0) {
		if (a)
			rmfree(kernelmap, (long)extent, a);
		return (0);
	}
	printf("%s%d at %s %x ", dname, unit, name, addr);
	if (br < 0 || br >= 7) {
		printf("bad priority (%d)\n", br);
		if (a)
			rmfree(kernelmap, (long)extent, a);
		return (0);
	}

	/*
	 * If br is 0, then no priority was specified in the
	 * config file and the device cannot use interrupts.
	 */
	if (br != 0) {
		/*
		 * If we are paranoid or vectored interrupts are not
		 * going to be used then set up for polling interrupts.
		 */
		if (paranoid || vp == (struct vec *)0) {
			printf("pri %d ", br);
			addintr(br, mdr);
		}

		/*
		 * now set up vectored interrupts if conditions are right
		 */
		if (vp != (struct vec *)0) {
			for (; vp->v_func; vp++) {
				printf("vec 0x%x ", vp->v_vec);
				if (vp->v_vec < VEC_MIN || vp->v_vec > VEC_MAX)
					panic("bad vector");
				else if (scb.scb_user[vp->v_vec - VEC_MIN] !=
				    vec_save)
					panic("duplicate vector");
				else
					scb.scb_user[vp->v_vec - VEC_MIN] =
					    vp->v_func;
			}
		}
	}
	printf("\n");
	return (reg);
}

#define SPURIOUS	0x80000000	/* recognized in locore.s */

int level2_spurious, level3_spurious, level4_spurious, level6_spurious;

not_serviced2()
{

	call_default_intr();
	if ((level2_spurious++ % 100) == 1)
		printf("iobus level 2 interrupt not serviced\n");
	return (SPURIOUS);
}

not_serviced3()
{

	call_default_intr();
	if ((level3_spurious++ % 100) == 1)
		printf("iobus level 3 interrupt not serviced\n");
	return (SPURIOUS);
}

not_serviced4()
{

	call_default_intr();
	if ((level4_spurious++ % 100) == 1)
		printf("iobus level 4 interrupt not serviced\n");
	return (SPURIOUS);
}

not_serviced6()
{

	call_default_intr();
	if ((level6_spurious++ % 100) == 1)
		printf("iobus level 6 interrupt not serviced\n");
	return (SPURIOUS);
}

typedef	int (*func)();

#define NVECT 10

/*
 * These vectors are used in locore.s to jump to device interrupt routines.
 */
func	level2_vector[NVECT] = {not_serviced2};
func	level3_vector[NVECT] = {not_serviced3};
func	level4_vector[NVECT] = {not_serviced4};
func	level6_vector[NVECT] = {not_serviced6};

func	*vector[7] = {NULL, NULL, level2_vector, level3_vector,
	level4_vector, NULL, level6_vector};

/*
 * Arrange for a driver to be called when a particular 
 * auto-vectored interrupt occurs.
 * NOTE: every device sharing a driver must be on the 
 * same interrupt level for polling interrupts because
 * there is only one entry made per driver.
 */
addintr(lvl, mdr)
	struct mb_driver *mdr;
{
	register func f;
	register func *fp;
	register int i;

	switch (lvl) {
	case 1:
		return;		/* bogus - these devices don't interrupt */
	case 2:
		fp = level2_vector;
		break;
	case 3:
		fp = level3_vector;
		break;
	case 4:
		fp = level4_vector;
		break;
	case 5:
		panic("addintr called with level 5");
		/* NOTREACHED */
	case 6:
		fp = level6_vector;
		break;
	default:
		panic("addintr: unknown level");
		/* NOTREACHED */
	}
	if ((f = mdr->mdr_intr) == NULL)
		return;
	for (i = 0; i < NVECT; i++) {
		if (*fp == NULL)	/* end of list found */
			break;
		if (*fp == f)		/* already in list */
			return;
		fp++;
	}
	if (i >= NVECT)
		panic("addintr: too many devices");
	fp[0] = fp[-1];		/* move not_serviced to end */
	fp[-1] = f;		/* add f to list */
}

/*
 * This is for crazy devices that don't know when they interrupt.
 * We just call them at the end after all the sane devices have decided
 * the interrupt is not their fault.
 */
func	default_intrs[NVECT];

add_default_intr(f)
	func f;
{
	register int i;
	register func *fp;

	fp = default_intrs;
	for (i = 0; i < NVECT; i++) {
		if (*fp == NULL)	/* end of list found */
			break;
		if (*fp == f)		/* already in list */
			return;
		fp++;
	}
	if (i >= NVECT)
		panic("add_default_intr: too many devices");
	*fp = f;		/* add f to list */
}

call_default_intr()
{
	register func *fp;

	for (fp = default_intrs; *fp; fp++)
		(*fp)();
}

/*
 * Some things, like cputype, are contained in the idprom, but are
 * needed and obtained earlier; hence they are not set (again) here.
 */
idprom()
{
	register u_char *cp, val = 0;
	register int i;
	struct idprom id;

	getidprom((char *)&id);
	cp = (u_char *)&id;
	for (i = 0; i < 16; i++)
		val ^= *cp++;
	if (val != 0)
		printf("WARNING: ID prom checksum error\n");
	if (id.id_format == 1) {
		localetheraddr(id.id_ether, NULL);
	} else
		printf("INVALID FORMAT CODE IN ID PROM\n");
}

int cpudelay = 3;		/* default to a medium range value here */

/*
 * We set the cpu type and associated variables.  Should there get to
 * be too many variables, they should be collected together in a
 * structure and indexed by cpu type.
 */
setcputype()
{
	struct idprom id;

	cpu = -1;
	getidprom((char *)&id);
	if (id.id_format == 1) {
		switch (id.id_machine) {
		case CPU_SUN3_160:
		case CPU_SUN3_50:
		case CPU_SUN3_260:
			cpu = id.id_machine;
			break;
		default:
			printf("UNKNOWN MACHINE TYPE 0x%x IN ID PROM\n",
			    id.id_machine);
			break;
		}
	} else
		printf("INVALID FORMAT TYPE IN ID PROM\n");

	if (cpu == -1) {
		printf("DEFAULTING MACHINE TYPE TO SUN3_160\n");
		cpu = CPU_SUN3_160;
	}

	/*
	 * Can't use the last segment for DVMA.
	 * The last is for on-board Ethernet scratch,
	 * u area, and miscellanous on-board devices.
	 * On the Sun-3, we can set dvmasize independent
	 * of the implementation.
	 */
	dvmasize = btoc(DVMASIZE) - NPAGSEG;

	switch (cpu) {
	case CPU_SUN3_160:
#ifndef SUN3_160
		panic("not configured for SUN3_160");
#endif !SUN3_160
		cpudelay = 3;
		break;
	case CPU_SUN3_50:
#ifndef SUN3_50
		panic("not configured for SUN3_50");
#endif !SUN3_50
		cpudelay = 3;
		break;
	case CPU_SUN3_260:
#ifndef SUN3_260
		panic("not configured for SUN3_260");
#endif !SUN3_260
		cpudelay = 2;
		break;
	}
}

machineid()
{
	struct idprom id;
	register int x;

	getidprom((char *)&id);
	x = id.id_machine << 24;
	x += id.id_serial;
	return (x);
}
