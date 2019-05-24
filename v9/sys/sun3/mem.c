#ifndef lint
static	char sccsid[] = "@(#)mem.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Memory special file
 */

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/conf.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/vm.h"
#include "../h/cmap.h"

#include "../machine/pte.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/eeprom.h"

#define	M_MEM		0	/* /dev/mem - physical main memory */
#define	M_KMEM		1	/* /dev/kmem - virtual kernel memory & I/O */
#define	M_NULL		2	/* /dev/null - EOF & Rathole */
#define	M_MBMEM		3	/* /dev/mbmem - (not supported) */
#define	M_MBIO		4	/* /dev/mbio - (not supported) */
#define M_VME16D16	5	/* /dev/vme16d16 - VME 16bit addr/16bit data */
#define M_VME24D16	6	/* /dev/vme24d16 - VME 24bit addr/16bit data */
#define M_VME32D16	7	/* /dev/vme32d16 - VME 32bit addr/16bit data */
#define M_VME16D32	8	/* /dev/vme16d32 - VME 16bit addr/32bit data */
#define M_VME24D32	9	/* /dev/vme24d32 - VME 24bit addr/32bit data */
#define M_VME32D32	10	/* /dev/vme32d32 - VME 32bit addr/32bit data */
#define M_EEPROM	11	/* /dev/eeprom - on board eeprom device */
#define	M_KMEMR		12	/* /dev/kmemr - public part of kernel memory
				 * (read only) */

/*
 * Check bus type memory spaces for accessibility on this machine
 */
mmopen(dev, flag) 
	dev_t dev;
{
	switch (minor(dev)) {
	case M_MEM:
	case M_KMEM:
	case M_KMEMR:
	case M_NULL:
		/* standard devices */
		break;

	case M_EEPROM:
		/* all Sun-3 machines must have EEPROM */
		break;

	case M_VME16D16:
	case M_VME24D16:
	case M_VME32D16:
	case M_VME16D32:
	case M_VME24D32:
	case M_VME32D32:
		/* SUN3_50 is the only Sun-3 machine w/o VMEbus */
		if (cpu == CPU_SUN3_50) {
			u.u_error = EINVAL;
			return;
		}
		break;

	case M_MBMEM:
	case M_MBIO:
	default:
		/* Unsupported or unknown type */
		u.u_error = EINVAL;
		return;
	}
	return;
}

mmread(dev)
	dev_t dev;
{
	mmrw(dev, B_READ);
}

mmwrite(dev)
	dev_t dev;
{
	mmrw(dev, B_WRITE);
}

mmrw(dev, rw)
	dev_t dev;
{
	register int o;
	register u_int c, v;
	register struct iovec *iov;
	int error = 0;
	int pgsp;

	while (u.u_count > 0 && u.u_error == 0) {
		switch (minor(dev)) {

		case M_MEM:
			v = btop(u.u_offset);
			if (v >= physmem)
				goto fault;
			mapin(mmap, btop(vmmap), v, 1, PG_V | PG_KR);
			o = (int)u.u_offset & PGOFSET;
			c = min((u_int)(NBPG - o), u.u_count);
			c = min(c, (u_int)(NBPG -((int)u.u_base&PGOFSET)));
			iomove((caddr_t)&vmmap[o], c, rw);
			break;

		case M_KMEM:
			c = u.u_count;
			if (kernacc((caddr_t)u.u_offset, c, rw))
				iomove((caddr_t)u.u_offset, c, rw);
			else
				mmpeekio(rw, (caddr_t)u.u_offset, (int)c);
			break;

		case M_KMEMR:
			if (rw == B_WRITE)
				goto fault;
			c = u.u_count;
			if ((u_long)u.u_offset < (u_long)KERNELBASE)
				goto fault;
			if (!kernacc((caddr_t)u.u_offset, c, B_READ))
				goto fault;
			iomove((caddr_t)u.u_offset, c, rw);
			break;

		case M_NULL:
			if (rw == B_WRITE) {
				u.u_offset += u.u_count;
				u.u_count = 0;
			}
			return;

		case M_EEPROM:
			mmeeprom(rw, (caddr_t)u.u_offset, u.u_count);
			break;

		case M_VME16D16:
			if (u.u_offset >= VME16_SIZE)
				goto fault;
			v = u.u_offset + VME16_BASE;
			pgsp = PGT_VME_D16;
			goto vme;

		case M_VME16D32:
			if (u.u_offset >= VME16_SIZE)
				goto fault;
			v = u.u_offset + VME16_BASE;
			pgsp = PGT_VME_D32;
			goto vme;

		case M_VME24D16:
			if (u.u_offset >= VME24_SIZE)
				goto fault;
			v =  u.u_offset + VME24_BASE;
			pgsp = PGT_VME_D16;
			goto vme;

		case M_VME24D32:
			if (u.u_offset >= VME24_SIZE)
				goto fault;
			v =  u.u_offset + VME24_BASE;
			pgsp = PGT_VME_D32;
			goto vme;

		case M_VME32D16:
			pgsp = PGT_VME_D16;
			goto vme;

		case M_VME32D32:
			pgsp = PGT_VME_D32;
			/* FALL THROUGH */

		vme:
			v = btop(v);

			mapin(mmap, btop(vmmap), pgsp | v, 1,
				rw == B_WRITE ? PG_V|PG_KW : PG_V|PG_KR);
			o = (int)u.u_offset & PGOFSET;
			c = min((u_int)(NBPG - o), u.u_count);
			mmpeekio(rw, &vmmap[o], (int)c);
			break;

		}
	}
	return;
fault:
	u.u_error = EFAULT;
	return;
}

mmpeekio(rw, addr, len)
	caddr_t addr;
	int len;
{
	register int c, o;
	short sh;
	char cm;

	while (len > 0) {
		if ((len|(int)addr) & 1) {
			c = sizeof (char);
			if (rw == B_READ) {
				if ((o = peekc(addr)) == -1)
					goto fault;
				cm = o;
			}
			iomove((caddr_t)&cm, c, rw);
			if (u.u_error)
				return;
			if (rw == B_WRITE && pokec(addr, c))
				goto fault;
		} else {
			c = sizeof (short);
			if (rw == B_READ) {
				if ((o = peek((short *)addr)) == -1)
					goto fault;
				sh = o;
			}
			iomove((caddr_t)&sh, c, rw);
			if (u.u_error)
				return;
			if (rw == B_WRITE && poke((short *)addr, sh))
				goto fault;
		}
		addr += c;
		len -= c;
	}
	return;
fault:
	u.u_error = EFAULT;
	return;
}

/*
 * If eeprombusy is true, then the eeprom has just
 * been written to and cannot be read or written
 * until the required 10 MS has passed.  It is
 * assumed that the only way the EEPROM is written
 * is thru the mmeeprom routine.
 */
int eeprombusy = 0;

mmeepromclear()
{

	eeprombusy = 0;
	wakeup((caddr_t)&eeprombusy);
}

mmeeprom(rw, addr, len)
	caddr_t addr;
	int len;
{
	int o, oo;
	int s;
	char c;

	if ((int)addr > EEPROM_SIZE)
		return (EFAULT);

	while (len > 0) {
		if ((int)addr == EEPROM_SIZE)
			goto fault;			/* EOF */

		s = splclock();
		while (eeprombusy)
			sleep((caddr_t)&eeprombusy, PUSER);
		(void) splx(s);

		if (rw == B_WRITE) {
			iomove((caddr_t)&c, 1, rw);
			if (u.u_error)
				return;
			o = c;
			if ((oo = peekc(EEPROM_ADDR + addr)) == -1)
				goto fault;
			/*
			 * Check to make sure that the data is actually
			 * changing before committing to doing the write.
			 * This avoids the unneeded eeprom lock out
			 * and reduces the number of times the eeprom
			 * is actually written to.
			 */
			if (o != oo) {
				if (pokec(EEPROM_ADDR + addr, (char)o))
					goto fault;
				/*
				 * Block out access to the eeprom for
				 * two clock ticks (longer than > 10 MS).
				 */
				eeprombusy = 1;
				timeout(mmeepromclear, (caddr_t)0, 2);
			}
		} else {
			if ((o = peekc(EEPROM_ADDR + addr)) == -1)
				goto fault;
			c = o;
			iomove((caddr_t)&c, 1, rw);
			if (u.u_error)
				return;
		}
		addr += sizeof (char);
		len -= sizeof (char);
	}
	return;
fault:
	u.u_error = EFAULT;
	return;
}

/*ARGSUSED*/
mmmmap(dev, off, prot)
	dev_t dev;
	off_t off;
{
	int pf;

	switch (minor(dev)) {

	case M_MEM:
		pf = btop(off);
		if (pf < physmem)
			return (PGT_OBMEM | pf);
		break;

	case M_VME16D16:
		if (off >= VME16_SIZE)
			break;
		return (PGT_VME_D16 | btop(off + VME16_BASE));

	case M_VME16D32:
		if (off >= VME16_SIZE)
			break;
		return (PGT_VME_D32 | btop(off + VME16_BASE));

	case M_VME24D16:
		if (off >= VME24_SIZE)
			break;
		return (PGT_VME_D16 | btop(off + VME24_BASE));

	case M_VME24D32:
		if (off >= VME24_SIZE)
			break;
		return (PGT_VME_D32 | btop(off + VME24_BASE));

	case M_VME32D16:
		return (PGT_VME_D16 | btop(off));

	case M_VME32D32:
		return (PGT_VME_D32 | btop(off));

	}
	return (-1);
}
