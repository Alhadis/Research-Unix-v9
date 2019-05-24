/*
 * This software is provided solely for use with
 * the National Instruments GPIB11-2.
 *
 * Copyright 1980, 1983 National Instruments
 *
 * Jeffrey Kodosky
 * REV D:  10/30/83
 * (4.1bsd mods)
 */

#include "ib.h"

#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dir.h"
#include "../h/callout.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/pte.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/reg.h"
#include "../h/ubareg.h"
#include "../h/ubavar.h"
#include <sys/ioctl.h>


#define TRI	0	/* 4 for Three-state highspeed timing; 0 for Normal timing */
#define EXT	1	/* 2 for Extended; 1 for Normal GPIB addressing */
#define MSA	0140	/* Msa&037 for Extended; 0140 for Normal GPIB addressing */
#define MA	025	/* GPIB address */
#define TRACE	0	/* Leave TRACE undefined to eliminate tracing code */
/*#define DEBUG	  0	  /* DEBUG may be defined (as anything) to include diagnostic xprintfs */
#define	DONE	0x0080	/* this is what they wanted */


u_short ibaddrs[]= { 
	0767710, 0 };	/* Unibus address of GPIB11-2 */

#define SIGSRQ	SIGINT

static struct buf ibbuf;
static int ibmap, ibiomap;
static short iboddc, ibodda;

#ifndef TRACE
#define T
#else
#define T	tracer()
#endif

#define xprintf

#define IB		((struct ib *) ibdinfo->ui_addr)
#define in(a)		IB->a
#define out(a,v)	IB->a=(v)
#define Exclude		sps=spl5()
#define Unexclude	splx(sps)
#define Try(f)		if((x=(f))<0) return x


#define INACTIVE   0		/* Software controller states */
#define IDLE	   1
#define INCHARGE   2
#define STANDBY	   3

#define GO	1		/* Control/status register bits */
#define OUT	2
#define SEL	4
#define ECC	010
#define XBA	060
#define IE	0100
#define LMR	0200
#define CIC	0400
#define ATN	01000
#define EOI	02000
#define OCSW	02000
#define TCS	04000
#define DAV	04000
#define SRQ_IE	010000
#define SRQ	010000
#define INT	020000
#define DMA_ENAB 020000
#define NEX	040000
#define REN	040000
#define IFC	0100000

#define DIR	0		/* Internal register addresses */
#define DOR	0
#define ISR1	1
#define IMR1	1
#define ISR2	2
#define IMR2	2
#define SPS	3
#define SPM	3
#define ADS	4
#define ADM	4
#define CPT	5
#define AUX	5
#define AD0	6
#define ADR	6
#define AD1	7
#define EOS	7

#define ERR_IE	    4		/* Internal register bits */
#define END_IE	  020
#define CPT_IE	 0200
#define DMAI_ENAB 020
#define DMAO_ENAB 040
#define SRQS	 0100
#define RSV	 0100
#define TA	    2
#define LA	    4
#define LON	 0100
#define TON	 0200
#define DL	  040
#define DT	 0100
#define ARS	 0200

#define _CLKR	  040		/* Hidden register addresses & offsets */
#define _PPR	 0140
#define _AUXA	 0200
#define _AUXB	 0240
#define CLKR	    0
#define PPR	    1
#define AUXA	    2
#define AUXB	    3

#define U	  020		/* Hidden register bits */
#define BIN	  020
#define S	  010
#define REOS	    4
#define HLDE	    2
#define HLDA	    1
#define CPT_ENAB    1
#define PACS	    1		/* Software status bits */
#define MON	    2

#define IST	011		/* Special interface functions */
#define NIST	  1
#define VSC	017
#define NVSC	  7
#define SEOI	  6
#define FH	  3
#define IR	  2
#define PON	  0

#define OK	   1	/* Error codes */
#define ENONE	  -1	/* No command byte available (READCOMMAND) */
#define ECACFLT	  -2	/* ATN not unasserted after IFC sent (bus problem) */
#define ENOTCAC	  -3	/* Not Active Controller for operation requiring CAC (software problem) */
#define ENOTSAC	  -4	/* Not System Controller for operation requiring SAC (software problem) */
#define EIFCLR	  -5	/* IFC caused operation to abort (bus problem) */
#define ETIMO	  -6	/* Operation did not complete within allotted time (bus problem) */
#define ENOFUN	  -7	/* Non-existent function code (software problem) */
#define ETCTIMO	  -8	/* Take control not completed within allotted time (bus problem) */
#define ENOIBDEV  -9	/* No Listeners addressed or no devices connected (bus problem) */
#define EIDMACNT -10	/* Internal DMA completed without bcr going to 0 (hardware problem) */
#define ENOPP	 -11	/* PP operation attempted on three-state GPIB (software problem) */
#define EITIMO	 -12	/* Internal DMA did not complete within allotted time (hardware problem) */
#define EINEXM	 -13	/* Internal DMA aborted due to non-existent memory (software/hardware problem) */
#define ENEXMEM	 -14	/* GPIB DMA aborted due to non-existent memory (software/hardware problem) */
#define ECNTRS	 -15	/* Bar and bcr are inconsistent following GPIB DMA (hardware problem) */

#define RQC_STB	 (RSV | 1)	/* Service request status bytes */
#define RQT_STB	 (RSV | 2)
#define RQL_STB	 (RSV | 4)

#define RQC	1		/* Asynchronous op codes */
#define CAC	2
#define TAC	3
#define LAC	4
#define CWT	5
#define WSRQ	6

#define TCT	011		/* GPIB multiline messages */
#define PPC	5
#define PPU	025
#define SCG	0140

#define IN	0
#define ITIMO	25		/* Internal loopcount timeout */
#define GTIMO	10		/* Default GPIB timeout in seconds */
#define TCTIMO	100
#define MONHWAT 32

#define RD	(DMA_ENAB|TCS|IN|GO)
#define WT	(DMA_ENAB|TCS|OUT|GO)
#define RDIR	(DMA_ENAB|IN|SEL|GO)
#define WTIR	(DMA_ENAB|OUT|SEL|GO)


int ibprobe(), ibattach();
struct uba_device *ibdinfo;
struct uba_driver ibdriver =
{
	ibprobe, 0, ibattach, 0, ibaddrs, "ib", &ibdinfo, 0, 0, 0
};

struct	ib {
	short unsigned	bcr, bar, csr, ccf;	/* Unibus device registers */
	char	internal[8];		/* Internal registers */
	char	hidden[4];		/* Hidden registers */
	char	cstate, istr, op;
	int	ans;
	int	timo;			/* Watchdog timer */
	struct proc *owner;		/* GPIB owning process */
	short unsigned	rdbcr, rdbar, rdcsr;
	char	rdinternal[8];
	int	arg[3];
#ifdef TRACE
#define TRSZ	32
	int	trin;
	int	trbuf[TRSZ];
#endif
} ib;

static int status(), spbyte();
static int command(), transfer(), clear(), remote(), local(), ppoll();
static int passctrl(), setstat(), monitor(), readcmd(), setparam(), testsrq();

int (*ibfn[])() =
{
	command, transfer, clear, remote, local, ppoll,
	passctrl, setstat, monitor, readcmd, setparam, testsrq,
	status, spbyte
};

#define NFNS	((sizeof ibfn)/(sizeof ibfn[0]))


ibprobe(ibp)
	struct ib *ibp;
{
	register int br, cvec;

	ibp->csr= LMR;
	ibp->bcr= -1;
	ibp->bar= 0;
	ibp->csr= DMA_ENAB|SEL|OUT|IE|GO;
	DELAY(10000);
	ibp->csr= LMR;
	return 1;
}

ibattach(ui)
	struct uba_device *ui;
{ 
}

ibopen(dev,rw)
{
	register int sps, x;

	Exclude;
	xprintf("open (%o)\n",ibopen);
	if(ib.owner || ibdinfo==0 || ibdinfo->ui_alive==0)
		u.u_error= ENXIO;
	else {	
		T;
		ibmap= uballoc(ibdinfo->ui_ubanum, &ib, sizeof ib, 0);
		xprintf("\tibmap= %o\n", ibmap);
		if((ib.ans=init())<0){
			x= ubarelse(ibdinfo->ui_ubanum, &ibmap);
			xprintf("\tubarelse= %d\n",x);
			u.u_error= EIO;
		}
		else ib.owner= u.u_procp;
	}
	Unexclude;
}

ibclose(dev)
{
	register int sps, x;

	Exclude;
	xprintf("close (%o)\n",ibclose);
	T;
	x= ubarelse(ibdinfo->ui_ubanum, &ibmap);
	xprintf("\tubarelse= %d\n",x);
	ib.cstate= INACTIVE;
	ib.csr= 0;
	out(csr,LMR);
	ib.owner= 0;
	Unexclude;
}

ibioctl(dev,cmd,addr)
	caddr_t addr;
{
	register int sps, x;

	Exclude;
	xprintf("ioctl (%o)\n",ibioctl);
	switch(cmd){
	case TIOCGETP:
		T;
		ib.arg[0]= ib.ans;
		ib.arg[1]= in(csr);
		ib.arg[2]= in(bcr);
		xprintf("\tgtty: %o %o %o\n",ib.arg[0],ib.arg[1],ib.arg[2]);
		if(copyout(ib.arg,addr,sizeof ib.arg)) u.u_error= EFAULT;
		break;
	case TIOCSETP:
		T;
		if(copyin(addr,ib.arg,sizeof ib.arg)){
			u.u_error= EFAULT;
			break;
		}
		xprintf("\tstty: %o %o %o\n",ib.arg[0],ib.arg[1],ib.arg[2]);
		if((x=ib.arg[0])<0 || x>=NFNS){
			ib.ans= ENOFUN; 
			u.u_error= EIO;
		}
		else if((ib.ans= (*ibfn[x])())<0) u.u_error= EIO;
		break;
	default: 
		u.u_error= ENOTTY;
		break;
	}
	Unexclude;
}

static
command()
{
	T;
	ib.op= CWT;
	return OK;
}

static
transfer()
{
	T;
	return gts(EXT);
}

static
clear()
{
	register int x;

	T;
	Try(unhold());
	out(csr, ib.csr | IFC);
	wait100us();
	if(in(csr)&ATN) return ECACFLT;
	ib.cstate= INCHARGE;
	out(csr, (ib.csr |= ATN | SRQ_IE | CIC));
	T;
	return lun(TON);
}

static
remote()
{
	T;
	out(csr, (ib.csr |= REN));
	return OK;
}

static
local()
{
	T;
	out(csr, (ib.csr &= ~REN));
	return OK;
}

static
ppoll()
{
	register int x;

	T;
	Try(tcs());
	Try(lun(LON));
	T;
	out(csr, (ib.csr |= EOI));
	Try(xfer(RDIR, &ib.rdinternal[CPT], 1, CPT));
	out(csr, (ib.csr &= ~EOI));
	return (ib.rdinternal[CPT]&0377|0400);
}

static
passctrl()
{
	T;
	return ENOIBDEV;
}

static
setstat()
{
	T;
	return OK;
}

static
monitor()
{
	T;
	return OK;
}

static
readcmd()
{
	T;
	return ENONE;
}

static
setparam()
{
	T;
	ib.timo= ib.arg[1];
	ib.internal[0]= ib.arg[2];
	ib.internal[EOS]= ib.arg[2]>>8;
	return OK;
}

static
testsrq()
{
	int ibtimer();

	T;
	if(in(csr)&SRQ) return OK;
	if(ib.csr&CIC) out(csr, (ib.csr |= SRQ_IE));
	if(ib.arg[1]){
		ib.op= WSRQ;
		if(ib.timo)
			timeout(ibtimer,0,ib.timo*hz);
		sleep(&ibbuf,10);
		return ib.ans;
	}
	return ENONE;
}

static
status()
{
	register char *c;
	register int ct;

	T;
	c= (caddr_t)&ib;
	u.u_base= (caddr_t)ib.arg[1];
	if((u.u_count=ib.arg[2]) > sizeof ib)
		u.u_count= sizeof ib;
	ct= u.u_count;
	while(passc(*c++)>=0) ;
	return ct;
}

static
spbyte()
{
	register int x;

	T;
	return OK;
}

static
init()
{
	register char *cp;
	register int x;

	out(csr, LMR);
	ib.csr= 0;
	cp= &ib.hidden[0];
	*cp++= _CLKR | 6;
	*cp++= _PPR | U;
	*cp++= _AUXA;
	*cp++= _AUXB | TRI | CPT_ENAB;

	cp= &ib.internal[0];
	*cp++= 0;
	*cp++= CPT_IE | ERR_IE;
	*cp++= 0;
	*cp++= 0;
	*cp++= EXT;
	*cp++= ib.hidden[CLKR];
	*cp++= ARS | MSA;
	*cp++= 0;

	ib.istr= 0;
	ib.op= 0;
	ib.ans= 0;
	ib.timo= GTIMO;
#ifdef TRACE
	ib.trin= 0;
#endif
	Try(irload());
	T;
	ib.cstate= IDLE;
	out(csr, ib.csr= IE);
	return 0;
}

static
ibstop()
{
	register int x;

	T;
	out(csr, (ib.csr &= ~(DMA_ENAB|GO)));
	ib.op= 0;
	ibdone(&ibbuf);
	return ETIMO;
}

static
irload()
{
	register int x;

	Try(xfer(WTIR,&ib.internal[ISR1],7,ISR1));
	ib.internal[AUX]= ib.hidden[AUXA];
	ib.internal[ADR]= MA;
	Try(xfer(WTIR,&ib.internal[AUX],2,AUX));
	Try(xfer(WTIR,&ib.hidden[AUXB],1,AUX));
	x= ib.internal[ADM];
	ib.internal[ADM]= 0;
	return lun(x);
}

static
lun(newadm)
	char newadm;
{	/* Note: rsv is cleared and not restored*/
	register int x;

	if(ib.internal[ADM]==newadm) return OK;
	ib.internal[ADM]= newadm;
	ib.internal[AUX]= PON;
	Try(xfer(WTIR,&ib.internal[ADM],2,ADM));
	Try(xfer(WTIR,&ib.hidden[PPR],1,AUX));
	return (ib.istr&S)? ldaux(IST): OK;
}

ibwrite(dev)
{
	register int sps;
	int ibstrategy();

	Exclude;
	T;
	if((ib.ans=wsetup())>=0)
		physio(ibstrategy, &ibbuf, dev, B_WRITE, minphys);
	ib.op= 0;
	if(ib.ans<0) u.u_error= EIO;
	Unexclude;
}

static
wsetup()
{
	register x;

	ib.ccf= 0;
	ib.csr &= ~ECC;
	if(ib.op==CWT)
		if((x=tcs())<0){
			T;
			return x;
		}
		else {	
			T;
			Try(lun(TON));
			ib.op= CAC;
		}
	else {	
		T;
		if((x=gts(TON))<0){
			T;
			return x;
		}
		if(ib.internal[0]&2){
			ib.ccf= SEOI; 
			ib.csr |= ECC;
		}
		ib.op= TAC;
	}
	ibodda= (int)u.u_base&1;
	u.u_base -= ibodda;
	iboddc= ((ibodda + u.u_count + 1)&~1) - u.u_count;
	u.u_count += iboddc;
	return OK;
}

ibread(dev)
{
	register int sps;
	int ibstrategy();

	Exclude;
	T;
	if((ib.ans=rsetup())>=0)
		physio(ibstrategy, &ibbuf, dev, B_READ, minphys);
	ib.op= 0;
	if(ib.ans<0) u.u_error= EIO;
	Unexclude;
}

static
rsetup()
{
	register int x;

	ib.ccf= 0;
	ib.csr &= ~ECC;
	if((x=gts(LON))<0){
		T;
		return x;
	}
	ib.hidden[AUXA] |= (ib.internal[0]&(2<<4)? REOS:0) | (ib.internal[0]&(1<<4)? BIN:0);
	if(ib.internal[0]&(4<<4)) ib.internal[IMR1] &= ~END_IE;
	else {	
		T;
		ib.internal[IMR1] |= END_IE;
		if(ib.cstate==STANDBY) ib.hidden[AUXA] |= HLDE;
	}
	ib.internal[AUX]= ib.hidden[AUXA];
	Try(xfer(WTIR, &ib.internal[IMR1], 7, IMR1));
	if(ib.cstate==STANDBY){
		T;
		ib.ccf= ib.hidden[AUXA]= ib.hidden[AUXA]&~HLDE|HLDA;
		ib.csr |= ECC;
	}
	ib.op= LAC;
	ibodda= (int)u.u_base&1;
	u.u_base -= ibodda;
	iboddc= ((ibodda + u.u_count + 1)&~1) - u.u_count;
	u.u_count += iboddc;
	return OK;
}

ibstrategy(bp)
	register struct buf *bp;
{
	register int x, rw;

	T;
	ibiomap= ubasetup(ibdinfo->ui_ubanum, bp, 0);
	xprintf("ibstrategy (%o): ibiomap= %o\n", ibstrategy, ibiomap);
	rw= bp->b_flags&B_READ? RD:(ib.op==RQC? OUT:WT);
	if((x=xfer(rw, ibiomap+ibodda, bp->b_bcount-iboddc, 0))<0){
		T;
		ib.ans= x;
		ibdone(bp);
	}	
}

static
ibdone(bp)
	register struct buf *bp;
{
	register int x;

	T;
	iodone(bp);
	x= ubarelse(ibdinfo->ui_ubanum, &ibiomap);
	xprintf("ibdone (%o): ubarelse= %d\n",ibdone,x);
}


static
gts(newadm)
	char newadm;
{
	register int x;

	T;
	Try(unhold());
	if(ib.cstate==STANDBY)
		return (ib.internal[ADM]==newadm)? OK : ENOIBDEV;
	T;
	if(ib.cstate==IDLE) return ENOTCAC;
	Try(lun(newadm));
	out(csr, (ib.csr &= ~ATN));
	ib.cstate= STANDBY;
	return OK;
}

static
tcs()
{
	if(ib.cstate==INCHARGE) return OK;
	T;
	if(ib.cstate==IDLE) return ENOTCAC;
	out(csr, (ib.csr |= ATN));
	ib.cstate= INCHARGE;
	return unhold();
}

static
unhold()
{
	register int x;

	if(ib.hidden[AUXA]&(HLDE|HLDA)){
		T;
		ib.hidden[AUXA]= _AUXA;
		Try(ldaux(FH));
		return xfer(WTIR, &ib.hidden[AUXA], 1, AUX);
	}
	return OK;
}

xfer(rw,bp,n,fr)
{	/* fr is internal reg addr  */
	register int i, x;
	int ibtimer();

	if(n<=0) return OK;
	if(rw&SEL){
		T;
		out(bcr, (-n<<8) | fr & 7);
		bp += ibmap - (int)&ib;
		out(bar, bp);
		out(csr, ib.csr & (REN|SRQ_IE|EOI|ATN|CIC) | (bp>>12) & XBA | rw);
		for(i=ITIMO; !((x=in(csr))&DONE); )
			if(--i<=0) return EITIMO;
		if(x&NEX) return EINEXM;
		if(in(bcr)&0177400) return EIDMACNT;
		out(csr, ib.csr & (REN|SRQ_IE|ATN|CIC|IE));
		return n;
	}
	ib.internal[IMR2]= rw&OUT? DMAO_ENAB:DMAI_ENAB;
	Try(xfer(WTIR,&ib.internal[IMR2],1,IMR2));
	T;
	out(bcr, ib.bcr= -n);
	out(bar, ib.bar= bp);
	out(ccf, ib.ccf);
	out(csr, (ib.csr= ib.csr & (REN|SRQ_IE|TCS|ATN|CIC|ECC) | (bp>>12) & XBA | IE | rw));
	ib.ans= 0;
	if(ib.timo) timeout(ibtimer,0,ib.timo*hz);
	return OK;
}

static
ldaux(a)
{
	ib.internal[AUX]= a;
	return xfer(WTIR, &ib.internal[AUX], 1, AUX);
}

ibtimer(id)
{
	int sps;

	Exclude;
	printf("timer (%o)\n",ibtimer);
	T;
	ib.ans= ibstop();
	Unexclude;
}

ibintr()
{
	register int x, i;
	register short unsigned y;

	xprintf("intr (%o)",ibintr);
	ib.rdcsr= in(csr);
	ib.rdbcr= in(bcr);
	ib.rdbar= in(bar);
	xprintf("\trdbcr,rdbar,rdcsr= %o %o %o\n",ib.rdbcr,ib.rdbar,ib.rdcsr);
	T;
	if((ib.rdcsr&SRQ) && (ib.csr&SRQ_IE)){
		xprintf("srq intr\n");
		T;
		out(csr, (ib.csr &= ~SRQ_IE) & ~GO);
		if(ib.op==WSRQ){
			ib.op= 0;
			ib.ans= OK;
			untimeout(ibtimer,0);
			wakeup(&ibbuf);
		}
		else if(ib.owner) psignal(ib.owner,SIGSRQ);
	}
	if(ib.rdcsr&INT){
		T;
		xprintf("int intr");
		out(csr, ib.csr & ~(DMA_ENAB|GO));
		ib.rdcsr= in(csr);
		ib.rdbcr= in(bcr);
		ib.rdbar= in(bar);
		xprintf("\trdbcr,rdbar,rdcsr,oldbcr= %o %o %o %o\n",ib.rdbcr,ib.rdbar,ib.rdcsr,x);
		if((x=xfer(RDIR, &ib.rdinternal[ISR1], 5, ISR1))<0) goto quit;
		if(ib.rdinternal[ISR1]&ERR_IE) ib.ans= ENOIBDEV;
		if(ib.internal[IMR1]&END_IE){
			T;
			ib.internal[IMR1] &= ~END_IE;
			if((x=xfer(WTIR,&ib.internal[IMR1],1,IMR1))<0) goto quit;
		}	
	}
	if((ib.rdcsr&DONE) && (ib.csr&GO)){
		T;
		ib.csr &= ~GO;
		if(ib.timo) untimeout(ibtimer,0);
		if(ib.rdcsr&NEX) ib.ans= ENEXMEM;
		x = y = ib.rdbcr - ib.bcr;
		if((ib.rdbar - ib.bar) != y){
			x= ECNTRS;
			printf("\trdbcr,rdbar,rdcsr= %o %o %o\n",ib.rdbcr,ib.rdbar,ib.rdcsr);
			printf("\tbcr,bar,csr= %o %o %o\n",ib.bcr,ib.bar,ib.csr);
		}
		ibbuf.b_resid= -ib.rdbcr;
		if(ib.ans==0)
quit:			
			ib.ans= x;
		ib.op= 0;
		T;
		ibdone(&ibbuf);
	}
}

static
wait100us()
{
	register int i;

	for(i=100; i-->0; ) ;
}

#ifdef TRACE
tracer(a)
{
	register int s, *p;

	s= spl7();
	p= &a;
	p--;
	xprintf("\tT %o\n", *p);
	ib.trbuf[ib.trin++]= *p;	/* save caller pc */
	if(ib.trin>=TRSZ) ib.trin= 0;
	splx(s);
}
#endif

untimeout(fn,arg)
	register int (*fn)();
	caddr_t arg;
{
	register struct callout *p, *q;
	int s;

	s= spl7();
	for(p= &calltodo; q=p->c_next; p=q)
		if(q->c_func==fn && q->c_arg==arg){
			p->c_next= q->c_next;
			q->c_next= callfree;
			callfree= q;
			if(p=p->c_next)
				p->c_time += q->c_time;
			break;
		}
	splx(s);
}

