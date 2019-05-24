#include "im.h"
/*
	Imagitek Scanner -- Andrew Hume
		1100 (?), on DR-11/W
	Ninth Edition Unix (related to BSD 4.1c)
*/
#define	DEBUG

#include "../h/param.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/buf.h"
#include "../h/systm.h"
#include "../h/pte.h"
#include "../h/map.h"
#include "../h/ubareg.h"
#include "../h/ubavar.h"

struct imdevice {
	short	wcr;		/* Unibus word count reg */
	u_short	bar;		/* Unibus address register */
	u_short	csr;		/* Unibus status/command reg */
	u_short	idr;		/* Input Data Register */
};
#define	eir	csr		/* Error and Information Register */
#define	odr	idr		/* Output Data Register */

/*
 * Unibus status/command register bits
 */

#define	GO		0x0001
#define	HSTRB		0x0002		/* host strobe fn1 */
#define	HINTR		0x0004		/* host interrupt fn2 */
#define	HRESET		0x0008		/* host reset fn3 */
#define	IENABLE		0x0040
#define	READY		0x0080
#define	SSTRB		0x0200		/* scanner strobe statc */
#define	SINTR		0x0400		/* scanner interrupt statb */
#define	SRESET		0x0800		/* scanner interface reset stata */
#define	MAINT		0x1000
#define	ATTN		0x2000		/* attention */
#define	NEX		0x4000
#define	ERROR		0x8000

/*
 * IOCTL
 */
#define	IMRESET	(('i'<<8)|1)

#define DMAPRI (PZERO-1)
#define WAITPRI (PZERO+1)
#define	FUNCMASK	(HSTRB|HINTR|HRESET)

#define	PAUSE	15	/* quick responses */
#define	LPAUSE	60	/* long reponses */

int	improbe(), imattach(), imintr();
struct	uba_device *imdinfo[NIM];
u_short	imstd[] = { 0, 0, 0 };
struct	uba_driver imdriver = {
	improbe, 0, imattach, 0, imstd, "im", imdinfo, 0, 0
};

/*
	this is a finite state automata driven by the interrupt routine
*/

enum states {
	IDLE,		/*0 idle or finishing off read */
	WRITEPROBE,	/*1 waiting for scanner ack */
	WRITE,		/*2 started dma to scanner */
	READPROBE,	/*3 waiting for ATTN from scanner */
	READ,		/*4 ready to start dma from scanner */
	RESETTING	/*5 imreset(dev) */
};

struct imsoftc {
	char open;		/* single open */
	enum states state;	/* driver state */
	short busy;		/* are we doing a transfer? */
	int ubinfo;
	int count;
	struct buf *bp;
	int bufp;
	short func;
} imsoftc[NIM];

int	imstrategy();
u_int	imminphys();
struct	buf rimbuf[NIM];

#define UNIT(dev) (minor(dev))

improbe(reg)
	caddr_t reg;
{
	register int br, imec;		/* value-result */
	register struct imdevice *imaddr = (struct imdevice *)reg;
	register csr;

#ifdef lint
	br = 0; imec = br; br = imec;
	imintr(0);
#endif
	if(imaddr->csr&SRESET){	/* offline so we have to set */
		br = 0x15;
		imec = 0174;
	} else {
		csr = IENABLE | HINTR | HRESET | HSTRB;
		imaddr->csr = csr;
		DELAY(10000);
		imaddr->csr = 0;
	}
	return(sizeof (struct imdevice));
}

/*ARGSUSED*/
imattach(ui)
	struct uba_device *ui;
{
}

imopen(dev)
	dev_t dev;
{
	register struct imsoftc *imp;
	register struct uba_device *ui;
	register struct imdevice *imaddr;

	if (UNIT(dev) >= NIM || (imp = &imsoftc[minor(dev)])->open ||
	    (ui = imdinfo[UNIT(dev)]) == 0 || ui->ui_alive == 0)
		u.u_error = EBUSY;
	else {
		imaddr = (struct imdevice *)imdinfo[UNIT(dev)]->ui_addr;
		imp->open = 1;
		imp->state = IDLE;
		imp->busy = 0;
		imaddr->csr = IENABLE;
	}
}

imclose(dev)
	dev_t dev;
{
	register struct imsoftc *imp = &imsoftc[UNIT(dev)];

	if(imp->state != IDLE)
		imreset(dev);
	imp->open = 0;
}

imread(dev)
	dev_t dev;
{
	register int unit = UNIT(dev);
	register struct imsoftc *imp = &imsoftc[unit];
	register struct imdevice *imaddr = (struct imdevice *)imdinfo[unit]->ui_addr;
	int i;
	short status, count;

	spl5();
#ifdef	DEBUG
printf("read: ");
#endif
	while(imp->state == READPROBE){
		if(tsleep((caddr_t)imp, DMAPRI+1, PAUSE) != TS_OK){
			imreset(dev);
			spl0();
			return;
		}
	}
	spl0();
	if(imp->state != READ){
		u.u_error = EGREG;
		return;
	}
	imp->func = 0;
	imaddr->csr = HSTRB;
	physio(imstrategy, &rimbuf[unit], dev, B_READ, imminphys);
	imaddr->csr = IENABLE;
}

imwrite(dev)
	dev_t dev;
{
	register int unit = UNIT(dev);
	register struct imsoftc *imp = &imsoftc[unit];
	register struct imdevice *imaddr = (struct imdevice *)imdinfo[unit]->ui_addr;
	int i;
	short count;
	short csr;

	if(imp->state != IDLE){
		u.u_error = EGREG;
		return;
	}
	imp->func = HINTR;
	if(imaddr->csr&(SINTR|SRESET)){	/* is it offline ? */
		printf("im: offline csr=0x%x\n", imaddr->csr);
		u.u_error = ENXIO;
		return;
	}
	imp->state = WRITEPROBE;
	imaddr->csr = IENABLE | HINTR | HSTRB;	/* poke scanner */
	spl5();
	if((tsleep((caddr_t)imp, DMAPRI+1, PAUSE) != TS_OK) || (imp->state != WRITE)){
		imreset(dev);
		spl0();
		return;
	}
	spl0();
	imaddr->csr = HINTR;
	physio(imstrategy, &rimbuf[unit], dev, B_WRITE, imminphys);
	/*
		the write is done although we may not have both interrupts
	*/
#ifdef	DEBUG
	printf("write done\n");
#endif
}

u_int
imminphys(bp)
	register struct buf *bp;
{

	if(bp->b_bcount > 65536)	/* may be able to do twice as much */
		bp->b_bcount = 65536;
}

imstrategy(bp)
	register struct buf *bp;
{
	register struct imsoftc *imp = &imsoftc[UNIT(bp->b_dev)];
	register struct uba_device *ui;
	register struct imdevice *imaddr;

	if(((ui = imdinfo[UNIT(bp->b_dev)]) == 0) || (ui->ui_alive == 0))
		goto bad;
	(void)spl5();
	imaddr = (struct imdevice *) ui->ui_addr;
	imp->bp = bp;
	if(bp->b_bcount<=0 || (bp->b_bcount&1) || ((int)bp->b_un.b_addr&1)){
		bp->b_error = EINVAL;
		goto bad;
	}
	imp->ubinfo = ubasetup(ui->ui_ubanum, bp, UBA_WANTBDP);
	imp->bufp = imp->ubinfo & 0x3ffff;
	imp->count = -(bp->b_bcount>>1);	/* it's a word count */
	imp->busy = 1;
	imstart(ui);
	if(tsleep((caddr_t)imp, DMAPRI+1, LPAUSE) != TS_OK){
		printf("im timeout: csr=0x%x wcr=%d\n", imaddr->csr, imaddr->wcr);
		/* stop the dma */
		imslam(imaddr);
		imaddr->wcr=0xFFFF;	/* gently; -1 first... */
		imaddr->wcr=0x0000;	/* then zero */
		/* reset device */
		imreset(bp->b_dev);
		ubarelse(ui->ui_ubanum, &imp->ubinfo);
		goto bad;
	}
	imp->bufp = 0;
	(void)spl0();
	ubarelse(ui->ui_ubanum, &imp->ubinfo);
	imp->bp = 0;
	bp->b_resid = -(imp->count<<1);
	iodone(bp);
	return;
   bad:
	(void)spl0();
	bp->b_flags |= B_ERROR;
	iodone(bp);
}

imslam(imaddr)
	register struct imdevice *imaddr;
{
	register i;
	imaddr->csr=MAINT;
	for(i=0; i<10; i++)
		;
	imaddr->csr=0;
	for(i=0; i<10; i++)
		;
}

imstart(ui)
	register struct uba_device *ui;
{
	register int csr;
	register struct imdevice *imaddr = (struct imdevice *) ui->ui_addr;
	register struct imsoftc *imp = &imsoftc[UNIT(ui->ui_unit)];

	csr = IENABLE|((imp->bufp>>12)&060) | imp->func;
	imaddr->wcr =  imp->count;
	imaddr->bar = imp->bufp;
	imaddr->csr = csr;	/* No GO bit; let function codes settle */
	imaddr->csr = csr|GO;
}

/*ARGSUSED*/
imioctl(dev, cmd, data)
	dev_t dev;
	int cmd;
	register caddr_t data;
{
	register struct imsoftc *imp = &imsoftc[UNIT(dev)];

	switch(cmd)
	{
	case IMRESET:
		spl5();
		imreset(dev);
		if((tsleep((caddr_t)imp, WAITPRI, PAUSE) != TS_OK) || (imp->state != IDLE)){
			spl0();
			u.u_error = EIO;
			imp->state = IDLE;
			return;
		}
		u.u_error = 0;
		spl0();
		break;
	default:
		u.u_error = ENOTTY;
		break;
	}
}

imintr(dev)
	dev_t dev;
{
	register struct imdevice *imaddr = (struct imdevice *)imdinfo[UNIT(dev)]->ui_addr;
	register struct imsoftc *imp = &imsoftc[UNIT(dev)];
	register csr;
	int attn = 0;
	int ostate = imp->state;
	int dma;

	csr = imaddr->csr;
	if(csr&ATTN){
		attn = 1;
	} else if(csr&ERROR){
		imaddr->eir |= ERROR;
		printf("im: read goo CSR %x EIR %x BAR %x WCR %x\n", csr, imaddr->eir,
			imaddr->bar, imaddr->wcr);
		imaddr->csr = csr&~FUNCMASK;
		imreset(dev);
	} else {
		;
	}
#ifdef	DEBUG
	printf("imintr: csr=0x%x ostate=%d attn=%d\n", csr, ostate, attn);
#endif
	switch(imp->state)
	{
	case IDLE:
		break;
	case WRITEPROBE:
		if(attn)
			imp->state = WRITE;
		else
			imreset(dev);
		break;
	case WRITE:
		dma = (csr&READY) && (imaddr->wcr == 0);
		if(!dma) printf("im: phooey! wcr=%d\n", imaddr->wcr);
		imaddr->csr = IENABLE;
		imp->count = imaddr->wcr;
		imp->state = READPROBE;
		break;
	case READPROBE:
		if(attn)
			imp->state = READ;
		else
			imreset(dev);
		break;
	case READ:
		/*
			because we may not get both an attn and a dma done
			drop into idle. if we get neither, something will screw
			up; that's ok because the user is in trouble anyway.
		*/
		dma = (csr&READY) && (imaddr->wcr == 0);
		imp->count = imaddr->wcr;
		if((csr & (SRESET|SINTR|SSTRB)) != SSTRB)
			imreset(dev);
		else
			imp->state = IDLE;
		break;
	case RESETTING:
		if((csr & (SRESET|SINTR|SSTRB)) == SRESET){
			imaddr->csr = IENABLE | HSTRB;
			csr = imaddr->idr;	/* release reset */
			return;			/* catch the next interrupt */
		}
		imaddr->csr &= ~FUNCMASK;
		imp->state = IDLE;
		break;
	default:
		printf("imintr: unknown state %d\n", imp->state);
		imreset(dev);
	}
#ifdef	DEBUG
	printf("imdbg: csr=0x%x ostate=%d state=%d\n", csr, ostate, imp->state);
#endif
	wakeup((caddr_t)imp);
}

/*
	imreset must be at spl5()
*/
imreset(dev)
	dev_t dev;
{
	register struct uba_device *ui = imdinfo[UNIT(dev)];
	register int csr;
	register struct imdevice *imaddr = (struct imdevice *) ui->ui_addr;
	register struct imsoftc *imp = &imsoftc[UNIT(ui->ui_unit)];

#ifdef	DEBUG
printf("imreset: state=%d\n", imp->state);
#endif
	if(imaddr->csr&SRESET){
		u.u_error = ENXIO;
		return;
	}
	imp->state = RESETTING;
	csr = IENABLE | HINTR | HRESET | HSTRB;
	imaddr->csr = csr;
	u.u_error = EIO;
}
