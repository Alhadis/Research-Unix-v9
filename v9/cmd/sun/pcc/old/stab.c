/*
 * Symbolic debugging info interface.
 *
 * Here we generate pseudo-ops that cause the assembler to put
 * symbolic debugging information into the object file.
 */


#ifndef lint
static	char sccsid[] = "@(#)stab.c 1.1 86/02/03 SMI"; /* from UCB X.X XX/XX/XX */
#endif
#include "cpass1.h"


#include <sys/types.h>
#include <a.out.h>
#include "stab.h"

#define private static
#define and &&
#define or ||
#define not !
#define div /
#define mod %
#define nil 0

#define bytes(bits) ((bits) / SZCHAR)
#define bsize(p) bytes(dimtab[p->sizoff])	/* size in bytes of a symbol */

#define NILINDEX -1
#define FORWARD -2
private int nfield;
#define NSTABFIELDS 7

typedef int Boolean;
#define false 0
#define true 1

typedef enum{ xname, lname, value } Valueform;

extern int ddebug;
extern int gdebug;
extern int optimize;
extern char *malloc();
extern char *strcpy();

int stabLCSYM;

/* the values of the stab fields we are currently working on */
private unsigned char typeval;
private short descval;
private Valueform vform;
private unsigned valueval;

/*
 * Flag for producing either sdb or dbx symbol information.
 */
int oldway = false;


/*
 * Since type names are lost in the travels and because C has
 * structural type equivalence we keep a table of type words that
 * we've already seen.  The first time we see a type, it is assigned
 * (inline) a number and future references just list that number.
 * Structures, unions, enums, and arrays must be handled carefully
 * since not all the necessary information is in the type word.
 */

typedef struct Typeid *Typeid;

struct Typeid {
    TWORD tword;
    int tarray;
    int tstruct;
    int tstrtag;
    int f_index;			/* File index */
    int tnum;				/* Type number */
    Typeid chain;
};

Typeid entertype();

#define TABLESIZE 2003
#define FTBL_SIZE 11

struct	file	{
	char	*ftitle;		/* Copy of ftitle name */
	int	f_index;		/* File name index */
	int	tnum;			/* Next type num within this file */
};

struct	file	dummy;
struct	file	*file_stack[FTBL_SIZE];	/* Hashed file table */
struct	file	**stkp = file_stack;	/* Stack pointer into file_stack[] */
struct	file	*curfile = &dummy;	/* Current file as we know it */
int	f_index;			/* Next file index */

private int t_int, t_char;
private Typeid typetable[TABLESIZE];

private Boolean firsttime = true;

/*
 * Generate debugging info for a parameter.
 * The offset isn't known when it is first entered into the symbol table
 * since the types are read later.
 */

fixarg(p)
struct symtab *p;
{
    int offset;
    if (oldway) {
	old_fixarg(p);
    } else if (gdebug) {
	printf("\t.stabs\t\"%s:p", p->sname);
	gentype(p);
	offset = bytes(argoff);
#ifdef mc68000
	if ( (p->stype==STRTY || p->stype==UNIONTY) && dimtab[p->sizoff]<=2*SZCHAR )
	    /* offset of itty bitty structs off by two */
	    offset += 2;
#endif
	printf("\",0x%x,0,%d,%d\n", N_PSYM, bsize(p), offset);
    }
}

/*
 * Determine if the given symbol is a global array with dimension 0,
 * which only makes sense if it's dimension is to be given later.
 * We therefore currently do not generate symbol information for
 * such entries.
 */

#define isglobal(class) ( \
    class == EXTDEF or class == EXTERN or class == STATIC \
)

private Boolean zero_length_array(p)
register struct symtab *p;
{
    Boolean b;
    int t;

    if (not isglobal(p->sclass)) {
	b = false;
    } else {
	t = p->stype;
	if (ISFTN(t)) {
	    t = DECREF(t);
	}
	b = (Boolean) (ISARY(t) and dimtab[p->dimoff] == 0);
    }
    return b;
}

/* 
 * for dbx, the collating sequence of registers includes two
 * pseudo-registers following a0-a7. These are of no interest to
 * the rest of the compiler, so the remainder of the register
 * sequence (i.e., fp0-fp7) must be adjusted accordingly.
 */
#define stabregno(r) ((r) >= FP0 ? (r)+2 : (r))

/*
 * Generate debugging info for a given symbol.
 */

outstab(sym)
struct symtab *sym;
{
    register struct symtab *p;
    char *classname;
    int offset;
    Boolean ignore;

    if (oldway) {
	old_outstab(sym);
    } else if (gdebug and not zero_length_array(sym)) {
	if (firsttime) {
	    firsttime = false;
	    inittypes();
	}
	ignore = false;
	p = sym;
	offset = bytes(p->offset);
	switch (p->sclass) {
	case REGISTER:
	    classname = "r";
	    offset = stabregno(p->offset);
	    break;

	/*
	 * Locals are the default class.
	 */
	case AUTO:
	    classname = "";
	    break;

	case STATIC:
	    if (ISFTN(p->stype)) {
		ignore = true;
	    } else if (p->slevel <= 1) {
		classname = "S";
	    } else {
		classname = "V";
	    }
	    break;

	case EXTDEF:
	case EXTERN:
	    if (ISFTN(p->stype)) {
		ignore = true;
	    } else {
		classname = "G";
	    }
	    break;

	case TYPEDEF:
	    classname = "t";
	    break;

	case PARAM:
	case MOS:
	case MOU:
	case MOE:
	    ignore = true;
	    break;

	case ENAME:
	case UNAME:
	case STNAME:
	    entertype(p->stype, NILINDEX, FORWARD, dimtab[p->sizoff + 3]);
	    ignore = true;
	    break;

	default:
	    if ((p->sclass&FIELD) == 0) {
		printf("||| no info for %s (%d) \n", p->sname, p->sclass);
	    }
	    ignore = true;
	    break;
	}
	if ( BTYPE(p->stype) == TERROR ) {
	    /* don't try to output symbols entered in error */
	    ignore = true;
	}
	if (not ignore) {
	    printf("\t.stabs\t\"%s:%s", p->sname, classname);
	    setinfo(p);
	    gentype(p);
	    geninfo(p);
	}
    }
}
/*
 * Look for the given type word in the type table.
 */

private Typeid typelookup(type, arrindex, strindex, strtag)
TWORD type;
int arrindex;
int strindex;
int strtag;
{
    register TWORD tword;
    register int i1, i2;
    Typeid t;

    for (t = typetable[type mod TABLESIZE]; t != nil; t = t->chain) {
	if (t->tword == type and
	  strindex == t->tstruct and strtag == t->tstrtag) {
	    if (arrindex == NILINDEX) {
		break;
	    } else {
		tword = type;
		i1 = arrindex;
		i2 = t->tarray;
		while (ISARY(tword) and dimtab[i1] == dimtab[i2]) {
		    ++i1;
		    ++i2;
		    tword >>= TSHIFT;
		}
		if (!ISARY(tword)) {
		    break;
		}
	    }
	}
    }
    return t;
}

/*
 * Enter a type word and associated symtab indices into the type table.
 */

private Typeid entertype(type, arrindex, strindex, strtag)
TWORD type;
int arrindex;
int strindex;
int strtag;
{
    register Typeid t;
    register int i;

    t = (Typeid) malloc(sizeof(struct Typeid));
    t->tword = type;
    t->tarray = arrindex;
    t->tstruct = strindex;
    t->tstrtag = strtag;
    t->f_index = curfile->f_index;
    t->tnum = curfile->tnum++;
    i = type mod TABLESIZE;
    t->chain = typetable[i];
    typetable[i] = t;
    return t;
}

/*
 * Change the information associated with a type table entry.
 * Since I'm lazy this just creates a new entry with the number
 * as the old one.
 */

private reentertype(typeid, type, arrindex, strindex, strtag)
Typeid typeid;
TWORD type;
int arrindex;
int strindex;
int strtag;
{
    register Typeid t;
    register int i;

    t = (Typeid) malloc(sizeof(struct Typeid));
    t->tword = type;
    t->tarray = arrindex;
    t->tstruct = strindex;
    t->tstrtag = strtag;
    t->f_index = typeid->f_index;
    t->tnum = typeid->tnum;
    i = type mod TABLESIZE;
    t->chain = typetable[i];
    typetable[i] = t;
}

/*
 * Initialize type table with predefined types.
 */

#define builtintype(type) entertype(type, NILINDEX, NILINDEX, NILINDEX)->tnum

private inittypes()
{
    int t;

    t_int = builtintype(INT);
    t_char = builtintype(CHAR);
    maketype("int", t_int, t_int, 0x80000000L, 0x7fffffffL);
    maketype("char", t_char, t_char, 0L, 127L);
    maketype("long", builtintype(LONG), t_int, 0x80000000L, 0x7fffffffL);
    maketype("short", builtintype(SHORT), t_int, 0xffff8000L, 0x7fffL);
    maketype("unsigned char", builtintype(UCHAR), t_int, 0L, 255L);
    maketype("unsigned short", builtintype(USHORT), t_int, 0L, 0xffffL);
    maketype("unsigned long", builtintype(ULONG), t_int, 0L, 0xffffffffL);
    maketype("unsigned int", builtintype(UNSIGNED), t_int, 0L, 0xffffffffL);
    maketype("float", builtintype(FLOAT), t_int, 4L, 0L);
    maketype("double", builtintype(DOUBLE), t_int, 8L, 0L);
    t = builtintype(UNDEF);
    printf("\t.stabs\t\"void:t(%d,%d)=(%d,%d)", curfile->f_index, t, 
      curfile->f_index, t);
    geninfo(nil);
    t = builtintype(FARG);
    printf("\t.stabs\t\"???:t(%d,%d)=(%d,%d)", curfile->f_index, t, 
      curfile->f_index, t_int);
    geninfo(nil);
}

/*
 * Generate info for a new range type.
 */

private maketype(name, tnum, eqtnum, lower, upper)
char *name;
int tnum, eqtnum;
long lower, upper;
{
    printf("\t.stabs\t\"%s:t(%d,%d)=r(%d,%d);%d;%d;", 
	name, curfile->f_index, tnum, curfile->f_index, eqtnum, lower, upper);
    geninfo(nil);
}

/*
 * Generate debugging information for the given type of the given symbol.
 */

private gentype(sym)
struct symtab *sym;
{
    register struct symtab *p;
    register TWORD t;
    register TWORD basictype;
    register Typeid typeid;
    int i, arrindex, strindex, strtag;
    char basicchar;

    p = sym;
    t = p->stype;
    if (ISFTN(t)) {
	t = DECREF(t);
    }
    basictype = BTYPE(t);
    if (ISARY(t)) {
	arrindex = p->dimoff;
    } else {
	arrindex = NILINDEX;
    }
    if (basictype == STRTY or basictype == UNIONTY or basictype == ENUMTY) {
	strindex = dimtab[p->sizoff + 1];
	if (strindex == -1) {
	    strindex = FORWARD;
	    strtag = dimtab[p->sizoff + 3];
	} else {
	    strtag = NILINDEX;
	}
    } else {
	strindex = NILINDEX;
	strtag = NILINDEX;
    }
    i = arrindex;
    typeid = typelookup(t, arrindex, strindex, strtag); 
    while (t != basictype and typeid == nil) {
	typeid = entertype(t, i, strindex, strtag);
	printf("(%d,%d)=", typeid->f_index, typeid->tnum);
	switch (t&TMASK) {
	case PTR:
	    printf("*");
	    break;

	case FTN:
	    printf("f");
	    break;

	case ARY:
	    printf("ar(0,%d);0;%d;", t_int, dimtab[i++] - 1);
	    break;
	}
	t = DECREF(t);
	if (t == basictype) {
	    typeid = typelookup(t, NILINDEX, strindex, strtag);
	} else {
	    typeid = typelookup(t, i, strindex, strtag);
	}
    }
    if (typeid == nil) {
	if (strindex == FORWARD) {
	    typeid = typelookup(t, NILINDEX, FORWARD, dimtab[p->sizoff + 3]);
	    if (typeid == nil) {
		cerror("unbelievable forward reference");
	    }
	    printf("(%d,%d)", typeid->f_index, typeid->tnum);
	} else {
	    genstruct(t, NILINDEX, NILINDEX, strindex, p->sname, bsize(p));
	}
    } else {
	printf("(%d,%d)", typeid->f_index, typeid->tnum);
	p = STP(strtag);
	if (typeid->tstruct == FORWARD and (p->sflags & SPRFORWARD) == 0) {
	    switch (basictype) {
	    case STRTY:
		basicchar = 's';
		break;
	    case UNIONTY:
		basicchar = 'u';
		break;
	    case ENUMTY:
		basicchar = 'e';
		break;
	    default:
		cerror("Bad basic type, %d, in gentype", basictype);
		basicchar = 's';
	    }
	    printf("=x%c%s:", basicchar, STP(strtag)->sname);
	    p->sflags |= SPRFORWARD;
	}
    }
}

/*
 * Generate type information for structures, unions, and enumerations.
 */

private genstruct(t, fileid, structid, index, name, size)
TWORD t;
int fileid;
int structid;
int index;
char *name;
int size;
{
    Typeid typeid;
    register int i;
    register struct symtab *field;
    int id;
    int fid;

    if (structid == NILINDEX) {
	typeid = entertype(t, NILINDEX, index, NILINDEX);
	id = typeid->tnum;
	fid = typeid->f_index;
    } else {
	id = structid;
	fid = fileid;
    }
    switch (t) {
    case STRTY:
    case UNIONTY:
	printf("(%d,%d)=%c%d", fid, id, t == STRTY ? 's' : 'u', size);
	i = index;
	while (dimtab[i] != -1) {
	    if (nfield > NSTABFIELDS && dimtab[i+1] != -1){
		continue_stab();
	    }
	    field = STP(dimtab[i]);
	    printf("%s:", field->sname);
	    gentype(field);
	    if (field->sclass > FIELD) {
		printf(",%d,%d;", field->offset, field->sclass - FIELD);
	    } else {
		printf(",%d,%d;", field->offset,
		    tsize(field->stype, field->dimoff, field->sizoff));
	    }
	    ++i;
	    ++nfield;
	}
	putchar(';');
	break;

    case ENUMTY:
	printf("(%d,%d)=e", fid, id);
	i = index;
	while (dimtab[i] != -1 ) {
	    if (nfield > NSTABFIELDS && dimtab[i+1] != -1){
		continue_stab();
	    }
	    field = STP(dimtab[i]);
	    printf("%s:%d,", field->sname, field->offset);
	    ++i;
	    ++nfield;
	}
	putchar(';');
	break;

    default:
	cerror("couldn't find basic type %d for %s\n", t, name);
	break;
    }
}

/*
 * Generate offset and size info.
 */

private setinfo(p)
register struct symtab *p;
{
    int stabtype;

    if (p == nil) {
	typeval = N_LSYM;
	descval = 0;
	vform = value;
	valueval = 0;
    } else {
	descval = bsize(p);
	switch (p->sclass) {
	    case EXTERN:
	    case EXTDEF:
		if (ISFTN(p->stype)) {
		    typeval = N_FUN;
		    vform = xname;
		    valueval = (unsigned)p->sname;
		} else {
		    typeval = N_GSYM;
		    vform = value;
		    valueval = 0;
		}
		break;

	    case STATIC:
		typeval = stabLCSYM ? N_LCSYM : N_STSYM;
		vform = xname;
		valueval = (unsigned)p->sname;
		if (ISFTN(p->stype)) {
		    typeval = N_FUN;
		} else if (p->slevel > 1) {
		    vform = lname;
		    valueval = p->offset;
		}
		break;

	    case REGISTER:
		typeval = N_RSYM;
		vform = value;
		valueval = stabregno(p->offset);
		break;

	    case PARAM:
		typeval = N_PSYM;
		vform = value;
		valueval = bytes(argoff);
		break;

	    default:
		typeval = N_LSYM;
		vform = value;
		valueval = bytes(p->offset);
		break;
	}
    }
}

private geninfo(p)
struct symtab *p;
{
    setinfo( p );
    printf("\",0x%x,0,%d," , typeval, descval );
    switch (vform){
    case value:	printf("%d\n", valueval); break;
    case xname:	printf("_%s\n", valueval); break;
    case lname:	printf("L%d\n", valueval); break;
    }
    nfield = 0;
}

/* stab strings for structures can be very big. Sometimes we would
 * like to continue them on a second line. For this case, we must
 * write out a back-slash at the end of the quoted string, then
 * put out the rest of the stab entry information, followed by a new
 * .stabs line. The information we put must be setup in globals by
 * a previous call to setinfo.
 */

private continue_stab()
{
    printf("\\\\\",0x%x,0,%d," , typeval, descval );
    switch (vform){
    case value:	printf("%d\n", valueval); break;
    case xname:	printf("_%s\n", valueval); break;
    case lname:	printf("L%d\n", valueval); break;
    }
    printf("\t.stabs\t\"");
    nfield = 0;
}

/*
 * Generate information for a newly-defined structure.
 */

outstruct(szindex, paramindex)
int szindex, paramindex;
{
    register Typeid typeid;
    register struct symtab *p;
    register int i, strindex;

    if (oldway) {
	/* do nothing */;
    } else if (gdebug) {
	i = dimtab[szindex + 3];
	p = STP(i);
	if (i != -1 ) {
	    strindex = dimtab[p->sizoff + 1];
	    typeid = typelookup(p->stype, NILINDEX, FORWARD, i);
	    if (typeid != nil) {
		if (typeid->f_index == curfile->f_index) {
		    reentertype(typeid, p->stype, NILINDEX, strindex, NILINDEX);
		} else {
		    typeid = entertype(p->stype, NILINDEX, FORWARD, NILINDEX);
		}
	        printf("\t.stabs\t\"%s:T", p->sname);
	        setinfo(p);
	        genstruct(p->stype, typeid->f_index, typeid->tnum, strindex, 
		  p->sname, bsize(p));
	        geninfo(p);
	    } else {
		cerror("Couldn't find struct %s", p->sname);
	    }
	}
    }
}

pstab(name, type)
char *name;
int type;
{
    register int i;
    register char c;

    if (!gdebug) {
	return;
    } else if (oldway) {
	old_pstab(name, type);
	return;
    }
    /* locctr(PROG);  /* .stabs must appear in .text for c2 */
#ifdef ASSTRINGS
    if ( name[0] == '\0')
	printf("\t.stabn\t");
    else
#ifndef FLEXNAMES
	printf("\t.stabs\t\"%.8s\",", name);
#else
	printf("\t.stabs\t\"%s\",", name);
#endif
#else
    printf("    .stab   ");
    for(i=0; i<8; i++) 
	if (c = name[i]) printf("'%c,", c);
	else printf("0,");
#endif
    printf("0%o,", type);
}

#ifdef STABDOT
pstabdot(type, value)
int type;
int value;
{
    if ( ! gdebug) {
	return;
    } else if (oldway) {
	old_pstabdot(type, value);
	return;
    }
    /* locctr(PROG);  /* .stabs must appear in .text for c2 */
    printf("\t.stabd\t");
    printf("0%o,0,0%o\n",type, value);
}
#endif

extern char NULLNAME[8];
extern int  labelno;
extern int  fdefflag;

psline(lineno)
    int lineno;
{
    static int nrecur = 0;
    static int lastlineno;
    register char *cp, *cq;
    register int i;
    int tmp;
    
    if (nrecur) {
	/*
	 * N_SLINE and N_SOL .stab lines must go into .text space.
	 * We use locctr() to get there and back, but locctr()
	 * turns around and calls US, so...
	 */
	return;
    }
    if (!gdebug) {
	return;
    } else if (oldway) {
	old_psline(lineno);
	return;
    }

    cq = ititle;
    cp = ftitle;

    while ( *cq ) if ( *cp++ != *cq++ ) goto neq;
    if ( *cp == '\0' ) goto eq;
    
neq:    for (i=0; i<100; i++)
	ititle[i] = '\0';
    cp = ftitle;
    cq = ititle;
    while ( *cp )  
	*cq++ = *cp++;
    *cq = '\0';
    *--cq = '\0';
    nrecur++; tmp = locctr(PROG);
#ifndef FLEXNAMES
    for ( cp = ititle+1; *(cp-1); cp += 8 ) {
	pstab(cp, N_SOL);
	if (gdebug) printf("0,0,LL%d\n", labelno);
    }
#else
    pstab(ititle+1, N_SOL);
    if (gdebug) printf("0,0,LL%d\n", labelno);
#endif
    *cq = '"';
    printf("LL%d:\n", labelno++);
    locctr(tmp); nrecur--;

eq: if (lineno == lastlineno) return;
    lastlineno = lineno;

    nrecur++; tmp = locctr(PROG);
    if ((tmp == PROG) and not optimize) {
#ifdef STABDOT
	pstabdot(N_SLINE, lineno);
#else
	pstab(NULLNAME, N_SLINE);
	printf("0,%d,LL%d\n", lineno, labelno);
	printf("LL%d:\n", labelno++);
#endif
    }
    locctr(tmp); nrecur--;
}
    
plcstab(level)
int level;
{
    if (!gdebug) {
	return;
    } else if (oldway) {
	old_plcstab(level);
	return;
    }
#ifdef STABDOT
    pstabdot(N_LBRAC, level);
#else
    pstab(NULLNAME, N_LBRAC);
    printf("0,%d,LL%d\n", level, labelno);
    printf("LL%d:\n", labelno++);
#endif
}
    
prcstab(level)
int level;
{
    if (!gdebug) {
	return;
    } else if (oldway) {
	old_prcstab(level);
	return;
    }
#ifdef STABDOT
    pstabdot(N_RBRAC, level);
#else
    pstab(NULLNAME, N_RBRAC);
    printf("0,%d,LL%d\n", level, labelno);
    printf("LL%d:\n", labelno++);
#endif
}
    
pfstab(sname) 
char *sname;
{
    register struct symtab *p;

    if (gdebug) {
	if (oldway) {
	    old_pfstab(sname);
	} else {
	    p = STP(lookup(sname, 0));
	    printf("\t.stabs\t\"%s:", p->sname); 
	    putchar((p->sclass == STATIC) ? 'f' : 'F'); setinfo(p);
	    gentype(p);
	    geninfo(p);
	}
    }
}

/*
 * Have the beginning of an include file.
 * Change our notion of the current file.
 */
stab_startheader()
{
	char	buf[256];

	if (!gdebug) {
		return;
	}
	if (firsttime) {
		firsttime = false;
		inittypes();
	}
	strcpy(buf, ftitle + 1);
	buf[strlen(buf) - 1] = '\0';
	insert_filename(buf);
	pstab(buf, N_BINCL);
	printf("0,0,0\n");
}

/*
 * Have the end of an include file.
 */
stab_endheader()
{
	char	buf[256];

	if (!gdebug) {
		return;
	}
	strcpy(buf, ftitle + 1);
	buf[strlen(buf) - 1] = '\0';
	pstab("", N_EINCL);
	printf("0,0,0\n");
	curfile = *--stkp;
}

/*
 * Old way of doing things.
 */

private old_fixarg(p)
struct symtab *p; {
	if (gdebug) {
		old_pstab(p->sname, N_PSYM);
		if (gdebug) printf("0,%d,%d\n", p->stype, argoff/SZCHAR);
		old_poffs(p);
	}
}

private old_outstab(p)
struct symtab *p; {
	register TWORD ptype;
	register char *pname;
	register char pclass;
	register int poffset;

	if (!gdebug) return;

	ptype = p->stype;
	pname = p->sname;
	pclass = p->sclass;
	poffset = p->offset;

	if (ISFTN(ptype)) {
		return;
	}
	
	switch (pclass) {
	
	case AUTO:
		old_pstab(pname, N_LSYM);
		printf("0,%d,%d\n", ptype, (-poffset)/SZCHAR);
		old_poffs(p);
		return;
	
	case EXTDEF:
	case EXTERN:
		old_pstab(pname, N_GSYM);
		printf("0,%d,0\n", ptype);
		old_poffs(p);
		return;
			
	case STATIC:
#ifdef LCOMM
		/* stabLCSYM is 1 during nidcl so we can get stab type right */
		old_pstab(pname, stabLCSYM ? N_LCSYM : N_STSYM);
#else
		old_pstab(pname, N_STSYM);
#endif
		if (p->slevel > 1) {
			printf("0,%d,L%d\n", ptype, poffset);
		} else {
			printf("0,%d,%s\n", ptype, exname(pname));
		}
		old_poffs(p);
		return;
	
	case REGISTER:
		old_pstab(pname, N_RSYM);
		printf("0,%d,%d\n", ptype, poffset);
		old_poffs(p);
		return;
	
	case MOS:
	case MOU:
		old_pstab(pname, N_SSYM);
		printf("0,%d,%d\n", ptype, poffset/SZCHAR);
		old_poffs(p);
		return;
	
	case PARAM:
		/* parameter stab entries are processed in dclargs() */
		return;
	
	default:
#ifndef FLEXNAMES
		if (ddebug) printf("	No .stab for %.8s\n", pname);
#else
		if (ddebug) printf("	No .stab for %s\n", pname);
#endif
		
	}
}

private old_pstab(name, type)
char *name;
int type; {
	register int i;
	register char c;
	if (!gdebug) return;
	/* locctr(PROG);  /* .stabs must appear in .text for c2 */
#ifdef ASSTRINGS
	if ( name[0] == '\0')
		printf("\t.stabn\t");
	else
#ifndef FLEXNAMES
		printf("\t.stabs\t\"%.8s\", ", name);
#else
		printf("\t.stabs\t\"%s\", ", name);
#endif
#else
	printf("	.stab	");
	for(i=0; i<8; i++) 
		if (c = name[i]) printf("'%c,", c);
		else printf("0,");
#endif
	printf("0%o,", type);
}

#ifdef STABDOT
private old_pstabdot(type, value)
	int	type;
	int	value;
{
	if ( ! gdebug) return;
	/* locctr(PROG);  /* .stabs must appear in .text for c2 */
	printf("\t.stabd\t");
	printf("0%o,0,0%o\n",type, value);
}
#endif

private old_poffs(p)
register struct symtab *p; {
	int s;
	if (!gdebug) return;
	if ((s = dimtab[p->sizoff]/SZCHAR) > 1) {
		old_pstab(p->sname, N_LENG);
		printf("1,0,%d\n", s);
	}
}

private old_psline(lineno) 
	int lineno;
{
	static int lastlineno;
	register char *cp, *cq;
	register int i;
	
	if (!gdebug) return;

	cq = ititle;
	cp = ftitle;

	while ( *cq ) if ( *cp++ != *cq++ ) goto neq;
	if ( *cp == '\0' ) goto eq;
	
neq:	for (i=0; i<100; i++)
		ititle[i] = '\0';
	cp = ftitle;
	cq = ititle;
	while ( *cp )  
		*cq++ = *cp++;
	*cq = '\0';
	*--cq = '\0';
#ifndef FLEXNAMES
	for ( cp = ititle+1; *(cp-1); cp += 8 ) {
		old_pstab(cp, N_SOL);
		if (gdebug) printf("0,0,LL%d\n", labelno);
		}
#else
	old_pstab(ititle+1, N_SOL);
	if (gdebug) printf("0,0,LL%d\n", labelno);
#endif
	*cq = '"';
	printf("LL%d:\n", labelno++);

eq:	if (lineno == lastlineno) return;
	lastlineno = lineno;

	if (fdefflag) {
#ifdef STABDOT
		old_pstabdot(N_SLINE, lineno);
#else
		old_pstab(NULLNAME, N_SLINE);
		printf("0,%d,LL%d\n", lineno, labelno);
		printf("LL%d:\n", labelno++);
#endif
		}
	}
	
private old_plcstab(level) {
	if (!gdebug) return;
#ifdef STABDOT
	old_pstabdot(N_LBRAC, level);
#else
	old_pstab(NULLNAME, N_LBRAC);
	printf("0,%d,LL%d\n", level, labelno);
	printf("LL%d:\n", labelno++);
#endif
	}
	
private old_prcstab(level) {
	if (!gdebug) return;
#ifdef STABDOT
	pstabdot(N_RBRAC, level);
#else
	pstab(NULLNAME, N_RBRAC);
	printf("0,%d,LL%d\n", level, labelno);
	printf("LL%d:\n", labelno++);
#endif
	}
	
private old_pfstab(sname) 
char *sname; {
	if (!gdebug) return;
	pstab(sname, N_FUN);
#ifndef FLEXNAMES
	printf("0,%d,_%.7s\n", lineno, sname);
#else
	printf("0,%d,_%s\n", lineno, sname);
#endif
}

/*
 * Have entered a new file.
 * Allocate a node for it.
 */
insert_filename(file)
char	*file;
{
	*stkp++ = curfile;
	curfile = (struct file *) malloc(sizeof(struct file));
	curfile->ftitle = strcpy(malloc(strlen(file) + 1), file);
	curfile->f_index = f_index++;
	curfile->tnum = 1;
}
