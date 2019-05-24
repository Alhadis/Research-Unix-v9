#ifndef lint
static	char sccsid[] = "@(#)float2.c 1.1 86/02/03 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 *	float2.c
 */

#include "cpass2.h"
#include "sky.h"
#define iscnode(x) (x->tn.op==REG && iscreg(x->tn.rval))

static skyused = 0;
int skybase = 0;


#define FADD	0
#define FADDS	1
#define FSUB	2
#define FSUBS	3
#define FMUL	4
#define FMULS	5
#define FDIV	6
#define FDIVS	7
#define FMOD	8
#define FMODS	9
#define FITOD	10
#define FITOS	11
#define FSTOD	12
#define FDTOS	13
#define FSTOI	14
#define FDTOI	15
#define FCMP	16
#define FCMPS	17
#define FPVT	18
#define FPVTS	19

extern NODE * double_conv();

ezsconv( p )
    register NODE *p;
{
    /* return 1 if *p is an SCONV that does not require run-time support */

    if (use68881)
	return(1);
    switch (p->in.type){
    case FLOAT:
    case DOUBLE:
	return 0;
    default:
	switch (p->in.left->in.type){
	case FLOAT:
	    /* ???p->in.left = double_conv( p->in.left ); */
	    /* fall through */
	case DOUBLE:
	    return 0;
	default: return 1;
	}
    }
}

static void
rightreg( l, r, d)
    NODE *l, *r, *d;
{
    if (usesky) return;
    if (l->in.op!=REG || (r && r->in.type==FLOAT && r->in.op!=REG))
	cerror("misplaced floating argument\n");
    if (l->tn.rval != D0){
	if (r && r->in.op==REG && r->tn.rval == D0){
	    if (l->tn.rval == D1){
		print_str("	exg	d0,d1\n");
		l->tn.rval = D0;
		r->tn.rval = D1;
		return;
	    } else {
		r->in.rall = MUSTDO|D1;
		order( r, INTAREG );
	    }
	}
	l->in.rall = MUSTDO|D0;
	order( l, INTAREG );
    }
    if (r && r->in.type==FLOAT && r->tn.rval != D1 ){
	r->in.rall = MUSTDO|D1;
	order( r, INTAREG );
    } else if (r && r->in.type==DOUBLE)
	if (tshape( r, SNAME|SOREG|SCON|STARNM))
	    expand( r, FOREFF, "	lea	A.,a0\n" );
	else {
	    /* must be *dp++ or *--dp. Horrors!! */
	    if ((l=r->in.left)->in.op == ASG MINUS)
		expand( l->in.left, FOREFF, "	subqw	#8,A.\n");
	    expand( l->in.left, INTBREG, "	movl	A.,a0\n");
	    if (l->in.op == INCR)
		expand( l->in.left, FOREFF, "	addqw	#8,A.\n");
	}
    if (d!=NULL){
	/* must be d0 */
	if (d->in.op!=REG || d->tn.rval!=D0)
	    cerror("misplaced floating argument\n");
    }
}

struct fopnames {
	char *opcode;
	char *fvroutine;
} fopnames[] = {
	"fabs", 	"abs",
	"fcos", 	"cos",
	"fsin", 	"sin",
	"ftan", 	"tan",
	"facos", 	"acos",
	"fasin", 	"asin",
	"fatan", 	"atan",
	"fcosh", 	"cosh",
	"fsinh", 	"sinh",
	"ftanh", 	"tanh",
	"fetox", 	"exp",
	"ftentox", 	"pow10",
	"ftwotox", 	"pow2",
	"flogn", 	"log",
	"flog10", 	"log10",
	"flog2", 	"log2",
	"", 		"sqr",
	"fsqrt", 	"sqrt",
	"fintrz", 	"aint",
	"", 		"anint",
	"",		"nint",
};

char
floatprefix()
{
	extern usesky, usefpa, use68881, useswitch;

	if (usefpa) return 'W';
	if (use68881) return 'M';
	if (useswitch) return 'V';
	if (usesky) return 'S';
	return 'F';
}

/*
 * load floating point result register A1 with operand q.
 * opcode is the mnemonic for a 68881 instruction that
 * accepts a memory, d-reg, or fp-reg source operand.
 */
fpload(q, cookie, opcode)
	register NODE *q;
	char *opcode;
{
	if ( q->in.op == REG && q->in.type == DOUBLE
	    && isareg(q->tn.rval) ) {
		/* register pair */
		expand(q, cookie,
		    "	movl	U.,sp@-\n	 movl	A.,sp@-\n");
		printf("	%s", opcode);
		expand(q, cookie, "ZF	sp@+,A1\n");
	} else {
		printf("	%s", opcode);
		expand(q, cookie, "ZF	A.,A1\n");
	}
}

floatcode( p, cookie )
    register NODE *p;
{
    /*
     * we mean to emit floating-point code. figure out what we want to do,
     * figure out who we are, and do the right thing.
     */
     register o, ty;
     register NODE *q, *r;
     NODE *d;
     NODE *x;
     int temp, m;
     int fname;

     ty = p->in.type;
     q  = p->in.left;
     r  = p->in.right;
     o  = p->in.op;

    d = &resc[0];
    x = NIL;
    switch (o){
    case ASG PLUS:	d = q;
    case     PLUS:	fname = FADD; 
			if (usesky && r->in.op == MUL) {
			    fname = FPVT;
			    x = q;
			    q = r->in.left;
			    r = r->in.right;
			}
			goto asop;
    case ASG MINUS:	d = q;
    case     MINUS:	fname = FSUB; 
			goto asop;
    case ASG MUL:	d = q;
    case     MUL:	fname = FMUL;
			goto asop;
    case ASG MOD:	d = q;
    case     MOD:	fname = FMOD; 
			goto asop;
    case ASG DIV:	d = q;
    case     DIV:	fname = FDIV;
	    asop:
			/* d0 op= d1  */
			/* d0 op= *a0 */
			/* and, for sky case */
			/* AWD op= AWD */
			/* or 
			/* AWD = AWD op AWD */
			if (p->in.type == FLOAT )
			    fname += 1;
			rightreg( q, r, d );
			floatop( fname, r, q, x, d );
			return;

    case REG:
    case NAME:
    case FCON:
    case OREG:
    case UNARY MUL:
		if (cookie&FORARG) {
		    /* float FORARG -- must convert */
#ifdef FLOATMATH
		    if (FLOATMATH>1 && ty == FLOAT){
			if (o == REG && iscreg(p->tn.rval)) {
			    /* 68881 coprocessor register */
			    expand( p, FORARG,"	fmoves	AR,Z-\n");
			} else {
			    expand( p, FORARG,"	movl	AR,Z-\n");
			}
			return;
		    }
#endif FLOATMATH
		    if (o == FCON) {
			p->in.type = DOUBLE;
		    } else {
			p = double_conv(p);
		    }
		    order( p, FORARG );
		    /* expand( p, FORARG,"	movl	UR,Z-\n	movl	AR,Z-\n"); */
		    return;
		}

		/* FORCC */
		m = getlab() ;
		expand( p, cookie, "	cmpl	#0x80000000,AL\n");
		print_str_d_nl( "	jeq	L", m );
		q = (o == UNARY MUL? q : p );
		if ( q->in.op == REG && isbreg(q->tn.rval) && !use68020 ) {
			/* can't tst an a-register with last year's model */
			expand( p, cookie, "	cmpw	#0,AL\n");
		} else {
			expand( p, cookie, "	tstl	AL\n");
		}
		if ( ty == DOUBLE ){
		    temp = m; m = getlab();
		    printf( "	jne	L%d\nL%d:", m, temp);
		    expand( p, cookie, "	tstl	UL\n");
		}
		print_label(m);
		return;
	/* end of case EA */

    case UNARY MINUS:
	/* negation of a floating-point (or double) quantity */
/*	expand( p, cookie, "	eorl	#0x80000000, AL\n" ); */
	expand( p, cookie, (q->tn.op==REG)? "	bchg	#31,AL\n" 
					  : "	bchg	#7,AL\n" );
	return;
    case LT:
    case LE:
    case GT:
    case GE:
    case EQ:
    case NE:
	fname = FCMP;
	if (q->in.type==FLOAT)
	    fname += 1;
	rightreg( q, r, NULL );
	floatop( fname, r, q, NIL, NIL );
	return;

	/* fortran intrinsics -- only unary ops for now */
    case FABS:
    case FCOS:
    case FSIN:
    case FTAN:
    case FACOS:
    case FASIN:
    case FATAN:
    case FCOSH:
    case FSINH:
    case FTANH:
    case FEXP:
    case F10TOX:
    case F2TOX:
    case FLOG10:
    case FLOG2:
    case FAINT:
	/*
	 * source in memory, d-regs, or fp-regs
	 * result in fp-reg A1
	 */
	if (use68881) {
		/* done in a single instruction by the 68881 */
		fpload(q, cookie, fopnames[o-FABS].opcode);
		return;
	}
	goto call_intrinsic;

    case FSQR:		/* sqr(x) = x*x */
	/*
	 * source in memory, d-regs, or fp-regs
	 * result in fp-reg A1
	 */
	if (use68881) {
		fpload(q, cookie, "fmove");
		expand(q, cookie, "	fmulx	A1,A1\n");
		return;
	}
	goto call_intrinsic;

    case FANINT:	/* float biased round towards zero */
	/*
	 * source in memory, d-regs, or fp-regs
	 * result in fp-reg A1
	 */
 	if (use68881) {
		fpload(q, cookie, "fmove");
		p->in.op = PLUS;	/* just so getlr will work */
		r = talloc();
		p->in.right = r;
		r->in.op = FCON;
		r->fpn.type = q->in.type;
		r->fpn.dval = 0.5;
 		expand(p, cookie, "	ftestx	A1\n");
 		expand(p, cookie, "	fjlt	1f\n");
 		expand(p, cookie, "	faddZF	ZK,A1\n");
 		expand(p, cookie, "	jra	2f\n");
 		expand(p, cookie, "1:	fsubZF	ZK,A1\n");
		expand(q, cookie, "2:	fintrzx	A1,A1\n");
 		return;
 	}
 	goto call_intrinsic;

    case FNINT:		/* int biased round towards zero */
	/*
	 * This one is different:
	 * source in fp-reg, result in d-reg A1
	 */
 	if (use68881) {
		if (!istnode(q) || !iscreg(q->tn.rval)) {
			order(q, INTCREG);
		}
		p->in.op = PLUS;	/* just so getlr will work */
		r = talloc();
		p->in.right = r;
		r->in.op = FCON;
		r->fpn.type = q->in.type;
		r->fpn.dval = 0.5;
 		expand(p, cookie, "	ftestx	AL\n");
 		expand(p, cookie, "	fjlt	1f\n");
 		expand(p, cookie, "	faddZF	ZK,AL\n");
 		expand(p, cookie, "	jra	2f\n");
 		expand(p, cookie, "1:	fsubZF	ZK,AL\n");
		expand(q, cookie, "2:	fintrzx	AL,AL\n");
		expand(p, cookie, "	fmovel	AL,A1\n");
 		return;
 	}
 	goto call_intrinsic;

    case FLOGN:
	/*
	 * KLUDGE to work around a bug in the A79J (RevC) mask set.
	 * In the first production masks of the 68881, flogn does
	 * not work correctly for (0.5 <= x <= 1.0).  The following
	 * work-around is required as long as some of the defective
	 * chips are in the field:
	 */
	if (use68881) {
		fpload(q, cookie, "fmove");
		expand(q, cookie, "	fcmps	#0r0.5,A1\n");
		expand(q, cookie, "	fjule	1f\n");
		expand(q, cookie, "	fsubl	#1,A1\n");
		expand(q, cookie, "	flognp1x A1,A1\n");
		expand(q, cookie, "	jra	2f\n");
		expand(q, cookie, "1:	flognx	A1,A1\n");
		expand(q, cookie, "2:");
		return;
	}
	goto call_intrinsic;

    case FSQRT:
	/*
	 * KLUDGE to work around a bug in the A79J (RevC) mask set.
	 * In the first production masks of the 68881, fsqrt fpx,fpy
	 * does not work reliably when fpx and fpy are different
	 * registers.
	 */
	if (use68881) {
		if (q->in.op == REG && iscreg(q->tn.rval)
		    && q->tn.rval != resc[0].tn.rval) {
			fpload(q, cookie, "fmove");
			expand(q, cookie, "	fsqrtx	A1,A1\n");
		} else {
			fpload(q, cookie, "fsqrt");
		}
		return;
	}
	goto call_intrinsic;
	
    call_intrinsic:
	rightreg(q, NULL, NULL);
	printf("	jsr	%c%s%c\n",
		floatprefix(),
		fopnames[o-FABS].fvroutine,
		(q->in.type == FLOAT ? 's' : 'd'));
	return;

    default:
huh:
	print_str("HEY! cookie = ");
	prcook( cookie);
	putchar('\n');
	fwalk( p, eprint, 0);
	cerror("floatcode got a tree it didn't expect");
    }

}



floatconv( p, c, cookie )
    register NODE *p;
    char c;
{
    int ty = p->in.type;
    NODE *q = p->in.left;
    register t2 = q->in.type;
    int fname;

    switch( ty ){
    case FLOAT:
	switch (t2){
	case FLOAT:  return;
	case DOUBLE: fname = FDTOS; break;
	default:     fname = FITOS; makeint( q, cookie ); break;
	}
	break;
    case DOUBLE:
	switch (t2){
	case DOUBLE: return;
	case FLOAT:  fname = FSTOD; break;
	default:     fname = FITOD;    makeint( q, cookie ); break;
	}
	break;
    default:
	switch(t2){
	case DOUBLE: fname = FDTOI; break;
	case FLOAT:  fname = FSTOI; break;
	default: return;
	}
    }
    rightreg( q, NULL, NULL );
    floatop( fname, NIL, q, NIL, &resc[0] );

}

makeint( p, cookie )
    NODE *p;
{
    switch( p->tn.type ){
	    case UCHAR:
		expand( p, cookie, "	andl	#0xff,AL\n" );
		return;
	    case USHORT:
		expand( p, cookie, "	andl	#0xffff,AL\n" );
		return;
	    case CHAR:
		expand( p, cookie, "	extw	AL\n" );
		/* fall through */
	    case SHORT:
		expand( p, cookie, "	extl	AL\n" );
		return;
	    default: 
		;
    }
}

/*
 * Note: the names below are INCOMPATIBLE with pre-3.0 libraries
 */
struct finfo {
	char * fname;
	short  skycode;
	char   nlsource; /* source from left */
	char   nrsource; /* source from right    */
	char   nxsource; /* 3rd source, from wherever */
	char   ndest;    /* size of sink	  */
	char   delay;	 /* need busy-wait loop before collecting results? */
} finfo [] = {
	{ "addd",  S_DADD3, 2, 2, 0, 2, 1}, /* fadd */
	{ "adds",  S_SADD3, 1, 1, 0, 1, 0}, /* fadds */
	{ "subd",  S_DSUB3, 2, 2, 0, 2, 1}, /* fsub */
	{ "subs",  S_SSUB3, 1, 1, 0, 1, 0}, /* fsubs */
	{ "muld",  S_DMUL3, 2, 2, 0, 2, 1}, /* fmul */
	{ "muls",  S_SMUL3, 1, 1, 0, 1, 0}, /* fmuls */
	{ "divd",  S_DDIV3, 2, 2, 0, 2, 1}, /* fdiv */
	{ "divs",  S_SDIV3, 1, 1, 0, 1, 1}, /* fdivs */
	{ "modd",  0			 }, /* fmod */
	{ "mods",  0			 }, /* fmods */
	{ "fltd",  S_ITOD,  1, 0, 0, 2, 0}, /* fitod */
	{ "flts",  S_ITOS,  1, 0, 0, 1, 0}, /* fitos */
	{ "stod",  S_STOD,1, 0, 0, 2, 0},   /* fstod */
	{ "dtos",  S_DTOS,2, 0, 0, 1, 0},   /* fdtos */
	{ "ints",  S_STOI,  1, 0, 0, 1, 0}, /* fstoi */
	{ "intd",  S_DTOI,  2, 0, 0, 1, 0}, /* fdtoi */
	{ "cmpd",  S_DCMP3, 2, 2, 0,-1, 0}, /* fcmp */
	{ "cmps",  S_SCMP3, 1, 1, 0,-1, 0}, /* fcmps */
	{ "",	   S_DPVT3, 2, 2, 2, 2, 1}, /* fpvt */
	{ "",	   S_SPVT3, 1, 1, 1, 1, 1}, /* fpvts */
};

static void 
move_to_sky( p, n, skybreg )
    register NODE *p;
    int n;
    char *skybreg;
{
    /* special hackery for *--dp */
    if ( n==2 && p->in.op==UNARY MUL && p->in.left->in.op==ASG MINUS ){
	/* do --, rewrite as oreg */
	NODE *q; NODE *l;
	l = p->in.left; /* ASG MINUS node */
	q = l->in.left;
	expand( q, INBREG|INTBREG, "	subqw	#8,A.\n" );
	*p = *q;
	p->in.op = OREG;
	l->in.op = FREE;
	l->in.right->in.op = FREE;
	q->in.op = FREE;
    }
    expand( p, FOREFF, "	movl	A.,");
    printf( "%s@\n", skybreg );
    if (n >1) {
	expand( p, FOREFF, "	movl	U.,");
	printf( "%s@\n", skybreg );
    }
}

/* #define READY 1:tstw SKYBASE@(-OPERAND+STATUS) ; bges 1b
/*	STATUS	=	2
/*	OPERAND	=	4
*/

floatop( f, r, l, x, d ) register f; NODE *r, *l, *x, *d; 
{
    register j;
    register char *skybreg;
    if (usesky && (j=finfo[f].skycode)){
	if (skybase) {
	    /* reg reserved by optimizer; use it */
	    if (!isbreg(skybase)) {
		/* not an a-register, must move it */
		printf(	"	movl	%s,a1\n" );
		skybreg = "a1";
	    } else {
		/* reg ok as is */
		skybreg = rnames[skybase];
	    }
	} else {
	    /* must load __skybase into a1 */
	    printf( "	movl	__skybase,a1\n" );
	    skybreg = "a1";
	}
	printf( "	movw	#%#X,%s@(-4)\n", j, skybreg);
	if (j=finfo[f].nlsource)
	    move_to_sky( l, j, skybreg );
	if (j=finfo[f].nrsource)
	    move_to_sky( r, j, skybreg );
	if (j=finfo[f].nxsource)
	    move_to_sky( x, j, skybreg );
	if (finfo[f].delay){
	    printf( "1:	tstw	%s@(-2)\n", skybreg );
	    printf( "	bges	1b\n" );
	}
	j = finfo[f].ndest;
	if (j < 0){
	    printf( "	movw	%s@,cc\n", skybreg );
#ifndef IEEECCODES
	    printf( "	andb	#0xfd,cc\n");
#endif	IEEECCODES
	} else {
	    printf( "	movl	%s@,", skybreg );
	    expand( d, FOREFF, "A.\n" );
	    if (j >1) {
		printf( "	movl	%s@,", skybreg );
		expand( d, FOREFF, "U.\n" );
	    }
	}
	skyused++;
    } else {
	printf( "	jsr	%c%s\n", floatprefix(), finfo[f].fname);
	if (d && (d->tn.op!=REG || d->tn.rval!=D0) && (j=finfo[f].ndest)>0){
	    /* must rewrite destination as advertised */
	    expand( d, FOREFF, "	movl	d0,A.\n" );
	    if (j >1)
		expand( d, FOREFF, "	movl	d1,U.\n" );
	}
    }
}


floatnote()
{
	/* printf( "	.globl	ieeeused\n" ); *//* troublemaker */
	if (skyused)
	    printf("	.globl	fsky_used\n" );
	else if (use68881)
	    printf("	.globl	f68881_used\n" );
	else if (usefpa)
	    printf("	.globl	ffpa_used\n" );
}
