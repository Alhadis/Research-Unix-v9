/*
 * Symbolic debugging info interface.
 *
 * Here we generate pseudo-ops that cause the assembler to put
 * symbolic debugging information into the object file.
 */
#include "cpass1.h"
#include <sys/types.h>
#include <a.out.h>
#include "stab.h"

static int inastruct;
static int inafunc;
static int inbegfunc;
static int inaparam;
static char strvalfmt[] = "	.stabs	\"%s\",0x%x,0,%d,%d\n";
static char strsymfmt[] = "	.stabs	\"%s\",0x%x,0,%d,%s\n";
static char nulvalfmt[] = "	.stabn	0x%x,0,%d,%d\n";
static char nulsymfmt[] = "	.stabn	0x%x,0,%d,%s\n";
static char dotfmt[]	= "	.stabd	0x%x,0,%d\n";

extern int gdebug;
extern char *strcpy();
int stabLCSYM;

/*
 * Old flag, not used anymore.
 */
int oldway = 0;

/*
 * Generate information for a newly-defined structure.
 */
outstruct(szindex, paramindex)
int szindex, paramindex;
{
	register struct symtab *p;
	register int i;

	if (!gdebug)
		return;
 	inastruct = 1;
	p = STP(dimtab[szindex + 3]);
	printf(strvalfmt, p->sname, N_BSTR, p->stype, 0);
	outstab(STP(dimtab[szindex + 3]));
	for( i = dimtab[szindex + 1]; dimtab[i] > 0; i++)
		outstab(STP(dimtab[i]));
	printf(strvalfmt, p->sname, N_ESTR, p->stype, dimtab[p->sizoff]/SZCHAR);
	inastruct = 0;
}

psline(lineno)
    int lineno;
{
	if (gdebug && !dbfile(NULL))
		printf(dotfmt, N_SLINE, lineno);
}
    
plcstab(level)
int level;
{
	if (gdebug)
		printf(dotfmt, N_LBRAC, level);
}
    
prcstab(level)
int level;
{
	if (gdebug)
		printf(dotfmt, N_RBRAC, level);
}
    
pfstab(sname) 
char *sname;
{
    register struct symtab *p;

	if (!gdebug)
		return;
	p = STP(lookup(sname, 0));
	inafunc = 1;
	inbegfunc = 1;
/*	dbfile(exname(p->sname));*/
	locctr(PROG);
	printf(strsymfmt, p->sname, N_BFUN, lineno, exname(p->sname));
	outstab(p);
	inbegfunc = 0;
}

#define BYTOFF(p)	(((off = (p)->offset) < 0 ? -off : off)/SZCHAR)

/*
 * Generate debugging info for a parameter.
 * The offset isn't known when it is first entered into the symbol table
 * since the types are read later.
 */
fixarg(p)
struct symtab *p;
{
	if (!gdebug)
		return;
	inaparam = 1;
	outstab(p);
	inaparam = 0;
}

static char labstr[16];

static char *
maklab(val)
{
	sprintf(labstr, "L%d", val);
	return labstr;
}

# ifndef OUTREGNO
# define OUTREGNO(p) ((p)->offset)
# endif

/*
 * Generate debugging info for a given symbol.
 */
outstab(p)
register struct symtab *p; {

	register TWORD t;
	register off, i;

	if (!gdebug)
		return;
	if (ISFTN(p->stype) && !inbegfunc)
		return;
	if( p->sclass > FIELD ) {
		if (!inastruct)
			return;
		printf(strvalfmt, p->sname, N_SFLD,
			((p->sclass - FIELD)<<BTSHIFT)|p->stype, p->offset);
		return;
	}

	switch( p->sclass )
	{
	case AUTO:
		printf(strvalfmt, p->sname, N_LSYM, p->stype, BYTOFF(p));
		break;
	case REGISTER:
		if (!inafunc)
			return;
		if (inaparam) {
			p->sclass = PARAM;
			outstab(p);
			p->sclass = REGISTER;
		}
		printf(strvalfmt, p->sname, N_RSYM, p->stype, OUTREGNO(p));
		break;
	case PARAM:
		if (!inaparam)
			return;
		printf(strvalfmt, p->sname, N_PSYM, p->stype, argoff/SZCHAR);
		break;
	case EXTERN:
	case EXTDEF:
		printf(strvalfmt, p->sname, N_GSYM, p->stype, 0);
		break;
	case STATIC:
		if (ISFTN(p->stype))
			printf(strvalfmt, p->sname, N_STFUN, p->stype, 0);
		else
			printf(strsymfmt, p->sname,
			    stabLCSYM ? N_LCSYM : N_STSYM, p->stype,
			    p->slevel ? maklab(p->offset) : exname(p->sname));
		break;
	case MOS:
	case MOU:
		if (!inastruct)
			return;
		printf(strvalfmt, p->sname, N_SSYM, p->stype, BYTOFF(p));
		break;
	case MOE:
		if (!inastruct)
			return;
		printf(strvalfmt, p->sname, N_SSYM, p->stype, p->offset);
		return;
	default:
		return;
	}
	/* make another entry to describe structs, unions, enums */
	switch( BTYPE(p->stype) ) {
	case STRTY:
	case UNIONTY:
	case ENUMTY:
		printf(strvalfmt, STP(dimtab[p->sizoff+3])->sname,
		    N_TYID, 0, 0);
	}

	/* make other entries with the dimensions */
	for( t=p->stype, i=p->dimoff; t&TMASK; t = DECREF(t) ) 
	{
		if( ISARY(t) ) printf(nulvalfmt, N_DIM, 0, dimtab[i++]);
	}

}

dbfunend( lab )	/* end of a function */
{
	if (!gdebug)
		return;
	inafunc = 0;
	printf(dotfmt, N_EFUN, lineno);
}

beg_file()      /* only used in cgram.y */
{
	if (!gdebug)
		return;
        dbfile(NULL);
}

static char orgfile[100], currfile[100], prtfile[100];

static int srcfilop = N_SO;

static char *
makstr(ip)
register char *ip;
{
	register c; register char *jp = prtfile;
	do {
		if ((c = *ip++) != '"')
			*jp++ = c;
	} while (c);
	return prtfile;
}

dbfile(pname)
char *pname;
{
	int seg;
	if (!strcmp(currfile, ftitle))
		return 0;
	strcpy(currfile, ftitle);
	seg = locctr(PROG);
	if (pname == NULL)
		printf("%s:", pname = maklab(getlab()));
	printf(strsymfmt, makstr(currfile), srcfilop, lineno, pname);
	if (srcfilop == N_SO) {	/* first file */
		strcpy(orgfile, ftitle);
		srcfilop = N_SOL;
	}
	if (seg >= 0)
		locctr(seg);
	return 1;
}

ejsdb()
{
	/* called at the end of the entire file */
	if (!gdebug)
		return;
	printf("\t.text\n");
	printf("%s:", maklab(getlab()));
	printf(strsymfmt, makstr(orgfile), N_ESO, lineno, labstr);
}
