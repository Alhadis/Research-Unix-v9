#ifndef lint
static  char sccsid[] = "@(#)bwtwo.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Sun Two Black & White Board(s) Driver
 */

#include "bwtwo.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/proc.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/file.h"
#include "../h/map.h"
#include "../h/vmmac.h"

#include "../sun/fbio.h"

#include "../machine/pte.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"

#include "../sundev/mbvar.h"
#include "../sundev/bw2reg.h"

#ifdef sun3
#include "../sun3/eeprom.h"
#endif sun3

extern	char	CADDR1[];

int	copyenpfnum;	/* pfnum before shadow memory mapped in */
caddr_t	copyenvirt = 0;	/* virtual address mapped to shadow memory */

struct bw2_softc {
	caddr_t	image;
} bw2_softc[NBWTWO];

#define	BW2SIZEX	1152
#define	BW2SIZEY	900
#define	BW2SQUARESIZEX	1024
#define	BW2SQUARESIZEY	1024

/*
 * Driver information for auto-configuration stuff.
 */
int	bwtwoprobe(), bwtwoattach(), bwtwointr();
struct	mb_device *bwtwoinfo[NBWTWO];
struct	mb_driver bwtwodriver = {
	bwtwoprobe, 0, bwtwoattach, 0, 0, bwtwointr,
	sizeof (struct bw2dev) + CLBYTES /* XXX */, "bwtwo", bwtwoinfo, 0, 0, 0,
};

/*
 * Only allow opens for writing or reading and writing
 * because reading is nonsensical.
 */
bwtwoopen(dev, flag)
	dev_t dev;
{

	fbopen(dev, flag, NBWTWO, bwtwoinfo);
}

bwtwoclose(dev, flag)
	dev_t dev;
{
}

/*ARGSUSED*/
bwtwommap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
	register caddr_t hold;
	register int unit = minor(dev);
	int page;

	hold = bwtwoinfo[unit]->md_addr;
	bwtwoinfo[unit]->md_addr = bw2_softc[unit].image;
	page = fbmmap(dev, off, prot, NBWTWO, bwtwoinfo, BW2_FBSIZE);
	bwtwoinfo[unit]->md_addr = hold;
	return (page);
}

/*
 * Determine if a bwtwo exists at the given address.
 */
/*ARGSUSED*/
bwtwoprobe(reg, unit)
	caddr_t reg;
	int	unit;
{
#ifdef sun3
	/* 
	 * For the sun3 we have to rely on the machine type since the bits 
	 * in the enable register may not be reliable.
	 */
	int result;

	/*
	 * XXX - kludge used to get around current mem_rop bug.
	 * We need to have the page before the frame buffer to not have
	 * have any "holes" in them as the mem_rop code will sometimes
	 * access this page.  We kludge here to avoid this problem by
	 * increasing the size of area to be mapped by adding CLBYTES
	 * to the device size mapped by the autoconfig code.  Here
	 * we remap all but the first page so that the first page map
	 * entry is duplicated and then we bump the virtual address
	 * up by CLBYTES in bwtwoattach().
	 */
	mapin(&Sysmap[btoc(reg + CLBYTES - KERNELBASE)],
	    (u_int)btoc(reg + CLBYTES),
	    ((*(u_int *)&Sysmap[btoc(reg - KERNELBASE)]) & PG_PFNUM),
	    btoc(BW2_FBSIZE), PG_V | PG_KW);

	switch (cpu) {
	case CPU_SUN3_160:
		result = BW2_FBSIZE;
		break;
	case CPU_SUN3_50:
		result = BW2_FBSIZE;
		break;
	case CPU_SUN3_260:
		result = 0;
		break;
	default:
		result = 0;
		break;
	}
	return (result);
#endif sun3
#ifdef sun2
	struct	bw2dev *bw2dev = (struct bw2dev *)(reg + CLBYTES); /* XXX */
	register struct	bw2cr *alias1;
	register struct	bw2cr *alias2;
	short	w1;
	register short	w2, wrestore;
	int	result = 0;

	/*
	 * XXX - kludge used to get around current mem_rop bug.
	 * We need to have the page before the frame buffer to not have
	 * have any "holes" in them as the mem_rop code will sometimes
	 * access this page.  We kludge here to avoid this problem by
	 * increasing the size of area to be mapped by adding CLBYTES
	 * to the device size mapped by the autoconfig code.  Here
	 * we remap all but the first page so that the first page map
	 * entry is duplicated and then we bump the virtual address
	 * up by CLBYTES above for bw2dev and in bwtwoattach().
	 */
	mapin(&Sysmap[btoc(reg + CLBYTES - KERNELBASE)],
	    (u_int)btoc(reg + CLBYTES),
	    ((*(u_int *)&Sysmap[btoc(reg - KERNELBASE)]) & PG_PFNUM),
	    btoc(BW2_FBSIZE), PG_V | PG_KW);

	bw2crmapin(bw2dev);
	alias1 = &bw2dev->bw2cr;
	alias2 = alias1 + 1;

	/*
	 * Two adjacent shorts should be the same because
	 * the control register is replicated every 2 bytes
	 * throughout the control page.
	 */
	if ((w1 = peek((short *)alias1)) == -1)
		return (0);
	wrestore = w1;
	((struct bw2cr *)&w1)->vc_copybase = 0xAA & BW2_COPYBASEMASK;
	if (poke((short *)alias1, w1))
		return (0);
	if ((w2 = peek((short *)alias2)) == -1)
		goto restore;
	if (w1 != w2)
		goto restore;
	((struct bw2cr *)&w1)->vc_copybase = ~0xAA & BW2_COPYBASEMASK;
	if (poke((short *)alias1, w1))
		goto restore;
	if ((w2 = peek((short *)alias2)) == -1)
		goto restore;
	if (w1 != w2)
		goto restore;
	result = BW2_FBSIZE;
restore:
	if (poke((short *)alias1, wrestore))
		panic("bwtwoprobe");
	return (result);
#endif sun2
}

/*
 * Set up the softc structure
 */
bwtwoattach(md)
register struct mb_device *md;
{
#ifdef sun2
	register struct bw2dev *bw2dev;
#endif
	int	pfnum;
	caddr_t	fbvirtaddr;
	caddr_t	v;
	int	i;

	/*
	 * XXX - Last part of mem_rop bug kludge, increase the virtual
	 * address set up by autoconfig by CLBYTES as we have remapped
	 * the first page to be a duplicate in bwtwoprobe().
	 */
	md->md_addr += CLBYTES;

	pfnum = getkpgmap(md->md_addr) & PG_PFNUM;
#ifdef sun3
	/*
	 * If we are on a SUN3_50 (Model 25), then we must
	 * reserve the on board memory for the frame buffer.
	 */
	if (cpu == CPU_SUN3_50) {
		if (fbobmemavail == 0)
			panic("No video memory");
		else
			fbobmemavail = 0;
	}
#endif sun3

	/*
	 * Have we passed this way before? 
	 */
	if (fbobmemavail == 0) {
		if (copyenvirt == 0) {
			copyenvirt = (caddr_t)(*romp->v_fbaddr);
			if (pfnum == copyenpfnum)
				bw2_softc[md->md_unit].image = copyenvirt;
			else
				bw2_softc[md->md_unit].image = md->md_addr;
		}
		return;
	}

	/* 
	 * We know that the copy memory can be used.  Use the
	 * shadow memory if the config flags say to use it.
	 */
	if ((md->md_flags & BW2_USECOPYMEM) == 0) {
		/* don't bother using reserved shadow memory */
		copyenvirt  = md->md_addr;
		bw2_softc[md->md_unit].image = md->md_addr;
		return;
	}

	/*
	 * Mark the onboard frame buffer memory as not available
	 * anymore as we are going to use it for copy memory.
	 */
	fbobmemavail = 0;

	if (*romp->v_outsink != OUTSCREEN || *romp->v_fbtype != FBTYPE_SUN2BW)
		fbvirtaddr = (caddr_t)md->md_addr;
	else {
		rmfree(kernelmap, (long)btoc(BW2_FBSIZE), 
			  btokmx((struct pte *)(md->md_addr)));
		mapout(&Usrptmap[btokmx((struct pte *)(md->md_addr))], 
			  btoc(BW2_FBSIZE));
		fbvirtaddr = (caddr_t)(*romp->v_fbaddr);
	} 
	copyenvirt = fbvirtaddr;
	copyenpfnum = getkpgmap(fbvirtaddr) & PG_PFNUM;

	/*
	 * Copy the current frame buffer memory
	 * to the copy memory as we map it in.
	 */
	for (v = (caddr_t)fbvirtaddr, i = btop(OBFBADDR);
	    i < btop(OBFBADDR + BW2_FBSIZE); v += NBPG, i++) {
		mapin(CMAP1, btop(CADDR1),
		    (u_int)(i | PGT_OBMEM), 1, PG_V | PG_KW);
		bcopy(v, CADDR1, NBPG);
		setpgmap(v, (long)(PG_V|PG_KW|PGT_OBMEM|i));
	}

#ifdef sun2
	bw2dev = (struct bw2dev *)md->md_addr;
	i = (OBFBADDR>>BW2_COPYSHIFT) | BW2_COPYENABLEMASK;
	(void) bwtwosetcr(&bw2dev->bw2cr, i, 1);
#endif sun2
#ifdef sun3
	setcopyenable();
#endif
	if (pfnum == copyenpfnum)
		bw2_softc[md->md_unit].image = copyenvirt;
	else
		bw2_softc[md->md_unit].image = md->md_addr;
}

/*ARGSUSED*/
bwtwoioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	register int unit = minor(dev);

	switch (cmd) {

	case FBIOGTYPE: {
		register struct fbtype *fb = (struct fbtype *)data;
#ifdef sun2
		register struct bw2dev *bw2dev =
		    (struct bw2dev *)bwtwoinfo[unit]->md_addr;
#endif
		fb->fb_type = FBTYPE_SUN2BW;
		fb->fb_depth = 1;
		fb->fb_cmsize = 2;
		fb->fb_size = BW2_FBSIZE;
#ifdef sun3
		/*
		 * Look at the eeprom for screen configuration,
		 * if unknown default to standard sizes.
		 */
		switch (EEPROM->ee_diag.eed_scrsize) {
		case EED_SCR_1024X1024:
			fb->fb_height = BW2SQUARESIZEY;
			fb->fb_width = BW2SQUARESIZEX;
			break;

		case EED_SCR_1152X900:
		default:
			fb->fb_height = BW2SIZEY;
			fb->fb_width = BW2SIZEX;
			break;
		}
#endif sun3
#ifdef sun2
		/*
		 * Check for "square screen" jumper
		 */
		if (cpu != CPU_SUN2_120 && bwtwoinfo[unit]->md_unit == 0 &&
		    bw2dev->bw2cr.vc_1024_jumper) {
			fb->fb_height = BW2SQUARESIZEY;
			fb->fb_width = BW2SQUARESIZEX;
		} else {
			fb->fb_height = BW2SIZEY;
			fb->fb_width = BW2SIZEX;
		}
#endif sun2
		break;
		}

	default:
		u.u_error = ENOTTY;
	}
}

bwtwointr()
{
	int bwtwointclear();

	return (fbintr(NBWTWO, bwtwoinfo, bwtwointclear));
}

/*
 * Turn off interrupts on bwtwo board.
 */
#ifdef sun3
/*ARGSUSED*/
bwtwointclear(bw2dev)
	struct	bw2dev *bw2dev;
{

	(void) setintrenable(0);
	return (0);
}
#endif sun3

#ifdef sun2
bwtwointclear(bw2dev)
	struct	bw2dev *bw2dev;
{
	int	int_active;

	int_active = bw2dev->bw2cr.vc_int;
	(void) setintrenable(&bw2dev->bw2cr);
	return (int_active);
}

setvideoenable(bw2cr)
	struct bw2cr *bw2cr;
{

	bwtwosetcr(bw2cr, BW2_VIDEOENABLEMASK, 1);
}

setintrenable(bw2cr)
	struct bw2cr *bw2cr;
{

	bwtwosetcr(bw2cr, BW2_INTENABLEMASK, 0);
}

/*
 * Special access approach to video ctrl register needed because byte writes,
 * generated when do bit writes, replicates itself on the subsequent byte as
 * well (this is a hardware bug).  Thus, we need to access the register as a
 * word.  Also these routines assume that only one bit changes at a time.
 */
bwtwosetcr(bw2cr, mask, value)
	struct	bw2cr *bw2cr;
	short	mask;
	int	value;
{
	register short	w;

	/*
	 * Read word from video control register.
	 */
	w = *((short *)bw2cr);
	/*
	 * Modify bit as requested.
	 */
	if (value)
		w |= mask;
	else
		w &= ~mask;
	/*
	 * Write word back to video control register.
	 */
	*((short *)bw2cr) = w;
	return;
}
#endif sun2

#ifndef sun3
/*
 * Given the video base virtual address,
 * map in the control register address.
 * This lets us handle minor implementation differences.
 */
bw2crmapin(bw2dev)
	struct bw2dev *bw2dev;
{
	struct bw2cr *bw2cr = &bw2dev->bw2cr;
	int pte = getkpgmap((caddr_t)bw2dev);
	int page, delta;

	page = pte & PGT_MASK;

	if (page == PGT_OBMEM)
		delta = (int)BW2MB_CR - (int)BW2MB_FB;
	else if (page == PGT_OBIO)
		delta = (int)BW2VME_CR - (int)BW2VME_FB;
	else
		panic("bwtwocraddr");

	mapin(&Sysmap[btoc((u_int)bw2cr - KERNELBASE)], btoc(bw2cr),
	    pte + btoc(delta), 1, PG_V | (pte & PG_PROT));
}
#endif !sun3
