/*
 * adb - routines to read a.out+core at startup
 * stuff in here is pretty vax dependent
 */
#include <a.out.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/param.h>
#include "defs.h"
#include "space.h"
#include "map.h"
#include "machine.h"
#include "base.h"

char	*symfil	= "a.out";
char	*corfil	= "core";

MAP	symmap[NMAP];
MAP	cormap[NMAP];

int fsym, fcor;

static ADDR datbase;
ADDR txtsize, datsize, stksize;
static ADDR entry;
static int magic;

setsym()
{
	ADDR loc;
	struct exec hdr;
	register MAP *mp;
	char *malloc();

	fsym = getfile(symfil, 1);
	mp = symmap;
	if (read(fsym, (char *)&hdr, sizeof hdr) != sizeof hdr ||
	    badmagic((int)hdr.a_magic)) {
		mp->f = mp->b = 0;
		mp->e = MAXFILE;
		mp->sp = DATASP;
		mp->flag = MPINUSE;
		mp++;
		mp->flag = 0;
		return;
	}
	magic = hdr.a_magic;
	entry = hdr.a_entry;
	loc = hdr.a_text+hdr.a_data;
	switch (magic) {

	case OMAGIC:
		txtsize = 0;
		datbase = 0;
		datsize = loc;
		mp->b = entry;
		mp->e = mp->b + loc;
		mp->f = N_TXTOFF(hdr);
		mp->sp = DATASP;
		mp->flag = MPINUSE;
		break;

	case ZMAGIC:
	case NMAGIC:
		txtsize = hdr.a_text;
		mp->b = N_PAGSIZ(hdr);
		mp->e = hdr.a_text + mp->b;
		mp->f = N_TXTOFF(hdr);
		mp->sp = INSTSP;
		mp->flag = MPINUSE;
		mp++;
		mp->b = datbase = round((WORD)(hdr.a_text + N_PAGSIZ(hdr)),
			(WORD)N_SEGSIZ(hdr));
		mp->e = datsize = datbase + hdr.a_data;
		mp->f = N_TXTOFF(hdr) + hdr.a_text;
		mp->sp = DATASP;
		mp->flag = MPINUSE;
		break;
	}
	mp++;
	mp->flag = 0;
	syminit(&hdr);
}

setcor()
{
	register MAP *mp;

	fcor = getfile(corfil,2);
	if (fcor < 0
	||  (mapimage() == 0 && mapcore() == 0)) {
		/* not a core image */
		mp = cormap;
		mp->b = 0;
		mp->e = MAXFILE;
		mp->f = 0;
		mp->sp = DATASP;
		mp->flag = MPINUSE;
		mp++;
		mp->flag = 0;
		return;
	}
}

/*
 * set up maps for a direct process image (/proc)
 */

#define	UOFF(x)	(ADDR)(&((struct user *)0)->x)

int
mapimage()
{
	register MAP *mp;

	if (trcimage() == 0)
		return (0);
	sigcode = trcunab(UOFF(u_code));
	txtsize = ctob(trcunab(UOFF(u_tsize)));
	datsize = ctob(trcunab(UOFF(u_dsize)));
	stksize = ctob(trcunab(UOFF(u_ssize)));
#if NOTDEF		/* no ID separation on VAX */
	if (magic == IMAGIC)
		datbase = 0;
	else
#endif
		datbase = txtsize;
	mp = cormap;
	if (txtsize) {
		mp->b = ctob(1);
		mp->e = mp->b + txtsize;
		mp->f = TXTBASE + mp->b;
		mp->flag = MPINUSE;
#if NOTDEF		/* no ID separation on VAX */
		if (magic == IMAGIC)
			mp->sp = INSTSP;
		else
#endif
			mp->sp = DATASP;
		mp++;
	}
	mp->b = ctob(stoc(ctos(btoc(txtsize) + 1)));
	mp->e = mp->b + datsize;
	mp->f = DATBASE + mp->b;
	mp->sp = DATASP;
	mp->flag = MPINUSE;
	mp++;
	mp->b = MAXSTOR - stksize;
	mp->e = MAXSTOR;
	mp->f = DATBASE + mp->b;
	mp->sp = DATASP;
	mp->flag = MPINUSE;
	mp++;
	mp->b = 0;
	mp->e = ctob(UPAGES);
	mp->f = UBASE;
	mp->sp = UBLKSP;
	mp->flag = MPINUSE;
	mp++;
	mp->flag = 0;
	rsnarf();
	return (1);
}

mapcore()
{
	struct user u;
	register MAP *mp;

	lseek(fcor, (off_t)0, 0);
	if (read(fcor, (char *)&u, sizeof(u)) != sizeof(u)
	||  badmagic(u.u_exdata.ux_mag))
		return (0);
	if (magic && magic != u.u_exdata.ux_mag)
		printf("%s: not from %s\n", corfil, symfil);
	magic = u.u_exdata.ux_mag;
	signo = u.u_arg[0];
	sigcode = u.u_code;
	txtsize = ctob(u.u_tsize);
	datsize = ctob(u.u_dsize);
	stksize = ctob(u.u_ssize);
	mp = cormap;
	switch (magic) {

	case OMAGIC:
		mp->b = ctob(1);
		mp->e = mp->b + txtsize + datsize;
		mp->f = ctob(UPAGES);
		mp->sp = DATASP;
		mp->flag = MPINUSE;
		mp++;
		mp->b = MAXSTOR - stksize;
		mp->e = MAXSTOR;
		mp->f = txtsize + datsize + ctob(UPAGES);
		mp->sp = DATASP;
		mp->flag = MPINUSE;
		break;

	case NMAGIC:
	case ZMAGIC:
		mp->b = ctob(stoc(ctos(u.u_tsize + 1)));
		mp->e = mp->b + datsize;
		mp->f = ctob(UPAGES);
		mp->sp = DATASP;
		mp->flag = MPINUSE;
		mp++;
		mp->b = MAXSTOR - stksize;
		mp->e = MAXSTOR;
		mp->f = datsize + ctob(UPAGES);
		mp->sp = DATASP;
		mp->flag = MPINUSE;
		break;
	}
	mp++;
	mp->b = 0;
	mp->e = ctob(UPAGES);
	mp->f = 0;
	mp->sp = UBLKSP;
	mp->flag = MPINUSE;
	mp++;
	mp->flag = 0;
	rsnarf();
	return (1);
}

badmagic(num)
int num;
{

	switch (num) {
	case OMAGIC:
	case NMAGIC:
	case ZMAGIC:
		return (0);

	default:
		return (1);
	}
}

cmdmap(itype, star)
register int star, itype;
{
	register MAP *mp;
	extern char lastc;

	if (itype & SYMF)
		mp = symmap;
	else
		mp = cormap;
	if (star)	/* UGH */
		mp++;
	if (expr(0))
		mp->b = expv; 
	if (expr(0))
		mp->e = expv; 
	if (expr(0))
		mp->f = expv; 
	mp->flag |= MPINUSE;
	if (rdc()=='?' && (itype&SYMF) == 0) {
		if (fcor)
			close(fcor);
		fcor=fsym;
		corfil=symfil;
	} else if (lastc == '/' && itype&SYMF) {
		if (fsym)
			close(fsym);
		fsym=fcor;
		symfil=corfil;
	} else
		reread();
}

create(f)
	char *f;
{
	register int fd;

	fd = creat(f, 0666);
	if (fd < 0)
		return (-1);
	close(fd);
	return (open(f, wtflag));
}

getfile(filnam, cnt)
	char *filnam;
{
	register int fsym;

	if (strcmp(filnam, "-") == 0)
		return (-1);
	fsym = open(filnam, wtflag);
	if (fsym < 0 && xargc > cnt) {
		if (wtflag)
			fsym = create(filnam);
		if (fsym < 0)
			printf("cannot open `%s'\n", filnam);
	}
	return (fsym);
}

setvar()
{

	var[varchk('b')] = datbase;
	var[varchk('d')] = datsize;
	var[varchk('e')] = entry;
	var[varchk('m')] = magic;
	var[varchk('s')] = stksize;
	var[varchk('t')] = txtsize;
}
