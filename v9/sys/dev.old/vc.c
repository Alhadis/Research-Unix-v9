#include "vc.h"
#if NVC > 0
/*
 * Versatec model 122 matrix printer/plotter
 * dma interface driver
 *
 */
#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/map.h"
#include "../h/pte.h"
#include "../h/ubavar.h"
#include "../h/ubareg.h"
#include "../h/vc_cmd.h"

unsigned minvcph();

#define	VCPRI	(PZERO-1)

struct	vcdevice {
	short	int	csr;
	short	int	buf;
};

#define	VC_ERROR	0100000
#define	VC_UNUSED1	0040000
#define	VC_SPP		0020000
#define	VC_BTCNT	0010000
#define	VC_ADDR		0004000
#define	VC_PP		0002000
#define	VC_SWPBT	0001000
#define	VC_UNUSED2	0000400	
#define	VC_READY	0000200
#define	VC_IENABLE	0000100
#define	VC_AD17		0000040
#define	VC_AD16		0000020
#define	VC_REMOTE	0000016
#define	VC_REMOTE2	0000010
#define	VC_REMOTE1	0000004
#define	VC_REMOTE0	0000002
#define	VC_DMAGO	0000001

#define VC_MASK		(VC_SPP | VC_PP | VC_SWPBT | VC_REMOTE)

struct vc_softc {
	int	sc_open;
	int	sc_count;
	int	sc_bufp;
	struct	buf *sc_bp;
	int	sc_ubinfo;
} vc_softc[NVC];

/* remote functions */
#define	VC_RLTER	(01 << 1)
#define	VC_CLEAR	(02 << 1)
#define	VC_RESET	(03 << 1)
#define	VC_RFFED	(04 << 1)
#define	VC_REOTR	(05 << 1)
#define	VC_RESET_ALL	(06 << 1)

/* high order 2 address bits */
#define	VC_XMEM(x)	((x) << 4)

struct	uba_device *vcdinfo[NVC];

#define	VCUNIT(dev)	(minor(dev))

struct	buf rvcbuf[NVC];

int	vcprobe(), vcattach();
struct	uba_device *vcdinfo[NVC];
u_short	vcstd[] = { 0777514, 0 };
struct	uba_driver vcdriver =
    { vcprobe, 0, vcattach, 0, vcstd, "vc", vcdinfo };

vcprobe(reg)
	caddr_t reg;
{
	register int br, cvec;		/* value-result */
	register struct vcdevice *vcaddr = (struct vcdevice *) reg;

	vcaddr->csr = VC_IENABLE;
	DELAY(10000);
	vcaddr->csr = 0;
}

/*ARGSUSED*/
vcattach(ui)
	struct uba_device *ui;
{

}

vcopen(dev)
	dev_t dev;
{
	register struct vc_softc *sc;
	register struct vcdevice *vcaddr;
	register struct uba_device *ui;
	if (VCUNIT(dev) >= NVC ||
	    ((sc = &vc_softc[minor(dev)])->sc_open) ||
	    (ui = vcdinfo[VCUNIT(dev)]) == 0 ||
	    (ui->ui_alive == 0)) {
		u.u_error = ENXIO;
		return;
	}
	vcaddr = (struct vcdevice *)ui->ui_addr;
	sc->sc_open = -1;
	sc->sc_count = 0;
	vcaddr->csr = VC_RESET_ALL;
	while (vcwait(dev));
	vcaddr->csr |= VC_IENABLE;
	vctimo(dev);
	if (u.u_error)
		vcclose(dev);
}

vcstrategy(bp)
	register struct buf *bp;
{
	register int e, oldpri;
	register struct vc_softc *sc = &vc_softc[VCUNIT(bp->b_dev)];
	register struct uba_device *ui = vcdinfo[VCUNIT(bp->b_dev)];
	register struct vcdevice *vcaddr = (struct vcdevice *)ui->ui_addr;

	oldpri = spl4();
	e = vcwait(bp->b_dev);
	sc->sc_bp = bp;
	sc->sc_ubinfo = ubasetup(ui->ui_ubanum, bp, UBA_NEEDBDP);
	if (e = vcwait(bp->b_dev))
		goto brkout;
	sc->sc_count = -(bp->b_bcount);
	vcstart(bp->b_dev);
	e = vcwait(bp->b_dev);
	sc->sc_count = 0;
brkout:
	(void) splx(oldpri);
	ubarelse(ui->ui_ubanum, &sc->sc_ubinfo);
	sc->sc_bp = 0;
	iodone(bp);
	if (e)
		u.u_error = EIO;
	wakeup((caddr_t)sc);
}

int	vcblock = 16384;

unsigned
minvcph(bp)
	struct buf *bp;
{

	if (bp->b_bcount > vcblock)
		bp->b_bcount = vcblock;
}

/*ARGSUSED*/
vcwrite(dev)
	dev_t dev;
{

	physio(vcstrategy, &rvcbuf[VCUNIT(dev)], dev, B_WRITE, minvcph);
}

vcwait(dev)
	dev_t dev;
{
	register int e;
	register struct vcdevice *vcaddr =
	    (struct vcdevice *)vcdinfo[VCUNIT(dev)]->ui_addr;
	register struct vc_softc *sc = &vc_softc[VCUNIT(dev)];

	for (;;) {
		if (vcaddr->csr & (VC_READY | VC_ERROR))
			break;
		sleep((caddr_t)sc, VCPRI);
	}
	if (!(vcaddr -> csr & VC_ERROR)) return(0);
	/* Check for NXM - VC_ERROR remains 1 if NOT NXM) */
	vcaddr -> csr &= ~VC_ERROR;
	if ((e = (vcaddr -> csr & VC_ERROR)) == 0)
		printf("vc%d: NXM\n", VCUNIT(dev));
	return (e);
}

vcstart(dev)
	dev_t;
{
	register struct vc_softc *sc = &vc_softc[VCUNIT(dev)];
	register struct vcdevice *vcaddr =
	    (struct vcdevice *)vcdinfo[VCUNIT(dev)]->ui_addr;

	if (sc->sc_count == 0) return;
	vcaddr->csr |= VC_IENABLE | VC_ADDR;
	vcaddr->csr &= ~VC_BTCNT;
	vcaddr->buf = sc->sc_ubinfo;
	vcaddr->csr |= VC_BTCNT;
	vcaddr->buf = sc->sc_count; /* count complemented in strategy */
	vcaddr->csr |= VC_DMAGO | VC_XMEM((sc -> sc_ubinfo >> 16) & 3);
}

/*ARGSUSED*/
vcioctl(dev, cmd, addr, flag)
	dev_t dev;
	int cmd;
	register caddr_t addr;
	int flag;
{
	register int m;
	register struct vc_softc *sc = &vc_softc[VCUNIT(dev)];
	register struct vcdevice *vcaddr =
	    (struct vcdevice *)vcdinfo[VCUNIT(dev)]->ui_addr;

	switch (cmd) {

	case VGETSTATE:
		(void) suword(addr, vcaddr -> csr);
		return;

	case VSETSTATE:
		m = fuword(addr);
		if (m == -1) {
			u.u_error = EFAULT;
			return;
		}
		vccmd(dev, m);
		break;

	default:
		u.u_error = ENOTTY;
		return;
	}
}

vccmd(dev, vcmd)
	dev_t dev;
	int vcmd;
{
	register int e, oldpri;
	register struct vc_softc *sc = &vc_softc[VCUNIT(dev)];
	register struct vcdevice *vcaddr =
	    (struct vcdevice *)vcdinfo[VCUNIT(dev)]->ui_addr;
	oldpri = spl4();
	e = (vcaddr -> csr & ~VC_MASK) | (vcmd & VC_MASK);
	vcaddr -> csr = e;
	if (e = vcwait(dev))
		u.u_error = EIO;
	(void) splx(oldpri);
}

vctimo(dev)
	dev_t dev;
{
	register struct vc_softc *sc = &vc_softc[VCUNIT(dev)];

	if (sc->sc_open)
		timeout(vctimo, (caddr_t)dev, hz/10);
	vcintr(dev);
}

/*ARGSUSED*/
vcintr(dev)
	dev_t dev;
{
	register struct vc_softc *sc = &vc_softc[VCUNIT(dev)];

	wakeup((caddr_t)sc);
}

vcclose(dev)
	dev_t dev;
{
	register struct vc_softc *sc = &vc_softc[VCUNIT(dev)];
	register struct vcdevice *vcaddr =
	    (struct vcdevice *)vcdinfo[VCUNIT(dev)]->ui_addr;

	sc->sc_open = 0;
	sc->sc_count = 0;
	sc->sc_ubinfo = 0;
	vcaddr->csr = 0;
}

vcreset(uban)
	int uban;
{
	register int vc11;
	register struct uba_device *ui;
	register struct vc_softc *sc = vc_softc;
	register struct vcdevice *vcaddr;

	for (vc11 = 0; vc11 < NVC; vc11++, sc++) {
		if ((ui = vcdinfo[vc11]) == 0 ||
		     ui->ui_alive == 0 ||
		     ui->ui_ubanum != uban ||
		     ((sc->sc_open) == 0))
			continue;
		printf(" vc%d", vc11);
		vcaddr = (struct vcdevice *)ui->ui_addr;
		vcaddr->csr = VC_IENABLE;
		if (!(vcaddr -> csr & VC_READY))
			continue;
		if (sc->sc_ubinfo) {
			printf("<%d>", (sc->sc_ubinfo>>28) & 0xf);
			ubarelse(ui->ui_ubanum, &sc->sc_ubinfo);
		}
		sc->sc_count = -(sc->sc_bp->b_bcount);
		sc->sc_ubinfo = ubasetup(ui->ui_ubanum, sc->sc_bp, UBA_NEEDBDP);
		vcstart(sc->sc_bp->b_dev);
	}
}
#endif
