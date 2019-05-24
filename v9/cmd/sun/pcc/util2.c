#ifndef lint
static	char sccsid[] = "@(#)util2.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include "cpass2.h"
#include "ctype.h"

/* 
 *	garbage ripped out of local2.68, which was huge
 */
# ifdef FORT
int ftlab1, ftlab2;
# endif
/* a lot of the machine dependent parts of the second pass */
#ifdef FORT
#   define fltfun 0
#else
    extern int fltfun;
#endif

#ifndef ONEPASS			/* 2nd pass of (possibly) several compilers */
    char pcname[]= "pc0";	/* name passed by pc command	*/
    int pascal=0;		/* = 1 if compiling pascal	*/
#endif

# define BITMASK(n) ((1L<<(n))-1)

/* output forms for floating point constants */
typedef enum {
	F_hex, F_eformat, F_loworder, F_highorder
} Floatform;

void adrput();
static void fcon();
static void dumpfcons();

void
print_d(value)
	register int value;
{	
	char buffer[8]; 
	register char *p= buffer+8; 
	register int i= 0;

	if (value == 0) {
		putchar('0'); 
		return;
	}
	if (value < 0) {
		putchar('-'); 
		value= -value;
	}
	if (value <= 32767) { 
		register short v= value;
		while (v != 0) {
			*--p= (v % 10) + '0';
			v /= 10; 
			i++; 
		}
	} else {
		while (value != 0) {
			*--p= (value % 10) + '0';
			value /= 10; 
			i++; 
		}
	}
	for(; i>0; i--)
		putchar(*p++);
}

void
print_x(value)
	register unsigned value;
{	
	char buffer[8]; 
	register char *p= buffer+8; 
	register int i= 0;

	if (value == 0) {
		putchar('0'); 
		return;
	}
	if ((int)value < 0) {
		value = -value;
		putchar('-'); 
	}
	while (value != 0) {
		*--p = "0123456789abcdef"[value & 0xf];
		value >>= 4;
		i++; 
	}
	putchar('0'); 
	putchar('x');
	fwrite(p, 1, i, stdout);
}

void
print_str(s)
	char *s;
{
	fputs(s, stdout);
}

void
print_str_nl(s)
	char *s;
{
	puts(s);
}

void
print_str_d(s, d)
	char *s;
	int d;
{
	fputs(s, stdout);
	print_d(d);
}

void
print_str_d_nl(s, d)
	char *s;
	int d;
{
	fputs(s, stdout);
	print_d(d);
	putchar('\n');
}

void
print_str_str(s1, s2)
	char *s1, *s2;
{
	fputs(s1, stdout);
	fputs(s2, stdout);
}

void
print_str_str_nl(s1, s2)
	char *s1, *s2;
{
	fputs(s1, stdout);
	puts(s2);
}

void print_label(l)
	int l;
{
	putchar('L');
	print_d(l);
	putchar(':');
}

where(c)
{
	fprintf( stderr, "%s, line %d: ", filename, lineno );
}

lineid( l, fn )
	char *fn; 
{
	/* identify line l and file fn */
	printf( "|	line %d, file %s\n", l, fn );
}

epr(p)
	NODE *p;
{
	extern fwalk();
	fwalk(p, eprint, 0);
}

int	usedregs; /* Flag word for registers used in current routine */
int	usedfpregs;	/* Flag word for floating point registers */
int toff, maxtoff;

cntbits(i)
	register int i;
{
	register int j,ans;

	for (ans=0, j=0; i!=0 && j<16; j++) { 
		if (i&1) ans++; 
		i >>= 1 ; 
	}
	return(ans);
}

/*
 * allocate storage for register save areas
 * and generate routine epilogs.  The d/a register
 * save area is always the LAST section of an
 * activation record. A label equated to its offset
 * is used to create the activation record at runtime.
 */
eobl2()
{
	OFFSZ regsave_off;	/* a6-relative offset of areg/dreg save area */
	OFFSZ fp_regsave_off;	/* a6-relative offset of fp reg save area */
	OFFSZ spoff;		/* a6-relative offset of local vars/temps */

#	ifndef FORT
	    extern int ftlab1, ftlab2;
#	endif

#	ifdef FREETEMP
	    spoff = maxoff + maxtemp;
#	else
	    spoff = maxoff;
#	endif FREETEMP

	spoff /= SZCHAR;
	SETOFF(spoff,sizeof(long));
	/*
	 * set masks for saving/restoring registers
	 */
	usedregs &= REGVARMASK;
	usedfpregs &= FREGVARMASK;
	/*
	 * allocate storage for register save areas
	 */
	fp_regsave_off = regsave_off = spoff;
	if (use68881 && usedfpregs) {
		fp_regsave_off += (SZEXTENDED/SZCHAR)*cntbits(usedfpregs);
		regsave_off = fp_regsave_off;
	}
	if (usedregs) {
		regsave_off += (SZLONG/SZCHAR)*cntbits(usedregs);
	}
	/*
	 * generate epilogue
	 */
	printf( "LE%d:\n",ftnno);
	if (use68881 && usedfpregs) {
		/*
		 * restore floating point registers.
		 */
		if (fp_regsave_off > 32767) {
			/* long offset */
			if (use68020) {
				printf("	fmovem	a6@(-0x%x:l),#0x%x\n",
					fp_regsave_off, usedfpregs);
			} else {
				/*
				 * 68881 but no 68020; I doubt this will
				 * ever happen, but just in case...
				 */
				printf("	movl	#-0x%x,a0\n",
					fp_regsave_off);
				printf("	fmovem	a6@(0,a0:l),#0x%x",
					usedfpregs);
			}
		} else {
			/* short offset */
			printf("	fmovem	a6@(-0x%x),#0x%x\n",
				fp_regsave_off, usedfpregs);
		}
	} /* usedfpregs */
	if (usedregs) {
		/*
		 * restore data and address registers
		 */
		if (regsave_off > 32767) {
			/* long offset */
			if (use68020) {
				printf( "	moveml	a6@(-0x%x:l),#0x%x\n",
					regsave_off, usedregs );
			} else {
				printf("	movl	#-0x%x,a0\n",
					regsave_off);
				printf("	moveml	a6@(0,a0:l),#0x%x\n",
					usedregs);
			}
		} else {
			/* short offset */
			printf( "	moveml	a6@(-0x%x),#0x%x\n",
				regsave_off, usedregs );
		}
	} /* usedregs */
	printf( "	unlk	a6\n	rts\n" );
	printf( "	LF%d = %ld\n", ftnno, regsave_off );
	printf( "	LS%d = 0x%x\n", ftnno, usedregs );
	printf( "	LFF%d = %ld\n", ftnno, fp_regsave_off );
	printf( "	LSS%d = 0x%x\n", ftnno, usedfpregs );
#	ifdef FREETEMP
	    printf("	LT%d = 0x%x\n", ftnno,
		cntbits(usedregs)*(SZINT/SZCHAR));
#	endif
#	ifdef STACKPROBE
	    printf("	LP%d =	0x%x\n", ftnno, maxtoff+8 );
#	endif
	maxtoff = 0;
	usedregs = 0;
	usedfpregs = 0;
	dumpfcons();
}

struct hoptab { int opmask; char * opstring; } ioptab[] = {

	ASG PLUS, "add",
	ASG MINUS, "sub",
	ASG OR,	"or",
	ASG AND, "and",
	ASG ER,	"eor",
	ASG MUL, "mul",
	ASG DIV, "div",
	ASG MOD, "div",
	ASG LS,	"sl",
	ASG RS,	"sr",
	-1, ""    };

hopcode( f, o )
{
	/* output the appropriate string from the above table */

	register struct hoptab *q;

	for( q = ioptab;  q->opmask>=0; ++q ){
		if( q->opmask == o ){
			if( f == 'F') putchar('f');
			printf(q->opstring);
			return;
			}
		}
	cerror( "no hoptab for %s", opst[o] );
}

char *
rnames[] = {  /* keyed to register number tokens */
	"d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
	"a0", "a1", "a2", "a3", "a4", "a5", "a6", "sp",
	"fp0", "fp1", "fp2", "fp3", "fp4", "fp5", "fp6", "fp7"
	};

int rstatus[] = {
	SAREG|STAREG,	SAREG|STAREG,
	SAREG|STAREG,	SAREG|STAREG,
	SAREG|STAREG,	SAREG|STAREG,
	SAREG|STAREG,	SAREG|STAREG,

	SBREG|STBREG,	SBREG|STBREG,
	SBREG|STBREG,	SBREG|STBREG,
	SBREG|STBREG,	SBREG|STBREG,
	SBREG,		SBREG,

	SCREG|STCREG,	SCREG|STCREG,
	SCREG|STCREG,	SCREG|STCREG,
	SCREG|STCREG,	SCREG|STCREG,
	SCREG|STCREG,	SCREG|STCREG,
    };

tlen(p) NODE *p;
{
	switch(p->in.type) {
		case CHAR:
		case UCHAR:
			return(SZCHAR/SZCHAR);

		case SHORT:
		case USHORT:
			return(SZSHORT/SZCHAR);

		case DOUBLE:
			return(SZDOUBLE/SZCHAR);

		default:
			return(SZINT/SZCHAR);
		}
}


rmove( rt, rs, t )
	register int rt,rs;
{
	int fsrce, fdest;

	fsrce = iscreg(rs);
	fdest = iscreg(rt);
	if (!fsrce && !fdest) {
		/*
		 * neither source nor dest is on the coprocessor
		 */
		if ( t==DOUBLE ){
			printf("	movl	%s,%s\n	movl	%s,%s\n",
				rnames[rs], rnames[rt],
				rnames[rs+1], rnames[rt+1]);
			markused(rs+1);
			markused(rt+1);
		} else {
			/* the most common case */
			printf("	movl	%s,%s\n",
				rnames[rs], rnames[rt] );
		}
	} else if (fsrce && !fdest) {
		/*
		 * source is on coprocessor, dest isn't
		 */
		if (t == DOUBLE) {
			/* royal pain; must use memory */
			printf("	fmoved	%s,sp@-\n", rnames[rs]);
			printf("	movl	sp@+,%s\n", rnames[rt]);
			printf("	movl	sp@+,%s\n", rnames[rt+1]);
			markused(rt+1);
		} else {
			/* t == FLOAT */
			printf("	fmoves	%s,%s\n",
			    rnames[rs], rnames[rt]);
		}
	} else if (!fsrce && fdest) {
		/*
		 * source isn't on coprocessor, dest is
		 */
		if (t == DOUBLE) {
			/* royal pain; must use memory */
			printf("	movl	%s,sp@-\n", rnames[rs+1]);
			printf("	movl	%s,sp@-\n", rnames[rs]);
			printf("	fmoved	sp@+,%s\n", rnames[rt]);
			markused(rs+1);
		} else {
			/* t == FLOAT */
			printf("	fmoves	%s,%s\n",
			    rnames[rs], rnames[rt]);
		}
	} else {
		/*
		 * both regs are on the coprocessor
		 */
		printf("	fmovex	%s,%s\n", rnames[rs], rnames[rt]);
	}
	markused(rs);
	markused(rt);
}

struct respref
respref[] = {
	INCREG|INTCREG,	INCREG|INTCREG,
	INTAREG|INTBREG,INTAREG|INTBREG,
	INAREG|INBREG,	INAREG|INBREG,
	INAREG|INBREG,	SZERO,
	INAREG|INBREG,	SOREG|STARREG|STARNM|SNAME|SCON,
	INTEMP,		INTEMP,
	FORARG,		FORARG,
	INTAREG,	SOREG|SNAME|SAREG|SBREG,
	0,	0 };

#ifdef FORT
/*
 * save register variables.
 * Use labels since we don't know
 * which ones are being used until
 * the end of the function.
 */
saveregs()
{
	printf("	link	a6,#0\n");
	printf("	addl	#-LF%d,sp\n", ftnno);
	printf("	moveml	#LS%d,sp@\n",ftnno);
	if (use68881) {
		/*
		 * save floating registers used for variables
		 */
		if (use68020) {
			printf("	fmovem	#LSS%d,a6@(-LFF%d:l)\n",
				ftnno, ftnno);
		} else {
			/* 68881 without 68020; unlikely combination */
			printf("	movl	#-LFF%d,a0\n", ftnno);
			printf("	fmovem	#LSS%d,a6@(0,a0:l)\n", ftnno);
		}
	}
}
#endif FORT


/*
 * Set up temporary registers.
 * Use any left over from register
 * variable allocation for scratch.
 */

setregs()
{
	register i;
	int nextdreg;
	int nextareg;
	int nextfreg;

	extern int skybase;

	nextdreg = NEXTD(maxtreg);
	nextareg = NEXTA(maxtreg);
	nextfreg = NEXTF(maxtreg);

#	ifdef FORT
	    /* reg reserved by iropt for __skybase */
	    skybase  = SKYBASE(maxtreg);
	    if (nextdreg == D0)
		nextdreg = MAX_DVAR;
	    if (nextareg == A0)
		nextareg = MAX_AVAR;
	    if (nextfreg == FP0)
		nextfreg = MAX_FVAR;
#	endif FORT

#	ifdef FREETEMP
	    tmpoff = 0; /* we number temp regsters differently here */
#	endif

	for( i=MIN_DVAR; i<=MAX_DVAR; i++ ){
		rstatus[i] = i <= nextdreg ? SAREG|STAREG : SAREG;
#		ifdef FORT
		    if (i > nextdreg)
			markused(i);
#		endif
	}
	for( i=MIN_AVAR; i<=MAX_AVAR; i++ ){
		rstatus[i] = i <= nextareg ? SBREG|STBREG : SBREG;
#		ifdef FORT
		    if (i > nextareg)
			markused(i);
#		endif
	}
	for( i=MIN_FVAR; i<=MAX_FVAR; i++ ){
		rstatus[i] = i <= nextfreg ? SCREG|STCREG : SCREG;
#		ifdef FORT
		    if (i > nextfreg)
			markused(i);
#		endif
	}

	fregs.d = (nextdreg - D0) + 1;
	fregs.a = (nextareg - A0) + 1;
	fregs.f = (nextfreg - FP0) + 1;

	if( xdebug ){
		/* -x changes number of free regs to 2, -xx to 3, etc. */
		if( (xdebug+1) < fregs.f ) fregs.f = xdebug+1;
	}
}

shltype( o, p )
	register o;
	NODE *p; 
{
	return( o== REG || o == NAME || o == ICON || o == OREG || o == FCON
		|| ( o==UNARY MUL && shumul(p->in.left)) );
}

flshape( p ) register NODE *p; 
{
	register o = p->in.op;
	if( o==NAME || o==REG || o==ICON || o==OREG ) return( 1 );
	return( o==UNARY MUL && shumul(p->in.left)==STARNM );
}

shtemp( p ) register NODE *p; 
{
	if( p->in.op == UNARY MUL ) p = p->in.left;
	if( p->in.op == REG )
		return( !istreg( p->tn.rval ) );
	if( p->in.op == OREG && !R2TEST(p->tn.rval) )
		return( !istreg( p->tn.rval ) );
	return( p->in.op == NAME || p->in.op == ICON );
}

spsz( t, v ) TWORD t; CONSZ v; 
{

	/* is v the size to increment something of type t */

	if( !ISPTR(t) ) return( 0 );
	t = DECREF(t);

	if( ISPTR(t) ) return( v == (SZPOINT/SZCHAR) );

	switch( t ){

	case UCHAR:
	case CHAR:
		return( v == 1 );

	case SHORT:
	case USHORT:
		return( v == (SZSHORT/SZCHAR) );

	case INT:
	case UNSIGNED:
	case FLOAT:
		return( v == (SZINT/SZCHAR) );

	case DOUBLE:
		return( v == (SZDOUBLE/SZCHAR) );
		}

	return( 0 );
}

indexreg( p ) register NODE *p; 
{
	if( p->in.op == REG && p->tn.rval >= A0 && p->tn.rval <= SP ) return(1);
	return(0);
}

shumul( p ) register NODE *p; 
{
	register o;
	extern int xdebug;

	if (xdebug) {
	     printf("\nshumul:op=%d, ", p->in.op);
	     switch (optype(p->in.op)){
	    case BITYPE:
			printf( " rop=%d, rname=%s, rlval=%D", 
			    p->in.right->in.op, p->in.right->in.name,  
			    p->in.right->tn.lval);
			/* fall through */
	    default:
			printf( "lop=%d, plty=%d ", 
			    p->in.left->in.op, p->in.left->in.type );
	    case LTYPE: ; /* do nothing */
	    }
	    putchar('\n');
	}


	o = p->in.op;
	if( indexreg(p) )
		return( STARNM );

	if( o == INCR && indexreg(p->in.left) && p->in.right->in.op == ICON &&
	    p->in.right->in.name[0] == '\0' &&
	    spsz( p->in.left->in.type, p->in.right->tn.lval ) )
		return( STARREG );
	if( o == ASG MINUS  && indexreg(p->in.left) && p->in.right->in.op == ICON &&
	    p->in.right->in.name[0] == '\0' &&
	    spsz( p->in.left->in.type, p->in.right->tn.lval ) )
		return( STARREG );

	return( 0 );
}

adrcon( val ) CONSZ val; 
{
	printf( CONFMT, val );
}

/* put out a floating e-format constant */

floatimmed(p)
	NODE *p;
{
	putchar('#');
	fcon( p, F_eformat );
}

conput( p ) register NODE *p; 
{
	switch( p->in.op ){
	case FCON:
		fcon( p, F_hex );
		return;

	case ICON:
		acon( p );
		return;

	default:
		cerror( "illegal conput" );
	}
}

/*
 * put out an explicit length qualifier.  This simplifies
 * handling of offsets in the 68020 assembler somewhat.
 */
offsize(p)
	register NODE *p;
{
	if ( p->tn.name[0] != '\0' || p->tn.lval > 32767
	    || p->tn.lval < -32768 ) {
		printf(":l");
	}
}

oregput(p)
	register NODE *p;
{
	register r;

	r = p->tn.rval;
	if (R2TEST(r)) {
		/*
		 * double indexing
		 */
		if (use68020) {
			/*
			 * For 68020, R2UPK3 encodes the following data:
			 *	int shortx:1; short index
			 *	int ibit:1; memory indirect mode (ignored)
			 *	int scale:4; scale factor (1,2,4,8)
			 */
			register flags;
			int base, index, scale, shortx, indirect;

			base = R2UPK1(r);
			index = R2UPK2(r);
			markused(base);
			markused(index);
			flags = R2UPK3(r);
			R2UPKFLGS(flags,shortx,indirect,scale);
			/*
			 * print base register and displacement (if any)
			 */
			printf(rnames[base]);
			printf("@(");
			acon(p);	      /* base displacement */
			offsize(p);	      /* size of displacement */
			putchar(',');
			/*
			 * print index register and scale factor.  For now,
			 * we assume all indices have been long-extended.
			 */
			printf(rnames[index]);
			putchar(':');
			putchar(shortx? 'w' : 'l');
			switch(scale) {
			case 0:
			case 1:
				break;
			case 2:
			case 4:
			case 8:
				printf(":%d",scale);
				break;
			default:
				cerror("illegal scale factor (%d)", scale);
				/*NOTREACHED*/
			}
			putchar(')');
		} /* use68020 */ 
		else {
			/*
			 * For the 68010, R2UPK3 encodes only whether
			 * a short index is used.
			 */
			int base, index, flags, shortx, ignored;
			base = R2UPK1(p->tn.rval);
			index = R2UPK2(p->tn.rval);
			flags = R2UPK3(p->tn.rval);
			R2UPKFLGS(flags,shortx,ignored,ignored);
			printf( "%s@(%d,%s:%c)",
				rnames[base],			/* breg */
				p->tn.lval,			/* disp */
				rnames[index], 			/* xreg */
				shortx ? 'w' : 'l' );		/* xlen */
			markused(base);
			markused(index);
		}
	} 
	else {
		int base = R2UPK2(p->tn.rval);
		markused(base);
		printf( rnames[base] );
		putchar('@');
		if( p->tn.lval != 0 || p->in.name[0] != '\0' ) { 
			putchar('('); 
			acon( p ); 
			if (use68020) offsize( p );
			putchar(')'); 
		} /* if */
	} /* else */
} /* oregput */

void
upput( p ) register NODE *p; 
{
	/* output the address of the second word in the
	   pair pointed to by p (for LONGs)*/
	CONSZ save;
	int r;

	if( p->in.op == FLD ){
		p = p->in.left;
	}

	save = p->tn.lval;
	switch( p->in.op ){

	case NAME:
		p->tn.lval += SZINT/SZCHAR;
		acon( p );
		break;

	case FCON:
		fcon( p, F_loworder );
		break;

	case ICON:
		/* addressable value of the constant */
		p->tn.lval &= BITMASK(SZINT);
		putchar('#');
		acon( p );
		break;

	case REG:
		r = p->tn.rval+1;
		if (r >= FP0) {
			cerror("illegal floating register pair");
		}
		print_str(rnames[r] );
		markused(r);
		break;

	case OREG:
		p->tn.lval += SZINT/SZCHAR;
		if( R2UPK2(p->tn.rval) == A6 ){  /* in the argument region */
		    if( p->in.name[0] != '\0' ) werror( "bad arg temp" );
		} 
#ifdef FREETEMP
		else if( R2UPK2(p->tn.rval) == SP ){  /* in the temp region */
		    if( p->in.name[0] != '\0' ) werror( "bad arg temp" );
		    printf( "sp@(LT%d+0x%x)", ftnno, toff+p->tn.lval);
		    p->tn.lval = save;
		    return;
		} 
#endif FREETEMP
		oregput(p);
		break;

	case UNARY MUL:
		if (p->in.left->in.op==INCR){
		    /* rewrite a-la adrput */
		    NODE *q, *l; int i;
		    l = p->in.left;
		    q = l->in.left;
		    p->in.op = OREG;
		    p->in.rall = q->in.rall;
		    p->tn.lval = q->tn.lval;
		    p->tn.rval = q->tn.rval;
#ifndef FLEXNAMES
		    for( i=0; i<NCHNAM; i++ )
			    p->in.name[i] = q->in.name[i];
#else
		    p->in.name = q->in.name;
#endif
		    adrput( p );
		    putchar('+');
		    p->tn.lval -= l->in.right->tn.lval;
		    tfree( l );
		} else if ( tshape(p, STARNM) ) {
		    adrput( p->in.left );
		    print_str("@(4)");
		} else {
		    /*  p->in.left->in.op==ASG MINUS */
		    /* just put and let adrput rewrite -- VERY carefully */
		    adrput( p->in.left->in.left );
		    print_str( "@-");
		}
		break;

	default:
		cerror( "illegal upper address" );
		break;

		}
	p->tn.lval = save;
}

void
adrput( p ) register NODE *p; 
{
	/* the 68k code saves lval and restores after the switch */
	register int r;
	/* output an address, with offsets, from p */

	if( p->in.op == FLD ){
	    p = p->in.left;
	}
	switch( p->in.op ){

	case NAME:
		acon( p );
		return;

	case FCON:
		/*
		 * put out floating constant in hex.
		 * Do not use in coprocessor instructions;
		 * use floatimmed(p) instead.
		 */
		if (p->in.type == FLOAT) {
			putchar('#');
			fcon( p, F_hex );
		} else {
			fcon( p, F_highorder );
		}
		return;

	case ICON:
		/* addressable value of the constant */
		putchar('#');
		acon( p );
		return;

	case REG:
		r = p->tn.rval;
		print_str(rnames[r] );
		markused(r);
		return;

	case OREG:
		if( R2UPK2(p->tn.rval) == A6 ){  /* in the argument region */
		    if( p->in.name[0] != '\0' ) werror( "bad arg temp" );
		} 
#ifdef FREETEMP
		else if( R2UPK2(p->tn.rval) == SP ){  /* in the temp region */
		    if( p->in.name[0] != '\0' ) werror( "bad arg temp" );
		    printf( "sp@(LT%d+%d)", ftnno, toff+p->tn.lval);
		    return;
		}
#endif FREETEMP
		oregput(p);
		break;

	case UNARY MUL:
		/* STARNM or STARREG found */
		if( tshape(p, STARNM) ) {
			adrput( p->in.left);
			putchar('@');
		}
		else {	/* STARREG - really auto inc or dec */
			/* turn into OREG so replacement node will
			   reflect the value of the expression */
			register i;
			register NODE *q, *l;

			l = p->in.left;
			q = l->in.left;
			if (p->in.type==DOUBLE )
			    if (l->in.op==INCR){
				/* just do it and let upput rewrite */
				adrput( q );
				print_str( "@+");
				return;
			    }
			    /* else fall through and we rewrite -- VERY carefully */
			p->in.op = OREG;
			p->in.rall = q->in.rall;
			p->tn.lval = q->tn.lval;
			p->tn.rval = q->tn.rval;
#ifndef FLEXNAMES
			for( i=0; i<NCHNAM; i++ )
				p->in.name[i] = q->in.name[i];
#else
			p->in.name = q->in.name;
#endif
			if( l->in.op == INCR ) {
				adrput( p );
				putchar('+');
				p->tn.lval -= l->in.right->tn.lval;
			} else {	
				/* l->in.op == ASG MINUS */
				adrput( p );
				putchar('-');
			}
			tfree( l );
		}
		return;

	case SCONV:
		adrput(p->in.left);	/* kludge for SFLOAT_SRCE */
		break;

	default:
		cerror( "illegal address" );
		return;

	}

}

acon( p ) register NODE *p; 
{ /* print out a constant */

	if( p->in.name[0] == '\0' ){
		print_x(p->tn.lval);
		}
	else if( p->tn.lval == 0 ) {
#ifndef FLEXNAMES
		printf( "%.8s", p->in.name );
#else
		print_str(p->in.name );
#endif
		}
	else {
#ifndef FLEXNAMES
		printf( "%.8s+", p->in.name );
#else
		print_str(p->in.name ); putchar('+');
#endif
		print_x(p->tn.lval );
		}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * fcon(p, form):  output a floating point constant in one of several forms
 */

static int fgetlab();

static char floatfmt[] = "0r%.9e";
static char doublefmt[] = "0r%.17e";

static void
fcon( p, form )
	register NODE *p;
	Floatform form;
{
	float x;
	long *lp;

	switch(form) {
	case F_highorder:
		/*
		 * put out a reference to the highorder word of a
		 * double constant from the pool
		 */
		printf("L%d", fgetlab(p));
		return;
	case F_loworder:
		/*
		 * put out a reference to the loworder word of a
		 * double constant from the pool
		 */
		printf("L%d+4", fgetlab(p));
		return;
	case F_eformat:
		/*
		 * put out a constant in e-floating point format;
		 * for coprocessor instructions.
		 */
		if (p->in.type == FLOAT) {
			printf(floatfmt, p->fpn.dval);
		} else {
			printf(doublefmt, p->fpn.dval);
		}
		return;
	case F_hex:
		/*
		 * put out the constant as one or two hex words;
		 * for initialization.
		 */
		if (p->in.type == FLOAT) {
			float x = p->fpn.dval;
			lp = (long*)&x;
			printf("0x%x", lp[0]);
		} else {
			lp = (long*)&p->fpn.dval;
			printf("0x%x,0x%x", lp[0], lp[1]);
		}
		return;
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * floating point constant pool -- a data structure that
 * associates floating point constants with labels
 */

struct floatcon {
	int conlab;
	TWORD contype;
	double conval;
	struct floatcon *left, *right;
};

static struct floatcon *fconpool;	/* the search tree */
static struct floatcon *newfloatcon();	/* floatcon storage allocator */
static int fconlookup();		/* search routine */

/*
 * search for, and create if necessary,
 * a label in the floating point constant pool
 */

static int
fgetlab(p)
	NODE *p;
{
	return(fconlookup(&fconpool, p));
}

static int
fconlookup(fpp, p)
	struct floatcon **fpp;
	NODE *p;
{
	struct floatcon *fp = *fpp;
	if (fp == NULL) {
		*fpp = fp = newfloatcon();
		fp->conval = p->fpn.dval;
		fp->contype = p->fpn.type;
		fp->conlab = getlab();
		fp->left = NULL;
		fp->right = NULL;
		return(fp->conlab);
	} else if (fp->conval == p->fpn.dval) {
		return(fp->conlab);
	} else if (fp->conval < p->fpn.dval) {
		return(fconlookup(&fp->left, p));
	} else {
		return(fconlookup(&fp->right,p));
	}
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

/*
 * storage allocation for the floating point constant pool
 */

#define FCONSEGSIZ 256
struct fconseg {
	struct fconseg  *nextseg;
	struct floatcon *nextcon;
	struct floatcon storage[FCONSEGSIZ];
};

static struct fconseg *curseg = NULL;

/*
 * allocate storage for a
 * labeled floating point constant
 */
static struct floatcon *
newfloatcon()
{
	struct fconseg *newseg;

	if (curseg == NULL
	    || curseg->nextcon == curseg->storage+FCONSEGSIZ) {
		newseg = (struct fconseg*)malloc(sizeof(struct fconseg));
		newseg->nextseg = curseg;
		curseg = newseg;
		curseg->nextcon = curseg->storage;
	}
	return(curseg->nextcon++);
}

/*
 * emit the floating point constant
 * pool to the output stream
 */
static void
dumpfcons()
{
	register struct floatcon *fp,*nextfree;
	struct fconseg *segp, *temp;
	long *lp;

	segp = curseg;
	while(segp != NULL) {
		nextfree = segp->nextcon;
		for (fp = segp->storage; fp < nextfree; fp++) {
			printf("L%d:	.long	", fp->conlab);
			if (fp->contype == FLOAT) {
				float x = fp->conval;
				lp = (long*)&x;
				printf("0x%lx", lp[0]);
			} else {
				lp = (long*)&fp->conval;
				printf("0x%lx,0x%lx", lp[0], lp[1]);
			}
			putchar('\n');
		}
		temp = segp->nextseg;
		free(segp);
		segp = temp;
	}
	curseg = NULL;
	fconpool = NULL;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -*/

genscall( p, cookie ) register NODE *p; 
{
	/* structure valued call */
	return( gencall( p, cookie ) );
}

gencall( p, cookie ) register NODE *p;
{
	/* generate the call given by p */
	register args;
	register m;

	if( p->in.right ) args = argsize( p->in.right );
	else args = 0;

	if( p->in.right ){ /* generate args */
		int residue = args % (SZLONG/SZCHAR);
		if (residue) {
			/*
			 * pad the argument list out to an
			 * even number of longwords.  This is
			 * the wrong way to do it, but doing
			 * it right isn't backwards compatible.
			 */
			printf("	subqw	#0x%x,sp\n", residue);
		}
		genargs( p->in.right );
	}

	if( !shltype( p->in.left->in.op, p->in.left ) ) {
		order( p->in.left, INBREG|SOREG );
	}

	p->in.op = UNARY CALL;
	m = match( p, INTAREG|INTBREG );
	popargs( args );
	return(m != MDONE);
}

popargs( size ) register size; 
{
	/* pop arguments from stack */

	if (size==0) return;
	/*
	 * round size up to a multiple of sizeof(long),
	 * to compensate for the hack in gencall() above.
	 */
	SETOFF(size, (SZLONG/SZCHAR));
	toff -= size;
	/* huh? if( toff == 0 && size >= 2 ) size -= 2; */
	if (size <=8 )
		printf( "	addqw	#0x%x,sp\n", size);
	else if (size <(1<<15))
		printf( "	lea	sp@(0x%x),sp\n", size);
	else
		printf( "	addl	#0x%x,sp\n", size);
}


nextcook( p, cookie ) 
	NODE *p; 
	register cookie;
{
	/* we have failed to match p with cookie; try another */
	if ( cookie == FORREW ) return( 0 );  /* hopeless! */
	if ( cookie & (INCREG|INTCREG) ) return (INTAREG);
	if ( !(cookie&(INTAREG|INTBREG|INTCREG)) )
		return( INTAREG|INTBREG|INTCREG );
	if ( !(cookie&INTEMP) && asgop(p->in.op) ) 
		return( INTEMP|INAREG|INTAREG|INTBREG|INBREG);
	if ( !(cookie&(INTAREG|INAREG)) && p->in.op == CHK )
		return(INTAREG|INAREG);
	return ( FORREW );
}

lastchance( p, cook ) NODE *p; 
{
	/* forget it! */
	return(0);
}


NODE * addroreg(l)
/*
 * OREG was built in clocal()
 * for an auto or formal parameter
 * now its address is being taken
 * local code must unwind it
 * back to PLUS/MINUS REG ICON
 * according to local conventions
 */
{
	cerror("address of OREG taken");
}



# ifndef ONEPASS
main( argc, argv ) char *argv[]; 
{
	int v;
	if ( strcmp(argv[0],pcname) == 0 )
		pascal = 1;
	v = mainp2( argc, argv );
	floatnote();
	exit( v );
}
# endif

/* return 1 if node is a SCONV from short to int */
isconv( p, t1, t2 )
    register NODE *p;
    TWORD t1, t2;
{
    register v;
    if ( p->in.op==SCONV && (p->in.type==INT || p->in.type==UNSIGNED) && 
	((v=p->in.left->in.type)==t1 || v == t2))
	      return( 1 );
	return( 0 );
}



myreader(p) register NODE *p; 
{
#ifdef FORT
	void unoptim2();
	if (!pascal) {
		unoptim2(p);
	}
#endif FORT
	optim2(p);
	canon( p );		/* expands r-vals for fields */
	toff = 0;  /* stack offset swindle */
#ifdef FREETEMP
	tmpoff = 0; /* we number temp regsters differently here */
#endif
}


special( p, shape ) register NODE *p; 
{
	/* special shape matching routine */

	switch (shape){
	case SPEC_FLD:
	    /* do-it-yourself multi-level matching */
	    if (tshape(p,SFLD))
		switch( p->in.left->tn.op){
		case OREG:
			/* if (R2TEST(p->in.left->tn.rval)) return 0; */
		case REG:
		case ICON:
		case NAME:
			return(1);
		}
		return 0;

	case SPEC_FLT:
	    /* depends on phase of moon */
	    if (usesky)
		return tshape(p,SCON|SAREG|STAREG|SBREG|SOREG|SNAME|STARREG);
	    else
		return tshape(p,SAREG|STAREG) && (p->tn.rval==D0 || p->tn.rval==D1);
	case SPEC_DFLT:
	    /* likewise */
	    if (usesky)
		return tshape(p,SCON|SAREG|STAREG|SBREG|SOREG|SNAME|STARREG);
	    else
		return tshape(p,SCON|			SOREG|SNAME|STARREG);

	case SFLOAT_SRCE:
	    /* source operand of 68881 floating point instruction */
	    if (p->in.op == SCONV) {
		/*
		 * the 68881 converts signed integral types into
		 * (extended) floating point format
		 */
		p = p->in.left;
		switch(p->in.type) {
		case CHAR:
		case SHORT:
		case INT:
		case LONG:
		case FLOAT:
		    /* d-registers, constants, and memory are all ok */
		    return tshape(p,
			SCON|SCREG|STCREG|SAREG|STAREG|SOREG|SNAME|STARNM|STARREG);
		case DOUBLE:
		    /* can't deal with register pairs */
		    return tshape(p,
			SCON|SCREG|STCREG|SOREG|SNAME|STARNM|STARREG);
		default:
		    return(0);
		}
	    }
	    if (p->in.type == FLOAT) {
		/* d-regs, f-registers, or memory */
		return tshape(p,
		    SCON|SCREG|STCREG|SAREG|STAREG|SOREG|SNAME|STARNM|STARREG);
	    }
	    /* type == DOUBLE: f-registers or memory only */
	    return tshape(p, SCON|SCREG|STCREG|SOREG|SNAME|STARNM|STARREG);

	case SPEC_PVT:
	    /* right subtree of x + y*z ; an idiom from matrix computations */
	    if (!usesky) return 0;
	    if (p->in.op != MUL) return 0;
	    if (tshape(p->in.left,SCON|SAREG|STAREG|SBREG|SOREG|SNAME|STARREG)){
		return tshape(p->in.right,
		    SCON|SAREG|STAREG|SBREG|SOREG|SNAME|STARREG);
	    }
	    return 0;

	case SSOREG:
	    /* someday soon, this will be useful */
	    return ( p->in.op == OREG && !R2TEST(p->tn.rval) );

	case SBASE:
	    /* half of a double-oreg */
	    return ( p->in.op == PLUS
		&& p->in.left->in.op == REG && isbreg(p->in.left->tn.rval)
		&& (p=p->in.right)->in.op == ICON && p->tn.name[0] == '\0'
		&& p->tn.lval >= -127 && p->tn.lval <= 127 );

	case SXREG:
	    /* the other half */
	    if ( p->in.op==SCONV
		&& (p->in.type == INT || p->in.type == UNSIGNED)
		&& p->in.left->in.type == SHORT )
		      p = p->in.left;
	    return ( p->in.op == REG );

	case SNONPOW2:
	    /*
	     * constant, NOT a power of 2.  This is a kludge to enable
	     * us to match a template for ASG MOD on the 68020.  The
	     * semantics of the DIVSLL instruction make it impossible
	     * to return a remainder in the lhs of <reg> %= <ea>, unless
	     * an extra instruction is generated. Returning the result
	     * in RESC1 makes the extra instruction unnecessary.
	     */
	    return( p->in.op == ICON && (p->tn.lval & p->tn.lval-1) );

	case SAUTOINC:
	    return( p->in.op == UNARY MUL
		&& p->in.left->in.op == INCR
		&& shumul(p->in.left) == STARREG );

	/*
	 *  CHK bounds pair with constant lower bound of 0
	 */
	case SZEROLB:
	    return( p->in.op == CM
		&& p->in.left->in.op == ICON
		&& p->in.left->tn.name == NULL
		&& p->in.left->tn.lval == 0 );

	case SFZERO:
	    return( p->in.op == FCON && p->fpn.dval == 0.0 );

	}

	if(p->tn.op != ICON || p->in.name[0] != '\0') return (0);
	switch( shape ) {
	case SCCON:
		return( p->tn.lval>= -128 && p->tn.lval <=127 );
	case SICON:
		return( p->tn.lval>= 0 && p->tn.lval <=32767 );
	case SSCON:
		return( p->tn.lval>= -32768 && p->tn.lval <=32767 );
	case S8CON:
		return( p->tn.lval>= 1 && p->tn.lval <=8 );
	default:
		cerror( "bad special shape" );
	}

	return( 0 );
}

# ifdef MULTILEVEL
# include "mldec.h"

struct ml_node mltree[] ={

DEFINCDEC,	INCR,	0,
	INCR,	SANY,	TANY,
		OPANY,	SAREG|STAREG,	TANY,
		OPANY,	SCON,	TANY,

DEFINCDEC,	ASG MINUS,	0,
	ASG MINUS,	SANY,	TANY,
		REG,	SANY,	TANY,
		ICON,	SANY,	TANY,

TSOREG,	1,	0,
	UNARY MUL,	SANY,	TANY,
		REG,	SANY,	TANY,

TSOREG,	2,	0,
	UNARY MUL,	SANY,	TANY,
		PLUS,	SANY,	TANY,
			REG,	SANY,	TANY,
			ICON,	SANY,	TCHAR|TUCHAR|TSHORT|TUSHORT|TINT|TUNSIGNED|TPOINT,

TSOREG,	2,	0,
	UNARY MUL,	SANY,	TANY,
		MINUS,	SANY,	TANY,
			REG,	SANY,	TANY,
			ICON,	SANY,	TCHAR|TUCHAR|TSHORT|TUSHORT|TINT|TUNSIGNED|TPOINT,
0,0,0};
# endif

#ifdef FREETEMP
extern unsigned offsz;

freetemp( k )
{ 
    /* allocate k integers worth of temp space */
    /* we also make the convention that, if the number of words is more than 1,
    /* it must be aligned for storing doubles... */
    /* allocate temps forwards from sp+(saved registers) */

	int t;

	if( k>1 ){
		SETOFF( tmpoff, ALDOUBLE );
	}
	t = tmpoff;
	tmpoff += k*SZINT;
	if( tmpoff >= offsz )
		cerror( "stack overflow" );
	if( tmpoff > maxtemp ) maxtemp = tmpoff ;
	return(t);

}
#endif FREETEMP

#ifdef FORT
void
unoptim2( p ) register NODE *p;
{
    /* FORTRAN thinks we can do double operations on float operands */
    NODE * double_conv();
    switch( optype(p->in.op)){
    case BITYPE:
	unoptim2(p->in.left);
	unoptim2(p->in.right);
	if (p->in.type == DOUBLE){
	    switch (p->in.op) {
	    case QUEST:
	    case CM:
	    case COMOP:
	    case CALL:
		    return;
	    }
	    if (p->in.right->in.type != DOUBLE)
		p->in.right = double_conv(p->in.right);
	    if (p->in.left->in.type != DOUBLE)
		p->in.left = double_conv(p->in.left);
	    return;
	}
	if ( logop(p->in.op) ){
	    if (p->in.left->in.type==DOUBLE && p->in.right->in.type != DOUBLE)
		p->in.right = double_conv(p->in.right);
	    if (p->in.right->in.type==DOUBLE && p->in.left->in.type != DOUBLE)
		p->in.left = double_conv(p->in.left);
	}
	return;
    case UTYPE:
	unoptim2(p->in.left);
	if (p->in.type == DOUBLE ){
	    switch( p->in.op ){
	    case UNARY MUL:
	    case UNARY CALL:
	    case SCONV:
		return;
	    }
	    if (p->in.left->in.type != DOUBLE)
		p->in.left = double_conv(p->in.left);
	}
	return;
    case LTYPE: return;
    }
}
#endif FORT

